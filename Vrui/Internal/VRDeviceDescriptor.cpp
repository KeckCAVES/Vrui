/***********************************************************************
VRDeviceDescriptor - Class describing the structure of an input device
represented by a VR device daemon.
Copyright (c) 2010-2018 Oliver Kreylos

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

void VRDeviceDescriptor::initButtons(int newNumButtons)
	{
	/* Allocate new button name and index arrays: */
	std::string* newButtonNames=newNumButtons>0?new std::string[newNumButtons]:0;
	int* newButtonIndices=newNumButtons>0?new int[newNumButtons]:0;
	
	/* Copy existing button names and indices: */
	int i;
	for(i=0;i<numButtons&&i<newNumButtons;++i)
		{
		newButtonNames[i]=buttonNames[i];
		newButtonIndices[i]=buttonIndices[i];
		}
	
	/* Create default names and invalid indices for the rest of the buttons: */
	for(;i<newNumButtons;++i)
		{
		newButtonNames[i]="Button";
		char index[10];
		newButtonNames[i].append(Misc::print(i,index+sizeof(index)-1));
		newButtonIndices[i]=-1;
		}
	
	/* Install the new button arrays: */
	numButtons=newNumButtons;
	delete[] buttonNames;
	buttonNames=newButtonNames;
	delete[] buttonIndices;
	buttonIndices=newButtonIndices;
	}

void VRDeviceDescriptor::initValuators(int newNumValuators)
	{
	/* Allocate new valuator name and index arrays: */
	std::string* newValuatorNames=newNumValuators>0?new std::string[newNumValuators]:0;
	int* newValuatorIndices=newNumValuators>0?new int[newNumValuators]:0;
	
	/* Copy existing valuator names and indices: */
	int i;
	for(i=0;i<numValuators&&i<newNumValuators;++i)
		{
		newValuatorNames[i]=valuatorNames[i];
		newValuatorIndices[i]=valuatorIndices[i];
		}
	
	/* Create default names and invalid indices for the rest of the valuators: */
	for(;i<newNumValuators;++i)
		{
		newValuatorNames[i]="Valuator";
		char index[10];
		newValuatorNames[i].append(Misc::print(i,index+sizeof(index)-1));
		newValuatorIndices[i]=-1;
		}
	
	/* Install the new valuator arrays: */
	numValuators=newNumValuators;
	delete[] valuatorNames;
	valuatorNames=newValuatorNames;
	delete[] valuatorIndices;
	valuatorIndices=newValuatorIndices;
	}

void VRDeviceDescriptor::initHapticFeatures(int newNumHapticFeatures)
	{
	/* Allocate new haptic feature name and index arrays: */
	std::string* newHapticFeatureNames=newNumHapticFeatures>0?new std::string[newNumHapticFeatures]:0;
	int* newHapticFeatureIndices=newNumHapticFeatures>0?new int[newNumHapticFeatures]:0;
	
	/* Copy existing haptic feature names and indices: */
	int i;
	for(i=0;i<numHapticFeatures&&i<newNumHapticFeatures;++i)
		{
		newHapticFeatureNames[i]=hapticFeatureNames[i];
		newHapticFeatureIndices[i]=hapticFeatureIndices[i];
		}
	
	/* Create default names and invalid indices for the rest of the haptic features: */
	for(;i<newNumHapticFeatures;++i)
		{
		newHapticFeatureNames[i]="HapticFeature";
		char index[10];
		newHapticFeatureNames[i].append(Misc::print(i,index+sizeof(index)-1));
		newHapticFeatureIndices[i]=-1;
		}
	
	/* Install the new haptic feature arrays: */
	numHapticFeatures=newNumHapticFeatures;
	delete[] hapticFeatureNames;
	hapticFeatureNames=newHapticFeatureNames;
	delete[] hapticFeatureIndices;
	hapticFeatureIndices=newHapticFeatureIndices;
	}

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
	 numButtons(0),buttonNames(0),buttonIndices(0),
	 numValuators(0),valuatorNames(0),valuatorIndices(0),
	 numHapticFeatures(0),hapticFeatureNames(0),hapticFeatureIndices(0)
	{
	/* Initialize buttons, valuators, and haptic features: */
	initButtons(sNumButtons);
	initValuators(sNumValuators);
	initHapticFeatures(sNumHapticFeatures);
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

namespace {

/****************
Helper functions:
****************/

std::string getTrackTypeName(int trackType)
	{
	if(trackType==(VRDeviceDescriptor::TRACK_POS|VRDeviceDescriptor::TRACK_DIR|VRDeviceDescriptor::TRACK_ORIENT))
		return "6D";
	else if(trackType==(VRDeviceDescriptor::TRACK_POS|VRDeviceDescriptor::TRACK_DIR))
		return "Ray";
	else if(trackType==VRDeviceDescriptor::TRACK_POS)
		return "3D";
	else
		return "None";
	}

}

void VRDeviceDescriptor::save(Misc::ConfigurationFileSection& configFileSection) const
	{
	configFileSection.storeValue<std::string>("./name",name);
	configFileSection.storeValue<std::string>("./trackType",getTrackTypeName(trackType));
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
	/* Update device name: */
	name=configFileSection.retrieveValue<std::string>("./name",name);
	
	/* Update device tracking type: */
	std::string trackTypeName=configFileSection.retrieveValue<std::string>("./trackType",getTrackTypeName(trackType));
	if(trackTypeName=="6D")
		trackType=TRACK_POS|TRACK_DIR|TRACK_ORIENT;
	else if(trackTypeName=="Ray")
		trackType=TRACK_POS|TRACK_DIR;
	else if(trackTypeName=="3D")
		trackType=TRACK_POS;
	else
		trackType=TRACK_NONE;
	
	if(trackType&TRACK_DIR)
		{
		/* Update ray definition: */
		rayDirection=configFileSection.retrieveValue<Vector>("./rayDirection",rayDirection);
		rayStart=configFileSection.retrieveValue<float>("./rayStart",rayStart);
		}
	
	/* Update other device state: */
	hasBattery=configFileSection.retrieveValue<bool>("./hasBattery",hasBattery);
	canPowerOff=configFileSection.retrieveValue<bool>("./canPowerOff",canPowerOff);
	
	/* Update or reset tracker index: */
	if(trackType&TRACK_POS)
		trackerIndex=configFileSection.retrieveValue<int>("./trackerIndex",trackerIndex);
	else
		trackerIndex=-1;
	
	/* Update number of buttons: */
	int newNumButtons=configFileSection.retrieveValue<int>("./numButtons",numButtons);
	if(numButtons!=newNumButtons)
		initButtons(newNumButtons);
	
	/* Update button arrays: */
	if(numButtons>0)
		{
		if(configFileSection.hasTag("./buttonNames"))
			{
			/* Update button names: */
			Misc::DynamicArrayValueCoder<std::string> bnCoder(buttonNames,numButtons);
			configFileSection.retrieveValueWC<std::string*>("./buttonNames",buttonNames,bnCoder);
			}
		
		if(configFileSection.hasTag("./buttonIndexBase"))
			{
			/* Create button indices using base index: */
			int buttonIndexBase=configFileSection.retrieveValue<int>("./buttonIndexBase");
			for(int i=0;i<numButtons;++i)
				buttonIndices[i]=buttonIndexBase+i;
			}
		else if(configFileSection.hasTag("./buttonIndices"))
			{
			/* Read button indices: */
			Misc::FixedArrayValueCoder<int> biCoder(buttonIndices,numButtons);
			configFileSection.retrieveValueWC<int*>("./buttonIndices",biCoder);
			}
		}
	
	/* Update number of valuators: */
	int newNumValuators=configFileSection.retrieveValue<int>("./numValuators",numValuators);
	if(numValuators!=newNumValuators)
		initValuators(newNumValuators);
	
	/* Update valuator arrays: */
	if(numValuators>0)
		{
		if(configFileSection.hasTag("./valuatorNames"))
			{
			/* Update valuator names: */
			Misc::DynamicArrayValueCoder<std::string> bnCoder(valuatorNames,numValuators);
			configFileSection.retrieveValueWC<std::string*>("./valuatorNames",valuatorNames,bnCoder);
			}
		
		if(configFileSection.hasTag("./valuatorIndexBase"))
			{
			/* Create valuator indices using base index: */
			int valuatorIndexBase=configFileSection.retrieveValue<int>("./valuatorIndexBase");
			for(int i=0;i<numValuators;++i)
				valuatorIndices[i]=valuatorIndexBase+i;
			}
		else if(configFileSection.hasTag("./valuatorIndices"))
			{
			/* Read valuator indices: */
			Misc::FixedArrayValueCoder<int> biCoder(valuatorIndices,numValuators);
			configFileSection.retrieveValueWC<int*>("./valuatorIndices",biCoder);
			}
		}
	
	/* Update number of haptic features: */
	int newNumHapticFeatures=configFileSection.retrieveValue<int>("./numHapticFeatures",numHapticFeatures);
	if(numHapticFeatures!=newNumHapticFeatures)
		initHapticFeatures(newNumHapticFeatures);
	
	/* Update haptic feature arrays: */
	if(numHapticFeatures>0)
		{
		if(configFileSection.hasTag("./hapticFeatureNames"))
			{
			/* Update haptic feature names: */
			Misc::DynamicArrayValueCoder<std::string> bnCoder(hapticFeatureNames,numHapticFeatures);
			configFileSection.retrieveValueWC<std::string*>("./hapticFeatureNames",hapticFeatureNames,bnCoder);
			}
		
		if(configFileSection.hasTag("./hapticFeatureIndexBase"))
			{
			/* Create haptic feature indices using base index: */
			int hapticFeatureIndexBase=configFileSection.retrieveValue<int>("./hapticFeatureIndexBase");
			for(int i=0;i<numHapticFeatures;++i)
				hapticFeatureIndices[i]=hapticFeatureIndexBase+i;
			}
		else if(configFileSection.hasTag("./hapticFeatureIndices"))
			{
			/* Read haptic feature indices: */
			Misc::FixedArrayValueCoder<int> biCoder(hapticFeatureIndices,numHapticFeatures);
			configFileSection.retrieveValueWC<int*>("./hapticFeatureIndices",biCoder);
			}
		}
	}

}
