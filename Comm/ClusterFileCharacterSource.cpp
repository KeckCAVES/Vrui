/***********************************************************************
ClusterFileCharacterSource - High-performance ASCII file reader for
files distributed across a cluster using a multicast pipe.
Copyright (c) 2007-2009 Oliver Kreylos

This file is part of the Portable Communications Library (Comm).

The Portable Communications Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Portable Communications Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Portable Communications Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Comm/ClusterFileCharacterSource.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <Misc/ThrowStdErr.h>
#include <Comm/MulticastPipe.h>

namespace Comm {

/*******************************************
Methods of class ClusterFileCharacterSource:
*******************************************/

void ClusterFileCharacterSource::fillBuffer(void)
	{
	int readSize;
	if(pipe==0||pipe->isMaster())
		{
		/* Read at most one buffer's worth of data from the input file: */
		readSize=int(read(inputFd,buffer,bufferSize));
		if(readSize<0) // That's an error condition
			{
			if(pipe!=0)
				{
				/* Signal the read error to the slaves: */
				pipe->write<int>(-1);
				pipe->finishMessage();
				}
			throw ReadError();
			}
		
		if(pipe!=0)
			{
			/* Send the read buffer to the slaves: */
			pipe->write<int>(readSize);
			pipe->write<unsigned char>(buffer,readSize);
			pipe->finishMessage();
			}
		}
	else
		{
		/* Receive the size of the read data from the master: */
		readSize=pipe->read<int>();
		if(readSize<0) // That's an error condition
			throw ReadError();
		
		/* Receive the read buffer: */
		pipe->read<unsigned char>(buffer,readSize);
		}
	
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

ClusterFileCharacterSource::ClusterFileCharacterSource(const char* inputFileName,MulticastPipe* sPipe,size_t sBufferSize)
	:CharacterSource(sBufferSize),
	 inputFd(-1),
	 pipe(sPipe)
	{
	bool ok;
	if(pipe==0||pipe->isMaster())
		{
		/* Open the input file: */
		inputFd=open(inputFileName,O_RDONLY);
		ok=inputFd>=0;
		
		if(pipe!=0)
			{
			/* Send an error indicator to the slaves: */
			pipe->write<int>(ok?1:0);
			pipe->finishMessage();
			}
		}
	else
		{
		/* Receive the error indicator from the master: */
		ok=pipe->read<int>()!=0;
		}
	
	if(!ok)
		throw OpenError(Misc::printStdErrMsg("ClusterFileCharacterSource: Error while opening input file %s",inputFileName));
	}

ClusterFileCharacterSource::~ClusterFileCharacterSource(void)
	{
	/* Close the input file: */
	if(inputFd>=0)
		close(inputFd);
	
	/* Close the pipe: */
	delete pipe;
	}

}
