/***********************************************************************
MasterStandardFile - Class to represent a standard file on the master
node of a cluster.
Copyright (c) 2011 Oliver Kreylos

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

#include <Comm/MasterStandardFile.h>

#include <string.h>
#include <Misc/ThrowStdErr.h>
#include <Comm/MulticastPacket.h>
#include <Comm/MulticastPipeMultiplexer.h>

namespace Comm {

/***********************************
Methods of class MasterStandardFile:
***********************************/

size_t MasterStandardFile::readData(IO::File::Byte* buffer,size_t bufferSize)
	{
	/* Call the base class method: */
	size_t dataSize=IO::StandardFile::readData(buffer,bufferSize);
	
	/* Create a new multicast packet and forward the just-read data to the slaves: */
	MulticastPacket* packet=multiplexer->newPacket();
	packet->packetSize=dataSize;
	memcpy(packet->packet,buffer,dataSize);
	multiplexer->sendPacket(pipeId,packet);
	
	return dataSize;
	}

MasterStandardFile::MasterStandardFile(MulticastPipeMultiplexer* sMultiplexer,const char* fileName,IO::File::AccessMode accessMode)
	:IO::StandardFile(fileName,accessMode),
	 multiplexer(sMultiplexer),pipeId(multiplexer->openPipe())
	{
	/* Disable read-through: */
	canReadThrough=false;
	
	/* Resize the buffered file's read and write buffers to hold exactly one multicast packet: */
	if(accessMode!=WriteOnly)
		IO::StandardFile::resizeReadBuffer(MulticastPacket::maxPacketSize);
	if(accessMode!=ReadOnly)
		IO::StandardFile::resizeWriteBuffer(MulticastPacket::maxPacketSize);
	}

MasterStandardFile::MasterStandardFile(MulticastPipeMultiplexer* sMultiplexer,const char* fileName,IO::File::AccessMode accessMode,int flags,int mode)
	:IO::StandardFile(fileName,accessMode,flags,mode),
	 multiplexer(sMultiplexer),pipeId(multiplexer->openPipe())
	{
	/* Disable read-through: */
	canReadThrough=false;
	
	/* Resize the buffered file's read and write buffers to hold exactly one multicast packet: */
	if(accessMode!=WriteOnly)
		IO::StandardFile::resizeReadBuffer(MulticastPacket::maxPacketSize);
	if(accessMode!=ReadOnly)
		IO::StandardFile::resizeWriteBuffer(MulticastPacket::maxPacketSize);
	}

MasterStandardFile::~MasterStandardFile(void)
	{
	/* Close the pipe: */
	multiplexer->closePipe(pipeId);
	}

int MasterStandardFile::getFd(void) const
	{
	/* Default behavior is not supported: */
	Misc::throwStdErr("Comm::MasterStandardFile::getFd: File does not have file descriptor");
	
	/* Just to make compiler happy: */
	return -1;
	}

size_t MasterStandardFile::resizeReadBuffer(size_t newReadBufferSize)
	{
	/* Install a packet-sized read buffer: */
	return IO::StandardFile::resizeReadBuffer(MulticastPacket::maxPacketSize);
	}

void MasterStandardFile::resizeWriteBuffer(size_t newReadBufferSize)
	{
	/* Install a packet-sized write buffer: */
	IO::StandardFile::resizeWriteBuffer(MulticastPacket::maxPacketSize);
	}

IO::SeekableFile::Offset MasterStandardFile::getSize(void) const
	{
	/* Call the base class method: */
	Offset fileSize=IO::StandardFile::getSize();
	
	/* Send a packet with the file size to the slaves: */
	MulticastPacket* oobPacket=multiplexer->newPacket();
	oobPacket->packetSize=sizeof(Offset);
	memcpy(oobPacket->packet,&fileSize,sizeof(Offset));
	multiplexer->sendPacket(pipeId,oobPacket);
	
	return fileSize;
	}

}
