/***********************************************************************
Queue - Simple unlimited-size queue to send data from one or more
producers to one or more consumers.
Copyright (c) 2012 Oliver Kreylos

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

#ifndef THREADS_QUEUE_INCLUDED
#define THREADS_QUEUE_INCLUDED

#include <stddef.h>
#include <new>
#include <Threads/Mutex.h>
#include <Threads/Cond.h>

namespace Threads {

template <class ValueParam, size_t chunkSizeParam =8192>
class Queue
	{
	/* Embedded classes: */
	public:
	typedef ValueParam Value; // Type of communicated data
	static const size_t chunkSize=chunkSizeParam; // Size of a queue chunk in bytes
	
	private:
	struct ChunkHeader // Structure for queue chunk headers
		{
		/* Elements: */
		public:
		ChunkHeader* succ; // Pointer to next queue chunk
		};
	
	static const size_t numChunkElements=(chunkSize-sizeof(ChunkHeader))/sizeof(Value); // Number of queue elements per queue chunk
	
	/* Elements: */
	Mutex queueMutex; // Mutex to serialize access to the queue and condition variable
	Cond queueCond; // Condition variable to signal queue becoming non-empty or non-full
	ChunkHeader* headChunk; // Pointer to queue chunk containing first queue element
	size_t headIndex; // Index of first queue element in head chunk
	ChunkHeader* tailChunk; // Pointer to queue chunk containing last queue element
	size_t tailIndex; // Index of last queue element in head chunk
	
	/* Private methods: */
	ChunkHeader* allocChunk(void) // Allocates a new queue chunk
		{
		/* Allocate raw memory of exact requested chunk size: */
		ChunkHeader* newChunk=reinterpret_cast<ChunkHeader*>(new unsigned char[chunkSize]);
		
		/* Initialize the chunk: */
		newChunk->succ=0;
		Value* vPtr=reinterpret_cast<Value*>(newChunk+1);
		for(size_t i=0;i<numChunkElements;++i,++vPtr)
			new(vPtr) Value;
		
		return newChunk;
		}
	void freeChunk(ChunkHeader* chunk) // Deletes the given chunk and the elements in it
		{
		/* Destroy all elements: */
		Value* vPtr=reinterpret_cast<Value*>(chunk+1);
		for(size_t i=0;i<numChunkElements;++i,++vPtr)
			vPtr->~Value();
		
		/* Delete the chunk's raw memory: */
		delete[] reinterpret_cast<unsigned char*>(chunk);
		}
	
	/* Constructors and destructors: */
	public:
	Queue(void) // Creates an empty queue
		:headChunk(allocChunk()),headIndex(0),
		 tailChunk(headChunk),tailIndex(0)
		{
		}
	private:
	Queue(const Queue& source); // Prohibit copy constructor
	Queue& operator=(const Queue& source); // Prohibit assignment operator
	public:
	~Queue(void) // Destroys the queue and all elements
		{
		while(headChunk!=0)
			{
			ChunkHeader* succ=headChunk->succ;
			freeChunk(headChunk);
			headChunk=succ;
			}
		}
	
	/* Methods: */
	void push(const Value& value) // Pushes the given value into the queue
		{
		Mutex::Lock queueLock(queueMutex);
		
		/* Check if the tail chunk is full: */
		if(tailIndex==numChunkElements)
			{
			/* Add a new tail chunk: */
			ChunkHeader* newTailChunk=allocChunk();
			tailChunk->succ=newTailChunk;
			tailChunk=newTailChunk;
			tailIndex=0;
			}
		
		/* Store the value: */
		reinterpret_cast<Value*>(tailChunk+1)[tailIndex]=value;
		
		/* Wake up a consumer if the queue has just become non-empty: */
		if(tailChunk==headChunk&&tailIndex==headIndex)
			queueCond.broadcast();
		
		/* Advance the tail position: */
		++tailIndex;
		}
	Value pop(void) // Returns and removes the first value from the queue; blocks if queue is empty
		{
		Mutex::Lock queueLock(queueMutex);
		
		/* Block while the queue is empty: */
		while(tailChunk==headChunk&&tailIndex==headIndex)
			queueCond.wait(queueMutex);
		
		/* Return the first element from the queue: */
		Value result=reinterpret_cast<Value*>(headChunk+1)[headIndex];
		
		/* Advance the head position: */
		if(++headIndex==numChunkElements)
			{
			/* Check if this is the only chunk in the queue: */
			if(tailChunk==headChunk)
				{
				/* Recycle the chunk to save one dealloc/alloc pair: */
				headIndex=0;
				tailIndex=0;
				}
			else
				{
				/* Delete the head chunk: */
				ChunkHeader* nextHeadChunk=headChunk->succ;
				freeChunk(headChunk);
				headChunk=nextHeadChunk;
				headIndex=0;
				}
			}
		
		return result;
		}
	};

}

#endif
