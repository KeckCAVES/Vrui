/***********************************************************************
ReadBuffer - Class to read from a memory buffer using a pipe interface.
Copyright (c) 2010 Oliver Kreylos

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

#include <Misc/ReadBuffer.h>

#include <Misc/ThrowStdErr.h>

namespace Misc {

/**************************************
Methods of class ReadBuffer::ReadError:
**************************************/

ReadBuffer::ReadError::ReadError(size_t numBytes,size_t numBytesRead)
	:std::runtime_error(printStdErrMsg("Misc::ReadBuffer: Error reading %u bytes from buffer, read %u bytes instead",numBytes,numBytesRead))
	{
	}

/***************************
Methods of class ReadBuffer:
***************************/

ReadBuffer::ReadBuffer(size_t sBufferSize,Endianness sEndianness)
	:bufferSize(sBufferSize),buffer(new unsigned char[bufferSize]),
	 readPtr(buffer),unread(bufferSize)
	{
	setEndianness(sEndianness);
	}

ReadBuffer::ReadBuffer(size_t sBufferSize,bool sMustSwapEndianness)
	:bufferSize(sBufferSize),buffer(new unsigned char[bufferSize]),
	 readPtr(buffer),unread(bufferSize)
	{
	setEndianness(sMustSwapEndianness);
	}

ReadBuffer::~ReadBuffer(void)
	{
	delete[] buffer;
	}

void ReadBuffer::setEndianness(Endianness newEndianness)
	{
	endianness=newEndianness;
	
	/* Determine the buffer's endianness swapping behavior: */
	#if __BYTE_ORDER==__LITTLE_ENDIAN
	mustSwapEndianness=endianness==BigEndian;
	#endif
	#if __BYTE_ORDER==__BIG_ENDIAN
	mustSwapEndianness=endianness==LittleEndian;
	#endif
	}

void ReadBuffer::setEndianness(bool newMustSwapEndianness)
	{
	mustSwapEndianness=newMustSwapEndianness;
	
	/* Determine the buffer's endianness: */
	if(mustSwapEndianness)
		{
		#if __BYTE_ORDER==__LITTLE_ENDIAN
		endianness=BigEndian;
		#elif __BYTE_ORDER==__BIG_ENDIAN
		endianness=LittleEndian;
		#else
		endianness=DontCare;
		#endif
		}
	else
		{
		#if __BYTE_ORDER==__LITTLE_ENDIAN
		endianness=LittleEndian;
		#elif __BYTE_ORDER==__BIG_ENDIAN
		endianness=BigEndian;
		#else
		endianness=DontCare;
		#endif
		}
	}

}
