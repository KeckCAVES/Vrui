/***********************************************************************
MultipipeDispatcher - Class to distribute input device and ancillary
data between the nodes in a multipipe VR environment.
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

#include <string.h>
#include <Comm/MulticastPipe.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/InputGraphManager.h>

#include <Vrui/MultipipeDispatcher.h>

namespace Vrui {

/************************************
Methods of class MultipipeDispatcher:
************************************/

MultipipeDispatcher::MultipipeDispatcher(Comm::MulticastPipe* sPipe,InputDeviceManager* sInputDeviceManager)
	:pipe(sPipe),
	 inputDeviceManager(sInputDeviceManager),
	 numInputDevices(0),
	 totalNumButtons(0),
	 totalNumValuators(0),
	 trackingStates(0),
	 buttonStates(0),
	 valuatorStates(0)
	{
	if(pipe->isMaster())
		{
		/* Distribute the input device configuration from the input device manager to all slave nodes: */
		
		/* Send number of input devices: */
		numInputDevices=inputDeviceManager->getNumInputDevices();
		pipe->write<int>(numInputDevices);
		
		/* Send configuration of all input devices: */
		for(int i=0;i<numInputDevices;++i)
			{
			/* Get pointer to input device: */
			InputDevice* id=inputDeviceManager->getInputDevice(i);
			
			/* Send input device name: */
			int nameLen=strlen(id->getDeviceName());
			pipe->write<int>(nameLen);
			pipe->write<char>(id->getDeviceName(),nameLen+1);
			
			/* Send track type: */
			pipe->write<int>(id->getTrackType());
			
			/* Send device ray direction: */
			pipe->write<Vector>(id->getDeviceRayDirection());
			
			/* Send number of buttons: */
			pipe->write<int>(id->getNumButtons());
			totalNumButtons+=id->getNumButtons();
			
			/* Send number of valuators: */
			pipe->write<int>(id->getNumValuators());
			totalNumValuators+=id->getNumValuators();
			
			/* Send device glyph: */
			pipe->write<Glyph>(inputDeviceManager->getInputGraphManager()->getInputDeviceGlyph(id));
			}
		
		pipe->finishMessage();
		}
	else
		{
		/* Receive the input device configuration from the master node: */
		
		/* Read number of input devices: */
		numInputDevices=pipe->read<int>();
		
		/* Read configuration of all input devices: */
		for(int i=0;i<numInputDevices;++i)
			{
			/* Read input device name: */
			int nameLen=pipe->read<int>();
			char* name=new char[nameLen+1];
			pipe->read<char>(name,nameLen+1);
			
			/* Read track type: */
			int trackType=pipe->read<int>();
			
			/* Read device ray direction: */
			Vector deviceRayDirection=pipe->read<Vector>();
			
			/* Read number of buttons: */
			int numButtons=pipe->read<int>();
			totalNumButtons+=numButtons;
			
			/* Read number of valuators: */
			int numValuators=pipe->read<int>();
			totalNumValuators+=numValuators;
			
			/* Read device glyph: */
			Glyph deviceGlyph=pipe->read<Glyph>();
			
			/* Create the input device: */
			InputDevice* id=inputDeviceManager->createInputDevice(name,trackType,numButtons,numValuators,true);
			delete[] name;
			
			/* Initialize the input device: */
			id->setDeviceRayDirection(deviceRayDirection);
			inputDeviceManager->getInputGraphManager()->getInputDeviceGlyph(id)=deviceGlyph;
			}
		}
	
	/* Create the input device state marshalling structures: */
	trackingStates=new InputDeviceTrackingState[numInputDevices];
	buttonStates=new bool[totalNumButtons];
	valuatorStates=new double[totalNumValuators];
	}

MultipipeDispatcher::~MultipipeDispatcher(void)
	{
	delete[] trackingStates;
	delete[] buttonStates;
	delete[] valuatorStates;
	}

void MultipipeDispatcher::dispatchState(void)
	{
	if(pipe->isMaster())
		{
		/* Gather the current state of all input devices: */
		bool* bsPtr=buttonStates;
		double* vsPtr=valuatorStates;
		for(int i=0;i<numInputDevices;++i)
			{
			InputDevice* id=inputDeviceManager->getInputDevice(i);
			trackingStates[i].transformation=id->getTransformation();
			trackingStates[i].linearVelocity=id->getLinearVelocity();
			trackingStates[i].angularVelocity=id->getAngularVelocity();
			for(int j=0;j<id->getNumButtons();++j,++bsPtr)
				*bsPtr=id->getButtonState(j);
			for(int j=0;j<id->getNumValuators();++j,++vsPtr)
				*vsPtr=id->getValuator(j);
			}
		
		/* Send the input device states to the slave nodes: */
		pipe->write<InputDeviceTrackingState>(trackingStates,numInputDevices);
		pipe->write<bool>(buttonStates,totalNumButtons);
		pipe->write<double>(valuatorStates,totalNumValuators);
		}
	else
		{
		/* Receive the input device states from the master node: */
		pipe->read<InputDeviceTrackingState>(trackingStates,numInputDevices);
		pipe->read<bool>(buttonStates,totalNumButtons);
		pipe->read<double>(valuatorStates,totalNumValuators);
		
		/* Set the state of all input devices: */
		bool* bsPtr=buttonStates;
		double* vsPtr=valuatorStates;
		for(int i=0;i<numInputDevices;++i)
			{
			InputDevice* id=inputDeviceManager->getInputDevice(i);
			id->setTransformation(trackingStates[i].transformation);
			id->setLinearVelocity(trackingStates[i].linearVelocity);
			id->setAngularVelocity(trackingStates[i].angularVelocity);
			for(int j=0;j<id->getNumButtons();++j,++bsPtr)
				id->setButtonState(j,*bsPtr);
			for(int j=0;j<id->getNumValuators();++j,++vsPtr)
				id->setValuator(j,*vsPtr);
			}
		}
	}

}
