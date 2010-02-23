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

#ifndef COMM_TCPSOCKETCHARACTERSOURCE_INCLUDED
#define COMM_TCPSOCKETCHARACTERSOURCE_INCLUDED

#include <Misc/CharacterSource.h>
#include <Comm/TCPSocket.h>

namespace Comm {

class TCPSocketCharacterSource:public Misc::CharacterSource
	{
	/* Elements: */
	private:
	TCPSocket socket; // Object representing the TCP socket
	
	/* Protected methods from CharacterSource: */
	protected:
	virtual void fillBuffer(void);
	
	/* Constructors and destructors: */
	public:
	TCPSocketCharacterSource(const TCPSocket& sSocket,size_t sBufferSize =16384); // Starts reading characters from the given connected TCP socket
	virtual ~TCPSocketCharacterSource(void); // Destroys the character source
	};

}

#endif
