/***********************************************************************
UIManagerPlanar - UI manager class that aligns user interface components
on a fixed plane.
Copyright (c) 2015 Oliver Kreylos

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

#ifndef VRUI_UIMANAGERPLANAR_INCLUDED
#define VRUI_UIMANAGERPLANAR_INCLUDED

#include <Geometry/Rotation.h>
#include <Geometry/Plane.h>
#include <Vrui/UIManager.h>

namespace Vrui {

class UIManagerPlanar:public UIManager
	{
	/* Elements: */
	private:
	Plane plane; // Equation of the interaction plane
	Rotation orientation; // Orientation for plane-aligned transformations
	bool constrainMovement; // Flag whether to restrict movement of UI components that are already popped up
	
	/* Constructors and destructors: */
	public:
	UIManagerPlanar(const Misc::ConfigurationFileSection& configFileSection); // Initializes UI manager from the given configuration file section
	
	/* Methods from GLMotif::WidgetArranger: */
	virtual Transformation calcTopLevelTransform(GLMotif::Widget* topLevelWidget);
	virtual Transformation calcTopLevelTransform(GLMotif::Widget* topLevelWidget,const GLMotif::Point& hotspot);
	virtual Transformation calcTopLevelTransform(GLMotif::Widget* topLevelWidget,const Transformation& widgetToWorld);
	
	/* Methods from UIManager: */
	virtual Point projectRay(const Ray& ray) const;
	virtual void projectDevice(InputDevice* device) const;
	virtual ONTransform calcUITransform(const Point& point) const;
	virtual ONTransform calcUITransform(const Ray& ray) const;
	virtual ONTransform calcUITransform(const InputDevice* device) const;
	};

}

#endif
