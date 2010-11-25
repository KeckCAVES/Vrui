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

#ifndef MISC_READBUFFER_INCLUDED
#define MISC_READBUFFER_INCLUDED

#include <string.h>
#include <stdexcept>
#include <Misc/Endianness.h>

namespace Misc {

class ReadBuffer
	{
	/* Embedded classes: */
	public:
	enum Endianness // Enumerated type to enforce buffer endianness
		{
		DontCare,LittleEndian,BigEndian
		};
	
	class ReadError:public std::runtime_error // Exception class to report buffer reading errors
		{
		/* Constructors and destructors: */
		public:
		ReadError(size_t numBytes,size_t numBytesRead); // Error where numBytesRead were read instead of numBytes
		};
	
	/* Elements: */
	private:
	size_t bufferSize; // Size of the buffer
	unsigned char* buffer; // Pointer to the buffer
	Endianness endianness; // Endianness of data in the buffer
	bool mustSwapEndianness; // Flag if current buffer endianness is different from machine endianness
	unsigned char* readPtr; // Pointer to the current read position
	size_t unread; // Amount of unread data in the buffer
	
	/* Constructors and destructors: */
	public:
	ReadBuffer(size_t sBufferSize,Endianness sEndianness =DontCare); // Creates a read buffer of the given size with the given endianness
	ReadBuffer(size_t sBufferSize,bool sMustSwapEndianness); // Creates a read buffer of the given size with the given endianness swapping behavior
	private:
	ReadBuffer(const ReadBuffer& source); // Prohibit copy constructor
	ReadBuffer& operator=(const ReadBuffer& source); // Prohibit assignment operator
	public:
	~ReadBuffer(void); // Destroys the read buffer
	
	/* Methods: */
	Endianness getEndianness(void) // Returns current endianness setting of buffer
		{
		return endianness;
		}
	void setEndianness(Endianness newEndianness); // Sets current endianness setting of buffer
	void setEndianness(bool newMustSwapEndianness); // Sets current endianness swapping behavior of buffer
	size_t getBufferSize(void) const // Returns the size of the buffer
		{
		return bufferSize;
		}
	template <class SourceParam>
	void readFromSource(SourceParam& source) // Fills the entire buffer by reading from a binary data source
		{
		/* Read the entire buffer: */
		source.readRaw(buffer,bufferSize);
		
		/* Rewind the buffer: */
		rewind();
		}
	void rewind(void) // Resets the buffer to commence reading from the beginning
		{
		readPtr=buffer;
		unread=bufferSize;
		}
	size_t getUnread(void) const // Returns the amount of unread data left in the buffer
		{
		return unread;
		}
	bool eof(void) const // Returns true if the entire buffer has been read
		{
		return unread==0;
		}
	
	/* Endianness-safe binary I/O interface: */
	bool mustSwapOnRead(void) // Returns true if the buffer must endianness-swap data on read
		{
		return mustSwapEndianness;
		}
	void readRaw(void* data,size_t dataSize) // Reads a chunk of data from the buffer
		{
		/* Check if there is enough unread data in the buffer: */
		if(dataSize<=unread)
			{
			/* Read data from the buffer: */
			memcpy(data,readPtr,dataSize);
			readPtr+=dataSize;
			unread-=dataSize;
			}
		else
			throw ReadError(dataSize,unread);
		}
	template <class DataParam>
	DataParam read(void) // Reads single value
		{
		DataParam result;
		readRaw(&result,sizeof(DataParam));
		if(mustSwapEndianness)
			swapEndianness(result);
		return result;
		}
	template <class DataParam>
	DataParam& read(DataParam& data) // Reads single value through reference
		{
		readRaw(&data,sizeof(DataParam));
		if(mustSwapEndianness)
			swapEndianness(data);
		return data;
		}
	template <class DataParam>
	void read(DataParam* data,size_t numItems) // Reads array of values
		{
		readRaw(data,numItems*sizeof(DataParam));
		if(mustSwapEndianness)
			swapEndianness(data,numItems);
		}
	};

}

#endif
