/***********************************************************************
InputDeviceAdapterDummy - Class to create "dummy" devices to simulate
behavior of non-existent devices.
Copyright (c) 2015-2016 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_INPUTDEVICEADAPTERDUMMY_INCLUDED
#define VRUI_INTERNAL_INPUTDEVICEADAPTERDUMMY_INCLUDED

#include <string>
#include <vector>
#include <Vrui/Internal/InputDeviceAdapter.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}

namespace Vrui {

class InputDeviceAdapterDummy:public InputDeviceAdapter
	{
	/* Elements: */
	private:
	std::vector<std::string> buttonNames; // Array of button names for all defined input devices
	std::vector<std::string> valuatorNames; // Array of valuator names for all defined input devices
	
	/* Protected methods from InputDeviceAdapter: */
	virtual void createInputDevice(int deviceIndex,const Misc::ConfigurationFileSection& configFileSection);
	
	/* Constructors and destructors: */
	public:
	InputDeviceAdapterDummy(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection);
	virtual ~InputDeviceAdapterDummy(void);
	
	/* Methods: */
	virtual std::string getFeatureName(const InputDeviceFeature& feature) const;
	virtual int getFeatureIndex(InputDevice* device,const char* featureName) const;
	virtual void updateInputDevices(void);
	virtual TrackerState peekTrackerState(int deviceIndex);
	};

}

#endif
