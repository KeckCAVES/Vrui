/***********************************************************************
OpenVRHost - Class to wrap a low-level OpenVR tracking and display
device driver in a VRDevice.
Copyright (c) 2016-2018 Oliver Kreylos

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
#include <vector>
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

class OpenVRHost:public VRDevice,public vr::IVRSettings,public vr::IVRDriverContext,public vr::IVRProperties,public vr::IVRDriverInput,public vr::IVRDriverLog,public vr::IVRServerDriverHost,public vr::IVRResources,public vr::IVRDriverManager
	{
	/* Embedded classes: */
	private:
	typedef Vrui::VRDeviceState::TrackerState TrackerState; // Type for tracker states
	typedef TrackerState::PositionOrientation PositionOrientation; // Type for tracker position/orientation
	
	enum DeviceTypes // Enumerated type identifying supported OpenVR device types
		{
		HMD=0,Controller,Tracker,BaseStation,NumDeviceTypes
		};
	
	struct DeviceConfiguration // Structure defining settings shared by devices of the same type
		{
		/* Elements: */
		public:
		std::string nameTemplate; // Template to generate default names for devices of this type; can contain up to one %u conversion
		bool haveTracker; // Flag whether devices of this type are tracked
		unsigned int numButtons; // Number of buttons on devices of this type
		std::vector<std::string> buttonNames; // List of names of buttons on devices of this type
		unsigned int numValuators; // Number of valuators on devices of this type
		std::vector<std::string> valuatorNames; // List of names of valuators on devices of this type
		unsigned int numHapticFeatures; // Number of haptic features on devices of this type
		std::vector<std::string> hapticFeatureNames; // List of names of haptic features on devices of this type
		unsigned int numPowerFeatures; // Number of power features on devices of this type
		};
	
	struct DeviceState // Structure describing the current state of a tracked device
		{
		/*******************************************************************
		DeviceState structures are assigned to tracked devices in the order
		that they are reported by the low-level OpenVR driver. The HMD is
		usually the first reported device, but the other devices can come in
		any order.
		
		DeviceState structures are mapped to VRDeviceManager state objects
		via several indices, specifically trackerIndex and hmdConfiguration.
		*******************************************************************/
		
		/* Elements: */
		public:
		DeviceTypes deviceType; // Device's type
		std::string serialNumber; // Device's serial number
		vr::ITrackedDeviceServerDriver* driver; // Pointer to the driver interface for this tracked device
		vr::IVRDisplayComponent* display; // Pointer to the device's display component if it is an HMD
		int trackerIndex; // Index of this device's tracker; -1 if no tracker associated, i.e., for tracking base stations
		
		/* Device state reported by OpenVR: */
		bool willDriftInYaw; // Flag if the device does not have external orientation drift correction and will therefore drift in yaw over time
		bool isWireless; // Flag if the device is wireless
		bool hasProximitySensor; // Flag if device has face detector (only for HMDs)
		bool providesBatteryStatus; // Master flag if the device is battery-powered
		bool canPowerOff; // Flag if the device can turn on and off
		
		/* Configured device state: */
		PositionOrientation worldTransform; // Device world transformation as reported by the driver
		PositionOrientation localTransform; // Device local transformation as reported by the driver; baked into trackerPostTransformation
		unsigned int virtualDeviceIndex; // Index of the virtual device representing this tracked device in the VR device manager
		
		/* Current device state: */
		float lensCenters[2][2]; // Left and right lens centers relative to their respective screens (only for HMDs)
		Vrui::BatteryState batteryState; // Device's current battery state (only for battery-powered devices)
		bool proximitySensorState; // Current state of face detector (only for HMDs)
		Vrui::HMDConfiguration* hmdConfiguration; // Pointer to a configuration object if this device is an HMD
		unsigned int nextButtonIndex; // Next button index to be used during device initialization
		unsigned int numButtons; // Number of buttons already created for this device state
		unsigned int nextValuatorIndex; // Next button index to be used during device initialization
		unsigned int numValuators; // Number of valuators already created for this device state
		unsigned int nextHapticFeatureIndex; // Next haptic feature index to be used during device initialization
		unsigned int numHapticFeatures; // Number of haptic features already created for this device state
		bool connected; // Flag whether the device is currently connected
		bool tracked; // Flag whether the device is currently tracked
		
		/* Constructors and destructors: */
		DeviceState(void);
		};
	
	struct HapticEvent // Structure to store pending haptic events
		{
		/* Elements: */
		public:
		vr::PropertyContainerHandle_t containerHandle; // Handle of the container to which the haptic component belongs
		vr::VRInputComponentHandle_t componentHandle; // Handle of the haptic component
		bool pending; // Flag whether a haptic event is currently pending
		float duration; // Duration of haptic pulse in seconds
		float frequency; // Frequency of haptic pulse
		float amplitude; // Amplitude of haptic pulse
		};
	
	/* Elements: */
	
	/* Low-level OpenVR driver configuration: */
	std::string openvrRootDir; // Root directory containing the OpenVR installation
	std::string openvrDriverRootDir; // Root directory containing the OpenVR tracking driver
	void* openvrDriverDsoHandle; // Handle for the dynamic shared object containing the OpenVR tracking driver
	vr::IServerTrackedDeviceProvider* openvrTrackedDeviceProvider; // Pointer to the tracked device provider, i.e., the tracking driver object
	
	/* OpenVRHost driver module configuration: */
	Misc::ConfigurationFileSection openvrSettingsSection; // Configuration file section containing driver settings
	std::string openvrDriverConfigDir; // Configuration file directory to be used by the OpenVR tracking driver
	vr::DriverHandle_t driverHandle; // Container handle to be used for the driver itself
	vr::PropertyContainerHandle_t deviceHandleBase; // Base container handle for tracked devices
	bool printLogMessages; // Flag whether log messages from the OpenVR driver will be printed
	unsigned int threadWaitTime; // Number of microseconds to sleep in the device thread
	volatile bool exiting; // Flag to indicate driver module shutdown
	
	/* Tracked device configuration: */
	DeviceConfiguration deviceConfigurations[NumDeviceTypes]; // Array of device type descriptors
	unsigned int maxNumDevices[NumDeviceTypes+1]; // Maximum number of devices per device type; last entry is total maximum number of devices
	TrackerPostTransformation* configuredPostTransformations; // Configured post-transformations for all trackers
	unsigned int numHapticFeatures; // Number of haptic features
	
	/* Current tracked device states: */
	DeviceState* deviceStates; // Array of device states for potentially connected devices
	unsigned int* virtualDeviceIndices[NumDeviceTypes]; // Indices of virtual devices assigned to OpenVR device states of each device type
	unsigned int numConnectedDevices[NumDeviceTypes+1]; // Number of devices of each device type currently connected; last entry is total number
	HapticEvent* hapticEvents; // List of pending haptic events for all haptic features
	DeviceState** powerFeatureDevices; // Pointers to devices that can be powered off
	Vrui::HMDConfiguration* hmdConfiguration; // Pointer to the configuration object for the head-mounted display
	vr::VRInputComponentHandle_t nextComponentHandle; // Handle to be assigned to the next input component
	unsigned int* componentFeatureIndices; // Array mapping component handles to button or valuator indices
	
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
	
	/* Methods from vr::IVRDriverContext: */
	virtual void* GetGenericInterface(const char* pchInterfaceVersion,vr::EVRInitError* peError);
	virtual vr::DriverHandle_t GetDriverHandle(void);
	
	/* Methods from vr::IVRProperties: */
	virtual vr::ETrackedPropertyError ReadPropertyBatch(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyRead_t* pBatch,uint32_t unBatchEntryCount);
	virtual vr::ETrackedPropertyError WritePropertyBatch(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyWrite_t* pBatch,uint32_t unBatchEntryCount);
	virtual const char* GetPropErrorNameFromEnum(vr::ETrackedPropertyError error);
	virtual vr::PropertyContainerHandle_t TrackedDeviceToPropertyContainer(vr::TrackedDeviceIndex_t nDevice);
	
	/* Methods from vr::IVRDriverInput: */
	virtual vr::EVRInputError CreateBooleanComponent(vr::PropertyContainerHandle_t ulContainer,const char* pchName,vr::VRInputComponentHandle_t* pHandle);
	virtual vr::EVRInputError UpdateBooleanComponent(vr::VRInputComponentHandle_t ulComponent,bool bNewValue, double fTimeOffset);
	virtual vr::EVRInputError CreateScalarComponent(vr::PropertyContainerHandle_t ulContainer,const char* pchName,vr::VRInputComponentHandle_t* pHandle,vr::EVRScalarType eType,vr::EVRScalarUnits eUnits);
	virtual vr::EVRInputError UpdateScalarComponent(vr::VRInputComponentHandle_t ulComponent,float fNewValue, double fTimeOffset);
	virtual vr::EVRInputError CreateHapticComponent(vr::PropertyContainerHandle_t ulContainer,const char* pchName,vr::VRInputComponentHandle_t* pHandle);
	
	/* Methods from vr::IVRDriverLog: */
	virtual void Log(const char* pchLogMessage);
	
	/* Methods from vr::IVRServerDriverHost: */
	virtual bool TrackedDeviceAdded(const char* pchDeviceSerialNumber,vr::ETrackedDeviceClass eDeviceClass,vr::ITrackedDeviceServerDriver* pDriver);
	virtual void TrackedDevicePoseUpdated(uint32_t unWhichDevice,const vr::DriverPose_t& newPose,uint32_t unPoseStructSize);
	virtual void VsyncEvent(double vsyncTimeOffsetSeconds);
	virtual void VendorSpecificEvent(uint32_t unWhichDevice,vr::EVREventType eventType,const vr::VREvent_Data_t& eventData,double eventTimeOffset);
	virtual bool IsExiting(void);
	virtual bool PollNextEvent(vr::VREvent_t* pEvent,uint32_t uncbVREvent);
	virtual void GetRawTrackedDevicePoses(float fPredictedSecondsFromNow,vr::TrackedDevicePose_t* pTrackedDevicePoseArray,uint32_t unTrackedDevicePoseArrayCount);
	virtual void TrackedDeviceDisplayTransformUpdated(uint32_t unWhichDevice,vr::HmdMatrix34_t eyeToHeadLeft,vr::HmdMatrix34_t eyeToHeadRight);
	
	/* Methods from vr::IVRResources: */
	virtual uint32_t LoadSharedResource(const char* pchResourceName,char* pchBuffer,uint32_t unBufferLen);
	virtual uint32_t GetResourceFullPath(const char* pchResourceName,const char* pchResourceTypeDirectory,char* pchPathBuffer,uint32_t unBufferLen);
	
	/* Methods from vr::IVRDriverManager: */
	virtual uint32_t GetDriverCount(void) const;
	virtual uint32_t GetDriverName(vr::DriverId_t nDriver,char* pchValue,uint32_t unBufferSize);
	virtual vr::DriverHandle_t GetDriverHandle(const char *pchDriverName);
	};

#endif
