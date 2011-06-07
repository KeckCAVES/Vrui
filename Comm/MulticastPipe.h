/***********************************************************************
MulticastPipe - Class to represent data streams between a single master
and several slaves, with the bulk of communication from the master to
all the slaves in parallel.
Copyright (c) 2005-2011 Oliver Kreylos

This file is part of the Portable Communications Library (Comm).

The Portable Communications Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Portable Communications Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Portable Communications Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef COMM_MULTICASTPIPE_INCLUDED
#define COMM_MULTICASTPIPE_INCLUDED

#include <stddef.h>
#include <Comm/GatherOperation.h>

/* Forward declarations: */
namespace Comm {
struct MulticastPacket;
class MulticastPipeMultiplexer;
}

namespace Comm {

class MulticastPipe
	{
	/* Elements: */
	private:
	MulticastPipeMultiplexer* multiplexer; // Pointer to the multiplexer used to coordinate between multiple pipes
	bool master; // Flag if this instance of the multicast pipe is on the master node
	unsigned int pipeId; // Unique identifier of this pipe for the same multiplexer
	MulticastPacket* packet; // Pointer to current packet
	size_t packetPos; // Data position in current packet
	
	/* Constructors and destructors: */
	public:
	MulticastPipe(MulticastPipeMultiplexer* sMultiplexer); // Creates new pipe for the given multiplexer
	private:
	MulticastPipe(const MulticastPipe& source); // Prohibit copy constructor
	MulticastPipe& operator=(const MulticastPipe& source); // Prohibit assignment operato
	public:
	~MulticastPipe(void); // Closes the pipe
	
	/* Methods: */
	MulticastPipeMultiplexer* getMultiplexer(void) const // Returns pointer to the multiplexer managing this pipe
		{
		return multiplexer;
		}
	bool isMaster(void) const // Returns true if this instance of the pipe is on the master node
		{
		return master;
		}
	
	/* Synchronization interface: */
	void barrier(void); // Blocks the calling thread until all nodes in a multicast pipe have reached the same point in the program
	unsigned int gather(unsigned int value,GatherOperation::OpCode op); // Blocks the calling thread until all nodes in a multicast pipe have exchanged a value; returns final accumulated value
	
	/* Common interface: */
	void broadcastRaw(void* data,size_t size); // Sends data from master to all slaves; does not change data on master
	template <class DataParam>
	void broadcast(DataParam& data) // Sends single value of arbitrary type from master to all slaves; does not change value on master
		{
		broadcastRaw(&data,sizeof(DataParam));
		}
	template <class DataParam>
	void broadcast(DataParam* data,size_t numItems) // Sends array of values of arbitrary type from master to all slaves; does not change values on master
		{
		broadcastRaw(data,sizeof(DataParam)*numItems);
		}
	void finishMessage(void); // Sends the current message as soon as possible and starts a new message
	
	/* Master-side interface: */
	bool mustSwapOnWrite(void) const // Dummy function always returning false
		{
		return false;
		}
	void writeRaw(const void* data,size_t size); // Writes uninterpreted binary data of given size to pipe
	template <class DataParam>
	void write(const DataParam& data) // Writes single value of arbitrary type
		{
		writeRaw(&data,sizeof(DataParam));
		}
	template <class DataParam>
	void write(const DataParam* data,size_t numItems) // Writes array of values of arbitrary type
		{
		writeRaw(data,sizeof(DataParam)*numItems);
		}
	
	/* Slave-side interface: */
	bool mustSwapOnRead(void) const // Dummy function always returning false
		{
		return false;
		}
	void readRaw(void* data,size_t size); // Reads uninterpreted binary data of given size from pipe
	template <class DataParam>
	DataParam read(void) // Reads single value of arbitrary type
		{
		DataParam result;
		readRaw(&result,sizeof(DataParam));
		return result;
		}
	template <class DataParam>
	DataParam& read(DataParam& data) // Reads single value of arbitrary type through reference
		{
		readRaw(&data,sizeof(DataParam));
		return data;
		}
	template <class DataParam>
	void read(DataParam* data,size_t numItems) // Reads array of values of arbitrary type
		{
		readRaw(data,sizeof(DataParam)*numItems);
		}
	};

}

#endif
