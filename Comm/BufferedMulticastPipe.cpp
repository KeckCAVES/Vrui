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

#include <Comm/BufferedMulticastPipe.h>

#include <Comm/MulticastPacket.h>
#include <Comm/MulticastPipeMultiplexer.h>

namespace Comm {

/**************************************
Methods of class BufferedMulticastPipe:
**************************************/

size_t BufferedMulticastPipe::readData(IO::File::Byte* buffer,size_t bufferSize)
	{
	/* Delete the current (completely read) packet: */
	if(packet!=0)
		multiplexer->deletePacket(packet);
	
	/* Get the next packet from the multicast pipe multiplexer: */
	packet=multiplexer->receivePacket(pipeId);
	
	/* Install the new packet as the buffered file's read buffer: */
	setReadBuffer(packet->packetSize,reinterpret_cast<Byte*>(packet->packet),false);
	
	return packet->packetSize;
	}

void BufferedMulticastPipe::writeData(const IO::File::Byte* buffer,size_t bufferSize)
	{
	/* Do nothing; slaves are not supposed to write */
	}

BufferedMulticastPipe::BufferedMulticastPipe(MulticastPipeMultiplexer* sMultiplexer)
	:IO::File(),
	 MulticastPipeSupport(sMultiplexer),
	 packet(0)
	{
	/* Disable read-through: */
	canReadThrough=false;
	}

BufferedMulticastPipe::~BufferedMulticastPipe(void)
	{
	/* Uninstall the buffered file's read buffer: */
	setReadBuffer(0,0,false);
	
	/* Delete the current packet: */
	if(packet!=0)
		multiplexer->deletePacket(packet);
	}

size_t BufferedMulticastPipe::resizeReadBuffer(size_t newReadBufferSize)
	{
	/* Can't do anything because multicast packets have fixed size: */
	return MulticastPacket::maxPacketSize;
	}

}
