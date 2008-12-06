/***********************************************************************
SpaceBallNavigationTool - Class to represent a raw 6-DOF SpaceBall
device as a navigation tool combined with a virtual input device.
Copyright (c) 2006-2008 Oliver Kreylos

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

#ifndef VRUI_SPACEBALLNAVIGATIONTOOL_INCLUDED
#define VRUI_SPACEBALLNAVIGATIONTOOL_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Tools/NavigationTool.h>

/* Forward declarations: */
class GLContextData;
namespace Vrui {
class Glyph;
}

namespace Vrui {

class SpaceBallNavigationTool;

class SpaceBallNavigationToolFactory:public ToolFactory
	{
	friend class SpaceBallNavigationTool;
	
	/* Embedded classes: */
	public:
	struct AxisDescriptor // Structure to describe a rotational or translational SpaceBall axis
		{
		/* Elements: */
		public:
		int index; // Index of axis on raw SpaceBall device
		Vector axis; // Rotational or translational axis
		};
	
	/* Elements: */
	private:
	int numButtons; // Number of buttons on the raw SpaceBall devices
	bool* buttonToggleFlags; // Flag whether each SpaceBall button acts as a toggle
	int numRotationAxes; // Number of rotational axes on the raw SpaceBall device
	AxisDescriptor* rotationAxes; // Descriptors of rotational axes
	Scalar rotateFactor; // Conversion factor from SpaceBall valuator values to radians
	int numTranslationAxes; // Number of translational axes on the raw SpaceBall device
	AxisDescriptor* translationAxes; // Descriptors of translational axes
	Scalar translateFactor; // Conversion factor from SpaceBall valuator values to physical units
	int navigationToggleButtonIndex; // Index of button that acts as navigation toggle
	int zoomToggleButtonIndex; // Index of button that acts as zooming toggle in navigation mode
	Glyph deviceGlyph; // Glyph to be used for virtual joystick devices
	bool showScreenCenter; // Flag whether to draw the center of the screen during navigation
	
	/* Constructors and destructors: */
	public:
	SpaceBallNavigationToolFactory(ToolManager& toolManager);
	virtual ~SpaceBallNavigationToolFactory(void);
	
	/* Methods: */
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class SpaceBallNavigationTool:public NavigationTool
	{
	friend class SpaceBallNavigationToolFactory;
	
	/* Embedded classes: */
	public:
	enum NavigationMode // Enumerated type for states the tool can be in
		{
		IDLE,MOVING,ZOOMING
		};
	
	/* Elements: */
	private:
	static SpaceBallNavigationToolFactory* factory; // Pointer to the factory object for this class
	
	/* Transient virtual input device state: */
	InputDevice* spaceBall; // Pointer to the virtual SpaceBall input device
	bool* toggleButtonStates; // Current state of all simulated toggle buttons
	
	/* Transient navigation state: */
	NavigationMode navigationMode; // The tool's current navigation mode
	Point screenCenter; // Center of screen; center of rotation and scaling operations
	NavTrackerState preScale; // Transformation to be applied to the navigation transformation before scaling
	NavTrackerState rotation; // Current accumulated rotation transformation
	Scalar zoom; // Current accumulated zoom factor
	NavTrackerState postScale; // Transformation to be applied to the navigation transformation after scaling
	
	/* Constructors and destructors: */
	public:
	SpaceBallNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~SpaceBallNavigationTool(void);
	
	/* Methods: */
	virtual void initialize(void);
	virtual void deinitialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int buttonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

}

#endif
