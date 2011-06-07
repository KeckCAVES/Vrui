/***********************************************************************
ClusterStandardFile - Class for high-performance reading/writing from/to
standard operating system files distributed among a cluster over a
multicast pipe.
Copyright (c) 2011 Oliver Kreylos

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

#include <Comm/ClusterStandardFile.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <Misc/ThrowStdErr.h>
#include <Comm/MulticastPipe.h>

namespace Comm {

/************************************
Methods of class ClusterStandardFile:
************************************/

size_t ClusterStandardFile::readData(IO::File::Byte* buffer,size_t bufferSize)
	{
	size_t readSize;
	
	if(pipe==0||pipe->isMaster())
		{
		/* Check if file needs to be repositioned: */
		if(lastOp==1&&readPos!=writePos)
			lseek(fd,readPos,SEEK_SET);
		lastOp=0;
		
		/* Read more data from source: */
		ssize_t readResult;
		do
			{
			readResult=::read(fd,buffer,bufferSize);
			}
		while(readResult<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR));
		
		/* Handle the result from the read call: */
		if(readResult<0)
			{
			/* Unknown error; probably a bad thing: */
			if(pipe!=0)
				{
				/* Signal error to the slave nodes: */
				pipe->write<int>(-1);
				pipe->finishMessage();
				}
			Misc::throwStdErr("Comm::ClusterStandardFile: Fatal error while reading from source");
			}
		readSize=size_t(readResult);
		
		/* Advance the read pointer: */
		readPos+=readResult;
		
		if(pipe!=0)
			{
			/* Forward the just-read data to the slave nodes: */
			pipe->write<int>(readSize);
			pipe->write<Byte>(buffer,readSize);
			pipe->finishMessage();
			}
		}
	else
		{
		/* Receive the size of the read data from the master node and check for error signal: */
		int messageSize=pipe->read<int>();
		if(messageSize<0)
			Misc::throwStdErr("Comm::ClusterStandardFile: Fatal error while reading from source");
		
		/* Read the data: */
		readSize=size_t(messageSize);
		pipe->read<Byte>(buffer,readSize);
		}
	
	return readSize;
	}

void ClusterStandardFile::writeData(const IO::File::Byte* buffer,size_t bufferSize)
	{
	if(pipe==0||pipe->isMaster())
		{
		/* Check if file needs to be repositioned: */
		if(lastOp==0&&writePos!=readPos)
			lseek(fd,writePos,SEEK_SET);
		lastOp=1;

		while(bufferSize>0)
			{
			ssize_t writeResult=::write(fd,buffer,bufferSize);
			if(writeResult>0)
				{
				/* Prepare to write more data: */
				buffer+=writeResult;
				bufferSize-=writeResult;

				/* Advance the write pointer: */
				writePos+=writeResult;
				}
			else if(writeResult<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR))
				{
				/* Do nothing */
				}
			else if(writeResult==0)
				{
				/* Sink has reached end-of-file: */
				if(pipe!=0)
					{
					pipe->write<int>(int(bufferSize));
					pipe->finishMessage();
					}
				throw WriteError(bufferSize);
				}
			else
				{
				/* Unknown error; probably a bad thing: */
				if(pipe!=0)
					{
					pipe->write<int>(-1);
					pipe->finishMessage();
					}
				Misc::throwStdErr("Comm::ClusterStandardFile: Fatal error while writing to sink");
				}
			}
		}
	else
		{
		/* Receive an error indicator from the master node: */
		int error=pipe->read<int>();
		if(error<0)
			Misc::throwStdErr("Comm::ClusterStandardFile: Fatal error while writing to sink");
		else
			throw WriteError(size_t(error));
		}
	}

ClusterStandardFile::ClusterStandardFile(const char* fileName,MulticastPipe* sPipe,IO::File::AccessMode sAccessMode)
	:IO::File(sAccessMode),
	 fd(-1),
	 lastOp(0),readPos(0),writePos(0),
	 pipe(sPipe)
	{
	if(pipe==0||pipe->isMaster())
		{
		/* Create flags and mode to open the file: */
		int flags=0x0;
		switch(sAccessMode)
			{
			case ReadOnly:
				flags=O_RDONLY;
				break;
			
			case WriteOnly:
				flags=O_WRONLY|O_CREAT|O_TRUNC;
				break;
			
			case ReadWrite:
				flags|=O_RDWR|O_CREAT;
				break;
			}
		
		mode_t mode=S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;
		
		/* Open the file: */
		fd=open(fileName,flags,mode);
		if(pipe!=0)
			{
			/* Send open result to slave nodes: */
			pipe->write<int>(fd<0);
			pipe->finishMessage();
			}
		
		if(fd<0)
			{
			const char* mode=0; // Just to shut up compiler
			switch(sAccessMode)
				{
				case ReadOnly:
					mode="reading";
					break;
				
				case WriteOnly:
					mode="writing";
					break;
				
				case ReadWrite:
					mode="reading/writing";
					break;
				}
			
			throw OpenError(Misc::printStdErrMsg("Comm::ClusterStandardFile: Unable to open file %s for %s",fileName,mode));
			}
		}
	else
		{
		/* Read status flag from the master node: */
		if(pipe->read<int>())
			{
			const char* mode=0; // Just to shut up compiler
			switch(sAccessMode)
				{
				case ReadOnly:
					mode="reading";
					break;
				
				case WriteOnly:
					mode="writing";
					break;
				
				case ReadWrite:
					mode="reading/writing";
					break;
				}
			
			throw OpenError(Misc::printStdErrMsg("Comm::ClusterStandardFile: Unable to open file %s for %s",fileName,mode));
			}
		}
	}

ClusterStandardFile::ClusterStandardFile(const char* fileName,MulticastPipe* sPipe,IO::File::AccessMode sAccessMode,int flags,int mode)
	:IO::File(sAccessMode),
	 fd(-1),
	 lastOp(0),readPos(0),writePos(0),
	 pipe(sPipe)
	{
	if(pipe==0||pipe->isMaster())
		{
		/* Adjust flags to open the file: */
		switch(sAccessMode)
			{
			case ReadOnly:
				flags&=~(O_WRONLY|O_RDWR|O_CREAT|O_TRUNC|O_APPEND);
				flags|=O_RDONLY;
				break;
			
			case WriteOnly:
				flags&=~(O_RDONLY|O_RDWR);
				flags|=O_WRONLY;
				break;
			
			case ReadWrite:
				flags&=~(O_RDONLY|O_WRONLY);
				flags|=O_RDWR;
				break;
			}
		
		/* Open the file: */
		fd=open(fileName,flags,mode);
		if(pipe!=0)
			{
			/* Send open result to slave nodes: */
			pipe->write<int>(fd<0);
			pipe->finishMessage();
			}
		
		if(fd<0)
			{
			const char* mode=0; // Just to shut up compiler
			switch(sAccessMode)
				{
				case ReadOnly:
					mode="reading";
					break;
				
				case WriteOnly:
					mode="writing";
					break;
				
				case ReadWrite:
					mode="reading/writing";
					break;
				}
			
			throw OpenError(Misc::printStdErrMsg("Comm::ClusterStandardFile: Unable to open file %s for %s",fileName,mode));
			}
		}
	else
		{
		/* Read status flag from the master node: */
		if(pipe->read<int>())
			{
			const char* mode=0; // Just to shut up compiler
			switch(sAccessMode)
				{
				case ReadOnly:
					mode="reading";
					break;
				
				case WriteOnly:
					mode="writing";
					break;
				
				case ReadWrite:
					mode="reading/writing";
					break;
				}
			
			throw OpenError(Misc::printStdErrMsg("Comm::ClusterStandardFile: Unable to open file %s for %s",fileName,mode));
			}
		}
	}

ClusterStandardFile::~ClusterStandardFile(void)
	{
	if(pipe==0||pipe->isMaster())
		{
		/* Flush the write buffer, and then close the file: */
		flush();
		close(fd);
		}
	
	/* Close the pipe: */
	delete pipe;
	}

void ClusterStandardFile::seekSet(ClusterStandardFile::Offset newOffset)
	{
	/* Need to implement this... */
	}

}

