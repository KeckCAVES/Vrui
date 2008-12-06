/***********************************************************************
RefCounted - Base class for objects with automatic destruction based on
thread-safe reference counting. Reference-counted objects must be
created using the single-object new operator.
Copyright (c) 2007 Oliver Kreylos

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

#ifndef THREADS_REFCOUNTED_INCLUDED
#define THREADS_REFCOUNTED_INCLUDED

#include <pthread.h>

namespace Threads {

class RefCounted
	{
	/* Elements: */
	private:
	pthread_mutex_t refCountMutex; // Mutual exclusion semaphore protecting the reference counter
	unsigned int refCount; // Current number of autopointers referencing this object
	
	/* Constructors and destructors: */
	public:
	RefCounted(void) // Creates an unreferenced object
		:refCount(0)
		{
		/* Initialize the mutex: */
		pthread_mutex_init(&refCountMutex,0);
		}
	RefCounted(const RefCounted& source) // Copy constructor; creates unreferenced object
		:refCount(0)
		{
		/* Initialize the mutex: */
		pthread_mutex_init(&refCountMutex,0);
		}
	RefCounted& operator=(const RefCounted& source) // Assignment operator; assigning does not change reference count
		{
		return *this;
		}
	virtual ~RefCounted(void) // Virtual destructor; called when the reference count reaches zero
		{
		/* Destroy the mutex: */
		pthread_mutex_destroy(&refCountMutex);
		}
	
	/* Methods: */
	void ref(void) // Method called when an autopointer starts referencing this object
		{
		pthread_mutex_lock(&refCountMutex);
		++refCount;
		pthread_mutex_unlock(&refCountMutex);
		}
	void unref(void) // Method called when an autopointer stops referencing an object; destroys object when reference count reaches zero
		{
		bool mustDelete=false;
		pthread_mutex_lock(&refCountMutex);
		--refCount;
		if(refCount==0)
			mustDelete=true;
		pthread_mutex_unlock(&refCountMutex);
		if(mustDelete)
			delete this;
		}
	};

}

#endif
