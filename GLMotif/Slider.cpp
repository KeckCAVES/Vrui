/***********************************************************************
Slider - Class for horizontal or vertical sliders.
Copyright (c) 2001-2010 Oliver Kreylos

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
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLColorOperations.h>
#include <GL/GLVertexTemplates.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/Event.h>
#include <GLMotif/Container.h>

#include <GLMotif/Slider.h>

namespace GLMotif {

/***********************
Methods of class Slider:
***********************/

void Slider::positionShaft(void)
	{
	/* Position shaft according to widget size and slider orientation: */
	shaftBox.origin=getInterior().origin;
	shaftBox.doOffset(Vector(marginWidth,marginWidth,-shaftDepth));
	shaftBox.size[2]=shaftDepth;
	switch(orientation)
		{
		case HORIZONTAL:
			shaftBox.size[0]=getInterior().size[0]-marginWidth*2.0;
			shaftBox.origin[1]+=(getInterior().size[1]-marginWidth*2.0-shaftWidth)*0.5;
			shaftBox.size[1]=shaftWidth;
			break;
		
		case VERTICAL:
			shaftBox.origin[0]+=(getInterior().size[0]-marginWidth*2.0-shaftWidth)*0.5;
			shaftBox.size[0]=shaftWidth;
			shaftBox.size[1]=getInterior().size[1]-marginWidth*2.0;
			break;
		}
	}

void Slider::positionSlider(void)
	{
	/* Position slider handle according to widget size and slider orientation: */
	sliderBox.origin=shaftBox.origin;
	sliderBox.size[2]=sliderHeight+shaftDepth;
	GLfloat sliderPosition=(value-valueMin)/(valueMax-valueMin);
	switch(orientation)
		{
		case HORIZONTAL:
			sliderBox.origin[0]+=(shaftBox.size[0]-sliderLength)*sliderPosition;
			sliderBox.size[0]=sliderLength;
			sliderBox.origin[1]+=(shaftBox.size[1]-sliderWidth)*0.5;
			sliderBox.size[1]=sliderWidth;
			break;
		
		case VERTICAL:
			sliderBox.origin[0]+=(shaftBox.size[0]-sliderWidth)*0.5;
			sliderBox.size[0]=sliderWidth;
			sliderBox.origin[1]+=(shaftBox.size[1]-sliderLength)*sliderPosition;
			sliderBox.size[1]=sliderLength;
			break;
		}
	}

void Slider::clickRepeatTimerEventCallback(Misc::TimerEventScheduler::CallbackData* cbData)
	{
	/* Only react to event if still in click-repeat mode: */
	if(isClicking)
		{
		/* Adjust value and reposition slider: */
		GLfloat newValue=value+clickValueIncrement;
		if(newValue<valueMin)
			newValue=valueMin;
		else if(newValue>valueMax)
			newValue=valueMax;
		if(newValue!=value)
			{
			/* Update the slider's state: */
			value=newValue;
			positionSlider();
			
			/* Call the value changed callbacks: */
			ValueChangedCallbackData cbData(this,ValueChangedCallbackData::CLICKED,value);
			valueChangedCallbacks.call(&cbData);
			
			Misc::TimerEventScheduler* tes=getManager()->getTimerEventScheduler();
			if(tes!=0)
				{
				/* Schedule a timer event for click repeat: */
				nextClickEventTime+=0.1;
				tes->scheduleEvent(nextClickEventTime,this,&Slider::clickRepeatTimerEventCallback);
				}
			
			/* Invalidate the visual representation: */
			update();
			}
		}
	}

Slider::Slider(const char* sName,Container* sParent,Slider::Orientation sOrientation,GLfloat sSliderWidth,GLfloat sShaftLength,bool sManageChild)
	:DragWidget(sName,sParent,false),
	 orientation(sOrientation),
	 valueMin(0.0),valueMax(1000.0),valueIncrement(1.0),value(500.0),
	 isClicking(false)
	{
	/* Get the style sheet: */
	const StyleSheet* ss=getStyleSheet();
	
	/* Slider defaults to no border: */
	setBorderWidth(0.0f);
	
	/* Set the slider margin: */
	marginWidth=sSliderWidth*0.25f;
	
	/* Set the slider handle dimensions: */
	sliderWidth=sSliderWidth;
	sliderLength=sliderWidth*0.5f;
	sliderHeight=sliderWidth*0.5f;
	sliderColor=ss->sliderHandleColor;
	
	/* Set the slider shaft dimensions: */
	shaftWidth=ss->sliderShaftWidth;
	shaftLength=sShaftLength;
	shaftDepth=ss->sliderShaftDepth;
	shaftColor=ss->sliderShaftColor;
	
	/* Manage me: */
	if(sManageChild)
		manageChild();
	}

Slider::Slider(const char* sName,Container* sParent,Slider::Orientation sOrientation,GLfloat sShaftLength,bool sManageChild)
	:DragWidget(sName,sParent,false),
	 orientation(sOrientation),
	 sliderHeight(0.0f),shaftDepth(0.0f),
	 valueMin(0.0),valueMax(1000.0),valueIncrement(1.0),value(500.0),
	 isClicking(false)
	{
	/* Get the style sheet: */
	const StyleSheet* ss=getStyleSheet();
	
	/* Slider defaults to no border: */
	setBorderWidth(0.0f);
	
	/* Set the slider margin: */
	marginWidth=ss->sliderMarginWidth;
	
	/* Set the slider handle dimensions: */
	sliderWidth=ss->sliderHandleWidth;
	sliderLength=ss->sliderHandleLength;
	sliderHeight=ss->sliderHandleHeight;
	sliderColor=ss->sliderHandleColor;
	
	/* Set the slider shaft dimensions: */
	shaftWidth=ss->sliderShaftWidth;
	shaftLength=sShaftLength;
	shaftDepth=ss->sliderShaftDepth;
	shaftColor=ss->sliderShaftColor;
	
	/* Manage me: */
	if(sManageChild)
		manageChild();
	}

Slider::~Slider(void)
	{
	/* Need to remove all click-repeat timer events from the event scheduler, just in case: */
	Misc::TimerEventScheduler* tes=getManager()->getTimerEventScheduler();
	if(tes!=0)
		tes->removeAllEvents(this,&Slider::clickRepeatTimerEventCallback);
	}

Vector Slider::calcNaturalSize(void) const
	{
	/* Determine width and length of the slider and shaft: */
	GLfloat width=shaftWidth;
	if(width<sliderWidth)
		width=sliderWidth;
	width+=marginWidth;
	GLfloat length=sliderLength;
	if(length<shaftLength)
		length=shaftLength;
	length+=marginWidth;
	
	/* Return size depending on slider orientation: */
	if(orientation==HORIZONTAL)
		return calcExteriorSize(Vector(length,width,0.0f));
	else
		return calcExteriorSize(Vector(width,length,0.0f));
	}

ZRange Slider::calcZRange(void) const
	{
	/* Return parent class' z range: */
	ZRange myZRange=DragWidget::calcZRange();
	
	/* Adjust for shaft depth and slider height: */
	myZRange+=ZRange(getInterior().origin[2]-shaftDepth,getInterior().origin[2]+sliderHeight);
	
	return myZRange;
	}

void Slider::resize(const Box& newExterior)
	{
	/* Resize the parent class widget: */
	DragWidget::resize(newExterior);
	
	/* Adjust the shaft and slider handle positions: */
	positionShaft();
	positionSlider();
	}

void Slider::draw(GLContextData& contextData) const
	{
	/* Draw parent class decorations: */
	DragWidget::draw(contextData);
	
	/* Draw the shaft margin: */
	glColor(backgroundColor);
	glBegin(GL_QUAD_STRIP);
	glNormal3f(0.0f,0.0f,1.0f);
	glVertex(shaftBox.getCorner(4));
	glVertex(getInterior().getCorner(0));
	glVertex(shaftBox.getCorner(5));
	glVertex(getInterior().getCorner(1));
	glVertex(shaftBox.getCorner(7));
	glVertex(getInterior().getCorner(3));
	glVertex(shaftBox.getCorner(6));
	glVertex(getInterior().getCorner(2));
	glVertex(shaftBox.getCorner(4));
	glVertex(getInterior().getCorner(0));
	glEnd();
	
	/* Draw the shaft: */
	glColor(shaftColor);
	glBegin(GL_QUADS);
	glNormal3f(0.0f,1.0f,0.0f);
	glVertex(shaftBox.getCorner(4));
	glVertex(shaftBox.getCorner(5));
	glVertex(shaftBox.getCorner(1));
	glVertex(shaftBox.getCorner(0));
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
	glVertex(shaftBox.getCorner(1));
	glVertex(shaftBox.getCorner(5));
	glVertex(shaftBox.getCorner(7));
	glVertex(shaftBox.getCorner(3));
	glNormal3f(0.0f,0.0f,1.0f);
	glVertex(shaftBox.getCorner(0));
	glVertex(shaftBox.getCorner(1));
	glVertex(shaftBox.getCorner(3));
	glVertex(shaftBox.getCorner(2));
	glEnd();
	
	/* Draw the slider handle: */
	glColor(sliderColor);
	switch(orientation)
		{
		case HORIZONTAL:
			{
			GLfloat x1=sliderBox.origin[0];
			glBegin(GL_QUAD_STRIP);
			glNormal3f(-1.0f,0.0f,0.0f);
			glVertex3f(x1,shaftBox.origin[1]+shaftBox.size[1],sliderBox.origin[2]);
			glVertex3f(x1,shaftBox.origin[1],sliderBox.origin[2]);
			glVertex3f(x1,shaftBox.origin[1]+shaftBox.size[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x1,shaftBox.origin[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x1,sliderBox.origin[1]+sliderBox.size[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x1,sliderBox.origin[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x1,sliderBox.origin[1]+sliderBox.size[1]*0.75,sliderBox.origin[2]+sliderBox.size[2]);
			glVertex3f(x1,sliderBox.origin[1]+sliderBox.size[1]*0.25,sliderBox.origin[2]+sliderBox.size[2]);
			glEnd();
			GLfloat x2=sliderBox.origin[0]+sliderBox.size[0];
			glBegin(GL_QUAD_STRIP);
			glNormal3f(1.0f,0.0f,0.0f);
			glVertex3f(x2,shaftBox.origin[1],sliderBox.origin[2]);
			glVertex3f(x2,shaftBox.origin[1]+shaftBox.size[1],sliderBox.origin[2]);
			glVertex3f(x2,shaftBox.origin[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x2,shaftBox.origin[1]+shaftBox.size[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x2,sliderBox.origin[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x2,sliderBox.origin[1]+sliderBox.size[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x2,sliderBox.origin[1]+sliderBox.size[1]*0.25,sliderBox.origin[2]+sliderBox.size[2]);
			glVertex3f(x2,sliderBox.origin[1]+sliderBox.size[1]*0.75,sliderBox.origin[2]+sliderBox.size[2]);
			glEnd();
			glBegin(GL_QUADS);
			glNormal3f(0.0f,0.0f,-1.0f);
			glVertex3f(x1,shaftBox.origin[1],sliderBox.origin[2]);
			glVertex3f(x1,shaftBox.origin[1]+shaftBox.size[1],sliderBox.origin[2]);
			glVertex3f(x2,shaftBox.origin[1]+shaftBox.size[1],sliderBox.origin[2]);
			glVertex3f(x2,shaftBox.origin[1],sliderBox.origin[2]);
			glNormal3f(0.0f,1.0f,0.0f);
			glVertex3f(x1,shaftBox.origin[1]+shaftBox.size[1],sliderBox.origin[2]);
			glVertex3f(x1,shaftBox.origin[1]+shaftBox.size[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x2,shaftBox.origin[1]+shaftBox.size[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x2,shaftBox.origin[1]+shaftBox.size[1],sliderBox.origin[2]);
			glNormal3f(0.0f,0.0f,-1.0f);
			glVertex3f(x1,shaftBox.origin[1]+shaftBox.size[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x1,sliderBox.origin[1]+sliderBox.size[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x2,sliderBox.origin[1]+sliderBox.size[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x2,shaftBox.origin[1]+shaftBox.size[1],sliderBox.origin[2]+shaftDepth);
			glNormal3f(0.0f,1.0f,0.25f);
			glVertex3f(x1,sliderBox.origin[1]+sliderBox.size[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x1,sliderBox.origin[1]+sliderBox.size[1]*0.75,sliderBox.origin[2]+sliderBox.size[2]);
			glVertex3f(x2,sliderBox.origin[1]+sliderBox.size[1]*0.75,sliderBox.origin[2]+sliderBox.size[2]);
			glVertex3f(x2,sliderBox.origin[1]+sliderBox.size[1],sliderBox.origin[2]+shaftDepth);
			glNormal3f(0.0f,0.0f,1.0f);
			glVertex3f(x1,sliderBox.origin[1]+sliderBox.size[1]*0.75,sliderBox.origin[2]+sliderBox.size[2]);
			glVertex3f(x1,sliderBox.origin[1]+sliderBox.size[1]*0.25,sliderBox.origin[2]+sliderBox.size[2]);
			glVertex3f(x2,sliderBox.origin[1]+sliderBox.size[1]*0.25,sliderBox.origin[2]+sliderBox.size[2]);
			glVertex3f(x2,sliderBox.origin[1]+sliderBox.size[1]*0.75,sliderBox.origin[2]+sliderBox.size[2]);
			glNormal3f(0.0f,-1.0f,0.25f);
			glVertex3f(x1,sliderBox.origin[1]+sliderBox.size[1]*0.25,sliderBox.origin[2]+sliderBox.size[2]);
			glVertex3f(x1,sliderBox.origin[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x2,sliderBox.origin[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x2,sliderBox.origin[1]+sliderBox.size[1]*0.25,sliderBox.origin[2]+sliderBox.size[2]);
			glNormal3f(0.0f,0.0f,-1.0f);
			glVertex3f(x1,sliderBox.origin[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x1,shaftBox.origin[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x2,shaftBox.origin[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x2,sliderBox.origin[1],sliderBox.origin[2]+shaftDepth);
			glNormal3f(0.0f,-1.0f,0.0f);
			glVertex3f(x1,shaftBox.origin[1],sliderBox.origin[2]+shaftDepth);
			glVertex3f(x1,shaftBox.origin[1],sliderBox.origin[2]);
			glVertex3f(x2,shaftBox.origin[1],sliderBox.origin[2]);
			glVertex3f(x2,shaftBox.origin[1],sliderBox.origin[2]+shaftDepth);
			glEnd();
			break;
			}
		
		case VERTICAL:
			{
			GLfloat y1=sliderBox.origin[1];
			glBegin(GL_QUAD_STRIP);
			glNormal3f(0.0f,-1.0f,0.0f);
			glVertex3f(shaftBox.origin[0],y1,sliderBox.origin[2]);
			glVertex3f(shaftBox.origin[0]+shaftBox.size[0],y1,sliderBox.origin[2]);
			glVertex3f(shaftBox.origin[0],y1,sliderBox.origin[2]+shaftDepth);
			glVertex3f(shaftBox.origin[0]+shaftBox.size[0],y1,sliderBox.origin[2]+shaftDepth);
			glVertex3f(sliderBox.origin[0],y1,sliderBox.origin[2]+shaftDepth);
			glVertex3f(sliderBox.origin[0]+sliderBox.size[0],y1,sliderBox.origin[2]+shaftDepth);
			glVertex3f(sliderBox.origin[0]+sliderBox.size[0]*0.25,y1,sliderBox.origin[2]+sliderBox.size[2]);
			glVertex3f(sliderBox.origin[0]+sliderBox.size[0]*0.75,y1,sliderBox.origin[2]+sliderBox.size[2]);
			glEnd();
			GLfloat y2=sliderBox.origin[1]+sliderBox.size[1];
			glBegin(GL_QUAD_STRIP);
			glNormal3f(0.0f,1.0f,0.0f);
			glVertex3f(shaftBox.origin[0]+shaftBox.size[0],y2,sliderBox.origin[2]);
			glVertex3f(shaftBox.origin[0],y2,sliderBox.origin[2]);
			glVertex3f(shaftBox.origin[0]+shaftBox.size[0],y2,sliderBox.origin[2]+shaftDepth);
			glVertex3f(shaftBox.origin[0],y2,sliderBox.origin[2]+shaftDepth);
			glVertex3f(sliderBox.origin[0]+sliderBox.size[0],y2,sliderBox.origin[2]+shaftDepth);
			glVertex3f(sliderBox.origin[0],y2,sliderBox.origin[2]+shaftDepth);
			glVertex3f(sliderBox.origin[0]+sliderBox.size[0]*0.75,y2,sliderBox.origin[2]+sliderBox.size[2]);
			glVertex3f(sliderBox.origin[0]+sliderBox.size[0]*0.25,y2,sliderBox.origin[2]+sliderBox.size[2]);
			glEnd();
			glBegin(GL_QUADS);
			glNormal3f(0.0f,0.0f,-1.0f);
			glVertex3f(shaftBox.origin[0],y1,sliderBox.origin[2]);
			glVertex3f(shaftBox.origin[0],y2,sliderBox.origin[2]);
			glVertex3f(shaftBox.origin[0]+shaftBox.size[0],y2,sliderBox.origin[2]);
			glVertex3f(shaftBox.origin[0]+shaftBox.size[0],y1,sliderBox.origin[2]);
			glNormal3f(1.0f,0.0f,0.0f);
			glVertex3f(shaftBox.origin[0]+shaftBox.size[0],y1,sliderBox.origin[2]);
			glVertex3f(shaftBox.origin[0]+shaftBox.size[0],y2,sliderBox.origin[2]);
			glVertex3f(shaftBox.origin[0]+shaftBox.size[0],y2,sliderBox.origin[2]+shaftDepth);
			glVertex3f(shaftBox.origin[0]+shaftBox.size[0],y1,sliderBox.origin[2]+shaftDepth);
			glNormal3f(0.0f,0.0f,-1.0f);
			glVertex3f(shaftBox.origin[0]+shaftBox.size[0],y1,sliderBox.origin[2]+shaftDepth);
			glVertex3f(shaftBox.origin[0]+shaftBox.size[0],y2,sliderBox.origin[2]+shaftDepth);
			glVertex3f(sliderBox.origin[0]+sliderBox.size[0],y2,sliderBox.origin[2]+shaftDepth);
			glVertex3f(sliderBox.origin[0]+sliderBox.size[0],y1,sliderBox.origin[2]+shaftDepth);
			glNormal3f(1.0f,0.0f,0.25f);
			glVertex3f(sliderBox.origin[0]+sliderBox.size[0],y1,sliderBox.origin[2]+shaftDepth);
			glVertex3f(sliderBox.origin[0]+sliderBox.size[0],y2,sliderBox.origin[2]+shaftDepth);
			glVertex3f(sliderBox.origin[0]+sliderBox.size[0]*0.75,y2,sliderBox.origin[2]+sliderBox.size[2]);
			glVertex3f(sliderBox.origin[0]+sliderBox.size[0]*0.75,y1,sliderBox.origin[2]+sliderBox.size[2]);
			glNormal3f(0.0f,0.0f,1.0f);
			glVertex3f(sliderBox.origin[0]+sliderBox.size[0]*0.75,y1,sliderBox.origin[2]+sliderBox.size[2]);
			glVertex3f(sliderBox.origin[0]+sliderBox.size[0]*0.75,y2,sliderBox.origin[2]+sliderBox.size[2]);
			glVertex3f(sliderBox.origin[0]+sliderBox.size[0]*0.25,y2,sliderBox.origin[2]+sliderBox.size[2]);
			glVertex3f(sliderBox.origin[0]+sliderBox.size[0]*0.25,y1,sliderBox.origin[2]+sliderBox.size[2]);
			glNormal3f(-1.0f,0.0f,0.25f);
			glVertex3f(sliderBox.origin[0]+sliderBox.size[0]*0.25,y1,sliderBox.origin[2]+sliderBox.size[2]);
			glVertex3f(sliderBox.origin[0]+sliderBox.size[0]*0.25,y2,sliderBox.origin[2]+sliderBox.size[2]);
			glVertex3f(sliderBox.origin[0],y2,sliderBox.origin[2]+shaftDepth);
			glVertex3f(sliderBox.origin[0],y1,sliderBox.origin[2]+shaftDepth);
			glNormal3f(0.0f,0.0f,-1.0f);
			glVertex3f(sliderBox.origin[0],y1,sliderBox.origin[2]+shaftDepth);
			glVertex3f(sliderBox.origin[0],y2,sliderBox.origin[2]+shaftDepth);
			glVertex3f(sliderBox.origin[0],y2,sliderBox.origin[2]);
			glVertex3f(sliderBox.origin[0],y1,sliderBox.origin[2]);
			glEnd();
			break;
			}
		}
	}

void Slider::pointerButtonDown(Event& event)
	{
	/* Where inside the widget did the event hit? */
	int dimension=orientation==HORIZONTAL?0:1;
	GLfloat picked=event.getWidgetPoint().getPoint()[dimension];
	if(picked<sliderBox.origin[dimension]) // Hit below slider?
		{
		/* Decrement value and reposition slider: */
		GLfloat newValue=value-valueIncrement;
		if(newValue<valueMin)
			newValue=valueMin;
		if(newValue!=value)
			{
			/* Update the slider: */
			value=newValue;
			positionSlider();
			
			/* Call the value changed callbacks: */
			ValueChangedCallbackData cbData(this,ValueChangedCallbackData::CLICKED,value);
			valueChangedCallbacks.call(&cbData);
			
			/* Schedule a timer event for click repeat: */
			isClicking=true;
			clickValueIncrement=-valueIncrement;
			Misc::TimerEventScheduler* tes=getManager()->getTimerEventScheduler();
			if(tes!=0)
				{
				nextClickEventTime=tes->getCurrentTime()+0.5;
				tes->scheduleEvent(nextClickEventTime,this,&Slider::clickRepeatTimerEventCallback);
				}
			
			/* Update the visual representation: */
			update();
			}
		}
	else if(picked>sliderBox.origin[dimension]+sliderBox.size[dimension]) // Hit above slider?
		{
		/* Increment value and reposition slider: */
		GLfloat newValue=value+valueIncrement;
		if(newValue>valueMax)
			newValue=valueMax;
		if(newValue!=value)
			{
			/* Update the slider: */
			value=newValue;
			positionSlider();
			
			/* Call the value changed callbacks: */
			ValueChangedCallbackData cbData(this,ValueChangedCallbackData::CLICKED,value);
			valueChangedCallbacks.call(&cbData);
			
			/* Schedule a timer event for click repeat: */
			isClicking=true;
			clickValueIncrement=valueIncrement;
			Misc::TimerEventScheduler* tes=getManager()->getTimerEventScheduler();
			if(tes!=0)
				{
				nextClickEventTime=tes->getCurrentTime()+0.5;
				tes->scheduleEvent(nextClickEventTime,this,&Slider::clickRepeatTimerEventCallback);
				}
			
			/* Update the visual representation: */
			update();
			}
		}
	else // Hit slider!
		{
		/* Start dragging: */
		dragOffset=sliderBox.origin[dimension]-picked;
		startDragging(event);
		}
	}

void Slider::pointerButtonUp(Event& event)
	{
	stopDragging(event);
	
	/* Cancel any pending click-repeat events: */
	Misc::TimerEventScheduler* tes=getManager()->getTimerEventScheduler();
	if(tes!=0)
		tes->removeEvent(nextClickEventTime,this,&Slider::clickRepeatTimerEventCallback);
	isClicking=false;
	}

void Slider::pointerMotion(Event& event)
	{
	if(isDragging)
		{
		/* Update the slider value and position: */
		int dimension=orientation==HORIZONTAL?0:1;
		GLfloat newSliderPosition=event.getWidgetPoint().getPoint()[dimension]+dragOffset;
		
		/* Calculate the new slider value and reposition the slider: */
		GLfloat newValue=(newSliderPosition-shaftBox.origin[dimension])*(valueMax-valueMin)/(shaftBox.size[dimension]-sliderLength)+valueMin;
		if(newValue<valueMin)
			newValue=valueMin;
		else if(newValue>valueMax)
			newValue=valueMax;
		if(valueIncrement>0.0f)
			newValue=GLfloat(floor(double(newValue)/double(valueIncrement)+0.5)*double(valueIncrement));
		if(newValue!=value)
			{
			/* Update the slider: */
			value=newValue;
			positionSlider();
			
			/* Call the value changed callbacks: */
			ValueChangedCallbackData cbData(this,ValueChangedCallbackData::DRAGGED,value);
			valueChangedCallbacks.call(&cbData);
			
			/* Update the visual representation: */
			update();
			}
		}
	}

void Slider::setMarginWidth(GLfloat newMarginWidth)
	{
	/* Set the new margin width: */
	marginWidth=newMarginWidth;
	
	if(isManaged)
		{
		/* Try adjusting the widget size to accomodate the new margin width: */
		parent->requestResize(this,calcNaturalSize());
		}
	else
		resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
	}

void Slider::setValue(GLfloat newValue)
	{
	/* Update the value and reposition the slider: */
	value=newValue;
	positionSlider();
	
	/* Update the visual representation: */
	update();
	}

void Slider::setValueRange(GLfloat newValueMin,GLfloat newValueMax,GLfloat newValueIncrement)
	{
	/* Update the value range: */
	valueMin=newValueMin;
	valueMax=newValueMax;
	valueIncrement=newValueIncrement;
	
	/* Adjust the current value and reposition the slider: */
	if(value<valueMin)
		value=valueMin;
	else if(value>valueMax)
		value=valueMax;
	if(valueIncrement>0.0f)
		value=GLfloat(floor(double(value)/double(valueIncrement)+0.5)*double(valueIncrement));
	positionSlider();
	
	/* Update the visual representation: */
	update();
	}

}
