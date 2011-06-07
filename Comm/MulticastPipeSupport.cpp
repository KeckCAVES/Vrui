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

#include <Comm/MulticastPipeSupport.h>

#include <Comm/MulticastPipeMultiplexer.h>

namespace Comm {

/*************************************
Methods of class MulticastPipeSupport:
*************************************/

MulticastPipeSupport::MulticastPipeSupport(MulticastPipeMultiplexer* sMultiplexer)
	:multiplexer(sMultiplexer),
	 pipeId(multiplexer->openPipe())
	{
	}

MulticastPipeSupport::~MulticastPipeSupport(void)
	{
	/* Close the pipe: */
	multiplexer->closePipe(pipeId);
	}

void MulticastPipeSupport::barrier(void)
	{
	/* Pass call through to multicast pipe multiplexer: */
	multiplexer->barrier(pipeId);
	}

unsigned int MulticastPipeSupport::gather(unsigned int value,GatherOperation::OpCode op)
	{
	/* Pass call through to multicast pipe multiplexer: */
	return multiplexer->gather(pipeId,value,op);
	}

}
