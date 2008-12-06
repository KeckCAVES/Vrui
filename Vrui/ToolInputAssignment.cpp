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

#include <Vrui/ToolInputLayout.h>

#include <Vrui/ToolInputAssignment.h>

namespace Vrui {

/************************************
Methods of class ToolInputAssignment:
************************************/

ToolInputAssignment::ToolInputAssignment(const ToolInputLayout& layout)
	:numDevices(layout.getNumDevices()),
	 devices(new InputDevice*[numDevices]),
	 buttonIndices(new int*[numDevices]),
	 valuatorIndices(new int*[numDevices])
	{
	/* Initialize input device assignments: */
	for(int device=0;device<numDevices;++device)
		{
		devices[device]=0;
		
		buttonIndices[device]=new int[layout.getNumButtons(device)];
		for(int i=0;i<layout.getNumButtons(device);++i)
			buttonIndices[device][i]=-1;
		
		valuatorIndices[device]=new int[layout.getNumValuators(device)];
		for(int i=0;i<layout.getNumValuators(device);++i)
			valuatorIndices[device][i]=-1;
		}
	}

ToolInputAssignment::~ToolInputAssignment(void)
	{
	/* Delete assignment arrays: */
	delete[] devices;
	for(int i=0;i<numDevices;++i)
		delete[] buttonIndices[i];
	delete[] buttonIndices;
	for(int i=0;i<numDevices;++i)
		delete[] valuatorIndices[i];
	delete[] valuatorIndices;
	}

void ToolInputAssignment::setDevice(int deviceIndex,InputDevice* newAssignedDevice)
	{
	devices[deviceIndex]=newAssignedDevice;
	}

void ToolInputAssignment::setButtonIndex(int deviceIndex,int buttonIndex,int newAssignedButtonIndex)
	{
	buttonIndices[deviceIndex][buttonIndex]=newAssignedButtonIndex;
	}

void ToolInputAssignment::setValuatorIndex(int deviceIndex,int valuatorIndex,int newAssignedValuatorIndex)
	{
	valuatorIndices[deviceIndex][valuatorIndex]=newAssignedValuatorIndex;
	}

}
