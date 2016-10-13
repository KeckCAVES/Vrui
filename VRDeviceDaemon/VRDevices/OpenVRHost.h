/***********************************************************************
OpenVRHost - Class to wrap a low-level OpenVR tracking and display
device driver in a VRDevice.
Copyright (c) 2016 Oliver Kreylos

This file is part of the Vrui VR Device Driver Daemon (VRDeviceDaemon).

The Vrui VR Device Driver Daemon is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vrui VR Device Driver Daemon is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui VR Device Driver Daemon; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef OPENVRHOST_INCLUDED
#define OPENVRHOST_INCLUDED

#include <Misc/Timer.h>
#include <Threads/Mutex.h>

#include <VRDeviceDaemon/VRDevice.h>

/* Disable a bunch of warnings and include the header file for OpenVR's internal driver interface: */
#define GNUC 1
#define nullptr 0
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wlong-long"
#include <openvr_driver.h>
#pragma GCC diagnostic pop

/* Forward declarations: */
namespace Vrui {
class HMDConfiguration;
}

class OpenVRHost:public VRDevice,public vr::IServerDriverHost
	{
	/* Embedded classes: */
	private:
	typedef Vrui::VRDeviceState::TrackerState TrackerState; // Type for tracker states
	typedef TrackerState::PositionOrientation PositionOrientation; // Type for tracker position/orientation
	
	struct DeviceState // Structure describing the current state of a tracked device
		{
		/* Elements: */
		public:
		vr::ITrackedDeviceServerDriver* driver; // Pointer to the driver interface for this tracked device
		TrackerPostTransformation configuredPostTransformation; // Originally configured tracker post-transformation
		bool connected; // Flag if this device is currently connected
		float batteryLevel; // Current battery level from 0.0-1.0; always 1.0 for plugged-in devices
		PositionOrientation worldTransform; // Device world transformation as reported by the driver
		PositionOrientation localTransform; // Device local transformation as reported by the driver; baked into trackerPostTransformation
		Vrui::HMDConfiguration* hmdConfiguration; // Pointer to a configuration object if this device is an HMD
		float ipd; // Currently configured inter-pupillary distance in meters
		
		/* Constructors and destructors: */
		DeviceState(void)
			:driver(0),connected(false),
			 batteryLevel(1.0f),
			 hmdConfiguration(0),ipd(0.0635f)
			{
			}
		};
	
	/* Elements: */
	void* openvrDriverDsoHandle; // Handle for the dynamic shared object containing the OpenVR tracking driver
	vr::IServerTrackedDeviceProvider* openvrServerInterface; // Pointer to the server-side interface of the driver contained in the dynamic shared object
	vr::IDriverLog* openvrLog; // An OpenVR-compatible logging facility
	vr::IVRSettings* openvrSettings; // An OpenVR-compatible settings manager
	unsigned int distortionMeshSize[2]; // Number of vertices in distortion meshes for connected HMDs
	DeviceState* deviceStates; // Array of device states for potentially connected devices
	unsigned int numConnectedDevices; // Number of currently connected tracked devices
	bool proximitySensor; // Current proximity sensor state
	unsigned int threadWaitTime; // Number of microseconds to sleep in the device thread
	
	/* Private methods: */
	void updateHMDConfiguration(DeviceState& deviceState) const; // If the device is an HMD, update its configuration
	
	/* Protected methods: */
	protected:
	virtual void deviceThreadMethod(void);
	
	/* Constructors and destructors: */
	public:
	OpenVRHost(VRDevice::Factory* sFactory,VRDeviceManager* sDeviceManager,Misc::ConfigurationFile& configFile);
	virtual ~OpenVRHost(void);
	
	/* Methods from VRDevice: */
	virtual void start(void);
	virtual void stop(void);
	
	/* Methods from vr::IServerDriverHost: */
	virtual bool TrackedDeviceAdded(const char* pchDeviceSerialNumber);
	virtual void TrackedDevicePoseUpdated(uint32_t unWhichDevice,const vr::DriverPose_t& newPose);
	virtual void TrackedDevicePropertiesChanged(uint32_t unWhichDevice);
	virtual void VsyncEvent(double vsyncTimeOffsetSeconds);
	virtual void TrackedDeviceButtonPressed(uint32_t unWhichDevice,vr::EVRButtonId eButtonId,double eventTimeOffset);
	virtual void TrackedDeviceButtonUnpressed(uint32_t unWhichDevice,vr::EVRButtonId eButtonId,double eventTimeOffset);
	virtual void TrackedDeviceButtonTouched(uint32_t unWhichDevice,vr::EVRButtonId eButtonId,double eventTimeOffset);
	virtual void TrackedDeviceButtonUntouched(uint32_t unWhichDevice,vr::EVRButtonId eButtonId,double eventTimeOffset);
	virtual void TrackedDeviceAxisUpdated(uint32_t unWhichDevice,uint32_t unWhichAxis,const vr::VRControllerAxis_t& axisState);
	virtual void MCImageUpdated(void);
	virtual vr::IVRSettings* GetSettings(const char* pchInterfaceVersion);
	virtual void PhysicalIpdSet(uint32_t unWhichDevice,float fPhysicalIpdMeters);
	virtual void ProximitySensorState(uint32_t unWhichDevice,bool bProximitySensorTriggered);
	virtual void VendorSpecificEvent(uint32_t unWhichDevice,vr::EVREventType eventType,const vr::VREvent_Data_t& eventData,double eventTimeOffset);
	virtual bool IsExiting(void);
	};

#endif
