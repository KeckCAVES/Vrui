/***********************************************************************
VRDeviceManager - Class to gather position, button and valuator data
from one or several VR devices and associate them with logical input
devices.
Copyright (c) 2002-2005 Oliver Kreylos

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

#include "VRDeviceManager.h"

#include <stdio.h>
#include <dlfcn.h>
#include <vector>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>

#include "VRFactory.h"
#include "VRDevice.h"
#include "VRCalibrator.h"

/********************************
Methods of class VRDeviceManager:
********************************/

VRDeviceManager::VRDeviceManager(Misc::ConfigurationFile& configFile)
	:deviceFactories(configFile.retrieveString("./deviceDirectory",SYSVRDEVICEDIRECTORY),this),
	 calibratorFactories(configFile.retrieveString("./calibratorDirectory",SYSVRCALIBRATORDIRECTORY)),
	 numDevices(0),
	 devices(0),
	 fullTrackerReportMask(0x0),trackerReportMask(0x0),trackerUpdateNotificationEnabled(false),
	 trackerUpdateCompleteCond(0)
	{
	/* Allocate device array: */
	typedef std::vector<std::string> StringList;
	StringList deviceNames=configFile.retrieveValue<StringList>("./deviceNames");
	numDevices=deviceNames.size();
	devices=new VRDevice*[numDevices];
	
	/* Initialize VR devices: */
	int numTrackers=0;
	int numButtons=0;
	int numValuators=0;
	for(int i=0;i<numDevices;++i)
		{
		/* Go to device's section: */
		configFile.setCurrentSection(deviceNames[i].c_str());
		
		/* Retrieve device type: */
		std::string deviceType=configFile.retrieveString("./deviceType");
		
		/* Initialize device: */
		#ifdef VERBOSE
		printf("VRDeviceManager: Loading device %s of type %s\n",deviceNames[i].c_str(),deviceType.c_str());
		fflush(stdout);
		#endif
		DeviceFactoryManager::Factory* deviceFactory=deviceFactories.getFactory(deviceType);
		devices[i]=deviceFactory->createObject(configFile);
		
		/* Query device configuration: */
		for(int j=0;j<devices[i]->getNumTrackers();++j)
			if(numTrackers<devices[i]->getTrackerIndex(j)+1)
				numTrackers=devices[i]->getTrackerIndex(j)+1;
		for(int j=0;j<devices[i]->getNumButtons();++j)
			if(numButtons<devices[i]->getButtonIndex(j)+1)
				numButtons=devices[i]->getButtonIndex(j)+1;
		for(int j=0;j<devices[i]->getNumValuators();++j)
			if(numValuators<devices[i]->getValuatorIndex(j)+1)
				numValuators=devices[i]->getValuatorIndex(j)+1;
		
		/* Return to parent section: */
		configFile.setCurrentSection("..");
		}
	#ifdef VERBOSE
	printf("VRDeviceManager: Managing %d trackers, %d buttons, %d valuators\n",numTrackers,numButtons,numValuators);
	fflush(stdout);
	#endif
	
	/* Set state layout and initialize state: */
	state.setLayout(numTrackers,numButtons,numValuators);
	Vrui::VRDeviceState::TrackerState defaultTs;
	defaultTs.positionOrientation=Vrui::VRDeviceState::TrackerState::PositionOrientation::identity;
	defaultTs.linearVelocity=Vrui::VRDeviceState::TrackerState::LinearVelocity::zero;
	defaultTs.angularVelocity=Vrui::VRDeviceState::TrackerState::AngularVelocity::zero;
	for(int i=0;i<numTrackers;++i)
		state.setTrackerState(i,defaultTs);
	for(int i=0;i<numButtons;++i)
		state.setButtonState(i,false);
	for(int i=0;i<numValuators;++i)
		state.setValuatorState(i,0);
	
	/* Check logical index associatons: */
	int* trackerDeviceIndices=new int[numTrackers];
	for(int i=0;i<numTrackers;++i)
		trackerDeviceIndices[i]=-1;
	int* buttonDeviceIndices=new int[numButtons];
	for(int i=0;i<numButtons;++i)
		buttonDeviceIndices[i]=-1;
	int* valuatorDeviceIndices=new int[numValuators];
	for(int i=0;i<numValuators;++i)
		valuatorDeviceIndices[i]=-1;
	
	for(int i=0;i<numDevices;++i)
		{
		for(int j=0;j<devices[i]->getNumTrackers();++j)
			{
			if(trackerDeviceIndices[devices[i]->getTrackerIndex(j)]!=-1)
				{
				fprintf(stderr,"VRDeviceManager: Logical tracker index %d assigned to devices %d and %d\n",devices[i]->getTrackerIndex(j),trackerDeviceIndices[devices[i]->getTrackerIndex(j)],i);
				fflush(stderr);
				}
			else
				trackerDeviceIndices[devices[i]->getTrackerIndex(j)]=j;
			fullTrackerReportMask|=1<<devices[i]->getTrackerIndex(j);
			}
		for(int j=0;j<devices[i]->getNumButtons();++j)
			if(buttonDeviceIndices[devices[i]->getButtonIndex(j)]!=-1)
				{
				fprintf(stderr,"VRDeviceManager: Logical button index %d assigned to devices %d and %d\n",devices[i]->getButtonIndex(j),buttonDeviceIndices[devices[i]->getButtonIndex(j)],i);
				fflush(stderr);
				}
			else
				buttonDeviceIndices[devices[i]->getButtonIndex(j)]=j;
		for(int j=0;j<devices[i]->getNumValuators();++j)
			if(valuatorDeviceIndices[devices[i]->getValuatorIndex(j)]!=-1)
				{
				fprintf(stderr,"VRDeviceManager: Logical valuator index %d assigned to devices %d and %d\n",devices[i]->getValuatorIndex(j),valuatorDeviceIndices[devices[i]->getValuatorIndex(j)],i);
				fflush(stderr);
				}
			else
				valuatorDeviceIndices[devices[i]->getValuatorIndex(j)]=j;
		}
	
	for(int i=0;i<numTrackers;++i)
		if(trackerDeviceIndices[i]==-1)
			{
			fprintf(stderr,"VRDeviceManager: Logical tracker index %d not assigned to any device\n",i);
			fflush(stderr);
			}
	for(int i=0;i<numButtons;++i)
		if(buttonDeviceIndices[i]==-1)
			{
			fprintf(stderr,"VRDeviceManager: Logical button index %d not assigned to any device\n",i);
			fflush(stderr);
			}
	for(int i=0;i<numValuators;++i)
		if(valuatorDeviceIndices[i]==-1)
			{
			fprintf(stderr,"VRDeviceManager: Logical valuator index %d not assigned to any device\n",i);
			fflush(stderr);
			}
	
	delete[] trackerDeviceIndices;
	delete[] buttonDeviceIndices;
	delete[] valuatorDeviceIndices;
	}

VRDeviceManager::~VRDeviceManager(void)
	{
	/* Delete device objects: */
	for(int i=0;i<numDevices;++i)
		VRDevice::destroy(devices[i]);
	delete[] devices;
	}

VRCalibrator* VRDeviceManager::createCalibrator(const std::string& calibratorType,Misc::ConfigurationFile& configFile)
	{
	CalibratorFactoryManager::Factory* calibratorFactory=calibratorFactories.getFactory(calibratorType);
	return calibratorFactory->createObject(configFile);
	}

void VRDeviceManager::setTrackerState(int trackerIndex,const Vrui::VRDeviceState::TrackerState& newTrackerState)
	{
	Threads::Mutex::Lock stateLock(stateMutex);
	state.setTrackerState(trackerIndex,newTrackerState);
	
	if(trackerUpdateNotificationEnabled)
		{
		/* Update tracker report mask: */
		trackerReportMask|=1<<trackerIndex;
		if(trackerReportMask==fullTrackerReportMask)
			{
			/* Wake up all client threads in stream mode: */
			trackerUpdateCompleteCond->broadcast();
			trackerReportMask=0x0;
			}
		}
	}

void VRDeviceManager::setButtonState(int buttonIndex,Vrui::VRDeviceState::ButtonState newButtonState)
	{
	Threads::Mutex::Lock stateLock(stateMutex);
	state.setButtonState(buttonIndex,newButtonState);
	}

void VRDeviceManager::setValuatorState(int valuatorIndex,Vrui::VRDeviceState::ValuatorState newValuatorState)
	{
	Threads::Mutex::Lock stateLock(stateMutex);
	state.setValuatorState(valuatorIndex,newValuatorState);
	}

void VRDeviceManager::enableTrackerUpdateNotification(Threads::MutexCond* sTrackerUpdateCompleteCond)
	{
	Threads::Mutex::Lock stateLock(stateMutex);
	trackerUpdateNotificationEnabled=true;
	trackerUpdateCompleteCond=sTrackerUpdateCompleteCond;
	trackerReportMask=0x0;
	}

void VRDeviceManager::disableTrackerUpdateNotification(void)
	{
	Threads::Mutex::Lock stateLock(stateMutex);
	trackerUpdateNotificationEnabled=false;
	trackerUpdateCompleteCond=0;
	}

void VRDeviceManager::updateState(void)
	{
	Threads::Mutex::Lock stateLock(stateMutex);
	if(trackerUpdateNotificationEnabled)
		{
		/* Wake up all client threads in stream mode: */
		trackerUpdateCompleteCond->broadcast();
		}
	}

void VRDeviceManager::start(void)
	{
	#ifdef VERBOSE
	printf("VRDeviceManager: Starting devices\n");
	fflush(stdout);
	#endif
	for(int i=0;i<numDevices;++i)
		devices[i]->start();
	}

void VRDeviceManager::stop(void)
	{
	#ifdef VERBOSE
	printf("VRDeviceManager: Stopping devices\n");
	fflush(stdout);
	#endif
	for(int i=0;i<numDevices;++i)
		devices[i]->stop();
	}
