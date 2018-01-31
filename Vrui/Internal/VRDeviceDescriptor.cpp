/***********************************************************************
VRDeviceDescriptor - Class describing the structure of an input device
represented by a VR device daemon.
Copyright (c) 2010-2017 Oliver Kreylos

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

#include <Vrui/Internal/VRDeviceDescriptor.h>

#include <Misc/SizedTypes.h>
#include <Misc/PrintInteger.h>
#include <Misc/StandardMarshallers.h>
#include <Misc/ArrayMarshallers.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <IO/File.h>
#include <Geometry/GeometryMarshallers.h>
#include <Geometry/GeometryValueCoders.h>

namespace Vrui {

/***********************************
Methods of class VRDeviceDescriptor:
***********************************/

VRDeviceDescriptor::VRDeviceDescriptor(void)
	:trackType(TRACK_NONE),rayDirection(0,1,0),rayStart(0.0f),
	 hasBattery(false),canPowerOff(false),
	 trackerIndex(-1),
	 numButtons(0),buttonNames(0),buttonIndices(0),
	 numValuators(0),valuatorNames(0),valuatorIndices(0),
	 numHapticFeatures(0),hapticFeatureNames(0),hapticFeatureIndices(0)
	{
	}

VRDeviceDescriptor::VRDeviceDescriptor(int sNumButtons,int sNumValuators,int sNumHapticFeatures)
	:trackType(TRACK_NONE),rayDirection(0,1,0),rayStart(0.0f),
	 hasBattery(false),canPowerOff(false),
	 trackerIndex(-1),
	 numButtons(sNumButtons),
	 buttonNames(numButtons>0?new std::string[numButtons]:0),buttonIndices(numButtons>0?new int[numButtons]:0),
	 numValuators(sNumValuators),
	 valuatorNames(numValuators>0?new std::string[numValuators]:0),valuatorIndices(numValuators>0?new int[numValuators]:0),
	 numHapticFeatures(sNumHapticFeatures),
	 hapticFeatureNames(numHapticFeatures>0?new std::string[numHapticFeatures]:0),hapticFeatureIndices(numHapticFeatures>0?new int[numHapticFeatures]:0)
	{
	/* Initialize button, valuator, and haptic feature indices: */
	for(int i=0;i<numButtons;++i)
		buttonIndices[i]=-1;
	for(int i=0;i<numValuators;++i)
		valuatorIndices[i]=-1;
	for(int i=0;i<numHapticFeatures;++i)
		hapticFeatureIndices[i]=-1;
	}

VRDeviceDescriptor::~VRDeviceDescriptor(void)
	{
	delete[] buttonNames;
	delete[] buttonIndices;
	delete[] valuatorNames;
	delete[] valuatorIndices;
	delete[] hapticFeatureNames;
	delete[] hapticFeatureIndices;
	}

void VRDeviceDescriptor::write(IO::File& sink,unsigned int protocolVersion) const
	{
	Misc::Marshaller<std::string>::write(name,sink);
	sink.write<Misc::SInt32>(trackType);
	Misc::Marshaller<Vector>::write(rayDirection,sink);
	sink.write<Misc::Float32>(rayStart);
	sink.write<Misc::SInt32>(trackerIndex);
	sink.write<Misc::SInt32>(numButtons);
	if(numButtons>0)
		{
		Misc::FixedArrayMarshaller<std::string>::write(buttonNames,numButtons,sink);
		Misc::FixedArrayMarshaller<Misc::SInt32>::write(buttonIndices,numButtons,sink);
		}
	sink.write<Misc::SInt32>(numValuators);
	if(numValuators>0)
		{
		Misc::FixedArrayMarshaller<std::string>::write(valuatorNames,numValuators,sink);
		Misc::FixedArrayMarshaller<Misc::SInt32>::write(valuatorIndices,numValuators,sink);
		}
	if(protocolVersion>=5U)
		sink.write<Misc::UInt8>(hasBattery);
	if(protocolVersion>=6U)
		{
		sink.write<Misc::UInt8>(canPowerOff);
		sink.write<Misc::SInt32>(numHapticFeatures);
		if(numHapticFeatures>0)
			{
			Misc::FixedArrayMarshaller<std::string>::write(hapticFeatureNames,numHapticFeatures,sink);
			Misc::FixedArrayMarshaller<Misc::SInt32>::write(hapticFeatureIndices,numHapticFeatures,sink);
			}
		}
	}

void VRDeviceDescriptor::read(IO::File& source,unsigned int protocolVersion)
	{
	name=Misc::Marshaller<std::string>::read(source);
	trackType=source.read<Misc::SInt32>();
	rayDirection=Vector(Misc::Marshaller<Geometry::Vector<Misc::Float32,3> >::read(source));
	rayStart=source.read<Misc::Float32>();
	trackerIndex=source.read<Misc::SInt32>();
	
	numButtons=source.read<Misc::SInt32>();
	delete[] buttonNames;
	buttonNames=0;
	delete[] buttonIndices;
	buttonIndices=0;
	if(numButtons>0)
		{
		buttonNames=new std::string[numButtons];
		Misc::FixedArrayMarshaller<std::string>::read(buttonNames,numButtons,source);
		buttonIndices=new int[numButtons];
		Misc::FixedArrayMarshaller<Misc::SInt32>::read(buttonIndices,numButtons,source);
		}
	
	numValuators=source.read<Misc::SInt32>();
	delete[] valuatorNames;
	valuatorNames=0;
	delete[] valuatorIndices;
	valuatorIndices=0;
	if(numValuators>0)
		{
		valuatorNames=new std::string[numValuators];
		Misc::FixedArrayMarshaller<std::string>::read(valuatorNames,numValuators,source);
		valuatorIndices=new int[numValuators];
		Misc::FixedArrayMarshaller<Misc::SInt32>::read(valuatorIndices,numValuators,source);
		}
	
	if(protocolVersion>=5U)
		hasBattery=source.read<Misc::UInt8>();
	
	delete[] hapticFeatureNames;
	hapticFeatureNames=0;
	delete[] hapticFeatureIndices;
	hapticFeatureIndices=0;
	if(protocolVersion>=6U)
		{
		canPowerOff=source.read<Misc::UInt8>();
		numHapticFeatures=source.read<Misc::SInt32>();
		if(numHapticFeatures>0)
			{
			hapticFeatureNames=new std::string[numHapticFeatures];
			Misc::FixedArrayMarshaller<std::string>::read(hapticFeatureNames,numHapticFeatures,source);
			hapticFeatureIndices=new int[numHapticFeatures];
			Misc::FixedArrayMarshaller<Misc::SInt32>::read(hapticFeatureIndices,numHapticFeatures,source);
			}
		}
	}

void VRDeviceDescriptor::save(Misc::ConfigurationFileSection& configFileSection) const
	{
	configFileSection.storeValue<std::string>("./name",name);
	std::string trackTypeString;
	if(trackType==(TRACK_POS|TRACK_DIR|TRACK_ORIENT))
		trackTypeString="6D";
	else if(trackType==(TRACK_POS|TRACK_DIR))
		trackTypeString="Ray";
	else if(trackType==TRACK_POS)
		trackTypeString="3D";
	else
		trackTypeString="None";
	configFileSection.storeValue<std::string>("./trackType",trackTypeString);
	if(trackType&TRACK_DIR)
		{
		configFileSection.storeValue<Vector>("./rayDirection",rayDirection);
		configFileSection.storeValue<float>("./rayStart",rayStart);
		}
	configFileSection.storeValue<bool>("./hasBattery",hasBattery);
	configFileSection.storeValue<bool>("./canPowerOff",canPowerOff);
	if(trackType&TRACK_POS)
		configFileSection.storeValue<int>("./trackerIndex",trackerIndex);
	if(numButtons>0)
		{
		configFileSection.storeValue<int>("./numButtons",numButtons);
		configFileSection.storeValueWC<std::string*>("./buttonNames",buttonNames,Misc::FixedArrayValueCoder<std::string>(buttonNames,numButtons));
		configFileSection.storeValueWC<int*>("./buttonIndices",buttonIndices,Misc::FixedArrayValueCoder<int>(buttonIndices,numButtons));
		}
	if(numValuators>0)
		{
		configFileSection.storeValue<int>("./numValuators",numValuators);
		configFileSection.storeValueWC<std::string*>("./valuatorNames",valuatorNames,Misc::FixedArrayValueCoder<std::string>(valuatorNames,numValuators));
		configFileSection.storeValueWC<int*>("./valuatorIndices",valuatorIndices,Misc::FixedArrayValueCoder<int>(valuatorIndices,numValuators));
		}
	if(numHapticFeatures>0)
		{
		configFileSection.storeValue<int>("./numHapticFeatures",numValuators);
		configFileSection.storeValueWC<std::string*>("./hapticFeatureNames",hapticFeatureNames,Misc::FixedArrayValueCoder<std::string>(hapticFeatureNames,numHapticFeatures));
		configFileSection.storeValueWC<int*>("./hapticFeatureIndices",hapticFeatureIndices,Misc::FixedArrayValueCoder<int>(hapticFeatureIndices,numHapticFeatures));
		}
	}

void VRDeviceDescriptor::load(const Misc::ConfigurationFileSection& configFileSection)
	{
	name=configFileSection.retrieveValue<std::string>("./name");
	std::string trackTypeString=configFileSection.retrieveValue<std::string>("./trackType","None");
	if(trackTypeString=="6D")
		trackType=TRACK_POS|TRACK_DIR|TRACK_ORIENT;
	else if(trackTypeString=="Ray")
		trackType=TRACK_POS|TRACK_DIR;
	else if(trackTypeString=="3D")
		trackType=TRACK_POS;
	else
		trackType=TRACK_NONE;
	rayDirection=configFileSection.retrieveValue<Vector>("./rayDirection",rayDirection);
	rayStart=configFileSection.retrieveValue<float>("./rayStart",rayStart);
	hasBattery=configFileSection.retrieveValue<bool>("./hasBattery",hasBattery);
	canPowerOff=configFileSection.retrieveValue<bool>("./canPowerOff",canPowerOff);
	if(trackType&TRACK_POS)
		trackerIndex=configFileSection.retrieveValue<int>("./trackerIndex");

	numButtons=configFileSection.retrieveValue<int>("./numButtons",0);
	delete[] buttonNames;
	buttonNames=0;
	delete[] buttonIndices;
	buttonIndices=0;
	if(numButtons>0)
		{
		buttonNames=new std::string[numButtons];
		for(int i=0;i<numButtons;++i)
			{
			buttonNames[i]="Button";
			char index[10];
			buttonNames[i].append(Misc::print(i,index+sizeof(index)-1));
			}
		Misc::DynamicArrayValueCoder<std::string> bnCoder(buttonNames,numButtons);
		configFileSection.retrieveValueWC<std::string*>("./buttonNames",buttonNames,bnCoder);
		
		buttonIndices=new int[numButtons];
		if(configFileSection.hasTag("./buttonIndexBase"))
			{
			int buttonIndexBase=configFileSection.retrieveValue<int>("./buttonIndexBase");
			for(int i=0;i<numButtons;++i)
				buttonIndices[i]=buttonIndexBase+i;
			}
		else
			{
			Misc::FixedArrayValueCoder<int> biCoder(buttonIndices,numButtons);
			configFileSection.retrieveValueWC<int*>("./buttonIndices",biCoder);
			}
		}
	
	numValuators=configFileSection.retrieveValue<int>("./numValuators",0);
	delete[] valuatorNames;
	valuatorNames=0;
	delete[] valuatorIndices;
	valuatorIndices=0;
	if(numValuators>0)
		{
		valuatorNames=new std::string[numValuators];
		for(int i=0;i<numValuators;++i)
			{
			valuatorNames[i]="Valuator";
			char index[10];
			valuatorNames[i].append(Misc::print(i,index+sizeof(index)-1));
			}
		Misc::DynamicArrayValueCoder<std::string> vnCoder(valuatorNames,numValuators);
		configFileSection.retrieveValueWC<std::string*>("./valuatorNames",valuatorNames,vnCoder);
		
		valuatorIndices=new int[numValuators];
		if(configFileSection.hasTag("./valuatorIndexBase"))
			{
			int valuatorIndexBase=configFileSection.retrieveValue<int>("./valuatorIndexBase");
			for(int i=0;i<numValuators;++i)
				valuatorIndices[i]=valuatorIndexBase+i;
			}
		else
			{
			Misc::FixedArrayValueCoder<int> viCoder(valuatorIndices,numValuators);
			configFileSection.retrieveValueWC<int*>("./valuatorIndices",viCoder);
			}
		}
	
	numHapticFeatures=configFileSection.retrieveValue<int>("./numHapticFeatures",0);
	delete[] hapticFeatureNames;
	hapticFeatureNames=0;
	delete[] hapticFeatureIndices;
	hapticFeatureIndices=0;
	if(numHapticFeatures>0)
		{
		hapticFeatureNames=new std::string[numHapticFeatures];
		for(int i=0;i<numHapticFeatures;++i)
			{
			hapticFeatureNames[i]="HapticFeature";
			char index[10];
			hapticFeatureNames[i].append(Misc::print(i,index+sizeof(index)-1));
			}
		Misc::DynamicArrayValueCoder<std::string> hfnCoder(hapticFeatureNames,numHapticFeatures);
		configFileSection.retrieveValueWC<std::string*>("./hapticFeatureNames",hapticFeatureNames,hfnCoder);
		
		hapticFeatureIndices=new int[numHapticFeatures];
		if(configFileSection.hasTag("./hapticFeatureIndexBase"))
			{
			int hapticFeatureIndexBase=configFileSection.retrieveValue<int>("./hapticFeatureIndexBase");
			for(int i=0;i<numHapticFeatures;++i)
				hapticFeatureIndices[i]=hapticFeatureIndexBase+i;
			}
		else
			{
			Misc::FixedArrayValueCoder<int> hfiCoder(hapticFeatureIndices,numHapticFeatures);
			configFileSection.retrieveValueWC<int*>("./hapticFeatureIndices",hfiCoder);
			}
		}
	}

}
