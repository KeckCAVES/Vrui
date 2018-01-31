/***********************************************************************
Context - Class representing libusb library contexts.
Copyright (c) 2010-2017 Oliver Kreylos

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

#include <USB/Context.h>

#include <libusb-1.0/libusb.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/Autopointer.h>

// DEBUGGING
#include <iostream>

namespace USB {

/********************************
Static elements of class Context:
********************************/

Context Context::theContext;

/************************
Methods of class Context:
************************/

namespace {

volatile bool goon;

}

void* Context::eventHandlingThreadMethod(void)
	{
	// Threads::Thread::setCancelState(Threads::Thread::CANCEL_ENABLE);
	// Threads::Thread::setCancelType(Threads::Thread::CANCEL_ASYNCHRONOUS);
	
	while(goon)
		{
		/* Block until the next USB event and handle it or a short time-out: */
		struct timeval maxWait;
		maxWait.tv_sec=0;
		maxWait.tv_usec=500000;
		libusb_handle_events_timeout(context,&maxWait);
		}
	
	return 0;
	}

void Context::ref(void)
	{
	/* Lock the reference mutex: */
	Threads::Mutex::Lock refLock(refMutex);
	
	/* Increment the reference counter and check if it was zero before: */
	if((refCount++)==0)
		{
		/* Initialize the USB context: */
		if(libusb_init(&context)!=0)
			Misc::throwStdErr("USB::Context: Error initializing USB context");
		
		/* Start the event handling thread: */
		goon=true;
		eventHandlingThread.start(this,&Context::eventHandlingThreadMethod);
		}
	}

void Context::unref(void)
	{
	/* Lock the reference mutex: */
	Threads::Mutex::Lock refLock(refMutex);
	
	/* Decrement the reference counter and check if it reached zero: */
	if((--refCount)==0)
		{
		/* Stop the background event processing thread: */
		goon=false;
		// eventHandlingThread.cancel();
		eventHandlingThread.join();
		
		/* Shut down the USB context: */
		libusb_exit(context);
		context=0;
		}
	}

Context::Context(void)
	:refCount(0),
	 context(0)
	{
	}

ContextPtr Context::acquireContext(void)
	{
	/* Return an autopointer to the singleton USB context object; autopointer's call to ref() will set up context on first call: */
	return ContextPtr(&theContext);
	}

void Context::setDebugLevel(int newDebugLevel)
	{
	libusb_set_debug(context,newDebugLevel);
	}

}
