/***********************************************************************
DaisyWheelTool - Class for tools to enter text by pointing at characters
on a dynamic daisy wheel.
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
#include <Math/Math.h>
#include <Math/Constants.h>
#include <GL/gl.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/DaisyWheelTool.h>

namespace Vrui {

/**************************************
Methods of class DaisyWheelToolFactory:
**************************************/

DaisyWheelToolFactory::DaisyWheelToolFactory(ToolManager& toolManager)
	:ToolFactory("DaisyWheelTool",toolManager),
	 innerRadius(getUiSize()*Scalar(8)),outerRadius(getUiSize()*Scalar(24))
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("UserInterfaceTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	innerRadius=cfs.retrieveValue<Scalar>("./innerRadius",innerRadius);
	outerRadius=cfs.retrieveValue<Scalar>("./outerRadius",outerRadius);
	
	/* Set tool class' factory pointer: */
	DaisyWheelTool::factory=this;
	}

DaisyWheelToolFactory::~DaisyWheelToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	DaisyWheelTool::factory=0;
	}

const char* DaisyWheelToolFactory::getName(void) const
	{
	return "Daisy Wheel";
	}

Tool* DaisyWheelToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new DaisyWheelTool(this,inputAssignment);
	}

void DaisyWheelToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveDaisyWheelToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("UserInterfaceTool");
	}

extern "C" ToolFactory* createDaisyWheelToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	DaisyWheelToolFactory* daisyWheelToolFactory=new DaisyWheelToolFactory(*toolManager);
	
	/* Return factory object: */
	return daisyWheelToolFactory;
	}

extern "C" void destroyDaisyWheelToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/***************************************
Static elements of class DaisyWheelTool:
***************************************/

DaisyWheelToolFactory* DaisyWheelTool::factory=0;

/*******************************
Methods of class DaisyWheelTool:
*******************************/

DaisyWheelTool::DaisyWheelTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:UserInterfaceTool(factory,inputAssignment),
	 numCharacters(26),
	 baseWeights(new Scalar[numCharacters]),
	 baseWeightSum(numCharacters),
	 dynamicWeights(new Scalar[numCharacters]),
	 dynamicWeightSum(numCharacters),
	 active(false)
	{
	/* Initialize the basic and dynamic weights: */
	for(int i=0;i<numCharacters;++i)
		baseWeights[i]=dynamicWeights[i]=Scalar(1);
	}

DaisyWheelTool::~DaisyWheelTool(void)
	{
	delete[] baseWeights;
	delete[] dynamicWeights;
	}

const ToolFactory* DaisyWheelTool::getFactory(void) const
	{
	return factory;
	}

void DaisyWheelTool::buttonCallback(int,int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Store the daisy wheel transformation: */
		wheelTransform=getDeviceTransformation(0);
		
		/* Initialize the zoom factors: */
		zoomAngle=Scalar(0);
		zoomStrength=Scalar(0);
		
		/* Activate the tool: */
		active=true;
		}
	else // Button has just been released
		{
		if(active)
			{
			/* Deactivate this tool: */
			active=false;
			}
		}
	}

void DaisyWheelTool::frame(void)
	{
	if(active)
		{
		/* Update the selection ray: */
		selectionRay=calcInteractionRay();
		
		/* Calculate the intersection point of selection ray and daisy wheel: */
		Ray wheelRay=selectionRay;
		wheelRay.inverseTransform(wheelTransform);
		if(wheelRay.getDirection()[1]!=Scalar(0))
			{
			Scalar lambda=(Scalar(0)-wheelRay.getOrigin()[1])/wheelRay.getDirection()[1];
			if(lambda>=Scalar(0))
				{
				Point wheelPoint=wheelRay(lambda);
				
				/* Calculate the zoom angle and zoom strength: */
				zoomStrength=(Geometry::mag(wheelPoint)-factory->innerRadius)*Scalar(0.5)/(factory->outerRadius-factory->innerRadius);
				if(zoomStrength<Scalar(0))
					zoomStrength=Scalar(0);
				else if(zoomStrength>Scalar(0.75))
					zoomStrength=Scalar(0.75);
				if(zoomStrength>Scalar(0))
					{
					zoomAngle=Math::atan2(wheelPoint[0],wheelPoint[2]);
					if(zoomAngle<Scalar(0))
						zoomAngle=Scalar(2)*Math::Constants<Scalar>::pi+zoomAngle;
					}
				}
			}
		}
	}

void DaisyWheelTool::display(GLContextData&) const
	{
	if(active)
		{
		/* Save OpenGL state: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		
		/* Draw the daisy wheel: */
		glPushMatrix();
		glMultMatrix(wheelTransform);
		
		/* Set up OpenGL state: */
		glDisable(GL_LIGHTING);
		
		glBegin(GL_QUADS);
		Scalar angle0=Scalar(0);
		Scalar angleDiff=zoomAngle-angle0;
		if(angleDiff<-Math::Constants<Scalar>::pi)
			angleDiff+=Scalar(2)*Math::Constants<Scalar>::pi;
		else if(angleDiff>Math::Constants<Scalar>::pi)
			angleDiff-=Scalar(2)*Math::Constants<Scalar>::pi;
		Scalar weight=Math::abs(angleDiff)/Math::Constants<Scalar>::pi;
		angleDiff*=Math::pow(weight,Scalar(1)-zoomStrength)/weight;
		Scalar wAngle0=zoomAngle-angleDiff;
		Scalar c0=Math::cos(wAngle0);
		Scalar s0=Math::sin(wAngle0);
		for(int i=0;i<numCharacters;++i)
			{
			Scalar angle1=angle0+dynamicWeights[i]*Scalar(2)*Math::Constants<Scalar>::pi/dynamicWeightSum;
			angleDiff=zoomAngle-angle1;
			if(angleDiff<-Math::Constants<Scalar>::pi)
				angleDiff+=Scalar(2)*Math::Constants<Scalar>::pi;
			else if(angleDiff>Math::Constants<Scalar>::pi)
				angleDiff-=Scalar(2)*Math::Constants<Scalar>::pi;
			weight=Math::abs(angleDiff)/Math::Constants<Scalar>::pi;
			angleDiff*=Math::pow(weight,Scalar(1)-zoomStrength)/weight;
			Scalar wAngle1=zoomAngle-angleDiff;
			Scalar c1=Math::cos(wAngle1);
			Scalar s1=Math::sin(wAngle1);
			
			if(i%2==0)
				glColor3f(1.0f,0.5f,0.5f);
			else
				glColor3f(0.0f,0.5f,1.0f);
			glVertex3f(s0*factory->innerRadius,0.0f,c0*factory->innerRadius);
			glVertex3f(s1*factory->innerRadius,0.0f,c1*factory->innerRadius);
			glVertex3f(s1*factory->outerRadius,0.0f,c1*factory->outerRadius);
			glVertex3f(s0*factory->outerRadius,0.0f,c0*factory->outerRadius);
			
			angle0=angle1;
			wAngle0=wAngle1;
			c0=c1;
			s0=s1;
			}
		glEnd();
		
		glPopMatrix();
		
		/* Draw the menu selection ray: */
		glLineWidth(3.0f);
		glColor3f(1.0f,0.0f,0.0f);
		glBegin(GL_LINES);
		glVertex(selectionRay.getOrigin());
		glVertex(selectionRay(getDisplaySize()*Scalar(5)));
		glEnd();
		
		/* Restore OpenGL state: */
		glPopAttrib();
		}
	}

}
