/***********************************************************************
FPSArmTool - Class to simulate an avatar arm in an FPS-like setting.
Copyright (c) 2014 Oliver Kreylos

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

#include <Vrui/Tools/FPSArmTool.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/GeometryValueCoders.h>

namespace Vrui {

/*************************************************
Methods of class FPSArmToolFactory::Configuration:
*************************************************/

FPSArmToolFactory::Configuration::Configuration(void)
	:transitionTime(1),
	 followPitch(true),followYaw(false)
	{
	}

void FPSArmToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	lowPosition=cfs.retrieveValue<ONTransform>("./lowPosition",lowPosition);
	highPosition=cfs.retrieveValue<ONTransform>("./highPosition",highPosition);
	transitionTime=cfs.retrieveValue<Scalar>("./transitionTime",transitionTime);
	followPitch=cfs.retrieveValue<bool>("./followPitch",followPitch);
	followYaw=cfs.retrieveValue<bool>("./followYaw",followYaw);
	}

void FPSArmToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue<ONTransform>("./lowPosition",lowPosition);
	cfs.storeValue<ONTransform>("./highPosition",highPosition);
	cfs.storeValue<Scalar>("./transitionTime",transitionTime);
	cfs.storeValue<bool>("./followPitch",followPitch);
	cfs.storeValue<bool>("./followYaw",followYaw);
	}

/**********************************
Methods of class FPSArmToolFactory:
**********************************/

FPSArmToolFactory::FPSArmToolFactory(ToolManager& toolManager)
	:ToolFactory("FPSArmTool",toolManager)
	{
	/* Insert class into class hierarchy: */
	TransformToolFactory* transformToolFactory=dynamic_cast<TransformToolFactory*>(toolManager.loadClass("TransformTool"));
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	
	/* Set tool class' factory pointer: */
	FPSArmTool::factory=this;
	}

FPSArmToolFactory::~FPSArmToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	FPSArmTool::factory=0;
	}

const char* FPSArmToolFactory::getName(void) const
	{
	return "FPS Arm";
	}

Tool* FPSArmToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new FPSArmTool(this,inputAssignment);
	}

void FPSArmToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveFPSArmToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createFPSArmToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	FPSArmToolFactory* factory=new FPSArmToolFactory(*toolManager);
	
	/* Return factory object: */
	return factory;
	}

extern "C" void destroyFPSArmToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/***********************************
Static elements of class FPSArmTool:
***********************************/

FPSArmToolFactory* FPSArmTool::factory=0;

/***************************
Methods of class FPSArmTool:
***************************/

FPSArmTool::FPSArmTool(

}
