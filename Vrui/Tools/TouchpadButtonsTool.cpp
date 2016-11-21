/***********************************************************************
TouchpadButtonsTool - Transform a clickable touchpad or analog stick to
multiple buttons arranged around a circle.
Copyright (c) 2016 Oliver Kreylos

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

#include <Vrui/Tools/TouchpadButtonsTool.h>

#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/**********************************************************
Methods of class TouchpadButtonsToolFactory::Configuration:
**********************************************************/

TouchpadButtonsToolFactory::Configuration::Configuration(void)
	:numButtons(4),centerRadius(0.5),useCenterButton(false)
	{
	}

void TouchpadButtonsToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	numButtons=cfs.retrieveValue<int>("./numButtons",numButtons);
	centerRadius=cfs.retrieveValue<double>("./centerRadius",centerRadius);
	useCenterButton=cfs.retrieveValue<bool>("./useCenterButton",useCenterButton);
	}

void TouchpadButtonsToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue<int>("./numButtons",numButtons);
	cfs.storeValue<double>("./centerRadius",centerRadius);
	cfs.storeValue<bool>("./useCenterButton",useCenterButton);
	}

/*******************************************
Methods of class TouchpadButtonsToolFactory:
*******************************************/

TouchpadButtonsToolFactory::TouchpadButtonsToolFactory(ToolManager& toolManager)
	:ToolFactory("TouchpadButtonsTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	layout.setNumValuators(2);
	
	/* Insert class into class hierarchy: */
	TransformToolFactory* transformToolFactory=dynamic_cast<TransformToolFactory*>(toolManager.loadClass("TransformTool"));
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	TouchpadButtonsTool::factory=this;
	}

TouchpadButtonsToolFactory::~TouchpadButtonsToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	TouchpadButtonsTool::factory=0;
	}

const char* TouchpadButtonsToolFactory::getName(void) const
	{
	return "Touchpad -> Buttons";
	}

const char* TouchpadButtonsToolFactory::getButtonFunction(int) const
	{
	return "Press Button";
	}

const char* TouchpadButtonsToolFactory::getValuatorFunction(int valuatorSlotIndex) const
	{
	static const char* valuatorNames[2]=
		{
		"Touchpad X Axis","Touchpad Y Axis"
		};
	return valuatorNames[valuatorSlotIndex];
	}

Tool* TouchpadButtonsToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new TouchpadButtonsTool(this,inputAssignment);
	}

void TouchpadButtonsToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveTouchpadButtonsToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createTouchpadButtonsToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	TouchpadButtonsToolFactory* touchpadButtonsToolFactory=new TouchpadButtonsToolFactory(*toolManager);
	
	/* Return factory object: */
	return touchpadButtonsToolFactory;
	}

extern "C" void destroyTouchpadButtonsToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/********************************************
Static elements of class TouchpadButtonsTool:
********************************************/

TouchpadButtonsToolFactory* TouchpadButtonsTool::factory=0;

/************************************
Methods of class TouchpadButtonsTool:
************************************/

TouchpadButtonsTool::TouchpadButtonsTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:TransformTool(factory,inputAssignment),
	 configuration(TouchpadButtonsTool::factory->configuration),
	 pressedButton(-1)
	{
	/* Set the transformation source device: */
	sourceDevice=getButtonDevice(0);
	}

TouchpadButtonsTool::~TouchpadButtonsTool(void)
	{
	}

void TouchpadButtonsTool::initialize(void)
	{
	/* Create a virtual input device to shadow the source input device: */
	int numButtons=configuration.numButtons;
	if(configuration.useCenterButton)
		++numButtons;
	transformedDevice=addVirtualInputDevice("TouchpadButtonsToolTransformedDevice",numButtons,0);
	
	/* Copy the source device's tracking type: */
	transformedDevice->setTrackType(sourceDevice->getTrackType());
	
	/* Disable the virtual input device's glyph: */
	getInputGraphManager()->getInputDeviceGlyph(transformedDevice).disable();
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(transformedDevice,this);
	
	/* Initialize the virtual input device's position: */
	resetDevice();
	}

const ToolFactory* TouchpadButtonsTool::getFactory(void) const
	{
	return factory;
	}

void TouchpadButtonsTool::buttonCallback(int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState)
		{
		/* Determine which button to press: */
		double x=getValuatorState(0);
		double y=getValuatorState(1);
		double r2=x*x+y*y;
		if(r2>=Math::sqr(configuration.centerRadius))
			{
			/* Calculate the angle of the touchpad contact: */
			double angle=Math::atan2(x,y);
			if(angle<0.0)
				angle+=2.0*Math::Constants<double>::pi;
			double anglePerButton=2.0*Math::Constants<double>::pi/double(configuration.numButtons);
			pressedButton=int(Math::floor(angle/anglePerButton+0.5))%configuration.numButtons;
			transformedDevice->setButtonState(pressedButton,true);
			}
		else if(configuration.useCenterButton)
			{
			/* Press the center button: */
			pressedButton=configuration.numButtons;
			transformedDevice->setButtonState(pressedButton,true);
			}
		}
	else if(pressedButton>=0)
		{
		/* Release the currently pressed button: */
		transformedDevice->setButtonState(pressedButton,false);
		pressedButton=-1;
		}
	}

InputDeviceFeatureSet TouchpadButtonsTool::getSourceFeatures(const InputDeviceFeature& forwardedFeature)
	{
	/* Paranoia: Check if the forwarded feature is on the transformed device: */
	if(forwardedFeature.getDevice()!=transformedDevice)
		Misc::throwStdErr("TouchpadButtonsTool::getSourceFeatures: Forwarded feature is not on transformed device");
	
	/* Return all input feature slots: */
	InputDeviceFeatureSet result;
	result.push_back(input.getButtonSlotFeature(0));
	for(int i=0;i<2;++i)
		result.push_back(input.getValuatorSlotFeature(i));
	
	return result;
	}

InputDeviceFeatureSet TouchpadButtonsTool::getForwardedFeatures(const InputDeviceFeature& sourceFeature)
	{
	/* Find the input assignment slot for the given feature: */
	int slotIndex=input.findFeature(sourceFeature);
	
	/* Check if the source feature belongs to this tool: */
	if(slotIndex<0)
		Misc::throwStdErr("TouchpadButtonsTool::getForwardedFeatures: Source feature is not part of tool's input assignment");
	
	/* Return all forwarded buttons: */
	InputDeviceFeatureSet result;
	for(int i=0;i<configuration.numButtons;++i)
		result.push_back(InputDeviceFeature(transformedDevice,InputDevice::BUTTON,i));
	if(configuration.useCenterButton)
		result.push_back(InputDeviceFeature(transformedDevice,InputDevice::BUTTON,configuration.numButtons));
	
	return result;
	}

}
