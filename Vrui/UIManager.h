/***********************************************************************
UIManager - Base class for managers arranging user interface components,
mapping user interface devices and tools, and create user-aligned
displays in physical space.
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

#ifndef VRUI_UIMANAGER_INCLUDED
#define VRUI_UIMANAGER_INCLUDED

#include <GLMotif/WidgetArranger.h>
#include <Vrui/Geometry.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
namespace Vrui {
class InputDevice;
class GUIInteractor;
}

namespace Vrui {

class UIManager:public GLMotif::WidgetArranger
	{
	/* Elements: */
	private:
	GUIInteractor* activeGuiInteractor; // The currently active GUI interactor
	GUIInteractor* mostRecentGuiInteractor; // Pointer to the most-recently used GUI interactor, to calculate an appropriate position to pop up dialog windows
	Point mostRecentHotSpot; // Final hot spot position when the most-recently used GUI interactor is destroyed
	Vector mostRecentDirection; // Final interaction direction when the most-recently used GUI interactor is destroyed
	
	/* Constructors and destructors: */
	public:
	UIManager(const Misc::ConfigurationFileSection& configFileSection); // Initializes UI manager from the given configuration file section
	
	/* New methods: */
	bool canActivateGuiInteractor(const GUIInteractor* guiInteractor) const // Returns true if the given GUI interaction tool can be activated, or is already active
		{
		return activeGuiInteractor==0||activeGuiInteractor==guiInteractor;
		}
	bool activateGuiInteractor(GUIInteractor* guiInteractor); // Tries activating the given GUI interaction tool; returns true if successful
	void deactivateGuiInteractor(GUIInteractor* guiInteractor); // Deactivates the given GUI interaction tool; does nothing if it's not active
	void destroyGuiInteractor(GUIInteractor* guiInteractor); // Called to notify the UI manager of the destruction of a GUI interaction tool
	Point getHotSpot(void) const; // Returns a hot spot for newly-opened top-level widgets
	Vector getDirection(void) const; // Returns an interaction direction for newly-opened top-level widgets
	virtual Point projectRay(const Ray& ray) const =0; // Projects a ray onto the UI surface
	virtual void projectDevice(InputDevice* device) const =0; // Projects an input device onto the UI surface based on its device ray
	virtual ONTransform calcUITransform(const Point& point) const =0; // Returns a transformation to align a UI component at the given position
	virtual ONTransform calcUITransform(const Ray& ray) const =0; // Returns a transformation to align a UI component along the given ray
	virtual ONTransform calcUITransform(const InputDevice* device) const =0; // Returns a transformation to align a UI component for interaction with the given device
	};

}

#endif
