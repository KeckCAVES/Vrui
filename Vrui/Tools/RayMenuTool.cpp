/***********************************************************************
RayMenuTool - Class for menu selection tools using ray selection.
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
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <GL/gl.h>
#include <GL/GLGeometryWrappers.h>
#include <GLMotif/Event.h>
#include <GLMotif/TitleBar.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/PopupMenu.h>
#include <Vrui/Viewer.h>
#include <Vrui/MutexMenu.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/RayMenuTool.h>

namespace Vrui {

/***********************************
Methods of class RayMenuToolFactory:
***********************************/

RayMenuToolFactory::RayMenuToolFactory(ToolManager& toolManager)
	:ToolFactory("RayMenuTool",toolManager),
	 initialMenuOffset(getInchFactor()*Scalar(6)),
	 interactWithWidgets(true)
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* menuToolFactory=toolManager.loadClass("MenuTool");
	menuToolFactory->addChildClass(this);
	addParentClass(menuToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	initialMenuOffset=cfs.retrieveValue<Scalar>("./initialMenuOffset",initialMenuOffset);
	interactWithWidgets=cfs.retrieveValue<bool>("./interactWithWidgets",interactWithWidgets);
	
	/* Set tool class' factory pointer: */
	RayMenuTool::factory=this;
	}

RayMenuToolFactory::~RayMenuToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	RayMenuTool::factory=0;
	}

const char* RayMenuToolFactory::getName(void) const
	{
	return "Free-Standing Menu";
	}

Tool* RayMenuToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new RayMenuTool(this,inputAssignment);
	}

void RayMenuToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveRayMenuToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("MenuTool");
	}

extern "C" ToolFactory* createRayMenuToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	RayMenuToolFactory* rayMenuToolFactory=new RayMenuToolFactory(*toolManager);
	
	/* Return factory object: */
	return rayMenuToolFactory;
	}

extern "C" void destroyRayMenuToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/************************************
Static elements of class RayMenuTool:
************************************/

RayMenuToolFactory* RayMenuTool::factory=0;

/****************************
Methods of class RayMenuTool:
****************************/

RayMenuTool::RayMenuTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:MenuTool(factory,inputAssignment),
	 viewer(0),
	 insideWidget(false),widgetActive(false),
	 dragging(false),draggedWidget(0)
	{
	/* Retrieve the viewer associated with this menu tool: */
	#if 0
	int viewerIndex=configFile.retrieveValue<int>("./viewerIndex");
	viewer=getViewer(viewerIndex);
	#else
	viewer=getMainViewer();
	#endif
	}

const ToolFactory* RayMenuTool::getFactory(void) const
	{
	return factory;
	}

void RayMenuTool::buttonCallback(int,int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Activation button has just been pressed
		{
		/* Check if the tool is interacting with a widget: */
		if(factory->interactWithWidgets)
			{
			/* If the widget manager accepts the event, preempt any cascaded tools until the button is released: */
			GLMotif::Event event(false);
			event.setWorldLocation(calcInteractionRay());
			if(getWidgetManager()->pointerButtonDown(event))
				{
				/* Activate the widget tool: */
				widgetActive=true;
				
				/* Drag the entire root widget if the event's target widget is a title bar: */
				if(dynamic_cast<GLMotif::TitleBar*>(event.getTargetWidget())!=0)
					{
					/* Start dragging: */
					dragging=true;
					draggedWidget=event.getTargetWidget();
					
					/* Calculate the dragging transformation: */
					NavTrackerState initialTracker=getDeviceTransformation(0);
					preScale=Geometry::invert(initialTracker);
					GLMotif::WidgetManager::Transformation initialWidget=getWidgetManager()->calcWidgetTransformation(draggedWidget);
					preScale*=NavTrackerState(initialWidget);
					}
				
				/* Cancel processing of this callback to preempt cascaded tools: */
				cbData->callbackList->requestInterrupt();
				}
			}
		
		/* Try activating this tool: */
		if(!widgetActive&&activate())
			{
			typedef GLMotif::WidgetManager::Transformation WTransform;
			typedef WTransform::Point WPoint;
			typedef WTransform::Vector WVector;
			
			/* Calculate the menu transformation: */
			WPoint globalHotSpot=calcInteractionRay()(factory->initialMenuOffset);
			
			/* Align the widget with the viewing direction: */
			WVector viewDirection=globalHotSpot-viewer->getHeadPosition();
			WVector x=Geometry::cross(viewDirection,getUpDirection());
			WVector y=Geometry::cross(x,viewDirection);
			WTransform menuTransformation=WTransform::translateFromOriginTo(globalHotSpot);
			WTransform::Rotation rot=WTransform::Rotation::fromBaseVectors(x,y);
			menuTransformation*=WTransform::rotate(rot);
			menuTransformation*=WTransform::scale(getInchFactor());
			GLMotif::Vector menuHotSpot=menu->getPopup()->calcHotSpot();
			menuTransformation*=WTransform::translate(-WVector(menuHotSpot.getXyzw()));
			
			/* Pop up the menu: */
			getWidgetManager()->popupPrimaryWidget(menu->getPopup(),menuTransformation);
			
			/* Deliver the event: */
			GLMotif::Event event(false);
			event.setWorldLocation(calcInteractionRay());
			getWidgetManager()->pointerButtonDown(event);
			}
		}
	else // Activation button has just been released
		{
		if(widgetActive)
			{
			/* Deliver the event: */
			GLMotif::Event event(true);
			event.setWorldLocation(calcInteractionRay());
			getWidgetManager()->pointerButtonUp(event);
			
			/* Deactivate this tool: */
			dragging=false;
			widgetActive=false;
			
			/* Cancel processing of this callback to preempt cascaded tools: */
			cbData->callbackList->requestInterrupt();
			}
		else if(isActive())
			{
			/* Deliver the event: */
			GLMotif::Event event(true);
			event.setWorldLocation(calcInteractionRay());
			getWidgetManager()->pointerButtonUp(event);
			
			/* Pop down the menu: */
			getWidgetManager()->popdownWidget(menu->getPopup());
			
			/* Deactivate the tool: */
			deactivate();
			}
		}
	}

void RayMenuTool::frame(void)
	{
	/* Update the selection ray: */
	selectionRay=calcInteractionRay();
	
	if(factory->interactWithWidgets)
		insideWidget=getWidgetManager()->findPrimaryWidget(selectionRay)!=0;
	
	if(widgetActive)
		{
		/* Deliver the event: */
		GLMotif::Event event(true);
		event.setWorldLocation(selectionRay);
		getWidgetManager()->pointerMotion(event);
		
		if(dragging)
			{
			/* Update the dragged widget's transformation: */
			NavTrackerState current=getDeviceTransformation(0);
			current*=preScale;
			getWidgetManager()->setPrimaryWidgetTransformation(draggedWidget,GLMotif::WidgetManager::Transformation(current));
			}
		}
	else if(isActive())
		{
		/* Update the selection ray: */
		selectionRay=calcInteractionRay();
		
		/* Deliver the event: */
		GLMotif::Event event(true);
		event.setWorldLocation(selectionRay);
		getWidgetManager()->pointerMotion(event);
		}
	}

void RayMenuTool::display(GLContextData&) const
	{
	if(insideWidget||widgetActive||isActive())
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
