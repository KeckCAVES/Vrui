/***********************************************************************
ClusterStandardFile - Class for high-performance reading/writing from/to
standard operating system files distributed among a cluster over a
multicast pipe.
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

#ifndef COMM_CLUSTERSTANDARDFILE_INCLUDED
#define COMM_CLUSTERSTANDARDFILE_INCLUDED

#include <sys/types.h>
#include <IO/File.h>

/* Forward declarations: */
namespace Comm {
class MulticastPipe;
}

namespace Comm {

class ClusterStandardFile:public IO::File
	{
	/* Embedded classes: */
	public:
	typedef off_t Offset; // Type for file offsets
	
	/* Elements: */
	private:
	int fd; // File descriptor of the underlying file
	int lastOp; // Flag whether last file access was a read or a write
	Offset readPos; // Current position of read pointer
	Offset writePos; // Current position of write pointer
	MulticastPipe* pipe; // Pipe connecting the cluster's master node and slave nodes
	
	/* Protected methods from IO::File: */
	protected:
	virtual size_t readData(Byte* buffer,size_t bufferSize);
	virtual void writeData(const Byte* buffer,size_t bufferSize);
	
	/* Constructors and destructors: */
	public:
	ClusterStandardFile(const char* fileName,MulticastPipe* sPipe,AccessMode sAccessMode = ReadOnly); // Opens a standard file with "DontCare" endianness setting and default flags and permissions; file inherits multicast pipe
	ClusterStandardFile(const char* fileName,MulticastPipe* sPipe,AccessMode sAccessMode,int flags,int mode =0); // Opens a standard file with "DontCare" endianness setting; file inherits multicast pipe
	virtual ~ClusterStandardFile(void);
	
	/* Methods: */
	void seekSet(Offset newOffset); // Sets the position of the file's read and write pointers to a position relative to the beginning of the file
	};

}

#endif
