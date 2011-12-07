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

#include <VRDeviceDaemon/VRDeviceManager.h>

#include <stdio.h>
#include <dlfcn.h>
#include <vector>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>

#include <VRDeviceDaemon/VRFactory.h>
#include <VRDeviceDaemon/VRDevice.h>
#include <VRDeviceDaemon/VRCalibrator.h>

/********************************
Methods of class VRDeviceManager:
********************************/

VRDeviceManager::VRDeviceManager(Misc::ConfigurationFile& configFile)
	:deviceFactories(configFile.retrieveString("./deviceDirectory",SYSVRDEVICEDIRECTORY),this),
	 calibratorFactories(configFile.retrieveString("./calibratorDirectory",SYSVRCALIBRATORDIRECTORY)),
	 numDevices(0),
	 devices(0),trackerIndexBases(0),buttonIndexBases(0),valuatorIndexBases(0),
	 fullTrackerReportMask(0x0),trackerReportMask(0x0),trackerUpdateNotificationEnabled(false),
	 trackerUpdateCompleteCond(0)
	{
	/* Allocate device and base index arrays: */
	typedef std::vector<std::string> StringList;
	StringList deviceNames=configFile.retrieveValue<StringList>("./deviceNames");
	numDevices=deviceNames.size();
	devices=new VRDevice*[numDevices];
	trackerIndexBases=new int[numDevices];
	buttonIndexBases=new int[numDevices];
	valuatorIndexBases=new int[numDevices];
	
	/* Initialize VR devices: */
	for(int deviceIndex=0;deviceIndex<numDevices;++deviceIndex)
		{
		/* Save the device's base indices: */
		trackerIndexBases[deviceIndex]=int(trackerNames.size());
		buttonIndexBases[deviceIndex]=int(buttonNames.size());
		valuatorIndexBases[deviceIndex]=int(valuatorNames.size());
		
		/* Go to device's section: */
		configFile.setCurrentSection(deviceNames[deviceIndex].c_str());
		
		/* Retrieve device type: */
		std::string deviceType=configFile.retrieveString("./deviceType");
		
		/* Initialize device: */
		#ifdef VERBOSE
		printf("VRDeviceManager: Loading device %s of type %s\n",deviceNames[deviceIndex].c_str(),deviceType.c_str());
		fflush(stdout);
		#endif
		DeviceFactoryManager::Factory* deviceFactory=deviceFactories.getFactory(deviceType);
		devices[deviceIndex]=deviceFactory->createObject(configFile);
		
		/* Override device's tracker names: */
		try
			{
			StringList deviceTrackerNames=configFile.retrieveValue<StringList>("./trackerNames");
			int trackerIndex=trackerIndexBases[deviceIndex];
			int numTrackers=trackerNames.size();
			for(StringList::iterator dtnIt=deviceTrackerNames.begin();dtnIt!=deviceTrackerNames.end()&&trackerIndex<numTrackers;++dtnIt,++trackerIndex)
				trackerNames[trackerIndex]=*dtnIt;
			}
		catch(Misc::ConfigurationFile::TagNotFoundError)
			{
			/* Ignore the error... */
			}
		
		/* Override device's button names: */
		try
			{
			StringList deviceButtonNames=configFile.retrieveValue<StringList>("./buttonNames");
			int buttonIndex=buttonIndexBases[deviceIndex];
			int numButtons=buttonNames.size();
			for(StringList::iterator dtnIt=deviceButtonNames.begin();dtnIt!=deviceButtonNames.end()&&buttonIndex<numButtons;++dtnIt,++buttonIndex)
				buttonNames[buttonIndex]=*dtnIt;
			}
		catch(Misc::ConfigurationFile::TagNotFoundError)
			{
			/* Ignore the error... */
			}
		
		/* Override device's valuator names: */
		try
			{
			StringList deviceValuatorNames=configFile.retrieveValue<StringList>("./valuatorNames");
			int valuatorIndex=valuatorIndexBases[deviceIndex];
			int numValuators=valuatorNames.size();
			for(StringList::iterator dtnIt=deviceValuatorNames.begin();dtnIt!=deviceValuatorNames.end()&&valuatorIndex<numValuators;++dtnIt,++valuatorIndex)
				valuatorNames[valuatorIndex]=*dtnIt;
			}
		catch(Misc::ConfigurationFile::TagNotFoundError)
			{
			/* Ignore the error... */
			}
		
		/* Return to parent section: */
		configFile.setCurrentSection("..");
		}
	#ifdef VERBOSE
	printf("VRDeviceManager: Managing %d trackers, %d buttons, %d valuators\n",int(trackerNames.size()),int(buttonNames.size()),int(valuatorNames.size()));
	fflush(stdout);
	#endif
	
	/* Set server state's layout: */
	state.setLayout(trackerNames.size(),buttonNames.size(),valuatorNames.size());
	
	/* Read names of all virtual devices: */
	StringList virtualDeviceNames=configFile.retrieveValue<StringList>("./virtualDeviceNames",StringList());
	
	#if 0
	/* Initialize virtual devices: */
	for(StringList::iterator vdnIt=virtualDeviceNames.begin();vdnIt!=virtualDeviceNames.end();++vdnIt)
		{
		/* Go to virtual device's section: */
		configFile.setCurrentSection(vdnIt->c_str());
		
		/* Read the virtual device's button indices: */
		std::vector<int>
		
		
		/* Read the virtual device's tracker index: */
		int trackerIndex=configFile.retrieveValue<int>("./trackerIndex",-1);
		
		/* Read the virtual device's ray direction: */
		Vrui::VRDeviceDescriptor::Vector rayDirection=
		
		
		
		
		
		
		
		
		
		/* Return to parent section: */
		configFile.setCurrentSection("..");
		}
	#ifdef VERBOSE
	printf("VRDeviceManager: Managing %d virtual devices\n",int(virtualDevices.size()));
	fflush(stdout);
	#endif
	
	#endif
	}

VRDeviceManager::~VRDeviceManager(void)
	{
	/* Delete device objects: */
	for(int i=0;i<numDevices;++i)
		VRDevice::destroy(devices[i]);
	delete[] devices;
	
	/* Delete base index arrays: */
	delete[] trackerIndexBases;
	delete[] buttonIndexBases;
	delete[] valuatorIndexBases;
	
	/* Delete virtual devices: */
	for(std::vector<Vrui::VRDeviceDescriptor*>::iterator vdIt=virtualDevices.begin();vdIt!=virtualDevices.end();++vdIt)
		delete *vdIt;
	}

int VRDeviceManager::addTracker(const char* name)
	{
	/* Get the next tracker index: */
	int result=trackerNames.size();
	
	/* Push back a new tracker name: */
	trackerNames.push_back(name!=0?name:"");
	
	/* Update the tracker report mask: */
	fullTrackerReportMask=(0x1U<<(result+1))-1U;
	
	return result;
	}

int VRDeviceManager::addButton(const char* name)
	{
	/* Get the next button index: */
	int result=buttonNames.size();
	
	/* Push back a new button name: */
	buttonNames.push_back(name!=0?name:"");
	
	return result;
	}

int VRDeviceManager::addValuator(const char* name)
	{
	/* Get the next valuator index: */
	int result=valuatorNames.size();
	
	/* Push back a new valuator name: */
	valuatorNames.push_back(name!=0?name:"");
	
	return result;
	}

void VRDeviceManager::addVirtualDevice(Vrui::VRDeviceDescriptor* newVirtualDevice)
	{
	/* Store the virtual device: */
	virtualDevices.push_back(newVirtualDevice);
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
