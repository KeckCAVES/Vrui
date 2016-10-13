/***********************************************************************
PlaneProjectorTool - Class to create virtual input devices at the
intersection of a device ray and a controllable 2D plane.
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

#include <Vrui/Tools/PlaneProjectorTool.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/Ray.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/*********************************************************
Methods of class PlaneProjectorToolFactory::Configuration:
*********************************************************/

PlaneProjectorToolFactory::Configuration::Configuration(void)
	:snapOrientation(false)
	{
	}

void PlaneProjectorToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	snapOrientation=cfs.retrieveValue<bool>("./snapOrientation",snapOrientation);
	}

void PlaneProjectorToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue<bool>("./snapOrientation",snapOrientation);
	}

/******************************************
Methods of class PlaneProjectorToolFactory:
******************************************/

PlaneProjectorToolFactory::PlaneProjectorToolFactory(ToolManager& toolManager)
	:ToolFactory("PlaneProjectorTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(0,true);
	layout.setNumValuators(0,true);
	
	/* Insert class into class hierarchy: */
	ToolFactory* transformToolFactory=toolManager.loadClass("TransformTool");
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Load class settings: */
	configuration.read(toolManager.getToolClassSection(getClassName()));
	
	/* Set tool class' factory pointer: */
	PlaneProjectorTool::factory=this;
	}

PlaneProjectorToolFactory::~PlaneProjectorToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	PlaneProjectorTool::factory=0;
	}

const char* PlaneProjectorToolFactory::getName(void) const
	{
	return "Plane Projector";
	}

Tool* PlaneProjectorToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new PlaneProjectorTool(this,inputAssignment);
	}

void PlaneProjectorToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolvePlaneProjectorToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createPlaneProjectorToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	PlaneProjectorToolFactory* planeProjectorToolFactory=new PlaneProjectorToolFactory(*toolManager);
	
	/* Return factory object: */
	return planeProjectorToolFactory;
	}

extern "C" void destroyPlaneProjectorToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/*******************************************
Static elements of class PlaneProjectorTool:
*******************************************/

PlaneProjectorToolFactory* PlaneProjectorTool::factory=0;

/***********************************
Methods of class PlaneProjectorTool:
***********************************/

PlaneProjectorTool::PlaneProjectorTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:TransformTool(sFactory,inputAssignment),
	 config(factory->configuration)
	{
	/* Initialize the current projection plane equation: */
	center=getInverseNavigationTransformation().transform(getDisplayCenter());
	normal=getInverseNavigationTransformation().transform(getForwardDirection());
	}

void PlaneProjectorTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Update the configuration: */
	config.read(configFileSection);
	
	/* Update the current tool state: */
	center=configFileSection.retrieveValue<Point>("./planeCenter",center);
	normal=configFileSection.retrieveValue<Vector>("./planeNormal",normal);
	
	/* Calculate the plane rotation: */
	rotation=Rotation::rotateFromTo(Vector(0,0,1),normal);
	}

void PlaneProjectorTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Save the current configuration: */
	config.write(configFileSection);
	
	/* Save the current tool state: */
	configFileSection.storeValue<Point>("./planeCenter",center);
	configFileSection.storeValue<Vector>("./planeNormal",normal);
	}

void PlaneProjectorTool::initialize(void)
	{
	/* Let the base class do its thing: */
	TransformTool::initialize();
	
	/* Disable the transformed device's glyph: */
	Vrui::getInputGraphManager()->getInputDeviceGlyph(transformedDevice).disable();
	
	/* Initialize the virtual input device's position and orientation: */
	// TODO
	}

const ToolFactory* PlaneProjectorTool::getFactory(void) const
	{
	return factory;
	}

void PlaneProjectorTool::frame(void)
	{
	/* Update the current projection plane equation in physical space: */
	centerPhys=getNavigationTransformation().transform(center);
	normalPhys=getNavigationTransformation().transform(normal);
	rotationPhys=getNavigationTransformation().getRotation()*rotation;
	
	if(sourceDevice->is6DOFDevice())
		{
		/* Project the source device's position orthogonally onto the plane: */
		Point devicePos=sourceDevice->getPosition();
		devicePos-=normalPhys*((devicePos*normalPhys-centerPhys*normalPhys)/normalPhys.sqr());
		
		/* Update the transformed device's position and orientation: */
		if(config.snapOrientation)
			{
			}
		else
			{
			/* Change position only: */
			transformedDevice->setDeviceRay(sourceDevice->getDeviceRayDirection(),Vrui::Scalar(0));
			TrackerState ts(devicePos-Point::origin,sourceDevice->getOrientation());
			transformedDevice->setTransformation(ts);
			}
		}
	else
		{
		/* Project the source device's position along its interaction ray: */
		Ray ray=sourceDevice->getRay();
		Scalar dn,lambda;
		if((dn=normalPhys*ray.getDirection())!=Scalar(0)
		   &&(lambda=(centerPhys*normalPhys-ray.getOrigin()*normalPhys)/dn)>=Scalar(0))
			{
			/* Position the transformed device at the ray intersection: */
			if(config.snapOrientation)
				{
				}
			else
				{
				/* Change position only: */
				transformedDevice->setDeviceRay(sourceDevice->getDeviceRayDirection(),-lambda);
				TrackerState ts(ray(lambda)-Point::origin,sourceDevice->getOrientation());
				transformedDevice->setTransformation(ts);
				}
			}
		}
	}

void PlaneProjectorTool::display(GLContextData& contextData) const
	{
	/* Draw the projection plane: */
	
	}

}
