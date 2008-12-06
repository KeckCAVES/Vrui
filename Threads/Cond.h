/***********************************************************************
Cond - Wrapper class for pthreads condition variables, mostly providing
"resource allocation as creation" paradigm.
Copyright (c) 2005 Oliver Kreylos

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

#ifndef THREADS_COND_INCLUDED
#define THREADS_COND_INCLUDED

#include <pthread.h>
#include <Misc/Time.h>
#include <Threads/Mutex.h>

namespace Threads {

class Cond
	{
	/* Elements: */
	private:
	pthread_cond_t cond; // Low-level pthread condition variable handle
	
	/* Constructors and destructors: */
	public:
	Cond(pthread_condattr_t* condAttributes =0) // Creates condition variable with given attributes (or default condition variable)
		{
		pthread_cond_init(&cond,condAttributes);
		}
	private:
	Cond(const Cond& source); // Prohibit copy constructor
	Cond& operator=(const Cond& source); // Prohibit assignment
	public:
	~Cond(void)
		{
		pthread_cond_destroy(&cond);
		}
	
	/* Methods: */
	void signal(void) // Signals the condition variable
		{
		pthread_cond_signal(&cond);
		}
	void broadcast(void) // Broadcasts the condition variable
		{
		pthread_cond_broadcast(&cond);
		}
	void wait(Mutex& mutex) // Waits on condition variable; calling thread must hold lock on given mutex
		{
		pthread_cond_wait(&cond,&mutex.mutex);
		}
	bool timedWait(Mutex& mutex,const Misc::Time& abstime) // Waits on condition variable; returns true if signal occurred; returns false if time expires
		{
		return pthread_cond_timedwait(&cond,&mutex.mutex,&abstime)==0;
		}
	};

}

#endif
