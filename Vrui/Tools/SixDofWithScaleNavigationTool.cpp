/***********************************************************************
SixDofWithScaleNavigationTool - Class for simple 6-DOF dragging using a
single input device, with an additional input device used as a slider
for zooming.
Copyright (c) 2004-2009 Oliver Kreylos

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
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GL/GLModels.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/SixDofWithScaleNavigationTool.h>

namespace Vrui {

/***************************************************************
Methods of class SixDofWithScaleNavigationToolFactory::DataItem:
***************************************************************/

SixDofWithScaleNavigationToolFactory::DataItem::DataItem(void)
	{
	/* Create tools' model display list: */
	modelListId=glGenLists(1);
	}

SixDofWithScaleNavigationToolFactory::DataItem::~DataItem(void)
	{
	/* Destroy tools' model display list: */
	glDeleteLists(modelListId,1);
	}

/*****************************************************
Methods of class SixDofWithScaleNavigationToolFactory:
*****************************************************/

SixDofWithScaleNavigationToolFactory::SixDofWithScaleNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("SixDofWithScaleNavigationTool",toolManager),
	 scaleDeviceDistance(getInchFactor()*Scalar(6)),
	 deviceScaleDirection(0,1,0),
	 scaleFactor(getInchFactor()*Scalar(12))
	{
	/* Initialize tool layout: */
	layout.setNumDevices(2);
	layout.setNumButtons(0,1);
	layout.setNumButtons(1,0);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("NavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	scaleDeviceDistance=cfs.retrieveValue<Scalar>("./scaleDeviceDistance",scaleDeviceDistance);
	scaleDeviceDistance2=scaleDeviceDistance*scaleDeviceDistance;
	deviceScaleDirection=cfs.retrieveValue<Vector>("./deviceScaleDirection",deviceScaleDirection);
	scaleFactor=cfs.retrieveValue<Scalar>("./scaleFactor",scaleFactor);
	
	/* Set tool class' factory pointer: */
	SixDofWithScaleNavigationTool::factory=this;
	}

SixDofWithScaleNavigationToolFactory::~SixDofWithScaleNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	SixDofWithScaleNavigationTool::factory=0;
	}

const char* SixDofWithScaleNavigationToolFactory::getName(void) const
	{
	return "6-DOF plus Scaling Device";
	}

Tool* SixDofWithScaleNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new SixDofWithScaleNavigationTool(this,inputAssignment);
	}

void SixDofWithScaleNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

void SixDofWithScaleNavigationToolFactory::initContext(GLContextData& contextData) const
	{
	/* Create a new data item: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Create the tool model display list: */
	glNewList(dataItem->modelListId,GL_COMPILE);
	
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_POLYGON_BIT|GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	glLineWidth(1.0f);
	glColor3f(1.0f,1.0f,1.0f);
	
	/* Render a sphere of radius scaleDeviceDistance around the scaling device's position: */
	glDrawSphereIcosahedron(scaleDeviceDistance,3);
	
	/* Render the scaling direction: */
	glLineWidth(3.0f);
	glColor3f(1.0f,0.0f,0.0f);
	glBegin(GL_LINES);
	Point pos=Point::origin;
	glVertex(pos);
	pos+=deviceScaleDirection*scaleDeviceDistance*Scalar(1.25);
	glVertex(pos);
	glEnd();
	
	/* Reset OpenGL state: */
	glPopAttrib();
	
	glEndList();
	}

extern "C" void resolveSixDofWithScaleNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("NavigationTool");
	}

extern "C" ToolFactory* createSixDofWithScaleNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	SixDofWithScaleNavigationToolFactory* sixDofWithScaleNavigationToolFactory=new SixDofWithScaleNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return sixDofWithScaleNavigationToolFactory;
	}

extern "C" void destroySixDofWithScaleNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/******************************************************
Static elements of class SixDofWithScaleNavigationTool:
******************************************************/

SixDofWithScaleNavigationToolFactory* SixDofWithScaleNavigationTool::factory=0;

/**********************************************
Methods of class SixDofWithScaleNavigationTool:
**********************************************/

SixDofWithScaleNavigationTool::SixDofWithScaleNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(factory,inputAssignment),
	 navigationMode(IDLE)
	{
	}

const ToolFactory* SixDofWithScaleNavigationTool::getFactory(void) const
	{
	return factory;
	}

void SixDofWithScaleNavigationTool::buttonCallback(int,int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		if(navigationMode==IDLE&&activate())
			{
			/* Decide whether to go to moving or scaling mode: */
			if(Geometry::sqrDist(getDevicePosition(0),getDevicePosition(1))<=factory->scaleDeviceDistance2) // Want to scale
				{
				/* Determine the scaling center and initial scale: */
				scalingCenter=getDevicePosition(1);
				Vector scaleDirection=getDeviceTransformation(1).transform(factory->deviceScaleDirection);
				initialScale=getDevicePosition(0)*scaleDirection;
				
				/* Initialize the navigation transformations: */
				preScale=NavTrackerState::translateFromOriginTo(scalingCenter);
				postScale=NavTrackerState::translateToOriginFrom(scalingCenter);
				postScale*=getNavigationTransformation();
				
				/* Go from MOVING to SCALING mode: */
				navigationMode=SCALING;
				}
			else // Want to move
				{
				/* Initialize the navigation transformations: */
				preScale=Geometry::invert(getDeviceTransformation(0));
				preScale*=getNavigationTransformation();
				
				/* Go from IDLE to MOVING mode: */
				navigationMode=MOVING;
				}
			}
		}
	else // Button has just been released
		{
		/* Deactivate this tool: */
		deactivate();
		
		/* Go from MOVING or SCALING to IDLE mode: */
		navigationMode=IDLE;
		}
	}

void SixDofWithScaleNavigationTool::frame(void)
	{
	/* Act depending on this tool's current state: */
	switch(navigationMode)
		{
		case IDLE:
			/* Do nothing */
			break;
		
		case MOVING:
			{
			/* Compose the new navigation transformation: */
			NavTrackerState navigation=getDeviceTransformation(0);
			navigation*=preScale;
			
			/* Update Vrui's navigation transformation: */
			setNavigationTransformation(navigation);
			break;
			}
		
		case SCALING:
			{
			/* Compose the new navigation transformation: */
			NavTrackerState navigation=preScale;
			Vector scaleDirection=getDeviceTransformation(1).transform(factory->deviceScaleDirection);
			Scalar currentScale=Math::exp((getDevicePosition(0)*scaleDirection-initialScale)/factory->scaleFactor);
			navigation*=NavTrackerState::scale(currentScale);
			navigation*=postScale;
			
			/* Update Vrui's navigation transformation: */
			setNavigationTransformation(navigation);
			break;
			}
		}
	}

void SixDofWithScaleNavigationTool::display(GLContextData& contextData) const
	{
	/* Get a pointer to the context entry: */
	SixDofWithScaleNavigationToolFactory::DataItem* dataItem=contextData.retrieveDataItem<SixDofWithScaleNavigationToolFactory::DataItem>(factory);
	
	/* Translate coordinate system to scaling device's position and orientation: */
	glPushMatrix();
	glMultMatrix(getDeviceTransformation(1));
	
	/* Execute the tool model display list: */
	glCallList(dataItem->modelListId);
	
	/* Go back to physical coordinate system: */
	glPopMatrix();
	}

}
