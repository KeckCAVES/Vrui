/***********************************************************************
InputDeviceAdapterMultitouch - Class to convert a direct-mode
multitouch-capable screen into a set of Vrui input devices.
Copyright (c) 2015 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_INPUTDEVICEADAPTERMULTITOUCH_INCLUDED
#define VRUI_INTERNAL_INPUTDEVICEADAPTERMULTITOUCH_INCLUDED

#include <Misc/HashTable.h>
#include <Vrui/Geometry.h>
#include <Vrui/Internal/InputDeviceAdapter.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
namespace Vrui {
class VRWindow;
}

namespace Vrui {

class InputDeviceAdapterMultitouch:public InputDeviceAdapter
	{
	/* Embedded classes: */
	private:
	struct DeviceMapper // Structure mapping a current multitouch contact to a Vrui input device
		{
		/* Embedded classes: */
		public:
		enum State // Device mapper state
			{
			Inactive, // No associated touch contact
			Modifier, // Touch contact selecting a button plane from a panel
			Activating, // New touch contact; waiting for additional multi-contacts to trigger different buttons
			Active // Primary or secondary tracked touch contact
			};
		
		/* Elements: */
		public:
		InputDevice* device; // Vrui input device mapped to this device mapper
		State state; // Current device mapper state
		double activationTimeout; // Timeout for mapper activation
		DeviceMapper* pred; // Predecessor in list of joined touch contacts (head of list is primary)
		DeviceMapper* succ; // Successor in list of joined touch contacts
		int buttonIndex; // Index of device button triggered by this device mapping
		VRWindow* window; // Window from which the most recent multitouch event for this device mapper was received
		Scalar windowPos[2]; // The current position of the touch contact assigned to this device mapper in window coordinates
		Scalar offset[2]; // Offset vector for primary contacts to smoothly manage creation and loss of secondary contacts
		bool dead; // Flag indicating that the touch contact associated with this device mapper has finished
		
		/* Constructors and destructors: */
		DeviceMapper(void)
			:device(0),
			 state(Inactive),
			 activationTimeout(0.0),
			 pred(0),succ(0),
			 buttonIndex(-1),
			 window(0),
			 dead(false)
			{
			windowPos[0]=windowPos[1]=Scalar(0.5);
			}
		};
	
	typedef Misc::HashTable<int,DeviceMapper*> TouchIdMapper;
	
	/* Elements: */
	int maxNumDevices; // Maximum number of parallel touch contacts
	int numModifierButtons; // Number of modifier buttons on the left-swipe panel
	int numDeviceButtons; // Number of simulated buttons per touch contact per modifier plane
	double activationInterval; // Time interval for multi-contact activation
	double multicontactRadius; // Maximum radius to detect multi-contact events
	DeviceMapper* deviceMappers; // Array of device mappers from multitouch events to Vrui input devices
	int modifierPlane; // Index of currently active modifier plane
	TouchIdMapper touchIdMapper; // Hash table mapping XInput2 touch IDs to device mapper structures
	int modifierTouchId; // ID of touch contact currently selecting in the modifier panel
	int previousModifierPlane; // Index of modifier plane active when the current modifier touch event started
	double modifierPanelTimeout; // Timeout for drawing the modifier button panel after the modifier touch finished
	VRWindow* mostRecentTouchWindow; // The window that generated the most recently received touch event
	
	/* Constructors and destructors: */
	public:
	InputDeviceAdapterMultitouch(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection);
	virtual ~InputDeviceAdapterMultitouch(void);
	
	/* Methods from InputDeviceAdapter: */
	virtual std::string getFeatureName(const InputDeviceFeature& feature) const;
	virtual int getFeatureIndex(InputDevice* device,const char* featureName) const;
	virtual void updateInputDevices(void);
	virtual void glRenderAction(GLContextData& contextData) const;
	
	/* New methods: */
	void touchBegin(VRWindow* newWindow,int touchId,Scalar touchX,Scalar touchY); // Signals a new touch contact
	void touchUpdate(VRWindow* newWindow,int touchId,Scalar touchX,Scalar touchY); // Updates an existing touch contact
	void touchEnd(VRWindow* newWindow,int touchId,Scalar touchX,Scalar touchY); // Ends a touch contact
	};

}

#endif
