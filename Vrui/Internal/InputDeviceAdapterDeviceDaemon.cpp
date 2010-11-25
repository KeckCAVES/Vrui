/***********************************************************************
InputDeviceAdapterDeviceDaemon - Class to convert from Vrui's own
distributed device driver architecture to Vrui's internal device
representation.
Copyright (c) 2004-2010 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Vrui/Internal/InputDeviceAdapterDeviceDaemon.h>

#include <stdio.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceFeature.h>

namespace Vrui {

/***********************************************
Methods of class InputDeviceAdapterDeviceDaemon:
***********************************************/

void InputDeviceAdapterDeviceDaemon::packetNotificationCallback(VRDeviceClient* client,void* userData)
	{
	requestUpdate();
	}

void InputDeviceAdapterDeviceDaemon::createInputDevice(int deviceIndex,const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Call base class method to initialize the input device: */
	InputDeviceAdapterIndexMap::createInputDevice(deviceIndex,configFileSection);
	
	/* Read the list of button names for this device: */
	/* Read the names of all button features: */
	typedef std::vector<std::string> StringList;
	StringList tempButtonNames=configFileSection.retrieveValue<StringList>("./buttonNames",StringList());
	int buttonIndex=0;
	for(StringList::iterator bnIt=tempButtonNames.begin();bnIt!=tempButtonNames.end()&&buttonIndex<inputDevices[deviceIndex]->getNumButtons();++bnIt,++buttonIndex)
		{
		/* Store the button name: */
		buttonNames.push_back(*bnIt);
		}
	for(;buttonIndex<inputDevices[deviceIndex]->getNumButtons();++buttonIndex)
		{
		char buttonName[40];
		snprintf(buttonName,sizeof(buttonName),"Button%d",buttonIndex);
		buttonNames.push_back(buttonName);
		}
	
	/* Read the names of all valuator features: */
	StringList tempValuatorNames=configFileSection.retrieveValue<StringList>("./valuatorNames",StringList());
	int valuatorIndex=0;
	for(StringList::iterator vnIt=tempValuatorNames.begin();vnIt!=tempValuatorNames.end()&&valuatorIndex<inputDevices[deviceIndex]->getNumValuators();++vnIt,++valuatorIndex)
		{
		/* Store the valuator name: */
		valuatorNames.push_back(*vnIt);
		}
	for(;valuatorIndex<inputDevices[deviceIndex]->getNumValuators();++valuatorIndex)
		{
		char valuatorName[40];
		snprintf(valuatorName,sizeof(valuatorName),"Valuator%d",valuatorIndex);
		valuatorNames.push_back(valuatorName);
		}
	}

InputDeviceAdapterDeviceDaemon::InputDeviceAdapterDeviceDaemon(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection)
	:InputDeviceAdapterIndexMap(sInputDeviceManager),
	 deviceClient(configFileSection)
	{
	/* Initialize input device adapter: */
	InputDeviceAdapterIndexMap::initializeAdapter(deviceClient.getState().getNumTrackers(),deviceClient.getState().getNumButtons(),deviceClient.getState().getNumValuators(),configFileSection);
	
	/* Start VR devices: */
	deviceClient.enablePacketNotificationCB(packetNotificationCallback,0);
	deviceClient.activate();
	deviceClient.startStream();
	}

InputDeviceAdapterDeviceDaemon::~InputDeviceAdapterDeviceDaemon(void)
	{
	/* Stop VR devices: */
	deviceClient.stopStream();
	deviceClient.deactivate();
	deviceClient.disablePacketNotificationCB();
	}

std::string InputDeviceAdapterDeviceDaemon::getFeatureName(const InputDeviceFeature& feature) const
	{
	/* Find the input device owning the given feature: */
	bool deviceFound=false;
	int buttonIndexBase=0;
	int valuatorIndexBase=0;
	for(int deviceIndex=0;deviceIndex<numInputDevices;++deviceIndex)
		{
		if(inputDevices[deviceIndex]==feature.getDevice())
			{
			deviceFound=true;
			break;
			}
		
		/* Go to the next device: */
		buttonIndexBase+=inputDevices[deviceIndex]->getNumButtons();
		valuatorIndexBase+=inputDevices[deviceIndex]->getNumValuators();
		}
	if(!deviceFound)
		Misc::throwStdErr("InputDeviceAdapterDeviceDaemon::getFeatureName: Unknown device %s",feature.getDevice()->getDeviceName());
	
	/* Check whether the feature is a button or a valuator: */
	std::string result;
	if(feature.isButton())
		{
		/* Return the button feature's name: */
		result=buttonNames[buttonIndexBase+feature.getIndex()];
		}
	if(feature.isValuator())
		{
		/* Return the valuator feature's name: */
		result=valuatorNames[valuatorIndexBase+feature.getIndex()];
		}
	
	return result;
	}

int InputDeviceAdapterDeviceDaemon::getFeatureIndex(InputDevice* device,const char* featureName) const
	{
	/* Find the input device owning the given feature: */
	bool deviceFound=false;
	int buttonIndexBase=0;
	int valuatorIndexBase=0;
	for(int deviceIndex=0;deviceIndex<numInputDevices;++deviceIndex)
		{
		if(inputDevices[deviceIndex]==device)
			{
			deviceFound=true;
			break;
			}
		
		/* Go to the next device: */
		buttonIndexBase+=inputDevices[deviceIndex]->getNumButtons();
		valuatorIndexBase+=inputDevices[deviceIndex]->getNumValuators();
		}
	if(!deviceFound)
		Misc::throwStdErr("InputDeviceAdapterDeviceDaemon::getFeatureIndex: Unknown device %s",device->getDeviceName());
	
	/* Check if the feature names a button or a valuator: */
	for(int buttonIndex=0;buttonIndex<device->getNumButtons();++buttonIndex)
		if(buttonNames[buttonIndexBase+buttonIndex]==featureName)
			return device->getButtonFeatureIndex(buttonIndex);
	for(int valuatorIndex=0;valuatorIndex<device->getNumValuators();++valuatorIndex)
		if(valuatorNames[valuatorIndexBase+valuatorIndex]==featureName)
			return device->getValuatorFeatureIndex(valuatorIndex);
	
	return -1;
	}

void InputDeviceAdapterDeviceDaemon::updateInputDevices(void)
	{
	deviceClient.lockState();
	const VRDeviceState& state=deviceClient.getState();
	for(int deviceIndex=0;deviceIndex<numInputDevices;++deviceIndex)
		{
		/* Get pointer to the input device: */
		InputDevice* device=inputDevices[deviceIndex];
		
		/* Don't update tracker-related state for devices that are not tracked: */
		if(trackerIndexMapping[deviceIndex]>=0)
			{
			/* Get device's tracker state from VR device client: */
			const VRDeviceState::TrackerState& ts=state.getTrackerState(trackerIndexMapping[deviceIndex]);
			
			/* Set device's transformation: */
			device->setTransformation(TrackerState(ts.positionOrientation));
			
			/* Set device's linear and angular velocities: */
			device->setLinearVelocity(Vector(ts.linearVelocity));
			device->setAngularVelocity(Vector(ts.angularVelocity));
			}
		
		/* Update button states: */
		for(int i=0;i<device->getNumButtons();++i)
			device->setButtonState(i,state.getButtonState(buttonIndexMapping[deviceIndex][i]));
		
		/* Update valuator states: */
		for(int i=0;i<device->getNumValuators();++i)
			device->setValuator(i,state.getValuatorState(valuatorIndexMapping[deviceIndex][i]));
		}
	
	deviceClient.unlockState();
	}

}
