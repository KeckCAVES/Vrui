/***********************************************************************
ClusterFileCharacterSource - High-performance ASCII file reader for
files distributed across a cluster using a multicast pipe.
Copyright (c) 2007-2009 Oliver Kreylos

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

#ifndef COMM_CLUSTERFILECHARACTERSOURCE_INCLUDED
#define COMM_CLUSTERFILECHARACTERSOURCE_INCLUDED

#include <Misc/CharacterSource.h>

/* Forward declarations: */
namespace Comm {
class MulticastPipe;
}

namespace Comm {

class ClusterFileCharacterSource:public Misc::CharacterSource
	{
	/* Elements: */
	private:
	int inputFd; // File descriptor of input file on the master node
	MulticastPipe* pipe; // Multicast pipe to distribute input file from master to slaves; owned by character source
	
	/* Protected methods from CharacterSource: */
	protected:
	virtual void fillBuffer(void);
	
	/* Constructors and destructors: */
	public:
	ClusterFileCharacterSource(const char* inputFileName,MulticastPipe* sPipe,size_t sBufferSize =16384); // Opens the given input file over the given pipe; adopts pipe
	virtual ~ClusterFileCharacterSource(void); // Destroys the character source
	};

}

#endif
