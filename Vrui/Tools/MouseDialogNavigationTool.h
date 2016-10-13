/***********************************************************************
MouseDialogNavigationTool - Class providing a newbie-friendly interface
to the standard MouseNavigationTool using a dialog box of navigation
options.
Copyright (c) 2007-2015 Oliver Kreylos

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

#ifndef VRUI_MOUSEDIALOGNAVIGATIONTOOL_INCLUDED
#define VRUI_MOUSEDIALOGNAVIGATIONTOOL_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GLMotif/RadioBox.h>
#include <GLMotif/ToggleButton.h>
#include <Vrui/NavigationTool.h>

/* Forward declarations: */
class GLContextData;
namespace GLMotif {
class PopupWindow;
}

namespace Vrui {

class MouseDialogNavigationTool;

class MouseDialogNavigationToolFactory:public ToolFactory
	{
	friend class MouseDialogNavigationTool;
	
	/* Embedded classes: */
	private:
	struct Configuration // Structure containing tool settings
		{
		/* Elements: */
		public:
		Scalar rotatePlaneOffset; // Offset of rotation plane from screen plane
		Scalar rotateFactor; // Distance the device has to be moved to rotate by one radians
		bool dollyCenter; // Flag whether to dolly around the display center or current device position
		bool scaleCenter; // Flag whether to scale around the display center or current device position
		Vector dollyingDirection; // Direction of dollying line in physical coordinates
		Vector scalingDirection; // Direction of scaling line in physical coordinates
		Scalar dollyFactor; // Distance the device has to be moved along the scaling line to dolly by one physical unit
		Scalar scaleFactor; // Distance the device has to be moved along the scaling line to scale by factor of e
		Scalar spinThreshold; // Distance the device has to be moved on the last step of rotation to activate spinning
		int fixedMode; // If >=0, fixes the tool in a single navigation mode without showing the dialog box: 0 - rotating, 1 - panning, 2 - dollying, 3 - scaling
		
		/* Constructors and destructors: */
		Configuration(void); // Creates default configuration
		
		/* Methods: */
		void read(const Misc::ConfigurationFileSection& cfs); // Overrides configuration from configuration file section
		void write(Misc::ConfigurationFileSection& cfs) const; // Writes configuration to configuration file section
		};
	
	/* Elements: */
	Configuration configuration; // Default configuration for all tools
	
	/* Constructors and destructors: */
	public:
	MouseDialogNavigationToolFactory(ToolManager& toolManager);
	virtual ~MouseDialogNavigationToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class MouseDialogNavigationTool:public NavigationTool
	{
	friend class MouseDialogNavigationToolFactory;
	
	/* Embedded classes: */
	private:
	enum NavigationMode // Enumerated type for states the tool can be in
		{
		ROTATING,PANNING,DOLLYING,SCALING
		};
	
	/* Elements: */
	static MouseDialogNavigationToolFactory* factory; // Pointer to the factory object for this class
	MouseDialogNavigationToolFactory::Configuration configuration; // Private configuration of this tool
	GLMotif::PopupWindow* navigationDialogPopup; // Pointer to the navigation dialog window
	
	/* Transient navigation state: */
	ONTransform interactionPlane; // Local coordinate plane in which navigation interactions happen
	Point currentPos; // Current projected position of mouse input device on screen
	double lastMoveTime; // Application time at which the projected position last changed
	NavigationMode navigationMode; // The tool's current navigation mode
	bool spinning; // Flag whether the tool is currently spinning
	Point screenCenter; // Center of screen; center of rotation and scaling operations
	Vector dollyDirection; // Transformation direction of dollying
	Point motionStart; // Start position of mouse motion
	Vector rotateOffset; // Offset vector applied to device position during rotations
	Point lastRotationPos; // Last mouse position during rotation
	Vector spinAngularVelocity; // Angular velocity when spinning
	NavTrackerState preScale; // Transformation to be applied to the navigation transformation before scaling
	NavTrackerState rotation; // Current accumulated rotation transformation
	NavTrackerState postScale; // Transformation to be applied to the navigation transformation after scaling
	bool showScreenCenter; // Flag whether to render the screen center
	
	/* Private methods: */
	void startNavigating(void); // Sets up common navigation state
	Point calcInteractionPos(void) const; // Returns the current device position in the interaction plane
	void startRotating(void); // Sets up rotation
	void startPanning(void); // Sets up panning
	void startDollying(void); // Sets up dollying
	void startScaling(void); // Sets up scaling
	void navigationModesValueChangedCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData);
	void showScreenCenterToggleValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	
	/* Constructors and destructors: */
	public:
	MouseDialogNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~MouseDialogNavigationTool(void);
	
	/* Methods from Tool: */
	virtual void configure(const Misc::ConfigurationFileSection& configFileSection);
	virtual void storeState(Misc::ConfigurationFileSection& configFileSection) const;
	virtual void initialize(void);
	virtual void deinitialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

}

#endif
