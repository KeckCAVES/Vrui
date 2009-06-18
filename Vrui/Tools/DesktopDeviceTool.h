/***********************************************************************
DesktopDeviceTool - Class to represent a desktop input device (joystick,
spaceball, etc.) as a virtual input device.
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

#ifndef VRUI_DESKTOPDEVICETOOL_INCLUDED
#define VRUI_DESKTOPDEVICETOOL_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Geometry.h>
#include <Vrui/Tools/TransformTool.h>

/* Forward declarations: */
class GLContextData;
namespace Vrui {
class Glyph;
}

namespace Vrui {

class DesktopDeviceTool;

class DesktopDeviceToolFactory:public ToolFactory
	{
	friend class DesktopDeviceTool;
	
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
	int homeButtonIndex; // Index of button that resets the virtual input device's position and orientation to the initials
	Glyph deviceGlyph; // Glyph to be used for virtual input devices
	
	/* Constructors and destructors: */
	public:
	DesktopDeviceToolFactory(ToolManager& toolManager);
	virtual ~DesktopDeviceToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class DesktopDeviceTool:public TransformTool
	{
	friend class DesktopDeviceToolFactory;
	
	/* Elements: */
	private:
	static DesktopDeviceToolFactory* factory; // Pointer to the factory object for this class
	
	/* Transient virtual input device state: */
	TrackerState homePosition; // Initial position and orientation of the virtual input device
	int axisIndexBase; // Base index of current axis shift level
	
	/* Protected methods: */
	protected:
	bool setButtonState(int buttonIndex,bool newButtonState);
	
	/* Constructors and destructors: */
	public:
	DesktopDeviceTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~DesktopDeviceTool(void);
	
	/* Methods from Tool: */
	virtual void initialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int buttonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void valuatorCallback(int deviceIndex,int deviceValuatorIndex,InputDevice::ValuatorCallbackData* cbData);
	virtual void frame(void);
	};

}

#endif
