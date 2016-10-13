/***********************************************************************
Pipe - Wrapper class for UNIX unnamed pipes for inter-process
communication between a parent and child process, or for FIFO self-
communication.
Copyright (c) 2016 Oliver Kreylos

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

#include <Misc/Pipe.h>

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <Misc/ThrowStdErr.h>

namespace Misc {

/*********************
Methods of class Pipe:
*********************/

Pipe::Pipe(void)
	{
	/* Open a pipe: */
	pipeFds[1]=pipeFds[0]=-1;
	if(pipe(pipeFds)<0)
		{
		int error=errno;
		Misc::throwStdErr("Pipe::Pipe: Could not open pipe due to error %d (%s)",error,strerror(error));
		}
	}

Pipe::~Pipe(void)
	{
	/* Close both pipe ends: */
	for(int i=0;i<2;++i)
		{
		if(pipeFds[0]>=0)
			close(pipeFds[0]);
		}
	}

void Pipe::closeRead(void)
	{
	/* Close the read end of the pipe: */
	if(pipeFds[0]>=0)
		close(pipeFds[0]);
	pipeFds[0]=-1;
	}

void Pipe::closeWrite(void)
	{
	/* Close the write end of the pipe: */
	if(pipeFds[1]>=0)
		close(pipeFds[1]);
	pipeFds[1]=-1;
	}

}
