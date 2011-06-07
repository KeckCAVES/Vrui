/***********************************************************************
BufferedTCPSocket - Class for high-performance reading/writing from/to
TCP sockets.
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

#ifndef COMM_BUFFEREDTCPSOCKET_INCLUDED
#define COMM_BUFFEREDTCPSOCKET_INCLUDED

#include <string>
#include <Comm/Pipe.h>

/* Forward declarations: */
namespace Comm {
class TCPSocket;
}

namespace Comm {

class BufferedTCPSocket:public Comm::Pipe
	{
	/* Elements: */
	private:
	int fd; // File descriptor of the underlying TCP socket
	
	/* Protected methods from IO::File: */
	protected:
	virtual size_t readData(Byte* buffer,size_t bufferSize);
	virtual void writeData(const Byte* buffer,size_t bufferSize);
	
	/* Constructors and destructors: */
	public:
	BufferedTCPSocket(const char* hostName,int portId); // Opens a TCP socket connected to the given port on the given host with "DontCare" endianness setting
	BufferedTCPSocket(TCPSocket& listenSocket); // Opens a TCP socket connected to a waiting incoming socket on the given listening socket with "DontCare" endianness setting
	virtual ~BufferedTCPSocket(void);
	
	/* Methods from IO::File: */
	virtual int getFd(void) const;
	
	/* Methods from Pipe: */
	virtual bool waitForData(void) const;
	virtual bool waitForData(const Misc::Time& timeout) const;
	virtual void shutdown(bool read,bool write);
	
	/* New methods: */
	int getPortId(void) const; // Returns port ID assigned to the socket
	std::string getAddress(void) const; // Returns internet address assigned to the socket in dotted notation
	std::string getHostName(void) const; // Returns host name of socket, or internet address in dotted notation if host name cannot be resolved
	int getPeerPortId(void) const; // Returns port ID of remote socket
	std::string getPeerAddress(void) const; // Returns internet address of remote socket in dotted notation
	std::string getPeerHostName(void) const; // Returns host name of remote socket, or internet address in dotted notation if host name cannot be resolved
	};

}

#endif
