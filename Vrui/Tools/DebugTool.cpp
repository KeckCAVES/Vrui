/***********************************************************************
DebugTool - Tool class to futz with internal Vrui state for debugging
purposes.
Copyright (c) 2018 Oliver Kreylos

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

#include <Vrui/Tools/DebugTool.h>

#include <Vrui/ToolManager.h>

namespace Vrui {

/* Debugging variables: */
extern bool deviceDaemonPredictOnUpdate;
extern bool lensCorrectorDisableReproject;

/*********************************
Methods of class DebugToolFactory:
*********************************/

DebugToolFactory::DebugToolFactory(ToolManager& toolManager)
	:ToolFactory("DebugTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	
	#if 0
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("Tool");
	toolFactory->addChildClass(this);
	addParentClass(0);
	#endif
	
	/* Set tool class' factory pointer: */
	DebugTool::factory=this;
	}

DebugToolFactory::~DebugToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	DebugTool::factory=0;
	}

const char* DebugToolFactory::getName(void) const
	{
	return "Toggle Debug Options";
	}

Tool* DebugToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new DebugTool(this,inputAssignment);
	}

void DebugToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveDebugToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	#if 0
	/* Load base classes: */
	manager.loadClass("Tool");
	#endif
	}

extern "C" ToolFactory* createDebugToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	DebugToolFactory* debugToolFactory=new DebugToolFactory(*toolManager);
	
	/* Return factory object: */
	return debugToolFactory;
	}

extern "C" void destroyDebugToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/**********************************
Static elements of class DebugTool:
**********************************/

DebugToolFactory* DebugTool::factory=0;

/**************************
Methods of class DebugTool:
**************************/

DebugTool::DebugTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:Tool(factory,inputAssignment)
	{
	/* Disable input device motion prediction and warp reprojection: */
	deviceDaemonPredictOnUpdate=false;
	lensCorrectorDisableReproject=true;
	}

DebugTool::~DebugTool(void)
	{
	/* Enable input device motion prediction and warp reprojection: */
	deviceDaemonPredictOnUpdate=true;
	lensCorrectorDisableReproject=false;
	}

const ToolFactory* DebugTool::getFactory(void) const
	{
	return factory;
	}

void DebugTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	switch(buttonSlotIndex)
		{
		case 0:
			if(cbData->newButtonState)
				{
				/* Enable warp reprojection: */
				lensCorrectorDisableReproject=false;
				}
			else
				{
				/* Disable warp reprojection: */
				lensCorrectorDisableReproject=true;
				}
			break;
		}
	}

}
