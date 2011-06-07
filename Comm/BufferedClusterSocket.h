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

#ifndef COMM_BUFFEREDCLUSTERSOCKET_INCLUDED
#define COMM_BUFFEREDCLUSTERSOCKET_INCLUDED

#include <IO/File.h>
#include <Comm/ClusterPipe.h>

namespace Comm {

class BufferedClusterSocket:public IO::File,private ClusterPipe
	{
	/* Embedded classes: */
	public:
	enum Endianness
		{
		DontCare=ClusterPipe::DontCare,
		LittleEndian=ClusterPipe::LittleEndian,
		BigEndian=ClusterPipe::BigEndian,
		Automatic=ClusterPipe::Automatic
		};
	
	/* Protected methods from IO::File: */
	protected:
	virtual size_t readData(Byte* buffer,size_t bufferSize);
	virtual void writeData(const Byte* buffer,size_t bufferSize);
	
	/* Constructors and destructors: */
	public:
	BufferedClusterSocket(const TCPSocket* sSocket,MulticastPipe* sPipe,Endianness sEndianness =Automatic); // Creates a cluster socket over an existing TCP socket; assumes ownership of pipe
	BufferedClusterSocket(std::string hostname,int portId,MulticastPipe* sPipe,Endianness sEndianness =Automatic); // Creates a cluster socket connected to a remote host; assumes ownership of pipe
	
	/* Methods: */
	void shutdown(bool shutdownRead,bool shutdownWrite) // Shuts down the read or write part of the socket; further reads or writes are not allowed
		{
		ClusterPipe::shutdown(shutdownRead,shutdownWrite);
		}
	};

}

#endif
