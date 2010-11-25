/***********************************************************************
MulticastPipe - Class to represent data streams between a single master
and several slaves, with the bulk of communication from the master to
all the slaves in parallel.
Copyright (c) 2005-2010 Oliver Kreylos

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

#include <string.h>
#include <Misc/ThrowStdErr.h>
#include <Comm/MulticastPacket.h>
#include <Comm/MulticastPipeMultiplexer.h>

#include <Comm/MulticastPipe.h>

namespace Comm {

/******************************
Methods of class MulticastPipe:
******************************/

MulticastPipe::MulticastPipe(MulticastPipeMultiplexer* sMultiplexer,unsigned int sPipeId)
	:multiplexer(sMultiplexer),master(multiplexer->isMaster()),
	 pipeId(sPipeId),
	 packet(0),packetPos(0)
	{
	/* Create a a new packet if on the master: */
	if(master)
		{
		/* Create empty packet to be filled by pipe: */
		packet=multiplexer->newPacket();
		packet->packetSize=MulticastPacket::maxPacketSize;
		}
	}

MulticastPipe::~MulticastPipe(void)
	{
	/* Check if there is an unsent message fragment: */
	if(master&&packetPos>0)
		{
		/* Send the current packet as-is: */
		packet->packetSize=packetPos;
		multiplexer->sendPacket(this,packet);
		packet=0;
		}
	
	/* Delete the current packet: */
	if(packet!=0)
		multiplexer->deletePacket(packet);
	
	/* Close the pipe: */
	multiplexer->closePipe(this);
	}

void MulticastPipe::barrier(void)
	{
	/* Check if there is an unsent message fragment: */
	if(master&&packetPos>0)
		{
		/* Send the current packet as-is: */
		packet->packetSize=packetPos;
		multiplexer->sendPacket(this,packet);
		packet=multiplexer->newPacket();
		packet->packetSize=MulticastPacket::maxPacketSize;
		packetPos=0;
		}
	
	/* Pass call through to multicast pipe multiplexer: */
	multiplexer->barrier(this);
	}

unsigned int MulticastPipe::gather(unsigned int value,GatherOperation::OpCode op)
	{
	/* Check if there is an unsent message fragment: */
	if(master&&packetPos>0)
		{
		/* Send the current packet as-is: */
		packet->packetSize=packetPos;
		multiplexer->sendPacket(this,packet);
		packet=multiplexer->newPacket();
		packet->packetSize=MulticastPacket::maxPacketSize;
		packetPos=0;
		}
	
	/* Pass call through to multicast pipe multiplexer: */
	return multiplexer->gather(this,value,op);
	}

void MulticastPipe::broadcastRaw(void* data,size_t size)
	{
	/* Write or read data depending on whether this is the master node or a slave node: */
	if(master)
		{
		/* Write the data: */
		writeRaw(data,size);
		}
	else
		{
		/* Read the data: */
		readRaw(data,size);
		}
	}

void MulticastPipe::finishMessage(void)
	{
	/* Check if there is an unsent message fragment: */
	if(master&&packetPos>0)
		{
		/* Send the current packet as-is and create a new one: */
		packet->packetSize=packetPos;
		multiplexer->sendPacket(this,packet);
		packet=multiplexer->newPacket();
		packet->packetSize=MulticastPacket::maxPacketSize;
		packetPos=0;
		}
	}

void MulticastPipe::writeRaw(const void* data,size_t size)
	{
	/* Check if this is the master node: */
	if(!master)
		Misc::throwStdErr("MulticastPipe::write: Can only be called from the master node");
	
	/* Write data without overflowing packets: */
	const char* dataPtr=static_cast<const char*>(data);
	while(size>0)
		{
		/* Calculate amount of data that can be written into current packet: */
		size_t writeSize=packet->packetSize-packetPos;
		if(writeSize>size)
			writeSize=size;
		
		/* Write data into packet: */
		memcpy(packet->packet+packetPos,dataPtr,writeSize);
		dataPtr+=writeSize;
		size-=writeSize;
		
		/* Adjust packet state: */
		packetPos+=writeSize;
		if(packetPos==packet->packetSize)
			{
			/* Send the current packet and create a new one: */
			multiplexer->sendPacket(this,packet);
			packet=multiplexer->newPacket();
			packet->packetSize=MulticastPacket::maxPacketSize;
			packetPos=0;
			}
		}
	}

void MulticastPipe::readRaw(void* data,size_t size)
	{
	/* Check if this is a slave node: */
	if(master)
		Misc::throwStdErr("MulticastPipe::read: Can only be called from a slave node");
	
	/* Read data without underflowing packets: */
	char* dataPtr=static_cast<char*>(data);
	while(size>0)
		{
		/* Request a new packet if there is no current packet: */
		if(packet==0)
			{
			/* Read the next packet: */
			packet=multiplexer->receivePacket(this);
			packetPos=0;
			}
		
		/* Calculate amount of data that can be read from current packet: */
		size_t readSize=packet->packetSize-packetPos;
		if(readSize>size)
			readSize=size;
		
		/* Read data from packet: */
		memcpy(dataPtr,packet->packet+packetPos,readSize);
		dataPtr+=readSize;
		size-=readSize;
		
		/* Adjust packet state: */
		packetPos+=readSize;
		if(packetPos==packet->packetSize)
			{
			/* Discard the current packet: */
			multiplexer->deletePacket(packet);
			packet=0;
			}
		}
	}

}
