/***********************************************************************
BufferedClusterSocket - Class to provide cluster-wide access to a TCP
socket via an automatic data forwarding mechanism.
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

#include <Comm/BufferedClusterSocket.h>

namespace Comm {

/**************************************
Methods of class BufferedClusterSocket:
**************************************/

size_t BufferedClusterSocket::readData(IO::File::Byte* buffer,size_t bufferSize)
	{
	return ClusterPipe::readUpto(buffer,bufferSize);
	}

void BufferedClusterSocket::writeData(const IO::File::Byte* buffer,size_t bufferSize)
	{
	ClusterPipe::writeRaw(buffer,bufferSize);
	}

BufferedClusterSocket::BufferedClusterSocket(const TCPSocket* sSocket,MulticastPipe* sPipe,BufferedClusterSocket::Endianness sEndianness)
	:IO::File(ReadWrite),
	 ClusterPipe(sSocket,sPipe,ClusterPipe::Endianness(sEndianness))
	{
	}

BufferedClusterSocket::BufferedClusterSocket(std::string hostname,int portId,MulticastPipe* sPipe,BufferedClusterSocket::Endianness sEndianness)
	:IO::File(ReadWrite),
	 ClusterPipe(hostname,portId,sPipe,ClusterPipe::Endianness(sEndianness))
	{
	}

}
