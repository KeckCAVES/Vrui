/***********************************************************************
ValuatorFlyNavigationTool - Class providing a fly navigation tool using
a single valuator.
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
#include <Math/Math.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <Vrui/Viewer.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/ValuatorFlyNavigationTool.h>

namespace Vrui {

/*************************************************
Methods of class ValuatorFlyNavigationToolFactory:
*************************************************/

ValuatorFlyNavigationToolFactory::ValuatorFlyNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("ValuatorFlyNavigationTool",toolManager),
	 valuatorThreshold(0),
	 flyDirection(Vector(0,1,0)),
	 flyFactor(getDisplaySize()*Scalar(0.5))
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumValuators(0,1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("NavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	valuatorThreshold=cfs.retrieveValue<Scalar>("./valuatorThreshold",valuatorThreshold);
	flyDirection=cfs.retrieveValue<Vector>("./flyDirection",flyDirection);
	flyDirection.normalize();
	flyFactor=cfs.retrieveValue<Scalar>("./flyFactor",flyFactor);
	
	/* Set tool class' factory pointer: */
	ValuatorFlyNavigationTool::factory=this;
	}


ValuatorFlyNavigationToolFactory::~ValuatorFlyNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	ValuatorFlyNavigationTool::factory=0;
	}

Tool* ValuatorFlyNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new ValuatorFlyNavigationTool(this,inputAssignment);
	}

void ValuatorFlyNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveValuatorFlyNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("NavigationTool");
	}

extern "C" ToolFactory* createValuatorFlyNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	ValuatorFlyNavigationToolFactory* valuatorFlyNavigationToolFactory=new ValuatorFlyNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return valuatorFlyNavigationToolFactory;
	}

extern "C" void destroyValuatorFlyNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/**************************************************
Static elements of class ValuatorFlyNavigationTool:
**************************************************/

ValuatorFlyNavigationToolFactory* ValuatorFlyNavigationTool::factory=0;

/******************************************
Methods of class ValuatorFlyNavigationTool:
******************************************/

ValuatorFlyNavigationTool::ValuatorFlyNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(factory,inputAssignment),
	 viewer(0),
	 currentValue(0)
	{
	/* Retrieve the viewer associated with this menu tool: */
	#if 0
	int viewerIndex=configFile.retrieveValue<int>("./viewerIndex");
	viewer=getViewer(viewerIndex);
	#else
	viewer=getMainViewer();
	#endif
	}

const ToolFactory* ValuatorFlyNavigationTool::getFactory(void) const
	{
	return factory;
	}

void ValuatorFlyNavigationTool::valuatorCallback(int,int,InputDevice::ValuatorCallbackData* cbData)
	{
	/* Map the raw valuator value according to a "broken line" scheme: */
	Scalar v=Scalar(cbData->newValuatorValue);
	Scalar th=factory->valuatorThreshold;
	Scalar s=Scalar(1)-th;
	if(v<-th)
		v=(v+th)/s;
	else if(v>th)
		v=(v-th)/s;
	else
		v=Scalar(0);
	currentValue=v;
	
	if(currentValue!=Scalar(0))
		{
		/* Try activating this tool: */
		activate();
		}
	else
		{
		/* Deactivate this tool: */
		deactivate();
		}
	}

void ValuatorFlyNavigationTool::frame(void)
	{
	/* Act depending on this tool's current state: */
	if(isActive())
		{
		/* Get the current state of the input device: */
		const TrackerState& ts=input.getDevice(0)->getTransformation();
		
		/* Calculate the current flying velocity: */
		Vector v=ts.transform(factory->flyDirection);
		v*=currentValue*factory->flyFactor*getCurrentFrameTime();
		
		/* Compose the new navigation transformation: */
		NavTransform t=NavTransform::translate(v);
		t*=getNavigationTransformation();
		
		/* Update Vrui's navigation transformation: */
		setNavigationTransformation(t);
		}
	}

}
