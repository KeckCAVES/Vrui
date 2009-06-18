/***********************************************************************
MouseSurfaceNavigationTool - Class for navigation tools that use the
mouse to move along an application-defined surface.
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

#ifndef VRUI_MOUSESURFACENAVIGATIONTOOL_INCLUDED
#define VRUI_MOUSESURFACENAVIGATIONTOOL_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Tools/SurfaceNavigationTool.h>

/* Forward declarations: */
class GLContextData;
namespace GLMotif {
class Widget;
}
namespace Vrui {
class InputDeviceAdapterMouse;
}

namespace Vrui {

class MouseSurfaceNavigationTool;

class MouseSurfaceNavigationToolFactory:public ToolFactory
	{
	friend class MouseSurfaceNavigationTool;
	
	/* Elements: */
	private:
	Scalar rotateFactor; // Distance the device has to be moved to rotate by one radians
	Vector screenScalingDirection; // Direction of scaling vector in screen's coordinates
	Scalar scaleFactor; // Distance the device has to be moved along the scaling line to scale by factor of e
	Scalar wheelScaleFactor; // Scaling factor for one wheel click
	bool fixAzimuth; // Flag whether to fix the tool's azimuth angle during panning
	bool showCompass; // Flag whether to draw a virtual compass
	bool showScreenCenter; // Flag whether to draw the center of the screen during navigation
	bool interactWithWidgets; // Flag if the mouse navigation tool doubles as a widget tool (this is an evil hack)
	
	/* Constructors and destructors: */
	public:
	MouseSurfaceNavigationToolFactory(ToolManager& toolManager);
	virtual ~MouseSurfaceNavigationToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class MouseSurfaceNavigationTool:public SurfaceNavigationTool
	{
	friend class MouseSurfaceNavigationToolFactory;
	
	/* Embedded classes: */
	private:
	enum NavigationMode // Enumerated type for states the tool can be in
		{
		IDLE,WIDGETING,ROTATING,PANNING,SCALING,SCALING_WHEEL
		};
	
	/* Elements: */
	static MouseSurfaceNavigationToolFactory* factory; // Pointer to the factory object for this class
	
	InputDeviceAdapterMouse* mouseAdapter; // Pointer to the mouse input device adapter owning the input device associated with this tool
	
	/* Transient navigation state: */
	Point currentPos; // Current projected position of mouse input device on screen
	Scalar currentValue; // Value of the associated valuator
	NavigationMode navigationMode; // The tool's current navigation mode
	Point lastPos; // Last mouse position during navigation
	NavTransform physicalFrame; // Local coordinate frame in physical coordinates
	NavTransform surfaceFrame; // Current local coordinate frame aligned to the surface in navigation coordinates
	Scalar azimuth; // Current azimuth of viewer position relative to local coordinate frame
	Scalar elevation; // Current elevation of viewer position relative to local coordinate frame
	GLMotif::Widget* draggedWidget; // Pointer to currently dragged root widget
	NavTrackerState initialWidget; // Initial widget transformation during widget dragging
	
	/* Private methods: */
	void initNavState(void); // Initializes the tool's navigation state when it is activated
	void applyNavState(void) const; // Sets the navigation transformation based on the tool's current navigation state
	Point calcScreenPos(void) const; // Calculates the screen position of the input device
	
	/* Constructors and destructors: */
	public:
	MouseSurfaceNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods from Tool: */
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int buttonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void valuatorCallback(int deviceIndex,int valuatorIndex,InputDevice::ValuatorCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

}

#endif
