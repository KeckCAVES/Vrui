/***********************************************************************
EventDispatcher - Class to dispatch events from a central listener to
any number of interested clients.
Copyright (c) 2016 Oliver Kreylos

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

#ifndef THREADS_EVENTDISPATCHER_INCLUDED
#define THREADS_EVENTDISPATCHER_INCLUDED

#include <sys/types.h>
#ifdef __APPLE__
#include <unistd.h>
#endif
#include <vector>
#include <Threads/Spinlock.h>

namespace Threads {

class EventDispatcher
	{
	/* Embedded classes: */
	public:
	typedef unsigned int ListenerKey; // Type for keys to uniquely identify registered event listeners
	
	enum IOEventType // Enumerated type for input/output event types
		{
		Read=0x01,Write=0x02,Exception=0x04
		};
	
	typedef bool (*IOEventCallback)(ListenerKey eventKey,int eventType,void* userData); // Type for input/output event callback functions; if callback returns true, event listener will immediately be removed from dispatcher
	
	private:
	struct IOEventListener // Structure representing listeners that have registered interest in some input/output event(s)
		{
		/* Elements: */
		public:
		ListenerKey key; // Unique key identifying this event
		int fd; // File descriptor belonging to the event
		int typeMask; // Mask of event types (read, write, exception) in which the listener is interested
		IOEventCallback callback; // Function called when an event occurs
		void* callbackUserData; // Opaque pointer to be passed to callback function
		
		/* Constructors and destructors: */
		IOEventListener(ListenerKey sKey,int sFd,int sTypeMask,IOEventCallback sCallback,void* sCallbackUserData)
			:key(sKey),fd(sFd),typeMask(sTypeMask),callback(sCallback),callbackUserData(sCallbackUserData)
			{
			}
		};
	
	/* Elements: */
	private:
	Spinlock pipeMutex; // Mutex protecting the self-pipe used to change the dispatcher's internal state
	int pipeFds[2]; // A uni-directional unnamed pipe to trigger events internal to the dispatcher
	ListenerKey nextKey; // Next key to be assigned to an event listener
	std::vector<IOEventListener> ioEventListeners; // List of currently registered input/output event listeners
	fd_set readFds,writeFds,exceptionFds; // Three sets of file descriptors waiting for reads, writes, and exceptions, respectively
	int numReadFds,numWriteFds,numExceptionFds; // Number of file descriptors in the three descriptor sets
	int maxFd; // Largest file descriptor set in any of the three descriptor sets
	bool hadBadFd; // Flag if the last invocation of dispatchNextEvent() tripped on a bad file descriptor
	
	/* Constructors and destructors: */
	public:
	EventDispatcher(void); // Creates an event dispatcher
	private:
	EventDispatcher(const EventDispatcher& source); // Prohibit copy constructor
	EventDispatcher& operator=(const EventDispatcher& source); // Prohibit assignment operator
	public:
	~EventDispatcher(void);
	
	/* Methods: */
	bool dispatchNextEvent(void); // Waits for the next event and dispatches it; returns false if the stop() method was called
	void dispatchEvents(void); // Waits for and dispatches events until stopped
	void interrupt(void); // Forces an invocation of dispatchNextEvent() to return with a true value
	void stop(void); // Forces an invocation of dispatchNextEvent() to return with a false value, or an invocation of dispatchEvents() to return
	ListenerKey addIOEventListener(int eventFd,int eventTypeMask,IOEventCallback eventCallback,void* eventCallbackUserData); // Adds a new input/output event listener for the given file descriptor and event type mask; returns unique event listener key
	void removeIOEventListener(ListenerKey listenerKey); // Removes the input/output event listener with the given listener key
	};

}

#endif
