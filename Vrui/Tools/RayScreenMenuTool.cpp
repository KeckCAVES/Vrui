/***********************************************************************
RayScreenMenuTool - Class for menu selection tools using ray selection
that align menus to screen planes.
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
#include <Vrui/VRScreen.h>
#include <Vrui/MutexMenu.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/RayScreenMenuTool.h>

namespace Vrui {

/*****************************************
Methods of class RayScreenMenuToolFactory:
*****************************************/

RayScreenMenuToolFactory::RayScreenMenuToolFactory(ToolManager& toolManager)
	:ToolFactory("RayScreenMenuTool",toolManager),
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
	interactWithWidgets=cfs.retrieveValue<bool>("./interactWithWidgets",interactWithWidgets);
	
	/* Set tool class' factory pointer: */
	RayScreenMenuTool::factory=this;
	}

RayScreenMenuToolFactory::~RayScreenMenuToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	RayScreenMenuTool::factory=0;
	}

const char* RayScreenMenuToolFactory::getName(void) const
	{
	return "Screen-Aligned Menu";
	}

Tool* RayScreenMenuToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new RayScreenMenuTool(this,inputAssignment);
	}

void RayScreenMenuToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveRayScreenMenuToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("MenuTool");
	}

extern "C" ToolFactory* createRayScreenMenuToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	RayScreenMenuToolFactory* rayMenuToolFactory=new RayScreenMenuToolFactory(*toolManager);
	
	/* Return factory object: */
	return rayMenuToolFactory;
	}

extern "C" void destroyRayScreenMenuToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/******************************************
Static elements of class RayScreenMenuTool:
******************************************/

RayScreenMenuToolFactory* RayScreenMenuTool::factory=0;

/**********************************
Methods of class RayScreenMenuTool:
**********************************/

RayScreenMenuTool::RayScreenMenuTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
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

const ToolFactory* RayScreenMenuTool::getFactory(void) const
	{
	return factory;
	}

void RayScreenMenuTool::buttonCallback(int,int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Activation button has just been pressed
		{
		/* Check if the tool is interacting with a widget: */
		if(factory->interactWithWidgets)
			{
			/* If the widget manager accepts the event, preempt any cascaded tools until the button is released: */
			Ray ray=calcInteractionRay();
			GLMotif::Event event(false);
			event.setWorldLocation(ray);
			if(getWidgetManager()->pointerButtonDown(event))
				{
				/* Activate the widget tool: */
				widgetActive=true;
				
				/* Drag the entire root widget if the event's target widget is a title bar: */
				if(dynamic_cast<GLMotif::TitleBar*>(event.getTargetWidget())!=0)
					{
					/* Find the closest intersection with any screen: */
					std::pair<VRScreen*,Scalar> si=findScreen(ray);
					if(si.first!=0)
						{
						/* Start dragging: */
						dragging=true;
						draggedWidget=event.getTargetWidget();
						
						/* Calculate the dragging transformation: */
						NavTrackerState initialTracker=NavTrackerState::translateFromOriginTo(ray(si.second));
						preScale=Geometry::invert(initialTracker);
						GLMotif::WidgetManager::Transformation initialWidget=getWidgetManager()->calcWidgetTransformation(draggedWidget);
						preScale*=NavTrackerState(initialWidget);
						}
					}
				
				/* Cancel processing of this callback to preempt cascaded tools: */
				cbData->callbackList->requestInterrupt();
				}
			}
		
		/* Try activating this tool: */
		if(!widgetActive&&activate())
			{
			/* Calculate the menu selection ray: */
			Ray ray=calcInteractionRay();
			
			/* Find the closest intersection with any screen: */
			std::pair<VRScreen*,Scalar> si=findScreen(ray);
			
			if(si.first!=0)
				{
				typedef GLMotif::WidgetManager::Transformation WTransform;
				typedef WTransform::Point WPoint;
				typedef WTransform::Vector WVector;
				
				/* Calculate the menu transformation: */
				WPoint globalHotSpot=ray(si.second);
				
				/* Try to align the menu with the viewing direction: */
				ONTransform screenT=si.first->getScreenTransformation();
				WTransform menuTransformation=WTransform::translate(globalHotSpot-screenT.getOrigin());
				menuTransformation*=screenT;
				GLMotif::Vector menuHotSpot=menu->getPopup()->calcHotSpot();
				menuTransformation*=WTransform::translate(-WVector(menuHotSpot.getXyzw()));
				
				/* Pop up the menu: */
				getWidgetManager()->popupPrimaryWidget(menu->getPopup(),menuTransformation);
				
				/* Deliver the event: */
				GLMotif::Event event(false);
				event.setWorldLocation(ray);
				getWidgetManager()->pointerButtonDown(event);
				}
			else
				deactivate();
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

void RayScreenMenuTool::frame(void)
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
			/* Find the closest intersection with any screen: */
			std::pair<VRScreen*,Scalar> si=findScreen(selectionRay);
			
			if(si.first!=0)
				{
				/* Update the dragged widget's transformation: */
				NavTrackerState current=NavTrackerState::translateFromOriginTo(selectionRay(si.second));
				current*=preScale;
				getWidgetManager()->setPrimaryWidgetTransformation(draggedWidget,GLMotif::WidgetManager::Transformation(current));
				}
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

void RayScreenMenuTool::display(GLContextData&) const
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
