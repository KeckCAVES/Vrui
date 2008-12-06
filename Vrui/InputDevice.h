/***********************************************************************
InputDevice - Class to represent input devices (6-DOF tracker with
associated buttons and valuators) in virtual reality environments.
Copyright (c) 2000-2005 Oliver Kreylos

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

#ifndef VRUI_INPUTDEVICE_INCLUDED
#define VRUI_INPUTDEVICE_INCLUDED

#include <Misc/CallbackList.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/Geometry.h>

namespace Vrui {

class InputDevice // Class for input devices
	{
	/* Embedded classes: */
	public:
	enum TrackType // Data type for input device tracking capabilities
		{
		TRACK_NONE=0x0, // No tracking at all
		TRACK_POS=0x1, // 3D position
		TRACK_DIR=0x2, // One 3D direction
		TRACK_ORIENT=0x4 // Full 3D orientation
		};
	
	struct CallbackData:public Misc::CallbackData // Generic callback data for input device events
		{
		/* Elements: */
		public:
		InputDevice* inputDevice; // The device that caused the callback
		
		/* Constructors and destructors: */
		CallbackData(InputDevice* sInputDevice)
			:inputDevice(sInputDevice)
			{
			}
		};
	
	struct ButtonCallbackData:public CallbackData // Callback data for button events
		{
		/* Elements: */
		public:
		int buttonIndex; // Index of button that changed state
		bool newButtonState; // New state of that button
		
		/* Constructors and destructors: */
		ButtonCallbackData(InputDevice* sInputDevice,int sButtonIndex,bool sNewButtonState)
			:CallbackData(sInputDevice),
			 buttonIndex(sButtonIndex),newButtonState(sNewButtonState)
			{
			}
		};
	
	struct ValuatorCallbackData:public CallbackData // Callback data for valuator events
		{
		/* Elements: */
		public:
		int valuatorIndex; // Index of valuator that changed value
		double oldValuatorValue,newValuatorValue; // Old and new valuator values
		
		/* Constructors and destructors: */
		ValuatorCallbackData(InputDevice* sInputDevice,int sValuatorIndex,double sOldValuatorValue,double sNewValuatorValue)
			:CallbackData(sInputDevice),
			 valuatorIndex(sValuatorIndex),oldValuatorValue(sOldValuatorValue),newValuatorValue(sNewValuatorValue)
			{
			}
		};
	
	/* Elements: */
	private:
	char* deviceName; // Arbitrary label to identify input devices
	int trackType; // Bitfield of tracking capabilities
	Vector deviceRayDirection; // Preferred direction of ray devices in device coordinates
	int numButtons; // Number of buttons on that device
	int numValuators; // Number of valuators on that device
	
	/* Callback management: */
	Misc::CallbackList trackingCallbacks; // List of tracking callbacks
	Misc::CallbackList* buttonCallbacks; // List of button callbacks for each button
	Misc::CallbackList* valuatorCallbacks; // List of valuator callbacks for each valuator
	
	/* Current device state: */
	TrackerState transformation; // Full (orthonormal) transformation of locator device
	Vector linearVelocity,angularVelocity; // Velocities of locator device in units/second
	bool* buttonStates; // Array of button press state(s)
	double* valuatorValues; // Array of valuator values, normalized from -1 to 1
	
	/* State for disabling callbacks: */
	bool callbacksEnabled; // Flag if callbacks are enabled
	bool* savedButtonStates; // Button states are saved at the time callbacks are disabled
	double* savedValuatorValues; // Valuator values are saved at the time callbacks are disabled
	
	/* Constructors and destructors: */
	public:
	InputDevice(void);
	InputDevice(const char* sDeviceName,int sTrackType,int sNumButtons =0,int sNumValuators =0);
	InputDevice(const InputDevice& source); // Prohibit copy constructor
	private:
	InputDevice& operator=(const InputDevice& source); // Prohibit assignment operator
	public:
	~InputDevice(void);
	
	/* Methods: */
	InputDevice& set(const char* sDeviceName,int sTrackType,int sNumButtons =0,int sNumValuators =0); // Changes input device's layout after creation
	void setDeviceRayDirection(const Vector& newDeviceRayDirection); // Sets input device's ray direction in device coordinates
	
	/* Device layout access methods: */
	const char* getDeviceName(void) const
		{
		return deviceName;
		}
	int getTrackType(void) const
		{
		return trackType;
		}
	bool hasPosition(void) const
		{
		return trackType&TRACK_POS;
		}
	bool hasDirection(void) const
		{
		return trackType&TRACK_DIR;
		}
	bool hasOrientation(void) const
		{
		return trackType&TRACK_ORIENT;
		}
	bool isPositionDevice(void) const
		{
		return trackType==TRACK_POS;
		}
	bool isRayDevice(void) const
		{
		return trackType==(TRACK_POS|TRACK_DIR);
		}
	bool is6DOFDevice(void) const
		{
		return trackType==(TRACK_POS|TRACK_DIR|TRACK_ORIENT);
		}
	const Vector& getDeviceRayDirection(void) const
		{
		return deviceRayDirection;
		}
	int getNumButtons(void) const
		{
		return numButtons;
		}
	int getNumValuators(void) const
		{
		return numValuators;
		}
	
	/* Callback registration methods: */
	Misc::CallbackList& getTrackingCallbacks(void)
		{
		return trackingCallbacks;
		}
	Misc::CallbackList& getButtonCallbacks(int buttonIndex)
		{
		return buttonCallbacks[buttonIndex];
		}
	Misc::CallbackList& getValuatorCallbacks(int valuatorIndex)
		{
		return valuatorCallbacks[valuatorIndex];
		}
	
	/* Device state manipulation methods: */
	void setTransformation(const TrackerState& newTransformation);
	void setLinearVelocity(const Vector& newLinearVelocity)
		{
		linearVelocity=newLinearVelocity;
		}
	void setAngularVelocity(const Vector& newAngularVelocity)
		{
		angularVelocity=newAngularVelocity;
		}
	void clearButtonStates(void);
	void setButtonState(int index,bool newButtonState);
	void setSingleButtonPressed(int index);
	void setValuator(int index,double value);
	
	/* Current state access methods: */
	Point getPosition(void) const
		{
		return transformation.getOrigin();
		}
	const TrackerState::Rotation& getOrientation(void) const
		{
		return transformation.getRotation();
		}
	const TrackerState& getTransformation(void) const
		{
		return transformation;
		}
	Vector getRayDirection(void) const
		{
		return transformation.transform(deviceRayDirection);
		}
	const Vector& getLinearVelocity(void) const
		{
		return linearVelocity;
		}
	const Vector& getAngularVelocity(void) const
		{
		return angularVelocity;
		}
	bool getButtonState(int index) const
		{
		return buttonStates[index];
		}
	double getValuator(int index) const
		{
		return valuatorValues[index];
		}
	
	/* Callback enable/disable methods: */
	void disableCallbacks(void);
	void enableCallbacks(void);
	};

}

#endif
