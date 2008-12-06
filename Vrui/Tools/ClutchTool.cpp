/***********************************************************************
ClutchTool - Class to offset the position and orientation of an input
device using a "clutch" button to disengage a virtual device from a
source device.
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

#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/ClutchTool.h>

namespace Vrui {

/**********************************
Methods of class ClutchToolFactory:
**********************************/

ClutchToolFactory::ClutchToolFactory(ToolManager& toolManager)
	:ToolFactory("ClutchTool",toolManager),
	 clutchButtonToggleFlag(false)
	{
	/* Insert class into class hierarchy: */
	TransformToolFactory* transformToolFactory=dynamic_cast<TransformToolFactory*>(toolManager.loadClass("TransformTool"));
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	clutchButtonToggleFlag=cfs.retrieveValue<bool>("./clutchButtonToggleFlag",clutchButtonToggleFlag);
	
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,transformToolFactory->getNumButtons()+1);
	layout.setNumValuators(0,transformToolFactory->getNumValuators());
	
	/* Set tool class' factory pointer: */
	ClutchTool::factory=this;
	}

ClutchToolFactory::~ClutchToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	ClutchTool::factory=0;
	}

Tool* ClutchToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new ClutchTool(this,inputAssignment);
	}

void ClutchToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveClutchToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createClutchToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	ClutchToolFactory* mouseToolFactory=new ClutchToolFactory(*toolManager);
	
	/* Return factory object: */
	return mouseToolFactory;
	}

extern "C" void destroyClutchToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/**********************************
Static elements of class ClutchTool:
**********************************/

ClutchToolFactory* ClutchTool::factory=0;

/**************************
Methods of class ClutchTool:
**************************/

ClutchTool::ClutchTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:TransformTool(factory,inputAssignment),
	 last(input.getDevice(0)->getTransformation()),
	 clutchButtonState(false)
	{
	}

ClutchTool::~ClutchTool(void)
	{
	}

const ToolFactory* ClutchTool::getFactory(void) const
	{
	return factory;
	}

void ClutchTool::buttonCallback(int,int deviceButtonIndex,InputDevice::ButtonCallbackData* cbData)
	{
	if(deviceButtonIndex==0) // Clutch button
		{
		bool mustInit=false;
		if(factory->clutchButtonToggleFlag)
			{
			if(!cbData->newButtonState)
				{
				clutchButtonState=!clutchButtonState;
				mustInit=!clutchButtonState;
				}
			}
		else
			{
			clutchButtonState=cbData->newButtonState;
			mustInit=!clutchButtonState;
			}
		
		if(mustInit)
			{
			/* Remember the current input device transformation: */
			last=input.getDevice(0)->getTransformation();
			}
		}
	else // Pass-through button
		{
		if(setButtonState(deviceButtonIndex-1,cbData->newButtonState))
			transformedDevice->setButtonState(deviceButtonIndex-1,buttonStates[deviceButtonIndex-1]);
		}
	}

void ClutchTool::frame(void)
	{
	if(!clutchButtonState)
		{
		/* Calculate the source device's incremental transformation: */
		const TrackerState& current=input.getDevice(0)->getTransformation();
		TrackerState delta=current*Geometry::invert(last);
		last=current;
		
		/* Apply the incremental transformation to the transformed device: */
		TrackerState clutch=transformedDevice->getTransformation();
		clutch.leftMultiply(delta);
		clutch.renormalize();
		transformedDevice->setTransformation(clutch);
		}
	}

}
