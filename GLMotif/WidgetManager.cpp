/***********************************************************************
WidgetManager - Class to manage top-level GLMotif UI components and user
events.
Copyright (c) 2001-2008 Oliver Kreylos

This file is part of the GLMotif Widget Library (GLMotif).

The GLMotif Widget Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GLMotif Widget Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the GLMotif Widget Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <GL/gl.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/Event.h>
#include <GLMotif/Widget.h>

#include <GLMotif/WidgetManager.h>

namespace GLMotif {

/********************************************
Methods of class WidgetManager::PopupBinding:
********************************************/

WidgetManager::PopupBinding::PopupBinding(Widget* sTopLevelWidget,const WidgetManager::Transformation& sWidgetToWorld,WidgetManager::PopupBinding* sParent,WidgetManager::PopupBinding* sSucc)
	:topLevelWidget(sTopLevelWidget),widgetToWorld(sWidgetToWorld),visible(true),
	 parent(sParent),pred(0),succ(sSucc),firstSecondary(0)
	{
	}

WidgetManager::PopupBinding::~PopupBinding(void)
	{
	/* Delete all this binding's secondary bindings: */
	while(firstSecondary!=0)
		{
		PopupBinding* next=firstSecondary->succ;
		delete firstSecondary;
		firstSecondary=next;
		}
	}

WidgetManager::PopupBinding* WidgetManager::PopupBinding::findTopLevelWidget(const Point& point)
	{
	Point widgetPoint=widgetToWorld.inverseTransform(point);
	PopupBinding* foundBinding=0;
	
	/* Check if our widget contains the given point: */
	if(topLevelWidget->isInside(widgetPoint))
		foundBinding=this;
	
	/* Traverse through all secondary bindings: */
	for(PopupBinding* bPtr=firstSecondary;bPtr!=0&&foundBinding==0;bPtr=bPtr->succ)
		foundBinding=bPtr->findTopLevelWidget(widgetPoint);
	
	return foundBinding;
	}

WidgetManager::PopupBinding* WidgetManager::PopupBinding::findTopLevelWidget(const Ray& ray)
	{
	Ray widgetRay=ray;
	widgetRay.inverseTransform(widgetToWorld);
	PopupBinding* foundBinding=0;
	
	/* Check if our widget intersects the given ray: */
	Point intersection;
	Scalar lambda=topLevelWidget->intersectRay(widgetRay,intersection);
	if(lambda>=Scalar(0)&&topLevelWidget->isInside(intersection))
		foundBinding=this;
	
	/* Traverse through all secondary bindings: */
	for(PopupBinding* bPtr=firstSecondary;bPtr!=0&&foundBinding==0;bPtr=bPtr->succ)
		foundBinding=bPtr->findTopLevelWidget(widgetRay);
	
	return foundBinding;
	}

void WidgetManager::PopupBinding::draw(bool overlayWidgets,GLContextData& contextData) const
	{
	if(visible)
		{
		glPushMatrix();
		
		/* Go to the top-level widget's coordinate system: */
		glMultMatrix(widgetToWorld);
		
		/* Draw all its secondary top level widgets: */
		for(PopupBinding* bPtr=firstSecondary;bPtr!=0;bPtr=bPtr->succ)
			bPtr->draw(overlayWidgets,contextData);
		
		/* Draw the top level widget: */
		topLevelWidget->draw(contextData);
		
		if(overlayWidgets)
			{
			/* Draw the top level widget again to squash the z buffer: */
			GLdouble depthRange[2];
			glGetDoublev(GL_DEPTH_RANGE,depthRange);
			glDepthRange(0.0,0.0);
			GLboolean colorMask[4];
			glGetBooleanv(GL_COLOR_WRITEMASK,colorMask);
			glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
			topLevelWidget->draw(contextData);
			glColorMask(colorMask[0],colorMask[1],colorMask[2],colorMask[3]);
			glDepthRange(depthRange[0],depthRange[1]);
			}
		
		glPopMatrix();
		}
	}

/******************************
Methods of class WidgetManager:
******************************/

void WidgetManager::deleteQueuedWidgets(void)
	{
	/* Delete all queued-up widgets: */
	for(std::vector<Widget*>::iterator dlIt=deletionList.begin();dlIt!=deletionList.end();++dlIt)
		{
		/* Release a pointer grab held by the widget: */
		if(pointerGrabWidget==*dlIt)
			{
			hardGrab=false;
			pointerGrabWidget=0;
			}
		
		/* Pop down the widget if it is a managed root widget: */
		if((*dlIt)->getParent()==0)
			{
			/* Find the widget's binding: */
			PopupBinding* bPtr;
			for(bPtr=firstBinding;bPtr!=0&&bPtr->topLevelWidget!=*dlIt;bPtr=bPtr->getSucc())
				;
			
			if(bPtr!=0)
				{
				/* Remove the binding from the list: */
				if(bPtr->pred!=0)
					bPtr->pred->succ=bPtr->succ;
				else if(bPtr->parent!=0)
					bPtr->parent->firstSecondary=bPtr->succ;
				else
					firstBinding=bPtr->succ;
				if(bPtr->succ!=0)
					bPtr->succ->pred=bPtr->pred;
				delete bPtr;
				}
			}
		
		delete *dlIt;
		}
	deletionList.clear();
	}

WidgetManager::WidgetManager(void)
	:styleSheet(0),timerEventScheduler(0),drawOverlayWidgets(false),
	 firstBinding(0),
	 time(0.0),
	 hardGrab(false),pointerGrabWidget(0),
	 inEventProcessing(false)
	{
	}

WidgetManager::~WidgetManager(void)
	{
	/* Delete all bindings: */
	while(firstBinding!=0)
		{
		PopupBinding* next=firstBinding->succ;
		delete firstBinding;
		firstBinding=next;
		}
	}

void WidgetManager::setStyleSheet(const StyleSheet* newStyleSheet)
	{
	styleSheet=newStyleSheet;
	}

void WidgetManager::setTimerEventScheduler(Misc::TimerEventScheduler* newTimerEventScheduler)
	{
	timerEventScheduler=newTimerEventScheduler;
	}

void WidgetManager::setDrawOverlayWidgets(bool newDrawOverlayWidgets)
	{
	drawOverlayWidgets=newDrawOverlayWidgets;
	}

void WidgetManager::popupPrimaryWidget(Widget* topLevelWidget,const WidgetManager::Transformation& widgetToWorld)
	{
	PopupBinding* newBinding=new PopupBinding(topLevelWidget,widgetToWorld,0,firstBinding);
	if(firstBinding!=0)
		firstBinding->pred=newBinding;
	firstBinding=newBinding;
	}

void WidgetManager::popupSecondaryWidget(Widget* owner,Widget* topLevelWidget,const Vector& offset)
	{
	/* Find the owner's binding: */
	Widget* root=owner->getRoot();
	PopupBinding* bPtr;
	for(bPtr=firstBinding;bPtr!=0&&bPtr->topLevelWidget!=root;bPtr=bPtr->getSucc())
		;
	
	if(bPtr!=0)
		{
		PopupBinding* newBinding=new PopupBinding(topLevelWidget,Transformation::translate(Transformation::Vector(offset.getXyzw())),bPtr,bPtr->firstSecondary);
		if(bPtr->firstSecondary!=0)
			bPtr->firstSecondary->pred=newBinding;
		bPtr->firstSecondary=newBinding;
		}
	}

void WidgetManager::popdownWidget(Widget* widget)
	{
	/* Find the widget's binding: */
	Widget* root=widget->getRoot();
	PopupBinding* bPtr;
	for(bPtr=firstBinding;bPtr!=0&&bPtr->topLevelWidget!=root;bPtr=bPtr->getSucc())
		;
	
	if(bPtr!=0)
		{
		/* Remove the binding from the list: */
		if(bPtr->pred!=0)
			bPtr->pred->succ=bPtr->succ;
		else if(bPtr->parent!=0)
			bPtr->parent->firstSecondary=bPtr->succ;
		else
			firstBinding=bPtr->succ;
		if(bPtr->succ!=0)
			bPtr->succ->pred=bPtr->pred;
		delete bPtr;
		}
	}

void WidgetManager::show(Widget* widget)
	{
	/* Find the widget's binding: */
	Widget* root=widget->getRoot();
	PopupBinding* bPtr;
	for(bPtr=firstBinding;bPtr!=0&&bPtr->topLevelWidget!=root;bPtr=bPtr->getSucc())
		;
	
	/* Mark the binding as visible: */
	if(bPtr!=0)
		bPtr->visible=true;
	}

void WidgetManager::hide(Widget* widget)
	{
	/* Find the widget's binding: */
	Widget* root=widget->getRoot();
	PopupBinding* bPtr;
	for(bPtr=firstBinding;bPtr!=0&&bPtr->topLevelWidget!=root;bPtr=bPtr->getSucc())
		;
	
	/* Mark the binding as invisible: */
	if(bPtr!=0)
		bPtr->visible=false;
	}

bool WidgetManager::isManaged(const Widget* widget) const
	{
	/* Find the widget's binding: */
	const Widget* root=widget->getRoot();
	PopupBinding* bPtr;
	for(bPtr=firstBinding;bPtr!=0&&bPtr->topLevelWidget!=root;bPtr=bPtr->getSucc())
		;
	
	return bPtr!=0;
	}

bool WidgetManager::isVisible(const Widget* widget) const
	{
	/* Find the widget's binding: */
	const Widget* root=widget->getRoot();
	PopupBinding* bPtr;
	for(bPtr=firstBinding;bPtr!=0&&bPtr->topLevelWidget!=root;bPtr=bPtr->getSucc())
		;
	
	return bPtr!=0&&bPtr->visible;
	}

Widget* WidgetManager::findPrimaryWidget(const Point& point)
	{
	/* Find a recipient for this event amongst all primary bindings: */
	PopupBinding* foundBinding=0;
	for(PopupBinding* bPtr=firstBinding;bPtr!=0&&foundBinding==0;bPtr=bPtr->succ)
		foundBinding=bPtr->findTopLevelWidget(point);
	
	/* Bail out if no widget was found: */
	if(foundBinding==0)
		return 0;
	
	/* Find the primary top level widget containing the found widget: */
	while(foundBinding->parent!=0)
		foundBinding=foundBinding->parent;
	
	/* Return the top level widget: */
	return foundBinding->topLevelWidget;
	}

Widget* WidgetManager::findPrimaryWidget(const Ray& ray)
	{
	/* Find a recipient for this event amongst all primary bindings: */
	PopupBinding* foundBinding=0;
	for(PopupBinding* bPtr=firstBinding;bPtr!=0&&foundBinding==0;bPtr=bPtr->succ)
		foundBinding=bPtr->findTopLevelWidget(ray);
	
	/* Bail out if no widget was found: */
	if(foundBinding==0)
		return 0;
	
	/* Find the primary top level widget containing the found widget: */
	while(foundBinding->parent!=0)
		foundBinding=foundBinding->parent;
	
	/* Return the top level widget: */
	return foundBinding->topLevelWidget;
	}

WidgetManager::Transformation WidgetManager::calcWidgetTransformation(const Widget* widget) const
	{
	/* Find the widget's binding: */
	const Widget* root=widget->getRoot();
	const PopupBinding* bPtr;
	for(bPtr=firstBinding;bPtr!=0&&bPtr->topLevelWidget!=root;bPtr=bPtr->getSucc())
		;
	
	/* Concatenate all transformations up to the primary top level widget: */
	Transformation result=Transformation::identity;
	while(bPtr!=0)
		{
		result.leftMultiply(bPtr->widgetToWorld);
		bPtr=bPtr->parent;
		}
	
	return result;
	}

void WidgetManager::setPrimaryWidgetTransformation(Widget* widget,const WidgetManager::Transformation& newWidgetToWorld)
	{
	/* Find the widget's top level widget: */
	Widget* root=widget->getRoot();
	
	/* Find the root amongst the managed top level widgets: */
	for(PopupBinding* bPtr=firstBinding;bPtr!=0;bPtr=bPtr->succ)
		if(bPtr->topLevelWidget==root)
			{
			bPtr->widgetToWorld=newWidgetToWorld;
			break;
			}
	}

void WidgetManager::deleteWidget(Widget* widget)
	{
	if(inEventProcessing)
		{
		/* Mark the widget for deletion: */
		deletionList.push_back(widget);
		}
	else
		{
		/* Delete the widget immediately: */
		delete widget;
		}
	}

void WidgetManager::setTime(double newTime)
	{
	/* Set the time: */
	time=newTime;
	}

void WidgetManager::draw(GLContextData& contextData) const
	{
	/* Traverse all primary top level widgets: */
	for(const PopupBinding* bPtr=firstBinding;bPtr!=0;bPtr=bPtr->succ)
		bPtr->draw(drawOverlayWidgets,contextData);
	}

bool WidgetManager::pointerButtonDown(Event& event)
	{
	bool result=false;
	
	inEventProcessing=true;
	
	if(pointerGrabWidget!=0)
		{
		/* Allow the grabbing widget to modify the event: */
		pointerGrabWidget->findRecipient(event);
		}
	else
		{
		/* Find a recipient for this event amongst the primary top-level windows: */
		PopupBinding* foundTopLevel=0;
		for(PopupBinding* bPtr=firstBinding;bPtr!=0;bPtr=bPtr->succ)
			if(bPtr->topLevelWidget->findRecipient(event)&&drawOverlayWidgets)
				{
				foundTopLevel=bPtr;
				break;
				}
		
		if(foundTopLevel!=0&&foundTopLevel!=firstBinding)
			{
			/* Move the found top level widget to the front of the stacking order: */
			foundTopLevel->pred->succ=foundTopLevel->succ;
			if(foundTopLevel->succ!=0)
				foundTopLevel->succ->pred=foundTopLevel->pred;
			foundTopLevel->pred=0;
			foundTopLevel->succ=firstBinding;
			foundTopLevel->succ->pred=foundTopLevel;
			firstBinding=foundTopLevel;
			}
		}
	
	if(event.getTargetWidget()!=0)
		{
		if(!hardGrab)
			{
			/* Initiate a "soft" pointer grab: */
			pointerGrabWidget=event.getTargetWidget();
			}
		
		/* Pass the event to the found target: */
		event.getTargetWidget()->pointerButtonDown(event);
		
		result=true;
		}
	
	inEventProcessing=false;
	
	if(!deletionList.empty())
		deleteQueuedWidgets();
	
	return result;
	}

bool WidgetManager::pointerButtonUp(Event& event)
	{
	bool result=false;
	
	inEventProcessing=true;
	
	if(pointerGrabWidget!=0)
		{
		/* Allow the grabbing widget to modify the event: */
		pointerGrabWidget->findRecipient(event);
		
		/* Pass the event to the grabbing widget: */
		pointerGrabWidget->pointerButtonUp(event);
		
		/* Release any "soft" grabs: */
		if(!hardGrab)
			pointerGrabWidget=0;
		
		result=pointerGrabWidget!=0;
		}
	
	inEventProcessing=false;
	
	if(!deletionList.empty())
		deleteQueuedWidgets();
	
	return result;
	}

bool WidgetManager::pointerMotion(Event& event)
	{
	bool result=false;
	
	inEventProcessing=true;
	
	if(pointerGrabWidget!=0)
		{
		/* Allow the grabbing widget to modify the event: */
		pointerGrabWidget->findRecipient(event);
		
		/* Pass the event to the grabbing widget: */
		pointerGrabWidget->pointerMotion(event);
		
		result=pointerGrabWidget!=0;
		}
	else
		{
		/* Find a recipient for this event amongst the primary top-level windows: */
		for(PopupBinding* bPtr=firstBinding;bPtr!=0;bPtr=bPtr->succ)
			bPtr->topLevelWidget->findRecipient(event);
		
		if(event.getTargetWidget()!=0)
			{
			/* Pass the event to the found target: */
			event.getTargetWidget()->pointerMotion(event);
			
			result=true;
			}
		}
	
	inEventProcessing=false;
	
	if(!deletionList.empty())
		deleteQueuedWidgets();
	
	return result;
	}

void WidgetManager::grabPointer(Widget* widget)
	{
	if(pointerGrabWidget==0)
		{
		/* Find the widget's binding: */
		Widget* root=widget->getRoot();
		PopupBinding* bPtr;
		for(bPtr=firstBinding;bPtr!=0&&bPtr->topLevelWidget!=root;bPtr=bPtr->getSucc())
			;

		if(bPtr!=0)
			{
			hardGrab=true;
			pointerGrabWidget=widget;
			pointerGrabWidgetToWorld=calcWidgetTransformation(widget);
			}
		}
	else if(pointerGrabWidget==widget)
		hardGrab=true;
	}

void WidgetManager::releasePointer(Widget* widget)
	{
	if(widget==pointerGrabWidget&&hardGrab)
		{
		hardGrab=false;
		pointerGrabWidget=0;
		}
	}

}
