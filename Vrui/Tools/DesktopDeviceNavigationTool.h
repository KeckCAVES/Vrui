/***********************************************************************
DesktopDeviceNavigationTool - Class to represent a desktop input device
(joystick, spaceball, etc.) as a navigation tool combined with a virtual
input device.
Copyright (c) 2006-2009 Oliver Kreylos

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

#ifndef VRUI_DESKTOPDEVICENAVIGATIONTOOL_INCLUDED
#define VRUI_DESKTOPDEVICENAVIGATIONTOOL_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Geometry.h>
#include <Vrui/Tools/NavigationTool.h>

/* Forward declarations: */
class GLContextData;
namespace Vrui {
class Glyph;
}

namespace Vrui {

class DesktopDeviceNavigationTool;

class DesktopDeviceNavigationToolFactory:public ToolFactory
	{
	friend class DesktopDeviceNavigationTool;
	
	/* Embedded classes: */
	public:
	struct AxisDescriptor // Structure to describe a rotational or translational axis
		{
		/* Elements: */
		public:
		int index; // Index of axis on raw device
		Vector axis; // Rotational or translational axis
		};
	
	/* Elements: */
	private:
	int numButtons; // Number of buttons on the raw device
	bool* buttonToggleFlags; // Flag whether each device button acts as a toggle
	int* buttonAxisShiftMasks; // Bit masks whether each device button acts as a shift button for device axes
	int numValuators; // Number of valuators on the raw device
	int numRotationAxes; // Number of rotational axes
	AxisDescriptor* rotationAxes; // Descriptors of rotational axes
	Scalar rotateFactor; // Conversion factor from device valuator values to radians
	int numTranslationAxes; // Number of translational axes
	AxisDescriptor* translationAxes; // Descriptors of translational axes
	Scalar translateFactor; // Conversion factor from device valuator values to physical units
	int navigationButtonIndex; // Index of button that acts as navigation toggle
	bool invertNavigation; // Flag whether to invert axis behavior in navigation mode (model-in-hand vs camera-in-hand)
	int numZoomAxes; // Number of zooming axes
	AxisDescriptor* zoomAxes; // Descriptions of zooming axes
	Scalar zoomFactor; // Conversion factor from device valuator values to scaling factors
	Point navigationCenter; // Center point for rotation and zoom navigation
	int homeButtonIndex; // Index of button that resets the virtual input device's position and orientation to the initials
	Glyph deviceGlyph; // Glyph to be used for virtual input devices
	bool showScreenCenter; // Flag whether to draw the center of the screen during navigation
	
	/* Constructors and destructors: */
	public:
	DesktopDeviceNavigationToolFactory(ToolManager& toolManager);
	virtual ~DesktopDeviceNavigationToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class DesktopDeviceNavigationTool:public NavigationTool
	{
	friend class DesktopDeviceNavigationToolFactory;
	
	/* Elements: */
	private:
	static DesktopDeviceNavigationToolFactory* factory; // Pointer to the factory object for this class
	
	/* Transient virtual input device state: */
	InputDevice* virtualDevice; // Pointer to the virtual input device
	TrackerState homePosition; // Initial position and orientation of the virtual input device
	bool* toggleButtonStates; // Current state of all simulated toggle buttons
	int axisIndexBase; // Base index of current axis shift level
	
	/* Transient navigation state: */
	NavTrackerState preScale; // Transformation to be applied to the navigation transformation before scaling
	NavTrackerState rotation; // Current accumulated rotation transformation
	Scalar zoom; // Current accumulated zoom factor
	NavTrackerState postScale; // Transformation to be applied to the navigation transformation after scaling
	
	/* Private methods: */
	void setButtonState(int buttonIndex,bool newButtonState); // Sets a device button to a new state
	
	/* Constructors and destructors: */
	public:
	DesktopDeviceNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~DesktopDeviceNavigationTool(void);
	
	/* Methods from Tool: */
	virtual void initialize(void);
	virtual void deinitialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int buttonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

}

#endif
