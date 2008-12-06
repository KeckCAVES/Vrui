/***********************************************************************
SpaceBallRaw - VR device driver class exposing the "raw" interface of a
6-DOF joystick as a collection of buttons and valuators. The conversion
from the raw values into 6-DOF states is done at the application end.
Copyright (c) 2004-2005 Oliver Kreylos

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
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/Time.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>

#include "VRDeviceManager.h"

#include "SpaceBallRaw.h"

/*****************************
Methods of class SpaceBallRaw:
*****************************/

bool SpaceBallRaw::readLine(int lineBufferSize,char* lineBuffer,double timeout)
	{
	int devicePortFd=devicePort.getFd();
	
	/* Initialize watched file descriptor sets: */
	fd_set readFds,writeFds,exceptFds;
	FD_ZERO(&readFds);
	FD_ZERO(&writeFds);
	FD_ZERO(&exceptFds);
	
	/* Compute "deadline" as current time + timeout: */
	Misc::Time deadline=Misc::Time::now();
	deadline.increment(timeout);
	
	/* Return characters until carriage return is read or timeout expires: */
	int numRead=0;
	bool lineComplete=false;
	while(numRead<lineBufferSize-1)
		{
		/* Compute time interval until deadline: */
		Misc::Time timeout=deadline;
		timeout-=Misc::Time::now();
		if(timeout.tv_sec<0L) // Deadline expired?
			break;
		
		/* Wait until device ready for reading: */
		FD_SET(devicePortFd,&readFds);
		struct timeval tv=timeout;
		select(devicePortFd+1,&readFds,&writeFds,&exceptFds,&tv);
		if(FD_ISSET(devicePortFd,&readFds))
			{
			/* Read next available byte from the device port: */
			read(devicePortFd,lineBuffer+numRead,1);
			
			/* Check if we just read a CR or LF: */
			if(lineBuffer[numRead]=='\r'||lineBuffer[numRead]=='\n')
				{
				lineComplete=true;
				break;
				}
			else
				++numRead;
			}
		else
			break;
		}
	
	/* Terminate read string and return: */
	lineBuffer[numRead]='\0';
	return lineComplete;
	}

int SpaceBallRaw::readPacket(int packetBufferSize,unsigned char* packetBuffer)
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

void SpaceBallRaw::deviceThreadMethod(void)
	{
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
				/* Parse a data packet: */
				short int rawData[6];
				rawData[0]=(short int)(((unsigned int)packet[ 3]<<8)|(unsigned int)packet[ 4]);
				rawData[1]=(short int)(((unsigned int)packet[ 5]<<8)|(unsigned int)packet[ 6]);
				rawData[2]=(short int)(((unsigned int)packet[ 7]<<8)|(unsigned int)packet[ 8]);
				rawData[3]=(short int)(((unsigned int)packet[ 9]<<8)|(unsigned int)packet[10]);
				rawData[4]=(short int)(((unsigned int)packet[11]<<8)|(unsigned int)packet[12]);
				rawData[5]=(short int)(((unsigned int)packet[13]<<8)|(unsigned int)packet[14]);
				
				/* Set valuator values: */
				for(int i=0;i<6;++i)
					{
					/* Convert raw device data to valuator value: */
					double value=double(rawData[i])*axisGains[i];
					if(value<-1.0)
						value=-1.0;
					else if(value>1.0)
						value=1.0;
					
					/* Set valuator value: */
					setValuatorState(i,value);
					}
				
				/* Mark manager state as complete: */
				updateState();
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
				
				/* Mark manager state as complete: */
				updateState();
				break;
				}
			}
		}
	}

SpaceBallRaw::SpaceBallRaw(VRDevice::Factory* sFactory,VRDeviceManager* sDeviceManager,Misc::ConfigurationFile& configFile)
	:VRDevice(sFactory,sDeviceManager,configFile),
	 devicePort(configFile.retrieveString("./devicePort").c_str())
	{
	/* Set device configuration: */
	setNumTrackers(0,configFile);
	setNumButtons(12,configFile);
	setNumValuators(6,configFile);
	
	/* Read axis manipulation factors: */
	double axisGain=configFile.retrieveValue<double>("./axisGain",1.0);
	double linearAxisGain=configFile.retrieveValue<double>("./linearAxisGain",axisGain);
	double angularAxisGain=configFile.retrieveValue<double>("./angularAxisGain",axisGain);
	for(int i=0;i<6;++i)
		{
		char axisGainTag[40];
		snprintf(axisGainTag,sizeof(axisGainTag),"./axisGain%d",i);
		axisGains[i]=configFile.retrieveValue<double>(axisGainTag,i<3?linearAxisGain:angularAxisGain);
		}
	
	/* Set device port parameters: */
	int deviceBaudRate=configFile.retrieveValue<int>("./deviceBaudRate",9600);
	devicePort.setSerialSettings(deviceBaudRate,8,Comm::SerialPort::PARITY_NONE,2,false);
	devicePort.setRawMode(1,0);
	
	/* Wait for status message from device: */
	#ifdef VERBOSE
	printf("SpaceBallRaw: Reading initialization message\n");
	fflush(stdout);
	#endif
	char lineBuffer[256];
	const int numResponses=4;
	char* responseTexts[numResponses]={"\021","@1 Spaceball alive and well","","@2 Firmware version"};
	int responseLengths[numResponses]={2,27,1,19};
	for(int i=0;i<numResponses;++i)
		{
		/* Try reading a line from the device port: */
		if(!readLine(256,lineBuffer,10.0))
			Misc::throwStdErr("SpaceBallRaw: Timeout while reading status message");
		
		/* Check if line is correct SpaceBall response: */
		if(strncmp(lineBuffer,responseTexts[i],responseLengths[i])!=0)
			Misc::throwStdErr("SpaceBallRaw: Incorrect response while reading status message");
		}
	}

void SpaceBallRaw::start(void)
	{
	/* Start device communication thread: */
	startDeviceThread();
	
	/* Enable automatic device updates: */
	#ifdef VERBOSE
	printf("SpaceBallRaw: Enabling automatic update mode\n");
	fflush(stdout);
	#endif
	devicePort.writeString("M\r");
	}

void SpaceBallRaw::stop(void)
	{
	/* Disable automatic device updates: */
	#ifdef VERBOSE
	printf("SpaceBallRaw: Disabling automatic update mode\n");
	fflush(stdout);
	#endif
	devicePort.writeString("-\r");
	
	/* Stop device communication thread: */
	stopDeviceThread();
	}

/*************************************
Object creation/destruction functions:
*************************************/

extern "C" VRDevice* createObjectSpaceBallRaw(VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager,Misc::ConfigurationFile& configFile)
	{
	VRDeviceManager* deviceManager=static_cast<VRDeviceManager::DeviceFactoryManager*>(factoryManager)->getDeviceManager();
	return new SpaceBallRaw(factory,deviceManager,configFile);
	}

extern "C" void destroyObjectSpaceBallRaw(VRDevice* device,VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager)
	{
	delete device;
	}
