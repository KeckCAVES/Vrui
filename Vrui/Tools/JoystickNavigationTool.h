/***********************************************************************
JoystickNavigationTool - Class to represent a raw joystick device as a
navigation tool combined with a virtual input device.
Copyright (c) 2005-2008 Oliver Kreylos

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

#ifndef VRUI_JOYSTICKNAVIGATIONTOOL_INCLUDED
#define VRUI_JOYSTICKNAVIGATIONTOOL_INCLUDED

#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Tools/NavigationTool.h>

/* Forward declarations: */
namespace Vrui {
class Glyph;
}

namespace Vrui {

class JoystickNavigationTool;

class JoystickNavigationToolFactory:public ToolFactory
	{
	friend class JoystickNavigationTool;
	
	/* Embedded classes: */
	public:
	struct AxisDescriptor // Structure to describe a rotational or translational joystick axis
		{
		/* Elements: */
		public:
		int index; // Index of axis on raw joystick device
		Vector axis; // Rotational or translational axis
		};
	
	/* Elements: */
	private:
	int numButtons; // Number of buttons on the raw joystick devices
	bool* buttonToggleFlags; // Flag whether each joystick button acts as a toggle
	int numRotationAxes; // Number of rotational axes on the raw joystick device
	AxisDescriptor* rotationAxes; // Descriptors of rotational axes
	Scalar rotateFactor; // Conversion factor from joystick valuator values to radians
	int numTranslationAxes; // Number of translational axes on the raw joystick device
	AxisDescriptor* translationAxes; // Descriptors of translational axes
	Scalar translateFactor; // Conversion factor from joystick valuator values to physical units
	int navigationToggleButtonIndex; // Index of button that acts as navigation toggle
	Glyph deviceGlyph; // Glyph to be used for virtual joystick devices
	
	/* Constructors and destructors: */
	public:
	JoystickNavigationToolFactory(ToolManager& toolManager);
	virtual ~JoystickNavigationToolFactory(void);
	
	/* Methods: */
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class JoystickNavigationTool:public NavigationTool
	{
	friend class JoystickNavigationToolFactory;
	
	/* Elements: */
	private:
	static JoystickNavigationToolFactory* factory; // Pointer to the factory object for this class
	InputDevice* joystick; // Pointer to the virtual joystick input device
	bool* toggleButtonStates; // Current state of all simulated toggle buttons
	
	/* Transient navigation state: */
	NavTrackerState preScale; // Transformation to be applied to the navigation transformation before scaling
	
	/* Constructors and destructors: */
	public:
	JoystickNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~JoystickNavigationTool(void);
	
	/* Methods: */
	virtual void initialize(void);
	virtual void deinitialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int buttonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	};

}

#endif
