/***********************************************************************
OpenVRHost - Class to wrap a low-level OpenVR tracking and display
device driver in a VRDevice.
Copyright (c) 2016-2017 Oliver Kreylos

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

#include <string>
#include <Misc/ConfigurationFile.h>
#include <Vrui/Internal/BatteryState.h>
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

class OpenVRHost:public VRDevice,public vr::IVRDriverContext,public vr::IVRServerDriverHost,public vr::IVRProperties,public vr::IVRSettings,public vr::IVRDriverLog,public vr::IVRResources,public vr::IVRDriverManager
	{
	/* Embedded classes: */
	private:
	typedef Vrui::VRDeviceState::TrackerState TrackerState; // Type for tracker states
	typedef TrackerState::PositionOrientation PositionOrientation; // Type for tracker position/orientation
	
	struct DeviceState // Structure describing the current state of a tracked device
		{
		/*******************************************************************
		DeviceState structures are assigned to tracked device in the order
		that they are reported by the low-level OpenVR driver. The HMD is
		usually the first reported device, but the other devices can come in
		any order.
		
		DeviceState structures are mapped to VRDeviceManager state objects
		via several indices, specifically trackerIndex and hmdConfiguration.
		*******************************************************************/
		
		/* Elements: */
		public:
		std::string serialNumber; // Device's serial number
		vr::ITrackedDeviceServerDriver* driver; // Pointer to the driver interface for this tracked device
		vr::IVRDisplayComponent* display; // Pointer to the device's display component if it is an HMD
		int trackerIndex; // Index of this device's tracker; -1 if no tracker associated, i.e., for tracking base stations
		int* buttonIndices; // Array mapping OpenVR button IDs to button indices
		int valuatorIndexBase; // Base index for this device's valuators
		
		/* Device state reported by OpenVR: */
		bool willDriftInYaw; // Flag if the device does not have external orientation drift correction
		bool isWireless; // Flag if the device is wireless
		bool hasProximitySensor; // Flag if device has face detector (only for HMDs)
		bool providesBatteryStatus; // Master flag if the device is battery-powered
		bool canPowerOff; // Flag if the device can turn on and off
		
		/* Configured device state: */
		PositionOrientation worldTransform; // Device world transformation as reported by the driver
		PositionOrientation localTransform; // Device local transformation as reported by the driver; baked into trackerPostTransformation
		unsigned int virtualDeviceIndex; // Index of the virtual device representing this tracked device in the VR device manager
		
		/* Current device state: */
		float lensCenters[2][2]; // Left and right lens centers relative to their respective screens
		Vrui::BatteryState batteryState; // Device's current battery state
		bool proximitySensorState; // Current state of face detector (only for HMDs)
		Vrui::HMDConfiguration* hmdConfiguration; // Pointer to a configuration object if this device is an HMD
		bool connected; // Flag whether the device is currently connected
		bool tracked; // Flag whether the device is currently tracked
		
		/* Constructors and destructors: */
		DeviceState(void);
		
		/* Methods: */
		void setNumButtons(int newNumButtons); // Allocates and initializes a button index array
		};
	
	/* Elements: */
	std::string openvrRootDir; // Root directory containing the OpenVR installation
	std::string openvrDriverRootDir; // Root directory containing the OpenVR tracking driver
	void* openvrDriverDsoHandle; // Handle for the dynamic shared object containing the OpenVR tracking driver
	vr::IServerTrackedDeviceProvider* openvrTrackedDeviceProvider; // Pointer to the tracked device provider, i.e., the tracking driver object
	Misc::ConfigurationFileSection openvrSettingsSection; // Configuration file section containing driver settings
	std::string openvrDriverConfigDir; // Configuration file directory to be used by the OpenVR tracking driver
	unsigned int threadWaitTime; // Number of microseconds to sleep in the device thread
	unsigned int distortionMeshSize[2]; // Number of vertices in distortion meshes for connected HMDs
	bool printLogMessages; // Flag whether log messages from the OpenVR driver will be printed
	unsigned int maxNumControllers; // Maximum number of supported controllers
	unsigned int maxNumTrackers; // Maximum number of supported generic trackers
	unsigned int maxNumBaseStations; // Maximum number of supported tracking base stations
	TrackerPostTransformation* configuredPostTransformations; // Array of originally configured tracker post-transformations for each tracked device
	unsigned int hmdDeviceIndex; // Virtual device index for the head-mounted display
	Vrui::HMDConfiguration* hmdConfiguration; // Pointer to the configuration object for the head-mounted display
	unsigned int* controllerDeviceIndices; // Array of virtual device indices for controllers
	unsigned int* trackerDeviceIndices; // Array of virtual device indices for generic trackers
	DeviceState* deviceStates; // Array of device states for potentially connected devices
	DeviceState** powerFeatureDevices; // Pointers to devices that can be powered off
	vr::IVRControllerComponent** controllers; // Array of connected controller components
	unsigned int numConnectedDevices; // Total number of device state structures currently populated
	unsigned int numControllers; // Number of currently connected controllers
	unsigned int numTrackers; // Number of currently connected generic trackers
	unsigned int numBaseStations; // Number of currently connected tracking base stations
	bool exiting; // Flag to indicate driver module shutdown
	
	/* Private methods: */
	void updateHMDConfiguration(DeviceState& deviceState) const; // If the device is an HMD, update its configuration
	
	/* Protected methods from VRDevice: */
	protected:
	virtual void deviceThreadMethod(void);
	
	/* Constructors and destructors: */
	public:
	OpenVRHost(VRDevice::Factory* sFactory,VRDeviceManager* sDeviceManager,Misc::ConfigurationFile& configFile);
	virtual ~OpenVRHost(void);
	
	/* Methods from VRDevice: */
	virtual void initialize(void);
	virtual void start(void);
	virtual void stop(void);
	virtual void powerOff(int devicePowerFeatureIndex);
	virtual void hapticTick(int deviceHapticFeatureIndex,unsigned int duration);
	
	/* Methods from vr::IVRDriverContext: */
	virtual void* GetGenericInterface(const char* pchInterfaceVersion,vr::EVRInitError* peError);
	virtual vr::DriverHandle_t GetDriverHandle();
	
	/* Methods from vr::IVRServerDriverHost: */
	virtual bool TrackedDeviceAdded(const char* pchDeviceSerialNumber,vr::ETrackedDeviceClass eDeviceClass,vr::ITrackedDeviceServerDriver* pDriver);
	virtual void TrackedDevicePoseUpdated(uint32_t unWhichDevice,const vr::DriverPose_t& newPose,uint32_t unPoseStructSize);
	virtual void VsyncEvent(double vsyncTimeOffsetSeconds);
	virtual void TrackedDeviceButtonPressed(uint32_t unWhichDevice,vr::EVRButtonId eButtonId,double eventTimeOffset);
	virtual void TrackedDeviceButtonUnpressed(uint32_t unWhichDevice,vr::EVRButtonId eButtonId,double eventTimeOffset);
	virtual void TrackedDeviceButtonTouched(uint32_t unWhichDevice,vr::EVRButtonId eButtonId,double eventTimeOffset);
	virtual void TrackedDeviceButtonUntouched(uint32_t unWhichDevice,vr::EVRButtonId eButtonId,double eventTimeOffset);
	virtual void TrackedDeviceAxisUpdated(uint32_t unWhichDevice,uint32_t unWhichAxis,const vr::VRControllerAxis_t& axisState);
	virtual void ProximitySensorState(uint32_t unWhichDevice,bool bProximitySensorTriggered);
	virtual void VendorSpecificEvent(uint32_t unWhichDevice,vr::EVREventType eventType,const vr::VREvent_Data_t& eventData,double eventTimeOffset);
	virtual bool IsExiting(void);
	virtual bool PollNextEvent(vr::VREvent_t* pEvent,uint32_t uncbVREvent);
	virtual void GetRawTrackedDevicePoses(float fPredictedSecondsFromNow,vr::TrackedDevicePose_t* pTrackedDevicePoseArray,uint32_t unTrackedDevicePoseArrayCount);
	
	/* Methods from vr::IVRProperties: */
	virtual vr::ETrackedPropertyError ReadPropertyBatch(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyRead_t* pBatch,uint32_t unBatchEntryCount);
	virtual vr::ETrackedPropertyError WritePropertyBatch(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyWrite_t* pBatch,uint32_t unBatchEntryCount);
	virtual const char* GetPropErrorNameFromEnum(vr::ETrackedPropertyError error);
	virtual vr::PropertyContainerHandle_t TrackedDeviceToPropertyContainer(vr::TrackedDeviceIndex_t nDevice);
	
	/* Methods from vr::IVRSettings: */
	virtual const char* GetSettingsErrorNameFromEnum(vr::EVRSettingsError eError);
	virtual bool Sync(bool bForce,vr::EVRSettingsError* peError);
	virtual void SetBool(const char* pchSection,const char* pchSettingsKey,bool bValue,vr::EVRSettingsError* peError);
	virtual void SetInt32(const char* pchSection,const char* pchSettingsKey,int32_t nValue,vr::EVRSettingsError* peError);
	virtual void SetFloat(const char* pchSection,const char* pchSettingsKey,float flValue,vr::EVRSettingsError* peError);
	virtual void SetString(const char* pchSection,const char* pchSettingsKey,const char* pchValue,vr::EVRSettingsError* peError);
	virtual bool GetBool(const char* pchSection,const char* pchSettingsKey,vr::EVRSettingsError* peError);
	virtual int32_t GetInt32(const char* pchSection,const char*pchSettingsKey,vr::EVRSettingsError* peError);
	virtual float GetFloat(const char* pchSection,const char* pchSettingsKey,vr::EVRSettingsError* peError);
	virtual void GetString(const char* pchSection,const char* pchSettingsKey,char* pchValue,uint32_t unValueLen,vr::EVRSettingsError* peError);
	virtual void RemoveSection(const char* pchSection,vr::EVRSettingsError* peError);
	virtual void RemoveKeyInSection(const char* pchSection,const char* pchSettingsKey,vr::EVRSettingsError* peError);
	
	/* Methods from vr::IVRDriverLog: */
	virtual void Log(const char* pchLogMessage);
	
	/* Methods from vr::IVRDriverManager: */
	virtual uint32_t GetDriverCount(void) const;
	virtual uint32_t GetDriverName(vr::DriverId_t nDriver,char* pchValue,uint32_t unBufferSize);
	
	/* Methods from vr::IVRResources: */
	virtual uint32_t LoadSharedResource(const char* pchResourceName,char* pchBuffer,uint32_t unBufferLen);
	virtual uint32_t GetResourceFullPath(const char* pchResourceName,const char* pchResourceTypeDirectory,char* pchPathBuffer,uint32_t unBufferLen);
	};

#endif
