/***********************************************************************
InputDeviceManager - Class to manage physical and virtual input devices,
tools associated to input devices, and the input device update graph.
Copyright (c) 2004-2005 Oliver Kreylos

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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/InputDeviceAdapter.h>
#include <Vrui/InputDeviceAdapterMouse.h>
#include <Vrui/InputDeviceAdapterDeviceDaemon.h>
#include <Vrui/InputDeviceAdapterVisBox.h>
#include <Vrui/InputDeviceAdapterHID.h>
#include <Vrui/InputDeviceAdapterPlayback.h>

#include <Vrui/InputDeviceManager.h>

namespace Vrui {

/****************
Helper functions:
****************/

namespace {

static int getPrefixLength(const char* deviceName)
	{
	/* Find the last colon in the device name: */
	const char* colonPtr=0;
	const char* cPtr;
	bool onlyDigits=false;
	for(cPtr=deviceName;*cPtr!='\0';++cPtr)
		{
		if(*cPtr==':')
			{
			colonPtr=cPtr;
			onlyDigits=true;
			}
		else if(!isdigit(*cPtr))
			onlyDigits=false;
		}
	
	if(onlyDigits&&colonPtr[1]!='\0')
		return colonPtr-deviceName;
	else
		return cPtr-deviceName;
	}

}

/***********************************
Methods of class InputDeviceManager:
***********************************/

InputDeviceManager::InputDeviceManager(InputGraphManager* sInputGraphManager)
	:inputGraphManager(sInputGraphManager),
	 numInputDeviceAdapters(0),inputDeviceAdapters(0)
	{
	}

InputDeviceManager::~InputDeviceManager(void)
	{
	/* Delete all input device adapters: */
	for(int i=0;i<numInputDeviceAdapters;++i)
		delete inputDeviceAdapters[i];
	delete[] inputDeviceAdapters;
	}

void InputDeviceManager::initialize(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Retrieve the list of input device adapters: */
	typedef std::vector<std::string> StringList;
	StringList inputDeviceAdapterNames=configFileSection.retrieveValue<StringList>("./inputDeviceAdapterNames");
	numInputDeviceAdapters=inputDeviceAdapterNames.size();
	inputDeviceAdapters=new InputDeviceAdapter*[numInputDeviceAdapters];
	
	/* Initialize input device adapters: */
	int numIgnoredAdapters=0;
	for(int i=0;i<numInputDeviceAdapters;++i)
		{
		/* Go to input device adapter's section: */
		Misc::ConfigurationFileSection inputDeviceAdapterSection=configFileSection.getSection(inputDeviceAdapterNames[i].c_str());
		
		/* Determine input device adapter's type: */
		std::string inputDeviceAdapterType=inputDeviceAdapterSection.retrieveString("./inputDeviceAdapterType");
		bool typeFound=true;
		try
			{
			if(inputDeviceAdapterType=="Mouse")
				{
				/* Create mouse input device adapter: */
				inputDeviceAdapters[i]=new InputDeviceAdapterMouse(this,inputDeviceAdapterSection);
				}
			else if(inputDeviceAdapterType=="DeviceDaemon")
				{
				/* Create device daemon input device adapter: */
				inputDeviceAdapters[i]=new InputDeviceAdapterDeviceDaemon(this,inputDeviceAdapterSection);
				}
			else if(inputDeviceAdapterType=="VisBox")
				{
				/* Create VisBox input device adapter: */
				inputDeviceAdapters[i]=new InputDeviceAdapterVisBox(this,inputDeviceAdapterSection);
				}
			else if(inputDeviceAdapterType=="HID")
				{
				/* Create HID input device adapter: */
				inputDeviceAdapters[i]=new InputDeviceAdapterHID(this,inputDeviceAdapterSection);
				}
			else if(inputDeviceAdapterType=="Playback")
				{
				/* Create device daemon input device adapter: */
				inputDeviceAdapters[i]=new InputDeviceAdapterPlayback(this,inputDeviceAdapterSection);
				}
			else
				typeFound=false;
			}
		catch(std::runtime_error err)
			{
			/* Print a warning message: */
			std::cout<<"InputDeviceManager: Ignoring input device adapter "<<inputDeviceAdapterNames[i];
			std::cout<<" due to exception "<<err.what()<<std::endl;
			
			/* Ignore the input device adapter: */
			inputDeviceAdapters[i]=0;
			++numIgnoredAdapters;
			}
		
		if(!typeFound)
			Misc::throwStdErr("InputDeviceManager: Unknown input device adapter type \"%s\"",inputDeviceAdapterType.c_str());
		}
	
	if(numIgnoredAdapters!=0)
		{
		/* Remove any ignored input device adapters from the array: */
		InputDeviceAdapter** newInputDeviceAdapters=new InputDeviceAdapter*[numInputDeviceAdapters-numIgnoredAdapters];
		int newNumInputDeviceAdapters=0;
		for(int i=0;i<numInputDeviceAdapters;++i)
			if(inputDeviceAdapters[i]!=0)
				{
				newInputDeviceAdapters[newNumInputDeviceAdapters]=inputDeviceAdapters[i];
				++newNumInputDeviceAdapters;
				}
		delete[] inputDeviceAdapters;
		numInputDeviceAdapters=newNumInputDeviceAdapters;
		inputDeviceAdapters=newInputDeviceAdapters;
		}
	
	/* Check if there are any valid input device adapters: */
	if(numInputDeviceAdapters==0)
		Misc::throwStdErr("InputDeviceManager: No valid input device adapters found; I refuse to work under conditions like these!");
	}

InputDeviceAdapter* InputDeviceManager::findInputDeviceAdapter(InputDevice* device) const
	{
	/* Search all input device adapters: */
	for(int i=0;i<numInputDeviceAdapters;++i)
		{
		/* Search through all input devices: */
		for(int j=0;j<inputDeviceAdapters[i]->getNumInputDevices();++j)
			if(inputDeviceAdapters[i]->getInputDevice(j)==device)
				return inputDeviceAdapters[i];
		}
	
	return 0;
	}

InputDevice* InputDeviceManager::createInputDevice(const char* deviceName,int trackType,int numButtons,int numValuators,bool physicalDevice)
	{
	/* Get the length of the given device name's prefix: */
	int deviceNamePrefixLength=getPrefixLength(deviceName);
		
	/* Check if a device of the same name prefix already exists: */
	bool exists=false;
	int maxAliasIndex=0;
	for(InputDevices::iterator devIt=inputDevices.begin();devIt!=inputDevices.end();++devIt)
		{
		/* Compare the two prefixes: */
		if(getPrefixLength(devIt->getDeviceName())==deviceNamePrefixLength&&strncmp(deviceName,devIt->getDeviceName(),deviceNamePrefixLength)==0)
			{
			exists=true;
			if(devIt->getDeviceName()[deviceNamePrefixLength]==':')
				{
				int aliasIndex=atoi(devIt->getDeviceName()+deviceNamePrefixLength);
				if(maxAliasIndex<aliasIndex)
					maxAliasIndex=aliasIndex;
				}
			}
		}
	
	/* Create a new (uninitialized) input device: */
	InputDevices::iterator newDevice=inputDevices.insert(inputDevices.end(),InputDevice());
	InputDevice* newDevicePtr=&(*newDevice); // This looks iffy, but I don't know of a better way
	
	/* Initialize the new input device: */
	if(exists)
		{
		/* Create a new alias name for the new device: */
		char newDeviceNameBuffer[256];
		strncpy(newDeviceNameBuffer,deviceName,deviceNamePrefixLength);
		snprintf(newDeviceNameBuffer+deviceNamePrefixLength,sizeof(newDeviceNameBuffer)-deviceNamePrefixLength,":%d",maxAliasIndex+1);
		newDevicePtr->set(newDeviceNameBuffer,trackType,numButtons,numValuators);
		}
	else
		newDevicePtr->set(deviceName,trackType,numButtons,numValuators);
	
	/* Add the new input device to the input graph: */
	inputGraphManager->addInputDevice(newDevicePtr);
	
	/* If it's a physical device, grab it permanently: */
	if(physicalDevice)
		inputGraphManager->grabInputDevice(newDevicePtr,0); // Passing in null as grabber grabs for the physical layer
	
	/* Call the input device creation callbacks: */
	InputDeviceCreationCallbackData cbData(newDevicePtr);
	inputDeviceCreationCallbacks.call(&cbData);
	
	/* Return a pointer to the new input device: */
	return newDevicePtr;
	}

InputDevice* InputDeviceManager::getInputDevice(int deviceIndex)
	{
	InputDevices::iterator idIt;
	for(idIt=inputDevices.begin();idIt!=inputDevices.end()&&deviceIndex>0;++idIt,--deviceIndex)
		;
	if(idIt!=inputDevices.end())
		return &(*idIt);
	else
		return 0;
	}

InputDevice* InputDeviceManager::findInputDevice(const char* deviceName)
	{
	/* Search the given name in the list of devices: */
	InputDevice* result=0;
	for(InputDevices::iterator devIt=inputDevices.begin();devIt!=inputDevices.end();++devIt)
		if(strcmp(deviceName,devIt->getDeviceName())==0)
			{
			result=&(*devIt);
			break;
			}
	return result;
	}

void InputDeviceManager::destroyInputDevice(InputDevice* inputDevice)
	{
	/* Call the input device destruction callbacks: */
	InputDeviceDestructionCallbackData cbData(inputDevice);
	inputDeviceDestructionCallbacks.call(&cbData);
	
	/* Remove the device from the input graph: */
	inputGraphManager->removeInputDevice(inputDevice);
	
	/* Find the input device in the list: */
	for(InputDevices::iterator idIt=inputDevices.begin();idIt!=inputDevices.end();++idIt)
		if(&(*idIt)==inputDevice)
			{
			/* Delete it: */
			inputDevices.erase(idIt);
			break;
			}
	}

void InputDeviceManager::updateInputDevices(void)
	{
	/* Grab new data from all input device adapters: */
	for(int i=0;i<numInputDeviceAdapters;++i)
		inputDeviceAdapters[i]->updateInputDevices();
	}

}
