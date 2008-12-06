/***********************************************************************
InputDeviceDataSaver - Class to save input device data to a file for
later playback.
Copyright (c) 2004-2005 Oliver Kreylos

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

#include <iostream>
#include <Misc/CreateNumberedFileName.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Sound/SoundDataFormat.h>
#include <Sound/SoundRecorder.h>
#include <Vrui/Geometry.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceManager.h>

#include <Vrui/InputDeviceDataSaver.h>

namespace Vrui {

/*************************************
Methods of class InputDeviceDataSaver:
*************************************/

std::string InputDeviceDataSaver::getInputDeviceDataFileName(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Retrieve the base file name: */
	std::string inputDeviceDataFileName=configFileSection.retrieveString("./inputDeviceDataFileName");
	
	/* Make the file name unique: */
	char numberedFileName[1024];
	Misc::createNumberedFileName(inputDeviceDataFileName.c_str(),4,numberedFileName);
	return numberedFileName;
	}

InputDeviceDataSaver::InputDeviceDataSaver(const Misc::ConfigurationFileSection& configFileSection,InputDeviceManager& inputDeviceManager)
	:inputDeviceDataFile(getInputDeviceDataFileName(configFileSection).c_str(),"wb",Misc::File::LittleEndian),
	 numInputDevices(inputDeviceManager.getNumInputDevices()),
	 inputDevices(new InputDevice*[numInputDevices]),
	 soundRecorder(0),
	 firstFrame(true)
	{
	/* Save number of input devices: */
	inputDeviceDataFile.write<int>(numInputDevices);
	
	/* Save layout of all input devices in the input device manager: */
	for(int i=0;i<numInputDevices;++i)
		{
		/* Get pointer to the input device: */
		inputDevices[i]=inputDeviceManager.getInputDevice(i);
		
		/* Save input device's layout: */
		char name[40];
		strncpy(name,inputDevices[i]->getDeviceName(),40);
		name[39]='\0';
		inputDeviceDataFile.write(name,40);
		inputDeviceDataFile.write<int>(inputDevices[i]->getTrackType());
		inputDeviceDataFile.write<int>(inputDevices[i]->getNumButtons());
		inputDeviceDataFile.write<int>(inputDevices[i]->getNumValuators());
		inputDeviceDataFile.write(inputDevices[i]->getDeviceRayDirection().getComponents(),3);
		}
	
	/* Check if the user wants to record a commentary track: */
	std::string soundFileName=configFileSection.retrieveString("./soundFileName","");
	if(soundFileName!="")
		{
		try
			{
			/* Create a sound data format for recording: */
			Sound::SoundDataFormat soundFormat;
			soundFormat.bitsPerSample=configFileSection.retrieveValue<int>("./sampleResolution",soundFormat.bitsPerSample);
			soundFormat.samplesPerFrame=configFileSection.retrieveValue<int>("./numChannels",soundFormat.samplesPerFrame);
			soundFormat.framesPerSecond=configFileSection.retrieveValue<int>("./sampleRate",soundFormat.framesPerSecond);
			
			/* Create a sound recorder for the given sound file name: */
			char numberedFileName[1024];
			soundRecorder=new Sound::SoundRecorder(soundFormat,Misc::createNumberedFileName(soundFileName.c_str(),4,numberedFileName));
			}
		catch(std::runtime_error error)
			{
			/* Print a message, but carry on: */
			std::cerr<<"InputDeviceDataSaver: Disabling sound recording due to exception "<<error.what()<<std::endl;
			}
		}
	}


InputDeviceDataSaver::~InputDeviceDataSaver(void)
	{
	delete[] inputDevices;
	delete soundRecorder;
	}

void InputDeviceDataSaver::saveCurrentState(double currentTimeStamp)
	{
	if(firstFrame)
		{
		if(soundRecorder!=0)
			soundRecorder->start();
		firstFrame=false;
		}
	
	/* Write current time stamp: */
	inputDeviceDataFile.write(currentTimeStamp);
	
	/* Write state of all input devices: */
	for(int i=0;i<numInputDevices;++i)
		{
		/* Write input device's tracker state: */
		if(inputDevices[i]->getTrackType()!=InputDevice::TRACK_NONE)
			{
			const TrackerState& t=inputDevices[i]->getTransformation();
			inputDeviceDataFile.write(t.getTranslation().getComponents(),3);
			inputDeviceDataFile.write(t.getRotation().getQuaternion(),4);
			}
		
		/* Write input device's button states: */
		for(int j=0;j<inputDevices[i]->getNumButtons();++j)
			{
			int buttonState=inputDevices[i]->getButtonState(j);
			inputDeviceDataFile.write(buttonState);
			}
		
		/* Write input device's valuator states: */
		for(int j=0;j<inputDevices[i]->getNumValuators();++j)
			{
			double valuatorState=inputDevices[i]->getValuator(j);
			inputDeviceDataFile.write(valuatorState);
			}
		}
	}

}
