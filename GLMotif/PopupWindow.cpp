/***********************************************************************
PopupWindow - Class for main windows with a draggable title bar and an
optional close button.
Copyright (c) 2001-2005 Oliver Kreylos

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
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLFont.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/Event.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/TitleBar.h>

#include <GLMotif/PopupWindow.h>

namespace GLMotif {

/****************************
Methods of class PopupWindow:
****************************/

PopupWindow::PopupWindow(const char* sName,WidgetManager* sManager,const char* sTitleString,const GLFont* font)
	:Container(sName,0,false),manager(sManager),
	 titleBar(0),
	 resizableMask(0x3),
	 childBorderWidth(0.0f),
	 child(0),
	 isResizing(false)
	{
	/* Get the style sheet: */
	const StyleSheet* ss=manager->getStyleSheet();
	
	/* Create the title bar widget: */
	titleBar=new TitleBar("TitleBar",this,sTitleString,font,false);
	
	/* Set the popup window's default layout: */
	setBorderWidth(0.0f);
	setBorderType(Widget::PLAIN);
	setBorderColor(ss->borderColor);
	setBackgroundColor(ss->bgColor);
	setForegroundColor(ss->fgColor);
	childBorderWidth=ss->popupWindowChildBorderWidth;
	
	titleBar->manageChild();
	}

PopupWindow::PopupWindow(const char* sName,WidgetManager* sManager,const char* sTitleString)
	:Container(sName,0,false),manager(sManager),
	 titleBar(0),
	 resizableMask(0x3),
	 childBorderWidth(0.0f),
	 child(0),
	 isResizing(false)
	{
	/* Get the style sheet: */
	const StyleSheet* ss=manager->getStyleSheet();
	
	/* Create the title bar widget: */
	titleBar=new TitleBar("TitleBar",this,sTitleString,false);
	
	/* Set the popup window's default layout: */
	setBorderWidth(0.0f);
	setBorderType(Widget::PLAIN);
	setBorderColor(ss->borderColor);
	setBackgroundColor(ss->bgColor);
	setForegroundColor(ss->fgColor);
	childBorderWidth=ss->popupWindowChildBorderWidth;
	
	titleBar->manageChild();
	}

PopupWindow::~PopupWindow(void)
	{
	manager->popdownWidget(this);
	
	delete titleBar;
	delete child;
	}

Vector PopupWindow::calcNaturalSize(void) const
	{
	/* Calculate the title bar size: */
	Vector result=titleBar->calcNaturalSize();
	
	/* Calculate the child's size: */
	if(child!=0)
		{
		Vector childSize=child->calcNaturalSize();
		childSize[0]+=2.0f*childBorderWidth;
		childSize[1]+=2.0f*childBorderWidth;
		
		/* Combine the title bar and child sizes: */
		if(result[0]<childSize[0])
			result[0]=childSize[0];
		result[1]+=childSize[1];
		}
	
	return calcExteriorSize(result);
	}

ZRange PopupWindow::calcZRange(void) const
	{
	/* Calculate the parent class widget's z range: */
	ZRange myZRange=Container::calcZRange();
	
	/* Calculate the title bar's z range: */
	myZRange+=titleBar->calcZRange();
	
	/* Calculate the child widget's z range: */
	if(child!=0)
		myZRange+=child->calcZRange();
	
	/* Adjust the minimum z value to accomodate the popup window's back side: */
	myZRange.first-=childBorderWidth;
	
	return myZRange;
	}

void PopupWindow::resize(const Box& newExterior)
	{
	/* Resize the parent class widget: */
	Container::resize(newExterior);
	
	/* Resize the title bar: */
	Box titleBarRect=getInterior();
	GLfloat titleBarHeight=titleBar->calcNaturalSize()[1];
	titleBarRect.origin[1]+=titleBarRect.size[1]-titleBarHeight;
	titleBarRect.size[1]=titleBarHeight;
	titleBar->resize(titleBarRect);
	
	/* Resize the child: */
	if(child!=0)
		{
		Box childRect=getInterior();
		childRect.origin[0]+=childBorderWidth;
		childRect.size[0]-=2.0f*childBorderWidth;
		childRect.origin[1]+=childBorderWidth;
		childRect.size[1]-=2.0f*childBorderWidth+titleBarHeight;
		child->resize(childRect);
		}
	
	/* Resize the parent class widget again to calculate the correct z range: */
	Container::resize(newExterior);
	}

Vector PopupWindow::calcHotSpot(void) const
	{
	return titleBar->calcHotSpot();
	}

void PopupWindow::draw(GLContextData& contextData) const
	{
	/* Draw the popup window's back side: */
	Box back=getExterior().offset(Vector(0.0,0.0,getZRange().first));
	glColor(borderColor);
	glBegin(GL_QUADS);
	glNormal3f(0.0f,0.0f,-1.0f);
	glVertex(back.getCorner(0));
	glVertex(back.getCorner(2));
	glVertex(back.getCorner(3));
	glVertex(back.getCorner(1));
	glNormal3f(0.0f,-1.0f,0.0f);
	glVertex(back.getCorner(0));
	glVertex(back.getCorner(1));
	glVertex(getExterior().getCorner(1));
	glVertex(getExterior().getCorner(0));
	glNormal3f(0.0f,1.0f,0.0f);
	glVertex(back.getCorner(3));
	glVertex(back.getCorner(2));
	glVertex(getExterior().getCorner(2));
	glVertex(getExterior().getCorner(3));
	glEnd();
	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(-1.0f,0.0f,0.0f);
	glVertex(titleBar->getExterior().getCorner(0));
	glVertex(getExterior().getCorner(2));
	glVertex(back.getCorner(2));
	glVertex(back.getCorner(0));
	glVertex(getExterior().getCorner(0));
	glEnd();
	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(1.0f,0.0f,0.0f);
	glVertex(titleBar->getExterior().getCorner(1));
	glVertex(getExterior().getCorner(1));
	glVertex(back.getCorner(1));
	glVertex(back.getCorner(3));
	glVertex(getExterior().getCorner(3));
	glEnd();
	
	/* Draw the title bar: */
	titleBar->draw(contextData);
	
	/* Draw the child border: */
	Box childBox=getInterior();
	childBox.size[1]-=titleBar->getExterior().size[1];
	childBox.doInset(Vector(childBorderWidth,childBorderWidth,0.0f));
	glColor(backgroundColor);
	glBegin(GL_QUAD_STRIP);
	glNormal3f(0.0f,0.0f,1.0f);
	glVertex(childBox.getCorner(0));
	glVertex(getExterior().getCorner(0));
	glVertex(childBox.getCorner(1));
	glVertex(getExterior().getCorner(1));
	glVertex(childBox.getCorner(3));
	glVertex(getExterior().getCorner(3));
	glVertex(childBox.getCorner(2));
	glVertex(getExterior().getCorner(2));
	glVertex(childBox.getCorner(0));
	glVertex(getExterior().getCorner(0));
	glEnd();
	
	/* Draw the child: */
	if(child!=0)
		child->draw(contextData);
	}

bool PopupWindow::findRecipient(Event& event)
	{
	/* Take all events while resizing: */
	if(isResizing)
		return event.setTargetWidget(this,event.calcWidgetPoint(this));
	
	/* Check the title bar first: */
	if(titleBar->findRecipient(event))
		return true;
	
	/* Check the child next: */
	if(child!=0&&child->findRecipient(event))
		return true;
	
	/* Check ourselves: */
	Event::WidgetPoint wp=event.calcWidgetPoint(this);
	if(isInside(wp.getPoint()))
		return event.setTargetWidget(this,wp);
	else
		return false;
	}

void PopupWindow::pointerButtonDown(Event& event)
	{
	/* Check which part of the resizing border was clicked: */
	const Point& p=event.getWidgetPoint().getPoint();
	GLfloat left=getExterior().origin[0];
	GLfloat right=left+getExterior().size[0];
	GLfloat bottom=getExterior().origin[1];
	// GLfloat top=bottom+getExterior().size[1];
	GLfloat cornerSize=childBorderWidth*5.0f;
	resizeBorderMask=0x0;
	if(resizableMask&0x1)
		{
		if(p[0]<=left+cornerSize)
			{
			resizeBorderMask|=0x1;
			resizeOffset[0]=left-p[0];
			}
		else if(p[0]>=right-cornerSize)
			{
			resizeBorderMask|=0x2;
			resizeOffset[0]=right-p[0];
			}
		}
	if(resizableMask&0x2)
		{
		if(p[1]<=bottom+cornerSize)
			{
			resizeBorderMask|=0x4;
			resizeOffset[1]=bottom-p[1];
			}
		#if 0 // Can't -- or shouldn't -- resize by pulling on top
		else if(p[1]>=top-cornerSize)
			{
			resizeBorderMask|=0x8;
			resizeOffset[1]=top-p[1];
			}
		#endif
		}
	isResizing=true;
	}

void PopupWindow::pointerButtonUp(Event& event)
	{
	/* Stop resizing: */
	isResizing=false;
	}

void PopupWindow::pointerMotion(Event& event)
	{
	if(isResizing)
		{
		const Point& p=event.getWidgetPoint().getPoint();
		
		/* Get the current exterior box: */
		Box exterior=getExterior();
		
		/* Get the minimal exterior size: */
		Vector minSize=calcNaturalSize();
		
		/* Resize the box horizontally: */
		if(resizeBorderMask&0x1)
			{
			GLfloat right=exterior.origin[0]+exterior.size[0];
			GLfloat newLeft=p[0]+resizeOffset[0];
			GLfloat newSize=right-newLeft;
			if(newSize<minSize[0])
				newSize=minSize[0];
			exterior.origin[0]=right-newSize;
			exterior.size[0]=newSize;
			}
		else if(resizeBorderMask&0x2)
			{
			GLfloat left=exterior.origin[0];
			GLfloat newRight=p[0]+resizeOffset[0];
			GLfloat newSize=newRight-left;
			if(newSize<minSize[0])
				newSize=minSize[0];
			exterior.size[0]=newSize;
			}
		
		/* Resize the box vertically: */
		if(resizeBorderMask&0x4)
			{
			GLfloat top=exterior.origin[1]+exterior.size[1];
			GLfloat newBottom=p[1]+resizeOffset[1];
			GLfloat newSize=top-newBottom;
			if(newSize<minSize[1])
				newSize=minSize[1];
			exterior.origin[1]=top-newSize;
			exterior.size[1]=newSize;
			}
		else if(resizeBorderMask&0x8)
			{
			GLfloat bottom=exterior.origin[1];
			GLfloat newTop=p[1]+resizeOffset[1];
			GLfloat newSize=newTop-bottom;
			if(newSize<minSize[1])
				newSize=minSize[1];
			exterior.size[1]=newSize;
			}
		
		if(exterior.size[0]!=getExterior().size[0]||exterior.size[1]!=getExterior().size[1])
			{
			/* Resize the popup window: */
			resize(exterior);
			}
		}
	}

void PopupWindow::addChild(Widget* newChild)
	{
	if(newChild!=titleBar)
		{
		/* Delete the current child: */
		delete child;
		child=0;
		
		/* Add the new child: */
		child=newChild;
		
		/* Resize the widget: */
		resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
		}
	}

void PopupWindow::requestResize(Widget* requestChild,const Vector& newExteriorSize)
	{
	/* Calculate the title bar's size: */
	Vector newSize=requestChild==titleBar?newExteriorSize:titleBar->calcNaturalSize();
	
	if(child!=0)
		{
		/* Calculate the child's size: */
		Vector childSize=requestChild==child?newExteriorSize:child->calcNaturalSize();
		childSize[0]+=2.0f*childBorderWidth;
		childSize[1]+=2.0f*childBorderWidth;
		
		/* Merge the child's size with the new window size: */
		if(newSize[0]<childSize[0])
			newSize[0]=childSize[0];
		newSize[1]+=childSize[1];
		}
	
	/* Resize the widget: */
	resize(Box(Vector(0.0f,0.0f,0.0f),calcExteriorSize(newSize)));
	}

Widget* PopupWindow::getFirstChild(void)
	{
	/* Return the only child: */
	return child;
	}

Widget* PopupWindow::getNextChild(Widget*)
	{
	/* Since there is only one child, always return null: */
	return 0;
	}

void PopupWindow::setTitleBarColor(const Color& newTitleBarColor)
	{
	/* Set title bar color: */
	titleBar->setBorderColor(newTitleBarColor);
	titleBar->setBackgroundColor(newTitleBarColor);
	}

void PopupWindow::setTitleBarTextColor(const Color& newTitleBarTextColor)
	{
	/* Set title bar text color: */
	titleBar->setForegroundColor(newTitleBarTextColor);
	}

void PopupWindow::setTitleBorderWidth(GLfloat newTitleBorderWidth)
	{
	/* Set border width of the title bar: */
	titleBar->setBorderWidth(newTitleBorderWidth);
	
	/* Resize the widget: */
	resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
	}

void PopupWindow::setTitleString(const char* newTitleString)
	{
	/* Change the title bar's label string: */
	titleBar->setLabel(newTitleString);
	
	/* Resize the widget: */
	resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
	}

void PopupWindow::setResizableFlags(bool horizontal,bool vertical)
	{
	/* Set the resizable flag mask: */
	resizableMask=0x0;
	if(horizontal)
		resizableMask|=0x1;
	if(vertical)
		resizableMask|=0x2;
	}

void PopupWindow::setChildBorderWidth(GLfloat newChildBorderWidth)
	{
	/* Change the child border width: */
	childBorderWidth=newChildBorderWidth;
	
	/* Resize the widget: */
	resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
	}

const char* PopupWindow::getTitleString(void) const
	{
	return titleBar->getLabel();
	}

}
