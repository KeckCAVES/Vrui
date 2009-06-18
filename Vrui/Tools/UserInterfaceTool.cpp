/***********************************************************************
UserInterfaceTool - Base class for tools related to user interfaces
(interaction with dialog boxes, context menus, virtual input devices).
Copyright (c) 2008-2009 Oliver Kreylos

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
#include <Vrui/Viewer.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/UserInterfaceTool.h>

namespace Vrui {

/*****************************************
Methods of class UserInterfaceToolFactory:
*****************************************/

UserInterfaceToolFactory::UserInterfaceToolFactory(ToolManager& toolManager)
	:ToolFactory("UserInterfaceTool",toolManager),
	 useEyeRay(false),
	 rayOffset(getUiSize()*Scalar(2))
	{
	#if 0
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("Tool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	#endif
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	useEyeRay=cfs.retrieveValue<bool>("./useEyeRay",useEyeRay);
	rayOffset=cfs.retrieveValue<Scalar>("./rayOffset",rayOffset);
	
	/* Set tool class' factory pointer: */
	UserInterfaceTool::factory=this;
	}

UserInterfaceToolFactory::~UserInterfaceToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	UserInterfaceTool::factory=0;
	}

const char* UserInterfaceToolFactory::getName(void) const
	{
	return "User Interface";
	}

extern "C" ToolFactory* createUserInterfaceToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	UserInterfaceToolFactory* userInterfaceToolFactory=new UserInterfaceToolFactory(*toolManager);
	
	/* Return factory object: */
	return userInterfaceToolFactory;
	}

extern "C" void destroyUserInterfaceToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/******************************************
Static elements of class UserInterfaceTool:
******************************************/

UserInterfaceToolFactory* UserInterfaceTool::factory=0;

/**********************************
Methods of class UserInterfaceTool:
**********************************/

Ray UserInterfaceTool::calcInteractionRay(void) const
	{
	Ray result;
	
	if(factory->useEyeRay)
		{
		/* Shoot a ray from the main viewer: */
		Point start=getDevicePosition(0);
		Vector direction=start-getMainViewer()->getHeadPosition();
		direction.normalize();
		result=Ray(start,direction);
		}
	else
		{
		/* Use the device's ray direction: */
		result=getDeviceRay(0);
		}
	
	/* Offset the ray start point backwards: */
	result.setOrigin(result.getOrigin()-result.getDirection()*(factory->rayOffset/result.getDirection().mag()));
	return result;
	}

UserInterfaceTool::UserInterfaceTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:Tool(factory,inputAssignment)
	{
	}

}
