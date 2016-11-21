/***********************************************************************
InputDeviceAdapterDummy - Class to create "dummy" devices to simulate
behavior of non-existent devices.
Copyright (c) 2015-2016 Oliver Kreylos

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

#include <Vrui/Internal/InputDeviceAdapterDummy.h>

#include <stdio.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <Vrui/Geometry.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/InputGraphManager.h>

namespace Vrui {

/****************************************
Methods of class InputDeviceAdapterDummy:
****************************************/

void InputDeviceAdapterDummy::createInputDevice(int deviceIndex,const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Call the base class method: */
	InputDeviceAdapter::createInputDevice(deviceIndex,configFileSection);
	
	/* Set the just-created device's position and orientation: */
	TrackerState transform=configFileSection.retrieveValue<TrackerState>("./transform",TrackerState::identity);
	inputDevices[deviceIndex]->setTransformation(transform);
	
	/* Set device's linear and angular velocities to zero: */
	inputDevices[deviceIndex]->setLinearVelocity(Vector::zero);
	inputDevices[deviceIndex]->setAngularVelocity(Vector::zero);
	
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

InputDeviceAdapterDummy::InputDeviceAdapterDummy(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection)
	:InputDeviceAdapter(sInputDeviceManager)
	{
	/* Initialize input device adapter: */
	InputDeviceAdapter::initializeAdapter(configFileSection);
	}

InputDeviceAdapterDummy::~InputDeviceAdapterDummy(void)
	{
	}

std::string InputDeviceAdapterDummy::getFeatureName(const InputDeviceFeature& feature) const
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
		Misc::throwStdErr("InputDeviceAdapterDummy::getFeatureName: Unknown device %s",feature.getDevice()->getDeviceName());
	
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

int InputDeviceAdapterDummy::getFeatureIndex(InputDevice* device,const char* featureName) const
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
		Misc::throwStdErr("InputDeviceAdapterDummy::getFeatureIndex: Unknown device %s",device->getDeviceName());
	
	/* Check if the feature names a button or a valuator: */
	for(int buttonIndex=0;buttonIndex<device->getNumButtons();++buttonIndex)
		if(buttonNames[buttonIndexBase+buttonIndex]==featureName)
			return device->getButtonFeatureIndex(buttonIndex);
	for(int valuatorIndex=0;valuatorIndex<device->getNumValuators();++valuatorIndex)
		if(valuatorNames[valuatorIndexBase+valuatorIndex]==featureName)
			return device->getValuatorFeatureIndex(valuatorIndex);
	
	return -1;
	}

void InputDeviceAdapterDummy::updateInputDevices(void)
	{
	/* Nothing to do! */
	}

TrackerState InputDeviceAdapterDummy::peekTrackerState(int deviceIndex)
	{
	if(deviceIndex>=0)
		{
		/* Return the constant tracker state for the requested device: */
		return inputDevices[deviceIndex]->getTransformation();
		}
	else
		{
		/* Fall back to base class, which will throw an exception: */
		return InputDeviceAdapter::peekTrackerState(deviceIndex);
		}
	}

}
