/***********************************************************************
VRDeviceClient - Class encapsulating the VR device protocol's client
side.
Copyright (c) 2002-2010 Oliver Kreylos

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

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Comm/TCPSocket.h>

namespace Vrui {

/*******************************
Methods of class VRDeviceClient:
*******************************/

void* VRDeviceClient::streamReceiveThreadMethod(void)
	{
	while(true)
		{
		/* Wait for next packet reply message: */
		VRDevicePipe::MessageIdType message=pipe.readMessage();
		if(message==VRDevicePipe::PACKET_REPLY)
			{
			/* Read server's state: */
			{
			Threads::Mutex::Lock stateLock(stateMutex);
			state.read(pipe);
			}
			
			/* Signal packet reception: */
			packetSignalCond.broadcast();
			
			/* Invoke packet notification callback: */
			{
			Threads::Mutex::Lock packetNotificationLock(packetNotificationMutex);
			if(packetNotificationCB!=0)
				packetNotificationCB(this,packetNotificationCBData);
			}
			}
		else if(message==VRDevicePipe::STOPSTREAM_REPLY)
			break;
		else
			throw ProtocolError("VRDeviceClient: Mismatching message while waiting for PACKET_REPLY");
		}
	
	return 0;
	}

void VRDeviceClient::initClient(void)
	{
	/* Initiate connection: */
	pipe.writeMessage(VRDevicePipe::CONNECT_REQUEST);
	pipe.write<unsigned int>(VRDevicePipe::protocolVersionNumber);
	pipe.flush();
	
	/* Wait for server's reply: */
	if(!pipe.waitForData(30,0,false))
		throw ProtocolError("VRDeviceClient: Timeout while waiting for CONNECT_REPLY");
	if(pipe.readMessage()!=VRDevicePipe::CONNECT_REPLY)
		throw ProtocolError("VRDeviceClient: Mismatching message while waiting for CONNECT_REPLY");
	unsigned int serverProtocolVersionNumber=pipe.read<unsigned int>();
	
	/* Check server version number or something... */
	
	/* Read server's layout and initialize current state: */
	state.readLayout(pipe);
	}

VRDeviceClient::VRDeviceClient(const char* deviceServerName,int deviceServerPort)
	:pipe(Comm::TCPSocket(deviceServerName,deviceServerPort)),
	 active(false),streaming(false),
	 packetNotificationCB(0),packetNotificationCBData(0)
	{
	initClient();
	}

VRDeviceClient::VRDeviceClient(const Misc::ConfigurationFileSection& configFileSection)
	:pipe(Comm::TCPSocket(configFileSection.retrieveString("./serverName"),configFileSection.retrieveValue<int>("./serverPort"))),
	 active(false),streaming(false),
	 packetNotificationCB(0),packetNotificationCBData(0)
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
	}

void VRDeviceClient::activate(void)
	{
	if(!active)
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
		pipe.writeMessage(VRDevicePipe::DEACTIVATE_REQUEST);
		pipe.flush();
		}
	}

void VRDeviceClient::getPacket(void)
	{
	if(active)
		{
		if(streaming)
			{
			/* Wait for arrival of next packet: */
			packetSignalCond.wait();
			}
		else
			{
			/* Send packet request message: */
			pipe.writeMessage(VRDevicePipe::PACKET_REQUEST);
			pipe.flush();
			
			/* Wait for packet reply message: */
			pipe.waitForData(10,0); // Throw exception if reply does not arrive in time
			if(pipe.readMessage()!=VRDevicePipe::PACKET_REPLY)
				throw ProtocolError("VRDeviceClient: Mismatching message while waiting for PACKET_REPLY");
			
			/* Read server's state: */
			{
			Threads::Mutex::Lock stateLock(stateMutex);
			state.read(pipe);
			}
			
			/* Invoke packet notification callback: */
			{
			Threads::Mutex::Lock packetNotificationLock(packetNotificationMutex);
			if(packetNotificationCB!=0)
				packetNotificationCB(this,packetNotificationCBData);
			}
			}
		}
	}

void VRDeviceClient::startStream(void)
	{
	if(active)
		{
		/* Start packet receiving thread: */
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
	}

void VRDeviceClient::stopStream(void)
	{
	if(streaming)
		{
		/* Send stop streaming message: */
		streaming=false;
		pipe.writeMessage(VRDevicePipe::STOPSTREAM_REQUEST);
		pipe.flush();
		
		/* Wait for packet receiving thread to die: */
		streamReceiveThread.join();
		}
	}

void VRDeviceClient::enablePacketNotificationCB(VRDeviceClient::PacketNotificationCBType newPacketNotificationCB,void* newPacketNotificationCBData)
	{
	Threads::Mutex::Lock packetNotificationLock(packetNotificationMutex);
	packetNotificationCB=newPacketNotificationCB;
	packetNotificationCBData=newPacketNotificationCBData;
	}

void VRDeviceClient::disablePacketNotificationCB(void)
	{
	Threads::Mutex::Lock packetNotificationLock(packetNotificationMutex);
	packetNotificationCB=0;
	packetNotificationCBData=0;
	}

}
