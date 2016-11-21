/***********************************************************************
UIManagerFree - UI manager class that allows arbitrary positions and
orientations for UI components.
Copyright (c) 2015-2016 Oliver Kreylos

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

#ifndef VRUI_UIMANAGERFREE_INCLUDED
#define VRUI_UIMANAGERFREE_INCLUDED

#include <Vrui/UIManager.h>

namespace Vrui {

class UIManagerFree:public UIManager
	{
	/* Elements: */
	private:
	bool alignUiWithPointer; // Flag to align UI elements with the current interactor's pointing direction in addition to the viewing direction
	
	/* Private methods: */
	ONTransform alignUITransform(const Point& point) const; // Calculates a UI transformation for the given point
	
	/* Constructors and destructors: */
	public:
	UIManagerFree(const Misc::ConfigurationFileSection& configFileSection); // Initializes UI manager from the given configuration file section
	
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
