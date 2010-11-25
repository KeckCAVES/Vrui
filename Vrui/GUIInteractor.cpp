/***********************************************************************
GUIInteractor - Helper class to implement tool classes that interact
with graphical user interface elements.
Copyright (c) 2010 Oliver Kreylos

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
#include <GL/GLGeometryWrappers.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/Event.h>
#include <GLMotif/Draggable.h>
#include <GLMotif/Widget.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>
#include <Vrui/VRScreen.h>
#include <Vrui/Viewer.h>
#include <Vrui/Internal/Vrui.h>

namespace Vrui {

/**************************************
Static elements of class GUIInteractor:
**************************************/

GUIInteractor* GUIInteractor::activeInteractor=0;

/******************************
Methods of class GUIInteractor:
******************************/

GUIInteractor::GUIInteractor(bool sUseEyeRays,Scalar sRayOffset,InputDevice* sDevice)
	:useEyeRays(sUseEyeRays),rayOffset(sRayOffset),device(sDevice),
	 pointing(false),interacting(false),draggedWidget(0)
	{
	}

GUIInteractor::~GUIInteractor(void)
	{
	}

void GUIInteractor::updateRay(void)
	{
	if(useEyeRays)
		{
		/* Shoot a ray from the main viewer: */
		Point start=getMainViewer()->getHeadPosition();
		ray=Ray(start,device->getPosition()-start);
		ray.normalizeDirection();
		}
	else
		{
		/* Use the device's ray direction: */
		ray=Ray(device->getPosition(),device->getRayDirection());
		ray.normalizeDirection();
		
		/* Offset the ray start point backwards: */
		ray.setOrigin(ray(-rayOffset));
		}
	}

NavTrackerState GUIInteractor::calcInteractionTransform(void) const
	{
	NavTrackerState result;
	
	if(device->isRayDevice())
		{
		/*******************************************************************
		Calculate a transformation aligned with the first screen intersected
		by the ray:
		*******************************************************************/
		
		/* Get the first screen intersection: */
		std::pair<VRScreen*,Scalar> si=findScreen(ray);
		if(si.first!=0)
			{
			/* Get the screen's transformation: */
			result=si.first->getScreenTransformation();
			
			/* Set the intersection point as origin: */
			result.getTranslation()=ray(si.second)-Point::origin;
			}
		else
			{
			/* Create a translation to the ray's origin: */
			result=NavTrackerState::translateFromOriginTo(ray.getOrigin());
			}
		}
	else
		{
		/*******************************************************************
		Use the device's transformation directly:
		*******************************************************************/
		
		result=device->getTransformation();
		}
	
	return result;
	}

bool GUIInteractor::buttonDown(bool force)
	{
	/* Ensure that no other GUI interactor is currently active: */
	if(activeInteractor==0||activeInteractor==this)
		{
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

			/* Go into interaction mode: */
			interacting=true;
			}
		
		if(interacting&&activeInteractor==0)
			{
			/* Activate this interactor: */
			activeInteractor=this;
			setMostRecentGUIInteractor(this);
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
		interacting=false;
		draggedWidget=0;
		activeInteractor=0;
		}
	}

void GUIInteractor::move(void)
	{
	if(activeInteractor==0||activeInteractor==this)
		{
		/* Check if the interactor is pointing at a widget: */
		pointing=getWidgetManager()->findPrimaryWidget(ray)!=0;
		
		/* Check if the interactor is interacting with a widget: */
		if(interacting)
			{
			/* Check if the interactor is dragging a top-level widget: */
			if(draggedWidget!=0)
				{
				/* Calculate the new dragging transformation: */
				NavTrackerState newTransform=calcInteractionTransform();
				newTransform*=draggingTransform;
				newTransform.renormalize();
				getWidgetManager()->setPrimaryWidgetTransformation(draggedWidget,newTransform);
				}
			
			/* Deliver the event: */
			GLMotif::Event event(ray,true);
			getWidgetManager()->pointerMotion(event);
			}
		}
	}

void GUIInteractor::glRenderAction(GLContextData& contextData) const
	{
	/* Check if the interaction ray needs to be drawn: */
	if(pointing||interacting)
		{
		/* Save and set up OpenGL state: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glColor3f(1.0f,0.0f,0.0f);
		glLineWidth(3.0f);
		
		/* Draw the current interaction ray: */
		glBegin(GL_LINES);
		glVertex(ray.getOrigin());
		glVertex(ray(getDisplaySize()*Scalar(5)));
		glEnd();
		
		/* Restore OpenGL state: */
		glPopAttrib();
		}
	}

Point GUIInteractor::calcHotSpot(void) const
	{
	if(device->isRayDevice())
		{
		/* Get the first screen intersection: */
		std::pair<VRScreen*,Scalar> si=findScreen(ray);
		if(si.first!=0)
			{
			/* Return the intersection point: */
			return ray(si.second);
			}
		else
			{
			/* Return the ray's origin: */
			return ray.getOrigin();
			}
		}
	else
		{
		/* Return the device's position: */
		return device->getPosition();
		}
	}

}
