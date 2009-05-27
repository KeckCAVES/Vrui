/***********************************************************************
TransformTool - Base class for tools used to transform the position or
orientation of input devices.
Copyright (c) 2007-2009 Oliver Kreylos

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

#ifndef VRUI_TRANSFORMTOOL_INCLUDED
#define VRUI_TRANSFORMTOOL_INCLUDED

#include <Vrui/Tools/Tool.h>

/* Forward declarations: */
namespace Vrui {
class ToolManager;
}

namespace Vrui {

class TransformTool;

class TransformToolFactory:public ToolFactory
	{
	friend class TransformTool;
	
	/* Elements: */
	private:
	int numButtons; // Number of buttons to create on the transformed device
	bool* buttonToggleFlags; // Array of flags whether each button acts as a toggle
	int numValuators; // Number of valuators to create on the transformed device
	
	/* Constructors and destructors: */
	public:
	TransformToolFactory(ToolManager& toolManager);
	virtual ~TransformToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	
	/* New methods: */
	int getNumButtons(void) const
		{
		return numButtons;
		}
	bool getButtonToggleFlag(int buttonIndex) const
		{
		return buttonToggleFlags[buttonIndex];
		}
	int getNumValuators(void) const
		{
		return numValuators;
		}
	};

class TransformTool:public Tool
	{
	friend class TransformToolFactory;
	
	/* Elements: */
	private:
	static TransformToolFactory* factory; // Pointer to the factory object for this class
	protected:
	InputDevice* transformedDevice; // Pointer to the transformed device controlled by this tool
	bool* buttonStates; // Current states of pass-through buttons (toggled or direct)
	bool transformEnabled; // Flag whether the tool's transformation should be enabled
	private:
	int transformDisablerButtonIndex; // Index of button on whose behalf the transformation was disabled
	
	/* Protected methods: */
	protected:
	bool setButtonState(int buttonIndex,bool newButtonState); // Sets state of a button; returns true if button state has changed
	
	/* Constructors and destructors: */
	public:
	TransformTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~TransformTool(void);
	
	/* Methods: */
	virtual void initialize(void);
	virtual void deinitialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int deviceButtonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void valuatorCallback(int deviceIndex,int deviceValuatorIndex,InputDevice::ValuatorCallbackData* cbData);
	virtual void frame(void);
	};

}

#endif
