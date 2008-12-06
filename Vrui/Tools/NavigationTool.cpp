/***********************************************************************
NavigationTool - Base class for navigation tools.
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

#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/NavigationTool.h>

namespace Vrui {

/**************************************
Methods of class NavigationToolFactory:
**************************************/

NavigationToolFactory::NavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("NavigationTool",toolManager)
	{
	#if 0
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("Tool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	#endif
	}

extern "C" ToolFactory* createNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	NavigationToolFactory* navigationToolFactory=new NavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return navigationToolFactory;
	}

extern "C" void destroyNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/*******************************
Methods of class NavigationTool:
*******************************/

NavigationTool::NavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:Tool(factory,inputAssignment),
	 active(false)
	{
	}

NavigationTool::~NavigationTool(void)
	{
	/* Deactivate the tool if it is currently active: */
	if(active)
		deactivateNavigationTool(this);
	}

bool NavigationTool::activate(void)
	{
	if(!active)
		active=activateNavigationTool(this);
	return active;
	}

void NavigationTool::deactivate(void)
	{
	if(active)
		{
		deactivateNavigationTool(this);
		active=false;
		}
	}

}
