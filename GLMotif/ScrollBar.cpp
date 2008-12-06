/***********************************************************************
ScrollBar - Class for horizontal or vertical scroll bars, to be used as
a component by scrolling widgets like list boxes.
Copyright (c) 2008 Oliver Kreylos

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

#include <math.h>
#include <Misc/Utility.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLColorOperations.h>
#include <GL/GLVertexTemplates.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/Event.h>
#include <GLMotif/WidgetManager.h>

#include <GLMotif/ScrollBar.h>

namespace GLMotif {

/**************************
Methods of class ScrollBar:
**************************/

void ScrollBar::positionButtonsAndShaft(void)
	{
	/* Position the arrow buttons and scroll bar shaft: */
	for(int i=0;i<2;++i)
		arrowBox[i]=getInterior();
	shaftBox=getInterior();
	shaftBox.origin[2]-=shaftDepth;
	shaftBox.size[2]=shaftDepth;
	
	int mainDim=orientation==HORIZONTAL?0:1;
	GLfloat width=getInterior().size[1-mainDim];
	shaftBox.origin[mainDim]+=width;
	shaftBox.size[mainDim]-=2.0f*width;
	arrowBox[1].origin[mainDim]+=arrowBox[1].size[mainDim]-width;
	for(int i=0;i<2;++i)
		{
		arrowBox[i].size[mainDim]=width;
		arrowBevelBox[i]=arrowBox[i].inset(Vector(bevelWidth,bevelWidth,0.0f));
		arrowBevelBox[i].origin[2]+=bevelWidth;
		arrows[i].setArrowSize((width-bevelWidth*2.0f)*0.167f);
		arrows[i].setArrowBevelSize((width-bevelWidth*2.0f)*0.167f);
		arrows[i].setArrowBox(arrowBevelBox[i]);
		}
	}

void ScrollBar::positionHandle(void)
	{
	handleBottomBox=shaftBox;
	if(pageSize<positionMax-positionMin)
		{
		int mainDim=orientation==HORIZONTAL?0:1;
		handleBottomBox.size[mainDim]=shaftBox.size[mainDim]*GLfloat(pageSize)/GLfloat(positionMax-positionMin);
		if(handleBottomBox.size[mainDim]<scrollBarWidth*0.5f)
			handleBottomBox.size[mainDim]=scrollBarWidth*0.5f;
		if(reverse)
			handleBottomBox.origin[mainDim]+=GLfloat(positionMax-pageSize-position)*(shaftBox.size[mainDim]-handleBottomBox.size[mainDim])/GLfloat(positionMax-pageSize-positionMin);
		else
			handleBottomBox.origin[mainDim]+=GLfloat(position-positionMin)*(shaftBox.size[mainDim]-handleBottomBox.size[mainDim])/GLfloat(positionMax-pageSize-positionMin);
		}
	
	handleBox=handleBottomBox;
	handleBox.origin[2]+=shaftDepth;
	handleBox.size[2]=0.0f;
	
	handleBevelBox=handleBox.inset(Vector(bevelWidth,bevelWidth,0.0f));
	handleBevelBox.origin[2]+=bevelWidth;
	}

void ScrollBar::drawBeveledBox(const Box& base,const Box& bevel) const
	{
	GLfloat normalComponent=bevel.origin[2]>base.origin[2]?0.707f:-0.707f;
	glBegin(GL_QUADS);
	glNormal3f(0.0f,-normalComponent,0.707f);
	glVertex(bevel.getCorner(0));
	glVertex(base.getCorner(0));
	glVertex(base.getCorner(1));
	glVertex(bevel.getCorner(1));
	glNormal3f(normalComponent,0.0f,0.707f);
	glVertex(bevel.getCorner(1));
	glVertex(base.getCorner(1));
	glVertex(base.getCorner(3));
	glVertex(bevel.getCorner(3));
	glNormal3f(0.0f,normalComponent,0.707f);
	glVertex(bevel.getCorner(3));
	glVertex(base.getCorner(3));
	glVertex(base.getCorner(2));
	glVertex(bevel.getCorner(2));
	glNormal3f(-normalComponent,0.0f,0.707f);
	glVertex(bevel.getCorner(2));
	glVertex(base.getCorner(2));
	glVertex(base.getCorner(0));
	glVertex(bevel.getCorner(0));
	glEnd();
	}

void ScrollBar::clickRepeatTimerEventCallback(Misc::TimerEventScheduler::CallbackData* cbData)
	{
	/* Only react to event if still in click-repeat mode: */
	if(isClicking)
		{
		/* Adjust position and reposition handle: */
		int newPosition=position+clickPositionIncrement;
		if(newPosition>positionMax-pageSize)
			newPosition=positionMax-pageSize;
		if(newPosition<positionMin)
			newPosition=positionMin;
		bool changed=newPosition!=position;
		position=newPosition;
		positionHandle();
		if(changed)
			{
			ValueChangedCallbackData cbData(this,clickChangeReason,position);
			valueChangedCallbacks.call(&cbData);
			
			/* Schedule a timer event for click repeat: */
			nextClickEventTime+=0.1;
			getManager()->getTimerEventScheduler()->scheduleEvent(nextClickEventTime,this,&ScrollBar::clickRepeatTimerEventCallback);
			}
		}
	}

void ScrollBar::scheduleClickRepeat(int increment,ScrollBar::ValueChangedCallbackData::ChangeReason reason,double interval)
	{
	/* Schedule a timer event for click repeat: */
	isClicking=true;
	clickPositionIncrement=increment;
	clickChangeReason=reason;
	Misc::TimerEventScheduler* tes=getManager()->getTimerEventScheduler();
	nextClickEventTime=tes->getCurrentTime()+interval;
	tes->scheduleEvent(nextClickEventTime,this,&ScrollBar::clickRepeatTimerEventCallback);
	}

ScrollBar::ScrollBar(const char* sName,Container* sParent,Orientation sOrientation,bool sReverse,bool sManageChild)
	:DragWidget(sName,sParent,false),
	 orientation(sOrientation),reverse(sReverse),
	 positionMin(0),positionMax(1000),pageSize(100),
	 position(500),
	 armedArrowIndex(-1),
	 isClicking(false)
	{
	/* Get the style sheet: */
	const StyleSheet* ss=getStyleSheet();
	
	/* Set the scroll bar dimensions: */
	scrollBarWidth=ss->size*2.0f;
	bevelWidth=ss->size*0.25f;
	shaftDepth=ss->sliderShaftDepth;
	
	/* Scroll bar defaults to no border: */
	setBorderWidth(0.0f);
	
	/* Initialize the arrow buttons: */
	switch(orientation)
		{
		case HORIZONTAL:
			arrows[0].setDirection(Arrow::LEFT);
			arrows[1].setDirection(Arrow::RIGHT);
			break;
		
		case VERTICAL:
			arrows[0].setDirection(Arrow::DOWN);
			arrows[1].setDirection(Arrow::UP);
			break;
		}
	for(int i=0;i<2;++i)
		{
		arrows[i].setStyle(Arrow::SIMPLE);
		arrows[i].setDepth(Arrow::IN);
		arrows[i].setArrowColor(backgroundColor);
		}
	
	/* Set the scroll bar component colors: */
	shaftColor=ss->sliderShaftColor;
	handleColor=ss->sliderHandleColor;
	
	/* Manage me: */
	if(sManageChild)
		manageChild();
	}

ScrollBar::~ScrollBar(void)
	{
	/* Need to remove all click-repeat timer events from the event scheduler, just in case: */
	getManager()->getTimerEventScheduler()->removeAllEvents(this,&ScrollBar::clickRepeatTimerEventCallback);
	}

Vector ScrollBar::calcNaturalSize(void) const
	{
	/* Return size depending on scroll bar orientation: */
	if(orientation==HORIZONTAL)
		return calcExteriorSize(Vector(scrollBarWidth*3.0f,scrollBarWidth,0.0f));
	else
		return calcExteriorSize(Vector(scrollBarWidth,scrollBarWidth*3.0f,0.0f));
	}

ZRange ScrollBar::calcZRange(void) const
	{
	/* Return parent class' z range: */
	ZRange myZRange=DragWidget::calcZRange();
	
	/* Adjust for shaft depth and button bevels: */
	myZRange+=ZRange(getInterior().origin[2]-Misc::max(shaftDepth,bevelWidth),getInterior().origin[2]+bevelWidth);
	
	return myZRange;
	}

void ScrollBar::resize(const Box& newExterior)
	{
	/* Resize the parent class widget: */
	DragWidget::resize(newExterior);
	
	/* Adjust the shaft and scroll bar handle positions: */
	positionButtonsAndShaft();
	positionHandle();
	}

void ScrollBar::setBackgroundColor(const Color& newBackgroundColor)
	{
	/* Call the base class method: */
	DragWidget::setBackgroundColor(newBackgroundColor);
	
	/* Let the arrow glyphs track the background color: */
	for(int i=0;i<2;++i)
		arrows[i].setArrowColor(newBackgroundColor);
	}

void ScrollBar::draw(GLContextData& contextData) const
	{
	/* Draw parent class decorations: */
	DragWidget::draw(contextData);
	
	/* Draw filler triangles to merge scroll bar with widget border: */
	glColor(backgroundColor);
	switch(orientation)
		{
		case HORIZONTAL:
			glBegin(GL_TRIANGLE_FAN);
			glNormal3f(0.0f,0.0f,1.0f);
			glVertex(getInterior().getCorner(1));
			glVertex(arrowBox[1].getCorner(1));
			glVertex(shaftBox.getCorner(5));
			glVertex(shaftBox.getCorner(4));
			glVertex(arrowBox[0].getCorner(0));
			glVertex(getInterior().getCorner(0));
			glEnd();
			
			glBegin(GL_TRIANGLE_FAN);
			glNormal3f(0.0f,0.0f,1.0f);
			glVertex(getInterior().getCorner(2));
			glVertex(arrowBox[0].getCorner(2));
			glVertex(shaftBox.getCorner(6));
			glVertex(shaftBox.getCorner(7));
			glVertex(arrowBox[1].getCorner(3));
			glVertex(getInterior().getCorner(3));
			glEnd();
			break;
		
		case VERTICAL:
			glBegin(GL_TRIANGLE_FAN);
			glNormal3f(0.0f,0.0f,1.0f);
			glVertex(getInterior().getCorner(0));
			glVertex(arrowBox[0].getCorner(0));
			glVertex(shaftBox.getCorner(4));
			glVertex(shaftBox.getCorner(6));
			glVertex(arrowBox[1].getCorner(2));
			glVertex(getInterior().getCorner(2));
			glEnd();
			
			glBegin(GL_TRIANGLE_FAN);
			glNormal3f(0.0f,0.0f,1.0f);
			glVertex(getInterior().getCorner(3));
			glVertex(arrowBox[1].getCorner(3));
			glVertex(shaftBox.getCorner(7));
			glVertex(shaftBox.getCorner(5));
			glVertex(arrowBox[0].getCorner(1));
			glVertex(getInterior().getCorner(1));
			glEnd();
			break;
		}
	
	/* Draw the scroll bar shaft: */
	glColor(shaftColor);
	glBegin(GL_QUADS);
	glNormal3f(0.0f,0.0f,1.0f);
	glVertex(shaftBox.getCorner(0));
	glVertex(shaftBox.getCorner(1));
	glVertex(shaftBox.getCorner(3));
	glVertex(shaftBox.getCorner(2));
	glNormal3f(0.0f,1.0f,0.0f);
	glVertex(shaftBox.getCorner(1));
	glVertex(shaftBox.getCorner(0));
	glVertex(shaftBox.getCorner(4));
	glVertex(shaftBox.getCorner(5));
	glNormal3f(0.0f,-1.0f,0.0f);
	glVertex(shaftBox.getCorner(2));
	glVertex(shaftBox.getCorner(3));
	glVertex(shaftBox.getCorner(7));
	glVertex(shaftBox.getCorner(6));
	glNormal3f(1.0f,0.0f,0.0f);
	glVertex(shaftBox.getCorner(0));
	glVertex(shaftBox.getCorner(2));
	glVertex(shaftBox.getCorner(6));
	glVertex(shaftBox.getCorner(4));
	glNormal3f(-1.0f,0.0f,0.0f);
	glVertex(shaftBox.getCorner(3));
	glVertex(shaftBox.getCorner(1));
	glVertex(shaftBox.getCorner(5));
	glVertex(shaftBox.getCorner(7));
	glEnd();
	
	/* Draw the handle bottom: */
	glColor(handleColor);
	switch(orientation)
		{
		case HORIZONTAL:
			glBegin(GL_QUADS);
			glNormal3f(-1.0f,0.0f,0.0f);
			glVertex(handleBottomBox.getCorner(2));
			glVertex(handleBottomBox.getCorner(0));
			glVertex(handleBottomBox.getCorner(4));
			glVertex(handleBottomBox.getCorner(6));
			glNormal3f(1.0f,0.0f,0.0f);
			glVertex(handleBottomBox.getCorner(1));
			glVertex(handleBottomBox.getCorner(3));
			glVertex(handleBottomBox.getCorner(7));
			glVertex(handleBottomBox.getCorner(5));
			glEnd();
			break;
		
		case VERTICAL:
			glBegin(GL_QUADS);
			glNormal3f(0.0f,-1.0f,0.0f);
			glVertex(handleBottomBox.getCorner(0));
			glVertex(handleBottomBox.getCorner(1));
			glVertex(handleBottomBox.getCorner(5));
			glVertex(handleBottomBox.getCorner(4));
			glNormal3f(0.0f,1.0f,0.0f);
			glVertex(handleBottomBox.getCorner(3));
			glVertex(handleBottomBox.getCorner(2));
			glVertex(handleBottomBox.getCorner(6));
			glVertex(handleBottomBox.getCorner(7));
			glEnd();
			break;
		}
	
	/* Draw the handle top: */
	drawBeveledBox(handleBox,handleBevelBox);
	glBegin(GL_QUADS);
	glNormal3f(0.0f,0.0f,1.0f);
	glVertex(handleBevelBox.getCorner(0));
	glVertex(handleBevelBox.getCorner(1));
	glVertex(handleBevelBox.getCorner(3));
	glVertex(handleBevelBox.getCorner(2));
	glEnd();
	
	/* Draw the arrow buttons: */
	for(int i=0;i<2;++i)
		{
		glColor(backgroundColor);
		drawBeveledBox(arrowBox[i],arrowBevelBox[i]);
		arrows[i].draw(contextData);
		}
	}

void ScrollBar::pointerButtonDown(Event& event)
	{
	/* Where inside the widget did the event hit? */
	int mainDim=orientation==HORIZONTAL?0:1;
	GLfloat picked=event.getWidgetPoint().getPoint()[mainDim];
	if(picked<shaftBox.origin[mainDim]) // Decrement button selected
		{
		/* Arm the decrement button: */
		armedArrowIndex=0;
		arrowBevelBox[0]=arrowBox[0].inset(Vector(bevelWidth,bevelWidth,0.0f));
		arrowBevelBox[0].origin[2]-=bevelWidth;
		arrows[0].setArrowBox(arrowBevelBox[0]);
		
		/* Update the scroll bar position: */
		int increment=reverse?1:-1;
		int newPosition=position+increment;
		if(newPosition>positionMax-pageSize)
			newPosition=positionMax-pageSize;
		if(newPosition<positionMin)
			newPosition=positionMin;
		if(position!=newPosition)
			{
			position=newPosition;
			positionHandle();
			ValueChangedCallbackData cbData(this,reverse?ValueChangedCallbackData::ITEM_UP:ValueChangedCallbackData::ITEM_DOWN,position);
			valueChangedCallbacks.call(&cbData);
			
			/* Schedule a timer event for click repeat: */
			scheduleClickRepeat(increment,cbData.reason,0.5);
			}
		}
	else if(picked<handleBox.origin[mainDim]) // Page down area selected
		{
		/* Update the scroll bar position: */
		int increment=reverse?pageSize:-pageSize;
		int newPosition=position+increment;
		if(newPosition>positionMax-pageSize)
			newPosition=positionMax-pageSize;
		if(newPosition<positionMin)
			newPosition=positionMin;
		if(position!=newPosition)
			{
			position=newPosition;
			positionHandle();
			ValueChangedCallbackData cbData(this,reverse?ValueChangedCallbackData::PAGE_UP:ValueChangedCallbackData::PAGE_DOWN,position);
			valueChangedCallbacks.call(&cbData);
			
			/* Schedule a timer event for click repeat: */
			scheduleClickRepeat(increment,cbData.reason,0.5);
			}
		}
	else if(picked<handleBox.origin[mainDim]+handleBox.size[mainDim]) // Scroll bar handle selected
		{
		/* Start dragging: */
		dragOffset=handleBox.origin[mainDim]-picked;
		startDragging(event);
		}
	else if(picked<arrowBox[1].origin[mainDim]) // Page up area selected
		{
		/* Update the scroll bar position: */
		int increment=reverse?-pageSize:pageSize;
		int newPosition=position+increment;
		if(newPosition>positionMax-pageSize)
			newPosition=positionMax-pageSize;
		if(newPosition<positionMin)
			newPosition=positionMin;
		if(position!=newPosition)
			{
			position=newPosition;
			positionHandle();
			ValueChangedCallbackData cbData(this,reverse?ValueChangedCallbackData::PAGE_DOWN:ValueChangedCallbackData::PAGE_UP,position);
			valueChangedCallbacks.call(&cbData);
			
			/* Schedule a timer event for click repeat: */
			scheduleClickRepeat(increment,cbData.reason,0.5);
			}
		}
	else // Increment button selected
		{
		/* Arm the increment button: */
		armedArrowIndex=1;
		arrowBevelBox[1]=arrowBox[1].inset(Vector(bevelWidth,bevelWidth,0.0f));
		arrowBevelBox[1].origin[2]-=bevelWidth;
		arrows[1].setArrowBox(arrowBevelBox[1]);
		
		/* Update the scroll bar position: */
		int increment=reverse?-1:1;
		int newPosition=position+increment;
		if(newPosition>positionMax-pageSize)
			newPosition=positionMax-pageSize;
		if(newPosition<positionMin)
			newPosition=positionMin;
		if(position!=newPosition)
			{
			position=newPosition;
			positionHandle();
			ValueChangedCallbackData cbData(this,reverse?ValueChangedCallbackData::ITEM_DOWN:ValueChangedCallbackData::ITEM_UP,position);
			valueChangedCallbacks.call(&cbData);
			
			/* Schedule a timer event for click repeat: */
			scheduleClickRepeat(increment,cbData.reason,0.5);
			}
		}
	}

void ScrollBar::pointerButtonUp(Event& event)
	{
	stopDragging(event);
	
	/* Cancel any pending click-repeat events: */
	Misc::TimerEventScheduler* tes=getManager()->getTimerEventScheduler();
	tes->removeEvent(nextClickEventTime,this,&ScrollBar::clickRepeatTimerEventCallback);
	isClicking=false;
	
	/* Unarm the armed arrow button: */
	if(armedArrowIndex>=0)
		{
		arrowBevelBox[armedArrowIndex]=arrowBox[armedArrowIndex].inset(Vector(bevelWidth,bevelWidth,0.0f));
		arrowBevelBox[armedArrowIndex].origin[2]+=bevelWidth;
		arrows[armedArrowIndex].setArrowBox(arrowBevelBox[armedArrowIndex]);
		armedArrowIndex=-1;
		}
	}

void ScrollBar::pointerMotion(Event& event)
	{
	if(isDragging)
		{
		int mainDim=orientation==HORIZONTAL?0:1;
		
		/* Update the handle position: */
		GLfloat newHandlePosition=event.getWidgetPoint().getPoint()[mainDim]+dragOffset;
		
		/* Calculate the new scroll bar position: */
		int newPosition;
		if(reverse)
			newPosition=int(floor((shaftBox.origin[mainDim]+shaftBox.size[mainDim]-handleBox.size[mainDim]-newHandlePosition)*GLfloat(positionMax-pageSize-positionMin)/(shaftBox.size[mainDim]-handleBox.size[mainDim])+GLfloat(positionMin)+0.5f));
		else
			newPosition=int(floor((newHandlePosition-shaftBox.origin[mainDim])*GLfloat(positionMax-pageSize-positionMin)/(shaftBox.size[mainDim]-handleBox.size[mainDim])+GLfloat(positionMin)+0.5f));
		if(newPosition>positionMax-pageSize)
			newPosition=positionMax-pageSize;
		if(newPosition<positionMin)
			newPosition=positionMin;
		if(newPosition!=position)
			{
			position=newPosition;
			positionHandle();
			ValueChangedCallbackData cbData(this,ValueChangedCallbackData::DRAGGED,position);
			valueChangedCallbacks.call(&cbData);
			}
		}
	}

void ScrollBar::setBevelWidth(GLfloat newBevelWidth)
	{
	/* Set the new bevel width: */
	bevelWidth=newBevelWidth;
	
	/* Update the positions of the scroll bar components: */
	positionButtonsAndShaft();
	positionHandle();
	}

void ScrollBar::setPosition(int newPosition)
	{
	/* Update the position and reposition the scroll bar handle: */
	position=newPosition;
	positionHandle();
	}

void ScrollBar::setPositionRange(int newPositionMin,int newPositionMax,int newPageSize)
	{
	/* Update the position range: */
	positionMin=newPositionMin;
	positionMax=newPositionMax;
	pageSize=newPageSize;
	
	/* Adjust the current position and reposition the scroll bar handle: */
	if(position<positionMin)
		position=positionMin;
	else if(position>positionMax-pageSize)
		position=positionMax-pageSize;
	positionHandle();
	}

}
