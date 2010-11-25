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

#include <Misc/WriteBuffer.h>

namespace Misc {

/****************************
Methods of class WriteBuffer:
****************************/

void WriteBuffer::addTail(void)
	{
	BufferChunk* newTail=new BufferChunk;
	newTail->succ=0;
	tail->succ=newTail;
	tail=newTail;
	writePtr=tail->buffer;
	tailFree=BufferChunk::bufferSize;
	}

WriteBuffer::WriteBuffer(Endianness sEndianness)
	:head(new BufferChunk),tail(head),writePtr(tail->buffer),tailFree(BufferChunk::bufferSize)
	{
	tail->succ=0;
	setEndianness(sEndianness);
	}

WriteBuffer::WriteBuffer(bool sMustSwapEndianness)
	:head(new BufferChunk),tail(head),writePtr(tail->buffer),tailFree(BufferChunk::bufferSize)
	{
	tail->succ=0;
	setEndianness(sMustSwapEndianness);
	}

WriteBuffer::~WriteBuffer(void)
	{
	while(head!=0)
		{
		BufferChunk* succ=head->succ;
		delete head;
		head=succ;
		}
	}

void WriteBuffer::setEndianness(Endianness newEndianness)
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

void WriteBuffer::setEndianness(bool newMustSwapEndianness)
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

size_t WriteBuffer::getDataSize(void) const
	{
	size_t result=0;
	
	/* Accumulate the sizes of all full buffers: */
	for(const BufferChunk* bPtr=head;bPtr!=tail;bPtr=bPtr->succ)
		result+=BufferChunk::bufferSize;
	
	/* Add the amount of data in the last buffer chunk: */
	result+=BufferChunk::bufferSize-tailFree;
	
	return result;
	}

void WriteBuffer::clear(void)
	{
	/* Delete all buffer chunks except the head: */
	BufferChunk* bPtr=head->succ;
	while(bPtr!=0)
		{
		BufferChunk* succ=bPtr->succ;
		delete bPtr;
		bPtr=succ;
		}
	
	/* Re-initialize the buffer: */
	head->succ=0;
	tail=head;
	writePtr=tail->buffer;
	tailFree=BufferChunk::bufferSize;
	}

}
