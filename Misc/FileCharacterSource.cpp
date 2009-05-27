/***********************************************************************
FileCharacterSource - High-performance ASCII file reader for standard
files.
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

#include <Misc/FileCharacterSource.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <Misc/ThrowStdErr.h>

namespace Misc {

/************************************
Methods of class FileCharacterSource:
************************************/

void FileCharacterSource::fillBuffer(void)
	{
	/* Read at most one buffer's worth of data from the input file: */
	ssize_t readSize=read(inputFd,buffer,bufferSize);
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

FileCharacterSource::FileCharacterSource(const char* inputFileName,size_t sBufferSize)
	:CharacterSource(sBufferSize),
	 inputFd(-1)
	{
	/* Open the input file: */
	if((inputFd=open(inputFileName,O_RDONLY))<0)
		throw OpenError(printStdErrMsg("FileCharacterSource: Error while opening input file %s",inputFileName));
	}

FileCharacterSource::~FileCharacterSource(void)
	{
	/* Close the input file: */
	if(inputFd>=0)
		close(inputFd);
	}

}
