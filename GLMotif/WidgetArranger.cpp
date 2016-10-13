/***********************************************************************
WidgetArranger - Abstract base class for helper objects arranging top-
level GLMotif widgets in a 3D display space.
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

#include <GLMotif/WidgetArranger.h>

#include <Geometry/OrthogonalTransformation.h>

namespace GLMotif {

/*******************************
Methods of class WidgetArranger:
*******************************/

WidgetArranger::~WidgetArranger(void)
	{
	}

WidgetArranger::Transformation WidgetArranger::calcTopLevelTransform(Widget* topLevelWidget,const WidgetArranger::Transformation& widgetToWorld)
	{
	/* Return the given transformation unchanged: */
	return widgetToWorld;
	}

}
