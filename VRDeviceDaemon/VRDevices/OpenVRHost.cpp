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
	:driver(0),display(0),
	 trackerIndex(-1),buttonIndices(0),valuatorIndexBase(0),
	 willDriftInYaw(true),isWireless(false),hasProximitySensor(false),providesBatteryStatus(false),canPowerOff(false),
	 proximitySensorState(false),hmdConfiguration(0),
	 connected(false),tracked(false)
	{
	for(int i=0;i<2;++i)
		for(int j=0;j<2;++j)
			lensCenters[i][j]=0.5f;
	}

void OpenVRHost::DeviceState::setNumButtons(int newNumButtons)
	{
	/* Allocate the button index array: */
	delete[] buttonIndices;
	buttonIndices=new int[newNumButtons];
	
	/* Initialize all button indices to "absent": */
	for(int i=0;i<newNumButtons;++i)
		buttonIndices[i]=-1;
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
		Vrui::HMDConfiguration::DistortionMeshVertex* dmPtr=deviceState.hmdConfiguration->getDistortionMesh(eyeIndex);
		for(unsigned int v=0;v<distortionMeshSize[1];++v)
			{
			float vf=float(v)/float(distortionMeshSize[1]-1);
			for(unsigned int u=0;u<distortionMeshSize[0];++u,++dmPtr)
				{
				/* Calculate the vertex's undistorted positions: */
				float uf=float(u)/float(distortionMeshSize[0]-1);
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
	 threadWaitTime(configFile.retrieveValue<unsigned int>("./threadWaitTime",100000U)),
	 printLogMessages(configFile.retrieveValue<bool>("./printLogMessages",false)),
	 configuredPostTransformations(0),hmdConfiguration(0),controllerDeviceIndices(0),trackerDeviceIndices(0),
	 deviceStates(0),
	 numConnectedDevices(0),numControllers(0),numTrackers(0),numBaseStations(0),
	 exiting(false)
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
	
	/* Read the number of distortion mesh vertices to calculate: */
	distortionMeshSize[1]=distortionMeshSize[0]=32;
	Misc::CFixedArrayValueCoder<unsigned int,2> distortionMeshSizeVC(distortionMeshSize);
	configFile.retrieveValueWC<unsigned int*>("./distortionMeshSize",distortionMeshSize,distortionMeshSizeVC);
	
	/* Read the maximum number of supported controllers, trackers, and tracking base stations: */
	maxNumControllers=configFile.retrieveValue<unsigned int>("./maxNumControllers",2);
	maxNumTrackers=configFile.retrieveValue<unsigned int>("./maxNumTrackers",0);
	maxNumBaseStations=configFile.retrieveValue<unsigned int>("./maxNumBaseStations",2);
	
	/* Initialize VRDevice's device state variables: */
	setNumTrackers(1U+maxNumControllers+maxNumTrackers,configFile); // One tracker per device
	setNumButtons(2+maxNumControllers*5,configFile); // Two buttons on headset plus five digital buttons per controller
	setNumValuators(maxNumControllers*3,configFile); // Three analog axes per controller
	
	/* Store the originally configured tracker post-transformations to manipulate them later: */
	configuredPostTransformations=new TrackerPostTransformation[getNumTrackers()];
	for(int i=0;i<getNumTrackers();++i)
		configuredPostTransformations[i]=trackerPostTransformations[i];
	
	/* Initialize OpenVR's device state array: */
	deviceStates=new DeviceState[1U+maxNumControllers+maxNumTrackers+maxNumBaseStations];
	
	/* Create a virtual device for the headset: */
	Vrui::VRDeviceDescriptor* hmdVd=new Vrui::VRDeviceDescriptor(2,0);
	hmdVd->name=configFile.retrieveString("./hmdName","HMD");
	hmdVd->trackType=Vrui::VRDeviceDescriptor::TRACK_POS|Vrui::VRDeviceDescriptor::TRACK_DIR|Vrui::VRDeviceDescriptor::TRACK_ORIENT;
	hmdVd->rayDirection=Vrui::VRDeviceDescriptor::Vector(0,0,-1);
	hmdVd->rayStart=0.0f;
	hmdVd->trackerIndex=getTrackerIndex(0);
	hmdVd->buttonNames[0]="Button";
	hmdVd->buttonIndices[0]=getButtonIndex(0);
	hmdVd->buttonNames[1]="FaceDetector";
	hmdVd->buttonIndices[1]=getButtonIndex(1);
	hmdDeviceIndex=addVirtualDevice(hmdVd);
	
	/* Add an HMD configuration for the headset: */
	hmdConfiguration=deviceManager->addHmdConfiguration();
	hmdConfiguration->setTrackerIndex(getTrackerIndex(0));
	hmdConfiguration->setEyePos(Vrui::HMDConfiguration::Point(-0.0635f*0.5f,0,0),Vrui::HMDConfiguration::Point(0.0635f*0.5f,0,0)); // Initialize default eye positions
	hmdConfiguration->setDistortionMeshSize(distortionMeshSize[0],distortionMeshSize[1]);
	
	/* Default names for controller buttons and valuators: */
	static const char* defaultButtonNames[5]={"System","Menu","Grip","Touchpad","Trigger"};
	static const char* defaultValuatorNames[3]={"TouchpadX","TouchpadY","AnalogTrigger"};
	
	/* Create a set of virtual devices for the controllers: */
	controllerDeviceIndices=new unsigned int[maxNumControllers];
	std::string controllerNameTemplate=configFile.retrieveString("./controllerNameTemplate","Controller%u");
	for(unsigned int i=0;i<maxNumControllers;++i)
		{
		Vrui::VRDeviceDescriptor* vd=new Vrui::VRDeviceDescriptor(5,3);
		vd->name=Misc::stringPrintf(controllerNameTemplate.c_str(),1U+i);
		vd->trackType=Vrui::VRDeviceDescriptor::TRACK_POS|Vrui::VRDeviceDescriptor::TRACK_DIR|Vrui::VRDeviceDescriptor::TRACK_ORIENT;
		vd->rayDirection=Vrui::VRDeviceDescriptor::Vector(0,0,-1);
		vd->rayStart=0.0f;
		vd->hasBattery=true;
		vd->trackerIndex=getTrackerIndex(1U+i);
		for(unsigned int j=0;j<5;++j)
			{
			vd->buttonNames[j]=defaultButtonNames[j];
			vd->buttonIndices[j]=getButtonIndex(2U+i*5+j);
			}
		for(unsigned int j=0;j<3;++j)
			{
			vd->valuatorNames[j]=defaultValuatorNames[j];
			vd->valuatorIndices[j]=getValuatorIndex(i*3+j);
			}
		controllerDeviceIndices[i]=addVirtualDevice(vd);
		}
	
	/* Create a set of virtual devices for the generic trackers: */
	trackerDeviceIndices=new unsigned int[maxNumTrackers];
	std::string trackerNameTemplate=configFile.retrieveString("./trackerNameTemplate","Tracker%u");
	for(unsigned int i=0;i<maxNumTrackers;++i)
		{
		Vrui::VRDeviceDescriptor* vd=new Vrui::VRDeviceDescriptor(0,0);
		vd->name=Misc::stringPrintf(trackerNameTemplate.c_str(),1U+i);
		vd->trackType=Vrui::VRDeviceDescriptor::TRACK_POS|Vrui::VRDeviceDescriptor::TRACK_DIR|Vrui::VRDeviceDescriptor::TRACK_ORIENT;
		vd->rayDirection=Vrui::VRDeviceDescriptor::Vector(0,0,-1);
		vd->rayStart=0.0f;
		vd->hasBattery=true;
		vd->trackerIndex=getTrackerIndex(1U+maxNumControllers+i);
		trackerDeviceIndices[i]=addVirtualDevice(vd);
		}
	}

OpenVRHost::~OpenVRHost(void)
	{
	/* Enter stand-by mode: */
	#ifdef VERBOSE
	printf("OpenVRHost: Powering down devices\n");
	fflush(stdout);
	#endif
	exiting=true;
	for(unsigned int i=0;i<numConnectedDevices;++i)
		{
		deviceStates[i].driver->Deactivate();
		deviceStates[i].driver->EnterStandby();
		}
	openvrTrackedDeviceProvider->EnterStandby();
	usleep(1000000);
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
	delete[] controllerDeviceIndices;
	delete[] trackerDeviceIndices;
	
	/* Delete the device state array: */
	for(unsigned int i=0;i<1U+maxNumControllers+maxNumTrackers+maxNumBaseStations;++i)
		delete[] deviceStates[i].buttonIndices;
	delete[] deviceStates;
	
	/* Close the OpenVR device driver dso: */
	dlclose(openvrDriverDsoHandle);
	}

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

void* OpenVRHost::GetGenericInterface(const char* pchInterfaceVersion,vr::EVRInitError* peError)
	{
	if(peError!=0)
		*peError=vr::VRInitError_None;
	
	/* Cast the driver module object to the requested type and return it: */
	if(strcmp(pchInterfaceVersion,vr::IVRServerDriverHost_Version)==0)
		return static_cast<vr::IVRServerDriverHost*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRSettings_Version)==0)
		return static_cast<vr::IVRSettings*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRProperties_Version)==0)
		return static_cast<vr::IVRProperties*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRDriverLog_Version)==0)
		return static_cast<vr::IVRDriverLog*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRDriverManager_Version)==0)
		return static_cast<vr::IVRDriverManager*>(this);
	else if(strcmp(pchInterfaceVersion,vr::IVRResources_Version)==0)
		return static_cast<vr::IVRResources*>(this);
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
	return 512;
	}

bool OpenVRHost::TrackedDeviceAdded(const char* pchDeviceSerialNumber,vr::ETrackedDeviceClass eDeviceClass,vr::ITrackedDeviceServerDriver* pDriver)
	{
	#ifdef VERBOSE
	/* Determine the new device's class: */
	const char* newDeviceClass;
	switch(eDeviceClass)
		{
		case vr::TrackedDeviceClass_Invalid:
			newDeviceClass="invalid tracked device";
			break;
		
		case vr::TrackedDeviceClass_HMD:
			newDeviceClass="head-mounted display";
			break;
		
		case vr::TrackedDeviceClass_Controller:
			newDeviceClass="controller";
			break;
		
		case vr::TrackedDeviceClass_GenericTracker:
			newDeviceClass="generic tracker";
			break;
		
		case vr::TrackedDeviceClass_TrackingReference:
			newDeviceClass="tracking base station";
			break;
		
		default:
			newDeviceClass="unknown device";
		}
	#endif
	
	/* Grab the next free device state structure: */
	DeviceState& ds=deviceStates[numConnectedDevices];
	ds.serialNumber=pchDeviceSerialNumber;
	ds.driver=pDriver;
	
	/* Treat the new device based on its type: */
	bool accepted=false;
	if(eDeviceClass==vr::TrackedDeviceClass_HMD)
		{
		/* Check if this is the first headset: */
		if(hmdConfiguration!=0)
			{
			/* Initialize the device state structure: */
			ds.trackerIndex=0;
			ds.setNumButtons(32);
			ds.buttonIndices[0]=0;
			ds.buttonIndices[31]=1;
			ds.virtualDeviceIndex=hmdDeviceIndex;
			ds.hmdConfiguration=hmdConfiguration;
			hmdConfiguration=0;
			
			/* Get the headset's display component: */
			ds.display=static_cast<vr::IVRDisplayComponent*>(ds.driver->GetComponent(vr::IVRDisplayComponent_Version));
			if(ds.display!=0)
				{
				/* Initialize the HMD's configuration: */
				updateHMDConfiguration(ds);
				}
			else
				{
				#ifdef VERBOSE
				printf("OpenVRHost: Warning: Head-mounted display with serial number %s does not advertise a display\n",pchDeviceSerialNumber);
				fflush(stdout);
				#endif
				}
			
			/* Accept the headset: */
			accepted=true;
			}
		else
			{
			#ifdef VERBOSE
			printf("OpenVRHost: Warning: Ignoring extra head-mounted display with serial number %s\n",pchDeviceSerialNumber);
			fflush(stdout);
			#endif
			}
		}
	else if(eDeviceClass==vr::TrackedDeviceClass_Controller)
		{
		/* Check if there is room for more controllers: */
		if(numControllers<maxNumControllers)
			{
			/* Initialize the device state structure: */
			ds.trackerIndex=1U+numControllers;
			ds.setNumButtons(vr::k_EButton_Max);
			int baseIndex=2+numControllers*5;
			ds.buttonIndices[vr::k_EButton_System]=baseIndex+0;
			ds.buttonIndices[vr::k_EButton_ApplicationMenu]=baseIndex+1;
			ds.buttonIndices[vr::k_EButton_Grip]=baseIndex+2;
			ds.buttonIndices[vr::k_EButton_SteamVR_Touchpad]=baseIndex+3;
			ds.buttonIndices[vr::k_EButton_SteamVR_Trigger]=baseIndex+4;
			ds.valuatorIndexBase=numControllers*3;
			ds.virtualDeviceIndex=controllerDeviceIndices[numControllers];
			
			/* Accept the controller: */
			++numControllers;
			accepted=true;
			}
		else
			{
			#ifdef VERBOSE
			printf("OpenVRHost: Warning: Ignoring extra controller with serial number %s\n",pchDeviceSerialNumber);
			fflush(stdout);
			#endif
			}
		}
	else if(eDeviceClass==vr::TrackedDeviceClass_GenericTracker)
		{
		/* Check if there is room for more generic trackers: */
		if(numTrackers<maxNumTrackers)
			{
			/* Initialize the device state structure: */
			ds.trackerIndex=1U+maxNumControllers+numTrackers;
			ds.virtualDeviceIndex=trackerDeviceIndices[numTrackers];
			
			/* Accept the tracker: */
			++numTrackers;
			accepted=true;
			}
		else
			{
			#ifdef VERBOSE
			printf("OpenVRHost: Warning: Ignoring extra tracker with serial number %s\n",pchDeviceSerialNumber);
			fflush(stdout);
			#endif
			}
		}
	else if(eDeviceClass==vr::TrackedDeviceClass_TrackingReference)
		{
		/* Check if there is room for more tracking base stations: */
		if(numBaseStations<maxNumBaseStations)
			{
			/* Accept the tracking base station: */
			++numBaseStations;
			accepted=true;
			}
		else
			{
			#ifdef VERBOSE
			printf("OpenVRHost: Warning: Ignoring extra tracking base station with serial number %s\n",pchDeviceSerialNumber);
			fflush(stdout);
			#endif
			}
		}
	
	if(accepted)
		{
		/* Activate the device: */
		#ifdef VERBOSE
		printf("OpenVRHost: Activating newly-added %s with serial number %s\n",newDeviceClass,pchDeviceSerialNumber);
		fflush(stdout);
		#endif
		ds.driver->Activate(numConnectedDevices);
		++numConnectedDevices;
		
		#ifdef VERBOSE
		printf("OpenVRHost: Done activating newly-added %s with serial number %s\n",newDeviceClass,pchDeviceSerialNumber);
		fflush(stdout);
		#endif
		}
	else
		{
		/* Reject the device: */
		#ifdef VERBOSE
		printf("OpenVRHost: Rejecting newly-added %s with serial number %s\n",newDeviceClass,pchDeviceSerialNumber);
		fflush(stdout);
		#endif
		}
	
	return accepted;
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
		
		/* Force a device state update if the HMD reported in: */
		if(ds.trackerIndex==0)
			updateState();
		}
	}

void OpenVRHost::VsyncEvent(double vsyncTimeOffsetSeconds)
	{
	#ifdef VERBOSE
	printf("OpenVRHost: Ignoring vsync event with time offset %f\n",vsyncTimeOffsetSeconds);
	fflush(stdout);
	#endif
	}

void OpenVRHost::TrackedDeviceButtonPressed(uint32_t unWhichDevice,vr::EVRButtonId eButtonId,double eventTimeOffset)
	{
	/* Check if the button is valid: */
	if(unWhichDevice<numConnectedDevices&&deviceStates[unWhichDevice].buttonIndices[eButtonId]>=0)
		{
		/* Set the button state in the device manager: */
		setButtonState(deviceStates[unWhichDevice].buttonIndices[eButtonId],true);
		}
	}

void OpenVRHost::TrackedDeviceButtonUnpressed(uint32_t unWhichDevice,vr::EVRButtonId eButtonId,double eventTimeOffset)
	{
	/* Check if the button is valid: */
	if(unWhichDevice<numConnectedDevices&&deviceStates[unWhichDevice].buttonIndices[eButtonId]>=0)
		{
		/* Set the button state in the device manager: */
		setButtonState(deviceStates[unWhichDevice].buttonIndices[eButtonId],false);
		}
	}

void OpenVRHost::TrackedDeviceButtonTouched(uint32_t unWhichDevice,vr::EVRButtonId eButtonId,double eventTimeOffset)
	{
	/* Just ignore this for now */
	}

void OpenVRHost::TrackedDeviceButtonUntouched(uint32_t unWhichDevice,vr::EVRButtonId eButtonId,double eventTimeOffset)
	{
	/* Just ignore this for now */
	}

void OpenVRHost::TrackedDeviceAxisUpdated(uint32_t unWhichDevice,uint32_t unWhichAxis,const vr::VRControllerAxis_t& axisState)
	{
	/* Check if the axis is valid: */
	if(unWhichDevice<numConnectedDevices)
		{
		int baseIndex=deviceStates[unWhichDevice].valuatorIndexBase;
		if(unWhichAxis==0U)
			{
			/* Set the touchpad valuators: */
			setValuatorState(baseIndex+0,axisState.x);
			setValuatorState(baseIndex+1,axisState.y);
			}
		else
			{
			/* Set the analog trigger valuator: */
			setValuatorState(baseIndex+2,axisState.x);
			}
		}
	}

void OpenVRHost::ProximitySensorState(uint32_t unWhichDevice,bool bProximitySensorTriggered)
	{
	if(deviceStates[unWhichDevice].proximitySensorState!=bProximitySensorTriggered)
		{
		#ifdef VERBOSE
		if(bProximitySensorTriggered)
			printf("OpenVRHost: Proximity sensor on device %u triggered\n",unWhichDevice);
		else
			printf("OpenVRHost: Proximity sensor on device %u untriggered\n",unWhichDevice);
		fflush(stdout);
		#endif
		
		deviceStates[unWhichDevice].proximitySensorState=bProximitySensorTriggered;
		}
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
	return false;
	}

void OpenVRHost::GetRawTrackedDevicePoses(float fPredictedSecondsFromNow,vr::TrackedDevicePose_t* pTrackedDevicePoseArray,uint32_t unTrackedDevicePoseArrayCount)
	{
	#ifdef VERBOSE
	printf("OpenVRHost: Ignoring GetRawTrackedDevicePoses request\n");
	fflush(stdout);
	#endif
	}

namespace {

/****************
Helper functions:
****************/

const char* propertyTypeName(vr::PropertyTypeTag_t tag)
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

void storeFloat(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyContainerHandle_t minHandle,vr::PropertyContainerHandle_t maxHandle,float value,vr::PropertyRead_t& prop)
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

void storeUint64(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyContainerHandle_t minHandle,vr::PropertyContainerHandle_t maxHandle,uint64_t value,vr::PropertyRead_t& prop)
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

void storeString(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyContainerHandle_t minHandle,vr::PropertyContainerHandle_t maxHandle,const std::string& value,vr::PropertyRead_t& prop)
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

bool retrieveFloat(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyContainerHandle_t minHandle,vr::PropertyContainerHandle_t maxHandle,vr::PropertyWrite_t& prop,float& value)
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

bool retrieveInt32(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyContainerHandle_t minHandle,vr::PropertyContainerHandle_t maxHandle,vr::PropertyWrite_t& prop,int32_t& value)
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

bool retrieveUint64(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyContainerHandle_t minHandle,vr::PropertyContainerHandle_t maxHandle,vr::PropertyWrite_t& prop,uint64_t& value)
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

bool retrieveBool(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyContainerHandle_t minHandle,vr::PropertyContainerHandle_t maxHandle,vr::PropertyWrite_t& prop,bool& value)
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

bool retrieveString(vr::PropertyContainerHandle_t ulContainerHandle,vr::PropertyContainerHandle_t minHandle,vr::PropertyContainerHandle_t maxHandle,vr::PropertyWrite_t& prop,std::string& value)
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
	DeviceState& ds=deviceStates[ulContainerHandle-256];
	
	/* Process all properties in this batch: */
	vr::ETrackedPropertyError result=vr::TrackedProp_Success;
	vr::PropertyRead_t* pPtr=pBatch;
	for(unsigned int entryIndex=0;entryIndex<unBatchEntryCount;++entryIndex,++pPtr)
		{
		/* Check for known property tags: */
		switch(pPtr->prop)
			{
			#if 0
			case vr::Prop_SerialNumber_String:
				storeString(ulContainerHandle,256,255+numConnectedDevices,ds.serialNumber,*pPtr);
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
			
			case vr::Prop_LensCenterLeftU_Float:
				storeFloat(ulContainerHandle,256,256,ds.lensCenters[0][0],*pPtr);
				break;
			
			case vr::Prop_LensCenterLeftV_Float:
				storeFloat(ulContainerHandle,256,256,ds.lensCenters[0][1],*pPtr);
				break;
			
			case vr::Prop_LensCenterRightU_Float:
				storeFloat(ulContainerHandle,256,256,ds.lensCenters[1][0],*pPtr);
				break;
			
			case vr::Prop_LensCenterRightV_Float:
				storeFloat(ulContainerHandle,256,256,ds.lensCenters[1][1],*pPtr);
				break;
			
			case vr::Prop_UserConfigPath_String:
				storeString(ulContainerHandle,512,512,openvrDriverConfigDir,*pPtr);
				break;
			
			case vr::Prop_InstallPath_String:
				storeString(ulContainerHandle,512,512,openvrDriverRootDir,*pPtr);
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
	DeviceState& ds=deviceStates[ulContainerHandle-256];
	
	/* Process all properties in this batch: */
	vr::ETrackedPropertyError result=vr::TrackedProp_Success;
	vr::PropertyWrite_t* pPtr=pBatch;
	for(unsigned int entryIndex=0;entryIndex<unBatchEntryCount;++entryIndex,++pPtr)
		{
		/* Check for known property tags: */
		switch(pPtr->prop)
			{
			/* Print some interesting properties: */
			case vr::Prop_SecondsFromVsyncToPhotons_Float:
				{
				float displayDelay=0.0f;
				retrieveFloat(ulContainerHandle,256,256,*pPtr,displayDelay);
				printf("OpenVRHost: Display delay from vsync = %fms\n",displayDelay*1000.0f);
				break;
				}
			
			case vr::Prop_DisplayMCImageLeft_String:
				{
				std::string mcImage;
				retrieveString(ulContainerHandle,256,256,*pPtr,mcImage);
				printf("OpenVRHost: Left Mura correction image is %s\n",mcImage.c_str());
				break;
				}
			
			case vr::Prop_DisplayMCImageRight_String:
				{
				std::string mcImage;
				retrieveString(ulContainerHandle,256,256,*pPtr,mcImage);
				printf("OpenVRHost: Right Mura correction image is %s\n",mcImage.c_str());
				break;
				}
			
			case vr::Prop_UserHeadToEyeDepthMeters_Float:
				{
				float ed=0.0f;
				retrieveFloat(ulContainerHandle,256,256,*pPtr,ed);
				printf("OpenVRHost: User eye depth = %f\n",ed);
				break;
				}
			
			/* Extract relevant properties: */
			case vr::Prop_WillDriftInYaw_Bool:
				retrieveBool(ulContainerHandle,256,255+numConnectedDevices,*pPtr,ds.willDriftInYaw);
				break;
			
			case vr::Prop_DeviceIsWireless_Bool:
				if(retrieveBool(ulContainerHandle,256,255+numConnectedDevices,*pPtr,ds.isWireless))
					{
					/* Notify the device manager: */
					deviceManager->updateBatteryState(ds.virtualDeviceIndex,ds.batteryState);
					}
				break;
			
			case vr::Prop_DeviceIsCharging_Bool:
				{
				bool newBatteryCharging;
				if(retrieveBool(ulContainerHandle,256,255+numConnectedDevices,*pPtr,newBatteryCharging)&&ds.batteryState.charging!=newBatteryCharging)
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
				if(retrieveFloat(ulContainerHandle,256,255+numConnectedDevices,*pPtr,newBatteryLevel))
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
				retrieveBool(ulContainerHandle,256,255+numConnectedDevices,*pPtr,ds.hasProximitySensor);
				break;
			
			case vr::Prop_DeviceProvidesBatteryStatus_Bool:
				retrieveBool(ulContainerHandle,256,255+numConnectedDevices,*pPtr,ds.providesBatteryStatus);
				break;
			
			case vr::Prop_DeviceCanPowerOff_Bool:
				retrieveBool(ulContainerHandle,256,255+numConnectedDevices,*pPtr,ds.canPowerOff);
				break;
			
			case vr::Prop_LensCenterLeftU_Float:
				retrieveFloat(ulContainerHandle,256,256,*pPtr,ds.lensCenters[0][0]);
				break;
			
			case vr::Prop_LensCenterLeftV_Float:
				retrieveFloat(ulContainerHandle,256,256,*pPtr,ds.lensCenters[0][1]);
				break;
			
			case vr::Prop_LensCenterRightU_Float:
				retrieveFloat(ulContainerHandle,256,256,*pPtr,ds.lensCenters[1][0]);
				break;
			
			case vr::Prop_LensCenterRightV_Float:
				retrieveFloat(ulContainerHandle,256,256,*pPtr,ds.lensCenters[1][1]);
				break;
			
			case vr::Prop_UserIpdMeters_Float:
				{
				float ipd;
				if(retrieveFloat(ulContainerHandle,256,256,*pPtr,ipd)&&ds.hmdConfiguration!=0)
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
	/* Tracked devices have fixed handles starting at 256, based on OpenVR's vrserver: */
	return 256+nDevice;
	}

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

void OpenVRHost::Log(const char* pchLogMessage)
	{
	if(printLogMessages)
		{
		printf("OpenVRHost: Driver log: %s",pchLogMessage);
		fflush(stdout);
		}
	}

uint32_t OpenVRHost::GetDriverCount(void) const
	{
	/* There appear to be two drivers: htc and lighthouse: */
	return 2;
	}

uint32_t OpenVRHost::GetDriverName(vr::DriverId_t nDriver,char* pchValue,uint32_t unBufferSize)
	{
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

uint32_t OpenVRHost::LoadSharedResource(const char* pchResourceName,char* pchBuffer,uint32_t unBufferLen)
	{
	std::cout<<"OpenVRHost: LoadSharedResource called with resource name "<<pchResourceName<<" and buffer size "<<unBufferLen<<std::endl;
	
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
		std::cout<<"OpenVRHost::LoadSharedResource: Resource "<<resourcePath<<" could not be loaded due to exception "<<err.what()<<std::endl;
		return 0;
		}
	}

uint32_t OpenVRHost::GetResourceFullPath(const char* pchResourceName,const char* pchResourceTypeDirectory,char* pchPathBuffer,uint32_t unBufferLen)
	{
	std::cout<<"OpenVRHost: GetResourceFullPath called with resource name "<<pchResourceName<<" and type directory "<<pchResourceTypeDirectory<<std::endl;
	pchPathBuffer[0]='\0';
	return 1;
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
