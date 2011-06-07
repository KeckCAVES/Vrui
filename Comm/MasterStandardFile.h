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

#ifndef COMM_MASTERSTANDARDFILE_INCLUDED
#define COMM_MASTERSTANDARDFILE_INCLUDED

#include <IO/StandardFile.h>

/* Forward declarations: */
namespace Comm {
class MulticastPipeMultiplexer;
}

namespace Comm {

class MasterStandardFile:public IO::StandardFile
	{
	/* Elements: */
	private:
	MulticastPipeMultiplexer* multiplexer; // Pointer to the multiplexer connecting the cluster
	unsigned int pipeId; // Unique identifier of this pipe for the same multiplexer
	
	/* Protected methods from IO::File: */
	protected:
	virtual size_t readData(Byte* buffer,size_t bufferSize);
	
	/* Constructors and destructors: */
	public:
	MasterStandardFile(MulticastPipeMultiplexer* sMultiplexer,const char* fileName,AccessMode accessMode =ReadOnly); // Opens a standard file with "DontCare" endianness setting and default flags and permissions
	MasterStandardFile(MulticastPipeMultiplexer* sMultiplexer,const char* fileName,AccessMode accessMode,int flags,int mode =0); // Opens a standard file with "DontCare" endianness setting
	virtual ~MasterStandardFile(void);
	
	/* Methods from IO::File: */
	virtual int getFd(void) const;
	virtual size_t resizeReadBuffer(size_t newReadBufferSize);
	virtual void resizeWriteBuffer(size_t newWriteBufferSize);
	
	/* Methods from IO::SeekableFile: */
	virtual Offset getSize(void) const;
	};

}

#endif
