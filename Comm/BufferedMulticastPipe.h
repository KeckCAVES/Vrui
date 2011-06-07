/***********************************************************************
BufferedMulticastPipe - Class for high-performance reading from
multicast pipes on the slave nodes of a cluster.
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

#ifndef COMM_BUFFEREDMULTICASTPIPE_INCLUDED
#define COMM_BUFFEREDMULTICASTPIPE_INCLUDED

#include <IO/File.h>
#include <Comm/MulticastPipeSupport.h>

/* Forward declarations: */
namespace Comm {
struct MulticastPacket;
}

namespace Comm {

class BufferedMulticastPipe:public IO::File,public Comm::MulticastPipeSupport
	{
	/* Elements: */
	private:
	MulticastPacket* packet; // Pointer to current packet
	
	/* Protected methods from IO::File: */
	protected:
	virtual size_t readData(Byte* buffer,size_t bufferSize);
	virtual void writeData(const Byte* buffer,size_t bufferSize);
	
	/* Constructors and destructors: */
	public:
	BufferedMulticastPipe(MulticastPipeMultiplexer* sMultiplexer); // Creates new buffered pipe for the given multiplexer
	private:
	BufferedMulticastPipe(const BufferedMulticastPipe& source); // Prohibit copy constructor
	BufferedMulticastPipe& operator=(const BufferedMulticastPipe& source); // Prohibit assignment operato
	public:
	virtual ~BufferedMulticastPipe(void); // Closes the pipe
	
	/* Methods from IO::File: */
	virtual size_t resizeReadBuffer(size_t newReadBufferSize);
	};

}

#endif
