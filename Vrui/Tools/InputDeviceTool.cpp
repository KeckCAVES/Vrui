/***********************************************************************
InputDeviceTool - Base class for tools used to interact with virtual
input devices.
Copyright (c) 2004-2008 Oliver Kreylos

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

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/CallbackList.h>
#include <Geometry/Point.h>
#include <Geometry/Ray.h>
#include <Vrui/InputDevice.h>
#include <Vrui/VirtualInputDevice.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/InputDeviceTool.h>

namespace Vrui {

/***************************************
Methods of class InputDeviceToolFactory:
***************************************/

InputDeviceToolFactory::InputDeviceToolFactory(ToolManager& toolManager)
	:ToolFactory("InputDeviceTool",toolManager),
	 createInputDevice(false),
	 newDeviceNumButtons(1),
	 virtualInputDevice(getVirtualInputDevice())
	{
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("UserInterfaceTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	createInputDevice=cfs.retrieveValue<bool>("./createInputDevice",createInputDevice);
	newDeviceNumButtons=cfs.retrieveValue<int>("./newDeviceNumButtons",newDeviceNumButtons);
	
	/* Set tool class' factory pointer: */
	InputDeviceTool::factory=this;
	}

InputDeviceToolFactory::~InputDeviceToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	InputDeviceTool::factory=0;
	}

extern "C" void resolveInputDeviceToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("UserInterfaceTool");
	}

extern "C" ToolFactory* createInputDeviceToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	InputDeviceToolFactory* inputDeviceToolFactory=new InputDeviceToolFactory(*toolManager);
	
	/* Return factory object: */
	return inputDeviceToolFactory;
	}

extern "C" void destroyInputDeviceToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/************************************************
Methods of class InputDeviceTool::ButtonHijacker:
************************************************/

InputDeviceTool::ButtonHijacker::ButtonHijacker(void)
	:targetDevice(0),buttonIndex(-1)
	{
	}

void InputDeviceTool::ButtonHijacker::buttonCallbackWrapper(Misc::CallbackData* cbData,void* userData)
	{
	/* Get a pointer to the hijacker structure: */
	ButtonHijacker* buttonHijacker=static_cast<ButtonHijacker*>(userData);
	
	/* Get a pointer to the real callback data structure: */
	InputDevice::ButtonCallbackData* cbData2=static_cast<InputDevice::ButtonCallbackData*>(cbData);
	
	/* Set the target button state on the target device: */
	buttonHijacker->targetDevice->setButtonState(buttonHijacker->buttonIndex,cbData2->newButtonState);
	
	/* Cancel processing of this callback: */
	cbData->callbackList->requestInterrupt();
	}

/****************************************
Static elements of class InputDeviceTool:
****************************************/

InputDeviceToolFactory* InputDeviceTool::factory=0;

/********************************
Methods of class InputDeviceTool:
********************************/

void InputDeviceTool::hijackButtons(void)
	{
	/* Hijack all buttons on the tool's device: */
	InputDevice* sourceDevice=input.getDevice(0);
	int targetButtonIndex=0;
	for(int i=0;targetButtonIndex<grabbedDevice->getNumButtons()&&i<sourceDevice->getNumButtons();++i)
		if(i!=input.getButtonIndex(0,0))
			{
			/* Set the hijacker's state: */
			buttonHijackers[i].targetDevice=grabbedDevice;
			buttonHijackers[i].buttonIndex=targetButtonIndex;
			
			/* Set the target device's button state to the source device's state: */
			grabbedDevice->setButtonState(targetButtonIndex,sourceDevice->getButtonState(i));
			
			/* Install the overriding callback: */
			sourceDevice->getButtonCallbacks(i).addToFront(ButtonHijacker::buttonCallbackWrapper,&buttonHijackers[i]);
			++targetButtonIndex;
			}
	}

void InputDeviceTool::releaseButtons(void)
	{
	/* Release all hijacked buttons on the tool's device: */
	InputDevice* sourceDevice=input.getDevice(0);
	for(int i=0;i<sourceDevice->getNumButtons();++i)
		if(buttonHijackers[i].targetDevice!=0)
			{
			/* Remove the overriding callback: */
			sourceDevice->getButtonCallbacks(i).remove(ButtonHijacker::buttonCallbackWrapper,&buttonHijackers[i]);
			
			/* Reset the hijacker's state: */
			buttonHijackers[i].targetDevice=0;
			buttonHijackers[i].buttonIndex=-1;
			}
	}

bool InputDeviceTool::activate(const Point& position)
	{
	/* Find an ungrabbed input device at the given position: */
	InputDevice* device=getInputGraphManager()->findInputDevice(position);
	if(device!=0)
		{
		/* Check if the event was meant for one of the input device's buttons: */
		int buttonIndex=getVirtualInputDevice()->pickButton(device,position);
		if(buttonIndex>=0)
			{
			/* Toggle the input device's button state: */
			device->setButtonState(buttonIndex,!device->getButtonState(buttonIndex));
			}
		else if(getInputGraphManager()->grabInputDevice(device,this))
			{
			/* Activate the tool: */
			active=true;
			grabbedDevice=device;
			
			/* Hijack buttons on the tool's device: */
			//hijackButtons();
			}
		}
	
	return active;
	}

bool InputDeviceTool::activate(const Ray& ray)
	{
	/* Find an ungrabbed input device with the given ray: */
	InputDevice* device=getInputGraphManager()->findInputDevice(ray);
	if(device!=0)
		{
		/* Check if the event was meant for one of the input device's buttons: */
		int buttonIndex=getVirtualInputDevice()->pickButton(device,ray);
		if(buttonIndex>=0)
			{
			/* Toggle the input device's button state: */
			device->setButtonState(buttonIndex,!device->getButtonState(buttonIndex));
			}
		else if(getInputGraphManager()->grabInputDevice(device,this))
			{
			/* Activate the tool: */
			active=true;
			grabbedDevice=device;
			
			/* Hijack buttons on the tool's device: */
			//hijackButtons();
			}
		}
	
	return active;
	}

void InputDeviceTool::deactivate(void)
	{
	if(active)
		{
		/* Release hijacked buttons on the tool's device: */
		//releaseButtons();
		
		/* Release the grabbed input device: */
		getInputGraphManager()->releaseInputDevice(grabbedDevice,this);
		
		/* Deactivate the tool: */
		active=false;
		grabbedDevice=0;
		}
	}

bool InputDeviceTool::grabNextDevice(void)
	{
	if(active)
		{
		/* Release hijacked buttons on the tool's device: */
		//releaseButtons();
		
		/* Release the grabbed input device: */
		getInputGraphManager()->releaseInputDevice(grabbedDevice,this);
		
		/* Deactivate the tool: */
		active=false;
		}
	
	/* Try grabbing the next device: */
	if(grabbedDevice==0)
		grabbedDevice=getInputGraphManager()->getFirstInputDevice();
	else
		grabbedDevice=getInputGraphManager()->getNextInputDevice(grabbedDevice);
	if(grabbedDevice!=0&&getInputGraphManager()->grabInputDevice(grabbedDevice,this))
		{
		/* Activate the tool: */
		active=true;
		
		/* Hijack buttons on the tool's device: */
		//hijackButtons();
		}
	else
		grabbedDevice=0;
	
	return active;
	}

InputDeviceTool::InputDeviceTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:UserInterfaceTool(sFactory,inputAssignment),
	 createdDevice(0),
	 buttonHijackers(0),
	 active(false),grabbedDevice(0)
	{
	/* Create the array of button hijackers: */
	buttonHijackers=new ButtonHijacker[input.getDevice(0)->getNumButtons()];
	}

InputDeviceTool::~InputDeviceTool(void)
	{
	/* Delete the array of button hijackers: */
	delete[] buttonHijackers;
	}

void InputDeviceTool::initialize(void)
	{
	/* Create a new unbound input device if selected: */
	if(factory->createInputDevice)
		createdDevice=addVirtualInputDevice("InputDeviceToolDevice",factory->newDeviceNumButtons,0);
	}

void InputDeviceTool::deinitialize(void)
	{
	/* Deactivate the tool if it is still active: */
	if(active)
		deactivate();
	
	/* Delete any created input devices: */
	if(createdDevice!=0)
		getInputDeviceManager()->destroyInputDevice(createdDevice);
	}

const ToolFactory* InputDeviceTool::getFactory(void) const
	{
	return factory;
	}

}
