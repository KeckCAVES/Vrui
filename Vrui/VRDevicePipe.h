/***********************************************************************
VRDevicePipe - Class defining the client-server protocol for remote VR
devices and VR applications.
Copyright (c) 2002-2005 Oliver Kreylos

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

#ifndef VRUI_VRDEVICEPIPE_INCLUDED
#define VRUI_VRDEVICEPIPE_INCLUDED

#include <Comm/TCPSocket.h>
#include <Vrui/VRDeviceState.h>

namespace Vrui {

class VRDevicePipe
	{
	/* Embedded classes: */
	public:
	typedef unsigned short int MessageIdType; // Network type for protocol messages
	
	enum MessageId // Enumerated type for protocol messages
		{
		CONNECT_REQUEST, // Request to connect to server
		CONNECT_REPLY, // Positive connect reply with server layout
		DISCONNECT_REQUEST, // Polite request to disconnect from server
		ACTIVATE_REQUEST, // Request to activate server (prepare for sending packets)
		DEACTIVATE_REQUEST, // Request to deactivate server (no more packet requests)
		PACKET_REQUEST, // Requests a single packet with current device state
		PACKET_REPLY, // Sends a device state packet
		STARTSTREAM_REQUEST, // Requests entering stream mode (server sends packets automatically)
		STOPSTREAM_REQUEST, // Requests leaving stream mode
		STOPSTREAM_REPLY // Server's reply after last stream packet has been sent
		};
	
	/* Elements: */
	private:
	Comm::TCPSocket socket; // Socket representing the other end of a connection
	
	/* Constructors and destructors: */
	public:
	VRDevicePipe(const Comm::TCPSocket& sSocket) // Creates pipe wrapper for existing socket
		:socket(sSocket)
		{
		}
	
	/* Methods: */
	Comm::TCPSocket& getSocket(void) // Returns TCP socket object
		{
		return socket;
		}
	void writeMessage(MessageId messageId) // Writes a protocol message to the pipe
		{
		MessageIdType message=messageId;
		/* Need to do endianness conversion here... */
		socket.blockingWrite(&message,sizeof(MessageIdType));
		}
	MessageIdType readMessage(void) // Reads a protocol message from the pipe
		{
		MessageIdType message;
		socket.blockingRead(&message,sizeof(MessageIdType));
		/* Need to do endianness conversion here... */
		return message;
		}
	template <class DataParam>
	void write(const DataParam& data) // Writes an element of the given data type to the pipe
		{
		/* Need to do endianness conversion here... */
		socket.blockingWrite(&data,sizeof(DataParam));
		}
	template <class DataParam>
	DataParam read(void) // Reads an element of the given data type from the pipe
		{
		DataParam result;
		socket.blockingRead(&result,sizeof(DataParam));
		/* Need to do endianness conversion here... */
		return result;
		}
	template <class DataParam>
	void write(size_t numElements,const DataParam* elements) // Writes an array of elements to the pipe
		{
		/* Need to do endianness conversion here... */
		socket.blockingWrite(elements,numElements*sizeof(DataParam));
		}
	template <class DataParam>
	void read(int numElements,DataParam* elements) // Reads an array of elements from the pipe
		{
		socket.blockingRead(elements,numElements*sizeof(DataParam));
		/* Need to do endianness conversion here... */
		}
	void flush(void) // Finishes writing data of a single message to the pipe
		{
		}
	
	/* Higher-level IO methods: */
	void writeLayout(const VRDeviceState& state) // Writes server layout
		{
		write(state.getNumTrackers());
		write(state.getNumButtons());
		write(state.getNumValuators());
		}
	void readLayout(VRDeviceState& state) // Reads server layout
		{
		int numTrackers=read<int>();
		int numButtons=read<int>();
		int numValuators=read<int>();
		state.setLayout(numTrackers,numButtons,numValuators);
		}
	void writeState(const VRDeviceState& state) // Writes current state
		{
		write(state.getNumTrackers(),state.getTrackerStates());
		write(state.getNumButtons(),state.getButtonStates());
		write(state.getNumValuators(),state.getValuatorStates());
		}
	void readState(VRDeviceState& state) // Reads current state
		{
		read(state.getNumTrackers(),state.getTrackerStates());
		read(state.getNumButtons(),state.getButtonStates());
		read(state.getNumValuators(),state.getValuatorStates());
		}
	};

}

#endif
