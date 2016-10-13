/***********************************************************************
VariableMemoryFile - Class to write to variable-sized in-memory files as
temporary file storage.
Copyright (c) 2011-2016 Oliver Kreylos

This file is part of the I/O Support Library (IO).

The I/O Support Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The I/O Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the I/O Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <IO/VariableMemoryFile.h>

namespace IO {

/************************************************
Methods of class VariableMemoryFile::BufferChain:
************************************************/

VariableMemoryFile::BufferChain::~BufferChain(void)
	{
	/* Delete the buffer chain: */
	while(head!=0)
		{
		BufferHeader* succ=head->succ;
		delete[] reinterpret_cast<Byte*>(head);
		head=succ;
		}
	}

size_t VariableMemoryFile::BufferChain::getDataSize(void) const
	{
	size_t result=0;
	for(const BufferHeader* bhPtr=head;bhPtr!=0;bhPtr=bhPtr->succ)
		result+=bhPtr->size;
	return result;
	}

/***********************************
Methods of class VariableMemoryFile:
***********************************/

size_t VariableMemoryFile::readData(File::Byte* buffer,size_t bufferSize)
	{
	/* Get the next buffer from the buffer list: */
	BufferHeader* nextBuffer=buffer==0?head:reinterpret_cast<BufferHeader*>(buffer)[-1].succ;
	if(nextBuffer!=0)
		{
		/* Install the next buffer as the file's read buffer: */
		setReadBuffer(nextBuffer->size,reinterpret_cast<Byte*>(nextBuffer+1),false);
		return nextBuffer->size;
		}
	else
		return 0; // Signal end-of-file
	}

void VariableMemoryFile::writeData(const File::Byte* buffer,size_t bufferSize)
	{
	/* Append the filled current buffer to the buffer list: */
	current->size=bufferSize;
	if(tail!=0)
		tail->succ=current;
	else
		head=current;
	tail=current;
	
	/* Allocate a new buffer: */
	Byte* newBuffer=new Byte[writeBufferSize+sizeof(BufferHeader)];
	current=new(newBuffer) BufferHeader;
	
	/* Install the new buffer as the buffered file's write buffer: */
	setWriteBuffer(writeBufferSize,reinterpret_cast<Byte*>(current+1),false); // current+1 points to actual data in buffer
	}

size_t VariableMemoryFile::writeDataUpTo(const File::Byte* buffer,size_t bufferSize)
	{
	/* Append the filled current buffer to the buffer list: */
	current->size=bufferSize;
	if(tail!=0)
		tail->succ=current;
	else
		head=current;
	tail=current;
	
	/* Allocate a new buffer: */
	Byte* newBuffer=new Byte[writeBufferSize+sizeof(BufferHeader)];
	current=new(newBuffer) BufferHeader;
	
	/* Install the new buffer as the buffered file's write buffer: */
	setWriteBuffer(writeBufferSize,reinterpret_cast<Byte*>(current+1),false); // current+1 points to actual data in buffer
	
	return bufferSize;
	}

VariableMemoryFile::VariableMemoryFile(size_t sWriteBufferSize)
	:File(),
	 writeBufferSize(sWriteBufferSize),
	 head(0),tail(0),current(0)
	{
	/* Allocate a new buffer: */
	Byte* newBuffer=new Byte[writeBufferSize+sizeof(BufferHeader)];
	current=new(newBuffer) BufferHeader;
	
	/* Disable read-through: */
	canReadThrough=false;
	
	/* Install the new buffer as the buffered file's write buffer: */
	setWriteBuffer(writeBufferSize,reinterpret_cast<Byte*>(current+1),false); // current+1 points to actual data in buffer
	canWriteThrough=false;
	}

VariableMemoryFile::~VariableMemoryFile(void)
	{
	/* Delete the buffer list: */
	while(head!=0)
		{
		BufferHeader* succ=head->succ;
		delete[] reinterpret_cast<Byte*>(head);
		head=succ;
		}
	
	/* Uninstall the buffered file's read and write buffers: */
	setReadBuffer(0,0,false);
	setWriteBuffer(0,0,false);
	
	/* Delete the current buffer: */
	delete[] reinterpret_cast<Byte*>(current);
	}

size_t VariableMemoryFile::getWriteBufferSize(void) const
	{
	return writeBufferSize;
	}

void VariableMemoryFile::resizeWriteBuffer(size_t newWriteBufferSize)
	{
	/* Can't actually do anything right now; just use the new size for the next allocated buffer: */
	writeBufferSize=newWriteBufferSize;
	}

size_t VariableMemoryFile::getDataSize(void) const
	{
	size_t result=0;
	
	/* Add the data sizes of all finished buffers: */
	for(const BufferHeader* bhPtr=head;bhPtr!=0;bhPtr=bhPtr->succ)
		result+=bhPtr->size;
	
	/* Add the amount of data in the current write buffer: */
	result+=getWritePtr();
	
	return result;
	}

void VariableMemoryFile::storeBuffers(VariableMemoryFile::BufferChain& chain)
	{
	/* Delete all existing data in the buffer chain: */
	while(chain.head!=0)
		{
		BufferHeader* succ=chain.head->succ;
		delete[] reinterpret_cast<Byte*>(chain.head);
		chain.head=succ;
		}
	
	/* Flush the write buffer: */
	flush();
	
	/* Move the current buffer list to the buffer chain: */
	chain.head=head;
	
	/* Clear the current buffer list: */
	head=0;
	tail=0;
	}

void VariableMemoryFile::clear(void)
	{
	/* Delete the buffer list: */
	while(head!=0)
		{
		BufferHeader* succ=head->succ;
		delete[] reinterpret_cast<Byte*>(head);
		head=succ;
		}
	tail=0;
	
	/* Reset the buffered file's read buffer: */
	setReadBuffer(0,0,false);
	
	/* Install the current buffer as the buffered file's write buffer: */
	setWriteBuffer(writeBufferSize,reinterpret_cast<Byte*>(current+1),false); // current+1 points to actual data in buffer
	}

}
