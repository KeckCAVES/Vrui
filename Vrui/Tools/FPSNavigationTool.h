/***********************************************************************
FPSNavigationTool - Class encapsulating the navigation behaviour of a
typical first-person shooter (FPS) game.
Copyright (c) 2005-2010 Oliver Kreylos

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
#include <GL/GLNumberRenderer.h>
#include <Vrui/SurfaceNavigationTool.h>

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
	Scalar fallAcceleration; // Acceleration when falling in physical space units per second^2, defaults to g
	Scalar probeSize; // Size of probe to use when aligning surface frames
	Scalar maxClimb; // Maximum amount of climb per frame
	bool fixAzimuth; // Flag whether to fix the tool's azimuth angle during movement
	bool showHud; // Flag whether to draw a heads-up display
	
	/* Constructors and destructors: */
	public:
	FPSNavigationToolFactory(ToolManager& toolManager);
	virtual ~FPSNavigationToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
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
	GLNumberRenderer numberRenderer; // Helper object to render numbers using a HUD-style font
	
	/* Transient navigation state: */
	Scalar oldMousePos[2]; // Mouse position in input device adapter before navigation started
	Point footPos; // Current position of main viewer's foot in physical coordinates
	Scalar headHeight; // Height of viewer's head above the foot point
	NavTransform surfaceFrame; // Current local coordinate frame aligned to the surface in navigation coordinates
	Scalar azimuth; // Current azimuth of viewer position relative to local coordinate frame
	Scalar elevation; // Current elevation of viewer position relative to local coordinate frame
	Vector moveVelocity; // Current movement velocity in frame coordinates
	Point lastMousePos; // Last mouse position in screen coordinates
	
	/* Private methods: */
	Point calcMousePosition(void) const; // Calculates the current mouse position in screen coordinates
	void applyNavState(void); // Applies the tool's current navigation state to the navigation transformation
	void initNavState(void); // Initializes the tool's navigation state when it is activated
	void stopNavState(void); // Leaves navigation mode
	
	/* Constructors and destructors: */
	public:
	FPSNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods from Tool: */
	virtual void initialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

}

#endif
