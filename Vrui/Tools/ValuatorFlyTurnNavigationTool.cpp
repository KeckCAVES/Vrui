/***********************************************************************
ValuatorFlyTurnNavigationTool - Class providing a fly navigation tool
with turning using two valuators.
Copyright (c) 2005-2008 Oliver Kreylos

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
#include <Math/Constants.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <Vrui/Viewer.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/ValuatorFlyTurnNavigationTool.h>

namespace Vrui {

/*****************************************************
Methods of class ValuatorFlyTurnNavigationToolFactory:
*****************************************************/

ValuatorFlyTurnNavigationToolFactory::ValuatorFlyTurnNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("ValuatorFlyTurnNavigationTool",toolManager),
	 valuatorThreshold(0),
	 valuatorExponent(1),
	 superAccelerationFactor(1.1),
	 flyDirection(Vector(0,1,0)),
	 flyFactor(getDisplaySize()*Scalar(0.5)),
	 rotationAxis(Vector(0,0,1)),
	 rotationCenter(Point::origin),
	 rotationFactor(Math::Constants<Scalar>::pi*Scalar(0.5))
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumValuators(0,2);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("NavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	valuatorThreshold=cfs.retrieveValue<Scalar>("./valuatorThreshold",valuatorThreshold);
	valuatorExponent=cfs.retrieveValue<Scalar>("./valuatorExponent",valuatorExponent);
	superAccelerationFactor=cfs.retrieveValue<Scalar>("./superAccelerationFactor",superAccelerationFactor);
	flyDirection=cfs.retrieveValue<Vector>("./flyDirection",flyDirection);
	flyDirection.normalize();
	flyFactor=cfs.retrieveValue<Scalar>("./flyFactor",flyFactor);
	rotationAxis=cfs.retrieveValue<Vector>("./rotationAxis",rotationAxis);
	rotationAxis.normalize();
	rotationCenter=cfs.retrieveValue<Point>("./rotationCenter",rotationCenter);
	rotationFactor=Math::rad(cfs.retrieveValue<Scalar>("./rotationFactor",Math::deg(rotationFactor)));
	
	/* Set tool class' factory pointer: */
	ValuatorFlyTurnNavigationTool::factory=this;
	}


ValuatorFlyTurnNavigationToolFactory::~ValuatorFlyTurnNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	ValuatorFlyTurnNavigationTool::factory=0;
	}

Tool* ValuatorFlyTurnNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new ValuatorFlyTurnNavigationTool(this,inputAssignment);
	}

void ValuatorFlyTurnNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveValuatorFlyTurnNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("NavigationTool");
	}

extern "C" ToolFactory* createValuatorFlyTurnNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	ValuatorFlyTurnNavigationToolFactory* valuatorFlyTurnNavigationToolFactory=new ValuatorFlyTurnNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return valuatorFlyTurnNavigationToolFactory;
	}

extern "C" void destroyValuatorFlyTurnNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/******************************************************
Static elements of class ValuatorFlyTurnNavigationTool:
******************************************************/

ValuatorFlyTurnNavigationToolFactory* ValuatorFlyTurnNavigationTool::factory=0;

/**********************************************
Methods of class ValuatorFlyTurnNavigationTool:
**********************************************/

ValuatorFlyTurnNavigationTool::ValuatorFlyTurnNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(factory,inputAssignment),
	 viewer(0),
	 superAcceleration(1)
	{
	for(int i=0;i<2;++i)
		currentValues[i]=Scalar(0);
	
	/* Retrieve the viewer associated with this menu tool: */
	#if 0
	int viewerIndex=configFile.retrieveValue<int>("./viewerIndex");
	viewer=getViewer(viewerIndex);
	#else
	viewer=getMainViewer();
	#endif
	}

const ToolFactory* ValuatorFlyTurnNavigationTool::getFactory(void) const
	{
	return factory;
	}

void ValuatorFlyTurnNavigationTool::valuatorCallback(int,int valuatorIndex,InputDevice::ValuatorCallbackData* cbData)
	{
	/* Map the raw valuator value according to a "broken line plus exponent" scheme: */
	Scalar v=Scalar(cbData->newValuatorValue);
	Scalar th=factory->valuatorThreshold;
	Scalar s=Scalar(1)-th;
	if(v<-th)
		{
		v=(v+th)/s;
		currentValues[valuatorIndex]=-Math::pow(-v,factory->valuatorExponent);
		}
	else if(v>th)
		{
		v=(v-th)/s;
		currentValues[valuatorIndex]=Math::pow(v,factory->valuatorExponent);
		}
	else
		currentValues[valuatorIndex]=Scalar(0);
	
	if(currentValues[0]!=Scalar(0)||currentValues[1]!=Scalar(0))
		{
		/* Try activating this tool: */
		if(!isActive()&&activate())
			{
			/* Reset the super acceleration: */
			superAcceleration=Scalar(1);
			}
		}
	else
		{
		/* Deactivate this tool: */
		deactivate();
		}
	}

void ValuatorFlyTurnNavigationTool::frame(void)
	{
	/* Act depending on this tool's current state: */
	if(isActive())
		{
		/* Get the current state of the input device: */
		const TrackerState& ts=input.getDevice(0)->getTransformation();
		
		/* Check whether to change the super acceleration factor: */
		if(Math::abs(currentValues[0])==Scalar(1))
			superAcceleration*=Math::pow(factory->superAccelerationFactor,getCurrentFrameTime());
		
		/* Calculate the current flying velocity: */
		Vector v=ts.transform(factory->flyDirection);
		v*=currentValues[0]*factory->flyFactor*superAcceleration*getCurrentFrameTime();
		
		/* Calculate the current angular velocity: */
		Vector w=ts.transform(factory->rotationAxis);
		w*=currentValues[1]*factory->rotationFactor*getCurrentFrameTime();
		
		/* Compose the new navigation transformation: */
		Point p=ts.transform(factory->rotationCenter);
		NavTransform t=NavTransform::translate(v);
		t*=NavTransform::translateFromOriginTo(p);
		t*=NavTransform::rotate(NavTransform::Rotation::rotateScaledAxis(w));
		t*=NavTransform::translateToOriginFrom(p);
		t*=getNavigationTransformation();
		
		/* Update Vrui's navigation transformation: */
		setNavigationTransformation(t);
		}
	}

}
