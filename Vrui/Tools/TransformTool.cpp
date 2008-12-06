/***********************************************************************
TransformTool - Base class for tools used to transform the position or
orientation of input devices.
Copyright (c) 2007-2008 Oliver Kreylos

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

#include <vector>
#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/TransformTool.h>

namespace Vrui {

/*************************************
Methods of class TransformToolFactory:
*************************************/

TransformToolFactory::TransformToolFactory(ToolManager& toolManager)
	:ToolFactory("TransformTool",toolManager),
	 numButtons(1),
	 buttonToggleFlags(0),
	 numValuators(0)
	{
	#if 0
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("Tool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	#endif
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	numButtons=cfs.retrieveValue<int>("./numButtons",numButtons);
	buttonToggleFlags=new bool[numButtons];
	for(int i=0;i<numButtons;++i)
		buttonToggleFlags[i]=false;
	std::vector<int> toggleButtonIndices=cfs.retrieveValue<std::vector<int> >("./toggleButtonIndices",std::vector<int>());
	for(std::vector<int>::const_iterator tbiIt=toggleButtonIndices.begin();tbiIt!=toggleButtonIndices.end();++tbiIt)
		{
		if(*tbiIt<0||*tbiIt>=numButtons)
			Misc::throwStdErr("TransformToolFactory::TransformToolFactory: Toggle button index out of valid range");
		buttonToggleFlags[*tbiIt]=true;
		}
	numValuators=cfs.retrieveValue<int>("./numValuators",numValuators);
	
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,numButtons);
	layout.setNumValuators(0,numValuators);
	
	/* Set tool class' factory pointer: */
	TransformTool::factory=this;
	}

TransformToolFactory::~TransformToolFactory(void)
	{
	delete[] buttonToggleFlags;
	}

extern "C" ToolFactory* createTransformToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	TransformToolFactory* navigationToolFactory=new TransformToolFactory(*toolManager);
	
	/* Return factory object: */
	return navigationToolFactory;
	}

extern "C" void destroyTransformToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/**************************************
Static elements of class TransformTool:
**************************************/

TransformToolFactory* TransformTool::factory=0;

/******************************
Methods of class TransformTool:
******************************/

bool TransformTool::setButtonState(int buttonIndex,bool newButtonState)
	{
	bool result=false;
	
	if(factory->buttonToggleFlags[buttonIndex])
		{
		if(!newButtonState)
			{
			result=true;
			buttonStates[buttonIndex]=!buttonStates[buttonIndex];
			}
		}
	else
		{
		result=buttonStates[buttonIndex]!=newButtonState;
		buttonStates[buttonIndex]=newButtonState;
		}
	
	return result;
	}

TransformTool::TransformTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:Tool(sFactory,inputAssignment),
	 transformedDevice(0),
	 buttonStates(0)
	{
	/* Initialize the button states array: */
	buttonStates=new bool[factory->numButtons];
	for(int i=0;i<factory->numButtons;++i)
		buttonStates[i]=false;
	}

TransformTool::~TransformTool(void)
	{
	delete[] buttonStates;
	}

void TransformTool::initialize(void)
	{
	/* Create a virtual input device to shadow the source input device: */
	transformedDevice=addVirtualInputDevice("TransformedDevice",factory->numButtons,factory->numValuators);
	
	/* Set the virtual device's glyph to the source device's glyph: */
	getInputGraphManager()->getInputDeviceGlyph(transformedDevice)=getInputGraphManager()->getInputDeviceGlyph(input.getDevice(0));
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(transformedDevice,this);
	
	/* Initialize the virtual input device's position: */
	transformedDevice->setTransformation(input.getDevice(0)->getTransformation());
	}

void TransformTool::deinitialize(void)
	{
	/* Release the virtual input device: */
	getInputGraphManager()->releaseInputDevice(transformedDevice,this);
	
	/* Destroy the virtual input device: */
	getInputDeviceManager()->destroyInputDevice(transformedDevice);
	}

const ToolFactory* TransformTool::getFactory(void) const
	{
	return factory;
	}

void TransformTool::buttonCallback(int,int deviceButtonIndex,InputDevice::ButtonCallbackData* cbData)
	{
	/* Set the new button state and forward to the transformed device if it changed: */
	if(setButtonState(deviceButtonIndex,cbData->newButtonState))
		transformedDevice->setButtonState(deviceButtonIndex,buttonStates[deviceButtonIndex]);
	}

void TransformTool::valuatorCallback(int,int deviceValuatorIndex,InputDevice::ValuatorCallbackData* cbData)
	{
	/* Forward the new valuator state to the transformed device: */
	transformedDevice->setValuator(deviceValuatorIndex,cbData->newValuatorValue);
	}

void TransformTool::frame(void)
	{
	/* Set the transformed device's position and orientation: */
	transformedDevice->setTransformation(input.getDevice(0)->getTransformation());
	}

}
