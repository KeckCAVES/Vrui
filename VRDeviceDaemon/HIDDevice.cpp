/***********************************************************************
HIDDevice - VR device driver class for generic input devices supported
by the Linux or MacOS X HID event interface. Reports buttons and
absolute axes.
Copyright (c) 2004-2005 Oliver Kreylos
MacOS X additions copyright (c) 2006 Braden Pellett

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

#include "HIDDevice.h"

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>

#include "VRDeviceManager.h"

namespace Misc {

/**************
Helper classes:
**************/

template <>
class ValueCoder<HIDDevice::AxisConverter>
	{
	/* Methods: */
	public:
	static std::string encode(const HIDDevice::AxisConverter& v)
		{
		return v.encode();
		}
	static HIDDevice::AxisConverter decode(const char* start,const char* end,const char** decodeEnd =0)
		{
		return HIDDevice::AxisConverter(start,end,decodeEnd);
		}
	};

}

/*****************************************
Methods of class HIDDevice::AxisConverter:
*****************************************/

HIDDevice::AxisConverter::AxisConverter(const char* start,const char* end,const char** decodeEnd)
	{
	/* Decode string as vector of floats containing (min, max, center, flat): */
	std::vector<float> values=Misc::ValueCoder<std::vector<float> >::decode(start,end,decodeEnd);
	if(values.size()!=4)
		throw Misc::DecodingError(std::string("Wrong number of elements in ")+std::string(start,end));
	
	/* Initialize the converter: */
	init(values[0],values[1],values[2],values[3]);
	}

void HIDDevice::AxisConverter::init(float min,float max,float center,float flat)
	{
	if(min>max)
		flat=-flat;
	negMin=center-flat;
	negMax=min;
	negFactor=1.0f/(negMin-negMax);
	posMin=center+flat;
	posMax=max;
	posFactor=1.0f/(posMax-posMin);
	
	printf("%f, %f, %f, %f, %f, %f\n",negMax,negMin,negFactor,posMin,posMax,posFactor);
	}

std::string HIDDevice::AxisConverter::encode(void) const
	{
	printf("%f, %f, %f, %f, %f, %f\n",negMax,negMin,negFactor,posMin,posMax,posFactor);
	
	std::vector<float> values;
	values.push_back(negMax);
	values.push_back(posMax);
	values.push_back((negMax+posMax)*0.5f);
	values.push_back(Math::abs(posMin-negMin)*0.5f);
	return Misc::ValueCoder<std::vector<float> >::encode(values);
	}

/*************************
System specific functions:
*************************/

#if defined (__LINUX__)
  #include "HIDDevice.Linux.cpp"
#elif defined (__DARWIN__)
  #include "HIDDevice.MacOSX.cpp"
#endif

/*************************************
Object creation/destruction functions:
*************************************/

extern "C" VRDevice* createObjectHIDDevice(VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager,Misc::ConfigurationFile& configFile)
	{
	VRDeviceManager* deviceManager=static_cast<VRDeviceManager::DeviceFactoryManager*>(factoryManager)->getDeviceManager();
	return new HIDDevice(factory,deviceManager,configFile);
	}

extern "C" void destroyObjectHIDDevice(VRDevice* device,VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager)
	{
	delete device;
	}
