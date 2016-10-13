/***********************************************************************
CascadeButton - Class for buttons that pop up secondary top-level
GLMotif UI components.
Copyright (c) 2001-2015 Oliver Kreylos

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

#include <GLMotif/CascadeButton.h>

#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLNormalTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/Event.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/Container.h>
#include <GLMotif/Popup.h>

#define GLMOTIF_CASCADEBUTTON_CENTER_POPUPS 1

namespace GLMotif {

/******************************
Methods of class CascadeButton:
******************************/

void CascadeButton::setArmed(bool newArmed)
	{
	/* Call the base class widget's setArmed method: */
	DecoratedButton::setArmed(newArmed);
	
	/* Pop the secondary top level widget up or down: */
	if(isArmed&&!isPopped&&popup!=0)
		{
		/* Calculate the popup's transformation: */
		Vector offset=getExterior().getCorner(1);
		#if GLMOTIF_CASCADEBUTTON_CENTER_POPUPS
		offset[1]+=getExterior().size[1]*0.5f;
		#else
		offset[1]+=getExterior().size[1];
		#endif
		Vector popupHotSpot=popup->getChild()->getExterior().getCorner(0);
		#if GLMOTIF_CASCADEBUTTON_CENTER_POPUPS
		popupHotSpot[1]+=popup->getChild()->getExterior().size[1]*0.5f;
		#else
		popupHotSpot[1]+=popup->getChild()->getExterior().size[1];
		#endif
		for(int i=0;i<3;++i)
			offset[i]-=popupHotSpot[i];
		offset[2]+=getZRange().second-popup->getChild()->getZRange().first;
		getManager()->popupSecondaryWidget(this,popup,offset);
		isPopped=true;
		
		/* Calculate the bottom-left and top-left corners of the popup: */
		Point::Vector off(offset.getXyzw());
		popupBottom=Point(popup->getExterior().getCorner(0).getXyzw())+off;
		popupTop=Point(popup->getExterior().getCorner(2).getXyzw())+off;
		popupBottom[2]=popupTop[2]=Scalar(Math::mid(getZRange().first,getZRange().second));
		}
	else if(!isArmed&&isPopped)
		{
		popup->getManager()->popdownWidget(popup);
		isPopped=false;
		}
	}

void CascadeButton::drawDecoration(GLContextData& contextData) const
	{
	/* Draw the cascade button arrow: */
	glColor(backgroundColor);
	arrow.draw(contextData);
	}

CascadeButton::CascadeButton(const char* sName,Container* sParent,const char* sLabel,const GLFont* sFont,bool sManageChild)
	:DecoratedButton(sName,sParent,sLabel,sFont,false),
	 popup(0),isPopped(false),
	 foundWidget(0),
	 arrow(GlyphGadget::FANCY_ARROW_RIGHT,GlyphGadget::IN,0.0f)
	{
	/* Get the style sheet: */
	const StyleSheet* ss=getStyleSheet();
	
	/* Set the arrow sizes: */
	arrow.setGlyphSize(ss->size*0.25f);
	arrow.setBevelSize(ss->size*0.25f);
	arrow.setGlyphColor(backgroundColor);
	
	/* Set the decoration position and size: */
	setDecorationPosition(DecoratedButton::DECORATION_RIGHT);
	GLfloat width=arrow.getPreferredBoxSize();
	setDecorationSize(Vector(width,width,0.0f));
	
	/* Manage me: */
	if(sManageChild)
		manageChild();
	}

CascadeButton::CascadeButton(const char* sName,Container* sParent,const char* sLabel,bool sManageChild)
	:DecoratedButton(sName,sParent,sLabel,false),
	 popup(0),isPopped(false),
	 foundWidget(0),
	 arrow(GlyphGadget::FANCY_ARROW_RIGHT,GlyphGadget::IN,0.0f)
	{
	/* Get the style sheet: */
	const StyleSheet* ss=getStyleSheet();
	
	/* Set the arrow sizes: */
	arrow.setGlyphSize(ss->size*0.25f);
	arrow.setBevelSize(ss->size*0.25f);
	arrow.setGlyphColor(backgroundColor);
	
	/* Set the decoration position and size: */
	setDecorationPosition(DecoratedButton::DECORATION_RIGHT);
	GLfloat width=arrow.getPreferredBoxSize();
	setDecorationSize(Vector(width,width,0.0f));
	
	/* Manage me: */
	if(sManageChild)
		manageChild();
	}

CascadeButton::~CascadeButton(void)
	{
	delete popup;
	}

ZRange CascadeButton::calcZRange(void) const
	{
	/* Calculate the parent class widget's z range: */
	ZRange myZRange=DecoratedButton::calcZRange();
	
	/* Adjust for the cascade arrow: */
	myZRange+=arrow.calcZRange();
	
	return myZRange;
	}
	
void CascadeButton::resize(const Box& newExterior)
	{
	/* Resize the parent class widget: */
	DecoratedButton::resize(newExterior);
	
	/* Position the cascade arrow: */
	arrow.setGlyphBox(decorationBox);
	}

void CascadeButton::setBackgroundColor(const Color& newBackgroundColor)
	{
	/* Call the base class method: */
	DecoratedButton::setBackgroundColor(newBackgroundColor);
	
	/* Let the arrow glyph track the background color: */
	arrow.setGlyphColor(newBackgroundColor);
	}

bool CascadeButton::findRecipient(Event& event)
	{
	foundWidget=0;
	
	/* Reject events if the widget is disabled: */
	if(!isEnabled())
		return false;
	
	/* Find a recipient inside the popup first, if it's popped up: */
	if(isPopped&&popup->findRecipient(event))
		{
		/* Replace the found widget with ourselves to intercept future events: */
		foundWidget=event.overrideTargetWidget(this);
		return true;
		}
	
	/* Find the event's point in our coordinate system: */
	Event::WidgetPoint wp=event.calcWidgetPoint(this);
	foundPos=wp.getPoint();
	
	/* If the point is inside our bounding box, put us down as recipient: */
	if(isInside(foundPos))
		return event.setTargetWidget(this,wp);
	
	/* If the popup is popped up, check if the pointer is moving towards it: */
	if(isPopped&&foundPos[0]>=lastEventPos[0])
		{
		/* Calculate a plane orthogonal to the widget's plane containing the pointer's last movement line: */
		Point::Vector dir=foundPos-lastEventPos;
		Point::Vector normal(dir[1],dir[0],0); // 2D cross product
		Scalar o=foundPos*normal;
		
		/* The pointer is moving towards the popup if the top and bottom popup points are on different sides of the plane: */
		if((popupBottom*normal-o)*(popupTop*normal-o)<=Scalar(0))
			return event.setTargetWidget(this,wp);
		}
	
	return false;
	}

void CascadeButton::pointerButtonDown(Event& event)
	{
	/* Arm the button: */
	setArmed(true);
	
	if(isPopped)
		{
		/* Repair the event and forward it to the popup: */
		event.overrideTargetWidget(foundWidget);
		popup->pointerButtonDown(event);
		
		lastEventPos=foundPos;
		}
	}

void CascadeButton::pointerButtonUp(Event& event)
	{
	if(isPopped)
		{
		/* Repair the event and forward it to the popup: */
		event.overrideTargetWidget(foundWidget);
		popup->pointerButtonUp(event);
		
		lastEventPos=foundPos;
		}
	
	setArmed(false);
	}

void CascadeButton::pointerMotion(Event& event)
	{
	if(isPopped)
		{
		/* Repair the event and forward it to the popup: */
		event.overrideTargetWidget(foundWidget);
		popup->pointerMotion(event);
		
		lastEventPos=foundPos;
		}
	}

void CascadeButton::setPopup(Popup* newPopup)
	{
	if(isPopped)
		{
		popup->getManager()->popdownWidget(popup);
		isPopped=false;
		}
	delete popup;
	popup=newPopup;
	}

void CascadeButton::setArrowBorderSize(GLfloat newArrowBorderSize)
	{
	/* Adjust the arrow glyph: */
	arrow.setBevelSize(newArrowBorderSize);
	
	/* Set the decoration width: */
	GLfloat width=arrow.getPreferredBoxSize();
	setDecorationSize(Vector(width,width,0.0f));
	}

void CascadeButton::setArrowSize(GLfloat newArrowSize)
	{
	/* Adjust the arrow glyph: */
	arrow.setGlyphSize(newArrowSize);
	
	/* Set the decoration width: */
	GLfloat width=arrow.getPreferredBoxSize();
	setDecorationSize(Vector(width,width,0.0f));
	}

}
