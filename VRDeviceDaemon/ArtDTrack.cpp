/***********************************************************************
ArtDTrack - Class for ART DTrack tracking devices.
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
#include <Misc/Endianness.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>

#include "VRDeviceManager.h"

#include "ArtDTrack.h"

/***************************************************************
Helper function to extract binary data from DTrack message body:
***************************************************************/

template <class DataParam>
inline DataParam ArtDTrack::extractData(const char*& dataPtr)
	{
	/* Extract data item: */
	DataParam result=*reinterpret_cast<const DataParam*>(dataPtr);
	
	#if __BYTE_ORDER==__BIG_ENDIAN
	/* Convert endianness of data item: */
	Misc::swapEndianness(result);
	#endif
	
	/* Advance data pointer: */
	dataPtr+=sizeof(DataParam);
	
	/* Return data item: */
	return result;
	}

template <class DataParam>
inline void ArtDTrack::skipData(const char*& dataPtr)
	{
	/* Advance data pointer: */
	dataPtr+=sizeof(DataParam);
	}

/**************************
Methods of class ArtDTrack:
**************************/

void ArtDTrack::deviceThreadMethod(void)
	{
	while(true)
		{
		/* Wait for the next data message from the DTrack daemon: */
		char messageBuffer[1024];
		dataSocket.receiveMessage(messageBuffer,sizeof(messageBuffer));
		
		/* Parse the received message: */
		const char* mPtr=messageBuffer;
		// unsigned int frameNr=extractData<unsigned int>(mPtr);
		skipData<unsigned int>(mPtr); // Skip frame number
		int numBodies=extractData<int>(mPtr);
		for(int i=0;i<numBodies;++i)
			{
			/* Read body's ID and measurement quality: */
			int trackerId=int(extractData<unsigned int>(mPtr));
			// float quality=extractData<float>(mPtr);
			skipData<float>(mPtr); // Skip measurement quality
			
			/* Read body's position: */
			Vector pos;
			for(int j=0;j<3;++j)
				pos[j]=VScalar(extractData<float>(mPtr));
			
			/* Read body's orientation as Euler angles: */
			RScalar angles[3];
			for(int j=0;j<3;++j)
				angles[j]=Math::rad(extractData<float>(mPtr));
			
			/* Convert Euler angles to rotation: */
			Rotation o=Rotation::identity;
			o*=Rotation::rotateX(angles[0]);
			o*=Rotation::rotateY(angles[1]);
			o*=Rotation::rotateZ(angles[2]);
			
			/* Skip body's orientation as rotation matrix: */
			for(int j=0;j<9;++j)
				extractData<float>(mPtr);
			
			/* Set tracker position and orientation: */
			if(trackerId<getNumTrackers())
				trackerStates[trackerId].positionOrientation=PositionOrientation(pos,o);
			}
		
		/* Update all tracker states (including those that were not updated): */
		for(int i=0;i<getNumTrackers();++i)
			setTrackerState(i,trackerStates[i]);
		}
	}

ArtDTrack::ArtDTrack(VRDevice::Factory* sFactory,VRDeviceManager* sDeviceManager,Misc::ConfigurationFile& configFile)
	:VRDevice(sFactory,sDeviceManager,configFile),
	 controlSocket(-1,configFile.retrieveString("./serverName"),configFile.retrieveValue<int>("./serverControlPort")),
	 dataSocket(configFile.retrieveValue<int>("./serverDataPort"),0),
	 trackerStates(0)
	{
	/* Set DTrack's layout: */
	setNumTrackers(configFile.retrieveValue<int>("./numTrackers"),configFile);
	
	/* Initialize tracker states: */
	trackerStates=new Vrui::VRDeviceState::TrackerState[getNumTrackers()];
	for(int i=0;i<getNumTrackers();++i)
		{
		trackerStates[i].positionOrientation=PositionOrientation(Vector::zero,Rotation::identity);
		trackerStates[i].linearVelocity=Vrui::VRDeviceState::TrackerState::LinearVelocity::zero;
		trackerStates[i].angularVelocity=Vrui::VRDeviceState::TrackerState::AngularVelocity::zero;
		}
	}

ArtDTrack::~ArtDTrack(void)
	{
	delete[] trackerStates;
	}

void ArtDTrack::start(void)
	{
	/* Start device communication thread: */
	startDeviceThread();
	
	/* Activate DTrack: */
	#ifdef VERBOSE
	printf("ArtDTrack: Activating cameras and reconstruction\n");
	#endif
	char* msg1="dtrack 10 3";
	controlSocket.sendMessage(msg1,strlen(msg1)+1);
	
	delay(0.5);
	
	/* Start sending measurements: */
	#ifdef VERBOSE
	printf("ArtDTrack: Starting continuous update mode\n");
	#endif
	char* msg2="dtrack 31";
	controlSocket.sendMessage(msg2,strlen(msg2)+1);
	}

void ArtDTrack::stop(void)
	{
	/* Stop sending measurements: */
	#ifdef VERBOSE
	printf("ArtDTrack: Stopping continuous update mode\n");
	#endif
	char* msg1="dtrack 32";
	controlSocket.sendMessage(msg1,strlen(msg1)+1);
	
	delay(0.5);
	
	/* Deactivate DTrack: */
	#ifdef VERBOSE
	printf("ArtDTrack: Deactivating cameras and reconstruction\n");
	#endif
	char* msg2="dtrack 10 0";
	controlSocket.sendMessage(msg2,strlen(msg2)+1);
	
	/* Stop device communication thread: */
	stopDeviceThread();
	}

/*************************************
Object creation/destruction functions:
*************************************/

extern "C" VRDevice* createObjectArtDTrack(VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager,Misc::ConfigurationFile& configFile)
	{
	VRDeviceManager* deviceManager=static_cast<VRDeviceManager::DeviceFactoryManager*>(factoryManager)->getDeviceManager();
	return new ArtDTrack(factory,deviceManager,configFile);
	}

extern "C" void destroyObjectArtDTrack(VRDevice* device,VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager)
	{
	delete device;
	}
