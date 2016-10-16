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

#include <VRDeviceDaemon/VRDevices/OpenVRHost.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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
	return 1;
	}

int SDL_GetCurrentDisplayMode(int displayIndex,SDL_DisplayMode* mode)
	{
	memset(mode,0,sizeof(SDL_DisplayMode));
	mode->format=0x16161804U; // Hard-coded for SDL_PIXELFORMAT_RGB888
	mode->w=2160;
	mode->h=1200;
	mode->refresh_rate=89;
	mode->driverdata=0;
	
	return 0;
	}

int SDL_GetDisplayBounds(int displayIndex,SDL_Rect* rect)
	{
	rect->x=0;
	rect->y=0;
	rect->w=2160;
	rect->h=1200;
	
	return 0;
	}

const char* SDL_GetDisplayName(int displayIndex)
	{
	return "HTC Vive 5\"";
	}

namespace {

/****************************
Helper classes and functions:
****************************/

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

/****************
Logging facility:
****************/

class MyIDriverLog:public vr::IDriverLog
	{
	/* Elements: */
	private:
	bool printMessages;
	
	/* Constructors and destructors: */
	public:
	MyIDriverLog(bool sPrintMessages)
		:printMessages(sPrintMessages)
		{
		}
	
	/* Methods from vr::IDriverLog: */
	virtual void Log(const char* pchLogMessage);
	};

void MyIDriverLog::Log(const char* pchLogMessage)
	{
	if(printMessages)
		std::cout<<"OpenVRHost: Log message: "<<pchLogMessage;
	}

/*************************
Dummy VR settings manager:
*************************/

class MyIVRSettings:public vr::IVRSettings
	{
	/* Elements: */
	private:
	Misc::ConfigurationFileSection settingsSection; // Configuration file section containing driver settings
	
	/* Constructors and destructors: */
	public:
	MyIVRSettings(const Misc::ConfigurationFileSection& sSettingsSection);
	
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
	};

MyIVRSettings::MyIVRSettings(const Misc::ConfigurationFileSection& sSettingsSection)
	:settingsSection(sSettingsSection)
	{
	}

const char* MyIVRSettings::GetSettingsErrorNameFromEnum(vr::EVRSettingsError eError)
	{
	// std::cout<<"GetSettingsErrorNameFromEnum for "<<eError<<std::endl;
	return "Unknown";
	}

bool MyIVRSettings::Sync(bool bForce,vr::EVRSettingsError* peError)
	{
	std::cout<<"Sync with "<<bForce<<std::endl;
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	return false;
	}

void MyIVRSettings::SetBool(const char* pchSection,const char* pchSettingsKey,bool bValue,vr::EVRSettingsError* peError)
	{
	std::cout<<"SetBool for "<<pchSection<<"/"<<pchSettingsKey<<" with value "<<bValue<<std::endl;
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

void MyIVRSettings::SetInt32(const char* pchSection,const char* pchSettingsKey,int32_t nValue,vr::EVRSettingsError* peError)
	{
	std::cout<<"SetInt32 for "<<pchSection<<"/"<<pchSettingsKey<<" with value "<<nValue<<std::endl;
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

void MyIVRSettings::SetFloat(const char* pchSection,const char* pchSettingsKey,float flValue,vr::EVRSettingsError* peError)
	{
	std::cout<<"SetFloat for "<<pchSection<<"/"<<pchSettingsKey<<" with value "<<flValue<<std::endl;
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

void MyIVRSettings::SetString(const char* pchSection,const char* pchSettingsKey,const char* pchValue,vr::EVRSettingsError* peError)
	{
	std::cout<<"SetString for "<<pchSection<<"/"<<pchSettingsKey<<" with value "<<pchValue<<std::endl;
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

bool MyIVRSettings::GetBool(const char* pchSection,const char* pchSettingsKey,vr::EVRSettingsError* peError)
	{
	std::cout<<"GetBool for "<<pchSection<<"/"<<pchSettingsKey<<std::endl;
	Misc::ConfigurationFileSection section=settingsSection.getSection(pchSection);
	bool result=section.retrieveValue<bool>(pchSettingsKey,false);
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	return result;
	}

int32_t MyIVRSettings::GetInt32(const char* pchSection,const char*pchSettingsKey,vr::EVRSettingsError* peError)
	{
	std::cout<<"GetInt32 for "<<pchSection<<"/"<<pchSettingsKey<<std::endl;
	Misc::ConfigurationFileSection section=settingsSection.getSection(pchSection);
	int32_t result=section.retrieveValue<int>(pchSettingsKey,0);
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	return result;
	}

float MyIVRSettings::GetFloat(const char* pchSection,const char* pchSettingsKey,vr::EVRSettingsError* peError)
	{
	std::cout<<"GetFloat for "<<pchSection<<"/"<<pchSettingsKey<<std::endl;
	Misc::ConfigurationFileSection section=settingsSection.getSection(pchSection);
	float result=section.retrieveValue<float>(pchSettingsKey,0.0f);
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	return result;
	}

void MyIVRSettings::GetString(const char* pchSection,const char* pchSettingsKey,char* pchValue,uint32_t unValueLen,vr::EVRSettingsError* peError)
	{
	std::cout<<"GetString for "<<pchSection<<"/"<<pchSettingsKey<<std::endl;
	Misc::ConfigurationFileSection section=settingsSection.getSection(pchSection);
	strncpy(pchValue,section.retrieveString(pchSettingsKey,"").c_str(),unValueLen);
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

void MyIVRSettings::RemoveSection(const char* pchSection,vr::EVRSettingsError* peError)
	{
	std::cout<<"RemoveSection for section "<<pchSection<<std::endl;
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

void MyIVRSettings::RemoveKeyInSection(const char* pchSection,const char* pchSettingsKey,vr::EVRSettingsError* peError)
	{
	std::cout<<"RemoveKeyInSection for "<<pchSection<<"/"<<pchSettingsKey<<std::endl;
	if(peError!=0)
		*peError=vr::VRSettingsError_None;
	}

}

/***************************
Methods of class OpenVRHost:
***************************/

void OpenVRHost::updateHMDConfiguration(OpenVRHost::DeviceState& deviceState) const
	{
	/* Check if the device is an HMD: */
	vr::IVRDisplayComponent* display=reinterpret_cast<vr::IVRDisplayComponent*>(deviceState.driver->GetComponent(vr::IVRDisplayComponent_Version));
	if(display!=0&&deviceState.hmdConfiguration!=0)
		{
		vr::ETrackedPropertyError propError=vr::TrackedProp_Success;
		deviceManager->lockHmdConfigurations();
		
		std::cout<<"OpenVRHost: Updating HMD configuration..."<<std::flush;
		
		/* Update the HMD's inter-pupillary distance: */
		float newIpd=deviceState.driver->GetFloatTrackedDeviceProperty(vr::Prop_UserIpdMeters_Float,&propError);
		if(propError==vr::TrackedProp_Success&&deviceState.ipd!=newIpd&&newIpd!=0.0f)
			{
			deviceState.hmdConfiguration->setIpd(newIpd);
			deviceState.ipd=newIpd;
			}
		
		/* Update recommended pre-distortion render target size: */
		uint32_t renderTargetSize[2];
		display->GetRecommendedRenderTargetSize(&renderTargetSize[0],&renderTargetSize[1]);
		deviceState.hmdConfiguration->setRenderTargetSize(renderTargetSize[0],renderTargetSize[1]);
		
		bool distortionMeshesUpdated=false;
		for(int eyeIndex=0;eyeIndex<2;++eyeIndex)
			{
			vr::EVREye eye=eyeIndex==0?vr::Eye_Left:vr::Eye_Right;
			
			/* Update output viewport: */
			uint32_t viewport[4];
			display->GetEyeOutputViewport(eye,&viewport[0],&viewport[1],&viewport[2],&viewport[3]);
			deviceState.hmdConfiguration->setViewport(eyeIndex,viewport[0],viewport[1],viewport[2],viewport[3]);
			
			/* Update tangent-space FoV boundaries: */
			float fov[4];
			display->GetProjectionRaw(eye,&fov[0],&fov[1],&fov[2],&fov[3]);
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
					vr::DistortionCoordinates_t out=display->ComputeDistortion(eye,uf,vf);
					
					/* Check if there is actually a change: */
					distortionMeshesUpdated=distortionMeshesUpdated||out.rfRed[0]!=dmPtr->red[0]||out.rfRed[1]!=dmPtr->red[1]||out.rfGreen[0]!=dmPtr->green[0]||out.rfGreen[1]!=dmPtr->green[1]||out.rfBlue[0]!=dmPtr->blue[0]||out.rfBlue[1]!=dmPtr->blue[1];
					for(int i=0;i<2;++i)
						{
						dmPtr->red[i]=Misc::Float32(out.rfRed[i]);
						dmPtr->green[i]=Misc::Float32(out.rfGreen[i]);
						dmPtr->blue[i]=Misc::Float32(out.rfBlue[i]);
						}
					}
				}
			}
		if(distortionMeshesUpdated)
			deviceState.hmdConfiguration->updateDistortionMeshes();
		
		std::cout<<" done"<<std::endl;
		
		deviceManager->updateHmdConfiguration(deviceState.hmdConfiguration);
		deviceManager->unlockHmdConfigurations();
		}
	}

void OpenVRHost::deviceThreadMethod(void)
	{
	while(true)
		{
		openvrServerInterface->RunFrame();
		usleep(threadWaitTime);
		}
	}

OpenVRHost::OpenVRHost(VRDevice::Factory* sFactory,VRDeviceManager* sDeviceManager,Misc::ConfigurationFile& configFile)
	:VRDevice(sFactory,sDeviceManager,configFile),
	 openvrDriverDsoHandle(0),
	 openvrServerInterface(0),openvrLog(0),openvrSettings(0),
	 deviceStates(0),
	 numConnectedDevices(0),
	 proximitySensor(false)
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
	
	/* Retrieve the name of the OpenVR device driver: */
	std::string openvrDriverName=configFile.retrieveString("./openvrDriverName","lighthouse");
	
	/* Retrieve the directory containing the OpenVR device driver: */
	std::string openvrDriverRootDir=VRDEVICEDAEMON_CONFIG_OPENVRHOST_STEAMVRDIR;
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
	
	/*********************************************************************
	Second initialization step: Initialize the server-side interface of
	the OpenVR driver contained in the shared library.
	*********************************************************************/
	
	/* Get a pointer to the server-side driver object: */
	int error=0;
	openvrServerInterface=reinterpret_cast<vr::IServerTrackedDeviceProvider*>(HmdDriverFactory(vr::IServerTrackedDeviceProvider_Version,&error));
	if(openvrServerInterface==0)
		{
		dlclose(openvrDriverDsoHandle);
		Misc::throwStdErr("OpenVRHost: Unable to retrieve server-side driver object due to OpenVR error %d",error);
		}
	
	/* Create log and settings managers: */
	openvrLog=new MyIDriverLog(configFile.retrieveValue<bool>("./printLogMessages",false));
	openvrSettings=new MyIVRSettings(configFile.getSection("Settings"));
	
	/* Retrieve the OpenVR device driver configuration directory: */
	std::string openvrDriverConfigDir="config/";
	openvrDriverConfigDir.append(openvrDriverName);
	openvrDriverConfigDir=configFile.retrieveString("./openvrDriverConfigDir",openvrDriverConfigDir);
	openvrDriverConfigDir=pathcat(steamRootDir,openvrDriverConfigDir);
	
	/* Initialize the server-side driver object: */
	vr::EVRInitError initError=openvrServerInterface->Init(openvrLog,this,openvrDriverConfigDir.c_str(),openvrDriverRootDir.c_str());
	if(initError!=vr::VRInitError_None)
		{
		#if 0
		/* Cannot delete these, as the base classes don't have virtual destructors: */
		delete static_cast<MyIDriverLog*>(openvrLog);
		delete static_cast<MyIVRSettings*>(openvrSettings);
		#endif
		dlclose(openvrDriverDsoHandle);
		Misc::throwStdErr("OpenVRHost: Unable to initialize server-side driver object due to OpenVR error %d",int(initError));
		}
	
	/* Read the number of distortion mesh vertices to calculate: */
	distortionMeshSize[1]=distortionMeshSize[0]=32;
	Misc::CFixedArrayValueCoder<unsigned int,2> distortionMeshSizeVC(distortionMeshSize);
	configFile.retrieveValueWC<unsigned int*>("./distortionMeshSize",distortionMeshSize,distortionMeshSizeVC);
	
	/* Read the maximum number of supported controllers: */
	unsigned int maxNumControllers=configFile.retrieveValue<unsigned int>("./maxNumControllers",2);
	unsigned int maxNumDevices=1U+maxNumControllers;
	
	/* Initialize VRDevice's device state variables: */
	setNumTrackers(maxNumDevices,configFile); // One tracker per device
	setNumButtons(maxNumControllers*5,configFile); // Five digital buttons per controller
	setNumValuators(maxNumControllers*3,configFile); // Three analog axes per controller
	
	/* Initialize OpenVR's device state array: */
	deviceStates=new DeviceState[maxNumDevices];
	for(unsigned int i=0;i<maxNumDevices;++i)
		{
		/* Copy the originally configured tracker post-transformation to manipulate it later: */
		deviceStates[i].configuredPostTransformation=trackerPostTransformations[i];
		}
	
	/* Create a virtual device for the headset: */
	Vrui::VRDeviceDescriptor* hmdVd=new Vrui::VRDeviceDescriptor(0,0);
	hmdVd->name=configFile.retrieveString("./hmdName","HMD");
	hmdVd->trackType=Vrui::VRDeviceDescriptor::TRACK_POS|Vrui::VRDeviceDescriptor::TRACK_DIR|Vrui::VRDeviceDescriptor::TRACK_ORIENT;
	hmdVd->rayDirection=Vrui::VRDeviceDescriptor::Vector(0,0,-1);
	hmdVd->rayStart=0.0f;
	hmdVd->trackerIndex=getTrackerIndex(0);
	addVirtualDevice(hmdVd);
	
	/* Default names for controller buttons and valuators: */
	static const char* defaultButtonNames[5]={"System","Menu","Grip","Touchpad","Trigger"};
	static const char* defaultValuatorNames[3]={"TouchpadX","TouchpadY","AnalogTrigger"};
	
	/* Create a set of virtual devices for the controllers: */
	std::string controllerNameTemplate=configFile.retrieveString("./controllerNameTemplate","Controller%u");
	for(unsigned int i=0;i<maxNumControllers;++i)
		{
		Vrui::VRDeviceDescriptor* vd=new Vrui::VRDeviceDescriptor(5,3);
		vd->name=Misc::stringPrintf(controllerNameTemplate.c_str(),i+1);
		vd->trackType=Vrui::VRDeviceDescriptor::TRACK_POS|Vrui::VRDeviceDescriptor::TRACK_DIR|Vrui::VRDeviceDescriptor::TRACK_ORIENT;
		vd->rayDirection=Vrui::VRDeviceDescriptor::Vector(0,0,-1);
		vd->rayStart=0.0f;
		vd->trackerIndex=getTrackerIndex(i+1);
		for(unsigned int j=0;j<5;++j)
			{
			vd->buttonNames[j]=defaultButtonNames[j];
			vd->buttonIndices[j]=getButtonIndex(i*5+j);
			}
		for(unsigned int j=0;j<3;++j)
			{
			vd->valuatorNames[j]=defaultValuatorNames[j];
			vd->valuatorIndices[j]=getValuatorIndex(i*3+j);
			}
		addVirtualDevice(vd);
		}
	
	/* Add an HMD configuration for the headset: */
	deviceStates[0].hmdConfiguration=deviceManager->addHmdConfiguration();
	deviceStates[0].hmdConfiguration->setTrackerIndex(getTrackerIndex(0));
	deviceStates[0].hmdConfiguration->setEyePos(Vrui::HMDConfiguration::Point(-deviceStates[0].ipd*0.5f,0,0),Vrui::HMDConfiguration::Point(deviceStates[0].ipd*0.5f,0,0)); // Initialize default eye positions
	deviceStates[0].hmdConfiguration->setDistortionMeshSize(distortionMeshSize[0],distortionMeshSize[1]);
	
	/*********************************************************************
	Third initialization step: Activate all currently-connected tracked
	devices.
	*********************************************************************/
	
	/* Start the device thread already to dispatch connection/disconnection messages: */
	threadWaitTime=100000U;
	startDeviceThread();
	
	/* Leave stand-by mode: */
	openvrServerInterface->LeaveStandby();
	
	/* Query the number of initially-connected devices: */
	unsigned int numDevices=openvrServerInterface->GetTrackedDeviceCount();
	std::cout<<"OpenVRHost: Activating "<<numDevices<<" tracked devices"<<std::endl;
	for(unsigned int i=0;i<numDevices;++i)
		{
		/* Get a handle for the i-th tracked device: */
		vr::ITrackedDeviceServerDriver* driver=deviceStates[numConnectedDevices].driver=openvrServerInterface->GetTrackedDeviceDriver(i);
		if(driver!=0)
			{
			/* Activate the tracked device: */
			std::cout<<"OpenVRHost: Activating tracked device "<<i<<" with ID "<<numConnectedDevices<<std::endl;
			vr::EVRInitError error=driver->Activate(numConnectedDevices);
			if(error==vr::VRInitError_None)
				{
				/* Query the configuration of a potential HMD: */
				updateHMDConfiguration(deviceStates[numConnectedDevices]);
				
				/* Query the device's battery level: */
				vr::ETrackedPropertyError error=vr::TrackedProp_Success;
				if(driver->GetBoolTrackedDeviceProperty(vr::Prop_DeviceProvidesBatteryStatus_Bool,&error)&&error==vr::TrackedProp_Success)
					{
					deviceStates[numConnectedDevices].batteryLevel=driver->GetFloatTrackedDeviceProperty(vr::Prop_DeviceBatteryPercentage_Float,&error);
					if(error!=vr::TrackedProp_Success)
						deviceStates[numConnectedDevices].batteryLevel=0.0f; // If there was an error, assume battery is empty
					}
				else
					deviceStates[numConnectedDevices].batteryLevel=1.0f; // Devices without battery are assumed fully charged
				std::cout<<"OpenVRHost: Battery level of device "<<numConnectedDevices<<" is "<<deviceStates[numConnectedDevices].batteryLevel*100.0f<<"%"<<std::endl;
				}
			else
				std::cout<<"OpenVRHost: Unable to activate tracked device "<<numConnectedDevices<<" due to OpenVR error "<<error<<std::endl;
			
			++numConnectedDevices;
			}
		else
			std::cout<<"OpenVRHost: Error retrieving tracked device "<<i<<std::endl;
		}
	}

OpenVRHost::~OpenVRHost(void)
	{
	/* Enter stand-by mode: */
	for(unsigned int i=0;i<numConnectedDevices;++i)
		{
		deviceStates[i].driver->Deactivate();
		deviceStates[i].driver->EnterStandby();
		}
	openvrServerInterface->EnterStandby();
	usleep(100000);
	openvrServerInterface->Cleanup();
	
	/* Stop the device thread: */
	stopDeviceThread();
	
	/* Delete the device state array: */
	delete[] deviceStates;
	
	#if 0
	/* Cannot delete these, as the base classes don't have virtual destructors: */
	delete static_cast<MyIDriverLog*>(openvrLog);
	delete static_cast<MyIVRSettings*>(openvrSettings);
	#endif
	
	/* Close the OpenVR device driver dso: */
	dlclose(openvrDriverDsoHandle);
	}

void OpenVRHost::start(void)
	{
	/* Give the device communication thread its full performance: */
	threadWaitTime=500U;
	}

void OpenVRHost::stop(void)
	{
	/* Reduce performance of the device communication thread: */
	threadWaitTime=100000U;
	}

bool OpenVRHost::TrackedDeviceAdded(const char* pchDeviceSerialNumber)
	{
	bool result=false;
	std::cout<<"OpenVRHost: Adding device with serial number "<<pchDeviceSerialNumber<<std::endl;
	
	/* Get a handle for the new connected device: */
	vr::ITrackedDeviceServerDriver* driver=deviceStates[numConnectedDevices].driver=openvrServerInterface->FindTrackedDeviceDriver(pchDeviceSerialNumber);
	if(driver!=0)
		{
		/* Activate the connected device: */
		vr::EVRInitError error=driver->Activate(numConnectedDevices);
		if(error==vr::VRInitError_None)
			{
			/* Query the configuration of a potential HMD: */
			updateHMDConfiguration(deviceStates[numConnectedDevices]);
			
			/* Query the device's battery level: */
			vr::ETrackedPropertyError error=vr::TrackedProp_Success;
			if(driver->GetBoolTrackedDeviceProperty(vr::Prop_DeviceProvidesBatteryStatus_Bool,&error)&&error==vr::TrackedProp_Success)
				{
				deviceStates[numConnectedDevices].batteryLevel=driver->GetFloatTrackedDeviceProperty(vr::Prop_DeviceBatteryPercentage_Float,&error);
				if(error!=vr::TrackedProp_Success)
					deviceStates[numConnectedDevices].batteryLevel=0.0f; // If there was an error, assume battery is empty
				}
			else
				deviceStates[numConnectedDevices].batteryLevel=1.0f; // Devices without battery are assumed fully charged
			std::cout<<"OpenVRHost: Battery level of device "<<numConnectedDevices<<" is "<<deviceStates[numConnectedDevices].batteryLevel*100.0f<<"%"<<std::endl;
			
			/* Start using the new device: */
			result=true;
			}
		else
			std::cout<<"OpenVRHost: Unable to activate connected device "<<numConnectedDevices<<" due to OpenVR error "<<error<<std::endl;
		
		++numConnectedDevices;
		}
	else
		std::cout<<"OpenVRHost: Error retrieving connected device "<<numConnectedDevices<<std::endl;
	
	return result;
	}

void OpenVRHost::TrackedDevicePoseUpdated(uint32_t unWhichDevice,const vr::DriverPose_t& newPose)
	{
	DeviceState& state=deviceStates[unWhichDevice];
	
	if(newPose.deviceIsConnected&&newPose.poseIsValid)
		{
		if(!state.connected)
			{
			std::cout<<"OpenVRHost: Device "<<unWhichDevice<<" has been connected"<<std::endl;
			state.connected=true;
			}
		
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
		if(state.worldTransform!=world)
			{
			std::cout<<"OpenVRHost: World transform for device "<<unWhichDevice<<": "<<world<<std::endl;
			state.worldTransform=world;
			}
		if(state.localTransform!=local)
			{
			std::cout<<"OpenVRHost: Local transform for device "<<unWhichDevice<<": "<<local<<std::endl;
			state.localTransform=local;
			
			/* Combine the driver's reported local transformation and the configured tracker post-transformation: */
			trackerPostTransformations[unWhichDevice]=local*state.configuredPostTransformation;
			}
		
		/* Get the device's driver transformation: */
		Rotation driverRot(RScalar(newPose.qRotation.x),RScalar(newPose.qRotation.y),RScalar(newPose.qRotation.z),RScalar(newPose.qRotation.w));
		Vector driverTrans(Scalar(newPose.vecPosition[0]),Scalar(newPose.vecPosition[1]),Scalar(newPose.vecPosition[2]));
		PositionOrientation driver(driverTrans,driverRot);
		
		/* Assemble the device's world-space tracking state: */
		ts.positionOrientation=world*driver;
		ts.linearVelocity=world.transform(Vrui::VRDeviceState::TrackerState::LinearVelocity(newPose.vecVelocity));
		ts.angularVelocity=world.transform(Vrui::VRDeviceState::TrackerState::AngularVelocity(newPose.vecAngularVelocity));
		
		/* Set the tracker state in the device manager, which will apply the device's local transformation and configured post-transformation: */
		setTrackerState(unWhichDevice,ts);
		
		/* Force a device state update if the HMD reported in: */
		if(unWhichDevice==0)
			updateState();
		}
	else
		{
		if(state.connected)
			{
			std::cout<<"OpenVRHost: Device "<<unWhichDevice<<" has been disconnected"<<std::endl;
			state.connected=false;
			}
		}
	}

void OpenVRHost::TrackedDevicePropertiesChanged(uint32_t unWhichDevice)
	{
	DeviceState& ds=deviceStates[unWhichDevice];
	
	#if 0
	std::cout<<"OpenVRHost: Changed properties on device "<<unWhichDevice<<std::endl;
	vr::ETrackedPropertyError propError=vr::TrackedProp_Success;
	int32_t deviceClass=ds.driver->GetInt32TrackedDeviceProperty(vr::Prop_DeviceClass_Int32,&propError);
	if(propError==vr::TrackedProp_Success)
		{
		std::cout<<"OpenVRHost: Device "<<unWhichDevice<<" is of class "<<deviceClass<<std::endl;
		}
	else
		std::cout<<"OpenVRHost: Error requesting device property"<<std::endl;
	#endif
	
	/* Query the configuration of a potential HMD: */
	updateHMDConfiguration(ds);
	
	/* Query the device's battery level: */
	vr::ETrackedPropertyError error=vr::TrackedProp_Success;
	if(ds.driver->GetBoolTrackedDeviceProperty(vr::Prop_DeviceProvidesBatteryStatus_Bool,&error)&&error==vr::TrackedProp_Success)
		{
		ds.batteryLevel=ds.driver->GetFloatTrackedDeviceProperty(vr::Prop_DeviceBatteryPercentage_Float,&error);
		if(error!=vr::TrackedProp_Success)
			ds.batteryLevel=0.0f; // If there was an error, assume battery is empty
		}
	else
		ds.batteryLevel=1.0f; // Devices without battery are assumed fully charged
	std::cout<<"OpenVRHost: Battery level of device "<<unWhichDevice<<" is "<<int(Math::floor(ds.batteryLevel*100.0f+0.5f))<<"%"<<std::endl;
	}

void OpenVRHost::VsyncEvent(double vsyncTimeOffsetSeconds)
	{
	std::cout<<"OpenVRHost: Vsync occurred at "<<vsyncTimeOffsetSeconds<<std::endl;
	}

void OpenVRHost::TrackedDeviceButtonPressed(uint32_t unWhichDevice,vr::EVRButtonId eButtonId,double eventTimeOffset)
	{
	// std::cout<<"Button "<<eButtonId<<" pressed on device "<<unWhichDevice<<std::endl;
	if(unWhichDevice>=1U)
		{
		int baseIndex=(int(unWhichDevice)-1)*5;
		int buttonIndex=eButtonId>=32?eButtonId-29:eButtonId;
		setButtonState(baseIndex+buttonIndex,true);
		}
	}

void OpenVRHost::TrackedDeviceButtonUnpressed(uint32_t unWhichDevice,vr::EVRButtonId eButtonId,double eventTimeOffset)
	{
	// std::cout<<"Button "<<eButtonId<<" unpressed on device "<<unWhichDevice<<std::endl;
	if(unWhichDevice>=1U)
		{
		int baseIndex=(int(unWhichDevice)-1)*5;
		int buttonIndex=eButtonId>=32?eButtonId-29:eButtonId;
		setButtonState(baseIndex+buttonIndex,false);
		}
	}

void OpenVRHost::TrackedDeviceButtonTouched(uint32_t unWhichDevice,vr::EVRButtonId eButtonId,double eventTimeOffset)
	{
	// std::cout<<"Button "<<eButtonId<<" touched on device "<<unWhichDevice<<std::endl;
	}

void OpenVRHost::TrackedDeviceButtonUntouched(uint32_t unWhichDevice,vr::EVRButtonId eButtonId,double eventTimeOffset)
	{
	// std::cout<<"Button "<<eButtonId<<" untouched on device "<<unWhichDevice<<std::endl;
	}

void OpenVRHost::TrackedDeviceAxisUpdated(uint32_t unWhichDevice,uint32_t unWhichAxis,const vr::VRControllerAxis_t& axisState)
	{
	// std::cout<<"Axis "<<unWhichAxis<<" updated on device "<<unWhichDevice<<std::endl;
	if(unWhichDevice>=1U)
		{
		int baseIndex=(int(unWhichDevice)-1)*3;
		if(unWhichAxis==0U)
			{
			setValuatorState(baseIndex+0,axisState.x);
			setValuatorState(baseIndex+1,axisState.y);
			}
		else
			setValuatorState(baseIndex+2,axisState.x);
		}
	}

void OpenVRHost::MCImageUpdated(void)
	{
	std::cout<<"OpenVRHost: MC image updated"<<std::endl;
	}

vr::IVRSettings* OpenVRHost::GetSettings(const char* pchInterfaceVersion)
	{
	/* Check if the requested API version matches the compiled-in one: */
	if(strcmp(pchInterfaceVersion,vr::IVRSettings_Version)==0)
		return openvrSettings;
	else
		{
		std::cout<<"OpenVRHost: Requested settings API version does not match compiled-in version"<<std::endl;
		return 0;
		}
	}

void OpenVRHost::PhysicalIpdSet(uint32_t unWhichDevice,float fPhysicalIpdMeters)
	{
	if(deviceStates[unWhichDevice].ipd!=fPhysicalIpdMeters)
		{
		/* Export the configuration of a potential HMD: */
		updateHMDConfiguration(deviceStates[unWhichDevice]);
		
		std::cout<<"OpenVRHost: Physical IPD on device "<<unWhichDevice<<" set to "<<fPhysicalIpdMeters*1000.0f<<"mm"<<std::endl;
		}
	}

void OpenVRHost::ProximitySensorState(uint32_t unWhichDevice,bool bProximitySensorTriggered)
	{
	if(proximitySensor!=bProximitySensorTriggered)
		{
		proximitySensor=bProximitySensorTriggered;
		std::cout<<"OpenVRHost: Proximity sensor on device "<<unWhichDevice<<(bProximitySensorTriggered?" triggered":" untriggered")<<std::endl;
		}
	}

void OpenVRHost::VendorSpecificEvent(uint32_t unWhichDevice,vr::EVREventType eventType,const vr::VREvent_Data_t& eventData,double eventTimeOffset)
	{
	std::cout<<"OpenVRHost: Vendor-specific event "<<eventType<<" received on device "<<unWhichDevice<<std::endl;
	}

bool OpenVRHost::IsExiting(void)
	{
	std::cout<<"OpenVRHost: IsExiting called"<<std::endl;
	return true;
	}

bool OpenVRHost::ContinueRunFrame(void)
	{
	return false;
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
