/***********************************************************************
Tool - Abstract base class for user interaction tools (navigation, menu
selection, selection, etc.).
Copyright (c) 2004-2008 Oliver Kreylos

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

#ifndef VRUI_TOOL_INCLUDED
#define VRUI_TOOL_INCLUDED

#include <Plugins/Factory.h>
#include <Vrui/InputDevice.h>
#include <Vrui/ToolInputLayout.h>
#include <Vrui/ToolInputAssignment.h>

/* Forward declarations: */
namespace Misc {
class CallbackData;
}
class GLContextData;
namespace Vrui {
class ToolManager;
}

namespace Vrui {

class Tool;

class ToolFactory:public Plugins::Factory
	{
	/* Elements: */
	protected:
	ToolInputLayout layout; // Input requirements of all tools created by this factory
	
	/* Constructors and destructors: */
	public:
	ToolFactory(const char* sClassName,ToolManager& toolManager); // Initializes tool factory settings
	
	/* Methods: */
	const ToolInputLayout& getLayout(void) const // Returns the input requirements of all tools created by this factory
		{
		return layout;
		}
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const; // Creates a tool of the class represented by this factory and assigns it to the given input device(s)
	virtual void destroyTool(Tool* tool) const; // Destroys a tool of the class represented by this factory
	};

class Tool
	{
	/* Elements: */
	protected:
	const ToolInputLayout& layout; // Layout of the tool's input
	ToolInputAssignment input; // Assignment of input device buttons and valuators to this tool
	
	/* Private methods: */
	private:
	static void buttonCallbackWrapper(Misc::CallbackData* cbData,void* userData);
	static void valuatorCallbackWrapper(Misc::CallbackData* cbData,void* userData);
	
	/* Constructors and destructors: */
	public:
	Tool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment); // Initializes tool with layout defined by given factory and given input assignment
	virtual ~Tool(void);
	
	/* Methods: */
	virtual void initialize(void); // Called right after a tool has been created and is fully installed
	virtual void deinitialize(void); // Called right before a tool is destroyed during runtime
	const ToolInputLayout& getLayout(void) const // Returns tool's input layout
		{
		return layout;
		}
	const ToolInputAssignment& getInputAssignment(void) const // Returns tool's input assignment
		{
		return input;
		}
	virtual const ToolFactory* getFactory(void) const; // Returns pointer to factory that created this tool
	void assignInputDevice(int deviceIndex,InputDevice* newAssignedDevice); // Re-assigns an input device (resets all button and valuator assignments of affected device)
	void assignButton(int deviceIndex,int deviceButtonIndex,int newAssignedButtonIndex); // Re-assigns a button of the given input device
	void assignValuator(int deviceIndex,int deviceValuatorIndex,int newAssignedValuatorIndex); // Re-assigns a valuator of the given input device
	virtual void buttonCallback(int deviceIndex,int deviceButtonIndex,InputDevice::ButtonCallbackData* cbData); // Method called when state of a button changes
	virtual void valuatorCallback(int deviceIndex,int deviceValuatorIndex,InputDevice::ValuatorCallbackData* cbData); // Method called when state of a valuator changes
	virtual void frame(void); // Method called exactly once every frame
	virtual void display(GLContextData& contextData) const; // Method for rendering the tool's current state into the current OpenGL context
	};

}

#endif
