/***********************************************************************
FlashlightTool - Class for tools that add an additional light source
into an environment when activated.
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

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <GL/gl.h>
#include <GL/GLValueCoders.h>
#include <GL/GLGeometryWrappers.h>
#include <Vrui/Lightsource.h>
#include <Vrui/LightsourceManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/FlashlightTool.h>

namespace Vrui {

/**************************************
Methods of class FlashlightToolFactory:
**************************************/

FlashlightToolFactory::FlashlightToolFactory(ToolManager& toolManager)
	:ToolFactory("FlashlightTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("UtilityTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	GLLight::Color lightColor=cfs.retrieveValue<GLLight::Color>("./lightColor",GLLight::Color(1.0f,1.0f,1.0f));
	light.diffuse=lightColor;
	light.specular=lightColor;
	light.spotCutoff=cfs.retrieveValue<GLfloat>("./lightSpotCutoff",180.0f);
	light.spotExponent=cfs.retrieveValue<GLfloat>("./lightSpotExponent",0.0f);
	
	/* Set tool class' factory pointer: */
	FlashlightTool::factory=this;
	}

FlashlightToolFactory::~FlashlightToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	FlashlightTool::factory=0;
	}

Tool* FlashlightToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new FlashlightTool(this,inputAssignment);
	}

void FlashlightToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveFlashlightToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("UtilityTool");
	}

extern "C" ToolFactory* createFlashlightToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	FlashlightToolFactory* flashlightToolFactory=new FlashlightToolFactory(*toolManager);
	
	/* Return factory object: */
	return flashlightToolFactory;
	}

extern "C" void destroyFlashlightToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/***************************************
Static elements of class FlashlightTool:
***************************************/

FlashlightToolFactory* FlashlightTool::factory=0;

/*******************************
Methods of class FlashlightTool:
*******************************/

FlashlightTool::FlashlightTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:UtilityTool(sFactory,inputAssignment),
	 lightsource(0),active(false)
	{
	/* Create a light source: */
	lightsource=getLightsourceManager()->createLightsource(true,factory->light);
	lightsource->disable();
	}

FlashlightTool::~FlashlightTool(void)
	{
	/* Destroy the light source: */
	getLightsourceManager()->destroyLightsource(lightsource);
	}

const ToolFactory* FlashlightTool::getFactory(void) const
	{
	return factory;
	}

void FlashlightTool::buttonCallback(int,int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Enable the light source: */
		active=true;
		lightsource->enable();
		}
	else // Button has just been released
		{
		/* Disable the light source: */
		active=false;
		lightsource->disable();
		}
	}

void FlashlightTool::frame(void)
	{
	if(active)
		{
		/* Get pointer to input device: */
		InputDevice* device=input.getDevice(0);
		
		/* Set the light source parameters: */
		Point start=device->getPosition();
		lightsource->getLight().position=GLLight::Position(GLfloat(start[0]),GLfloat(start[1]),GLfloat(start[2]),1.0f);
		Vector direction=device->getRayDirection();
		direction.normalize();
		lightsource->getLight().spotDirection=GLLight::SpotDirection(GLfloat(direction[0]),GLfloat(direction[1]),GLfloat(direction[2]));
		}
	}

}
