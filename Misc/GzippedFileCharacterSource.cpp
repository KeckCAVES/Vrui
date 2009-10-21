/***********************************************************************
GzippedFileCharacterSource - High-performance ASCII file reader for
gzip-compressed standard files.
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

#include <Misc/GzippedFileCharacterSource.h>

#include <zlib.h>
#include <Misc/ThrowStdErr.h>

namespace Misc {

/************************************
Methods of class FileCharacterSource:
************************************/

void GzippedFileCharacterSource::fillBuffer(void)
	{
	/* Read at most one buffer's worth of data from the compressed input file: */
	int readSize=gzread(inputFile,buffer,bufferSize);
	if(readSize<0) // That's an error condition
		throw ReadError();
	
	/* Check if the end of file has been read: */
	if(size_t(readSize)<bufferSize)
		{
		/* Set the buffer end pointer and the EOF pointer to the shortened content: */
		bufferEnd=buffer+readSize;
		eofPtr=bufferEnd;
		}
	
	/* Reset the read pointer: */
	rPtr=buffer;
	}

GzippedFileCharacterSource::GzippedFileCharacterSource(const char* inputFileName,size_t sBufferSize)
	:CharacterSource(sBufferSize),
	 inputFile(0)
	{
	/* Open the compressed input file: */
	if((inputFile=gzopen(inputFileName,"r"))==0)
		throw OpenError(printStdErrMsg("GzippedFileCharacterSource: Error while opening gzipped input file %s",inputFileName));
	}

GzippedFileCharacterSource::~GzippedFileCharacterSource(void)
	{
	/* Close the compressed input file: */
	if(inputFile!=0)
		gzclose(inputFile);
	}

}
