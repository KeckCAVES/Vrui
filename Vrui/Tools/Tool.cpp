/***********************************************************************
Tool - Abstract base class for user interaction tools (navigation, menu
selection, selection, etc.).
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

#include <string.h>
#include <Misc/ThrowStdErr.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/Tool.h>

namespace Vrui {

/****************************
Methods of class ToolFactory:
****************************/

ToolFactory::ToolFactory(const char* sClassName,ToolManager&)
	:Factory(sClassName)
	{
	}

Tool* ToolFactory::createTool(const ToolInputAssignment&) const
	{
	Misc::throwStdErr("Cannot create tool of abstract class %s",getClassName());
	
	/* Dummy statement to make the compiler happy: */
	return 0;
	}

void ToolFactory::destroyTool(Tool*) const
	{
	Misc::throwStdErr("Cannot destroy tool of abstract class %s",getClassName());
	}

/*********************
Methods of class Tool:
*********************/

void Tool::buttonCallbackWrapper(Misc::CallbackData* cbData,void* userData)
	{
	/* Determine callback target tool: */
	Tool* thisPtr=static_cast<Tool*>(userData);
	
	/* Determine device and button index: */
	InputDevice::ButtonCallbackData* bcbData=static_cast<InputDevice::ButtonCallbackData*>(cbData);
	int deviceIndex;
	for(deviceIndex=0;deviceIndex<thisPtr->layout.getNumDevices();++deviceIndex)
		if(thisPtr->input.getDevice(deviceIndex)==bcbData->inputDevice)
			break;
	if(deviceIndex>=thisPtr->layout.getNumDevices()) // Is the callback really for this tool?
		return;
	int buttonIndex;
	for(buttonIndex=0;buttonIndex<thisPtr->layout.getNumButtons(deviceIndex);++buttonIndex)
		if(thisPtr->input.getButtonIndex(deviceIndex,buttonIndex)==bcbData->buttonIndex)
			break;
	if(buttonIndex>=thisPtr->layout.getNumButtons(deviceIndex)) // Is the callback really for this tool?
		return;
	
	/* Call the tool's callback method: */
	thisPtr->buttonCallback(deviceIndex,buttonIndex,bcbData);
	}

void Tool::valuatorCallbackWrapper(Misc::CallbackData* cbData,void* userData)
	{
	/* Determine callback target tool: */
	Tool* thisPtr=static_cast<Tool*>(userData);
	
	/* Determine device and valuator index: */
	InputDevice::ValuatorCallbackData* vcbData=static_cast<InputDevice::ValuatorCallbackData*>(cbData);
	int deviceIndex;
	for(deviceIndex=0;deviceIndex<thisPtr->layout.getNumDevices();++deviceIndex)
		if(thisPtr->input.getDevice(deviceIndex)==vcbData->inputDevice)
			break;
	if(deviceIndex>=thisPtr->layout.getNumDevices()) // Is the callback really for this tool?
		return;
	int valuatorIndex;
	for(valuatorIndex=0;valuatorIndex<thisPtr->layout.getNumValuators(deviceIndex);++valuatorIndex)
		if(thisPtr->input.getValuatorIndex(deviceIndex,valuatorIndex)==vcbData->valuatorIndex)
			break;
	if(valuatorIndex>=thisPtr->layout.getNumValuators(deviceIndex)) // Is the callback really for this tool?
		return;
	
	/* Call the tool's callback method: */
	thisPtr->valuatorCallback(deviceIndex,valuatorIndex,vcbData);
	}

Tool::Tool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:layout(factory->getLayout()),
	 input(layout)
	{
	/* Set the tool's input assignment based on the given assignment: */
	for(int deviceIndex=0;deviceIndex<layout.getNumDevices();++deviceIndex)
		{
		/* Set the device: */
		InputDevice* device=inputAssignment.getDevice(deviceIndex);
		input.setDevice(deviceIndex,device);
		
		/* Set all buttons on the device: */
		for(int buttonIndex=0;buttonIndex<layout.getNumButtons(deviceIndex);++buttonIndex)
			{
			/* Set new button index: */
			int deviceButtonIndex=inputAssignment.getButtonIndex(deviceIndex,buttonIndex);
			input.setButtonIndex(deviceIndex,buttonIndex,deviceButtonIndex);
			
			/* Register button callback for new button: */
			device->getButtonCallbacks(deviceButtonIndex).add(buttonCallbackWrapper,this);
			}
		
		/* Set all valuators on the device: */
		for(int valuatorIndex=0;valuatorIndex<layout.getNumValuators(deviceIndex);++valuatorIndex)
			{
			/* Set new valuator index: */
			int deviceValuatorIndex=inputAssignment.getValuatorIndex(deviceIndex,valuatorIndex);
			input.setValuatorIndex(deviceIndex,valuatorIndex,deviceValuatorIndex);
			
			/* Register valuator callback for new valuator: */
			device->getValuatorCallbacks(deviceValuatorIndex).add(valuatorCallbackWrapper,this);
			}
		}
	}

Tool::~Tool(void)
	{
	/* Clear all input device assignments: */
	for(int device=0;device<layout.getNumDevices();++device)
		assignInputDevice(device,0);
	}

void Tool::initialize(void)
	{
	}

void Tool::deinitialize(void)
	{
	}

const ToolFactory* Tool::getFactory(void) const
	{
	Misc::throwStdErr("Tool::getFactory: Tool of abstract class does not have factory object");
	
	/* Just to make compiler happy: */
	return 0;
	}

void Tool::assignInputDevice(int deviceIndex,InputDevice* newAssignedDevice)
	{
	/* Clear all button assignments for the affected device: */
	for(int i=0;i<layout.getNumButtons(deviceIndex);++i)
		assignButton(deviceIndex,i,-1);
	
	/* Clear all valuator assignments for the affected device: */
	for(int i=0;i<layout.getNumValuators(deviceIndex);++i)
		assignValuator(deviceIndex,i,-1);
	
	/* Assign new input device: */
	input.setDevice(deviceIndex,newAssignedDevice);
	}

void Tool::assignButton(int deviceIndex,int deviceButtonIndex,int newAssignedButtonIndex)
	{
	/* Unregister button callback from previous button: */
	if(input.getButtonIndex(deviceIndex,deviceButtonIndex)>=0)
		input.getDevice(deviceIndex)->getButtonCallbacks(input.getButtonIndex(deviceIndex,deviceButtonIndex)).remove(buttonCallbackWrapper,this);
	
	/* Set new button index: */
	input.setButtonIndex(deviceIndex,deviceButtonIndex,newAssignedButtonIndex);
	
	/* Register button callback for new button: */
	if(newAssignedButtonIndex>=0)
		input.getDevice(deviceIndex)->getButtonCallbacks(newAssignedButtonIndex).add(buttonCallbackWrapper,this);
	}

void Tool::assignValuator(int deviceIndex,int deviceValuatorIndex,int newAssignedValuatorIndex)
	{
	/* Unregister valuator callback from previous valuator: */
	if(input.getValuatorIndex(deviceIndex,deviceValuatorIndex)>=0)
		input.getDevice(deviceIndex)->getValuatorCallbacks(input.getValuatorIndex(deviceIndex,deviceValuatorIndex)).remove(valuatorCallbackWrapper,this);
	
	/* Set new valuator index: */
	input.setValuatorIndex(deviceIndex,deviceValuatorIndex,newAssignedValuatorIndex);
	
	/* Register valuator callback for new valuator: */
	if(newAssignedValuatorIndex>=0)
		input.getDevice(deviceIndex)->getValuatorCallbacks(newAssignedValuatorIndex).add(valuatorCallbackWrapper,this);
	}

void Tool::buttonCallback(int,int,InputDevice::ButtonCallbackData*)
	{
	}

void Tool::valuatorCallback(int,int,InputDevice::ValuatorCallbackData*)
	{
	}

void Tool::frame(void)
	{
	}

void Tool::display(GLContextData&) const
	{
	}

}
