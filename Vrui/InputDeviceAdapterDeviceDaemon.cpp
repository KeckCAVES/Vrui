/***********************************************************************
InputDeviceAdapterDeviceDaemon - Class to convert from Vrui's own
distributed device driver architecture to Vrui's internal device
representation.
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

#include <Vrui/InputDevice.h>
#include <Vrui/Vrui.h>

#include <Vrui/InputDeviceAdapterDeviceDaemon.h>

namespace Vrui {

/***********************************************
Methods of class InputDeviceAdapterDeviceDaemon:
***********************************************/

void InputDeviceAdapterDeviceDaemon::packetNotificationCallback(VRDeviceClient* client,void* userData)
	{
	requestUpdate();
	}

InputDeviceAdapterDeviceDaemon::InputDeviceAdapterDeviceDaemon(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection)
	:InputDeviceAdapterIndexMap(sInputDeviceManager),
	 deviceClient(configFileSection)
	{
	/* Initialize input device adapter: */
	InputDeviceAdapterIndexMap::initializeAdapter(deviceClient.getState().getNumTrackers(),deviceClient.getState().getNumButtons(),deviceClient.getState().getNumValuators(),configFileSection);
	
	/* Start VR devices: */
	deviceClient.enablePacketNotificationCB(packetNotificationCallback,0);
	deviceClient.activate();
	deviceClient.startStream();
	}

InputDeviceAdapterDeviceDaemon::~InputDeviceAdapterDeviceDaemon(void)
	{
	/* Stop VR devices: */
	deviceClient.stopStream();
	deviceClient.deactivate();
	deviceClient.disablePacketNotificationCB();
	}

void InputDeviceAdapterDeviceDaemon::updateInputDevices(void)
	{
	deviceClient.lockState();
	const VRDeviceState& state=deviceClient.getState();
	for(int deviceIndex=0;deviceIndex<numInputDevices;++deviceIndex)
		{
		/* Get pointer to the input device: */
		InputDevice* device=inputDevices[deviceIndex];
		
		/* Don't update tracker-related state for devices that are not tracked: */
		if(trackerIndexMapping[deviceIndex]>=0)
			{
			/* Get device's tracker state from VR device client: */
			const VRDeviceState::TrackerState& ts=state.getTrackerState(trackerIndexMapping[deviceIndex]);
			
			/* Set device's transformation: */
			device->setTransformation(TrackerState(ts.positionOrientation));
			
			/* Set device's linear and angular velocities: */
			device->setLinearVelocity(Vector(ts.linearVelocity));
			device->setAngularVelocity(Vector(ts.angularVelocity));
			}
		
		/* Update button states: */
		for(int i=0;i<device->getNumButtons();++i)
			device->setButtonState(i,state.getButtonState(buttonIndexMapping[deviceIndex][i]));
		
		/* Update valuator states: */
		for(int i=0;i<device->getNumValuators();++i)
			device->setValuator(i,state.getValuatorState(valuatorIndexMapping[deviceIndex][i]));
		}
	
	deviceClient.unlockState();
	}

}
