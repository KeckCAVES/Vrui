/***********************************************************************
VRDeviceState - Class to represent the current state of a single or
multiple VR devices.
Copyright (c) 2002-2005 Oliver Kreylos

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

#ifndef VRUI_VRDEVICESTATE_INCLUDED
#define VRUI_VRDEVICESTATE_INCLUDED

#include <Geometry/OrthonormalTransformation.h>

namespace Vrui {

class VRDeviceState
	{
	/* Embedded classes: */
	public:
	struct TrackerState // Type for tracker states
		{
		/* Embedded classes: */
		public:
		typedef Geometry::OrthonormalTransformation<float,3> PositionOrientation; // Type for tracker position/orientation
		typedef Geometry::Vector<float,3> LinearVelocity; // Type for linear velocity vectors
		typedef Geometry::Vector<float,3> AngularVelocity; // Type for angular velocity vectors
		
		/* Elements: */
		PositionOrientation positionOrientation; // Current tracker position/orientation
		LinearVelocity linearVelocity; // Current linear velocity in units/s
		AngularVelocity angularVelocity; // Current angular velocity in radians/s
		};
	
	typedef bool ButtonState; // Type for button states
	typedef float ValuatorState; // Type for valuator states
	
	/* Elements: */
	private:
	int numTrackers; // Number of represented trackers
	TrackerState* trackerStates; // Array of current tracker states
	int numButtons; // Number of represented buttons
	ButtonState* buttonStates; // Array of current button states
	int numValuators; // Number of represented valuators
	ValuatorState* valuatorStates; // Array of current valuator states
	
	/* Constructors and destructors: */
	public:
	VRDeviceState(void) // Creates empty device state
		:numTrackers(0),trackerStates(0),
		 numButtons(0),buttonStates(0),
		 numValuators(0),valuatorStates(0)
		{
		}
	VRDeviceState(int sNumTrackers,int sNumButtons,int sNumValuators) // Creates device state of given layout
		:numTrackers(sNumTrackers),trackerStates(new TrackerState[numTrackers]),
		 numButtons(sNumButtons),buttonStates(new ButtonState[numButtons]),
		 numValuators(sNumValuators),valuatorStates(new ValuatorState[numValuators])
		{
		}
	~VRDeviceState(void)
		{
		delete[] trackerStates;
		delete[] buttonStates;
		delete[] valuatorStates;
		}
	
	/* Methods: */
	void setLayout(int newNumTrackers,int newNumButtons,int newNumValuators) // Sets the number of represented trackers, buttons and valuators
		{
		/* Delete old state arrays: */
		delete[] trackerStates;
		delete[] buttonStates;
		delete[] valuatorStates;
		
		/* Set new layout: */
		numTrackers=newNumTrackers;
		trackerStates=new TrackerState[numTrackers];
		numButtons=newNumButtons;
		buttonStates=new ButtonState[numButtons];
		numValuators=newNumValuators;
		valuatorStates=new ValuatorState[numValuators];
		}
	int getNumTrackers(void) const // Returns number of represented trackers
		{
		return numTrackers;
		}
	int getNumButtons(void) const // Returns number of represented buttons
		{
		return numButtons;
		}
	int getNumValuators(void) const // Returns number of represented valuators
		{
		return numValuators;
		}
	const TrackerState& getTrackerState(int trackerIndex) const // Returns state of single tracker
		{
		return trackerStates[trackerIndex];
		}
	void setTrackerState(int trackerIndex,const TrackerState& newTrackerState) // Updates state of single tracker
		{
		trackerStates[trackerIndex]=newTrackerState;
		}
	ButtonState getButtonState(int buttonIndex) const // Returns state of single button
		{
		return buttonStates[buttonIndex];
		}
	void setButtonState(int buttonIndex,ButtonState newButtonState) // Updates state of single button
		{
		buttonStates[buttonIndex]=newButtonState;
		}
	ValuatorState getValuatorState(int valuatorIndex) const // Returns state of single valuator
		{
		return valuatorStates[valuatorIndex];
		}
	void setValuatorState(int valuatorIndex,ValuatorState newValuatorState) // Updates state of single valuator
		{
		valuatorStates[valuatorIndex]=newValuatorState;
		}
	const TrackerState* getTrackerStates(void) const // Returns array of tracker states
		{
		return trackerStates;
		}
	TrackerState* getTrackerStates(void) // Ditto
		{
		return trackerStates;
		}
	const ButtonState* getButtonStates(void) const // Returns array of button states
		{
		return buttonStates;
		}
	ButtonState* getButtonStates(void) // Ditto
		{
		return buttonStates;
		}
	const ValuatorState* getValuatorStates(void) const // Returns array of valuator states
		{
		return valuatorStates;
		}
	ValuatorState* getValuatorStates(void) // Ditto
		{
		return valuatorStates;
		}
	};

}

#endif
