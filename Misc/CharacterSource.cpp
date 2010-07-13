/***********************************************************************
CharacterSource - Base class to implement high-performance ASCII file
readers.
Copyright (c) 2009-2010 Oliver Kreylos

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

#include <Misc/CharacterSource.h>

namespace Misc {

/********************************
Methods of class CharacterSource:
********************************/

CharacterSource::CharacterSource(size_t sBufferSize)
	:bufferSize(sBufferSize),
	 allocBuffer(new unsigned char[bufferSize+10]), // 10 characters maximum guaranteed ungetc()
	 buffer(allocBuffer+10),bufferEnd(buffer+bufferSize),
	 eofPtr(0),rPtr(bufferEnd)
	{
	}

CharacterSource::~CharacterSource(void)
	{
	/* Delete the allocated buffer: */
	delete[] allocBuffer;
	}

void CharacterSource::ungetChar(int character)
	{
	/* Check if the end of the unget buffer has been reached: */
	if(rPtr==allocBuffer)
		throw UngetError();
	
	/* Return the character to the source: */
	--rPtr;
	*rPtr=(unsigned char)character;
	}

}
