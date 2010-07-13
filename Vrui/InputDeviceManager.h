/***********************************************************************
InputDeviceManager - Class to manage physical and virtual input devices,
tools associated to input devices, and the input device update graph.
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

#ifndef VRUI_INPUTDEVICEMANAGER_INCLUDED
#define VRUI_INPUTDEVICEMANAGER_INCLUDED

#include <list>
#include <vector>
#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <Vrui/InputDevice.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
namespace Vrui {
class InputGraphManager;
class InputDeviceAdapter;
}

namespace Vrui {

class InputDeviceManager
	{
	/* Embedded classes: */
	public:
	class InputDeviceCreationCallbackData:public Misc::CallbackData // Callback data sent when an input device is created
		{
		/* Elements: */
		public:
		InputDevice* inputDevice; // Pointer to newly created input device
		
		/* Constructors and destructors: */
		InputDeviceCreationCallbackData(InputDevice* sInputDevice)
			:inputDevice(sInputDevice)
			{
			}
		};
	
	class InputDeviceDestructionCallbackData:public Misc::CallbackData // Callback data sent when an input device is destroyed
		{
		/* Elements: */
		public:
		InputDevice* inputDevice; // Pointer to input device to be destroyed
		
		/* Constructors and destructors: */
		InputDeviceDestructionCallbackData(InputDevice* sInputDevice)
			:inputDevice(sInputDevice)
			{
			}
		};
	
	private:
	typedef std::list<InputDevice> InputDevices;
	
	/* Elements: */
	private:
	InputGraphManager* inputGraphManager; // Pointer to the input graph manager
	int numInputDeviceAdapters; // Number of input device adapters managed by the input device manager
	InputDeviceAdapter** inputDeviceAdapters; // Array of pointers to managed input device adapters
	InputDevices inputDevices; // List of all created input devices
	Misc::CallbackList inputDeviceCreationCallbacks; // List of callbacks to be called after a new input device has been created
	Misc::CallbackList inputDeviceDestructionCallbacks; // List of callbacks to be called before an input device will be destroyed
	
	/* Constructors and destructors: */
	public:
	InputDeviceManager(InputGraphManager* sInputGraphManager);
	private:
	InputDeviceManager(const InputDeviceManager& source); // Prohibit copy constructor
	InputDeviceManager& operator=(const InputDeviceManager& source); // Prohibit assignment operator
	public:
	~InputDeviceManager(void);
	
	/* Methods: */
	void initialize(const Misc::ConfigurationFileSection& configFileSection); // Creates all input device adapters listed in the configuration file section
	int getNumInputDeviceAdapters(void) const // Returns number of input device adapters
		{
		return numInputDeviceAdapters;
		}
	InputDeviceAdapter* getInputDeviceAdapter(int inputDeviceAdapterIndex) // Returns pointer to an input device adapter
		{
		return inputDeviceAdapters[inputDeviceAdapterIndex];
		}
	InputDeviceAdapter* findInputDeviceAdapter(InputDevice* device) const; // Returns pointer to the input device adapter owning the given device (or 0)
	InputGraphManager* getInputGraphManager(void) const
		{
		return inputGraphManager;
		}
	InputDevice* createInputDevice(const char* deviceName,int trackType,int numButtons,int numValuators,bool physicalDevice =false);
	int getNumInputDevices(void) const
		{
		return inputDevices.size();
		}
	InputDevice* getInputDevice(int deviceIndex);
	InputDevice* findInputDevice(const char* deviceName);
	void destroyInputDevice(InputDevice* device);
	void updateInputDevices(void);
	Misc::CallbackList& getInputDeviceCreationCallbacks(void) // Returns list of input device creation callbacks
		{
		return inputDeviceCreationCallbacks;
		}
	Misc::CallbackList& getInputDeviceDestructionCallbacks(void) // Returns list of input device creation callbacks
		{
		return inputDeviceDestructionCallbacks;
		}
	};

}

#endif
