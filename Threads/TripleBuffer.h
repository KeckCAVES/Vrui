/***********************************************************************
TripleBuffer - Class to allow one-way asynchronous non-blocking
communication between a producer and a consumer, in which the producer
writes a stream of data into a buffer, and the consumer can retrieve the
most recently written value at any time.
Copyright (c) 2005-2014 Oliver Kreylos

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

#ifndef THREADS_TRIPLEBUFFER_INCLUDED
#define THREADS_TRIPLEBUFFER_INCLUDED

#include <Misc/SizedTypes.h>
#include <Threads/Atomic.h>

namespace Threads {

template <class ValueParam>
class TripleBuffer
	{
	/* Embedded classes: */
	public:
	typedef ValueParam Value; // Type of communicated data
	
	/* Elements: */
	private:
	
	/* Bit masks and shift values to access the fields of the bufferState byte: */
	static const Misc::UInt8 writtenMask=0x80U; // Bit indicating the "written" flag
	static const Misc::UInt8 lockedShift=4U; // LSB position of buffer index currently locked for read access
	static const Misc::UInt8 lockedMask=0x30U; // Bits containing the locked buffer index
	static const Misc::UInt8 mostRecentShift=2U; // LSB position of buffer index most recently written to
	static const Misc::UInt8 mostRecentMask=0x0cU; // Bits containing the most recent buffer index
	static const Misc::UInt8 availableShift=0U; // LSB position of buffer index currently locked for write access
	static const Misc::UInt8 availableMask=0x03U; // Bits containing the available buffer index
	
	Value buffer[3]; // The triple-buffer of values
	Threads::Atomic<volatile Misc::UInt8> bufferState; // Bit field encoding written flag and locked, most recent, and available buffer indices
	
	/* Constructors and destructors: */
	public:
	TripleBuffer(void) // Creates empty triple buffer
		:bufferState((2U<<lockedShift)|(1U<<mostRecentShift)|(0U<<availableShift))
		{
		}
	private:
	TripleBuffer(const TripleBuffer& source); // Prohibit copy constructor
	TripleBuffer& operator=(const TripleBuffer& source); // Prohibit assignment operator
	public:
	~TripleBuffer(void) // Destroys the triple buffer
		{
		}
	
	/* Low-level methods: */
	Value& getBuffer(int bufferIndex) // Low-level method to access triple buffer contents
		{
		return buffer[bufferIndex];
		}
	
	/* Producer-side methods: */
	Value& startNewValue(void) // Prepares buffer to receive a new value
		{
		/* Return the buffer slot currently locked for writing from shared memory: */
		return buffer[(bufferState.get()&availableMask)>>availableShift];
		}
	void postNewValue(void) // Marks a new buffer value as most recent after data has been written
		{
		/* Read the current buffer state from shared memory: */
		Misc::UInt8 bs=bufferState.get();
		
		/* Try swapping the most recent and available buffer slots atomically until it succeeds (at most two attempts): */
		while(true)
			{
			/* Swap the most recent and available buffer indices and set the written flag to true: */
			Misc::UInt8 newBs=writtenMask|(bs&lockedMask)|((bs&mostRecentMask)>>2)|((bs&availableMask)<<2);
			
			/* Try writing the new buffer state to shared memory and bail out if it succeeded: */
			if((newBs=bufferState.compareAndSwap(bs,newBs))==bs)
				break;
			
			/* Try again: */
			bs=newBs;
			}
		}
	void postNewValue(const Value& newValue) // Pushes a new data value into the buffer
		{
		/* Read the current buffer state from shared memory: */
		Misc::UInt8 bs=bufferState.get();
		
		/* Write the new value: */
		buffer[(bs&availableMask)>>availableShift]=newValue;
		
		/* Try swapping the most recent and available buffer slots atomically until it succeeds (at most two attempts): */
		while(true)
			{
			/* Swap the most recent and available buffer indices and set the written flag to true: */
			Misc::UInt8 newBs=writtenMask|(bs&lockedMask)|((bs&mostRecentMask)>>2)|((bs&availableMask)<<2);
			
			/* Try writing the new buffer state to shared memory and bail out if it succeeded: */
			if((newBs=bufferState.compareAndSwap(bs,newBs))==bs)
				break;
			
			/* Try again: */
			bs=newBs;
			}
		}
	#if 0
	const Value& getMostRecentValue(void) const // Returns the last posted value; must not be called in cases where consumer might change locked value
		{
		/* Read the most recent buffer index from shared memory: */
		return buffer[(bufferState.get()&mostRecentMask)>>mostRecentShift];
		}
	#endif
	
	/* Consumer-side methods: */
	bool hasNewValue(void) const // Returns true if a new data value is available for the consumer
		{
		/* Return the value of the "written" flag from shared memory: */
		return (bufferState.get()&writtenMask)!=0x00U;
		}
	bool lockNewValue(void) // Locks the most recently written value; returns true if the value is new
		{
		/* Read the current buffer state from shared memory: */
		Misc::UInt8 bs=bufferState.get();
		
		/* Check the "written" flag and bail out if there is no new value: */
		if((bs&writtenMask)==0x00U)
			return false;
		
		/* Try swapping the most recent and locked buffer slots atomically until it succeeds (might never succeed if producer keeps writing): */
		while(true)
			{
			/* Swap the most recent and locked buffer indices and set the written flag to false: */
			Misc::UInt8 newBs=((bs&lockedMask)>>2)|((bs&mostRecentMask)<<2)|(bs&availableMask);
			
			/* Try writing the new buffer state to shared memory and bail out if it succeeded: */
			if((newBs=bufferState.compareAndSwap(bs,newBs))==bs)
				break;
			
			/* Try again: */
			bs=newBs;
			}
		
		return true;
		}
	const Value& getLockedValue(void) const // Returns the currently locked value
		{
		/* Read the locked buffer index from shared memory: */
		return buffer[(bufferState.get()&lockedMask)>>lockedShift];
		}
	Value& getLockedValue(void) // Ditto
		{
		/* Read the locked buffer index from shared memory: */
		return buffer[(bufferState.get()&lockedMask)>>lockedShift];
		}
	};

}

#endif
