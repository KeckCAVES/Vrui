/***********************************************************************
GzippedFileCharacterSource - High-performance ASCII file reader for
gzip- compressed standard files with background readahead and
decompression.
Copyright (c) 2009 Oliver Kreylos

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

#include <Threads/GzippedFileCharacterSource.h>

#include <zlib.h>
#include <Misc/ThrowStdErr.h>

namespace Threads {

/************************************
Methods of class FileCharacterSource:
************************************/

void* GzippedFileCharacterSource::readAheadThreadMethod(void)
	{
	/* Enable immediate cancellation: */
	Thread::setCancelState(Thread::CANCEL_ENABLE);
	Thread::setCancelType(Thread::CANCEL_ASYNCHRONOUS);
	
	unsigned int nextBufferHalf=0;
	while(true)
		{
		/* Read into the next buffer half: */
		int readSize=gzread(inputFile,buffer+halfBufferSize*nextBufferHalf,halfBufferSize);
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

void GzippedFileCharacterSource::fillBuffer(void)
	{
	/* Check for an error signal from the readahead thread: */
	if(encounteredReadError==nextReadBuffer)
		throw ReadError();
	
	{
	Mutex::Lock bufferLock(bufferMutex);
	
	/* Release the previous buffer if there was one: */
	if(haveReadBuffers)
		{
		--numFilledBuffers;
		if(numFilledBuffers==1)
			{
			/* Wake up a potentially waiting readahead thread: */
			bufferCond.signal();
			}
		}
	
	/* Block until there is data in the buffer: */
	while(numFilledBuffers==0)
		bufferCond.wait(bufferMutex);
	}
	
	/* Set up to read from the next buffer half: */
	rPtr=buffer+halfBufferSize*nextReadBuffer;
	bufferEnd=rPtr+dataSizes[nextReadBuffer];
	
	/* Check if the end of file has been reached: */
	if(dataSizes[nextReadBuffer]!=halfBufferSize)
		eofPtr=bufferEnd;
	
	/* Go to the next buffer half: */
	nextReadBuffer=1-nextReadBuffer;
	haveReadBuffers=true;
	}

GzippedFileCharacterSource::GzippedFileCharacterSource(const char* inputFileName,size_t sBufferSize)
	:Misc::CharacterSource(sBufferSize),
	 inputFile(0),halfBufferSize(bufferSize/2),
	 numFilledBuffers(0),encounteredReadError(2),nextReadBuffer(0),haveReadBuffers(false)
	{
	/* Open the compressed input file: */
	if((inputFile=gzopen(inputFileName,"r"))==0)
		throw OpenError(Misc::printStdErrMsg("GzippedFileCharacterSource: Error while opening gzipped input file %s",inputFileName));
	
	/* Start the readahead thread: */
	readAheadThread.start(this,&GzippedFileCharacterSource::readAheadThreadMethod);
	}

GzippedFileCharacterSource::~GzippedFileCharacterSource(void)
	{
	/* Terminate the readahead thread: */
	readAheadThread.cancel();
	readAheadThread.join();
	
	/* Close the compressed input file: */
	if(inputFile!=0)
		gzclose(inputFile);
	}

}
