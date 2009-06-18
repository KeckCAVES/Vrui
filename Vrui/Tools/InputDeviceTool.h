/***********************************************************************
InputDeviceTool - Base class for tools used to interact with virtual
input devices.
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

#ifndef VRUI_INPUTDEVICETOOL_INCLUDED
#define VRUI_INPUTDEVICETOOL_INCLUDED

#include <Vrui/Geometry.h>
#include <Vrui/Tools/UserInterfaceTool.h>

/* Forward declarations: */
namespace Misc {
class CallbackData;
}
namespace Vrui {
class InputDevice;
class VirtualInputDevice;
class ToolManager;
}

namespace Vrui {

class InputDeviceTool;

class InputDeviceToolFactory:public ToolFactory
	{
	friend class InputDeviceTool;
	
	/* Elements: */
	private:
	bool createInputDevice; // Flag whether any newly created input device tool also creates a new unbound input device
	int newDeviceNumButtons; // Number of buttons on newly created input devices
	VirtualInputDevice* virtualInputDevice; // Pointer to the helper object for virtual input devices
	
	/* Constructors and destructors: */
	public:
	InputDeviceToolFactory(ToolManager& toolManager);
	virtual ~InputDeviceToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	};

class InputDeviceTool:public UserInterfaceTool
	{
	friend class InputDeviceToolFactory;
	
	/* Embedded classes: */
	private:
	struct ButtonHijacker // Structure to "hijack" a button from an input device and redirect it to another input device
		{
		/* Elements: */
		public:
		InputDevice* targetDevice; // Pointer to the target device of the hijacked button
		int buttonIndex; // Index of the hijacked button on the target input device
		
		/* Constructors and destructors: */
		ButtonHijacker(void);
		
		static void buttonCallbackWrapper(Misc::CallbackData* cbData,void* userData); // Hijacking callback registered with the source input device
		};
	
	/* Elements: */
	static InputDeviceToolFactory* factory; // Pointer to the factory object for this class
	InputDevice* createdDevice; // Pointer to an input device that was created by this input device tool
	ButtonHijacker* buttonHijackers; // Array of button hijackers to override buttons on the tool's input device
	bool active; // Flag whether the tool is active (has an input device grabbed)
	InputDevice* grabbedDevice; // Pointer to the input device grabbed by the tool
	
	/* Protected methods: */
	protected:
	void hijackButtons(void); // Reroutes buttons from the tool's input device to the currently grabbed device
	void releaseButtons(void); // Removes all installed button hijacks
	bool activate(const Point& position); // Tries grabbing an input device at the given position; returns true on success
	bool activate(const Ray& ray); // Tries grabbing an input device with the given ray; returns true on success
	bool isActive(void) const // Returns true if the tool is currently active
		{
		return active;
		}
	void deactivate(void); // Releases the grabbed input device and deactivates the tool
	bool grabNextDevice(void); // Grabs the next ungrabbed input device, or none if the last was grabbed; returns true if a device was grabbed
	InputDevice* getGrabbedDevice(void) const // Returns pointer to the grabbed input device
		{
		return grabbedDevice;
		}
	
	/* Constructors and destructors: */
	public:
	InputDeviceTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~InputDeviceTool(void);
	
	/* Methods from Tool: */
	virtual void initialize(void);
	virtual void deinitialize(void);
	virtual const ToolFactory* getFactory(void) const;
	};

}

#endif
