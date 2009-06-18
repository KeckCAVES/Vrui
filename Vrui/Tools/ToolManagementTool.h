/***********************************************************************
ToolManagementTool - A class to assign arbitrary tools to arbitrary
combinations of input devices / buttons / valuators using a modal dialog
and overriding callbacks.
Copyright (c) 2004-2009 Oliver Kreylos

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

#ifndef VRUI_TOOLMANAGEMENTTOOL_INCLUDED
#define VRUI_TOOLMANAGEMENTTOOL_INCLUDED

#include <Geometry/Ray.h>
#include <Vrui/Tools/MenuTool.h>

/* Forward declarations: */
namespace Misc {
class CallbackData;
}
namespace GLMotif {
class Widget;
class Button;
}
namespace Vrui {
class InputDevice;
class Viewer;
}

namespace Vrui {

class ToolManagementTool;

class ToolManagementToolFactory:public ToolFactory
	{
	friend class ToolManagementTool;
	
	/* Elements: */
	private:
	Scalar initialMenuOffset; // Offset of initial menu position along selection ray
	
	/* Constructors and destructors: */
	public:
	ToolManagementToolFactory(ToolManager& toolManager);
	virtual ~ToolManagementToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class ToolManagementTool:public MenuTool
	{
	friend class ToolManagementToolFactory;
	
	/* Embedded classes: */
	class SetCallbacksFunctor // Functor class to add or remove callbacks to/from menu buttons
		{
		/* Elements: */
		private:
		ToolManagementTool* tool; // Pointer to tool to set callbacks for
		bool remove; // If true, remove callbacks instead of adding them
		
		/* Constructors and destructors: */
		public:
		SetCallbacksFunctor(ToolManagementTool* sTool,bool sRemove);
		
		/* Methods: */
		void operator()(GLMotif::Widget* widget) const; // Processes the given widget
		};
	
	friend class SetCallbacksFunctor;
	
	/* Elements: */
	private:
	static ToolManagementToolFactory* factory; // Pointer to the factory object for this class
	const Viewer* viewer; // Viewer associated with the menu tool
	
	/* Probing state: */
	bool displayRay; // Flag whether to update and display the selection ray
	Ray selectionRay; // Current selection ray
	ToolFactory* createToolFactory; // Pointer to the factory for the tool supposed to be created
	ToolInputAssignment* tia; // Pointer to the tool input assignment structure being filled in
	bool probingForDevice;
	int currentDeviceIndex; // The index of the device currently being probed
	bool probingForButtons;
	int currentButtonIndex; // Index of currently probed button
	int firstPressedButtonIndex; // Index of first pressed button while probing
	
	/* Callback wrappers: */
	static void toolMenuSelectionCallbackWrapper(Misc::CallbackData* cbData,void* userData);
	static void inputDeviceButtonCallbackWrapper(Misc::CallbackData* cbData,void* userData);
	
	/* Private methods: */
	void finishCreatingTool(void); // Actually creates the new tool and assigns it to the probed devices and buttons
	void toolMenuSelectionCallback(GLMotif::Button* button);
	void inputDeviceButtonCallback(int buttonIndex,bool newState);
	
	/* Protected methods: */
	protected:
	void hijackButtons(void); // Reroutes buttons from the tool's input device to the currently grabbed device
	void releaseButtons(void); // Removes all installed button hijacks
	
	/* Constructors and destructors: */
	public:
	ToolManagementTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~ToolManagementTool(void);
	
	/* Methods from Tool: */
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int buttonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	
	/* New methods: */
	virtual void setMenu(MutexMenu* newMenu);
	};

}

#endif
