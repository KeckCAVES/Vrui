/***********************************************************************
PrintInputDeviceDataFile - Program to print the contents of a previously
saved input device data file in the format used by Vrui's
InputDeviceDataSaver and InputDeviceAdapterPlayback classes.
Copyright (c) 2008-2010 Oliver Kreylos

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
#include <iomanip>
#include <Misc/File.h>
#include <Geometry/Vector.h>
#include <Vrui/Geometry.h>
#include <Vrui/InputDevice.h>

/**************
Helper classes:
**************/

struct DeviceFileHeader // Structure to store device name and layout in device data files
	{
	/* Elements: */
	public:
	char name[40];
	int trackType;
	int numButtons;
	int numValuators;
	Vrui::Vector deviceRayDirection;
	};

int main(int argc,char* argv[])
	{
	/* Open the input file: */
	Misc::File inputDeviceDataFile(argv[1],"rb",Misc::File::LittleEndian);
	
	/* Read file header: */
	int numInputDevices=inputDeviceDataFile.read<int>();
	Vrui::InputDevice** inputDevices=new Vrui::InputDevice*[numInputDevices];
	
	/* Initialize devices: */
	for(int i=0;i<numInputDevices;++i)
		{
		/* Read device's layout from file: */
		DeviceFileHeader header;
		inputDeviceDataFile.read(header.name,sizeof(header.name));
		inputDeviceDataFile.read(header.trackType);
		inputDeviceDataFile.read(header.numButtons);
		inputDeviceDataFile.read(header.numValuators);
		inputDeviceDataFile.read(header.deviceRayDirection.getComponents(),3);
		
		/* Create new input device: */
		Vrui::InputDevice* newDevice=new Vrui::InputDevice;
		newDevice->set(header.name,header.trackType,header.numButtons,header.numValuators);
		newDevice->setDeviceRayDirection(header.deviceRayDirection);
		
		/* Store the input device: */
		inputDevices[i]=newDevice;
		}
	
	/* Read all data frames from the input device data file: */
	while(true)
		{
		/* Read the next time stamp: */
		double timeStamp;
		try
			{
			timeStamp=inputDeviceDataFile.read<double>();
			}
		catch(Misc::File::ReadError)
			{
			/* At end of file */
			break;
			}
		
		std::cout<<"Time stamp: "<<std::fixed<<std::setw(8)<<std::setprecision(3)<<timeStamp;
		
		/* Read data for all input devices: */
		for(int device=0;device<numInputDevices;++device)
			{
			/* Update tracker state: */
			if(inputDevices[device]->getTrackType()!=Vrui::InputDevice::TRACK_NONE)
				{
				Vrui::TrackerState::Vector translation;
				inputDeviceDataFile.read(translation.getComponents(),3);
				Vrui::Scalar quat[4];
				inputDeviceDataFile.read(quat,4);
				inputDevices[device]->setTransformation(Vrui::TrackerState(translation,Vrui::TrackerState::Rotation::fromQuaternion(quat)));
				}
			
			/* Update button states: */
			for(int i=0;i<inputDevices[device]->getNumButtons();++i)
				{
				int buttonState=inputDeviceDataFile.read<int>();
				inputDevices[device]->setButtonState(i,buttonState);
				}
			
			/* Update valuator states: */
			for(int i=0;i<inputDevices[device]->getNumValuators();++i)
				{
				double valuatorState=inputDeviceDataFile.read<double>();
				inputDevices[device]->setValuator(i,valuatorState);
				}
			}
		
		std::cout<<std::endl;
		}
	
	return 0;
	}
