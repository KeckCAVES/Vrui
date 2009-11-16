/***********************************************************************
InputDeviceAdapterJoystick - Class to directly connect a joystick or
other device supported by the Linux joystick layer to a Vrui input
device.
Copyright (c) 2009 Oliver Kreylos

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

#ifndef VRUI_INPUTDEVICEADAPTERJOYSTICK_INCLUDED
#define VRUI_INPUTDEVICEADAPTERJOYSTICK_INCLUDED

#ifdef __DARWIN__
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDLib.h>
#endif
#include <vector>
#ifdef __DARWIN__
#include <Misc/HashTable.h>
#endif
#include <Threads/Mutex.h>
#include <Threads/Thread.h>
#include <Math/BrokenLine.h>
#include <Vrui/InputDeviceAdapter.h>

#ifdef __DARWIN__

/**************
Helper classes:
**************/

template <class TypeRefParam>
class CFAutoRelease // Helper class to automatically release Core Foundation object references upon destruction
	{
	/* Embedded classes: */
	public:
	typedef TypeRefParam TypeRef; // Underlying Core Foundation reference class
	
	/* Elements: */
	private:
	TypeRef ref; // Object reference
	
	/* Constructors and destructors: */
	public:
	CFAutoRelease(void)
		:ref(0)
		{
		}
	CFAutoRelease(TypeRef sRef)
		:ref(sRef)
		{
		}
	CFAutoRelease(const CFAutoRelease& source)
		:ref(source.ref)
		{
		if(ref!=0)
			CFRetain(ref);
		}
	CFAutoRelease& operator=(TypeRef sourceRef)
		{
		if(ref!=sourceRef&&ref!=0)
			CFRelease(ref);
		ref=sourceRef;
		return *this;
		}
	CFAutoRelease& operator=(const CFAutoRelease& source)
		{
		if(this!=&source)
			{
			if(ref!=0)
				CFRelease(ref);
			ref=source.ref;
			if(ref!=0)
				CFRetain(ref);
			}
		return *this;
		}
	~CFAutoRelease(void)
		{
		if(ref!=0)
			CFRelease(ref);
		}
	
	/* Methods: */
	operator TypeRef(void) const
		{
		return ref;
		}
	};

#endif

/* Forward declarations: */
namespace Vrui {
class InputDevice;
}

namespace Vrui {

class InputDeviceAdapterJoystick:public InputDeviceAdapter
	{
	/* Embedded classes: */
	private:
	struct Device // Structure describing a joystick device
		{
		/* Elements: */
		public:
		#ifdef __LINUX__
		int deviceFd; // File descriptor for the joystick device
		#endif
		#ifdef __DARWIN__
		std::string name; // Name of device to be created
		long vendorId,productId; // Vendor and product IDs of the joystick device
		int deviceIndex; // Index of device among all devices with the same vendor/product ID
		#endif
		int firstButtonIndex; // Index of joystick's first button in device state array
		int numButtons; // Number of joystick's buttons
		int firstValuatorIndex; // Index of joystick's first axis in device state array
		int numValuators; // Number of joystick's axes
		Vrui::InputDevice* device; // Pointer to Vrui input device associated with the joystick
		};
	
	#ifdef __DARWIN__
	struct ElementKey // Structure to map (device, element cookie) pairs to button or valuator indices
		{
		/* Elements: */
		public:
		void* device; // Pointer to HID device object
		IOHIDElementCookie cookie; // Cookie for the element
		
		/* Constructors and destructors: */
		ElementKey(void* sDevice,IOHIDElementCookie sCookie)
			:device(sDevice),cookie(sCookie)
			{
			}
		
		/* Methods: */
		friend bool operator!=(const ElementKey& key1,const ElementKey& key2)
			{
			return key1.device!=key2.device||key1.cookie!=key2.cookie;
			}
		static size_t hash(const ElementKey& source,size_t tableSize)
			{
			return (reinterpret_cast<size_t>(source.device)+size_t(source.cookie))%tableSize;
			}
		};
	
	struct ElementDescriptor // Structure describing how to convert a HIDValue into a button or a valuator
		{
		/* Embedded classes: */
		public:
		enum ElementType // Enumerated type for element types
			{
			BUTTON,VALUATOR,HATSWITCH
			};
		
		typedef Math::BrokenLine<double> AxisMapper; // Type for axis mappers
		
		/* Elements: */
		public:
		ElementType elementType; // Type of this element; hat switch is a special case generating two valuators
		int index; // Element's index in button or valuator state array
		int hsMin,hsMax; // Min and max values for hat switches
		AxisMapper axisMapper; // Axis mapper for a valuator element
		
		/* Constructors and destructors: */
		ElementDescriptor(void)
			:hsMin(0),hsMax(0),axisMapper(0.0,0.0,0.0,0.0)
			{
			}
		};
	
	typedef Misc::HashTable<ElementKey,ElementDescriptor,ElementKey> ElementMap; // Type for hash tables mapping elements to element descriptors
	#endif
	
	/* Elements: */
	private:
	std::vector<Device> devices; // List of joystick devices
	Threads::Mutex deviceStateMutex; // Mutex protecting the device state array
	bool* buttonStates; // Button state array
	double* valuatorStates; // Valuator state array
	#ifdef __DARWIN__
	CFAutoRelease<IOHIDManagerRef> hidManager; // HID manager object
	ElementMap elementMap; // Hash table for elements
	#endif
	Threads::Thread devicePollingThread; // Thread polling the event files of all joystick devices
	
	/* Protected methods from InputDeviceAdapter: */
	virtual void createInputDevice(int deviceIndex,const Misc::ConfigurationFileSection& configFileSection);
	
	/* New private methods: */
	#ifdef __DARWIN__
	static void hidDeviceValueChangedCallbackWrapper(void* context,IOReturn result,void* device,IOHIDValueRef newValue)
		{
		static_cast<InputDeviceAdapterJoystick*>(context)->hidDeviceValueChangedCallback(result,device,newValue);
		}
	void hidDeviceValueChangedCallback(IOReturn result,void* device,IOHIDValueRef newValue); // Callback method when an element on a monitored HID device changes value
	#endif
	void* devicePollingThreadMethod(void); // Method polling the event files of all joystick devices
	
	/* Constructors and destructors: */
	public:
	InputDeviceAdapterJoystick(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection); // Creates adapter connected to a single joystick device
	virtual ~InputDeviceAdapterJoystick(void);
	
	/* Methods from InputDeviceAdapter: */
	virtual void updateInputDevices(void);
	};

}

#endif
