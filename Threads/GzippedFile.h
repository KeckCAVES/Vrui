/***********************************************************************
GzippedFile - Class for high-performance reading from gzip-compressed
standard files with background readahead and decompression.
Copyright (c) 2011 Oliver Kreylos

This file is part of the Portable Threading Library (Threads).

The Portable Threading Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Portable Threading Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Portable Threading Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef THREADS_GZIPPEDFILE_INCLUDED
#define THREADS_GZIPPEDFILE_INCLUDED

#include <IO/File.h>
#include <Threads/Mutex.h>
#include <Threads/Cond.h>
#include <Threads/Thread.h>

namespace Threads {

class GzippedFile:public IO::File
	{
	/* Elements: */
	private:
	void* inputFile; // File descriptor of gzipped input file
	size_t halfBufferSize; // Size of one buffer half
	Byte* doubleBuffer; // Pointer to allocated double buffer
	Mutex bufferMutex; // Mutex to serialize access to the character buffer
	Cond bufferCond; // Condition variable to signal buffer events
	unsigned int numFilledBuffers; // Number of filled buffer halves (0, 1, or 2)
	size_t dataSizes[2]; // Amount of data in the two buffer halves
	volatile unsigned int encounteredReadError; // Flag to signal a read error to the main thread
	Thread readAheadThread; // Descriptor for readahead and decompression thread
	unsigned int nextReadBuffer; // Index of buffer from which to read data next
	
	/* Protected methods from BufferedFile: */
	protected:
	virtual size_t readData(Byte* buffer,size_t bufferSize);
	virtual void writeData(const Byte* buffer,size_t bufferSize);
	
	/* Private methods: */
	private:
	void* readAheadThreadMethod(void);
	
	/* Constructors and destructors: */
	public:
	GzippedFile(const char* fileName); // Opens a gzip-compressed standard file with "DontCare" endianness setting
	virtual ~GzippedFile(void);
	
	/* Methods from BufferedFile: */
	virtual size_t resizeReadBuffer(size_t newReadBufferSize);
	};

}

#endif
