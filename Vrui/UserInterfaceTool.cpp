/***********************************************************************
UserInterfaceTool - Base class for tools related to user interfaces
(interaction with dialog boxes, context menus, virtual input devices).
Copyright (c) 2008-2015 Oliver Kreylos

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

#include <Vrui/UserInterfaceTool.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/OrthonormalTransformation.h>
#include <GL/GLValueCoders.h>
#include <Vrui/Vrui.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRScreen.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/********************************************************
Methods of class UserInterfaceToolFactory::Configuration:
********************************************************/

UserInterfaceToolFactory::Configuration::Configuration(void)
	:drawRay(true),rayColor(1.0f,0.0f,0.0f),rayWidth(3.0f)
	{
	}

void UserInterfaceToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	drawRay=cfs.retrieveValue<bool>("./drawRay",drawRay);
	rayColor=cfs.retrieveValue<GLColor<GLfloat,4> >("./rayColor",rayColor);
	rayWidth=cfs.retrieveValue<GLfloat>("./rayWidth",rayWidth);
	}

void UserInterfaceToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue<bool>("./drawRay",drawRay);
	cfs.storeValue<GLColor<GLfloat,4> >("./rayColor",rayColor);
	cfs.storeValue<GLfloat>("./rayWidth",rayWidth);
	}

/*****************************************
Methods of class UserInterfaceToolFactory:
*****************************************/

UserInterfaceToolFactory::UserInterfaceToolFactory(ToolManager& toolManager)
	:ToolFactory("UserInterfaceTool",toolManager)
	{
	#if 0
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("Tool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	#endif
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
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

/******************************************
Static elements of class UserInterfaceTool:
******************************************/

UserInterfaceToolFactory* UserInterfaceTool::factory=0;

/**********************************
Methods of class UserInterfaceTool:
**********************************/

UserInterfaceTool::UserInterfaceTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:Tool(factory,inputAssignment),
	 configuration(UserInterfaceTool::factory->configuration)
	{
	}

void UserInterfaceTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override private configuration data from given configuration file section: */
	configuration.read(configFileSection);
	}

void UserInterfaceTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write private configuration data to given configuration file section: */
	configuration.write(configFileSection);
	}

}
