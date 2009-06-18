/***********************************************************************
SixDofInputDeviceTool - Class for tools using a 6-DOF input device to
interact with virtual input devices.
Copyright (c) 2004-2009 Oliver Kreylos

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

#include <Misc/CallbackList.h>
#include <Vrui/InputDevice.h>
#include <Vrui/ToolManager.h>

#include <Vrui/Tools/SixDofInputDeviceTool.h>

namespace Vrui {

/*********************************************
Methods of class SixDofInputDeviceToolFactory:
*********************************************/

SixDofInputDeviceToolFactory::SixDofInputDeviceToolFactory(ToolManager& toolManager)
	:ToolFactory("SixDofInputDeviceTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* inputDeviceToolFactory=toolManager.loadClass("InputDeviceTool");
	inputDeviceToolFactory->addChildClass(this);
	addParentClass(inputDeviceToolFactory);
	
	/* Set tool class' factory pointer: */
	SixDofInputDeviceTool::factory=this;
	}

SixDofInputDeviceToolFactory::~SixDofInputDeviceToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	SixDofInputDeviceTool::factory=0;
	}

const char* SixDofInputDeviceToolFactory::getName(void) const
	{
	return "6-DOF Driver";
	}

Tool* SixDofInputDeviceToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new SixDofInputDeviceTool(this,inputAssignment);
	}

void SixDofInputDeviceToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveSixDofInputDeviceToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("InputDeviceTool");
	}

extern "C" ToolFactory* createSixDofInputDeviceToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	SixDofInputDeviceToolFactory* sixDofInputDeviceToolFactory=new SixDofInputDeviceToolFactory(*toolManager);
	
	/* Return factory object: */
	return sixDofInputDeviceToolFactory;
	}

extern "C" void destroySixDofInputDeviceToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/**********************************************
Static elements of class SixDofInputDeviceTool:
**********************************************/

SixDofInputDeviceToolFactory* SixDofInputDeviceTool::factory=0;

/**************************************
Methods of class SixDofInputDeviceTool:
**************************************/

SixDofInputDeviceTool::SixDofInputDeviceTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:InputDeviceTool(sFactory,inputAssignment),
	 deactivating(false)
	{
	}

const ToolFactory* SixDofInputDeviceTool::getFactory(void) const
	{
	return factory;
	}

void SixDofInputDeviceTool::buttonCallback(int,int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		if(isActive())
			{
			/* Prepare to deactivate the tool on button release: */
			deactivating=true;
			
			/* Cancel processing of this callback to preempt cascaded tools: */
			cbData->callbackList->requestInterrupt();
			}
		else
			{
			/* Try activating the tool: */
			if(activate(getDevicePosition(0)))
				{
				/* Initialize the dragging transformation: */
				preScale=Geometry::invert(getDeviceTransformation(0));
				preScale*=getGrabbedDevice()->getTransformation();
				
				/* Cancel processing of this callback to preempt cascaded tools: */
				cbData->callbackList->requestInterrupt();
				}
			}
		}
	else // Button has just been released
		{
		if(isActive())
			{
			if(deactivating)
				{
				/* Deactivate the tool: */
				deactivate();
				deactivating=false;
				}
			
			/* Cancel processing of this callback to preempt cascaded tools: */
			cbData->callbackList->requestInterrupt();
			}
		}
	}

void SixDofInputDeviceTool::frame(void)
	{
	if(isActive())
		{
		/* Calculate the current transformation: */
		TrackerState current=getDeviceTransformation(0);
		current*=preScale;
		
		/* Set the grabbed device's position and orientation: */
		getGrabbedDevice()->setTransformation(current);
		}
	}

}
