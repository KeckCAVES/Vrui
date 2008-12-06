/***********************************************************************
PolhemusFastrak - Class for tracking device of same name.
Copyright (c) 1998-2005 Oliver Kreylos

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

#include <stdio.h>
#include <Misc/Endianness.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/Time.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/GeometryValueCoders.h>

#include "VRDeviceManager.h"

#include "PolhemusFastrak.h"

/********************************
Methods of class PolhemusFastrak:
********************************/

float PolhemusFastrak::readFloat(const char* lsb) const
	{
	float result=*reinterpret_cast<const float*>(lsb);
	#if __BYTE_ORDER==__BIG_ENDIAN
	Misc::swapEndianness(result);
	#endif
	return result;
	}

char* PolhemusFastrak::readLine(int lineBufferSize,char* lineBuffer)
	{
	int state=0;
	char* bufPtr=lineBuffer;
	int bytesLeft=lineBufferSize-1;
	while(state<2)
		{
		/* Read next byte: */
		int input=devicePort.readByte();
		
		/* Process byte: */
		switch(state)
			{
			case 0: // Still waiting for CR
				if(input==13) // CR read?
					{
					/* Go to next state: */
					state=1;
					}
				else if(bytesLeft>0) // Still room in buffer?
					{
					/* Store byte in buffer: */
					*bufPtr=char(input);
					++bufPtr;
					--bytesLeft;
					}
				break;
			
			case 1: // Waiting for LF
				if(input==10) // LF read?
					{
					/* Go to final state: */
					state=2;
					}
				else
					{
					/* Continue reading characters: */
					state=0;
					}
				break;
			}
		}
	
	/* Terminate and return read string: */
	*bufPtr='\0';
	return lineBuffer;
	}

bool PolhemusFastrak::readStatusReply(void)
	{
	int numElapsedWaits=0;
	char buffer[1024];
	int state=0;
	while(numElapsedWaits<100&&state<4)
		{
		/* Wait for a byte to arrive, but don't wait too long: */
		if(!devicePort.waitForByte(Misc::Time(0.1)))
			{
			++numElapsedWaits;
			continue;
			}
		
		/* Read next byte and try matching status reply's prefix: */
		int input=devicePort.readByte();
		switch(state)
			{
			case 0: // Haven't matched anything
				if(input=='2')
					state=1;
				break;
			
			case 1: // Have matched "2"
				if(input=='2')
					state=2;
				else if(input>='1'&&input<='4')
					state=3;
				else
					state=0;
				break;
			
			case 2: // Have matched "22"
				if(input=='S')
					state=4;
				else if(input=='2')
					state=2;
				else if(input>='1'&&input<='4')
					state=3;
				else
					state=0;
				break;
			
			case 3: // Have matched "2[1,3,4]"
				if(input=='S')
					state=4;
				else if(input=='2')
					state=1;
				else
					state=0;
				break;
			}
		}
	
	/* Fail if we timed out while trying to match the prefix: */
	if(state!=4)
		return false;
	
	/* Read rest of status reply until final CR/LF pair: */
	char* cPtr=buffer;
	state=0;
	while(state<2)
		{
		int input=devicePort.readByte();
		*cPtr=char(input);
		++cPtr;
		switch(state)
			{
			case 0: // Haven't matched anything
				if(input==13)
					state=1;
				else
					state=0;
				break;
			
			case 1: // Have matched CR
				if(input==10)
					state=2;
				else if(input==13)
					state=1;
				else
					state=0;
				break;
			}
		}
	*cPtr=0;
	#ifdef VERBOSE
	printf("PolhemusFastrak: Received status reply\n  %s",buffer);
	fflush(stdout);
	#endif
	
	return true;
	}

void PolhemusFastrak::processBuffer(int station,const char* recordBuffer)
	{
	Vrui::VRDeviceState::TrackerState ts;
		
	/* Calculate raw position and orientation: */
	typedef PositionOrientation::Vector Vector;
	typedef Vector::Scalar VScalar;
	Vector v;
	v[0]=VScalar(readFloat(recordBuffer+8));
	v[1]=VScalar(readFloat(recordBuffer+12));
	v[2]=VScalar(readFloat(recordBuffer+16));

	typedef PositionOrientation::Rotation Rotation;
	typedef Rotation::Scalar RScalar;
	RScalar angles[3];
	angles[0]=Math::rad(RScalar(readFloat(recordBuffer+20)));
	angles[1]=Math::rad(RScalar(readFloat(recordBuffer+24)));
	angles[2]=Math::rad(RScalar(readFloat(recordBuffer+28)));
	Rotation o=Rotation::identity;
	o*=Rotation::rotateZ(angles[0]);
	o*=Rotation::rotateY(angles[1]);
	o*=Rotation::rotateX(angles[2]);
	
	/* Set new position and orientation: */
	ts.positionOrientation=Vrui::VRDeviceState::TrackerState::PositionOrientation(v,o);
	
	/* Calculate linear and angular velocities: */
	timers[station].elapse();
	if(notFirstMeasurements[station])
		{
		/* Estimate velocities by dividing position/orientation differences by elapsed time since last measurement: */
		double time=timers[station].getTime();
		ts.linearVelocity=(v-oldPositionOrientations[station].getTranslation())/Vrui::VRDeviceState::TrackerState::LinearVelocity::Scalar(time);
		Rotation dO=o*Geometry::invert(oldPositionOrientations[station].getRotation());
		ts.angularVelocity=dO.getScaledAxis()/Vrui::VRDeviceState::TrackerState::AngularVelocity::Scalar(time);
		}
	else
		{
		/* Force initial velocities to zero: */
		ts.linearVelocity=Vrui::VRDeviceState::TrackerState::LinearVelocity::zero;
		ts.angularVelocity=Vrui::VRDeviceState::TrackerState::AngularVelocity::zero;
		notFirstMeasurements[station]=true;
		}
	oldPositionOrientations[station]=ts.positionOrientation;
	
	/* Update button state: */
	if(station==0&&stylusEnabled)
		{
		/* Update stylus button state: */
		setButtonState(0,recordBuffer[33]=='1');
		}

	/* Update tracker state: */
	setTrackerState(station,ts);
	}

int PolhemusFastrak::readRecordSync(char* recordBuffer)
	{
	int station=-1;
	int state=0;
	while(state<5)
		{
		int input=devicePort.readByte();
		switch(state)
			{
			case 0: // Haven't matched anything
				if(input==13)
					state=1;
				else
					state=0;
				break;
			
			case 1: // Have matched CR
				if(input==10)
					state=2;
				else if(input==13)
					state=1;
				else
					state=0;
				break;
			
			case 2: // Have matched CR/LF
				if(input=='0')
					state=3;
				else if(input==13)
					state=1;
				else
					state=0;
				break;
			
			case 3: // Have matched CR/LF + 0
				if(input>='1'&&input<='4')
					{
					station=input-'1';
					state=4;
					}
				else if(input==13)
					state=1;
				else
					state=0;
				break;
			
			case 4: // Have matched CR/LF + 0 + <receiver number>
				if(input==' '||(input>='A'&&input<='Z')||(input>='a'&&input<='z'))
					state=5;
				else if(input==13)
					state=1;
				else
					state=0;
				break;
			}
		}
	
	/* Read rest of record: */
	devicePort.readBytes(26,recordBuffer+8);
	return station;
	}

int PolhemusFastrak::readRecordNoSync(char* recordBuffer)
	{
	/* Read complete record's worth of data: */
	devicePort.readBytes(31,recordBuffer+3);
	
	/* Check if we're still synchronized: */
	bool nSync=true;
	nSync=nSync&&recordBuffer[3]==char(13);
	nSync=nSync&&recordBuffer[4]==char(10);
	nSync=nSync&&recordBuffer[5]=='0';
	nSync=nSync&&recordBuffer[6]>='1'&&recordBuffer[6]<='4';
	nSync=nSync&&(recordBuffer[7]==' '||(recordBuffer[7]>='A'&&recordBuffer[7]<='Z')||(recordBuffer[7]>='a'&&recordBuffer[7]<='z'));
	if(!nSync)
		return -1;
	
	/* Return station number: */
	return recordBuffer[6]-'1';
	}

void PolhemusFastrak::deviceThreadMethod(void)
	{
	/* Reset first measurement flags: */
	for(int i=0;i<numTrackers;++i)
		notFirstMeasurements[i]=false;
	
	/* Read first record in synchronizing mode: */
	int station;
	char recordBuffer[256];
	station=readRecordSync(recordBuffer);
	
	/* Parse read buffer: */
	processBuffer(station,recordBuffer);
	
	while(true)
		{
		/* Try reading record in synchronized mode: */
		if((station=readRecordNoSync(recordBuffer))<0)
			{
			/* Fall back to synchronizing mode: */
			#ifdef VERBOSE
			printf("PolhemusFastrak: Resynchronizing with tracker stream\n");
			fflush(stdout);
			#endif
			station=readRecordSync(recordBuffer);
			}
		
		/* Parse read buffer: */
		processBuffer(station,recordBuffer);
		}
	}

PolhemusFastrak::PolhemusFastrak(VRDevice::Factory* sFactory,VRDeviceManager* sDeviceManager,Misc::ConfigurationFile& configFile)
	:VRDevice(sFactory,sDeviceManager,configFile),
	 devicePort(configFile.retrieveString("./devicePort").c_str()),
	 stylusEnabled(configFile.retrieveValue<bool>("./stylusEnabled",true)),
	 timers(0),notFirstMeasurements(0),oldPositionOrientations(0)
	{
	/* Set device configuration: */
	setNumTrackers(configFile.retrieveValue<int>("./numReceivers",4),configFile);
	if(stylusEnabled)
		setNumButtons(1,configFile); // Assume that first receiver is a stylus
	
	/* Create free-running timers: */
	timers=new Misc::Timer[numTrackers];
	notFirstMeasurements=new bool[numTrackers];
	oldPositionOrientations=new PositionOrientation[numTrackers];
	
	/* Set device port parameters: */
	int deviceBaudRate=configFile.retrieveValue<int>("./deviceBaudRate");
	devicePort.setSerialSettings(deviceBaudRate,8,Comm::SerialPort::PARITY_NONE,1,false);
	devicePort.setRawMode(1,0);
	
	if(configFile.retrieveValue<bool>("./resetDevice",false))
		{
		/* Reset device: */
		#ifdef VERBOSE
		printf("PolhemusFastrak: Resetting device\n");
		fflush(stdout);
		#endif
		devicePort.writeByte('\31');
		delay(15.0);
		}
	else
		{
		/* Stop continuous mode (in case it's still active): */
		#ifdef VERBOSE
		printf("PolhemusFastrak: Disabling continuous mode\n");
		fflush(stdout);
		#endif
		devicePort.writeByte('c');
		}
	
	/* Request status record to check if device is okey-dokey: */
	#ifdef VERBOSE
	printf("PolhemusFastrak: Requesting status record\n");
	fflush(stdout);
	#endif
	devicePort.writeByte('S');
	if(!readStatusReply())
		{
		/* Try resetting the device, seeing if that helps: */
		#ifdef VERBOSE
		printf("PolhemusFastrak: Resetting device\n");
		fflush(stdout);
		#endif
		devicePort.writeByte('\31');
		delay(15.0);
		
		/* Request another status record: */
		#ifdef VERBOSE
		printf("PolhemusFastrak: Re-requesting status record\n");
		fflush(stdout);
		#endif
		devicePort.writeByte('S');
		if(!readStatusReply())
			Misc::throwStdErr("PolhemusFastrak: Device not responding");
		}
	
	/* Retrieve tracker hemisphere: */
	int hemisphereVectors[6][3]={{1,0,0},{-1,0,0},{0,0,1},{0,0,-1},{0,1,0},{0,-1,0}};
	std::string hemisphere=configFile.retrieveString("./trackerHemisphere","+X");
	int hemisphereIndex=-1;
	if(hemisphere=="+X")
		hemisphereIndex=0;
	else if(hemisphere=="-X")
		hemisphereIndex=1;
	else if(hemisphere=="+Z")
		hemisphereIndex=2;
	else if(hemisphere=="-Z")
		hemisphereIndex=3;
	else if(hemisphere=="+Y")
		hemisphereIndex=4;
	else if(hemisphere=="-Y")
		hemisphereIndex=5;
	else
		Misc::throwStdErr("PolhemusFastrak: Unrecognized hemisphere value %s",hemisphere.c_str());
	
	/* Initialize all receivers: */
	#ifdef VERBOSE
	printf("PolhemusFastrak: Initializing receivers\n");
	fflush(stdout);
	#endif
	for(int i=0;i<numTrackers;++i)
		{
		char command[40];
		
		/* Enable receiver: */
		snprintf(command,sizeof(command),"l%d,1\r\n",i+1);
		devicePort.writeString(command);
		delay(0.1);
		
		/* Reset receiver's alignment frame: */
		snprintf(command,sizeof(command),"R%d\r\n",i+1);
		devicePort.writeString(command);
		delay(0.1);
		
		/* Disable boresight mode: */
		snprintf(command,sizeof(command),"b%d\r\n",i+1);
		devicePort.writeString(command);
		delay(0.1);
		
		/* Set receiver's hemisphere of operation: */
		snprintf(command,sizeof(command),"H%d,%d,%d,%d\r\n",i+1,hemisphereVectors[hemisphereIndex][0],hemisphereVectors[hemisphereIndex][1],hemisphereVectors[hemisphereIndex][2]);
		devicePort.writeString(command);
		delay(0.1);
		
		/* Set receiver's output format: */
		snprintf(command,sizeof(command),"O%d,2,4,16,1\r\n",i+1);
		devicePort.writeString(command);
		delay(0.1);
		}
	
	/* Set stylus tip offset: */
	try
		{
		/* Try getting a tip offset from the configuration file: */
		Geometry::Vector<float,3> tipOffset=configFile.retrieveValue<Geometry::Vector<float,3> >("./stylusTipOffset");
		
		/* If the tag is there, set the stylus tip offset: */
		#ifdef VERBOSE
		printf("PolhemusFastrak: Setting stylus tip offset\n");
		fflush(stdout);
		#endif
		char command[80];
		snprintf(command,sizeof(command),"N1,%8.4f,%8.4f,%8.4f\r\n",tipOffset[0],tipOffset[1],tipOffset[2]);
		devicePort.writeString(command);
		delay(0.1);
		}
	catch(Misc::ConfigurationFile::TagNotFoundError error)
		{
		/* If no tag for stylus offset is given, just leave it to the factory default */
		}
	
	/* Set stylus button to "mouse mode": */
	#ifdef VERBOSE
	printf("PolhemusFastrak: Setting stylus button mode\n");
	fflush(stdout);
	#endif
	devicePort.writeString("e1,0\r\n");
	delay(0.1);
	
	#if 0
	/* Query stylus tip offset: */
	devicePort.writeByte('F');
	delay(0.1);
	devicePort.writeString("N1,\r\n");
	delay(0.1);
	char lineBuffer[80];
	printf("%s\n",readLine(80,lineBuffer);
	fflush(stdout);
	#endif
	
	/* Set fixed metal compensation: */
	#ifdef VERBOSE
	printf("PolhemusFastrak: Setting fixed metal compensation mode\n");
	fflush(stdout);
	#endif
	if(configFile.retrieveValue<bool>("./enableMetalCompensation",false))
		devicePort.writeByte('D');
	else
		devicePort.writeByte('d');
	delay(0.1);
	
	/* Set unit mode to inches: */
	#ifdef VERBOSE
	printf("PolhemusFastrak: Setting unit mode\n");
	fflush(stdout);
	#endif
	devicePort.writeByte('U');
	delay(0.1);
	
	/* Enable binary mode: */
	#ifdef VERBOSE
	printf("PolhemusFastrak: Enabling binary mode\n");
	fflush(stdout);
	#endif
	devicePort.writeByte('f');
	}

PolhemusFastrak::~PolhemusFastrak(void)
	{
	delete[] timers;
	delete[] notFirstMeasurements;
	delete[] oldPositionOrientations;
	}

void PolhemusFastrak::start(void)
	{
	/* Start device communication thread: */
	startDeviceThread();
	
	/* Enable continuous mode: */
	#ifdef VERBOSE
	printf("PolhemusFastrak: Enabling continuous mode\n");
	fflush(stdout);
	#endif
	devicePort.writeByte('C');
	}

void PolhemusFastrak::stop(void)
	{
	/* Disable continuous mode: */
	#ifdef VERBOSE
	printf("PolhemusFastrak: Disabling continuous mode\n");
	fflush(stdout);
	#endif
	devicePort.writeByte('c');
	
	/* Stop device communication thread: */
	stopDeviceThread();
	}

/*************************************
Object creation/destruction functions:
*************************************/

extern "C" VRDevice* createObjectPolhemusFastrak(VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager,Misc::ConfigurationFile& configFile)
	{
	VRDeviceManager* deviceManager=static_cast<VRDeviceManager::DeviceFactoryManager*>(factoryManager)->getDeviceManager();
	return new PolhemusFastrak(factory,deviceManager,configFile);
	}

extern "C" void destroyObjectPolhemusFastrak(VRDevice* device,VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager)
	{
	delete device;
	}
