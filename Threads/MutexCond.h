/***********************************************************************
MutexCond - Convenience class for condition variables that are protected
by their own mutual exclusion semaphores.
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

#ifndef THREADS_MUTEXCOND_INCLUDED
#define THREADS_MUTEXCOND_INCLUDED

#include <pthread.h>
#include <Misc/Time.h>

namespace Threads {

class MutexCond
	{
	/* Embedded classes: */
	public:
	class Lock // Class to obtain mutex locks using construction mechanism
		{
		friend class MutexCond;
		
		/* Elements: */
		private:
		pthread_mutex_t* mutexPtr; // Pointer to mutex that was locked
		
		/* Constructors and destructors: */
		public:
		Lock(MutexCond& mutexCond) // Locks the given mutex-protected condition variable
			:mutexPtr(&mutexCond.mutex)
			{
			/* Lock the mutex: */
			pthread_mutex_lock(mutexPtr);
			}
		private:
		Lock(const Lock& source); // Prohibit copy constructor
		Lock& operator=(const Lock& source); // Prohibit assignment
		public:
		~Lock(void)
			{
			/* Unlock the mutex: */
			pthread_mutex_unlock(mutexPtr);
			}
		};
	
	friend class Lock;
	
	/* Elements: */
	private:
	pthread_mutex_t mutex; // Low-level pthread mutex handle
	pthread_cond_t cond; // Low-level pthread condition variable handle
	
	/* Constructors and destructors: */
	public:
	MutexCond(void) // Creates default mutex and default condition variable
		{
		pthread_mutex_init(&mutex,0);
		pthread_cond_init(&cond,0);
		}
	MutexCond(pthread_mutexattr_t* mutexAttributes) // Creates mutex with given attributes and default condition variable
		{
		pthread_mutex_init(&mutex,mutexAttributes);
		pthread_cond_init(&cond,0);
		}
	MutexCond(pthread_mutexattr_t* mutexAttributes,pthread_condattr_t* condAttributes) // Creates mutex and condition variable with given attributes
		{
		pthread_mutex_init(&mutex,mutexAttributes);
		pthread_cond_init(&cond,condAttributes);
		}
	private:
	MutexCond(const MutexCond& source); // Prohibit copy constructor
	MutexCond& operator=(const MutexCond& source); // Prohibit assignment operator
	public:
	~MutexCond(void)
		{
		pthread_mutex_destroy(&mutex);
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
	void wait(void) // Waits on condition variable
		{
		Lock lock(*this);
		pthread_cond_wait(&cond,&mutex);
		}
	bool timedWait(const Misc::Time& abstime) // Waits on condition variable; returns true if signal occurred; returns false if time expires
		{
		Lock lock(*this);
		return pthread_cond_timedwait(&cond,&mutex,&abstime)==0;
		}
	void wait(const Lock& lock) // Waits on condition variable when lock is already established
		{
		pthread_cond_wait(&cond,lock.mutexPtr);
		}
	bool timedWait(const Lock& lock,const Misc::Time& abstime) // Waits on condition variable when lock is already established; returns true if signal occurred; returns false if time expires
		{
		return pthread_cond_timedwait(&cond,lock.mutexPtr,&abstime)==0;
		}
	};

}

#endif
