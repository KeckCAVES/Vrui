/***********************************************************************
VRDevice - Abstract base class for hardware devices delivering
position, orientation, button events and valuator values.
Copyright (c) 2002-2005 Oliver Kreylos

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

#include "VRDevice.h"

#ifdef __SGI_IRIX__
#include <unistd.h>
#else
#include <errno.h>
#include <time.h>
#endif
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/GeometryValueCoders.h>

#include "VRFactory.h"
#include "VRCalibrator.h"
#include "VRDeviceManager.h"

/*************************
Methods of class VRDevice:
*************************/

void* VRDevice::deviceThreadMethodWrapper(void)
	{
	/* Enable immediate cancellation of this thread: */
	Threads::Thread::setCancelState(Threads::Thread::CANCEL_ENABLE);
	Threads::Thread::setCancelType(Threads::Thread::CANCEL_ASYNCHRONOUS);

	/* Call device thread method: */
	deviceThreadMethod();
	
	return 0;
	}

void VRDevice::deviceThreadMethod(void)
	{
	}

void VRDevice::delay(double seconds)
	{
	#ifdef __SGI_IRIX__
	long ticks=long(seconds*double(CLK_TCK)+0.5);
	while((ticks=sginap(ticks))!=0)
		;
	#else
	struct timespec req;
	req.tv_sec=time_t(Math::floor(seconds));
	req.tv_nsec=long((seconds-double(req.tv_sec))*1.0e9+0.5);
	struct timespec rem;
	while(nanosleep(&req,&rem)==-1&&errno==EINTR)
		req=rem;
	#endif
	}

void VRDevice::setNumTrackers(int newNumTrackers,const Misc::ConfigurationFile& configFile)
	{
	/* Set number of trackers: */
	numTrackers=newNumTrackers;
	
	/* Create tracker index mapping: */
	delete[] trackerIndices;
	trackerIndices=new int[numTrackers];
	
	/* Read base for automatic tracker index assignment: */
	int trackerIndexBase=configFile.retrieveValue<int>("./trackerIndexBase",0);
	for(int i=0;i<numTrackers;++i)
		{
		/* Read tracker index: */
		char indexTagName[40];
		snprintf(indexTagName,sizeof(indexTagName),"./trackerIndex%d",i);
		trackerIndices[i]=configFile.retrieveValue<int>(indexTagName,i+trackerIndexBase);
		}
	
	/* Initialize tracker post transformations: */
	trackerPostTransformations=new TrackerPostTransformation[numTrackers];
	for(int i=0;i<numTrackers;++i)
		{
		/* Read post transformation: */
		char transformationTagName[40];
		snprintf(transformationTagName,sizeof(transformationTagName),"./trackerPostTransformation%d",i);
		trackerPostTransformations[i]=configFile.retrieveValue<TrackerPostTransformation>(transformationTagName,TrackerPostTransformation::identity);
		}
	
	if(calibrator!=0)
		{
		/* Set the number of trackers in the calibrator: */
		calibrator->setNumTrackers(numTrackers);
		}
	}

void VRDevice::setNumButtons(int newNumButtons,const Misc::ConfigurationFile& configFile)
	{
	/* Set number of buttons: */
	numButtons=newNumButtons;
	
	/* Create button index mapping: */
	delete[] buttonIndices;
	buttonIndices=new int[numButtons];
	
	/* Read base for automatic button index assignment: */
	int buttonIndexBase=configFile.retrieveValue<int>("./buttonIndexBase",0);
	for(int i=0;i<numButtons;++i)
		{
		/* Read button index: */
		char indexTagName[40];
		snprintf(indexTagName,sizeof(indexTagName),"./buttonIndex%d",i);
		buttonIndices[i]=configFile.retrieveValue<int>(indexTagName,i+buttonIndexBase);
		}
	}

void VRDevice::setNumValuators(int newNumValuators,const Misc::ConfigurationFile& configFile)
	{
	/* Set number of valuators: */
	numValuators=newNumValuators;
	
	/* Create valuator thresholds and exponents and valuator index mapping: */
	delete[] valuatorThresholds;
	valuatorThresholds=new float[numValuators];
	delete[] valuatorExponents;
	valuatorExponents=new float[numValuators];
	delete[] valuatorIndices;
	valuatorIndices=new int[numValuators];
	
	/* Read default valuator threshold: */
	float valuatorThreshold=configFile.retrieveValue<float>("./valuatorThreshold",0.0f);
	
	/* Read default valuator exponent: */
	float valuatorExponent=configFile.retrieveValue<float>("./valuatorExponent",1.0f);
	
	/* Read base for automatic valuator index assignment: */
	int valuatorIndexBase=configFile.retrieveValue<int>("./valuatorIndexBase",0);
	for(int i=0;i<numValuators;++i)
		{
		/* Read valuator threshold: */
		char thresholdTagName[40];
		snprintf(thresholdTagName,sizeof(thresholdTagName),"./valuatorThreshold%d",i);
		valuatorThresholds[i]=configFile.retrieveValue<float>(thresholdTagName,valuatorThreshold);
		
		/* Read valuator exponent: */
		char exponentTagName[40];
		snprintf(exponentTagName,sizeof(exponentTagName),"./valuatorExponent%d",i);
		valuatorExponents[i]=configFile.retrieveValue<float>(exponentTagName,valuatorExponent);
		
		/* Read valuator index: */
		char indexTagName[40];
		snprintf(indexTagName,sizeof(indexTagName),"./valuatorIndex%d",i);
		valuatorIndices[i]=configFile.retrieveValue<int>(indexTagName,i+valuatorIndexBase);
		}
	}

void VRDevice::setTrackerState(int deviceTrackerIndex,const Vrui::VRDeviceState::TrackerState& state)
	{
	Vrui::VRDeviceState::TrackerState calibratedState=state;
	if(calibrator!=0)
		calibrator->calibrate(deviceTrackerIndex,calibratedState);
	calibratedState.positionOrientation*=trackerPostTransformations[deviceTrackerIndex];
	deviceManager->setTrackerState(trackerIndices[deviceTrackerIndex],calibratedState);
	}

void VRDevice::setButtonState(int deviceButtonIndex,Vrui::VRDeviceState::ButtonState newState)
	{
	deviceManager->setButtonState(buttonIndices[deviceButtonIndex],newState);
	}

void VRDevice::setValuatorState(int deviceValuatorIndex,Vrui::VRDeviceState::ValuatorState newState)
	{
	Vrui::VRDeviceState::ValuatorState calibratedState=newState;
	float th=valuatorThresholds[deviceValuatorIndex];
	if(calibratedState<-th)
		calibratedState=-Math::pow(-(calibratedState+th)/(1.0f-th),valuatorExponents[deviceValuatorIndex]);
	else if(calibratedState>th)
		calibratedState=Math::pow((calibratedState-th)/(1.0f-th),valuatorExponents[deviceValuatorIndex]);
	else
		calibratedState=0.0f;
	deviceManager->setValuatorState(valuatorIndices[deviceValuatorIndex],calibratedState);
	}

void VRDevice::updateState(void)
	{
	deviceManager->updateState();
	}

void VRDevice::startDeviceThread(void)
	{
	if(!active)
		{
		/* Create device communication thread: */
		deviceThread.start(this,&VRDevice::deviceThreadMethodWrapper);
		active=true;
		}
	}

void VRDevice::stopDeviceThread(void)
	{
	if(active)
		{
		/* Destroy device communication thread: */
		deviceThread.cancel();
		deviceThread.join();
		active=false;
		}
	}

VRDevice::VRDevice(VRDevice::Factory* sFactory,VRDeviceManager* sDeviceManager,Misc::ConfigurationFile& configFile)
	:factory(sFactory),
	 numTrackers(0),numButtons(0),numValuators(0),
	 trackerIndices(0),trackerPostTransformations(0),
	 buttonIndices(0),
	 valuatorThresholds(0),valuatorExponents(0),
	 valuatorIndices(0),
	 active(false),
	 deviceManager(sDeviceManager),
	 calibrator(0)
	{
	/* Check if the device has an attached calibrator: */
	std::string calibratorType=configFile.retrieveString("./calibratorType","None");
	if(calibratorType!="None")
		{
		/* Create the calibrator: */
		configFile.setCurrentSection(configFile.retrieveString("./calibratorName").c_str());
		calibrator=deviceManager->createCalibrator(calibratorType,configFile);
		configFile.setCurrentSection("..");
		}
	}

VRDevice::~VRDevice(void)
	{
	/* Stop device hardware if it's still active: */
	if(active)
		this->stop();
	
	/* Delete calibrator: */
	if(calibrator!=0)
		VRCalibrator::destroy(calibrator);
	
	/* Delete tracker post transformations: */
	delete[] trackerPostTransformations;
	
	/* Delete valuator thresholds and exponents: */
	delete[] valuatorThresholds;
	delete[] valuatorExponents;
	
	/* Delete index mappings: */
	delete[] trackerIndices;
	delete[] buttonIndices;
	delete[] valuatorIndices;
	}

void VRDevice::destroy(VRDevice* object)
	{
	object->factory->destroyObject(object);
	}

void VRDevice::start(void)
	{
	}

void VRDevice::stop(void)
	{
	}
