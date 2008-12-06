/***********************************************************************
CoordinateTransform - Base class for application-defined, potentially
non-linear, coordinate transformations from "user interest space" to
Vrui's navigation space. Used by measurement tools to display
measurements in the coordinates and units expected by users of
particular applications.
Base class implements identity transformation.
Copyright (c) 2008 Oliver Kreylos

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

#include <Misc/ThrowStdErr.h>

#include <Vrui/CoordinateTransform.h>

namespace Vrui {

/************************************
Methods of class CoordinateTransform:
************************************/

CoordinateTransform::~CoordinateTransform(void)
	{
	}

int CoordinateTransform::getNumComponents(void) const
	{
	return 3;
	}

const char* CoordinateTransform::getComponentName(int componentIndex) const
	{
	switch(componentIndex)
		{
		case 0:
			return "X";
			break;
		
		case 1:
			return "Y";
			break;
		
		case 2:
			return "Z";
			break;
		
		default:
			Misc::throwStdErr("CoordinateTransform::getComponentName: Invalid component index %d",componentIndex);
			return ""; // Never reached; just to make compiler happy
		}
	}

Point CoordinateTransform::transform(const Point& navigationPoint) const
	{
	return navigationPoint;
	}

}
