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

#include <Threads/EventDispatcher.h>

#include <math.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdexcept>
#include <Misc/ThrowStdErr.h>

namespace Threads {

namespace {

/**************
Helper classes:
**************/

struct PipeMessage // Type for messages sent on an EventDispatcher's self-pipe
	{
	/* Embedded classes: */
	public:
	enum MessageType // Enumerated type for types of messages
		{
		INTERRUPT=0,
		STOP,
		ADD_IO_LISTENER,
		REMOVE_IO_LISTENER,
		ADD_TIMER_LISTENER,
		REMOVE_TIMER_LISTENER,
		ADD_PROCESS_LISTENER,
		REMOVE_PROCESS_LISTENER,
		NUM_MESSAGETYPES
		};
	
	/* Elements: */
	int messageType; // Type of message
	union
		{
		struct
			{
			EventDispatcher::ListenerKey key;
			int fd;
			int typeMask;
			EventDispatcher::IOEventCallback callback;
			void* callbackUserData;
			} addIOListener; // Message to add a new input/output event listener to the event dispatcher
		EventDispatcher::ListenerKey removeIOListener; // Message to remove an input/output event listener from the event dispatcher
		struct
			{
			EventDispatcher::ListenerKey key;
			struct timeval time;
			struct timeval interval;
			EventDispatcher::TimerEventCallback callback;
			void* callbackUserData;
			} addTimerListener; // Message to add a new timer event listener to the event dispatcher
		EventDispatcher::ListenerKey removeTimerListener; // Message to remove a timer event listener from the event dispatcher
		struct
			{
			EventDispatcher::ListenerKey key;
			EventDispatcher::ProcessCallback callback;
			void* callbackUserData;
			} addProcessListener; // Message to add a new process listener to the event dispatcher
		EventDispatcher::ListenerKey removeProcessListener; // Message to remove a process listener from the event dispatcher
		};
	};

/****************
Helper functions:
****************/

bool readPipeMessage(int pipeFd,PipeMessage& pm)
	{
	/* Make sure to read the complete message: */
	unsigned char* readPtr=reinterpret_cast<unsigned char*>(&pm);
	size_t readSize=sizeof(PipeMessage);
	while(readSize>0U)
		{
		ssize_t readResult=read(pipeFd,readPtr,readSize);
		if(readResult>0)
			{
			readPtr+=readResult;
			readSize-=size_t(readResult);
			}
		else if(errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)
			return false;
		}
	
	return true;
	}

bool writePipeMessage(const PipeMessage& pm,int pipeFd)
	{
	/* Make sure to write the complete message: */
	const unsigned char* writePtr=reinterpret_cast<const unsigned char*>(&pm);
	size_t writeSize=sizeof(PipeMessage);
	while(writeSize>0U)
		{
		ssize_t writeResult=write(pipeFd,writePtr,writeSize);
		if(writeResult>0)
			{
			writePtr+=writeResult;
			writeSize-=size_t(writeResult);
			}
		else if(errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)
			return false;
		}
	
	return true;
	}

}

/**************************************
Methods of class EventDispatcher::Time:
**************************************/

EventDispatcher::Time::Time(double sSeconds)
	{
	/* Take integer and fractional parts of given time, ensuring that tv_usec>=0: */
	tv_sec=long(floor(sSeconds));
	tv_usec=long(floor((sSeconds-double(tv_sec))*1.0e6+0.5));
	
	/* Check for rounding: */
	if(tv_usec>=1000000)
		{
		++tv_sec;
		tv_usec=0;
		}
	}

EventDispatcher::Time EventDispatcher::Time::now(void)
	{
	Time result;
	gettimeofday(&result,0);
	return result;
	}

/********************************
Methods of class EventDispatcher:
********************************/

EventDispatcher::EventDispatcher(void)
	:nextKey(0),
	 numReadFds(0),numWriteFds(0),numExceptionFds(0),
	 hadBadFd(false)
	{
	/* Create the self-pipe: */
	if(pipe(pipeFds)!=0||pipeFds[0]<0||pipeFds[1]<0)
		throw std::runtime_error("Misc::EventDispatcher: Cannot open event pipe");
	
	/* Initialize the three file descriptor sets: */
	FD_ZERO(&readFds);
	FD_ZERO(&writeFds);
	FD_ZERO(&exceptionFds);
	
	/* Add the read end of the self-pipe to the read descriptor set: */
	FD_SET(pipeFds[0],&readFds);
	numReadFds=1;
	maxFd=pipeFds[0];
	}

EventDispatcher::~EventDispatcher(void)
	{
	/* Close the self-pipe: */
	close(pipeFds[0]);
	close(pipeFds[1]);
	
	/* Delete all timer event listeners: */
	for(TimerEventListenerHeap::Iterator telIt=timerEventListeners.begin();telIt!=timerEventListeners.end();++telIt)
		delete *telIt;
	}

bool EventDispatcher::dispatchNextEvent(void)
	{
	/* Create lists of watched file descriptors: */
	fd_set rds,wds,eds;
	int numRfds,numWfds,numEfds,numFds;
	if(hadBadFd)
		{
		/* Listen only on the self-pipe to recover from EBADF errors: */
		FD_ZERO(&rds);
		FD_SET(pipeFds[0],&rds);
		numRfds=1;
		numWfds=0;
		numEfds=0;
		numFds=pipeFds[0]+1;
		
		hadBadFd=false;
		}
	else
		{
		/* Copy all used file descriptor sets: */
		if(numReadFds>0)
			rds=readFds;
		if(numWriteFds>0)
			wds=writeFds;
		if(numExceptionFds>0)
			eds=exceptionFds;
		numRfds=numReadFds;
		numWfds=numWriteFds;
		numEfds=numExceptionFds;
		numFds=maxFd+1;
		}
	
	/* Process the heap of timer event listeners: */
	int numSetFds=0;
	while(!timerEventListeners.isEmpty())
		{
		/* Calculate the interval to the next timer event and dispatch elapsed events on-the-fly: */
		TimerEventListener* tel=timerEventListeners.getSmallest();
		Time interval=tel->time;
		interval-=Time::now();
		
		/* Check if the event has already elapsed: */
		if(interval.tv_sec<0)
			{
			/* Call the event callback: */
			if(tel->callback(tel->key,tel->callbackUserData))
				{
				/* Remove the event listener from the heap: */
				delete tel;
				timerEventListeners.removeSmallest();
				}
			else
				{
				/* Move the event time to the next iteration: */
				tel->time+=tel->interval;
				timerEventListeners.reinsertSmallest();
				}
			}
		else
			{
			/* Wait for the next event on any watched file descriptor or until the next timer event elapses: */
			numSetFds=select(numFds,numRfds>0?&rds:0,numWfds>0?&wds:0,numEfds>0?&eds:0,&interval);
			
			/* Done dispatching timer events: */
			goto doneWithTimerEvents;
			}
		}
	
	/* Wait for the next event on any watched file descriptor (we only get here if there are no active timer events): */
	numSetFds=select(numFds,numRfds>0?&rds:0,numWfds>0?&wds:0,numEfds>0?&eds:0,0);
	
	doneWithTimerEvents:
	
	/* Handle all received events: */
	if(numSetFds>0)
		{
		/* Check for a message on the self-pipe: */
		if(FD_ISSET(pipeFds[0],&rds))
			{
			/* Read the pipe message: */
			PipeMessage pm;
			if(!readPipeMessage(pipeFds[0],pm))
				{
				int error=errno;
				Misc::throwStdErr("Misc::EventDispatcher::dispatchNextEvent: Fatal error %d (%s) while reading command",error,strerror(error));
				}
			switch(pm.messageType)
				{
				case PipeMessage::STOP: // Stop dispatching events
					return false;
					break;
				
				case PipeMessage::ADD_IO_LISTENER: // Add input/output event listener
					{
					/* Add the new input/output event listener to the list: */
					ioEventListeners.push_back(IOEventListener(pm.addIOListener.key,pm.addIOListener.fd,pm.addIOListener.typeMask,pm.addIOListener.callback,pm.addIOListener.callbackUserData));
					
					/* Add the new input/output event listener's file descriptor to all selected descriptor sets: */
					if(pm.addIOListener.typeMask&Read)
						{
						FD_SET(pm.addIOListener.fd,&readFds);
						++numReadFds;
						}
					if(pm.addIOListener.typeMask&Write)
						{
						FD_SET(pm.addIOListener.fd,&writeFds);
						++numWriteFds;
						}
					if(pm.addIOListener.typeMask&Exception)
						{
						FD_SET(pm.addIOListener.fd,&exceptionFds);
						++numExceptionFds;
						}
					if(maxFd<pm.addIOListener.fd)
						maxFd=pm.addIOListener.fd;
					
					break;
					}
				
				case PipeMessage::REMOVE_IO_LISTENER: // Remove input/output event listener
					{
					/* Find the input/output event listener with the given key: */
					std::vector<IOEventListener>::iterator elIt;
					for(elIt=ioEventListeners.begin();elIt!=ioEventListeners.end()&&elIt->key!=pm.removeIOListener;++elIt)
						;
					if(elIt!=ioEventListeners.end())
						{
						/* Remove the input/output event listener from the list: */
						int eventFd=elIt->fd;
						int eventTypeMask=elIt->typeMask;
						*elIt=*(ioEventListeners.end()-1);
						ioEventListeners.pop_back();
						
						/* Remove the input/output event file descriptor from all selected descriptor sets: */
						if(eventTypeMask&Read)
							{
							FD_CLR(eventFd,&readFds);
							--numReadFds;
							}
						if(eventTypeMask&Write)
							{
							FD_CLR(eventFd,&writeFds);
							--numWriteFds;
							}
						if(eventTypeMask&Exception)
							{
							FD_CLR(eventFd,&exceptionFds);
							--numExceptionFds;
							}
						if(maxFd==eventFd)
							{
							/* Find the new largest file descriptor: */
							maxFd=pipeFds[0];
							for(elIt=ioEventListeners.begin();elIt!=ioEventListeners.end();++elIt)
								if(maxFd<elIt->fd)
									maxFd=elIt->fd;
							}
						}
					break;
					}
				
				case PipeMessage::ADD_TIMER_LISTENER: // Add timer event listener
					/* Add the new timer event listener to the heap: */
					timerEventListeners.insert(new TimerEventListener(pm.addTimerListener.key,pm.addTimerListener.time,pm.addTimerListener.interval,pm.addTimerListener.callback,pm.addTimerListener.callbackUserData));
					
					break;
				
				case PipeMessage::REMOVE_TIMER_LISTENER: // Remove timer event listener
					{
					/* Find the timer event listener with the given key: */
					TimerEventListenerHeap::Iterator elIt;
					for(elIt=timerEventListeners.begin();elIt!=timerEventListeners.end()&&(*elIt)->key!=pm.removeTimerListener;++elIt)
						;
					if(elIt!=timerEventListeners.end())
						{
						/* Remove the timer event listener from the heap: */
						delete *elIt;
						timerEventListeners.remove(elIt);
						}
					
					break;
					}
				
				case PipeMessage::ADD_PROCESS_LISTENER:
					/* Add the new process listener to the list: */
					processListeners.push_back(ProcessListener(pm.addProcessListener.key,pm.addProcessListener.callback,pm.addProcessListener.callbackUserData));
					
					break;
				
				case PipeMessage::REMOVE_PROCESS_LISTENER:
					{
					/* Find the process listener with the given key: */
					std::vector<ProcessListener>::iterator plIt;
					for(plIt=processListeners.begin();plIt!=processListeners.end()&&plIt->key!=pm.removeProcessListener;++plIt)
						;
					if(plIt!=processListeners.end())
						{
						/* Remove the process listener from the list: */
						*plIt=*(processListeners.end()-1);
						processListeners.pop_back();
						}
					
					break;
					}
				
				default:
					/* Do nothing: */
					;
				}
			
			--numSetFds;
			}
		
		/* Handle all input/output events: */
		for(std::vector<IOEventListener>::iterator elIt=ioEventListeners.begin();numSetFds>0&&elIt!=ioEventListeners.end();++elIt)
			{
			/* Check all event types for which the listener has registered interest: */
			bool removeListener=false;
			if((elIt->typeMask&Read)!=0&&FD_ISSET(elIt->fd,&rds))
				{
				/* Call the event callback: */
				removeListener=elIt->callback(elIt->key,Read,elIt->callbackUserData);
				--numSetFds;
				}
			if(!removeListener&&(elIt->typeMask&Write)!=0&&FD_ISSET(elIt->fd,&wds))
				{
				/* Call the event callback: */
				removeListener=elIt->callback(elIt->key,Write,elIt->callbackUserData);
				--numSetFds;
				}
			if(!removeListener&&(elIt->typeMask&Exception)!=0&&FD_ISSET(elIt->fd,&eds))
				{
				/* Call the event callback: */
				removeListener=elIt->callback(elIt->key,Exception,elIt->callbackUserData);
				--numSetFds;
				}
			
			/* Check if the listener wants to be removed: */
			if(removeListener)
				{
				/* Remove the event listener from the list: */
				int eventFd=elIt->fd;
				int eventTypeMask=elIt->typeMask;
				*elIt=ioEventListeners.back();
				ioEventListeners.pop_back();
				--elIt;
				
				/* Remove the event file descriptor from all selected descriptor sets: */
				if(eventTypeMask&Read)
					{
					FD_CLR(eventFd,&readFds);
					--numReadFds;
					}
				if(eventTypeMask&Write)
					{
					FD_CLR(eventFd,&writeFds);
					--numWriteFds;
					}
				if(eventTypeMask&Exception)
					{
					FD_CLR(eventFd,&exceptionFds);
					--numExceptionFds;
					}
				if(maxFd==eventFd)
					{
					/* Find the new largest file descriptor: */
					maxFd=pipeFds[0];
					for(std::vector<IOEventListener>::iterator it=ioEventListeners.begin();it!=ioEventListeners.end();++it)
						if(maxFd<it->fd)
							maxFd=it->fd;
					}
				}
			}
		}
	else if(numSetFds<0&&errno!=EINTR)
		{
		if(errno==EBADF)
			{
			/* Set error flag; only wait on self-pipe on next iteration to hopefully receive a "remove listener" message for the bad descriptor: */
			hadBadFd=true;
			}
		else
			{
			int error=errno;
			Misc::throwStdErr("Misc::EventDispatcher::dispatchNextEvent: Error %d (%s) during select",error,strerror(error));
			}
		}
	
	/* Call all process listeners: */
	for(std::vector<ProcessListener>::iterator plIt=processListeners.begin();plIt!=processListeners.end();++plIt)
		{
		/* Call the listener and check if it wants to be removed: */
		if(plIt->callback(plIt->key,plIt->callbackUserData))
			{
			/* Remove the event listener from the list: */
			*plIt=processListeners.back();
			processListeners.pop_back();
			--plIt;
			}
		}
	
	return true;
	}

void EventDispatcher::dispatchEvents(void)
	{
	/* Dispatch events until the stop() method is called: */
	while(dispatchNextEvent())
		;
	}

void EventDispatcher::interrupt(void)
	{
	/* Lock the self-pipe: */
	Threads::Spinlock::Lock pipeLock(pipeMutex);
	
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::INTERRUPT;
	if(!writePipeMessage(pm,pipeFds[1]))
		{
		int error=errno;
		Misc::throwStdErr("EventDispatcher::interrupt: Fatal error %d (%s) while writing command",error,strerror(error));
		}
	}

void EventDispatcher::stop(void)
	{
	/* Lock the self-pipe: */
	Threads::Spinlock::Lock pipeLock(pipeMutex);
	
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::STOP;
	if(!writePipeMessage(pm,pipeFds[1]))
		{
		int error=errno;
		Misc::throwStdErr("EventDispatcher::stop: Fatal error %d (%s) while writing command",error,strerror(error));
		}
	}

EventDispatcher::ListenerKey EventDispatcher::addIOEventListener(int eventFd,int eventTypeMask,EventDispatcher::IOEventCallback eventCallback,void* eventCallbackUserData)
	{
	/* Lock the self-pipe: */
	Threads::Spinlock::Lock pipeLock(pipeMutex);
	
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::ADD_IO_LISTENER;
	pm.addIOListener.key=nextKey;
	pm.addIOListener.fd=eventFd;
	pm.addIOListener.typeMask=eventTypeMask;
	pm.addIOListener.callback=eventCallback;
	pm.addIOListener.callbackUserData=eventCallbackUserData;
	if(!writePipeMessage(pm,pipeFds[1]))
		{
		int error=errno;
		Misc::throwStdErr("EventDispatcher::addIOEventListener: Fatal error %d (%s) while writing command",error,strerror(error));
		}
	
	/* Increment the next listener key: */
	++nextKey;
	
	return pm.addIOListener.key;
	}

void EventDispatcher::removeIOEventListener(EventDispatcher::ListenerKey listenerKey)
	{
	/* Lock the self-pipe: */
	Threads::Spinlock::Lock pipeLock(pipeMutex);
	
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::REMOVE_IO_LISTENER;
	pm.removeIOListener=listenerKey;
	if(!writePipeMessage(pm,pipeFds[1]))
		{
		int error=errno;
		Misc::throwStdErr("EventDispatcher::removeIOEventListener: Fatal error %d (%s) while writing command",error,strerror(error));
		}
	}

EventDispatcher::ListenerKey EventDispatcher::addTimerEventListener(const EventDispatcher::Time& eventTime,const EventDispatcher::Time& eventInterval,EventDispatcher::TimerEventCallback eventCallback,void* eventCallbackUserData)
	{
	/* Lock the self-pipe: */
	Threads::Spinlock::Lock pipeLock(pipeMutex);
	
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::ADD_TIMER_LISTENER;
	pm.addTimerListener.key=nextKey;
	pm.addTimerListener.time=eventTime;
	pm.addTimerListener.interval=eventInterval;
	pm.addTimerListener.callback=eventCallback;
	pm.addTimerListener.callbackUserData=eventCallbackUserData;
	if(!writePipeMessage(pm,pipeFds[1]))
		{
		int error=errno;
		Misc::throwStdErr("EventDispatcher::addTimerListener: Fatal error %d (%s) while writing command",error,strerror(error));
		}
	
	/* Increment the next listener key: */
	++nextKey;
	
	return pm.addTimerListener.key;
	}

void EventDispatcher::removeTimerEventListener(EventDispatcher::ListenerKey listenerKey)
	{
	/* Lock the self-pipe: */
	Threads::Spinlock::Lock pipeLock(pipeMutex);
	
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::REMOVE_TIMER_LISTENER;
	pm.removeTimerListener=listenerKey;
	if(!writePipeMessage(pm,pipeFds[1]))
		{
		int error=errno;
		Misc::throwStdErr("EventDispatcher::removeTimerEventListener: Fatal error %d (%s) while writing command",error,strerror(error));
		}
	}

EventDispatcher::ListenerKey EventDispatcher::addProcessListener(EventDispatcher::ProcessCallback eventCallback,void* eventCallbackUserData)
	{
	/* Lock the self-pipe: */
	Threads::Spinlock::Lock pipeLock(pipeMutex);
	
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::ADD_PROCESS_LISTENER;
	pm.addProcessListener.key=nextKey;
	pm.addProcessListener.callback=eventCallback;
	pm.addProcessListener.callbackUserData=eventCallbackUserData;
	if(!writePipeMessage(pm,pipeFds[1]))
		{
		int error=errno;
		Misc::throwStdErr("EventDispatcher::addProcessListener: Fatal error %d (%s) while writing command",error,strerror(error));
		}
	
	/* Increment the next listener key: */
	++nextKey;
	
	return pm.addProcessListener.key;
	}

void EventDispatcher::removeProcessListener(EventDispatcher::ListenerKey listenerKey)
	{
	/* Lock the self-pipe: */
	Threads::Spinlock::Lock pipeLock(pipeMutex);
	
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::REMOVE_PROCESS_LISTENER;
	pm.removeProcessListener=listenerKey;
	if(!writePipeMessage(pm,pipeFds[1]))
		{
		int error=errno;
		Misc::throwStdErr("EventDispatcher::removeProcessListener: Fatal error %d (%s) while writing command",error,strerror(error));
		}
	}

}
