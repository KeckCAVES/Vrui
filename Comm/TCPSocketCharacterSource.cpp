/***********************************************************************
TCPSocketCharacterSource - High-performance character-based reader for
TCP sockets.
Copyright (c) 2009 Oliver Kreylos

This file is part of the Miscellaneous Support Library (Misc).

The Miscellaneous Support Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Miscellaneous Support Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Miscellaneous Support Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Comm/TCPSocketCharacterSource.h>

#include <Misc/ThrowStdErr.h>

namespace Comm {

/*****************************************
Methods of class TCPSocketCharacterSource:
*****************************************/

void TCPSocketCharacterSource::fillBuffer(void)
	{
	/* Read at most one buffer's worth of data from the input file: */
	ssize_t readSize=socket.read(buffer,bufferSize);
	if(readSize<0) // That's an error condition
		throw ReadError();
	
	/* Set the buffer end pointer: */
	bufferEnd=buffer+readSize;
	
	/* Reset the read pointer: */
	rPtr=buffer;
	}

TCPSocketCharacterSource::TCPSocketCharacterSource(const TCPSocket& sSocket,size_t sBufferSize)
	:CharacterSource(sBufferSize),
	 socket(sSocket)
	{
	}

TCPSocketCharacterSource::~TCPSocketCharacterSource(void)
	{
	}

}
