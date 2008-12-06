/***********************************************************************
ToolInputAssignment - Class defining the input assignments of a tool.
Copyright (c) 2004-2005 Oliver Kreylos

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

#ifndef VRUI_TOOLINPUTASSIGNMENT_INCLUDED
#define VRUI_TOOLINPUTASSIGNMENT_INCLUDED

/* Forward declarations: */
namespace Vrui {
class InputDevice;
class ToolInputLayout;
}

namespace Vrui {

class ToolInputAssignment
	{
	/* Elements: */
	private:
	int numDevices; // Number of devices
	InputDevice** devices; // Array of pointers to assigned input devices
	int** buttonIndices; // Arrays of assigned button indices for each input device
	int** valuatorIndices; // Arrays of assigned valuator indices for each input device
	
	/* Constructors and destructors: */
	public:
	ToolInputAssignment(const ToolInputLayout& layout); // Creates "empty" assignment for given layout
	private:
	ToolInputAssignment(const ToolInputAssignment& source); // Disallow copy constructor
	ToolInputAssignment& operator=(const ToolInputAssignment& source); // Disallow assignment operator
	public:
	~ToolInputAssignment(void); // Destroys assignment
	
	/* Methods: */
	void setDevice(int deviceIndex,InputDevice* newAssignedDevice); // Assigns an input device
	void setButtonIndex(int deviceIndex,int buttonIndex,int newAssignedButtonIndex); // Assigns a button index
	void setValuatorIndex(int deviceIndex,int valuatorIndex,int newAssignedValuatorIndex); // Assigns a valuator index
	InputDevice* getDevice(int deviceIndex) const // Returns pointer to assigned input device
		{
		return devices[deviceIndex];
		}
	int getButtonIndex(int deviceIndex,int buttonIndex) const // Returns index of assigned button
		{
		return buttonIndices[deviceIndex][buttonIndex];
		}
	int getValuatorIndex(int deviceIndex,int valuatorIndex) const // Returns index of assigned valuator
		{
		return valuatorIndices[deviceIndex][valuatorIndex];
		}
	};

}

#endif
