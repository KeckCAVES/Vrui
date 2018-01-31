/***********************************************************************
VRDeviceManager - Class to gather position, button and valuator data
from one or several VR devices and associate them with logical input
devices.
Copyright (c) 2002-2017 Oliver Kreylos

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

#ifndef VRDEVICEMANAGER_INCLUDED
#define VRDEVICEMANAGER_INCLUDED

#include <string>
#include <Realtime/Time.h>
#include <Threads/Mutex.h>
#include <Threads/MutexCond.h>
#include <Vrui/Internal/VRDeviceState.h>
#include <Vrui/Internal/BatteryState.h>

#include <VRDeviceDaemon/VRFactoryManager.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFile;
}
namespace Vrui {
class VRDeviceDescriptor;
class HMDConfiguration;
}
class VRDevice;
class VRCalibrator;

class VRDeviceManager
	{
	/* Embedded classes: */
	public:
	class DeviceFactoryManager:public VRFactoryManager<VRDevice>
		{
		/* Elements: */
		private:
		VRDeviceManager* deviceManager; // Pointer to device manager "owning" this factory manager
		
		/* Constructors and destructors: */
		public:
		DeviceFactoryManager(const std::string& sDsoPath,VRDeviceManager* sDeviceManager)
			:VRFactoryManager<VRDevice>(sDsoPath),
			 deviceManager(sDeviceManager)
			{
			};
		
		/* Methods: */
		VRDeviceManager* getDeviceManager(void) const // Returns pointer to device manager
			{
			return deviceManager;
			};
		};
	
	typedef VRFactoryManager<VRCalibrator> CalibratorFactoryManager;
	
	typedef void (*TrackerUpdateCompleteCallback)(VRDeviceManager* manager,void* userData); // Callback called when all trackers have updated
	typedef void (*BatteryStateUpdatedCallback)(VRDeviceManager* manager,unsigned int deviceIndex,const Vrui::BatteryState& batteryState,void* userData); // Callback called when a device's battery state changes
	typedef void (*HMDConfigurationUpdatedCallback)(VRDeviceManager* manager,const Vrui::HMDConfiguration* hmdConfiguration,void* userData); // Callback called when an HMD configuration has changed
	
	struct Feature // Structure describing a client-controlled feature managed by a device driver module
		{
		/* Elements: */
		public:
		VRDevice* device; // Pointer to device driver module owning the power feature
		int deviceFeatureIndex; // Index of the power feature on the owning device driver module
		};
	
	/* Elements: */
	private:
	DeviceFactoryManager deviceFactories; // Factory manager to load VR device classes
	CalibratorFactoryManager calibratorFactories; // Factory manager to load VR calibrator classes
	int numDevices; // Number of managed devices
	VRDevice** devices; // Array of pointers to VR devices
	int* trackerIndexBases; // Array of base tracker indices for each VR device
	int* buttonIndexBases; // Array of base button indices for each VR device
	int* valuatorIndexBases; // Array of base valuator indices for each VR device
	int currentDeviceIndex; // Index of currently constructed device during initialization
	std::vector<std::string> trackerNames; // List of tracker names
	std::vector<std::string> buttonNames; // List of button names
	std::vector<std::string> valuatorNames; // List of valuator names
	Threads::Mutex stateMutex; // Mutex serializing access to all state elements
	Vrui::VRDeviceState state; // Current state of all managed devices
	std::vector<Vrui::VRDeviceDescriptor*> virtualDevices; // List of virtual devices combining selected trackers, buttons, and valuators
	Threads::Mutex batteryStateMutex; // Mutex serializing access to the list of virtual device battery states
	std::vector<Vrui::BatteryState> batteryStates; // List of current battery states for all virtual devices
	BatteryStateUpdatedCallback batteryStateUpdatedCallback; // Callback called when a virtual device's battery state has been updated
	void* batteryStateUpdatedCallbackData; // Data passed with battery state updated callback
	Threads::Mutex hmdConfigurationMutex; // Mutex serializing access to the list of HMD configurations
	std::vector<Vrui::HMDConfiguration*> hmdConfigurations; // List of HMD configurations
	HMDConfigurationUpdatedCallback hmdConfigurationUpdatedCallback; // Callback called when an HMD configuration has been updated
	void* hmdConfigurationUpdatedCallbackData; // Data passed with HMD configuration updated callback
	std::vector<Feature> powerFeatures; // List of device parts that can be powered off on request
	std::vector<Feature> hapticFeatures; // List of haptic feedback devices
	unsigned int fullTrackerReportMask; // Bitmask containing 1-bits for all used logical tracker indices
	unsigned int trackerReportMask; // Bitmask of logical tracker indices that have reported state
	bool trackerUpdateNotificationEnabled; // Flag if update notification is enabled
	Threads::MutexCond* trackerUpdateCompleteCond; // Condition variable to notify client threads that all tracker states has been updated
	TrackerUpdateCompleteCallback trackerUpdateCompleteCallback; // Callback called when all tracker states have been updated
	void* trackerUpdateCompleteCallbackData; // Data passed with tracker update complete callback
	
	/* Constructors and destructors: */
	public:
	VRDeviceManager(Misc::ConfigurationFile& configFile); // Creates device manager by reading current section of configuration file
	~VRDeviceManager(void);
	
	/* Methods to communicate with device driver modules during initialization: */
	int getTrackerIndexBase(void) const // Returns the tracker index base for the currently constructed device
		{
		return trackerIndexBases[currentDeviceIndex];
		}
	int getButtonIndexBase(void) const // Returns the button index base for the currently constructed device
		{
		return buttonIndexBases[currentDeviceIndex];
		}
	int getValuatorIndexBase(void) const // Returns the valuator index base for the currently constructed device
		{
		return valuatorIndexBases[currentDeviceIndex];
		}
	int addTracker(const char* name =0); // Adds a new tracker to the manager's namespace; returns tracker index
	int addButton(const char* name =0); // Adds a new button to the manager's namespace; returns button index
	int addValuator(const char* name =0); // Adds a new valuator to the manager's namespace; returns valuator index
	VRCalibrator* createCalibrator(const std::string& calibratorType,Misc::ConfigurationFile& configFile); // Loads calibrator of given type from current section in configuration file
	unsigned int addVirtualDevice(Vrui::VRDeviceDescriptor* newVirtualDevice); // Adds a virtual device; is adopted by device manager; returns new virtual device index
	Vrui::HMDConfiguration* addHmdConfiguration(void); // Adds a new HMD configuration
	int addPowerFeature(VRDevice* device,int deviceFeatureIndex); // Adds a new power feature; returns feature index
	int addHapticFeature(VRDevice* device,int deviceFeatureIndex); // Adds a new power feature; returns feature index
	
	/* Methods to communicate with device driver modules during operation: */
	static Vrui::VRDeviceState::TimeStamp getTimeStamp(void) // Returns a time stamp for the current time
		{
		/* Sample the monotonic time source: */
		Realtime::TimePointMonotonic now;
		
		/* Convert time point to a periodic time stamp with microsecond resolution: */
		return Vrui::VRDeviceState::TimeStamp(now.tv_sec*1000000+(now.tv_nsec+500)/1000);
		}
	static Vrui::VRDeviceState::TimeStamp getTimeStamp(double offset) // Returns a time stamp offset from the current time by the given amount in seconds (positive is in the future)
		{
		/* Sample the monotonic time source: */
		Realtime::TimePointMonotonic now;
		
		/* Convert the offset to nanoseconds: */
		long nsecOffset=long(Math::floor(offset*1.0e9+0.5));
		
		/* Convert offset time point to a periodic time stamp with microsecond resolution: */
		return Vrui::VRDeviceState::TimeStamp(now.tv_sec*1000000+(now.tv_nsec+nsecOffset+500)/1000);
		}
	void disableTracker(int trackerIndex); // Sets the given tracker's tracking state to invalid
	void setTrackerState(int trackerIndex,const Vrui::VRDeviceState::TrackerState& newTrackerState,Vrui::VRDeviceState::TimeStamp newTimeStamp); // Updates state of single tracker
	void setButtonState(int buttonIndex,Vrui::VRDeviceState::ButtonState newButtonState); // Updates state of single button
	void setValuatorState(int valuatorIndex,Vrui::VRDeviceState::ValuatorState newValuatorState); // Updates state of single valuator
	void updateState(void); // Tells device manager that the current state should be considered "complete"
	void updateBatteryState(unsigned int virtualDeviceIndex,const Vrui::BatteryState& newBatteryState); // Updates the battery state of the given virtual device with the given new state
	void updateHmdConfiguration(const Vrui::HMDConfiguration* hmdConfiguration); // Tells device manager that the given HMD configuration was updated; must be called with HMD configurations locked
	
	/* Methods to communicate with device server: */
	int getNumVirtualDevices(void) const // Returns the number of managed virtual input devices
		{
		return int(virtualDevices.size());
		}
	const Vrui::VRDeviceDescriptor& getVirtualDevice(int deviceIndex) const // Returns the virtual input device of the given index
		{
		return *(virtualDevices[deviceIndex]);
		}
	void lockState(void) // Locks current device states
		{
		stateMutex.lock();
		};
	void unlockState(void) // Unlocks current device states
		{
		stateMutex.unlock();
		};
	Vrui::VRDeviceState& getState(void) // Returns current state of all managed devices (state must be locked while being used)
		{
		return state;
		};
	void lockBatteryStates(void) // Locks current battery states
		{
		hmdConfigurationMutex.lock();
		};
	void unlockBatteryStates(void) // Unlocks current battery states
		{
		hmdConfigurationMutex.unlock();
		};
	Vrui::BatteryState& getBatteryState(unsigned int deviceIndex) // Returns the current battery state of the given virtual device (battery states must be locked while being used)
		{
		return batteryStates[deviceIndex];
		}
	unsigned int getNumHmdConfigurations(void) const // Returns the number of HMD configurations
		{
		return hmdConfigurations.size();
		}
	void lockHmdConfigurations(void) // Locks current HMD configurations
		{
		hmdConfigurationMutex.lock();
		};
	void unlockHmdConfigurations(void) // Unlocks current HMD configurations
		{
		hmdConfigurationMutex.unlock();
		};
	Vrui::HMDConfiguration& getHmdConfiguration(unsigned int index) // Returns current HMD configuration of given index (HMD configurations must be locked while being used)
		{
		return *hmdConfigurations[index];
		};
	unsigned int getNumPowerFeatures(void) const // Returns the number of power features
		{
		return powerFeatures.size();
		}
	void powerOff(unsigned int powerFeatureIndex); // Requests to power off the given power feature
	unsigned int getNumHapticFeatures(void) const // Returns the number of haptic features
		{
		return hapticFeatures.size();
		}
	void hapticTick(unsigned int hapticFeatureIndex,unsigned int duration); // Requests a haptic tick of the given duration in microseconds on the given power feature
	void enableTrackerUpdateNotification(Threads::MutexCond* sTrackerUpdateCompleteCond); // Sets a condition variable to be signalled when all trackers have updated
	void enableTrackerUpdateNotification(TrackerUpdateCompleteCallback newTrackerUpdateCompleteCallback,void* newTrackerUpdateCompleteCallbackData); // Sets a callback to be called when all trackers have updated; callback is called with device states locked
	void disableTrackerUpdateNotification(void); // Disables tracker update notification
	void setBatteryStateUpdatedCallback(BatteryStateUpdatedCallback newBatteryStateUpdatedCallback,void* newBatteryStateUpdatedCallbackData); // Sets a callback to be called when a virtual device's battery state is updated
	void setHmdConfigurationUpdatedCallback(HMDConfigurationUpdatedCallback newHmdConfigurationUpdatedCallback,void* newHmdConfigurationUpdatedCallbackData); // Sets a callback to be called when an HMD configuration is updated
	void start(void); // Starts device processing
	void stop(void); // Stops device processing
	};

#endif
