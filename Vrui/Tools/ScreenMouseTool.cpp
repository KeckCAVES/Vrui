/***********************************************************************
ScreenMouseTool - Class to create a virtual mouse from two valuators
and a screen rectangle.
Copyright (c) 2015 Oliver Kreylos

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

#include <Vrui/Tools/ScreenMouseTool.h>

#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/Vrui.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRScreen.h>

namespace Vrui {

/******************************************************
Methods of class ScreenMouseToolFactory::Configuration:
******************************************************/

ScreenMouseToolFactory::Configuration::Configuration(void)
	:deadZone(0.1),exponent(1)
	{
	/* Get the main screen: */
	VRScreen* mainScreen=getMainScreen();
	screenName=mainScreen->getName();
	
	/* Initialize velocity factors such that the entire screen can be traversed in 1s with max valuator value: */
	velocityFactors[0]=velocityFactors[1]=Math::max(mainScreen->getWidth(),mainScreen->getHeight());
	}

void ScreenMouseToolFactory::Configuration::load(const Misc::ConfigurationFileSection& cfs)
	{
	/* Get parameters: */
	screenName=cfs.retrieveString("./screenName",screenName);
	deadZone=cfs.retrieveValue<Scalar>("./deadZone",deadZone);
	exponent=cfs.retrieveValue<Scalar>("./exponent",exponent);
	velocityFactors=cfs.retrieveValue<Misc::FixedArray<Scalar,2> >("./velocityFactors",velocityFactors);
	}

void ScreenMouseToolFactory::Configuration::save(Misc::ConfigurationFileSection& cfs) const
	{
	/* Save parameters: */
	cfs.storeString("./screenName",screenName);
	cfs.storeValue<Scalar>("./deadZone",deadZone);
	cfs.storeValue<Scalar>("./exponent",exponent);
	cfs.storeValue<Misc::FixedArray<Scalar,2> >("./velocityFactors",velocityFactors);
	}

/***************************************
Methods of class ScreenMouseToolFactory:
***************************************/

ScreenMouseToolFactory::ScreenMouseToolFactory(ToolManager& toolManager)
	:ToolFactory("ScreenMouseToolFactory",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(0,true);
	layout.setNumValuators(2,true);
	
	/* Insert class into class hierarchy: */
	ToolFactory* transformToolFactory=toolManager.loadClass("TransformTool");
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Load class settings: */
	config.load(toolManager.getToolClassSection(getClassName()));
	
	/* Set tool class' factory pointer: */
	ScreenMouseTool::factory=this;
	}

ScreenMouseToolFactory::~ScreenMouseToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	ScreenMouseTool::factory=0;
	}

const char* ScreenMouseToolFactory::getName(void) const
	{
	return "Screen-based Virtual Mouse";
	}

const char* ScreenMouseToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	return "Forwarded Button";
	}

const char* ScreenMouseToolFactory::getValuatorFunction(int valuatorSlotIndex) const
	{
	switch(valuatorSlotIndex)
		{
		case 0:
			return "Translate X";
		
		case 1:
			return "Translate Y";
		
		default:
			return "Forwarded Valuator";
		}
	}

Tool* ScreenMouseToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new ScreenMouseTool(this,inputAssignment);
	}

void ScreenMouseToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveScreenMouseToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createScreenMouseToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	ScreenMouseToolFactory* factory=new ScreenMouseToolFactory(*toolManager);
	
	/* Return factory object: */
	return factory;
	}

extern "C" void destroyScreenMouseToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/****************************************
Static elements of class ScreenMouseTool:
****************************************/

ScreenMouseToolFactory* ScreenMouseTool::factory=0;

/********************************
Methods of class ScreenMouseTool:
********************************/

ScreenMouseTool::ScreenMouseTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:TransformTool(sFactory,inputAssignment),
	 config(factory->config),
	 screen(0),
	 screenPos(Point::origin)
	{
	/* Set the number of private valuators: */
	numPrivateValuators=2;
	}

void ScreenMouseTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Update the configuration: */
	config.load(configFileSection);
	}

void ScreenMouseTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Save the current configuration: */
	config.save(configFileSection);
	}

void ScreenMouseTool::initialize(void)
	{
	/* Let the base class do its thing: */
	TransformTool::initialize();
	
	/* Get a pointer to the requested screen: */
	screen=findScreen(config.screenName.c_str());
	if(screen==0)
		Misc::throwStdErr("ScreenMouseTool::initialize: Screen %s not found",config.screenName.c_str());
	
	/* Set the device's glyph as a cursor: */
	Glyph& deviceGlyph=getInputDeviceManager()->getInputGraphManager()->getInputDeviceGlyph(transformedDevice);
	deviceGlyph.enable();
	deviceGlyph.setGlyphType(Glyph::CURSOR);
	
	/* Initialize the transformed device's position on the screen: */
	screenPos=Point(Math::div2(screen->getWidth()),Math::div2(screen->getHeight()),0);
	
	/* Calculate the transformed device's physical-space position and orientation (rotate by 90 degrees to have y axis point into screen): */
	ONTransform screenT=screen->getScreenTransformation();
	ONTransform deviceT(screenT.transform(screenPos)-Point::origin,screenT.getRotation()*Rotation::rotateX(Math::rad(-90.0)));
	
	/* Transform the eye position to transformed device coordinates: */
	Point deviceEyePos=deviceT.inverseTransform(getMainViewer()->getHeadPosition());
	
	/* Calculate the ray direction and ray origin offset in transformed device coordinates: */
	Vector deviceRayDir=Point::origin-deviceEyePos;
	Scalar deviceRayDirLen=Geometry::mag(deviceRayDir);
	deviceRayDir/=deviceRayDirLen;
	Scalar deviceRayStart=-(deviceEyePos[1]+getFrontplaneDist())*deviceRayDirLen/deviceEyePos[1];
	
	/* Update the transformed device: */
	transformedDevice->setDeviceRay(deviceRayDir,deviceRayStart);
	transformedDevice->setTransformation(deviceT);
	}

const ToolFactory* ScreenMouseTool::getFactory(void) const
	{
	return factory;
	}

void ScreenMouseTool::frame(void)
	{
	/* Calculate the current valuator vector: */
	Vector val(Scalar(getValuatorState(0)),Scalar(getValuatorState(1)),0);
	Scalar valLen=val.mag();
	
	/* Check if there is any motion: */
	if(valLen>config.deadZone)
		{
		/* Calculate the offset and scaled valuator vector: */
		Scalar newValLen=Math::pow((valLen-config.deadZone)/(Scalar(1)-config.deadZone),config.exponent);
		if(newValLen>Scalar(1))
			newValLen=Scalar(1);
		val*=(newValLen/valLen);
		
		/* Apply the velocity scaling factors and update the transformed device's screen position: */
		Scalar dt=Scalar(getCurrentFrameTime());
		for(int i=0;i<2;++i)
			{
			val[i]*=config.velocityFactors[i];
			
			/* Update position: */
			screenPos[i]+=val[i]*dt;
			
			/* Clamp position against screen borders: */
			screenPos[i]=Math::clamp(screenPos[i],Scalar(0),screen->getScreenSize()[i]);
			}
		
		/* Calculate the transformed device's physical-space position and orientation (rotate by 90 degrees to have y axis point into screen): */
		ONTransform screenT=screen->getScreenTransformation();
		ONTransform deviceT(screenT.transform(screenPos)-Point::origin,screenT.getRotation()*Rotation::rotateX(Math::rad(-90.0)));
		
		/* Transform the eye position to transformed device coordinates: */
		Point deviceEyePos=deviceT.inverseTransform(getMainViewer()->getHeadPosition());
		
		/* Calculate the ray direction and ray origin offset in transformed device coordinates: */
		Vector deviceRayDir=Point::origin-deviceEyePos;
		Scalar deviceRayDirLen=Geometry::mag(deviceRayDir);
		deviceRayDir/=deviceRayDirLen;
		Scalar deviceRayStart=-(deviceEyePos[1]+getFrontplaneDist())*deviceRayDirLen/deviceEyePos[1];
		
		/* Update the transformed device: */
		transformedDevice->setDeviceRay(deviceRayDir,deviceRayStart);
		transformedDevice->setTransformation(deviceT);
		transformedDevice->setLinearVelocity(screenT.transform(val));
		
		/* Request another frame: */
		scheduleUpdate(getNextAnimationTime());
		}
	}

}
