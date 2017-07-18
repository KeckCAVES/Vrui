/***********************************************************************
GUIInteractor - Helper class to implement tool classes that interact
with graphical user interface elements.
Copyright (c) 2010-2016 Oliver Kreylos

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

#include <Vrui/GUIInteractor.h>

#include <Geometry/Point.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/Event.h>
#include <GLMotif/Draggable.h>
#include <GLMotif/Widget.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>
#include <Vrui/VRScreen.h>
#include <Vrui/Viewer.h>
#include <Vrui/UIManager.h>
#include <Vrui/Internal/Vrui.h>

namespace Vrui {

/******************************
Methods of class GUIInteractor:
******************************/

GUIInteractor::GUIInteractor(bool sUseEyeRays,Scalar sRayOffset,InputDevice* sDevice)
	:useEyeRays(sUseEyeRays),rayOffset(sRayOffset),device(sDevice),
	 pointing(false),interacting(false),draggedWidget(0)
	{
	/* Always use eye rays if the device has no direction: */
	if(!device->hasDirection())
		useEyeRays=true;
	}

GUIInteractor::~GUIInteractor(void)
	{
	/* Deregister this GUI interactor with the UI manager: */
	getUiManager()->destroyGuiInteractor(this);
	}

void GUIInteractor::updateRay(void)
	{
	if(useEyeRays)
		{
		/* Shoot a ray from the main viewer: */
		Point start=getMainViewer()->getHeadPosition();
		ray=Ray(start,device->getPosition()-start);
		}
	else
		{
		/* Use the device's ray direction: */
		ray=device->getRay();
		}
	
	/* Make the ray unit length: */
	ray.normalizeDirection();
	}

NavTrackerState GUIInteractor::calcInteractionTransform(void) const
	{
	/* Use the device's transformation directly: */
	return device->getTransformation();
	}

bool GUIInteractor::canActivate(void) const
	{
	return interacting||getUiManager()->canActivateGuiInteractor(this);
	}

bool GUIInteractor::buttonDown(bool force)
	{
	/* Try activating this GUI interactor: */
	if(getUiManager()->activateGuiInteractor(this))
		{
		interacting=true;
		
		/* Create a GLMotif event: */
		GLMotif::Event event(ray,false);
		
		/* Check if there is a recipient for the event: */
		if(getWidgetManager()->pointerButtonDown(event)||force)
			{
			/* Check whether the target widget is a draggable title bar: */
			if(dynamic_cast<GLMotif::Draggable*>(event.getTargetWidget())!=0)
				{
				/* Drag the entire top-level widget: */
				draggedWidget=event.getTargetWidget();
				
				/* Calculate the dragging transformation: */
				draggingTransform=calcInteractionTransform();
				draggingTransform.doInvert();
				GLMotif::WidgetManager::Transformation initialWidget=getWidgetManager()->calcWidgetTransformation(draggedWidget);
				draggingTransform*=NavTrackerState(initialWidget);
				draggingTransform.renormalize();
				}
			}
		else
			{
			/* No event sent; deactivate this GUI interactor again: */
			getUiManager()->deactivateGuiInteractor(this);
			interacting=false;
			}
		}
	
	return interacting;
	}

void GUIInteractor::buttonUp(void)
	{
	if(interacting)
		{
		/* Deliver the event: */
		GLMotif::Event event(ray,true);
		getWidgetManager()->pointerButtonUp(event);
		
		/* Deactivate the interactor: */
		getUiManager()->deactivateGuiInteractor(this);
		interacting=false;
		draggedWidget=0;
		}
	}

void GUIInteractor::move(void)
	{
	if(interacting||getUiManager()->canActivateGuiInteractor(this))
		{
		/* Check if the interactor is pointing at a widget: */
		pointing=getWidgetManager()->findPrimaryWidget(ray)!=0;
		
		/* Check if the interactor is dragging a top-level widget: */
		if(interacting&&draggedWidget!=0)
			{
			/* Calculate the new dragging transformation: */
			NavTrackerState newTransform=calcInteractionTransform();
			newTransform*=draggingTransform;
			newTransform.renormalize();
			getWidgetManager()->setPrimaryWidgetTransformation(draggedWidget,newTransform);
			}
		
		if(pointing||interacting)
			{
			/* Deliver the event: */
			GLMotif::Event event(ray,interacting);
			getWidgetManager()->pointerMotion(event);
			}
		}
	else
		{
		/* Stop pointing if another GUI interactor is active: */
		pointing=false;
		}
	}

bool GUIInteractor::textControl(const GLMotif::TextControlEvent& textControlEvent)
	{
	/* Try activating this GUI interactor: */
	if(getUiManager()->activateGuiInteractor(this))
		{
		/* Send a GLMotif event and a text control event to the widget manager: */
		GLMotif::Event event(ray,false);
		bool result=getWidgetManager()->textControl(event,textControlEvent);
		
		/* Deactivate the GUI interactor again: */
		getUiManager()->deactivateGuiInteractor(this);
		
		return result;
		}
	else
		return false;
	}

void GUIInteractor::glRenderAction(GLfloat rayWidth,const GLColor<GLfloat,4>& rayColor,GLContextData& contextData) const
	{
	/* Check if the interaction ray needs to be drawn: */
	if(!useEyeRays&&(pointing||interacting))
		{
		/* Save and set up OpenGL state: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(rayWidth);
		
		/* Draw the current interaction ray: */
		glBegin(GL_LINES);
		glColor(rayColor);
		glVertex(ray.getOrigin());
		glVertex(ray(getDisplaySize()*Scalar(5)));
		glEnd();
		
		/* Restore OpenGL state: */
		glPopAttrib();
		}
	}

Point GUIInteractor::calcHotSpot(void) const
	{
	/* Project the interaction ray into the UI manager's interaction surface: */
	return getUiManager()->projectRay(Ray(device->getPosition(),ray.getDirection()));
	}

}
