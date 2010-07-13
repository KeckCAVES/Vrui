/***********************************************************************
SpaceBall - Class for 6-DOF joysticks (Spaceball 4000FLX).
Copyright (c) 2002-2010 Oliver Kreylos

This file is part of the Vrui VR Device Driver Daemon (VRDeviceDaemon).

The Vrui VR Device Driver Daemon is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vrui VR Device Driver Daemon is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui VR Device Driver Daemon; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <VRDeviceDaemon/VRDevices/SpaceBall.h>

#include <Misc/Timer.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/GeometryValueCoders.h>

#include <VRDeviceDaemon/VRDeviceManager.h>

/**************************
Methods of class SpaceBall:
**************************/

int SpaceBall::readPacket(int packetBufferSize,unsigned char* packetBuffer)
	{
	/* Read characters until an end-of-line is encountered: */
	bool escape=false;
	int readBytes=0;
	while(readBytes<packetBufferSize-1)
		{
		/* Read next byte: */
		unsigned char byte=(unsigned char)(devicePort.readByte());

		/* Deal with escaped characters: */
		if(escape)
			{
			/* Process escaped character: */
			if(byte!='^') // Escaped circumflex stays
				byte&=0x1f;
			packetBuffer[readBytes]=byte;
			++readBytes;
			escape=false;
			}
		else
			{
			/* Process normal character: */
			if(byte=='^') // Circumflex is escape character
				escape=true;
			else if(byte=='\r') // Carriage return denotes end of packet
				break;
			else
				{
				packetBuffer[readBytes]=byte;
				++readBytes;
				}
			}
		}
	
	/* Terminate packet with ASCII NUL and return: */
	packetBuffer[readBytes]='\0'; 
	return readBytes;
	}

void SpaceBall::deviceThreadMethod(void)
	{
	/* Create free-running timer to estimate tracker velocities: */
	Misc::Timer timer;
	bool notFirstMeasurement=false;
	
	/* Receive lines from the serial port until interrupted: */
	while(true)
		{
		/* Read characters until an end-of-line is encountered: */
		unsigned char packet[256];
		readPacket(256,packet);
		
		/* Determine the packet type: */
		switch(packet[0])
			{
			case 'D':
				{
				Vrui::VRDeviceState::TrackerState ts;
				
				/* Parse a data packet: */
				short int rawData[6];
				rawData[0]=(short int)(((unsigned int)packet[ 3]<<8)|(unsigned int)packet[ 4]);
				rawData[1]=(short int)(((unsigned int)packet[ 5]<<8)|(unsigned int)packet[ 6]);
				rawData[2]=(short int)(((unsigned int)packet[ 7]<<8)|(unsigned int)packet[ 8]);
				rawData[3]=(short int)(((unsigned int)packet[ 9]<<8)|(unsigned int)packet[10]);
				rawData[4]=(short int)(((unsigned int)packet[11]<<8)|(unsigned int)packet[12]);
				rawData[5]=(short int)(((unsigned int)packet[13]<<8)|(unsigned int)packet[14]);
				
				/* Calibrate linear data: */
				PositionOrientation::Vector translation;
				for(int i=0;i<3;++i)
					translation[i]=double(rawData[i])*linearGain;
				translation[2]=-translation[2]; // Z-axis values are negated (?!?)
				
				/* Calibrate angular data: */
				PositionOrientation::Vector rotationAxis;
				for(int i=0;i<3;++i)
					rotationAxis[i]=double(rawData[i+3])*angularGain;
				rotationAxis[2]=-rotationAxis[2]; // Z-axis values are negated (?!?)
				
				/* Construct incremental transformation: */
				PositionOrientation t=PositionOrientation::translate(translation);
				PositionOrientation::Scalar rotationAngle=Geometry::mag(rotationAxis);
				PositionOrientation::Rotation rotation=PositionOrientation::Rotation::rotateAxis(rotationAxis,rotationAngle);
				t*=PositionOrientation::rotate(rotation);
				
				/* Accumulate current device position/orientation: */
				currentPositionOrientation*=t;
				// currentPositionOrientation.leftMultiply(t);
				ts.positionOrientation=currentPositionOrientation;
				
				/* Calculate linear and angular velocities: */
				timer.elapse();
				if(notFirstMeasurement)
					{
					/* Estimate velocities by dividing position/orientation differences by elapsed time since last measurement: */
					double time=timer.getTime();
					ts.linearVelocity=translation/Vrui::VRDeviceState::TrackerState::LinearVelocity::Scalar(time);
					ts.angularVelocity=rotationAxis/Vrui::VRDeviceState::TrackerState::AngularVelocity::Scalar(time);
					}
				else
					{
					/* Force initial velocities to zero: */
					ts.linearVelocity=Vrui::VRDeviceState::TrackerState::LinearVelocity::zero;
					ts.angularVelocity=Vrui::VRDeviceState::TrackerState::AngularVelocity::zero;
					notFirstMeasurement=true;
					}
				
				/* Update tracker state: */
				setTrackerState(0,ts);
				break;
				}
			
			case '.':
				{
				/* Parse a button event packet: */
				int buttonMask=0x0;
				buttonMask|=int(packet[2]&0x3f);
				buttonMask|=int(packet[2]&0x80)>>1;
				buttonMask|=int(packet[1]&0x1f)<<7;
				
				/* Update the current button states: */
				for(int i=0;i<12;++i)
					setButtonState(i,buttonMask&(1<<i));
				break;
				}
			}
		}
	}

SpaceBall::SpaceBall(VRDevice::Factory* sFactory,VRDeviceManager* sDeviceManager,Misc::ConfigurationFile& configFile)
	:VRDevice(sFactory,sDeviceManager,configFile),
	 devicePort(configFile.retrieveString("./devicePort").c_str()),
	 linearGain(configFile.retrieveValue<double>("./linearGain",1.0)),
	 angularGain(configFile.retrieveValue<double>("./angularGain",1.0)),
	 currentPositionOrientation(configFile.retrieveValue<PositionOrientation>("./initialState"))
	{
	/* Set device configuration: */
	setNumTrackers(1,configFile);
	setNumButtons(12,configFile);
	
	/* Set device port parameters: */
	int deviceBaudRate=configFile.retrieveValue<int>("./deviceBaudRate",9600);
	devicePort.setSerialSettings(deviceBaudRate,8,Comm::SerialPort::PARITY_NONE,2,false);
	devicePort.setRawMode(1,0);
	
	/* Wait for the fourth carriage return to arrive (end of startup message): */
	#ifdef VERBOSE
	printf("SpaceBall: Reading initialization message\n");
	fflush(stdout);
	#endif
	int numCrs=0;
	while(numCrs<4)
		if(devicePort.readByte()=='\r')
			++numCrs;
	}

void SpaceBall::start(void)
	{
	/* Start device communication thread: */
	startDeviceThread();
	
	/* Enable automatic device updates: */
	#ifdef VERBOSE
	printf("SpaceBall: Enabling automatic update mode\n");
	fflush(stdout);
	#endif
	devicePort.writeString("M\r");
	}

void SpaceBall::stop(void)
	{
	/* Disable automatic device updates: */
	#ifdef VERBOSE
	printf("SpaceBall: Disabling automatic update mode\n");
	fflush(stdout);
	#endif
	devicePort.writeString("-\r");
	
	/* Stop device communication thread: */
	stopDeviceThread();
	}

/*************************************
Object creation/destruction functions:
*************************************/

extern "C" VRDevice* createObjectSpaceBall(VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager,Misc::ConfigurationFile& configFile)
	{
	VRDeviceManager* deviceManager=static_cast<VRDeviceManager::DeviceFactoryManager*>(factoryManager)->getDeviceManager();
	return new SpaceBall(factory,deviceManager,configFile);
	}

extern "C" void destroyObjectSpaceBall(VRDevice* device,VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager)
	{
	delete device;
	}
