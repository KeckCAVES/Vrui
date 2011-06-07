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

#include <Threads/GzippedFile.h>

#include <zlib.h>
#include <Misc/ThrowStdErr.h>

namespace Threads {

/****************************
Methods of class GzippedFile:
****************************/

size_t GzippedFile::readData(IO::File::Byte* buffer,size_t bufferSize)
	{
	/* Wait until at least one buffer half is available: */
	{
	Mutex::Lock bufferLock(bufferMutex);
	while(numFilledBuffers==0)
		bufferCond.wait(bufferMutex);
	--numFilledBuffers;
	}
	
	/* Check for read errors: */
	if(encounteredReadError==nextReadBuffer)
		throw Error(Misc::printStdErrMsg("Threads::GzippedFile: Fatal error while reading from file"));
	
	/* Hand the next buffer half to the base class: */
	setReadBuffer(halfBufferSize,doubleBuffer+nextReadBuffer*halfBufferSize,false);
	size_t result=dataSizes[nextReadBuffer];
	nextReadBuffer=1-nextReadBuffer;
	
	/* Return amount of data in the buffer: */
	return result;
	}

void GzippedFile::writeData(const IO::File::Byte* buffer,size_t bufferSize)
	{
	/* Can't write to gzipped files; throw a fatal error: */
	throw Error(Misc::printStdErrMsg("Threads::GzippedFile: Writing to gzipped files not supported"));
	}

void* GzippedFile::readAheadThreadMethod(void)
	{
	/* Enable immediate cancellation: */
	Thread::setCancelState(Thread::CANCEL_ENABLE);
	Thread::setCancelType(Thread::CANCEL_ASYNCHRONOUS);
	
	unsigned int nextBufferHalf=0;
	while(true)
		{
		/* Read into the next buffer half: */
		int readSize=gzread(inputFile,doubleBuffer+halfBufferSize*nextBufferHalf,halfBufferSize);
		if(readSize<0)
			{
			/* Signal an error and bail out: */
			encounteredReadError=nextBufferHalf;
			break;
			}
		dataSizes[nextBufferHalf]=readSize;
		
		/* Go to the next buffer half: */
		nextBufferHalf=1-nextBufferHalf;
		
		{
		Mutex::Lock bufferLock(bufferMutex);
		
		/* Mark the buffer as filled: */
		++numFilledBuffers;
		if(numFilledBuffers==1)
			{
			/* Wake up a potentially waiting reader: */
			bufferCond.signal();
			}
		
		/* Block until there is room in the buffer: */
		while(numFilledBuffers==2)
			bufferCond.wait(bufferMutex);
		}
		}
	
	return 0;
	}

GzippedFile::GzippedFile(const char* fileName)
	:IO::File(),
	 inputFile(0),
	 halfBufferSize(8192),
	 doubleBuffer(0),
	 numFilledBuffers(0),encounteredReadError(2),nextReadBuffer(0)
	{
	/* Open the compressed input file: */
	if((inputFile=gzopen(fileName,"r"))==0)
		throw OpenError(Misc::printStdErrMsg("Threads::GzippedFile: Error while opening gzipped input file %s",fileName));
	
	/* Allocate the read double buffer: */
	doubleBuffer=new Byte[halfBufferSize*2];
	
	/* Start the readahead thread: */
	readAheadThread.start(this,&GzippedFile::readAheadThreadMethod);
	}

GzippedFile::~GzippedFile(void)
	{
	/* Terminate the readahead thread: */
	readAheadThread.cancel();
	readAheadThread.join();
	
	/* Release the buffered file's buffers: */
	setReadBuffer(0,0,false);
	delete[] doubleBuffer;
	
	/* Close the compressed input file: */
	if(inputFile!=0)
		gzclose(inputFile);
	}

size_t GzippedFile::resizeReadBuffer(size_t newReadBufferSize)
	{
	/* Ignore and return previous buffer size: */
	return halfBufferSize;
	}

}
