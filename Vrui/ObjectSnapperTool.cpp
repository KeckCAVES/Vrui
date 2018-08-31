/***********************************************************************
ObjectSnapperTool - Tool class to snap a virtual input device's position
and/or orientation to application-specified objects using a callback
mechanism.
Copyright (c) 2017-2018 Oliver Kreylos

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

#include <Vrui/ObjectSnapperTool.h>

#include <Misc/FunctionCalls.h>
#include <Math/Constants.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>
#include <Vrui/InputGraphManager.h>

namespace Vrui {

/******************************************************
Methods of class ObjectSnapperToolFactory::SnapRequest:
******************************************************/

bool ObjectSnapperToolFactory::SnapRequest::snapPoint(const Point& p)
	{
	/* Check if the snap request is ray-based: */
	bool result=false;
	if(rayBased)
		{
		/* Check the given point against the snap ray: */
		Vector po=p-snapRay.getOrigin();
		Scalar poLen=po.mag();
		if(poLen<snapRayMax&&po*snapRay.getDirection()>snapRayCosine*poLen)
			{
			/* Snap to the point: */
			snapRayMax=poLen;
			result=true;
			}
		}
	else
		{
		/* Check the given point against the snap position: */
		Scalar d2=Geometry::sqrDist(snapPosition,p);
		if(d2<Math::sqr(snapRadius))
			{
			/* Snap to the point: */
			snapRadius=Math::sqrt(d2);
			result=true;
			}
		}
	
	if(result)
		{
		/* Update the snap request: */
		snapped=true;
		snapResult=ONTransform(p-Point::origin,toolTransform.getRotation()); // Retain tool's original rotation
		}
	
	return result;
	}

/*****************************************
Methods of class ObjectSnapperToolFactory:
*****************************************/

ObjectSnapperToolFactory::ObjectSnapperToolFactory(ToolManager& toolManager)
	:ToolFactory("ObjectSnapperTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(0,true);
	layout.setNumValuators(0,true);
	
	/* Insert class into class hierarchy: */
	ToolFactory* transformToolFactory=toolManager.loadClass("TransformTool");
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Set tool class' factory pointer: */
	ObjectSnapperTool::factory=this;
	}

ObjectSnapperToolFactory::~ObjectSnapperToolFactory(void)
	{
	/* Delete all registered snap callbacks: */
	for(SnapFunctionList::iterator scIt=snapCallbacks.begin();scIt!=snapCallbacks.end();++scIt)
		delete *scIt;
	
	/* Reset tool class' factory pointer: */
	ObjectSnapperTool::factory=0;
	}

const char* ObjectSnapperToolFactory::getName(void) const
	{
	return "Object Snapper";
	}

Tool* ObjectSnapperToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new ObjectSnapperTool(this,inputAssignment);
	}

void ObjectSnapperToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveObjectSnapperToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createObjectSnapperToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	ObjectSnapperToolFactory* objectSnapperToolFactory=new ObjectSnapperToolFactory(*toolManager);
	
	/* Return factory object: */
	return objectSnapperToolFactory;
	}

extern "C" void destroyObjectSnapperToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/******************************************
Static elements of class ObjectSnapperTool:
******************************************/

ObjectSnapperToolFactory* ObjectSnapperTool::factory=0;

/**********************************
Methods of class ObjectSnapperTool:
**********************************/

ObjectSnapperTool::ObjectSnapperTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:TransformTool(sFactory,inputAssignment)
	{
	}

ObjectSnapperTool::~ObjectSnapperTool(void)
	{
	}

void ObjectSnapperTool::initialize(void)
	{
	/* Let the base class do its thing: */
	TransformTool::initialize();
	
	/* Disable the transformed device's glyph: */
	getInputGraphManager()->getInputDeviceGlyph(transformedDevice).disable();
	}

const ToolFactory* ObjectSnapperTool::getFactory(void) const
	{
	return factory;
	}

void ObjectSnapperTool::frame(void)
	{
	/* Create a snap request: */
	ObjectSnapperToolFactory::SnapRequest sr;
	sr.tool=this;
	NavTransform tt=getInverseNavigationTransformation()*NavTransform(sourceDevice->getTransformation());
	sr.toolTransform=ONTransform(tt.getTranslation(),tt.getRotation());
	sr.snapped=false;
	
	if(sourceDevice->is6DOFDevice())
		{
		/* Create a point-based snap request: */
		sr.rayBased=false;
		sr.snapPosition=getInverseNavigationTransformation().transform(sourceDevice->getPosition());
		sr.snapRadius=getPointPickDistance();
		}
	else
		{
		/* Create a ray-based snap request: */
		sr.rayBased=true;
		sr.snapRay=sourceDevice->getRay();
		sr.snapRay.transform(getInverseNavigationTransformation());
		sr.snapRay.normalizeDirection();
		sr.snapRayCosine=getRayPickCosine();
		sr.snapRayMax=Math::Constants<Scalar>::max;
		}
	
	/* Call all registered snap callbacks: */
	for(ObjectSnapperToolFactory::SnapFunctionList::iterator scIt=factory->snapCallbacks.begin();scIt!=factory->snapCallbacks.end();++scIt)
		(**scIt)(sr);
	
	/* Position the transformed device at the site of a successful snap request; otherwise, shadow source device: */
	if(sr.snapped)
		{
		NavTransform tdt=getNavigationTransformation()*NavTransform(sr.snapResult);
		transformedDevice->setTransformation(ONTransform(tdt.getTranslation(),tdt.getRotation()));
		
		// TODO: Recalculate transformed device's ray in some meaningful way
		}
	else
		resetDevice();
	}

void ObjectSnapperTool::addSnapCallback(ObjectSnapperToolFactory::SnapFunction* newSnapFunction)
	{
	/* Add the given callback to the factory object's callback list: */
	factory->snapCallbacks.push_back(newSnapFunction);
	}

}
