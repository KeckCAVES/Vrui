/***********************************************************************
StandardFile - Pair of classes for high-performance cluster-transparent
reading/writing from/to standard operating system files.
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

#ifndef COMM_STANDARDFILE_INCLUDED
#define COMM_STANDARDFILE_INCLUDED

#include <IO/SeekableFile.h>

/* Forward declarations: */
namespace Comm {
class MulticastPacket;
class MulticastPipeMultiplexer;
}

namespace Comm {

class StandardFileMaster:public IO::SeekableFile // Class to represent cluster files on the master node
	{
	/* Elements: */
	private:
	MulticastPipeMultiplexer& multiplexer; // Pointer to the multiplexer connecting the cluster
	unsigned int pipeId; // Unique identifier of this pipe for the same multiplexer
	int fd; // File descriptor of the underlying file
	Offset filePos; // Current position of the underlying file's read/write pointer
	
	/* Protected methods from IO::File: */
	protected:
	virtual size_t readData(Byte* buffer,size_t bufferSize);
	virtual void writeData(const Byte* buffer,size_t bufferSize);
	
	/* Private methods: */
	void openFile(const char* fileName,AccessMode accessMode,int flags,int mode); // Opens a file and handles errors
	
	/* Constructors and destructors: */
	public:
	StandardFileMaster(MulticastPipeMultiplexer& sMultiplexer,const char* fileName,AccessMode accessMode =ReadOnly); // Opens a standard file with "DontCare" endianness setting and default flags and permissions
	StandardFileMaster(MulticastPipeMultiplexer& sMultiplexer,const char* fileName,AccessMode accessMode,int flags,int mode =0); // Opens a standard file with "DontCare" endianness setting
	virtual ~StandardFileMaster(void);
	
	/* Methods from IO::File: */
	virtual int getFd(void) const;
	virtual size_t resizeReadBuffer(size_t newReadBufferSize);
	
	/* Methods from IO::SeekableFile: */
	virtual Offset getSize(void) const;
	};

class StandardFileSlave:public IO::SeekableFile // Class to represent cluster files on the slave nodes
	{
	/* Elements: */
	private:
	MulticastPipeMultiplexer& multiplexer; // Pointer to the multiplexer connecting the cluster
	unsigned int pipeId; // Unique identifier of this pipe for the same multiplexer
	MulticastPacket* packet; // Pointer to most recently received multicast packet; doubles as file's read buffer
	
	/* Protected methods from IO::File: */
	protected:
	virtual size_t readData(Byte* buffer,size_t bufferSize);
	virtual void writeData(const Byte* buffer,size_t bufferSize);
	
	/* Constructors and destructors: */
	public:
	StandardFileSlave(MulticastPipeMultiplexer& sMultiplexer,const char* fileName,AccessMode accessMode =ReadOnly); // Opens a standard file with "DontCare" endianness setting
	virtual ~StandardFileSlave(void);
	
	/* Methods from IO::File: */
	virtual int getFd(void) const;
	virtual size_t resizeReadBuffer(size_t newReadBufferSize);
	
	/* Methods from IO::SeekableFile: */
	virtual Offset getSize(void) const;
	};

}

#endif
