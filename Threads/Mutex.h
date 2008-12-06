/***********************************************************************
Mutex - Wrapper class for pthreads mutual exclusion semaphores, mostly
providing "resource allocation as creation" paradigm and lock objects.
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

#ifndef THREADS_MUTEX_INCLUDED
#define THREADS_MUTEX_INCLUDED

#include <pthread.h>

/* Forward declarations: */
namespace Threads {
class Cond;
}

namespace Threads {

class Mutex
	{
	/* Embedded classes: */
	public:
	class Lock // Class to obtain mutex locks using construction mechanism
		{
		/* Elements: */
		private:
		pthread_mutex_t* mutexPtr; // Pointer to mutex that was locked
		
		/* Constructors and destructors: */
		public:
		Lock(Mutex& mutex) // Locks the given mutex
			:mutexPtr(&mutex.mutex)
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
	friend class Cond;
	
	/* Elements: */
	private:
	pthread_mutex_t mutex; // Low-level pthread mutex handle
	
	/* Constructors and destructors: */
	public:
	Mutex(const pthread_mutexattr_t* mutexAttribute =0) // Creates mutex with given attributes (or default mutex)
		{
		pthread_mutex_init(&mutex,mutexAttribute);
		}
	private:
	Mutex(const Mutex& source); // Prohibit copy constructor
	Mutex& operator=(const Mutex& source); // Prohibit assignment
	public:
	~Mutex(void)
		{
		pthread_mutex_destroy(&mutex);
		}
	
	/* Methods: */
	void lock(void) // Locks mutex; blocks until lock is held
		{
		pthread_mutex_lock(&mutex);
		}
	bool tryLock(void) // Locks mutex and returns true if currently unlocked; returns false otherwise
		{
		return pthread_mutex_trylock(&mutex)==0;
		}
	void unlock(void) // Unlocks mutex
		{
		pthread_mutex_unlock(&mutex);
		}
	};

}

#endif
