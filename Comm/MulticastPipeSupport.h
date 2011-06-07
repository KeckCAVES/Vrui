/***********************************************************************
MulticastPipeSupport - Class to provide a multicast pipe to derived
classes implementing cluster-transparent file abstractions.
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

#ifndef COMM_MULTICASTPIPESUPPORT_INCLUDED
#define COMM_MULTICASTPIPESUPPORT_INCLUDED

#include <Comm/GatherOperation.h>

/* Forward declarations: */
namespace Comm {
class MulticastPipeMultiplexer;
}

namespace Comm {

class MulticastPipeSupport
	{
	/* Elements: */
	protected:
	MulticastPipeMultiplexer* multiplexer; // Pointer to the multiplexer used to coordinate between multiple pipes
	unsigned int pipeId; // Unique identifier of this pipe for the same multiplexer
	
	/* Constructors and destructors: */
	protected:
	MulticastPipeSupport(MulticastPipeMultiplexer* sMultiplexer); // Creates a new pipe on the given multiplexer
	~MulticastPipeSupport(void); // Closes the pipe
	
	/* New methods: */
	public:
	void barrier(void); // Blocks the calling thread until all nodes in a multicast pipe have reached the same point in the program
	unsigned int gather(unsigned int value,GatherOperation::OpCode op); // Blocks the calling thread until all nodes in a multicast pipe have exchanged a value; returns final accumulated value
	};

}

#endif
