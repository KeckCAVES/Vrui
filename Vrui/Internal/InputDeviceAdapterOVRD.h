/***********************************************************************
InputDeviceAdapterOVRD - Class to connect Oculus VR's Rift tracking
daemon to a Vrui application.
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

#ifndef VRUI_INTERNAL_INPUTDEVICEADAPTEROVRD_INCLUDED
#define VRUI_INTERNAL_INPUTDEVICEADAPTEROVRD_INCLUDED

#include <Misc/Autopointer.h>
#include <Threads/Thread.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/Geometry.h>
#include <Vrui/Internal/InputDeviceAdapter.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
namespace IO {
class FixedMemoryFile;
}
namespace Comm {
class TCPPipe;
}
namespace Vrui {
class SharedMemory;
}

namespace Vrui {

class InputDeviceAdapterOVRD:public InputDeviceAdapter
	{
	/* Elements: */
	private:
	Misc::Autopointer<Comm::TCPPipe> serverPipe; // TCP pipe connected to the OVR tracking daemon
	Misc::Autopointer<IO::FixedMemoryFile> messageBuffer; // Temporary message buffer to unblock protocol messages
	unsigned int hmdId; // Network ID of the tracked Oculus Rift HMD
	volatile bool keepListening; // Flag to shut down the listening thread
	Threads::Thread ovrdProtocolListenerThread; // Thread listening to additional protocol messages from the OVR tracking daemon
	SharedMemory* hmdMem; // Handle to OVR tracking daemon shared memory buffer containing HMD tracking data
	SharedMemory* camMem; // Handle to OVR tracking daemon shared memory buffer containing camera tracking data
	TrackerState cameraTransform; // Transformation from tracking camera space to Vrui physical space; i.e., position and orientation of camera in physical space
	TrackerState postTransform; // Post-transformation to adjust tracking origin and orientation within HMD's coordinate system
	
	/* Private methods: */
	void* ovrdProtocolListenerThreadMethod(void);
	
	/* Constructors and destructors: */
	public:
	InputDeviceAdapterOVRD(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection);
	virtual ~InputDeviceAdapterOVRD(void);
	
	/* Methods from InputDeviceAdapter: */
	virtual void updateInputDevices(void);
	virtual TrackerState peekTrackerState(int deviceIndex);
	};

}

#endif
