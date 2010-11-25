/***********************************************************************
WriteBuffer - Class to write into a list of memory buffers using a pipe
interface.
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

#ifndef MISC_WRITEBUFFER_INCLUDED
#define MISC_WRITEBUFFER_INCLUDED

#include <string.h>
#include <Misc/Endianness.h>

namespace Misc {

class WriteBuffer
	{
	/* Embedded classes: */
	public:
	enum Endianness // Enumerated type to enforce buffer endianness
		{
		DontCare,LittleEndian,BigEndian
		};
	
	private:
	struct BufferChunk // Structure for a single buffer chunk
		{
		/* Elements: */
		public:
		BufferChunk* succ; // Pointer to next buffer chunk
		static const size_t bufferSize=8192-sizeof(BufferChunk*);
		unsigned char buffer[bufferSize];
		};
	
	/* Elements: */
	private:
	Endianness endianness; // Endianness of data in the buffer
	bool mustSwapEndianness; // Flag if current buffer endianness is different from machine endianness
	BufferChunk* head; // Pointer to the first buffer chunk
	BufferChunk* tail; // Pointer to the last buffer chunk
	unsigned char* writePtr; // Pointer to the current write position in the last buffer chunk
	size_t tailFree; // Free size in last buffer chunk
	
	/* Private methods: */
	void addTail(void); // Adds another buffer chunk to the end of the list
	
	/* Constructors and destructors: */
	public:
	WriteBuffer(Endianness sEndianness =DontCare); // Creates an empty write buffer with the given endianness
	WriteBuffer(bool sMustSwapEndianness); // Creates an empty write buffer with the given endianness swapping behavior
	private:
	WriteBuffer(const WriteBuffer& source); // Prohibit copy constructor
	WriteBuffer& operator=(const WriteBuffer& source); // Prohibit assignment operator
	public:
	~WriteBuffer(void); // Destroys the write buffer
	
	/* Methods: */
	Endianness getEndianness(void) // Returns current endianness setting of buffer
		{
		return endianness;
		}
	void setEndianness(Endianness newEndianness); // Sets current endianness setting of buffer
	void setEndianness(bool newMustSwapEndianness); // Sets current endianness swapping behavior of buffer
	size_t getDataSize(void) const; // Returns the current size of the buffer
	void clear(void); // Clears the buffer
	template <class SinkParam>
	void writeToSink(SinkParam& sink) const // Writes all data in the buffer to a sink supporting raw binary I/O interface
		{
		/* Write all full buffer chunks: */
		for(const BufferChunk* bPtr=head;bPtr!=tail;bPtr=bPtr->succ)
			sink.writeRaw(bPtr->buffer,BufferChunk::bufferSize);
		
		/* Calculate the amount of data in the last buffer chunk: */
		size_t tailSize=BufferChunk::bufferSize-tailFree;
		if(tailSize>0)
			{
			/* Write the partially-filled tail buffer: */
			sink.writeRaw(tail->buffer,tailSize);
			}
		}
	
	/* Endianness-safe binary I/O interface: */
	bool mustSwapOnWrite(void) // Returns true if the buffer must endianness-swap data on write
		{
		return mustSwapEndianness;
		}
	void writeRaw(const void* data,size_t dataSize) // Writes a chunk of data into the buffer
		{
		/* Check for the common case first: */
		if(dataSize<=tailFree)
			{
			/* Write all data at once: */
			memcpy(writePtr,data,dataSize);
			writePtr+=dataSize;
			tailFree-=dataSize;
			}
		else
			{
			/* Write the data in segments while adding chunks to the buffer: */
			const unsigned char* dPtr=static_cast<const unsigned char*>(data);
			while(dataSize>0)
				{
				/* Ensure that there is room in the last buffer chunk: */
				if(tailFree==0)
					addTail();

				/* Write as much data as fits into the tail buffer: */
				size_t writeSize=dataSize;
				if(writeSize>tailFree)
					writeSize=tailFree;
				memcpy(writePtr,dPtr,writeSize);
				dPtr+=writeSize;
				dataSize-=writeSize;
				writePtr+=writeSize;
				tailFree-=writeSize;
				}
			}
		}
	template <class DataParam>
	void write(const DataParam& data) // Writes single value
		{
		if(mustSwapEndianness)
			{
			DataParam temp=data;
			swapEndianness(temp);
			writeRaw(&temp,sizeof(DataParam));
			}
		else
			writeRaw(&data,sizeof(DataParam));
		}
	template <class DataParam>
	void write(const DataParam* data,size_t numItems) // Writes array of values
		{
		if(mustSwapEndianness)
			{
			for(size_t i=0;i<numItems;++i)
				{
				DataParam temp=data[i];
				swapEndianness(temp);
				writeRaw(&temp,sizeof(DataParam));
				}
			}
		else
			writeRaw(data,numItems*sizeof(DataParam));
		}
	};

}

#endif
