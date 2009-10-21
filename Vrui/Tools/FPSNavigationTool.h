/***********************************************************************
FPSNavigationTool - Class encapsulating the navigation behaviour of a
typical first-person shooter (FPS) game.
Copyright (c) 2005-2009 Oliver Kreylos

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

#ifndef VRUI_FPSNAVIGATIONTOOL_INCLUDED
#define VRUI_FPSNAVIGATIONTOOL_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Tools/SurfaceNavigationTool.h>

/* Forward declarations: */
class GLContextData;
namespace Vrui {
class InputDeviceAdapterMouse;
}

namespace Vrui {

class FPSNavigationTool;

class FPSNavigationToolFactory:public ToolFactory
	{
	friend class FPSNavigationTool;
	
	/* Elements: */
	private:
	Scalar rotateFactor; // Distance the mouse has to be moved to rotate by one radians
	Scalar moveSpeed; // Moving speed when pressing move buttons
	
	/* Constructors and destructors: */
	public:
	FPSNavigationToolFactory(ToolManager& toolManager);
	virtual ~FPSNavigationToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class FPSNavigationTool:public SurfaceNavigationTool
	{
	friend class FPSNavigationToolFactory;
	
	/* Elements: */
	private:
	static FPSNavigationToolFactory* factory; // Pointer to the factory object for this class
	InputDeviceAdapterMouse* mouseAdapter; // Mouse adapter controlling the tool's input device (0 if none)
	Rotation navFrame; // Constant navigation frame in physical coordinates (x: right, y: front, z: up)
	
	/* Transient navigation state: */
	Scalar oldMousePos[2]; // Mouse position in input device adapter before navigation started
	NavTransform surfaceFrame; // Current local coordinate frame aligned to the surface in navigation coordinates
	Point headPos; // Current position of the head point in physical coordinates
	Point footPos; // Current position of the foot point in physical coordinates
	Scalar angles[2]; // Current yaw (around z) and pitch (around x) angles in radians
	Vector moveVelocity; // Current movement velocity in frame coordinates
	Point lastMousePos; // Last mouse position in screen coordinates
	
	/* Private methods: */
	Point calcMousePosition(void) const; // Calculates the current mouse position in screen coordinates
	void startNavigating(void); // Enters navigation mode
	void applyNavigation(void); // Applies the tool's current navigation state to the navigation transformation
	void stopNavigating(void); // Leaves navigation mode
	
	/* Constructors and destructors: */
	public:
	FPSNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods from Tool: */
	virtual void initialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int buttonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	};

}

#endif
