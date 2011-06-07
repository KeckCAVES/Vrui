/***********************************************************************
InputDeviceDataSaver - Class to save input device data to a file for
later playback.
Copyright (c) 2004-2011 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_INPUTDEVICEDATASAVER_INCLUDED
#define VRUI_INTERNAL_INPUTDEVICEDATASAVER_INCLUDED

#include <string>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
namespace IO {
class File;
}
namespace Sound {
class SoundRecorder;
}
namespace Vrui {
class InputDevice;
class InputDeviceManager;
#ifdef VRUI_INPUTDEVICEDATASAVER_USE_KINECT
class KinectRecorder;
#endif
}

namespace Vrui {

class InputDeviceDataSaver
	{
	/* Elements: */
	private:
	IO::File* inputDeviceDataFile; // File input device data is saved to
	int numInputDevices; // Number of saved (physical) input devices
	InputDevice** inputDevices; // Array of pointers to saved input devices
	Sound::SoundRecorder* soundRecorder; // Pointer to sound recorder object to record commentary tracks
	#ifdef VRUI_INPUTDEVICEDATASAVER_USE_KINECT
	KinectRecorder* kinectRecorder; // Pointer to 3D video recorder object
	#endif
	bool firstFrame; // Flag to identify the first frame of input device data
	
	/* Private methods: */
	static std::string getInputDeviceDataFileName(const Misc::ConfigurationFileSection& configFileSection);
	
	/* Constructors and destructors: */
	public:
	InputDeviceDataSaver(const Misc::ConfigurationFileSection& configFileSection,InputDeviceManager& inputDeviceManager,unsigned int randomSeed); // Creates an object saving all devices currently in the manager
	~InputDeviceDataSaver(void);
	
	/* Methods: */
	void saveCurrentState(double currentTimeStamp); // Saves current state of input devices
	};

}

#endif
