/***********************************************************************
VRDeviceClient - Class encapsulating the VR device protocol's client
side.
Copyright (c) 2002-2018 Oliver Kreylos

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

#include <Vrui/Internal/VRDeviceClient.h>

#include <Misc/SizedTypes.h>
#include <Misc/Time.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Realtime/Time.h>
#include <Vrui/Internal/VRDeviceDescriptor.h>
#include <Vrui/Internal/HMDConfiguration.h>

namespace Vrui {

namespace {

/****************
Helper functions:
****************/

void adjustTrackerStateTimeStamps(VRDeviceState& state,VRDeviceState::TimeStamp timeStampDelta) // Sets tracker state time stamps to current monotonic time
	{
	/* Adjust all tracker state time stamps: */
	VRDeviceState::TimeStamp* tsPtr=state.getTrackerTimeStamps();
	for(int i=0;i<state.getNumTrackers();++i,++tsPtr)
		*tsPtr+=timeStampDelta;
	}

void setTrackerStateTimeStamps(VRDeviceState& state) // Sets tracker state time stamps to current monotonic time
	{
	/* Get the current monotonic time: */
	Realtime::TimePointMonotonic now;
	
	/* Get the lower-order bits of the microsecond time: */
	VRDeviceState::TimeStamp ts=VRDeviceState::TimeStamp(now.tv_sec*1000000+(now.tv_nsec+500)/1000);
	
	/* Set all tracker state time stamps to the current time: */
	VRDeviceState::TimeStamp* tsPtr=state.getTrackerTimeStamps();
	for(int i=0;i<state.getNumTrackers();++i,++tsPtr)
		*tsPtr=ts;
	}

}

/*******************************
Methods of class VRDeviceClient:
*******************************/

void* VRDeviceClient::streamReceiveThreadMethod(void)
	{
	Threads::Thread::setCancelState(Threads::Thread::CANCEL_ENABLE);
	
	while(true)
		{
		/* Wait for next protocol message: */
		try
			{
			VRDevicePipe::MessageIdType message=pipe.readMessage();
			if(message==VRDevicePipe::PACKET_REPLY)
				{
				/* Read server's state: */
				{
				Threads::Mutex::Lock stateLock(stateMutex);
				state.read(pipe,serverHasTimeStamps,serverHasValidFlags);
				if(!serverHasTimeStamps)
					{
					/* Set all tracker time stamps to the current local time: */
					setTrackerStateTimeStamps(state);
					}
				else if(!local)
					{
					/* Adjust all received time stamps by the client/server clock difference: */
					adjustTrackerStateTimeStamps(state,timeStampDelta);
					}
				}
				
				/* Signal packet reception: */
				packetSignalCond.broadcast();
				
				/* Invoke packet notification callback: */
				if(packetNotificationCallback!=0)
					(*packetNotificationCallback)(this);
				}
			else if(message==VRDevicePipe::BATTERYSTATE_UPDATE)
				{
				Threads::Mutex::Lock batteryStatesLock(batteryStatesMutex);
				
				/* Read the index of the device whose battery state changed and the new battery state: */
				unsigned int deviceIndex=pipe.read<Misc::UInt16>();
				batteryStates[deviceIndex].read(pipe);
				
				/* Call the battery state change callback: */
				if(batteryStateUpdatedCallback!=0)
					(*batteryStateUpdatedCallback)(deviceIndex);
				}
			else if((message&~0x7U)==VRDevicePipe::HMDCONFIG_UPDATE)
				{
				/* Read the tracker index of the updated HMD configuration: */
				Misc::UInt16 updatedTrackerIndex=pipe.read<Misc::UInt16>();
				
				/* Find the to-be-updated HMD configuration in the list: */
				unsigned index;
				for(index=0;index<numHmdConfigurations&&updatedTrackerIndex!=hmdConfigurations[index].getTrackerIndex();++index)
					;
				if(index>=numHmdConfigurations)
					{
					/* Signal a protocol error and shut down: */
					if(errorCallback!=0)
						(*errorCallback)(ProtocolError("VRDeviceClient: Mismatching message while waiting for PACKET_REPLY",this));
					connectionDead=true;
					packetSignalCond.broadcast();
					break;
					}
				
				{
				Threads::Mutex::Lock hmdConfigurationLock(hmdConfigurationMutex);
				
				/* Read updated HMD configuration from server: */
				hmdConfigurations[index].read(message,updatedTrackerIndex,pipe);
				
				/* Call the update callback: */
				if(hmdConfigurationUpdatedCallbacks[index]!=0)
					(*hmdConfigurationUpdatedCallbacks[index])(hmdConfigurations[index]);
				}
				}
			else if(message==VRDevicePipe::STOPSTREAM_REPLY)
				break;
			else
				{
				/* Signal a protocol error and shut down: */
				if(errorCallback!=0)
					(*errorCallback)(ProtocolError("VRDeviceClient: Mismatching message while waiting for PACKET_REPLY",this));
				connectionDead=true;
				packetSignalCond.broadcast();
				break;
				}
			}
		catch(std::runtime_error err)
			{
			/* Signal an error and shut down: */
			if(errorCallback!=0)
				{
				std::string msg="VRDeviceClient: Caught exception ";
				msg.append(err.what());
				(*errorCallback)(ProtocolError(msg,this));
				}
			connectionDead=true;
			packetSignalCond.broadcast();
			break;
			}
		}
	
	return 0;
	}

void VRDeviceClient::initClient(void)
	{
	/* Determine whether client and server are running on the same host: */
	local=pipe.getAddress()==pipe.getPeerAddress();
	
	/* Initiate connection: */
	pipe.writeMessage(VRDevicePipe::CONNECT_REQUEST);
	pipe.write<Misc::UInt32>(VRDevicePipe::protocolVersionNumber);
	pipe.flush();
	
	/* Wait for server's reply: */
	if(!pipe.waitForData(Misc::Time(30,0)))
		throw ProtocolError("VRDeviceClient: Timeout while waiting for CONNECT_REPLY",this);
	if(pipe.readMessage()!=VRDevicePipe::CONNECT_REPLY)
		throw ProtocolError("VRDeviceClient: Mismatching message while waiting for CONNECT_REPLY",this);
	serverProtocolVersionNumber=pipe.read<Misc::UInt32>();
	
	/* Check server version number for compatibility: */
	if(serverProtocolVersionNumber<1U||serverProtocolVersionNumber>VRDevicePipe::protocolVersionNumber)
		throw ProtocolError("VRDeviceClient: Unsupported server protocol version",this);
	
	/* Read server's layout and initialize current state: */
	state.readLayout(pipe);
	
	/* Check if the server will send virtual input device descriptors: */
	if(serverProtocolVersionNumber>=2U)
		{
		/* Read the list of virtual devices managed by the server: */
		unsigned int numVirtualDevices=pipe.read<Misc::UInt32>();
		for(unsigned int deviceIndex=0;deviceIndex<numVirtualDevices;++deviceIndex)
			{
			/* Create a new virtual input device and read its layout from the server: */
			VRDeviceDescriptor* newDevice=new VRDeviceDescriptor;
			newDevice->read(pipe,serverProtocolVersionNumber);
			
			/* Store the virtual input device: */
			virtualDevices.push_back(newDevice);
			}
		}
	
	/* Check if the server will send tracker state time stamps: */
	serverHasTimeStamps=serverProtocolVersionNumber>=3U;
	
	/* Initialize the clock offset: */
	timeStampDelta=0;
	
	/* Create an array to cache virtual input devices' battery states: */
	if(!virtualDevices.empty())
		batteryStates=new BatteryState[virtualDevices.size()];
	
	/* Check if the server maintains battery states: */
	if(serverProtocolVersionNumber>=5U)
		{
		/* Read battery states for all virtual devices: */
		for(unsigned int i=0;i<virtualDevices.size();++i)
			batteryStates[i].read(pipe);
		}
	
	/* Check if the server maintains HMD configurations: */
	if(serverProtocolVersionNumber>=4U)
		{
		/* Read the list of HMD configurations maintained by the server: */
		numHmdConfigurations=pipe.read<Misc::UInt32>();
		
		/* Read initial HMD configurations: */
		hmdConfigurations=new HMDConfiguration[numHmdConfigurations];
		for(unsigned int i=0;i<numHmdConfigurations;++i)
			{
			/* Read the update message ID (server will send it): */
			VRDevicePipe::MessageIdType messageId=pipe.readMessage();
			
			/* Read the HMD configuration's tracker index: */
			Misc::UInt16 trackerIndex=pipe.read<Misc::UInt16>();
			
			/* Read the HMD configuration: */
			hmdConfigurations[i].read(messageId,trackerIndex,pipe);
			}
		
		/* Initialize HMD configuration update callback array: */
		hmdConfigurationUpdatedCallbacks=new HMDConfigurationUpdatedCallback*[numHmdConfigurations];
		for(unsigned int i=0;i<numHmdConfigurations;++i)
			hmdConfigurationUpdatedCallbacks[i]=0;
		}
	
	/* Check if the server will send tracker valid flags: */
	serverHasValidFlags=serverProtocolVersionNumber>=5U;
	
	/* Initialize all tracker states to "valid" if the server doesn't send valid flags: */
	if(!serverHasValidFlags)
		for(int i=0;i<state.getNumTrackers();++i)
			state.setTrackerValid(i,true);
	
	/* Check if the server maintains power and haptic features: */
	if(serverProtocolVersionNumber>=6U)
		{
		/* Read the number of power and haptic features maintained by the server: */
		numPowerFeatures=pipe.read<Misc::UInt32>();
		numHapticFeatures=pipe.read<Misc::UInt32>();
		}
	}

VRDeviceClient::VRDeviceClient(const char* deviceServerName,int deviceServerPort)
	:pipe(deviceServerName,deviceServerPort),
	 serverProtocolVersionNumber(0),serverHasTimeStamps(false),
	 batteryStates(0),batteryStateUpdatedCallback(0),
	 numHmdConfigurations(0),hmdConfigurations(0),hmdConfigurationUpdatedCallbacks(0),
	 numPowerFeatures(0),numHapticFeatures(0),
	 active(false),streaming(false),connectionDead(false),
	 packetNotificationCallback(0),errorCallback(0)
	{
	initClient();
	}

VRDeviceClient::VRDeviceClient(const Misc::ConfigurationFileSection& configFileSection)
	:pipe(configFileSection.retrieveString("./serverName").c_str(),configFileSection.retrieveValue<int>("./serverPort")),
	 serverProtocolVersionNumber(0),serverHasTimeStamps(false),
	 batteryStates(0),batteryStateUpdatedCallback(0),
	 numHmdConfigurations(0),hmdConfigurations(0),hmdConfigurationUpdatedCallbacks(0),
	 numPowerFeatures(0),numHapticFeatures(0),
	 active(false),streaming(false),connectionDead(false),
	 packetNotificationCallback(0),errorCallback(0)
	{
	initClient();
	}

VRDeviceClient::~VRDeviceClient(void)
	{
	/* Leave streaming mode: */
	if(streaming)
		stopStream();
	
	/* Deactivate client: */
	if(active)
		deactivate();
	
	/* Disconnect from server: */
	pipe.writeMessage(VRDevicePipe::DISCONNECT_REQUEST);
	pipe.flush();
	
	/* Delete all callbacks: */
	delete batteryStateUpdatedCallback;
	for(unsigned int i=0;i<numHmdConfigurations;++i)
		delete hmdConfigurationUpdatedCallbacks[i];
	delete[] hmdConfigurationUpdatedCallbacks;
	delete packetNotificationCallback;
	delete errorCallback;
	
	/* Delete all virtual input devices: */
	for(std::vector<VRDeviceDescriptor*>::iterator vdIt=virtualDevices.begin();vdIt!=virtualDevices.end();++vdIt)
		delete *vdIt;
	
	/* Delete battery states and HMD configurations: */
	delete[] batteryStates;
	delete[] hmdConfigurations;
	}

const HMDConfiguration& VRDeviceClient::getHmdConfiguration(unsigned int index) const
	{
	return hmdConfigurations[index];
	}

void VRDeviceClient::activate(void)
	{
	if(!active&&!connectionDead)
		{
		pipe.writeMessage(VRDevicePipe::ACTIVATE_REQUEST);
		pipe.flush();
		active=true;
		}
	}

void VRDeviceClient::deactivate(void)
	{
	if(active)
		{
		active=false;
		if(!connectionDead)
			{
			pipe.writeMessage(VRDevicePipe::DEACTIVATE_REQUEST);
			pipe.flush();
			}
		}
	}

void VRDeviceClient::getPacket(void)
	{
	if(active)
		{
		if(streaming)
			{
			if(connectionDead)
				throw ProtocolError("VRDeviceClient: Server disconnected",this);
			
			/* Wait for arrival of next packet: */
			packetSignalCond.wait();
			if(connectionDead)
				throw ProtocolError("VRDeviceClient: Server disconnected",this);
			}
		else
			{
			/* Send packet request message: */
			pipe.writeMessage(VRDevicePipe::PACKET_REQUEST);
			pipe.flush();
			
			/* Wait for packet reply message: */
			if(!pipe.waitForData(Misc::Time(10,0))) // Throw exception if reply does not arrive in time
				{
				connectionDead=true;
				throw ProtocolError("VRDeviceClient: Timout while waiting for PACKET_REPLY",this);
				}
			if(pipe.readMessage()!=VRDevicePipe::PACKET_REPLY)
				{
				connectionDead=true;
				throw ProtocolError("VRDeviceClient: Mismatching message while waiting for PACKET_REPLY",this);
				}
			
			/* Read server's state: */
			try
				{
				Threads::Mutex::Lock stateLock(stateMutex);
				state.read(pipe,serverHasTimeStamps,serverHasValidFlags);
				if(!serverHasTimeStamps)
					{
					/* Set all tracker time stamps to the current local time: */
					setTrackerStateTimeStamps(state);
					}
				else if(!local)
					{
					/* Adjust all received time stamps by the client/server clock difference: */
					adjustTrackerStateTimeStamps(state,timeStampDelta);
					}
				}
			catch(std::runtime_error err)
				{
				/* Mark the connection as dead and re-throw the original exception: */
				connectionDead=true;
				throw;
				}
			}
		}
	}

void VRDeviceClient::powerOff(unsigned int powerFeatureIndex)
	{
	/* Check if device server supports powering off devices: */
	if(serverProtocolVersionNumber>=6U)
		{
		/* Send power off request message: */
		pipe.writeMessage(VRDevicePipe::POWEROFF_REQUEST);
		pipe.write<Misc::UInt16>(powerFeatureIndex);
		pipe.flush();
		}
	}

void VRDeviceClient::hapticTick(unsigned int hapticFeatureIndex,unsigned int duration)
	{
	/* Check if device server supports haptic feedback: */
	if(serverProtocolVersionNumber>=6U)
		{
		/* Send haptic tick request message: */
		pipe.writeMessage(VRDevicePipe::HAPTICTICK_REQUEST);
		pipe.write<Misc::UInt16>(hapticFeatureIndex);
		pipe.write<Misc::UInt16>(duration);
		pipe.flush();
		}
	}

void VRDeviceClient::setBatteryStateUpdatedCallback(VRDeviceClient::BatteryStateUpdatedCallback* newBatteryStateUpdatedCallback)
	{
	/* Replace the previous callback with the new one: */
	delete batteryStateUpdatedCallback;
	batteryStateUpdatedCallback=newBatteryStateUpdatedCallback;
	}

void VRDeviceClient::setHmdConfigurationUpdatedCallback(unsigned int trackerIndex,VRDeviceClient::HMDConfigurationUpdatedCallback* newHmdConfigurationUpdatedCallback)
	{
	/* Find the HMD configuration associated with the given tracker index: */
	unsigned int index;
	for(index=0;index<numHmdConfigurations&&trackerIndex!=hmdConfigurations[index].getTrackerIndex();++index)
		;
	if(index<numHmdConfigurations)
		{
		/* Replace the previous callback for the given tracker index with the new one: */
		delete hmdConfigurationUpdatedCallbacks[index];
		hmdConfigurationUpdatedCallbacks[index]=newHmdConfigurationUpdatedCallback;
		}
	else
		{
		/* Delete the new callback and just ignore it: */
		delete newHmdConfigurationUpdatedCallback;
		}
	}

void VRDeviceClient::startStream(VRDeviceClient::Callback* newPacketNotificationCallback,VRDeviceClient::ErrorCallback* newErrorCallback)
	{
	if(active&&!streaming&&!connectionDead)
		{
		/* Install the new callback functions: */
		packetNotificationCallback=newPacketNotificationCallback;
		errorCallback=newErrorCallback;
		
		if(batteryStateUpdatedCallback!=0)
			{
			/* Send initial battery states for all devices: */
			Threads::Mutex::Lock batteryStatesLock(batteryStatesMutex);
			for(unsigned int i=0;i<virtualDevices.size();++i)
				(*batteryStateUpdatedCallback)(i);
			}
		
		/* Start the packet receiving thread: */
		streamReceiveThread.start(this,&VRDeviceClient::streamReceiveThreadMethod);
		
		/* Send start streaming message and wait for first state packet to arrive: */
		{
		Threads::MutexCond::Lock packetSignalLock(packetSignalCond);
		pipe.writeMessage(VRDevicePipe::STARTSTREAM_REQUEST);
		pipe.flush();
		packetSignalCond.wait(packetSignalLock);
		streaming=true;
		}
		}
	else
		{
		/* Just delete the new callback functions: */
		delete newPacketNotificationCallback;
		delete newErrorCallback;
		}
	}

void VRDeviceClient::stopStream(void)
	{
	if(streaming)
		{
		streaming=false;
		if(!connectionDead)
			{
			/* Send stop streaming message: */
			pipe.writeMessage(VRDevicePipe::STOPSTREAM_REQUEST);
			pipe.flush();
			
			/* Wait for packet receiving thread to die: */
			streamReceiveThread.join();
			}
		
		/* Delete the callback functions: */
		delete packetNotificationCallback;
		packetNotificationCallback=0;
		delete errorCallback;
		errorCallback=0;
		}
	}

}
