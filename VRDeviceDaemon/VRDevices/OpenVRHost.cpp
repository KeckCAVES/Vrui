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

#include <VRDeviceDaemon/VRDevices/OpenVRHost.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <dlfcn.h>
#include <Misc/SizedTypes.h>
#include <Misc/StringPrintf.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <IO/File.h>
#include <IO/OpenFile.h>
#include <Geometry/OutputOperators.h>
#include <Vrui/Internal/VRDeviceDescriptor.h>
#include <Vrui/Internal/HMDConfiguration.h>

#include <VRDeviceDaemon/VRDeviceManager.h>
#include <VRDeviceDaemon/VRDevices/OpenVRHost-Config.h>

/***********************************************************************
A fake implementation of SDL functions used by Valve's lighthouse
driver, to fool the driver into detecting a connected Vive HMD:
***********************************************************************/

/**********************************************
Inter-operability declarations from SDL2/SDL.h:
**********************************************/

extern "C" {

#ifndef DECLSPEC
	#if defined(__GNUC__) && __GNUC__ >= 4
		#define DECLSPEC __attribute__ ((visibility("default")))
	#else
		#define DECLSPEC
	#endif
#endif

#ifndef SDLCALL
#define SDLCALL
#endif

typedef struct
	{
	Misc::UInt32 format;
	int w;
	int h;
	int refresh_rate;
	void *driverdata;
	} SDL_DisplayMode;

typedef struct
	{
	int x,y;
	int w, h;
	} SDL_Rect;

DECLSPEC int SDLCALL SDL_GetNumVideoDisplays(void);
DECLSPEC int SDLCALL SDL_GetCurrentDisplayMode(int displayIndex,SDL_DisplayMode* mode);
DECLSPEC int SDLCALL SDL_GetDisplayBounds(int displayIndex,SDL_Rect* rect);
DECLSPEC const char* SDLCALL SDL_GetDisplayName(int displayIndex);

}

/******************
Fake SDL functions:
******************/

int SDL_GetNumVideoDisplays(void)
	{
	return 2; // Create two fake displays so the driver doesn't complain about the HMD being the primary
	}

int SDL_GetCurrentDisplayMode(int displayIndex,SDL_DisplayMode* mode)
	{
	memset(mode,0,sizeof(SDL_DisplayMode));
	mode->format=0x16161804U; // Hard-coded for SDL_PIXELFORMAT_RGB888
	if(displayIndex==1)
		{
		/* Return a fake Vive HMD: */
		mode->w=2160;
		mode->h=1200;
		mode->refresh_rate=89;
		mode->driverdata=0;
		}
	else
		{
		/* Return a fake monitor: */
		mode->w=1920;
		mode->h=1080;
		mode->refresh_rate=60;
		mode->driverdata=0;
		}
	
	return 0;
	}

int SDL_GetDisplayBounds(int displayIndex,SDL_Rect* rect)
	{
	if(displayIndex==1)
		{
		/* Return a fake Vive HMD: */
		rect->x=1920;
		rect->y=0;
		rect->w=2160;
		rect->h=1200;
		}
	else
		{
		/* Return a fake monitor: */
		rect->x=0;
		rect->y=0;
		rect->w=1920;
		rect->h=1080;
		}
	
	return 0;
	}

const char* SDL_GetDisplayName(int displayIndex)
	{
	if(displayIndex==1)
		return "HTC Vive 5\"";
	else
		return "Acme Inc. HD Display";
	}

/****************************************
Methods of class OpenVRHost::DeviceState:
****************************************/

OpenVRHost::DeviceState::DeviceState(void)
	:deviceType(NumDeviceTypes),
	 driver(0),display(0),
	 trackerIndex(-1),
	 willDriftInYaw(true),isWireless(false),hasProximitySensor(false),providesBatteryStatus(false),canPowerOff(false),
	 proximitySensorState(false),hmdConfiguration(0),
	 nextButtonIndex(0),numButtons(0),nextValuatorIndex(0),numValuators(0),nextHapticFeatureIndex(0),numHapticFeatures(0),
	 connected(false),tracked(false)
	{
	for(int i=0;i<2;++i)
		for(int j=0;j<2;++j)
			lensCenters[i][j]=0.5f;
	}

/***************************
Methods of class OpenVRHost:
***************************/

void OpenVRHost::updateHMDConfiguration(OpenVRHost::DeviceState& deviceState) const
	{
	deviceManager->lockHmdConfigurations();
	
	/* Update recommended pre-distortion render target size: */
	uint32_t renderTargetSize[2];
	deviceState.display->GetRecommendedRenderTargetSize(&renderTargetSize[0],&renderTargetSize[1]);
	deviceState.hmdConfiguration->setRenderTargetSize(renderTargetSize[0],renderTargetSize[1]);
	
	/* Update per-eye state: */
	bool distortionMeshesUpdated=false;
	for(int eyeIndex=0;eyeIndex<2;++eyeIndex)
		{
		vr::EVREye eye=eyeIndex==0?vr::Eye_Left:vr::Eye_Right;
		
		/* Update output viewport: */
		uint32_t viewport[4];
		deviceState.display->GetEyeOutputViewport(eye,&viewport[0],&viewport[1],&viewport[2],&viewport[3]);
		deviceState.hmdConfiguration->setViewport(eyeIndex,viewport[0],viewport[1],viewport[2],viewport[3]);
		
		/* Update tangent-space FoV boundaries: */
		float fov[4];
		deviceState.display->GetProjectionRaw(eye,&fov[0],&fov[1],&fov[2],&fov[3]);
		deviceState.hmdConfiguration->setFov(eyeIndex,fov[0],fov[1],fov[2],fov[3]);
		
		/* Evaluate and update lens distortion correction formula: */
		const Vrui::HMDConfiguration::UInt* dmSize=deviceState.hmdConfiguration->getDistortionMeshSize();
		Vrui::HMDConfiguration::DistortionMeshVertex* dmPtr=deviceState.hmdConfiguration->getDistortionMesh(eyeIndex);
		for(unsigned int v=0;v<dmSize[1];++v)
			{
			float vf=float(v)/float(dmSize[1]-1);
			for(unsigned int u=0;u<dmSize[0];++u,++dmPtr)
				{
				/* Calculate the vertex's undistorted positions: */
				float uf=float(u)/float(dmSize[0]-1);
				vr::DistortionCoordinates_t out=deviceState.display->ComputeDistortion(eye,uf,vf);
				Vrui::HMDConfiguration::Point2 red(out.rfRed);
				Vrui::HMDConfiguration::Point2 green(out.rfGreen);
				Vrui::HMDConfiguration::Point2 blue(out.rfBlue);
				
				/* Check if there is actually a change: */
				distortionMeshesUpdated=distortionMeshesUpdated||dmPtr->red!=red||dmPtr->green!=green||dmPtr->blue!=blue;
				dmPtr->red=red;
				dmPtr->green=green;
				dmPtr->blue=blue;
				}
			}
		}
	if(distortionMeshesUpdated)
		deviceState.hmdConfiguration->updateDistortionMeshes();
	
	/* Tell the device manager that the HMD configuration was updated: */
	deviceManager->updateHmdConfiguration(deviceState.hmdConfiguration);
	
	deviceManager->unlockHmdConfigurations();
	}

void OpenVRHost::deviceThreadMethod(void)
	{
	/* Run the OpenVR driver's main loop to dispatch events: */
	while(true)
		{
		openvrTrackedDeviceProvider->RunFrame();
		usleep(threadWaitTime);
		}
	}

namespace {

/****************
Helper functions:
****************/

std::string pathcat(const std::string& prefix,const std::string& suffix) // Concatenates two partial paths if the suffix is not absolute
	{
	/* Check if the path suffix is relative: */
	if(suffix.empty()||suffix[0]!='/')
		{
		/* Return the concatenation of the two paths: */
		std::string result;
		result.reserve(prefix.size()+1+suffix.size());
		
		result=prefix;
		result.push_back('/');
		result.append(suffix);
		
		return result;
		}
	else
		{
		/* Return the path suffix unchanged: */
		return suffix;
		}
	}

}

OpenVRHost::OpenVRHost(VRDevice::Factory* sFactory,VRDeviceManager* sDeviceManager,Misc::ConfigurationFile& configFile)
	:VRDevice(sFactory,sDeviceManager,configFile),
	 openvrDriverDsoHandle(0),
	 openvrTrackedDeviceProvider(0),
	 openvrSettingsSection(configFile.getSection("Settings")),
	 driverHandle(512),deviceHandleBase(256),
	 printLogMessages(configFile.retrieveValue<bool>("./printLogMessages",false)),
	 threadWaitTime(configFile.retrieveValue<unsigned int>("./threadWaitTime",100000U)),
	 exiting(false),
	 configuredPostTransformations(0),numHapticFeatures(0),
	 deviceStates(0),hapticEvents(0),powerFeatureDevices(0),hmdConfiguration(0),
	 nextComponentHandle(1),componentFeatureIndices(0)
	{
	/*********************************************************************
	First initialization step: Dynamically load the appropriate OpenVR
	driver shared library.
	*********************************************************************/
	
	/* Retrieve the Steam root directory: */
	std::string steamRootDir;
	if(strncmp(VRDEVICEDAEMON_CONFIG_OPENVRHOST_STEAMDIR,"$HOME/",6)==0)
		{
		const char* homeDir=getenv("HOME");
		if(homeDir!=0)
			steamRootDir=homeDir;
		steamRootDir.append(VRDEVICEDAEMON_CONFIG_OPENVRHOST_STEAMDIR+5);
		}
	else
		steamRootDir=VRDEVICEDAEMON_CONFIG_OPENVRHOST_STEAMDIR;
	steamRootDir=configFile.retrieveString("./steamRootDir",steamRootDir);
	
	/* Construct the OpenVR root directory: */
	openvrRootDir=VRDEVICEDAEMON_CONFIG_OPENVRHOST_STEAMVRDIR;
	openvrRootDir=configFile.retrieveString("./openvrRootDir",openvrRootDir);
	openvrRootDir=pathcat(steamRootDir,openvrRootDir);
	
	/* Retrieve the name of the OpenVR device driver: */
	std::string openvrDriverName=configFile.retrieveString("./openvrDriverName","lighthouse");
	
	/* Retrieve the directory containing the OpenVR device driver: */
	openvrDriverRootDir=VRDEVICEDAEMON_CONFIG_OPENVRHOST_STEAMVRDIR;
	openvrDriverRootDir.append("/drivers/");
	openvrDriverRootDir.append(openvrDriverName);
	openvrDriverRootDir.append("/bin/linux64");
	openvrDriverRootDir=configFile.retrieveString("./openvrDriverRootDir",openvrDriverRootDir);
	openvrDriverRootDir=pathcat(steamRootDir,openvrDriverRootDir);
	
	/* Retrieve the name of the OpenVR device driver dynamic library: */
	std::string openvrDriverDsoName="driver_";
	openvrDriverDsoName.append(openvrDriverName);
	openvrDriverDsoName.append(".so");
	openvrDriverDsoName=configFile.retrieveString("./openvrDriverDsoName",openvrDriverDsoName);
	openvrDriverDsoName=pathcat(openvrDriverRootDir,openvrDriverDsoName);
	
	/* Open the OpenVR device driver dso: */
	#ifdef VERBOSE
	printf("OpenVRHost: Loading OpenVR driver module from %s\n",openvrDriverDsoName.c_str());
	fflush(stdout);
	#endif
	openvrDriverDsoHandle=dlopen(openvrDriverDsoName.c_str(),RTLD_NOW);
	if(openvrDriverDsoHandle==0)
		Misc::throwStdErr("OpenVRHost: Unable to load OpenVR driver dynamic shared object %s due to error %s",openvrDriverDsoName.c_str(),dlerror());
	
	/* Retrieve the name of the main driver factory function: */
	std::string openvrFactoryFunctionName="HmdDriverFactory";
	openvrFactoryFunctionName=configFile.retrieveString("./openvrFactoryFunctionName",openvrFactoryFunctionName);
	
	/* Resolve the main factory function (using an evil hack to avoid warnings): */
	typedef void* (*HmdDriverFactoryFunction)(const char* pInterfaceName,int* pReturnCode);
	ptrdiff_t factoryIntermediate=reinterpret_cast<ptrdiff_t>(dlsym(openvrDriverDsoHandle,openvrFactoryFunctionName.c_str()));
	HmdDriverFactoryFunction HmdDriverFactory=reinterpret_cast<HmdDriverFactoryFunction>(factoryIntermediate);
	if(HmdDriverFactory==0)
		{
		dlclose(openvrDriverDsoHandle);
		Misc::throwStdErr("OpenVRHost: Unable to resolve OpenVR driver factory function %s due to error %s",openvrFactoryFunctionName.c_str(),dlerror());
		}
	
	/* Get a pointer to the server-side driver object: */
	int error=0;
	openvrTrackedDeviceProvider=reinterpret_cast<vr::IServerTrackedDeviceProvider*>(HmdDriverFactory(vr::IServerTrackedDeviceProvider_Version,&error));
	if(openvrTrackedDeviceProvider==0)
		{
		dlclose(openvrDriverDsoHandle);
		Misc::throwStdErr("OpenVRHost: Unable to retrieve server-side driver object due to error %d",error);
		}
	
	/*********************************************************************
	Second initialization step: Initialize the VR device driver module.
	*********************************************************************/
	
	/* Retrieve the OpenVR device driver configuration directory: */
	openvrDriverConfigDir="config/";
	openvrDriverConfigDir.append(openvrDriverName);
	openvrDriverConfigDir=configFile.retrieveString("./openvrDriverConfigDir",openvrDriverConfigDir);
	openvrDriverConfigDir=pathcat(steamRootDir,openvrDriverConfigDir);
	#ifdef VERBOSE
	printf("OpenVRHost: OpenVR driver module configuration directory is %s\n",openvrDriverConfigDir.c_str());
	fflush(stdout);
	#endif
	
	/* Create descriptors for supported device types: */
	
	/* Head-mounted devices: */
	deviceConfigurations[0].nameTemplate=configFile.retrieveString("./hmdName","HMD");
	deviceConfigurations[0].haveTracker=true;
	deviceConfigurations[0].numButtons=2;
	deviceConfigurations[0].buttonNames.push_back("Button");
	deviceConfigurations[0].buttonNames.push_back("FaceDetector");
	deviceConfigurations[0].numValuators=0;
	deviceConfigurations[0].numHapticFeatures=0;
	deviceConfigurations[0].numPowerFeatures=0;
	
	/* Controllers: */
	deviceConfigurations[1].nameTemplate=configFile.retrieveString("./controllerNameTemplate","Controller%u");
	deviceConfigurations[1].haveTracker=true;
	deviceConfigurations[1].numButtons=6;
	deviceConfigurations[1].buttonNames.push_back("System");
	deviceConfigurations[1].buttonNames.push_back("Grip");
	deviceConfigurations[1].buttonNames.push_back("Menu");
	deviceConfigurations[1].buttonNames.push_back("Trigger");
	deviceConfigurations[1].buttonNames.push_back("TouchpadClick");
	deviceConfigurations[1].buttonNames.push_back("TouchpadTouch");
	deviceConfigurations[1].numValuators=3;
	deviceConfigurations[1].valuatorNames.push_back("AnalogTrigger");
	deviceConfigurations[1].valuatorNames.push_back("TouchpadX");
	deviceConfigurations[1].valuatorNames.push_back("TouchpadY");
	deviceConfigurations[1].numHapticFeatures=1;
	deviceConfigurations[1].hapticFeatureNames.push_back("Haptic");
	deviceConfigurations[1].numPowerFeatures=1;
	
	/* Trackers: */
	deviceConfigurations[2].nameTemplate=configFile.retrieveString("./trackerNameTemplate","Tracker%u");
	deviceConfigurations[2].haveTracker=true;
	deviceConfigurations[2].numButtons=0;
	deviceConfigurations[2].numValuators=0;
	deviceConfigurations[2].numHapticFeatures=0;
	deviceConfigurations[2].numPowerFeatures=1;
	
	/* Tracking base stations: */
	deviceConfigurations[3].nameTemplate=configFile.retrieveString("./baseStationNameTemplate","BaseStation%u");
	deviceConfigurations[3].haveTracker=false;
	deviceConfigurations[3].numButtons=0;
	deviceConfigurations[3].numValuators=0;
	deviceConfigurations[3].numHapticFeatures=0;
	deviceConfigurations[3].numPowerFeatures=0;
	
	/* Read the maximum number of supported controllers, trackers, and tracking base stations: */
	maxNumDevices[HMD]=1;
	maxNumDevices[Controller]=configFile.retrieveValue<unsigned int>("./maxNumControllers",2);
	maxNumDevices[Tracker]=configFile.retrieveValue<unsigned int>("./maxNumTrackers",0);
	maxNumDevices[BaseStation]=configFile.retrieveValue<unsigned int>("./maxNumBaseStations",2);
	
	/* Calculate total number of device state components: */
	maxNumDevices[NumDeviceTypes]=0;
	unsigned int totalNumTrackers=0;
	unsigned int totalNumButtons=0;
	unsigned int totalNumValuators=0;
	numHapticFeatures=0;
	unsigned int totalNumPowerFeatures=0;
	for(unsigned int deviceType=HMD;deviceType<NumDeviceTypes;++deviceType)
		{
		/* Get the maximum number of devices of this type: */
		unsigned int mnd=maxNumDevices[deviceType];
		maxNumDevices[NumDeviceTypes]+=mnd;
		
		/* Calculate number of device state components for this device type: */
		if(deviceConfigurations[deviceType].haveTracker)
			totalNumTrackers+=mnd;
		totalNumButtons+=mnd*deviceConfigurations[deviceType].numButtons;
		totalNumValuators+=mnd*deviceConfigurations[deviceType].numValuators;
		numHapticFeatures+=mnd*deviceConfigurations[deviceType].numHapticFeatures;
		totalNumPowerFeatures+=mnd*deviceConfigurations[deviceType].numPowerFeatures;
		}
	
	/* Initialize VRDevice's device state variables: */
	setNumTrackers(totalNumTrackers,configFile);
	setNumButtons(totalNumButtons,configFile);
	setNumValuators(totalNumValuators,configFile);
	
	/* Store the originally configured tracker post-transformations to manipulate them later: */
	configuredPostTransformations=new TrackerPostTransformation[totalNumTrackers];
	for(unsigned int i=0;i<totalNumTrackers;++i)
		configuredPostTransformations[i]=trackerPostTransformations[i];
	
	/* Create array of OpenVR device states: */
	deviceStates=new DeviceState[maxNumDevices[NumDeviceTypes]];
	
	/* Create an array of pending haptic events: */
	hapticEvents=new HapticEvent[numHapticFeatures];
	
	/* Create power features: */
	for(unsigned int i=0;i<totalNumPowerFeatures;++i)
		deviceManager->addPowerFeature(this,i);
	
	/* Create array to map power features to OpenVR devices: */
	powerFeatureDevices=new DeviceState*[totalNumPowerFeatures];
	for(unsigned int i=0;i<totalNumPowerFeatures;++i)
		powerFeatureDevices[i]=0;
	
	/* Create virtual devices for all tracked device types: */
	unsigned int nextTrackerIndex=0;
	unsigned int nextButtonIndex=0;
	unsigned int nextValuatorIndex=0;
	unsigned int nextHapticFeatureIndex=0;
	for(unsigned int deviceType=HMD;deviceType<NumDeviceTypes;++deviceType)
		{
		DeviceConfiguration& dc=deviceConfigurations[deviceType];
		if(dc.haveTracker)
			{
			virtualDeviceIndices[deviceType]=new unsigned int[maxNumDevices[deviceType]];
			for(unsigned int deviceIndex=0;deviceIndex<maxNumDevices[deviceType];++deviceIndex)
				{
				/* Create a virtual device: */
				Vrui::VRDeviceDescriptor* vd=new Vrui::VRDeviceDescriptor(dc.numButtons,dc.numValuators,dc.numHapticFeatures);
				vd->name=Misc::stringPrintf(dc.nameTemplate.c_str(),1U+deviceIndex);
				
				vd->trackType=Vrui::VRDeviceDescriptor::TRACK_POS|Vrui::VRDeviceDescriptor::TRACK_DIR|Vrui::VRDeviceDescriptor::TRACK_ORIENT;
				vd->rayDirection=Vrui::VRDeviceDescriptor::Vector(0,0,-1);
				vd->rayStart=0.0f;
				
				/* Assign a tracker index: */
				vd->trackerIndex=getTrackerIndex(nextTrackerIndex++);
				
				/* Assign buttons names and indices: */
				for(unsigned int i=0;i<dc.numButtons;++i)
					{
					vd->buttonNames[i]=dc.buttonNames[i];
					vd->buttonIndices[i]=getButtonIndex(nextButtonIndex++);
					}
				
				/* Assign valuator names and indices: */
				for(unsigned int i=0;i<dc.numValuators;++i)
					{
					vd->valuatorNames[i]=dc.valuatorNames[i];
					vd->valuatorIndices[i]=getValuatorIndex(nextValuatorIndex++);
					}
				
				/* Assign haptic feature names and indices: */
				for(unsigned int i=0;i<dc.numHapticFeatures;++i)
					{
					vd->hapticFeatureNames[i]=dc.hapticFeatureNames[i];
					vd->hapticFeatureIndices[i]=deviceManager->addHapticFeature(this,nextHapticFeatureIndex++);
					}
				
				/* Override virtual device settings from a configuration file section of the device's name: */
				vd->load(configFile.getSection(vd->name.c_str()));
				
				/* Register the virtual device: */
				virtualDeviceIndices[deviceType][deviceIndex]=addVirtualDevice(vd);
				}
			}
		else
			virtualDeviceIndices[deviceType]=0;
		
		/* Initialize number of connected devices of this type: */
		numConnectedDevices[deviceType]=0;
		}
	
	/* Initialize the total number of connected devices: */
	numConnectedDevices[NumDeviceTypes]=0;
	
	/* Read the number of distortion mesh vertices to calculate: */
	unsigned int distortionMeshSize[2];
	distortionMeshSize[1]=distortionMeshSize[0]=32;
	Misc::CFixedArrayValueCoder<unsigned int,2> distortionMeshSizeVC(distortionMeshSize);
	configFile.retrieveValueWC<unsigned int*>("./distortionMeshSize",distortionMeshSize,distortionMeshSizeVC);
	
	/* Add an HMD configuration for the headset: */
	hmdConfiguration=deviceManager->addHmdConfiguration();
	hmdConfiguration->setTrackerIndex(getTrackerIndex(0));
	hmdConfiguration->setEyePos(Vrui::HMDConfiguration::Point(-0.0635f*0.5f,0,0),Vrui::HMDConfiguration::Point(0.0635f*0.5f,0,0)); // Initialize default eye positions
	hmdConfiguration->setDistortionMeshSize(distortionMeshSize[0],distortionMeshSize[1]);
	
	/* Initialize the component feature index array: */
	componentFeatureIndices=new unsigned int[totalNumButtons+totalNumValuators+numHapticFeatures];
	}

OpenVRHost::~OpenVRHost(void)
	{
	/* Enter stand-by mode: */
	#ifdef VERBOSE
	printf("OpenVRHost: Powering down devices\n");
	fflush(stdout);
	#endif
	exiting=true;
	
	/* Put all tracked devices into stand-by mode: */
	for(unsigned int i=0;i<numConnectedDevices[NumDeviceTypes];++i)
		deviceStates[i].driver->EnterStandby();
	
	/* Put the main server into stand-by mode: */
	openvrTrackedDeviceProvider->EnterStandby();
	usleep(100000);
	
	/* Deactivate all devices: */
	for(unsigned int i=0;i<numConnectedDevices[NumDeviceTypes];++i)
		deviceStates[i].driver->Deactivate();
	usleep(500000);
	
	#ifdef VERBOSE
	printf("OpenVRHost: Shutting down OpenVR driver module\n");
	fflush(stdout);
	#endif
	openvrTrackedDeviceProvider->Cleanup();
	
	/* Stop the device thread: */
	#ifdef VERBOSE
	printf("OpenVRHost: Stopping event processing\n");
	fflush(stdout);
	#endif
	stopDeviceThread();
	
	/* Delete VRDeviceManager association tables: */
	delete[] configuredPostTransformations;
	for(unsigned int deviceType=HMD;deviceType<NumDeviceTypes;++deviceType)
		delete[] virtualDeviceIndices[deviceType];
	delete[] hapticEvents;
	delete[] powerFeatureDevices;
	delete[] componentFeatureIndices;
	
	/* Delete the OpenVR device state array: */
	delete[] deviceStates;
	
	/* Close the OpenVR device driver dso: */
	dlclose(openvrDriverDsoHandle);
	}

/* Methods inherited from class VRDevice: */

void OpenVRHost::initialize(void)
	{
	/*********************************************************************
	Third initialization step: Initialize the server-side interface of the
	OpenVR driver contained in the shared library.
	*********************************************************************/
	
	/* Start the device thread already to dispatch driver messages during initialization: */
	#ifdef VERBOSE
	printf("OpenVRHost: Starting event processing\n");
	fflush(stdout);
	#endif
	startDeviceThread();
	
	/* Initialize the server-side driver object: */
	#ifdef VERBOSE
	printf("OpenVRHost: Initializing OpenVR driver module\n");
	fflush(stdout);
	#endif
	vr::EVRInitError initError=openvrTrackedDeviceProvider->Init(static_cast<vr::IVRDriverContext*>(this));
	if(initError!=vr::VRInitError_None)
		Misc::throwStdErr("OpenVRHost: Unable to initialize server-side driver object due to OpenVR error %d",int(initError));
	
	/* Leave stand-by mode: */
	#ifdef VERBOSE
	printf("OpenVRHost: Powering up devices\n");
	fflush(stdout);
	#endif
	openvrTrackedDeviceProvider->LeaveStandby();
	}

void OpenVRHost::start(void)
	{
	/* Could un-suspend OpenVR driver at this point... */
	}

void OpenVRHost::stop(void)
	{
	/* Could suspend OpenVR driver at this point... */
	}

void OpenVRHost::powerOff(int devicePowerFeatureIndex)
	{
	/* Power off the device if it is connected and can power off: */
	if(powerFeatureDevices[devicePowerFeatureIndex]!=0&&powerFeatureDevices[devicePowerFeatureIndex]->canPowerOff)
		{
		#ifdef VERBOSE
		printf("OpenVRHost: Powering off device with serial number %s\n",powerFeatureDevices[devicePowerFeatureIndex]->serialNumber.c_str());
		#endif
		
		/* Power off the device: */
		powerFeatureDevices[devicePowerFeatureIndex]->driver->EnterStandby();
		}
	}

void OpenVRHost::hapticTick(int deviceHapticFeatureIndex,unsigned int duration)
	{
	/* Bail out if there is already a pending event: */
	HapticEvent& he=hapticEvents[deviceHapticFeatureIndex];
	if(!he.pending)
		{
		/* Store the new event: */
		he.pending=true;
		he.duration=duration;
		he.frequency=1000;
		he.amplitude=255;
		}
	}

/* Methods inherited from class vr::IVRSettings: */

const char* OpenVRHost::GetSettingsErrorNameFromEnum(vr::EVRSettingsError eError)
	{
	switch(eError)
		{
		case vr::VRSettingsError_None:
			return "No error";
		
		case vr::VRSettingsError_IPCFailed:
			return "IPC failed";
		
		case vr::VRSettingsError_WriteFailed:
			return "Write failed";
		
		case vr::VRSettingsError_ReadFailed:
			return "Read failed";
		
		case vr::VRSettingsError_JsonParseFailed:
			return "Parse failed";
		
		case vr::VRSettingsError_UnsetSettingHasNoDefault:
			return "";
		
		default:
			return "Unknown settings error";
		}
	}

bool OpenVRHost::Sync(bool bForce,vr::EVRSettingsError* peError)
	{
	/* Don't know what to do: */
	return true;
	}

void OpenVRHost::SetBool(const char* pchSection,const char* pchSettingsKey,bool bValue,vr::EVRSettingsError* peError)
	{
	/* Go to the requested configuration subsection and store the value: */
	Misc::ConfigurationFileSection section=openvrSettingsSection.getSection(pchSection);
	section.storeValue<bool>(pchSettingsKey,bValue);
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

void OpenVRHost::SetInt32(const char* pchSection,const char* pchSettingsKey,int32_t nValue,vr::EVRSettingsError* peError)
	{
	/* Go to the requested configuration subsection and store the value: */
	Misc::ConfigurationFileSection section=openvrSettingsSection.getSection(pchSection);
	section.storeValue<int32_t>(pchSettingsKey,nValue);
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

void OpenVRHost::SetFloat(const char* pchSection,const char* pchSettingsKey,float flValue,vr::EVRSettingsError* peError)
	{
	/* Go to the requested configuration subsection and store the value: */
	Misc::ConfigurationFileSection section=openvrSettingsSection.getSection(pchSection);
	section.storeValue<float>(pchSettingsKey,flValue);
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

void OpenVRHost::SetString(const char* pchSection,const char* pchSettingsKey,const char* pchValue,vr::EVRSettingsError* peError)
	{
	/* Go to the requested configuration subsection and store the value: */
	Misc::ConfigurationFileSection section=openvrSettingsSection.getSection(pchSection);
	section.storeString(pchSettingsKey,pchValue);
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

bool OpenVRHost::GetBool(const char* pchSection,const char* pchSettingsKey,vr::EVRSettingsError* peError)
	{
	/* Go to the requested configuration subsection and retrieve the requested value: */
	Misc::ConfigurationFileSection section=openvrSettingsSection.getSection(pchSection);
	bool result=section.retrieveValue<bool>(pchSettingsKey,false);
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	return result;
	}

int32_t OpenVRHost::GetInt32(const char* pchSection,const char*pchSettingsKey,vr::EVRSettingsError* peError)
	{
	/* Go to the requested configuration subsection and retrieve the requested value: */
	Misc::ConfigurationFileSection section=openvrSettingsSection.getSection(pchSection);
	int32_t result=section.retrieveValue<int32_t>(pchSettingsKey,0);
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	return result;
	}

float OpenVRHost::GetFloat(const char* pchSection,const char* pchSettingsKey,vr::EVRSettingsError* peError)
	{
	/* Go to the requested configuration subsection and retrieve the requested value: */
	Misc::ConfigurationFileSection section=openvrSettingsSection.getSection(pchSection);
	float result=section.retrieveValue<float>(pchSettingsKey,0.0f);
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	return result;
	}

void OpenVRHost::GetString(const char* pchSection,const char* pchSettingsKey,char* pchValue,uint32_t unValueLen,vr::EVRSettingsError* peError)
	{
	/* Go to the requested configuration subsection and retrieve the requested value: */
	Misc::ConfigurationFileSection section=openvrSettingsSection.getSection(pchSection);
	std::string result=section.retrieveString(pchSettingsKey,"");
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	
	/* Copy the result string into the provided buffer: */
	if(unValueLen>=result.size()+1)
		memcpy(pchValue,result.c_str(),result.size()+1);
	else
		{
		pchValue[0]='\0';
		if(peError!=0)
			*peError=vr::VRSettingsError_ReadFailed;
		}
	}

void OpenVRHost::RemoveSection(const char* pchSection,vr::EVRSettingsError* peError)
	{
	/* Ignore this request: */
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

void OpenVRHost::RemoveKeyInSection(const char* pchSection,const char* pchSettingsKey,vr::EVRSettingsError* peError)
	{
	/* Ignore this request: */
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

/* Methods inherited from class vr::IVRDriverContext: */

void* OpenVRHost::GetGenericInterface(const char* pchInterfaceVersion,vr::EVRInitError* peError)
	{
	if(peError!=0)
		*peError=vr::VRInitError_None;
	
	/* Cast the driver module object to the requested type and return it: */
	if(strcmp(pchInterfaceVersion,vr::IVRSettings_Version)==0)
		return static_cast<vr::IVRSettings*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRProperties_Version)==0)
		return static_cast<vr::IVRProperties*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRDriverInput_Version)==0)
		return static_cast<vr::IVRDriverInput*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRDriverLog_Version)==0)
		return static_cast<vr::IVRDriverLog*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRServerDriverHost_Version)==0)
		return static_cast<vr::IVRServerDriverHost*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRResources_Version)==0)
		return static_cast<vr::IVRResources*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRDriverManager_Version)==0)
		return static_cast<vr::IVRDriverManager*>(this);
	else
		{
		/* Signal an error: */
		#ifdef VERBOSE
		printf("OpenVRHost: Error: Requested server interface %s not found\n",pchInterfaceVersion);
		fflush(stdout);
		#endif
		if(peError!=0)
			*peError=vr::VRInitError_Init_InterfaceNotFound;
		return 0;
		}
	}

vr::DriverHandle_t OpenVRHost::GetDriverHandle(void)
	{
	/* Driver itself has a fixed handle, based on OpenVR's vrserver: */
	return driverHandle;
	}

/* Methods inherited from class vr::IVRProperties: */

namespace {

/****************
Helper functions:
****************/

inline const char* propertyTypeName(vr::PropertyTypeTag_t tag)
	{
	switch(tag)
		{
		case vr::k_unInvalidPropertyTag:
			return "(invalid type)";
		
		case vr::k_unFloatPropertyTag:
			return "float";
		
		case vr::k_unInt32PropertyTag:
			return "32-bit integer";
		
		case vr::k_unUint64PropertyTag:
			return "64-bit unsigned integer";
		
		case vr::k_unBoolPropertyTag:
			return "boolean";
		
		case vr::k_unStringPropertyTag:
			return "string";
		
		case vr::k_unHmdMatrix34PropertyTag:
			return "3x4 matrix";
		
		case vr::k_unHmdMatrix44PropertyTag:
			return "4x4 matrix";
		
		case vr::k_unHmdVector3PropertyTag:
			return "affine vector";
		
		case vr::k_unHmdVector4PropertyTag:
			return "homogeneous vector";
		
		case vr::k_unHiddenAreaPropertyTag:
			return "hidden area";
		
		default:
			if(tag>=vr::k_unOpenVRInternalReserved_Start&&tag<vr::k_unOpenVRInternalReserved_End)
				return "(OpenVR internal type)";
			else
				return "(unknown type)";
		}
	}

inline void storeFloat(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyContainerHandle_t minHandle,vr::PropertyContainerHandle_t maxHandle,float value,vr::PropertyRead_t& prop)
	{
	/* Initialize the property: */
	prop.unRequiredBufferSize=sizeof(float);
	prop.eError=vr::TrackedProp_Success;
	
	/* Check for correctness: */
	if(ulContainerHandle>=minHandle&&ulContainerHandle<=maxHandle)
		{
		if(prop.unBufferSize>=prop.unRequiredBufferSize)
			{
			prop.unTag=vr::k_unFloatPropertyTag;
			*static_cast<float*>(prop.pvBuffer)=value;
			}
		else
			prop.eError=vr::TrackedProp_BufferTooSmall;
		}
	else
		prop.eError=vr::TrackedProp_InvalidDevice;
	}

inline void storeUint64(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyContainerHandle_t minHandle,vr::PropertyContainerHandle_t maxHandle,uint64_t value,vr::PropertyRead_t& prop)
	{
	/* Initialize the property: */
	prop.unRequiredBufferSize=sizeof(uint64_t);
	prop.eError=vr::TrackedProp_Success;
	
	/* Check for correctness: */
	if(ulContainerHandle>=minHandle&&ulContainerHandle<=maxHandle)
		{
		if(prop.unBufferSize>=prop.unRequiredBufferSize)
			{
			prop.unTag=vr::k_unUint64PropertyTag;
			*static_cast<uint64_t*>(prop.pvBuffer)=value;
			}
		else
			prop.eError=vr::TrackedProp_BufferTooSmall;
		}
	else
		prop.eError=vr::TrackedProp_InvalidDevice;
	}

inline void storeBool(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyContainerHandle_t minHandle,vr::PropertyContainerHandle_t maxHandle,bool value,vr::PropertyRead_t& prop)
	{
	/* Initialize the property: */
	prop.unRequiredBufferSize=sizeof(bool);
	prop.eError=vr::TrackedProp_Success;
	
	/* Check for correctness: */
	if(ulContainerHandle>=minHandle&&ulContainerHandle<=maxHandle)
		{
		if(prop.unBufferSize>=prop.unRequiredBufferSize)
			{
			prop.unTag=vr::k_unBoolPropertyTag;
			*static_cast<bool*>(prop.pvBuffer)=value;
			}
		else
			prop.eError=vr::TrackedProp_BufferTooSmall;
		}
	else
		prop.eError=vr::TrackedProp_InvalidDevice;
	}

inline void storeString(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyContainerHandle_t minHandle,vr::PropertyContainerHandle_t maxHandle,const std::string& value,vr::PropertyRead_t& prop)
	{
	/* Initialize the property to the empty string: */
	static_cast<char*>(prop.pvBuffer)[0]='\0';
	prop.unRequiredBufferSize=value.size()+1;
	prop.eError=vr::TrackedProp_Success;
	
	/* Check for correctness: */
	if(ulContainerHandle>=minHandle&&ulContainerHandle<=maxHandle)
		{
		if(prop.unBufferSize>=prop.unRequiredBufferSize)
			{
			prop.unTag=vr::k_unStringPropertyTag;
			memcpy(prop.pvBuffer,value.c_str(),prop.unRequiredBufferSize);
			}
		else
			prop.eError=vr::TrackedProp_BufferTooSmall;
		}
	else
		prop.eError=vr::TrackedProp_InvalidDevice;
	}

inline bool retrieveFloat(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyContainerHandle_t minHandle,vr::PropertyContainerHandle_t maxHandle,vr::PropertyWrite_t& prop,float& value)
	{
	/* Initialize the property: */
	prop.eError=vr::TrackedProp_Success;
	
	/* Check for correctness: */
	if(ulContainerHandle>=minHandle&&ulContainerHandle<=maxHandle)
		{
		if(prop.unTag==vr::k_unFloatPropertyTag)
			{
			if(prop.unBufferSize==sizeof(float))
				value=*static_cast<float*>(prop.pvBuffer);
			else
				prop.eError=vr::TrackedProp_BufferTooSmall;
			}
		else
			prop.eError=vr::TrackedProp_WrongDataType;
		}
	else
		prop.eError=vr::TrackedProp_InvalidDevice;
	
	return prop.eError==vr::TrackedProp_Success;
	}

inline bool retrieveInt32(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyContainerHandle_t minHandle,vr::PropertyContainerHandle_t maxHandle,vr::PropertyWrite_t& prop,int32_t& value)
	{
	/* Initialize the property: */
	prop.eError=vr::TrackedProp_Success;
	
	/* Check for correctness: */
	if(ulContainerHandle>=minHandle&&ulContainerHandle<=maxHandle)
		{
		if(prop.unTag==vr::k_unInt32PropertyTag)
			{
			if(prop.unBufferSize==sizeof(int32_t))
				value=*static_cast<int32_t*>(prop.pvBuffer);
			else
				prop.eError=vr::TrackedProp_BufferTooSmall;
			}
		else
			prop.eError=vr::TrackedProp_WrongDataType;
		}
	else
		prop.eError=vr::TrackedProp_InvalidDevice;
	
	return prop.eError==vr::TrackedProp_Success;
	}

inline bool retrieveUint64(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyContainerHandle_t minHandle,vr::PropertyContainerHandle_t maxHandle,vr::PropertyWrite_t& prop,uint64_t& value)
	{
	/* Initialize the property: */
	prop.eError=vr::TrackedProp_Success;
	
	/* Check for correctness: */
	if(ulContainerHandle>=minHandle&&ulContainerHandle<=maxHandle)
		{
		if(prop.unTag==vr::k_unUint64PropertyTag)
			{
			if(prop.unBufferSize==sizeof(uint64_t))
				value=*static_cast<uint64_t*>(prop.pvBuffer);
			else
				prop.eError=vr::TrackedProp_BufferTooSmall;
			}
		else
			prop.eError=vr::TrackedProp_WrongDataType;
		}
	else
		prop.eError=vr::TrackedProp_InvalidDevice;
	
	return prop.eError==vr::TrackedProp_Success;
	}

inline bool retrieveBool(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyContainerHandle_t minHandle,vr::PropertyContainerHandle_t maxHandle,vr::PropertyWrite_t& prop,bool& value)
	{
	/* Initialize the property: */
	prop.eError=vr::TrackedProp_Success;
	
	/* Check for correctness: */
	if(ulContainerHandle>=minHandle&&ulContainerHandle<=maxHandle)
		{
		if(prop.unTag==vr::k_unBoolPropertyTag)
			{
			if(prop.unBufferSize==sizeof(bool))
				value=*static_cast<bool*>(prop.pvBuffer);
			else
				prop.eError=vr::TrackedProp_BufferTooSmall;
			}
		else
			prop.eError=vr::TrackedProp_WrongDataType;
		}
	else
		prop.eError=vr::TrackedProp_InvalidDevice;
	
	return prop.eError==vr::TrackedProp_Success;
	}

inline bool retrieveString(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyContainerHandle_t minHandle,vr::PropertyContainerHandle_t maxHandle,vr::PropertyWrite_t& prop,std::string& value)
	{
	/* Initialize the property: */
	prop.eError=vr::TrackedProp_Success;
	
	/* Check for correctness: */
	if(ulContainerHandle>=minHandle&&ulContainerHandle<=maxHandle)
		{
		if(prop.unTag==vr::k_unStringPropertyTag)
			value=static_cast<char*>(prop.pvBuffer);
		else
			prop.eError=vr::TrackedProp_WrongDataType;
		}
	else
		prop.eError=vr::TrackedProp_InvalidDevice;
	
	return prop.eError==vr::TrackedProp_Success;
	}

}

vr::ETrackedPropertyError OpenVRHost::ReadPropertyBatch(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyRead_t* pBatch,uint32_t unBatchEntryCount)
	{
	/* Access the affected device state (in case this is a device property; won't be used if it isn't): */
	DeviceState& ds=deviceStates[ulContainerHandle-deviceHandleBase];
	
	/* Process all properties in this batch: */
	vr::ETrackedPropertyError result=vr::TrackedProp_Success;
	vr::PropertyRead_t* pPtr=pBatch;
	unsigned int minDeviceHandle=deviceHandleBase;
	unsigned int maxDeviceHandle=deviceHandleBase+numConnectedDevices[NumDeviceTypes]-1;
	for(unsigned int entryIndex=0;entryIndex<unBatchEntryCount;++entryIndex,++pPtr)
		{
		/* Check for known property tags: */
		switch(pPtr->prop)
			{
			#if 0
			case vr::Prop_SerialNumber_String:
				storeString(ulContainerHandle,minDeviceHandle,maxDeviceHandle,ds.serialNumber,*pPtr);
				break;
			
			case vr::Prop_DisplayMCImageLeft_String:
			case vr::Prop_DisplayMCImageRight_String:
				/* Return an empty string because OpenVR hangs in the MC image loader: */
				static_cast<char*>(pPtr->pvBuffer)[0]='\0';
				pPtr->unTag=vr::k_unStringPropertyTag;
				pPtr->unRequiredBufferSize=1;
				pPtr->eError=vr::TrackedProp_Success;
				break;
			#endif
			
			case vr::Prop_DeviceCanPowerOff_Bool:
				storeBool(ulContainerHandle,minDeviceHandle,maxDeviceHandle,ds.canPowerOff,*pPtr);
				break;
			
			case vr::Prop_LensCenterLeftU_Float:
				storeFloat(ulContainerHandle,minDeviceHandle,minDeviceHandle,ds.lensCenters[0][0],*pPtr);
				break;
			
			case vr::Prop_LensCenterLeftV_Float:
				storeFloat(ulContainerHandle,minDeviceHandle,minDeviceHandle,ds.lensCenters[0][1],*pPtr);
				break;
			
			case vr::Prop_LensCenterRightU_Float:
				storeFloat(ulContainerHandle,minDeviceHandle,minDeviceHandle,ds.lensCenters[1][0],*pPtr);
				break;
			
			case vr::Prop_LensCenterRightV_Float:
				storeFloat(ulContainerHandle,minDeviceHandle,minDeviceHandle,ds.lensCenters[1][1],*pPtr);
				break;
			
			case vr::Prop_UserConfigPath_String:
				storeString(ulContainerHandle,driverHandle,driverHandle,openvrDriverConfigDir,*pPtr);
				break;
			
			case vr::Prop_InstallPath_String:
				storeString(ulContainerHandle,driverHandle,driverHandle,openvrDriverRootDir,*pPtr);
				break;
			
			default:
				pPtr->eError=vr::TrackedProp_UnknownProperty;
			}
		if(pPtr->eError!=vr::TrackedProp_Success)
			{
			#ifdef VERYVERBOSE
			printf("OpenVRHost: Warning: Ignoring read of %s property %u for container %u due to error %s\n",propertyTypeName(pPtr->unTag),pPtr->prop,(unsigned int)(ulContainerHandle),GetPropErrorNameFromEnum(pPtr->eError));
			fflush(stdout);
			#endif
			result=pPtr->eError;
			}
		}
	
	return result;
	}

vr::ETrackedPropertyError OpenVRHost::WritePropertyBatch(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyWrite_t* pBatch,uint32_t unBatchEntryCount)
	{
	/* Access the affected device state (in case this is a device property; won't be used if it isn't): */
	DeviceState& ds=deviceStates[ulContainerHandle-deviceHandleBase];
	
	/* Process all properties in this batch: */
	vr::ETrackedPropertyError result=vr::TrackedProp_Success;
	vr::PropertyWrite_t* pPtr=pBatch;
	unsigned int minDeviceHandle=deviceHandleBase;
	unsigned int maxDeviceHandle=deviceHandleBase+numConnectedDevices[NumDeviceTypes]-1;
	for(unsigned int entryIndex=0;entryIndex<unBatchEntryCount;++entryIndex,++pPtr)
		{
		/* Check for known property tags: */
		switch(pPtr->prop)
			{
			/* Print some interesting properties: */
			case vr::Prop_SecondsFromVsyncToPhotons_Float:
				{
				float displayDelay=0.0f;
				retrieveFloat(ulContainerHandle,minDeviceHandle,minDeviceHandle,*pPtr,displayDelay);
				printf("OpenVRHost: Display delay from vsync = %fms\n",displayDelay*1000.0f);
				break;
				}
			
			case vr::Prop_DisplayMCImageLeft_String:
				{
				std::string mcImage;
				retrieveString(ulContainerHandle,minDeviceHandle,minDeviceHandle,*pPtr,mcImage);
				printf("OpenVRHost: Left Mura correction image is %s\n",mcImage.c_str());
				break;
				}
			
			case vr::Prop_DisplayMCImageRight_String:
				{
				std::string mcImage;
				retrieveString(ulContainerHandle,minDeviceHandle,minDeviceHandle,*pPtr,mcImage);
				printf("OpenVRHost: Right Mura correction image is %s\n",mcImage.c_str());
				break;
				}
			
			case vr::Prop_UserHeadToEyeDepthMeters_Float:
				{
				float ed=0.0f;
				retrieveFloat(ulContainerHandle,minDeviceHandle,minDeviceHandle,*pPtr,ed);
				printf("OpenVRHost: User eye depth = %f\n",ed);
				break;
				}
			
			/* Extract relevant properties: */
			case vr::Prop_WillDriftInYaw_Bool:
				retrieveBool(ulContainerHandle,minDeviceHandle,maxDeviceHandle,*pPtr,ds.willDriftInYaw);
				break;
			
			case vr::Prop_DeviceIsWireless_Bool:
				if(retrieveBool(ulContainerHandle,minDeviceHandle,maxDeviceHandle,*pPtr,ds.isWireless))
					{
					/* Notify the device manager: */
					deviceManager->updateBatteryState(ds.virtualDeviceIndex,ds.batteryState);
					}
				break;
			
			case vr::Prop_DeviceIsCharging_Bool:
				{
				bool newBatteryCharging;
				if(retrieveBool(ulContainerHandle,minDeviceHandle,maxDeviceHandle,*pPtr,newBatteryCharging)&&ds.batteryState.charging!=newBatteryCharging)
					{
					#ifdef VERBOSE
					if(newBatteryCharging)
						printf("OpenVRHost: Device %s is now charging\n",ds.serialNumber.c_str());
					else
						printf("OpenVRHost: Device %s is now discharging\n",ds.serialNumber.c_str());
					fflush(stdout);
					#endif
					ds.batteryState.charging=newBatteryCharging;
					
					/* Notify the device manager: */
					deviceManager->updateBatteryState(ds.virtualDeviceIndex,ds.batteryState);
					}
				break;
				}
			
			case vr::Prop_DeviceBatteryPercentage_Float:
				{
				float newBatteryLevel;
				if(retrieveFloat(ulContainerHandle,minDeviceHandle,maxDeviceHandle,*pPtr,newBatteryLevel))
					{
					unsigned int newBatteryPercent=(unsigned int)(Math::floor(newBatteryLevel*100.0f+0.5f));
					if(ds.batteryState.batteryLevel!=newBatteryPercent)
						{
						#ifdef VERBOSE
						printf("OpenVRHost: Battery level on device %s is %u%%\n",ds.serialNumber.c_str(),newBatteryPercent);
						fflush(stdout);
						#endif
						ds.batteryState.batteryLevel=newBatteryPercent;
					
						/* Notify the device manager: */
						deviceManager->updateBatteryState(ds.virtualDeviceIndex,ds.batteryState);
						}
					}
				break;
				}
			
			case vr::Prop_ContainsProximitySensor_Bool:
				retrieveBool(ulContainerHandle,minDeviceHandle,maxDeviceHandle,*pPtr,ds.hasProximitySensor);
				break;
			
			case vr::Prop_DeviceProvidesBatteryStatus_Bool:
				retrieveBool(ulContainerHandle,minDeviceHandle,maxDeviceHandle,*pPtr,ds.providesBatteryStatus);
				break;
			
			case vr::Prop_DeviceCanPowerOff_Bool:
				retrieveBool(ulContainerHandle,minDeviceHandle,maxDeviceHandle,*pPtr,ds.canPowerOff);
				break;
			
			case vr::Prop_LensCenterLeftU_Float:
				retrieveFloat(ulContainerHandle,minDeviceHandle,minDeviceHandle,*pPtr,ds.lensCenters[0][0]);
				break;
			
			case vr::Prop_LensCenterLeftV_Float:
				retrieveFloat(ulContainerHandle,minDeviceHandle,minDeviceHandle,*pPtr,ds.lensCenters[0][1]);
				break;
			
			case vr::Prop_LensCenterRightU_Float:
				retrieveFloat(ulContainerHandle,minDeviceHandle,minDeviceHandle,*pPtr,ds.lensCenters[1][0]);
				break;
			
			case vr::Prop_LensCenterRightV_Float:
				retrieveFloat(ulContainerHandle,minDeviceHandle,minDeviceHandle,*pPtr,ds.lensCenters[1][1]);
				break;
			
			case vr::Prop_UserIpdMeters_Float:
				{
				float ipd;
				if(retrieveFloat(ulContainerHandle,minDeviceHandle,minDeviceHandle,*pPtr,ipd)&&ds.hmdConfiguration!=0)
					{
					deviceManager->lockHmdConfigurations();
					
					/* Update the HMD's IPD: */
					ds.hmdConfiguration->setIpd(ipd);
					
					/* Update the HMD configuration in the device manager: */
					deviceManager->updateHmdConfiguration(ds.hmdConfiguration);
					
					deviceManager->unlockHmdConfigurations();
					}
				break;
				}
			
			/* Warn about unknown properties: */
			default:
				pPtr->eError=vr::TrackedProp_UnknownProperty;
			}
		if(pPtr->eError!=vr::TrackedProp_Success)
			{
			#ifdef VERYVERBOSE
			printf("OpenVRHost: Warning: Ignoring write of %s property %u for container %u due to error %s\n",propertyTypeName(pPtr->unTag),pPtr->prop,(unsigned int)(ulContainerHandle),GetPropErrorNameFromEnum(pPtr->eError));
			fflush(stdout);
			#endif
			result=pPtr->eError;
			}
		}
	
	return result;
	}

const char* OpenVRHost::GetPropErrorNameFromEnum(vr::ETrackedPropertyError error)
	{
	switch(error)
		{
		case vr::TrackedProp_Success:
			return "Success";
		
		case vr::TrackedProp_WrongDataType:
			return "Wrong data type";
		
		case vr::TrackedProp_WrongDeviceClass:
			return "Wrong device class";
		
		case vr::TrackedProp_BufferTooSmall:
			return "Buffer too small";
		
		case vr::TrackedProp_UnknownProperty:
			return "Unknown property";
		
		case vr::TrackedProp_InvalidDevice:
			return "Invalid device";
		
		case vr::TrackedProp_CouldNotContactServer:
			return "Could not contact server";
		
		case vr::TrackedProp_ValueNotProvidedByDevice:
			return "Value not provided by device";
		
		case vr::TrackedProp_StringExceedsMaximumLength:
			return "String exceeds maximum length";
		
		case vr::TrackedProp_NotYetAvailable:
			return "Not yet available";
		
		case vr::TrackedProp_PermissionDenied:
			return "Permission denied";
		
		case vr::TrackedProp_InvalidOperation:
			return "Invalid operation";
		
		default:
			return "Unknown error";
		}
	}

vr::PropertyContainerHandle_t OpenVRHost::TrackedDeviceToPropertyContainer(vr::TrackedDeviceIndex_t nDevice)
	{
	return deviceHandleBase+nDevice;
	}

/* Methods inherited from class vr::IVRDriverInput: */

vr::EVRInputError OpenVRHost::CreateBooleanComponent(vr::PropertyContainerHandle_t ulContainer,const char* pchName,vr::VRInputComponentHandle_t* pHandle)
	{
	/* Get a reference to the container device's device state: */
	if(ulContainer<deviceHandleBase||ulContainer>=deviceHandleBase+numConnectedDevices[NumDeviceTypes])
		{
		#ifdef VERBOSE
		printf("OpenVRHost: Ignoring boolean input %s due to invalid container handle %u\n",pchName,(unsigned int)ulContainer);
		fflush(stdout);
		#endif
		
		return vr::VRInputError_InvalidHandle;
		}
	unsigned int deviceIndex=ulContainer-deviceHandleBase;
	DeviceState& ds=deviceStates[deviceIndex];
	
	/* Check if there is room in the device's button array: */
	if(ds.numButtons>=deviceConfigurations[ds.deviceType].numButtons)
		{
		#ifdef VERBOSE
		printf("OpenVRHost: Ignoring extra boolean input %s on device %u\n",pchName,deviceIndex);
		fflush(stdout);
		#endif
		
		return vr::VRInputError_MaxCapacityReached;
		}
	
	/* Assign the next device button index to the next component handle: */
	*pHandle=nextComponentHandle;
	componentFeatureIndices[nextComponentHandle-1]=ds.nextButtonIndex;
	++nextComponentHandle;
	++ds.nextButtonIndex;
	++ds.numButtons;
	
	return vr::VRInputError_None;
	}

vr::EVRInputError OpenVRHost::UpdateBooleanComponent(vr::VRInputComponentHandle_t ulComponent,bool bNewValue, double fTimeOffset)
	{
	/* Set the button's state: */
	setButtonState(componentFeatureIndices[ulComponent-1],bNewValue);
	
	return vr::VRInputError_None;
	}

vr::EVRInputError OpenVRHost::CreateScalarComponent(vr::PropertyContainerHandle_t ulContainer,const char* pchName,vr::VRInputComponentHandle_t* pHandle,vr::EVRScalarType eType,vr::EVRScalarUnits eUnits)
	{
	/* Get a reference to the container device's device state: */
	if(ulContainer<deviceHandleBase||ulContainer>=deviceHandleBase+numConnectedDevices[NumDeviceTypes])
		{
		#ifdef VERBOSE
		printf("OpenVRHost: Ignoring analog input %s due to invalid container handle %u\n",pchName,(unsigned int)ulContainer);
		fflush(stdout);
		#endif
		
		return vr::VRInputError_InvalidHandle;
		}
	unsigned int deviceIndex=ulContainer-deviceHandleBase;
	DeviceState& ds=deviceStates[deviceIndex];
	
	/* Check if there is room in the device's valuator array: */
	if(ds.numValuators>=deviceConfigurations[ds.deviceType].numValuators)
		{
		#ifdef VERBOSE
		printf("OpenVRHost: Ignoring extra analog input %s on device %u\n",pchName,deviceIndex);
		fflush(stdout);
		#endif
		
		return vr::VRInputError_MaxCapacityReached;
		}
	
	/* Assign the next device valuator index to the next component handle: */
	*pHandle=nextComponentHandle;
	componentFeatureIndices[nextComponentHandle-1]=ds.nextValuatorIndex;
	++nextComponentHandle;
	++ds.nextValuatorIndex;
	++ds.numValuators;
	
	return vr::VRInputError_None;
	}

vr::EVRInputError OpenVRHost::UpdateScalarComponent(vr::VRInputComponentHandle_t ulComponent,float fNewValue, double fTimeOffset)
	{
	/* Set the valuator's state: */
	setValuatorState(componentFeatureIndices[ulComponent-1],fNewValue);
	
	return vr::VRInputError_None;
	}

vr::EVRInputError OpenVRHost::CreateHapticComponent(vr::PropertyContainerHandle_t ulContainer,const char* pchName,vr::VRInputComponentHandle_t* pHandle)
	{
	/* Get a reference to the container device's device state: */
	if(ulContainer<deviceHandleBase||ulContainer>=deviceHandleBase+numConnectedDevices[NumDeviceTypes])
		{
		#ifdef VERBOSE
		printf("OpenVRHost: Ignoring haptic feature %s due to invalid container handle %u\n",pchName,(unsigned int)ulContainer);
		fflush(stdout);
		#endif
		
		return vr::VRInputError_InvalidHandle;
		}
	unsigned int deviceIndex=ulContainer-deviceHandleBase;
	DeviceState& ds=deviceStates[deviceIndex];
	
	/* Check if there is room in the device's haptic feature array: */
	if(ds.numHapticFeatures>=deviceConfigurations[ds.deviceType].numHapticFeatures)
		{
		#ifdef VERBOSE
		printf("OpenVRHost: Ignoring extra haptic feature %s on device %u\n",pchName,deviceIndex);
		fflush(stdout);
		#endif
		
		return vr::VRInputError_MaxCapacityReached;
		}
	
	/* Assign the next device haptic feature index to the next component handle: */
	*pHandle=nextComponentHandle;
	HapticEvent& he=hapticEvents[ds.nextHapticFeatureIndex];
	he.containerHandle=ulContainer;
	he.componentHandle=nextComponentHandle;
	he.pending=false;
	he.duration=0;
	he.frequency=0;
	he.amplitude=0;
	++nextComponentHandle;
	++ds.nextHapticFeatureIndex;
	++ds.numHapticFeatures;
	
	return vr::VRInputError_None;
	}

/* Methods from vr::IVRDriverLog: */

void OpenVRHost::Log(const char* pchLogMessage)
	{
	if(printLogMessages)
		{
		printf("OpenVRHost: Driver log: %s",pchLogMessage);
		fflush(stdout);
		}
	}

/* Methods inherited from class vr::IVRServerDriverHost: */

bool OpenVRHost::TrackedDeviceAdded(const char* pchDeviceSerialNumber,vr::ETrackedDeviceClass eDeviceClass,vr::ITrackedDeviceServerDriver* pDriver)
	{
	/* Determine the new device's class: */
	DeviceTypes deviceType=NumDeviceTypes;
	const char* newDeviceClass=0;
	switch(eDeviceClass)
		{
		case vr::TrackedDeviceClass_Invalid:
			newDeviceClass="invalid tracked device";
			break;
		
		case vr::TrackedDeviceClass_HMD:
			deviceType=HMD;
			newDeviceClass="head-mounted display";
			break;
		
		case vr::TrackedDeviceClass_Controller:
			deviceType=Controller;
			newDeviceClass="controller";
			break;
		
		case vr::TrackedDeviceClass_GenericTracker:
			deviceType=Tracker;
			newDeviceClass="generic tracker";
			break;
		
		case vr::TrackedDeviceClass_TrackingReference:
			deviceType=BaseStation;
			newDeviceClass="tracking base station";
			break;
		
		default:
			newDeviceClass="unknown device";
		}
	
	/* Bail out if the device has unknown type or the state array is full: */
	if(deviceType==NumDeviceTypes||numConnectedDevices[deviceType]>=maxNumDevices[deviceType])
		{
		#ifdef VERBOSE
		printf("OpenVRHost: Warning: Ignoring %s with serial number %s\n",newDeviceClass,pchDeviceSerialNumber);
		fflush(stdout);
		#endif
		
		return false;
		}
	
	/* Grab the next free device state structure: */
	const DeviceConfiguration& dc=deviceConfigurations[deviceType];
	DeviceState& ds=deviceStates[numConnectedDevices[NumDeviceTypes]];
	ds.deviceType=deviceType;
	ds.serialNumber=pchDeviceSerialNumber;
	ds.driver=pDriver;
	
	/* Check whether the device is tracked: */
	if(dc.haveTracker)
		{
		/* Assign the device state's tracker index: */
		ds.trackerIndex=0;
		for(unsigned int dt=HMD;dt<deviceType;++dt)
			ds.trackerIndex+=maxNumDevices[dt];
		ds.trackerIndex+=numConnectedDevices[deviceType];
		
		/* Assign the device state's virtual device index: */
		ds.virtualDeviceIndex=virtualDeviceIndices[deviceType][numConnectedDevices[deviceType]];
		}
	
	if(deviceType==HMD)
		{
		/* Assign the device state's HMD configuration: */
		ds.hmdConfiguration=hmdConfiguration;
		hmdConfiguration=0;
		
		/* Get the device's display component: */
		ds.display=static_cast<vr::IVRDisplayComponent*>(ds.driver->GetComponent(vr::IVRDisplayComponent_Version));
		if(ds.display!=0)
			{
			/* Initialize the device state's HMD configuration: */
			updateHMDConfiguration(ds);
			}
		else
			{
			#ifdef VERBOSE
			printf("OpenVRHost: Warning: Head-mounted display with serial number %s does not advertise a display\n",pchDeviceSerialNumber);
			fflush(stdout);
			#endif
			}
		}
	
	/* Assign the device state's first button index: */
	ds.nextButtonIndex=0;
	for(unsigned int dt=HMD;dt<deviceType;++dt)
		ds.nextButtonIndex+=maxNumDevices[dt]*deviceConfigurations[dt].numButtons;
	ds.nextButtonIndex+=numConnectedDevices[deviceType]*dc.numButtons;
	
	/* Assign the device state's first valuator index: */
	ds.nextValuatorIndex=0;
	for(unsigned int dt=HMD;dt<deviceType;++dt)
		ds.nextValuatorIndex+=maxNumDevices[dt]*deviceConfigurations[dt].numValuators;
	ds.nextValuatorIndex+=numConnectedDevices[deviceType]*dc.numValuators;
	
	/* Assign the device state's first haptic feature index: */
	ds.nextHapticFeatureIndex=0;
	for(unsigned int dt=HMD;dt<deviceType;++dt)
		ds.nextHapticFeatureIndex+=maxNumDevices[dt]*deviceConfigurations[dt].numHapticFeatures;
	ds.nextHapticFeatureIndex+=numConnectedDevices[deviceType]*dc.numHapticFeatures;
	
	/* Associate the device state with its power features: */
	unsigned int powerFeatureIndexBase=0;
	for(unsigned int dt=HMD;dt<deviceType;++dt)
		powerFeatureIndexBase+=maxNumDevices[dt]*deviceConfigurations[dt].numPowerFeatures;
	powerFeatureIndexBase+=numConnectedDevices[deviceType]*dc.numPowerFeatures;
	for(unsigned int i=0;i<dc.numPowerFeatures;++i)
		powerFeatureDevices[powerFeatureIndexBase+i]=&ds;
	
	/* Increase the number of connected devices: */
	++numConnectedDevices[deviceType];
	++numConnectedDevices[NumDeviceTypes];
	
	/* Activate the device: */
	#ifdef VERBOSE
	printf("OpenVRHost: Activating newly-added %s with serial number %s\n",newDeviceClass,pchDeviceSerialNumber);
	fflush(stdout);
	#endif
	ds.driver->Activate(numConnectedDevices[NumDeviceTypes]-1);
	
	#ifdef VERBOSE
	printf("OpenVRHost: Done activating newly-added %s with serial number %s\n",newDeviceClass,pchDeviceSerialNumber);
	fflush(stdout);
	#endif
	
	return true;
	}

void OpenVRHost::TrackedDevicePoseUpdated(uint32_t unWhichDevice,const vr::DriverPose_t& newPose,uint32_t unPoseStructSize)
	{
	/* Get a time stamp for the new device pose: */
	Vrui::VRDeviceState::TimeStamp poseTimeStamp=deviceManager->getTimeStamp(newPose.poseTimeOffset);
	
	/* Update the state of the affected device: */
	DeviceState& ds=deviceStates[unWhichDevice];
	
	/* Check if the device connected or disconnected: */
	if(ds.connected!=newPose.deviceIsConnected)
		{
		#ifdef VERBOSE
		if(newPose.deviceIsConnected)
			printf("OpenVRHost: Tracked device with serial number %s is now connected\n",ds.serialNumber.c_str());
		else
			printf("OpenVRHost: Tracked device with serial number %s is now disconnected\n",ds.serialNumber.c_str());
		fflush(stdout);
		#endif
		
		ds.connected=newPose.deviceIsConnected;
		}
	
	/* Check if the device changed tracking state: */
	if(ds.tracked!=newPose.poseIsValid)
		{
		#ifdef VERBOSE
		if(newPose.poseIsValid)
			printf("OpenVRHost: Tracked device with serial number %s regained tracking\n",ds.serialNumber.c_str());
		else
			printf("OpenVRHost: Tracked device with serial number %s lost tracking\n",ds.serialNumber.c_str());
		fflush(stdout);
		#endif
		
		/* Disable the device if it is no longer tracked: */
		if(!newPose.poseIsValid)
			disableTracker(ds.trackerIndex);
		
		ds.tracked=newPose.poseIsValid;
		}
	
	/* Update the device's transformation if it is being tracked: */
	if(ds.tracked)
		{
		Vrui::VRDeviceState::TrackerState ts;
		typedef PositionOrientation::Vector Vector;
		typedef Vector::Scalar Scalar;
		typedef PositionOrientation::Rotation Rotation;
		typedef Rotation::Scalar RScalar;
		
		/* Get the device's world transformation: */
		Rotation worldRot(RScalar(newPose.qWorldFromDriverRotation.x),RScalar(newPose.qWorldFromDriverRotation.y),RScalar(newPose.qWorldFromDriverRotation.z),RScalar(newPose.qWorldFromDriverRotation.w));
		Vector worldTrans(Scalar(newPose.vecWorldFromDriverTranslation[0]),Scalar(newPose.vecWorldFromDriverTranslation[1]),Scalar(newPose.vecWorldFromDriverTranslation[2]));
		PositionOrientation world(worldTrans,worldRot);
		
		/* Get the device's local transformation: */
		Rotation localRot(RScalar(newPose.qDriverFromHeadRotation.x),RScalar(newPose.qDriverFromHeadRotation.y),RScalar(newPose.qDriverFromHeadRotation.z),RScalar(newPose.qDriverFromHeadRotation.w));
		Vector localTrans(Scalar(newPose.vecDriverFromHeadTranslation[0]),Scalar(newPose.vecDriverFromHeadTranslation[1]),Scalar(newPose.vecDriverFromHeadTranslation[2]));
		PositionOrientation local(localTrans,localRot);
		
		/* Check for changes: */
		if(ds.worldTransform!=world)
			{
			ds.worldTransform=world;
			
			#if 0
			if(ds.trackerIndex>=0)
				{
				/* Print the new world transformation for testing purposes: */
				Vector t=world.getTranslation();
				Vector ra=world.getRotation().getAxis();
				double rw=Math::deg(world.getRotation().getAngle());
				printf("New world transform for tracker %d: (%f, %f, %f), (%f, %f, %f), %f\n",ds.trackerIndex,t[0],t[1],t[2],ra[0],ra[1],ra[2],rw);
				}
			#endif
			}
		if(ds.localTransform!=local)
			{
			ds.localTransform=local;
			
			#if 0
			if(ds.trackerIndex>=0)
				{
				/* Print the new local transformation for testing purposes: */
				Vector t=local.getTranslation();
				Vector ra=local.getRotation().getAxis();
				double rw=Math::deg(local.getRotation().getAngle());
				printf("New local transform for tracker %d: (%f, %f, %f), (%f, %f, %f), %f\n",ds.trackerIndex,t[0],t[1],t[2],ra[0],ra[1],ra[2],rw);
				}
			#endif
			
			/* Combine the driver's reported local transformation and the configured tracker post-transformation: */
			trackerPostTransformations[ds.trackerIndex]=local*configuredPostTransformations[ds.trackerIndex];
			}
		
		/* Get the device's driver transformation: */
		Rotation driverRot(RScalar(newPose.qRotation.x),RScalar(newPose.qRotation.y),RScalar(newPose.qRotation.z),RScalar(newPose.qRotation.w));
		Vector driverTrans(Scalar(newPose.vecPosition[0]),Scalar(newPose.vecPosition[1]),Scalar(newPose.vecPosition[2]));
		PositionOrientation driver(driverTrans,driverRot);
		
		/* Assemble the device's world-space tracking state: */
		ts.positionOrientation=world*driver;
		
		/* Reported linear velocity is in base station space, as optimal for the sensor fusion algorithm: */
		ts.linearVelocity=ds.worldTransform.transform(Vrui::VRDeviceState::TrackerState::LinearVelocity(newPose.vecVelocity));
		
		/* Reported angular velocity is in IMU space, as optimal for the sensor fusion algorithm: */
		ts.angularVelocity=ts.positionOrientation.transform(Vrui::VRDeviceState::TrackerState::AngularVelocity(newPose.vecAngularVelocity));
		
		/* Set the tracker state in the device manager, which will apply the device's local transformation and configured post-transformation: */
		setTrackerState(ds.trackerIndex,ts,poseTimeStamp);
		}
	
	/* Force a device state update if the HMD reported in: */
	if(ds.trackerIndex==0)
		updateState();
	}

void OpenVRHost::VsyncEvent(double vsyncTimeOffsetSeconds)
	{
	#ifdef VERBOSE
	printf("OpenVRHost: Ignoring vsync event with time offset %f\n",vsyncTimeOffsetSeconds);
	fflush(stdout);
	#endif
	}

void OpenVRHost::VendorSpecificEvent(uint32_t unWhichDevice,vr::EVREventType eventType,const vr::VREvent_Data_t& eventData,double eventTimeOffset)
	{
	#ifdef VERBOSE
	printf("OpenVRHost: Ignoring vendor-specific event of type %d for device %u\n",int(eventType),unWhichDevice);
	fflush(stdout);
	#endif
	}

bool OpenVRHost::IsExiting(void)
	{
	/* Return true if the driver module is shutting down: */
	return exiting;
	}

bool OpenVRHost::PollNextEvent(vr::VREvent_t* pEvent,uint32_t uncbVREvent)
	{
	/* Check if there is a pending haptic event on any haptic component: */
	for(unsigned int hapticFeatureIndex=0;hapticFeatureIndex<numHapticFeatures;++hapticFeatureIndex)
		{
		HapticEvent& he=hapticEvents[hapticFeatureIndex];
		if(he.pending)
			{
			/* Fill in the event structure: */
			pEvent->eventType=vr::VREvent_Input_HapticVibration;
			pEvent->trackedDeviceIndex=he.containerHandle-deviceHandleBase;
			pEvent->eventAgeSeconds=0.0f;
			
			vr::VREvent_HapticVibration_t& hv=pEvent->data.hapticVibration;
			hv.containerHandle=he.containerHandle;
			hv.componentHandle=he.componentHandle;
			hv.fDurationSeconds=he.duration;
			hv.fFrequency=he.frequency;
			hv.fAmplitude=he.amplitude;
			
			/* Mark the event as processed: */
			he.pending=false;
			
			return true;
			}
		}
	
	return false;
	}

void OpenVRHost::GetRawTrackedDevicePoses(float fPredictedSecondsFromNow,vr::TrackedDevicePose_t* pTrackedDevicePoseArray,uint32_t unTrackedDevicePoseArrayCount)
	{
	#ifdef VERBOSE
	printf("OpenVRHost: Ignoring GetRawTrackedDevicePoses request\n");
	fflush(stdout);
	#endif
	}

void OpenVRHost::TrackedDeviceDisplayTransformUpdated(uint32_t unWhichDevice,vr::HmdMatrix34_t eyeToHeadLeft,vr::HmdMatrix34_t eyeToHeadRight)
	{
	printf("OpenVRHost: Ignoring TrackedDeviceDisplayTransformUpdated request\n");
	fflush(stdout);
	}

/* Methods inherited from class vr::IVRResources: */

uint32_t OpenVRHost::LoadSharedResource(const char* pchResourceName,char* pchBuffer,uint32_t unBufferLen)
	{
	#ifdef VERBOSE
	printf("OpenVRHost: LoadSharedResource called with resource name %s and buffer size %u\n",pchResourceName,unBufferLen);
	fflush(stdout);
	#endif
	
	/* Extract the driver name template from the given resource name: */
	const char* driverStart=0;
	const char* driverEnd=0;
	for(const char* rnPtr=pchResourceName;*rnPtr!='\0';++rnPtr)
		{
		if(*rnPtr=='{')
			driverStart=rnPtr;
		else if(*rnPtr=='}')
			driverEnd=rnPtr+1;
		}
	
	/* Assemble the resource path based on the OpenVR root directory and the driver name: */
	std::string resourcePath=openvrRootDir;
	resourcePath.append("/drivers/");
	resourcePath.append(std::string(driverStart+1,driverEnd-1));
	resourcePath.append("/resources");
	resourcePath.append(driverEnd);
	
	/* Open the resource file: */
	try
		{
		IO::SeekableFilePtr resourceFile=IO::openSeekableFile(resourcePath.c_str());
		
		/* Check if the resource fits into the given buffer: */
		size_t resourceSize=resourceFile->getSize();
		if(resourceSize<=unBufferLen)
			{
			/* Load the resource into the buffer: */
			resourceFile->readRaw(pchBuffer,resourceSize);
			}
		
		return uint32_t(resourceSize);
		}
	catch(std::runtime_error err)
		{
		#ifdef VERBOSE
		printf("OpenVRHost::LoadSharedResource: Resource %s could not be loaded due to exception %s\n",resourcePath.c_str(),err.what());
		fflush(stdout);
		#endif
		return 0;
		}
	}

uint32_t OpenVRHost::GetResourceFullPath(const char* pchResourceName,const char* pchResourceTypeDirectory,char* pchPathBuffer,uint32_t unBufferLen)
	{
	#ifdef VERBOSE
	printf("OpenVRHost: GetResourceFullPath called with resource name %s, type directory %s and buffer size %u\n",pchResourceName,pchResourceTypeDirectory,unBufferLen);
	fflush(stdout);
	#endif
	
	pchPathBuffer[0]='\0';
	return 1;
	}

/* Methods inherited from class vr::IVRDriverManager: */

uint32_t OpenVRHost::GetDriverCount(void) const
	{
	/* There appear to be two drivers: htc and lighthouse: */
	return 2;
	}

uint32_t OpenVRHost::GetDriverName(vr::DriverId_t nDriver,char* pchValue,uint32_t unBufferSize)
	{
	/* Return one of the two hard-coded driver names: */
	static const char* driverNames[2]={"lighthouse","htc"};
	if(nDriver<2)
		{
		size_t dnLen=strlen(driverNames[nDriver])+1;
		if(dnLen<=unBufferSize)
			memcpy(pchValue,driverNames[nDriver],dnLen);
		return dnLen;
		}
	else
		return 0;
	}

vr::DriverHandle_t OpenVRHost::GetDriverHandle(const char *pchDriverName)
	{
	#ifdef VERBOSE
	printf("OpenVRDriver::GetDriverHandle called with driver name %s\n",pchDriverName);
	fflush(stdout);
	#endif
	
	/* Driver itself has a fixed handle, based on OpenVR's vrserver: */
	return driverHandle;
	}

/*************************************
Object creation/destruction functions:
*************************************/

extern "C" VRDevice* createObjectOpenVRHost(VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager,Misc::ConfigurationFile& configFile)
	{
	VRDeviceManager* deviceManager=static_cast<VRDeviceManager::DeviceFactoryManager*>(factoryManager)->getDeviceManager();
	return new OpenVRHost(factory,deviceManager,configFile);
	}

extern "C" void destroyObjectOpenVRHost(VRDevice* device,VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager)
	{
	delete device;
	}
