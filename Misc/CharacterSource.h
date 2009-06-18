/***********************************************************************
CharacterSource - Base class to implement high-performance ASCII file
readers.
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

#ifndef MISC_CHARACTERSOURCE_INCLUDED
#define MISC_CHARACTERSOURCE_INCLUDED

#include <stdexcept>

namespace Misc {

class CharacterSource
	{
	/* Embedded classes: */
	public:
	class OpenError:public std::runtime_error // Exception class to report errors while opening a character source
		{
		/* Constructors and destructors: */
		public:
		OpenError(void)
			:std::runtime_error("CharacterSource: Error while opening character source")
			{
			}
		OpenError(const char* error)
			:std::runtime_error(error)
			{
			}
		};
	
	class ReadError:public std::runtime_error // Exception class to report errors while reading from a character source
		{
		/* Constructors and destructors: */
		public:
		ReadError(void)
			:std::runtime_error("CharacterSource: Error while reading from character source")
			{
			}
		};
	
	/* Elements: */
	protected:
	size_t bufferSize; // Size of file access buffer
	unsigned char* buffer; // Buffer for efficient file access
	unsigned char* bufferEnd; // Pointer behind last valid character in buffer
	unsigned char* eofPtr; // Pointer after the last character in the last buffer in the file, or 0 if there are more buffers
	unsigned char* rPtr; // Current reading position in buffer
	
	/* Protected methods: */
	virtual void fillBuffer(void) =0; // Fills the read buffer with more data from the input file
	
	/* Constructors and destructors: */
	public:
	CharacterSource(size_t sBufferSize); // Creates a character source with the given buffer size
	virtual ~CharacterSource(void); // Destroys the character source
	
	/* Methods: */
	inline bool eof(void) const // Returns true if the entire input file has been read
		{
		return rPtr==eofPtr;
		}
	inline int getc(void) // Returns the next character from the input file, or EOF (-1) if the entire file has been read
		{
		/* Check if the current buffer has been read: */
		if(rPtr==bufferEnd)
			{
			/* Check if the last buffer has been read: */
			if(rPtr==eofPtr)
				return -1; // Return EOF
			else
				{
				/* Read the next buffer from the input file: */
				fillBuffer();
				
				/* Check for end of file again: */
				if(rPtr==eofPtr)
					return -1; // Return EOF
				}
			}
		
		/* Return the next character from the current buffer: */
		return *(rPtr++);
		}
	};

}

#endif
