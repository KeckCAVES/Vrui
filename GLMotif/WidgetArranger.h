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

#ifndef GLMOTIF_WIDGETARRANGER_INCLUDED
#define GLMOTIF_WIDGETARRANGER_INCLUDED

#include <GLMotif/Types.h>

/* Forward declarations: */
namespace Geometry {
template <class ScalarParam,int dimensionParam>
class OrthogonalTransformation;
}
namespace GLMotif {
class Widget;
}

namespace GLMotif {

class WidgetArranger
	{
	/* Embedded classes: */
	public:
	typedef Geometry::Point<Scalar,3> Point;
	typedef Geometry::OrthogonalTransformation<Scalar,3> Transformation;
	
	/* Constructors and destructors: */
	public:
	virtual ~WidgetArranger(void);
	
	/* Methods: */
	virtual Transformation calcTopLevelTransform(Widget* topLevelWidget) =0; // Returns a default transformation for the given top-level widget
	virtual Transformation calcTopLevelTransform(Widget* topLevelWidget,const Point& hotspot) =0; // Returns a transformation to place the given top-level widget at the given hotspot position
	virtual Transformation calcTopLevelTransform(Widget* topLevelWidget,const Transformation& widgetToWorld); // Adjusts the given transformation for the given top-level widget, for example during dragging
	};

}

#endif
