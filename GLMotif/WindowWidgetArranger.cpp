/***********************************************************************
WindowWidgetArranger - Class for simple widget arrangers that align top-
level widgets to a rectangle in the (x, y) plane.
Copyright (c) 2015 Oliver Kreylos

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

#include <GLMotif/WindowWidgetArranger.h>

#include <Geometry/OrthogonalTransformation.h>
#include <GLMotif/Widget.h>

namespace GLMotif {

/*************************************
Methods of class WindowWidgetArranger:
*************************************/

WindowWidgetArranger::WindowWidgetArranger(void)
	:window(Window::Point(0,0),Window::Point(0,0))
	{
	}

WidgetArranger::Transformation WindowWidgetArranger::calcTopLevelTransform(Widget* topLevelWidget)
	{
	/* Get the widget's hot spot: */
	Point hotSpotWidget(topLevelWidget->calcHotSpot().getXyzw());
	
	/* Here we could find some appropriate new widget position in window space... */
	
	/* Return a window-aligned transformation that moves the widget's hot spot to the center of the window: */
	Window::Point centerWindow=Geometry::mid(window.min,window.max);
	return Transformation::translate(Point(centerWindow[0],centerWindow[1],0)-hotSpotWidget);
	}

WidgetArranger::Transformation WindowWidgetArranger::calcTopLevelTransform(Widget* topLevelWidget,const Point& hotSpot)
	{
	/* Get the widget's hot spot: */
	Point hotSpotWidget(topLevelWidget->calcHotSpot().getXyzw());
	
	/* Return a window-aligned transformation that moves the window's hot spot to the projected window position: */
	return Transformation::translate(Point(hotSpot[0],hotSpot[1],0)-hotSpotWidget);
	}

WidgetArranger::Transformation WindowWidgetArranger::calcTopLevelTransform(Widget* topLevelWidget,const WidgetArranger::Transformation& widgetToWorld)
	{
	/* Get the widget's hot spot: */
	Point hotSpotWidget(topLevelWidget->calcHotSpot().getXyzw());
	
	/* Transform the hot spot to window coordinates: */
	Point hotSpot=widgetToWorld.transform(hotSpot);
	
	/* Project the hot spot to the window plane: */
	hotSpot[2]=Scalar(0);
	
	/* Return a window-aligned transformation that moves the widget's hot spot to its projected window position: */
	Transformation result=Transformation::translate(hotSpot-hotSpotWidget);
	result*=Transformation::scaleAround(hotSpotWidget,widgetToWorld.getScaling());
	return result;
	}

void WindowWidgetArranger::setWindow(const WindowWidgetArranger::Window& newWindow)
	{
	/* Set the window rectangle: */
	window=newWindow;
	
	/* At this point, maybe move popped-up widgets or something... */
	}

}
