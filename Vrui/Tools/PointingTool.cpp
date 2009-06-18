/***********************************************************************
PointingTool - Base class for tools used to point at positions or
features in a virtual environment.
Copyright (c) 2009 Oliver Kreylos

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

#include <Vrui/ToolManager.h>

#include <Vrui/Tools/PointingTool.h>

namespace Vrui {

/************************************
Methods of class PointingToolFactory:
************************************/

PointingToolFactory::PointingToolFactory(ToolManager& toolManager)
	:ToolFactory("PointingTool",toolManager)
	{
	#if 0
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("Tool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	#endif
	}

const char* PointingToolFactory::getName(void) const
	{
	return "Pointer";
	}

extern "C" ToolFactory* createPointingToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	PointingToolFactory* pointingToolFactory=new PointingToolFactory(*toolManager);
	
	/* Return factory object: */
	return pointingToolFactory;
	}

extern "C" void destroyPointingToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/*****************************
Methods of class PointingTool:
*****************************/

PointingTool::PointingTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:Tool(factory,inputAssignment)
	{
	}

}
