/***********************************************************************
ClusterBufferedTCPSocket - Class to forward data received from a TCP
socket to slave nodes in a cluster environment via a multicast pipe.
Copyright (c) 2010-2011 Oliver Kreylos

This file is part of the Portable Communications Library (Comm).

The Portable Communications Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Portable Communications Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Portable Communications Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Comm/ClusterBufferedTCPSocket.h>

#include <string.h>
#include <Comm/MulticastPacket.h>
#include <Comm/MulticastPipeMultiplexer.h>

namespace Comm {

/*****************************************
Methods of class ClusterBufferedTCPSocket:
*****************************************/

size_t ClusterBufferedTCPSocket::readData(IO::File::Byte* buffer,size_t bufferSize)
	{
	/* Call the base class method to receive data from the socket: */
	size_t result=BufferedTCPSocket::readData(buffer,bufferSize);
	
	/* Forward all read data to the slave nodes: */
	size_t readSize=result;
	Byte* bufPtr=buffer;
	while(readSize>0)
		{
		/* Send a multicast packet's worth of data: */
		MulticastPacket* packet=multiplexer->newPacket();
		packet->packetSize=MulticastPacket::maxPacketSize;
		if(packet->packetSize>readSize)
			packet->packetSize=readSize;
		memcpy(packet->packet,bufPtr,packet->packetSize);
		multiplexer->sendPacket(pipeId,packet); // Packet is now held by multiplexer
		
		/* Advance the buffer pointer: */
		bufPtr+=packet->packetSize;
		readSize-=packet->packetSize;
		}
	
	/* Return the original amount of read data: */
	return result;
	}

ClusterBufferedTCPSocket::ClusterBufferedTCPSocket(const char* hostName,int portId,MulticastPipeMultiplexer* sMultiplexer)
	:BufferedTCPSocket(hostName,portId),
	 MulticastPipeSupport(sMultiplexer)
	{
	}

ClusterBufferedTCPSocket::ClusterBufferedTCPSocket(TCPSocket& listenSocket,MulticastPipeMultiplexer* sMultiplexer)
	:BufferedTCPSocket(listenSocket),
	 MulticastPipeSupport(sMultiplexer)
	{
	}

}
