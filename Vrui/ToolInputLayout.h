/***********************************************************************
ToolInputLayout - Class to represent the input requirements of tools.
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

#ifndef VRUI_TOOLINPUTLAYOUT_INCLUDED
#define VRUI_TOOLINPUTLAYOUT_INCLUDED

namespace Vrui {

class ToolInputLayout // Class describing the input layout of a tool
	{
	/* Elements: */
	private:
	int numDevices; // Number of devices used as input
	int* numButtons; // Array of number of buttons used for each device
	bool** buttonCascadable; // 2D array of button cascadable flags for each button on each device
	int* numValuators; // Array of number of valuators used for each device
	bool** valuatorCascadable; // 2D array of valuator cascadable flags for each valuator on each device
	
	/* Constructors and destructors: */
	public:
	ToolInputLayout(void); // Creates "empty" layout
	ToolInputLayout(int sNumDevices,const int sNumButtons[],const int sNumValuators[]); // Creates layout from given numbers
	private:
	ToolInputLayout(const ToolInputLayout& source); // Disallow copy constructor
	ToolInputLayout& operator=(const ToolInputLayout& source); // Disallow assignment operator
	public:
	~ToolInputLayout(void); // Destroys layout
	
	/* Methods: */
	void setNumDevices(int newNumDevices); // Changes number of devices used as input; leaves numbers of buttons and valuators of all devices undefined
	void setNumButtons(int deviceIndex,int newNumButtons); // Sets number of buttons used for given device
	void setButtonCascadable(int deviceIndex,int buttonIndex,bool newCascadable); // Sets cascadable state of the given button on the given device
	void setNumValuators(int deviceIndex,int newNumValuators); // Sets number of valuators used for given device
	void setValuatorCascadable(int deviceIndex,int valuatorIndex,bool newCascadable); // Sets cascadable state of the given valuator on the given device
	int getNumDevices(void) const // Returns number of devices used
		{
		return numDevices;
		}
	int getNumButtons(int deviceIndex) const // Returns number of buttons used for given device
		{
		return numButtons[deviceIndex];
		}
	bool isButtonCascadable(int deviceIndex,int buttonIndex) const // Returns true of the given button on the given device is cascadable
		{
		return buttonCascadable[deviceIndex][buttonIndex];
		}
	int getNumValuators(int deviceIndex) const // Returns number of valuators used for given device
		{
		return numValuators[deviceIndex];
		}
	bool isValuatorCascadable(int deviceIndex,int valuatorIndex) const // Returns true of the given valuator on the given device is cascadable
		{
		return valuatorCascadable[deviceIndex][valuatorIndex];
		}
	};

}

#endif
