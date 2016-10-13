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

#ifndef MISC_PIPE_INCLUDED
#define MISC_PIPE_INCLUDED

#include <unistd.h>

namespace Misc {

class Pipe
	{
	/* Elements: */
	private:
	int pipeFds[2]; // File descriptors for the read and write ends of the pipe, respectively
	
	/* Constructors and destructors: */
	public:
	Pipe(void); // Creates an unnamed pipe; throws exception on error
	private:
	Pipe(const Pipe& source); // Prohibit copy constructor
	Pipe& operator=(const Pipe& source); // Prohibit assignment operator
	public:
	~Pipe(void); // Closes the pipe
	
	/* Methods: */
	int getReadFd(void) const // Returns the file descriptor for the read end of the pipe
		{
		return pipeFds[0];
		}
	ssize_t read(void* buffer,size_t count) // Reads from the read end of the pipe
		{
		return ::read(pipeFds[0],buffer,count);
		}
	void closeRead(void); // Closes the read end of the pipe
	int getWriteFd(void) const // Returns the file descriptor for the write end of the pipe
		{
		return pipeFds[1];
		}
	ssize_t write(const void* buffer,size_t count) // Writes to the write end of the pipe
		{
		return ::write(pipeFds[1],buffer,count);
		}
	void closeWrite(void); // Closes the write end of the pipe
	};

}

#endif
