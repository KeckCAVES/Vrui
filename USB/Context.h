/***********************************************************************
Context - Class representing libusb library contexts.
Copyright (c) 2010-2016 Oliver Kreylos

This file is part of the USB Support Library (USB).

The USB Support Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The USB Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the USB Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef USB_CONTEXT_INCLUDED
#define USB_CONTEXT_INCLUDED

#include <Misc/Autopointer.h>
#include <Threads/Mutex.h>
#include <Threads/Thread.h>

/* Forward declarations: */
struct libusb_context;

namespace USB {

class Context;
typedef Misc::Autopointer<Context> ContextPtr; // Type for pointers to the singleton USB context object

class Context
	{
	friend class Misc::Autopointer<Context>;
	
	/* Elements: */
	private:
	static Context theContext; // The singleton USB context
	Threads::Mutex refMutex; // Mutex protecting the ref() and unref() methods to guarantee that a call to acquireContext returns a valid context
	unsigned int refCount; // Reference counter for the singleton USB context
	libusb_context* context; // USB context handle from the USB library
	Threads::Thread eventHandlingThread; // Thread to process and dispatch USB events in the background
	
	/* Private methods: */
	void* eventHandlingThreadMethod(void); // Background event handling method
	void ref(void); // Adds a reference to the context
	void unref(void); // Removes a reference from the context
	
	/* Constructors and destructors: */
	Context(void); // Creates an uninitialized USB context
	Context(const Context& source); // Prohibit copy constructor
	Context& operator=(const Context& source); // Prohibit assignment operator
	
	/* Methods: */
	public:
	static ContextPtr acquireContext(void); // Returns a pointer to the singleton USB context; throws exception if context can not be initialized
	void setDebugLevel(int newDebugLevel); // Sets the USB library's debugging level
	libusb_context* getContext(void) const // Returns a pointer to the low-level USB context handle
		{
		return context;
		}
	};

}

#endif
