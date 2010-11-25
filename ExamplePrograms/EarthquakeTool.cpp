/***********************************************************************
EarthquakeTool - Vrui tool class to snap a virtual input device to
events in an earthquake data set.
Copyright (c) 2009-2010 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include "EarthquakeTool.h"

#include <Geometry/Ray.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>
#include <Vrui/ToolManager.h>
#include <Vrui/InputGraphManager.h>

#include "EarthquakeSet.h"

/**************************************
Methods of class EarthquakeToolFactory:
**************************************/

EarthquakeToolFactory::EarthquakeToolFactory(Vrui::ToolManager& toolManager,const EarthquakeSet* sQuakes)
	:Vrui::ToolFactory("EarthquakeTool",toolManager),
	 quakes(sQuakes)
	{
	/* Insert class into class hierarchy: */
	Vrui::TransformToolFactory* transformToolFactory=dynamic_cast<Vrui::TransformToolFactory*>(toolManager.loadClass("TransformTool"));
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Initialize tool layout: */
	layout.setNumButtons(0,true);
	layout.setNumValuators(0,true);
	
	/* Set the custom tool class' factory pointer: */
	EarthquakeTool::factory=this;
	}

EarthquakeToolFactory::~EarthquakeToolFactory(void)
	{
	/* Reset the custom tool class' factory pointer: */
	EarthquakeTool::factory=0;
	}

const char* EarthquakeToolFactory::getName(void) const
	{
	return "Earthquake Projector";
	}

Vrui::Tool* EarthquakeToolFactory::createTool(const Vrui::ToolInputAssignment& inputAssignment) const
	{
	/* Create a new object of the custom tool class: */
	EarthquakeTool* newTool=new EarthquakeTool(this,inputAssignment);
	
	return newTool;
	}

void EarthquakeToolFactory::destroyTool(Vrui::Tool* tool) const
	{
	/* Cast the tool pointer to the Earthquake tool class (not really necessary): */
	EarthquakeTool* earthquakeTool=dynamic_cast<EarthquakeTool*>(tool);
	
	/* Destroy the tool: */
	delete earthquakeTool;
	}

/***************************************
Static elements of class EarthquakeTool:
***************************************/

EarthquakeToolFactory* EarthquakeTool::factory=0;

/*******************************
Methods of class EarthquakeTool:
*******************************/

EarthquakeTool::EarthquakeTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::TransformTool(factory,inputAssignment)
	{
	/* Set the source device: */
	if(input.getNumButtonSlots()>0)
		sourceDevice=getButtonDevice(0);
	else
		sourceDevice=getValuatorDevice(0);
	}

void EarthquakeTool::initialize(void)
	{
	/* Initialize the base tool: */
	TransformTool::initialize();
	
	/* Disable the transformed device's glyph: */
	Vrui::getInputGraphManager()->getInputDeviceGlyph(transformedDevice).disable();
	}

const Vrui::ToolFactory* EarthquakeTool::getFactory(void) const
	{
	return factory;
	}

void EarthquakeTool::frame(void)
	{
	const EarthquakeSet::Event* event=0;
	
	if(sourceDevice->is6DOFDevice())
		{
		/* Snap the device's position to the closest earthquake event: */
		EarthquakeSet::Point position=EarthquakeSet::Point(Vrui::getNavigationTransformation().inverseTransform(sourceDevice->getPosition()));
		event=factory->quakes->selectEvent(position,float(Vrui::getPointPickDistance()));
		}
	else
		{
		/* Snap the device's position to the closest earthquake event along a ray: */
		Vrui::Ray ray(sourceDevice->getPosition(),sourceDevice->getRayDirection());
		ray.transform(Vrui::getInverseNavigationTransformation());
		ray.normalizeDirection();
		event=factory->quakes->selectEvent(ray,float(Vrui::getRayPickCosine()));
		}
	
	if(event!=0)
		{
		/* Set the virtual device to the event's position: */
		Vrui::Point eventPos=Vrui::Point(event->position);
		Vrui::TrackerState ts=Vrui::TrackerState::translateFromOriginTo(Vrui::getNavigationTransformation().transform(eventPos));
		transformedDevice->setTransformation(ts);
		}
	else
		transformedDevice->setTransformation(sourceDevice->getTransformation());
	transformedDevice->setDeviceRayDirection(sourceDevice->getDeviceRayDirection());
	}
