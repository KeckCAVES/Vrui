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

#ifndef GLMOTIF_WINDOWWIDGETARRANGER_INCLUDED
#define GLMOTIF_WINDOWWIDGETARRANGER_INCLUDED

#include <Geometry/Box.h>
#include <GLMotif/WidgetArranger.h>

namespace GLMotif {

class WindowWidgetArranger:public WidgetArranger
	{
	/* Embedded classes: */
	public:
	typedef Geometry::Box<Scalar,2> Window; // Type for rectangles in window space
	
	/* Elements: */
	private:
	Window window; // The current boundaries of the display window
	
	/* Constructors and destructors: */
	public:
	WindowWidgetArranger(void); // Creates an uninitialized widget arranger; window rectangle must be set
	
	/* Methods from WidgetArranger: */
	virtual Transformation calcTopLevelTransform(Widget* topLevelWidget);
	virtual Transformation calcTopLevelTransform(Widget* topLevelWidget,const Point& hotspot);
	virtual Transformation calcTopLevelTransform(Widget* topLevelWidget,const Transformation& widgetToWorld);
	
	/* New methods: */
	void setWindow(const Window& newWindow); // Sets the window boundaries for widget placement
	};

}

#endif
