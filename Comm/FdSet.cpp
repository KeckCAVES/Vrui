/***********************************************************************
FdSet - Class to simplify using sets of file descriptors for the select
and pselect system calls
Copyright (c) 2008 Oliver Kreylos

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

#include <errno.h>
#include <sys/select.h>
#include <Misc/ThrowStdErr.h>

#include <Comm/FdSet.h>

namespace Comm {

/*******************************
Friend functions of class FdSet:
*******************************/

int select(FdSet* readFdSet,FdSet* writeFdSet,FdSet* exceptFdSet,struct timeval* timeout)
	{
	/* Find the largest file descriptor in any of the sets: */
	int maxFd=-1;
	if(readFdSet!=0&&maxFd<readFdSet->maxFd)
		maxFd=readFdSet->maxFd;
	if(writeFdSet!=0&&maxFd<writeFdSet->maxFd)
		maxFd=writeFdSet->maxFd;
	if(exceptFdSet!=0&&maxFd<exceptFdSet->maxFd)
		maxFd=exceptFdSet->maxFd;
	
	/* Call the select() function: */
	int result=select(maxFd+1,readFdSet,writeFdSet,exceptFdSet,timeout);
	if(result<0)
		{
		switch(errno)
			{
			case EBADF:
				Misc::throwStdErr("select failed due to bad file descriptor");
				break;
			
			case EINTR:
				/* Clear the file descriptor sets so that the caller doesn't get tripped up: */
				if(readFdSet!=0)
					readFdSet->clear();
				if(writeFdSet!=0)
					writeFdSet->clear();
				if(exceptFdSet!=0)
					exceptFdSet->clear();
				break;
			
			case EINVAL:
				Misc::throwStdErr("select failed due to invalid timeout value");
				break;
			
			case ENOMEM:
				Misc::throwStdErr("select failed due to internal error");
				break;
			
			default:
				Misc::throwStdErr("select failed for unknown reasons");
			}
		}
	else
		{
		/* Update the file descriptor sets: */
		if(readFdSet!=0)
			readFdSet->update();
		if(writeFdSet!=0)
			writeFdSet->update();
		if(exceptFdSet!=0)
			exceptFdSet->update();
		}
	
	return result;
	}

int pselect(FdSet* readFdSet,FdSet* writeFdSet,FdSet* exceptFdSet,const struct timespec* timeout,const sigset_t* sigmask)
	{
	/* Find the largest file descriptor in any of the sets: */
	int maxFd=-1;
	if(readFdSet!=0&&maxFd<readFdSet->maxFd)
		maxFd=readFdSet->maxFd;
	if(writeFdSet!=0&&maxFd<writeFdSet->maxFd)
		maxFd=writeFdSet->maxFd;
	if(exceptFdSet!=0&&maxFd<exceptFdSet->maxFd)
		maxFd=exceptFdSet->maxFd;
	
	/* Call the select() function: */
	int result=pselect(maxFd+1,readFdSet,writeFdSet,exceptFdSet,timeout,sigmask);
	if(result<0)
		{
		switch(errno)
			{
			case EBADF:
				Misc::throwStdErr("pselect failed due to invalid file descriptor");
				break;
			
			case EINTR:
				/* Clear the file descriptor sets so that the caller doesn't get tripped up: */
				if(readFdSet!=0)
					readFdSet->clear();
				if(writeFdSet!=0)
					writeFdSet->clear();
				if(exceptFdSet!=0)
					exceptFdSet->clear();
				break;
			
			case EINVAL:
				Misc::throwStdErr("pselect failed due to invalid timeout value");
				break;
			
			case ENOMEM:
				Misc::throwStdErr("pselect failed due to lack of memory");
				break;
			
			default:
				Misc::throwStdErr("pselect failed for unknown reasons");
			}
		}
	else
		{
		/* Update the file descriptor sets: */
		if(readFdSet!=0)
			readFdSet->update();
		if(writeFdSet!=0)
			writeFdSet->update();
		if(exceptFdSet!=0)
			exceptFdSet->update();
		}
	
	return result;
	}

}
