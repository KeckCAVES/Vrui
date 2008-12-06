/***********************************************************************
WidgetTool - Class for tools that can interact with GLMotif GUI widgets.
WidgetTool objects are cascadable and preempt button events if they
would fall into the area of interest of mapped widgets.
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

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <GL/gl.h>
#include <GL/GLGeometryWrappers.h>
#include <GLMotif/Event.h>
#include <GLMotif/TitleBar.h>
#include <GLMotif/WidgetManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/WidgetTool.h>

namespace Vrui {

/**********************************
Methods of class WidgetToolFactory:
**********************************/

WidgetToolFactory::WidgetToolFactory(ToolManager& toolManager)
	:ToolFactory("WidgetTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("UserInterfaceTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	
	/* Set tool class' factory pointer: */
	WidgetTool::factory=this;
	}

WidgetToolFactory::~WidgetToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	WidgetTool::factory=0;
	}

Tool* WidgetToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new WidgetTool(this,inputAssignment);
	}

void WidgetToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveWidgetToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("UserInterfaceTool");
	}

extern "C" ToolFactory* createWidgetToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	WidgetToolFactory* widgetToolFactory=new WidgetToolFactory(*toolManager);
	
	/* Return factory object: */
	return widgetToolFactory;
	}

extern "C" void destroyWidgetToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/***********************************
Static elements of class WidgetTool:
***********************************/

WidgetToolFactory* WidgetTool::factory=0;

/***************************
Methods of class WidgetTool:
***************************/

Ray WidgetTool::calcSelectionRay(void) const
	{
	/* Get pointer to input device: */
	InputDevice* device=input.getDevice(0);
	
	/* Calculate ray equation: */
	Point start=device->getPosition();
	Vector direction=device->getRayDirection();
	return Ray(start,direction);
	}

WidgetTool::WidgetTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:UserInterfaceTool(factory,inputAssignment),
	 insideWidget(false),active(false),dragging(false),draggedWidget(0)
	{
	}

const ToolFactory* WidgetTool::getFactory(void) const
	{
	return factory;
	}

void WidgetTool::buttonCallback(int,int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* If the widget manager accepts the event, preempt any cascaded tools until the button is released: */
		GLMotif::Event event(false);
		event.setWorldLocation(calcSelectionRay());
		if(getWidgetManager()->pointerButtonDown(event))
			{
			/* Activate this tool: */
			active=true;
			
			/* Drag the entire root widget if the event's target widget is a title bar: */
			if(dynamic_cast<GLMotif::TitleBar*>(event.getTargetWidget())!=0)
				{
				/* Start dragging: */
				dragging=true;
				draggedWidget=event.getTargetWidget();
				
				/* Calculate the dragging transformation: */
				NavTrackerState initialTracker=input.getDevice(0)->getTransformation();
				preScale=Geometry::invert(initialTracker);
				GLMotif::WidgetManager::Transformation initialWidget=getWidgetManager()->calcWidgetTransformation(draggedWidget);
				preScale*=NavTrackerState(initialWidget);
				}
			
			/* Cancel processing of this callback to preempt cascaded tools: */
			cbData->callbackList->requestInterrupt();
			}
		}
	else // Button has just been released
		{
		if(active)
			{
			/* Deliver the event: */
			GLMotif::Event event(true);
			event.setWorldLocation(calcSelectionRay());
			getWidgetManager()->pointerButtonUp(event);
			
			/* Deactivate this tool: */
			dragging=false;
			active=false;
			
			/* Cancel processing of this callback to preempt cascaded tools: */
			cbData->callbackList->requestInterrupt();
			}
		}
	}

void WidgetTool::frame(void)
	{
	/* Update the selection ray: */
	selectionRay=calcSelectionRay();
	insideWidget=getWidgetManager()->findPrimaryWidget(selectionRay)!=0;
	
	if(active)
		{
		/* Deliver the event: */
		GLMotif::Event event(true);
		event.setWorldLocation(selectionRay);
		getWidgetManager()->pointerMotion(event);
		
		if(dragging)
			{
			/* Update the dragged widget's transformation: */
			NavTrackerState current=input.getDevice(0)->getTransformation();
			current*=preScale;
			getWidgetManager()->setPrimaryWidgetTransformation(draggedWidget,GLMotif::WidgetManager::Transformation(current));
			}
		}
	}

void WidgetTool::display(GLContextData&) const
	{
	if(insideWidget||active)
		{
		/* Draw the menu selection ray: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glColor3f(1.0f,0.0f,0.0f);
		glLineWidth(3.0f);
		glBegin(GL_LINES);
		glVertex(selectionRay.getOrigin());
		glVertex(selectionRay(getDisplaySize()*Scalar(5)));
		glEnd();
		glPopAttrib();
		}
	}

}
