/***********************************************************************
Spinlock - Wrapper class for pthreads spinlocks, mostly providing
"resource allocation as creation" paradigm and lock objects. If the
host's pthreads library does not provide spinlocks, this class simulates
them using mutexes instead.
Copyright (c) 2011 Oliver Kreylos

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

#ifndef THREADS_SPINLOCK_INCLUDED
#define THREADS_SPINLOCK_INCLUDED

#include <pthread.h>
#include <Threads/Config.h>

namespace Threads {

class Spinlock
	{
	/* Embedded classes: */
	public:
	class Lock // Class to obtain spinlock locks using construction mechanism
		{
		/* Elements: */
		private:
		#if THREADS_CONFIG_HAVE_SPINLOCKS
		pthread_spinlock_t* spinlockPtr; // Pointer to spinlock that was locked
		#else
		pthread_mutex_t* spinlockPtr; // Pointer to spinlock that was locked
		#endif
		
		/* Constructors and destructors: */
		public:
		Lock(Spinlock& spinlock) // Locks the given mutex
			:spinlockPtr(&spinlock.spinlock)
			{
			/* Lock the spinlock: */
			#if THREADS_CONFIG_HAVE_SPINLOCKS
			pthread_spin_lock(spinlockPtr);
			#else
			pthread_mutex_lock(spinlockPtr);
			#endif
			}
		private:
		Lock(const Lock& source); // Prohibit copy constructor
		Lock& operator=(const Lock& source); // Prohibit assignment
		public:
		~Lock(void)
			{
			/* Unlock the spinlock: */
			#if THREADS_CONFIG_HAVE_SPINLOCKS
			pthread_spin_unlock(spinlockPtr);
			#else
			pthread_mutex_unlock(spinlockPtr);
			#endif
			}
		};
	
	friend class Lock;
	
	/* Elements: */
	private:
	#if THREADS_CONFIG_HAVE_SPINLOCKS
	pthread_spinlock_t spinlock; // Low-level pthread spinlock handle
	#else
	pthread_mutex_t spinlock; // Low-level pthread mutex handle standing in for missing spinlock
	#endif
	
	/* Constructors and destructors: */
	public:
	Spinlock(bool processShared=false) // Creates spinlock with given share flag
		{
		#if THREADS_CONFIG_HAVE_SPINLOCKS
		pthread_spin_init(&spinlock,processShared);
		#else
		pthread_mutex_init(&spinlock,0);
		#endif
		}
	private:
	Spinlock(const Spinlock& source); // Prohibit copy constructor
	Spinlock& operator=(const Spinlock& source); // Prohibit assignment
	public:
	~Spinlock(void)
		{
		#if THREADS_CONFIG_HAVE_SPINLOCKS
		pthread_spin_destroy(&spinlock);
		#else
		pthread_mutex_destroy(&spinlock);
		#endif
		}
	
	/* Methods: */
	void lock(void) // Locks spinlock; blocks until lock is held
		{
		#if THREADS_CONFIG_HAVE_SPINLOCKS
		pthread_spin_lock(&spinlock);
		#else
		pthread_mutex_lock(&spinlock);
		#endif
		}
	bool tryLock(void) // Locks spinlock and returns true if currently unlocked; returns false otherwise
		{
		#if THREADS_CONFIG_HAVE_SPINLOCKS
		return pthread_spin_trylock(&spinlock)==0;
		#else
		return pthread_mutex_trylock(&spinlock)==0;
		#endif
		}
	void unlock(void) // Unlocks spinlock
		{
		#if THREADS_CONFIG_HAVE_SPINLOCKS
		pthread_spin_unlock(&spinlock);
		#else
		pthread_mutex_unlock(&spinlock);
		#endif
		}
	};

}

#endif
