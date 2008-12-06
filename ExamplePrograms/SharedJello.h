/***********************************************************************
SharedJello - VR program to interact with "virtual Jell-O" in a
collaborative VR environment using a client/server approach and a
simplified force interaction model based on the Nanotech Construction
Kit.
Copyright (c) 2007 Oliver Kreylos

This file is part of the Virtual Jell-O interactive VR demonstration.

Virtual Jell-O is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

Virtual Jell-O is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with Virtual Jell-O; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SHAREDJELLO_INCLUDED
#define SHAREDJELLO_INCLUDED

#include <vector>
#include <Threads/Thread.h>
#include <GL/gl.h>
#include <Vrui/ToolManager.h>
#include <Vrui/DraggingToolAdapter.h>
#include <Vrui/Application.h>

#include "JelloAtom.h"
#include "JelloCrystal.h"
#include "JelloRenderer.h"
#include "SharedJelloPipe.h"

/* Forward declarations: */
namespace GLMotif {
class PopupMenu;
class PopupWindow;
class TextField;
class Slider;
}

class SharedJello:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	typedef JelloCrystal::Scalar Scalar;
	typedef JelloCrystal::Point Point;
	typedef JelloCrystal::Vector Vector;
	typedef JelloCrystal::Rotation Rotation;
	typedef JelloCrystal::Ray Ray;
	typedef JelloCrystal::Box Box;
	typedef SharedJelloPipe::ONTransform ONTransform;
	
	class AtomDragger:public Vrui::DraggingToolAdapter // Class to drag Jell-O atoms
		{
		/* Elements: */
		private:
		SharedJello* application; // Pointer to the application object "owning" this dragger
		public:
		unsigned int draggerID; // Unique ID for each dragger in the application
		bool draggerRayBased; // Flag if the dragger is ray-based
		Ray draggerRay; // Ray defining dragger's selection
		ONTransform draggerTransformation; // Transformation of the dragger in model coordinates
		bool active; // Flag if the dragger is active
		
		/* Constructors and destructors: */
		AtomDragger(Vrui::DraggingTool* sTool,SharedJello* sApplication,unsigned int sDraggerID);
		
		/* Methods: */
		virtual void idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData* cbData);
		virtual void dragStartCallback(Vrui::DraggingTool::DragStartCallbackData* cbData);
		virtual void dragCallback(Vrui::DraggingTool::DragCallbackData* cbData);
		virtual void dragEndCallback(Vrui::DraggingTool::DragEndCallbackData* cbData);
		};
	
	typedef std::vector<AtomDragger*> AtomDraggerList; // Type for lists of atom draggers
	
	friend class AtomDragger;
	
	/* Elements: */
	SharedJelloPipe* pipe; // Communication pipe connected to the shared Jell-O server
	Box domain; // The domain box of the Jell-O crystals
	Scalar atomMass;
	Scalar attenuation;
	Scalar gravity;
	unsigned int newParameterVersion;
	unsigned int parameterVersion;
	JelloCrystal* crystals[3]; // Triple buffer of client-side virtual Jell-O crystals
	JelloRenderer* renderers[3]; // Triple buffer of Jell-O crystal renderers
	volatile int lockedIndex; // Buffer index currently locked by the consumer
	volatile int mostRecentIndex; // Buffer index of most recently produced value
	Threads::Thread communicationThread; // The thread receiving state update packets from the server
	
	/* Interaction parameters: */
	unsigned int nextDraggerID; // ID to be assigned to the next atom dragger
	AtomDraggerList atomDraggers; // List of active atom draggers
	
	GLMotif::PopupMenu* mainMenu; // The program's main menu
	GLMotif::PopupWindow* settingsDialog; // The settings dialog
	GLMotif::TextField* jigglinessTextField;
	GLMotif::Slider* jigglinessSlider;
	GLMotif::TextField* viscosityTextField;
	GLMotif::Slider* viscositySlider;
	GLMotif::TextField* gravityTextField;
	GLMotif::Slider* gravitySlider;
	
	/* Private methods: */
	GLMotif::PopupMenu* createMainMenu(void);
	void updateSettingsDialog(void); // Updates the settings dialog
	GLMotif::PopupWindow* createSettingsDialog(void);
	void sendParamUpdate(void); // Sends a parameter update method to the server
	void* communicationThreadMethod(void); // The thread method receiving state updates from the server
	
	/* Constructors and destructors: */
	public:
	SharedJello(int& argc,char**& argv,char**& appDefaults);
	virtual ~SharedJello(void);
	
	/* Methods: */
	virtual void toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData);
	virtual void toolDestructionCallback(Vrui::ToolManager::ToolDestructionCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	void centerDisplayCallback(Misc::CallbackData* cbData);
	void showSettingsDialogCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void createInputDeviceCallback(Misc::CallbackData* cbData);
	void jigglinessSliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);
	void viscositySliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);
	void gravitySliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);
	};

#endif
