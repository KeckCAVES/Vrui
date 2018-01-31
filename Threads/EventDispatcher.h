/***********************************************************************
EventDispatcher - Class to dispatch events from a central listener to
any number of interested clients.
Copyright (c) 2016-2018 Oliver Kreylos

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
#include <sys/time.h>
#ifdef __APPLE__
#include <unistd.h>
#endif
#include <vector>
#include <Misc/PriorityHeap.h>
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
	
	class Time:public timeval // Class to specify time points or time intervals for timer events; microseconds are assumed in [0, 1000000) even if interval is negative
		{
		/* Constructors and destructors: */
		public:
		Time(void) // Dummy constructor
			{
			}
		Time(long sSec,long sUsec) // Initializes time interval
			{
			tv_sec=sSec;
			tv_usec=sUsec;
			}
		Time(const struct timeval& tv) // Constructor from base class
			{
			tv_sec=tv.tv_sec;
			tv_usec=tv.tv_usec;
			}
		Time(double sSeconds); // Initializes time interval from non-negative number of seconds
		
		/* Methods: */
		static Time now(void); // Returns the current wall-clock time as a time point
		bool operator==(const Time& other) const // Compares two time points or time intervals
			{
			return tv_sec==other.tv_sec&&tv_usec==other.tv_usec;
			}
		bool operator!=(const Time& other) const // Compares two time points or time intervals
			{
			return tv_sec!=other.tv_sec||tv_usec!=other.tv_usec;
			}
		bool operator<=(const Time& other) const // Compares two time points or time intervals
			{
			return tv_sec<other.tv_sec||(tv_sec==other.tv_sec&&tv_usec<=other.tv_usec);
			}
		Time& operator+=(const Time& other) // Adds a time interval to a time point or another time interval
			{
			/* Add time components: */
			tv_sec+=other.tv_sec;
			tv_usec+=other.tv_usec;
			
			/* Handle overflow: */
			if(tv_usec>=1000000)
				{
				++tv_sec;
				tv_usec-=1000000;
				}
			return *this;
			}
		Time& operator-=(const Time& other) // Subtracts a time interval from a time point, or two time intervals, or two time points
			{
			/* Subtract time components: */
			tv_sec-=other.tv_sec;
			tv_usec-=other.tv_usec;
			
			/* Handle underflow: */
			if(tv_usec<0)
				{
				--tv_sec;
				tv_usec+=1000000;
				}
			return *this;
			}
		};
	
	typedef bool (*IOEventCallback)(ListenerKey eventKey,int eventType,void* userData); // Type for input/output event callback functions; if callback returns true, event listener will immediately be removed from dispatcher
	typedef bool (*TimerEventCallback)(ListenerKey eventKey,void* userData); // Type for timer event callback functions; if callback returns true, event listener will immediately be removed from dispatcher
	typedef bool (*ProcessCallback)(ListenerKey processKey,void* userData); // Type for process callback functions; if callback returns true, event listener will immediately be removed from dispatcher
	
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
	
	struct TimerEventListener // Structure representing listeners that have registered interest in timer events
		{
		/* Elements: */
		public:
		ListenerKey key; // Unique key identifying this event
		Time time; // Time point at which to trigger the event
		Time interval; // Event interval for recurring events; one-shot events need to return true from callback to cancel further events
		TimerEventCallback callback; // Function called when an event occurs
		void* callbackUserData; // Opaque pointer to be passed to callback function
		
		/* Constructors and destructors: */
		TimerEventListener(ListenerKey sKey,const Time& sTime,const Time& sInterval,TimerEventCallback sCallback,void* sCallbackUserData)
			:key(sKey),time(sTime),interval(sInterval),callback(sCallback),callbackUserData(sCallbackUserData)
			{
			}
		};
	
	class TimerEventListenerComp // Helper class to compare timer event listener structures by next event time
		{
		/* Methods: */
		public:
		static bool lessEqual(const TimerEventListener* v1,const TimerEventListener* v2)
			{
			return v1->time<=v2->time;
			}
		};
	
	typedef Misc::PriorityHeap<TimerEventListener*,TimerEventListenerComp> TimerEventListenerHeap;
	
	struct ProcessListener // Structure representing listeners that are called after any event has been handled
		{
		/* Elements: */
		public:
		ListenerKey key; // Unique key identifying this event
		ProcessCallback callback; // Function called when an event occurs
		void* callbackUserData; // Opaque pointer to be passed to callback function
		
		/* Constructors and destructors: */
		ProcessListener(ListenerKey sKey,ProcessCallback sCallback,void* sCallbackUserData)
			:key(sKey),callback(sCallback),callbackUserData(sCallbackUserData)
			{
			}
		};
	
	/* Elements: */
	private:
	Spinlock pipeMutex; // Mutex protecting the self-pipe used to change the dispatcher's internal state
	int pipeFds[2]; // A uni-directional unnamed pipe to trigger events internal to the dispatcher
	ListenerKey nextKey; // Next key to be assigned to an event listener
	std::vector<IOEventListener> ioEventListeners; // List of currently registered input/output event listeners
	TimerEventListenerHeap timerEventListeners; // Heap of currently registered timer event listeners, sorted by next event time
	std::vector<ProcessListener> processListeners; // List of currently registered process event listeners
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
	ListenerKey addTimerEventListener(const Time& eventTime,const Time& eventInterval,TimerEventCallback eventCallback,void* eventCallbackUserData); // Adds a new timer event listener for the given event time and repeat interval; returns unique event listener key
	void removeTimerEventListener(ListenerKey listenerKey); // Removes the timer event listener with the given listener key
	ListenerKey addProcessListener(ProcessCallback eventCallback,void* eventCallbackUserData); // Adds a new process listener; returns unique event listener key
	void removeProcessListener(ListenerKey listenerKey); // Removes the process listener with the given listener key
	};

}

#endif
