/***********************************************************************
DisplayState - Class to store Vrui rendering state in a GLContextData
object so it can be queried by applications from inside their display
methods. Workaround until a "proper" method to pass display state into
applications is found.
Copyright (c) 2009 Oliver Kreylos

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

#ifndef VRUI_DISPLAYSTATE_INCLUDED
#define VRUI_DISPLAYSTATE_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Geometry.h>

/* Forward declarations: */
namespace Vrui {
class VRWindow;
class Viewer;
class VRScreen;
}

namespace Vrui {

class DisplayState
	{
	/* Elements: */
	public:
	const VRWindow* window; // The VR window being rendered to
	bool resized; // Flag whether the VR window has changed size since the last redraw
	const Viewer* viewer; // The viewer whose view is currently rendered
	int eyeIndex; // Index of the eye currently projected from
	Point eyePosition; // Exact eye position used for projection
	const VRScreen* screen; // The screen onto which the viewer's view is projected
	NavTransform modelviewPhysical; // Model view transformation for physical coordinates
	NavTransform modelviewNavigational; // Model view transformation for navigational coordinates
	};

}

#endif
