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

#ifndef COMM_CLUSTERBUFFEREDTCPSOCKET_INCLUDED
#define COMM_CLUSTERBUFFEREDTCPSOCKET_INCLUDED

#include <Comm/BufferedTCPSocket.h>
#include <Comm/MulticastPipeSupport.h>

namespace Comm {

class ClusterBufferedTCPSocket:public BufferedTCPSocket,public MulticastPipeSupport
	{
	/* Protected methods from IO::File: */
	protected:
	virtual size_t readData(Byte* buffer,size_t bufferSize);
	
	/* Constructors and destructors: */
	public:
	ClusterBufferedTCPSocket(const char* hostName,int portId,MulticastPipeMultiplexer* sMultiplexer); // Opens a TCP socket connected to the given port on the given host with "DontCare" endianness setting; forwards incoming data over given multicast pipe multiplexer
	ClusterBufferedTCPSocket(TCPSocket& listenSocket,MulticastPipeMultiplexer* sMultiplexer); // Opens a TCP socket connected to a waiting incoming socket on the given listening socket with "DontCare" endianness setting; forwards incoming data over given multicast pipe multiplexer
	private:
	ClusterBufferedTCPSocket(const ClusterBufferedTCPSocket& source); // Prohibit copy constructor
	ClusterBufferedTCPSocket& operator=(const ClusterBufferedTCPSocket& source); // Prohibit assignment operator
	};

}

#endif
