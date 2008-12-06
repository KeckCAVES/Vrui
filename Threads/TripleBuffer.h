/***********************************************************************
TripleBuffer - Class to allow one-way asynchronous non-blocking
communication between a producer and a consumer, in which the producer
writes a stream of data into a buffer, and the consumer can retrieve the
most recently written value at any time.
Copyright (c) 2005 Oliver Kreylos
***********************************************************************/

#ifndef THREADS_TRIPLEBUFFER_INCLUDED
#define THREADS_TRIPLEBUFFER_INCLUDED

namespace Threads {

template <class ValueParam>
class TripleBuffer
	{
	/* Embedded classes: */
	public:
	typedef ValueParam value; // Type of communicated data
	
	/* Elements: */
	private:
	volatile Value buffer[3]; // The triple-buffer of values
	volatile int lockedIndex; // Buffer index currently locked by the consumer
	volatile int mostRecentIndex; // Buffer index of most recently produced value
	
	/* Constructors and destructors: */
	public:
	TripleBuffer(void) // Creates empty triple buffer
		:lockedIndex(0),mostRecentIndex(0)
		{
		}
	private:
	TripleBuffer(const TripleBuffer& source); // Prohibit copy constructor
	TripleBuffer& operator=(const TripleBuffer& source); // Prohibit assignment operator
	public:
	~TripleBuffer(void) // Destroys the triple buffer
		{
		}
	
	/* Methods: */
	void setNewValue(const Value& newValue) // Pushes a new data value into the buffer
		{
		int nextIndex=(lockedIndex+1)%3;
		if(nextIndex==mostRecentIndex)
			nextIndex=(nextIndex+1)%3;
		buffer[nextIndex]=newValue;
		mostRecentIndex=nextIndex;
		}
	bool hasNewValue(void) const // Returns true if a new data value is available for the consumer
		{
		return mostRecentIndex!=lockedIndex;
		}
	Value& lockNewValue(void) // Returns a reference to the most recently pushed data value
		{
		lockedIndex=mostRecentIndex;
		return buffer[lockedIndex];
		}
	const Value& getLockedValue(void) const // Returns the currently locked value
		{
		return buffer[lockedIndex];
		}
	Value& getLockedValue(void) // Ditto
		{
		return buffer[lockedIndex];
		}
	}

}

#endif
