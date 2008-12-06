/***********************************************************************
ClusterPipe - Class layering a pipe abstraction over a TCPSocket
connected to a remote process and a MulticastPipe to forward traffic to
a cluster connected to the local process.
Copyright (c) 2007 Oliver Kreylos

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

#ifndef COMM_CLUSTERPIPE_INCLUDED
#define COMM_CLUSTERPIPE_INCLUDED

#include <Misc/Endianness.h>
#include <Threads/Mutex.h>
#include <Comm/TCPSocket.h>
#include <Comm/MulticastPipe.h>

namespace Comm {

class ClusterPipe
	{
	/* Elements: */
	private:
	Threads::Mutex socketMutex; // Mutex protecting write access to the socket
	TCPSocket* socket; // TCP socket from the cluster's head node to the outside world
	MulticastPipe* pipe; // Multicast pipe to communicate between cluster nodes (pipe is deleted on socket closure)
	bool mustSwapEndianness; // Flag if incoming data has to be endianness-swapped
	
	/* Constructors and destructors: */
	public:
	ClusterPipe(const TCPSocket* sSocket,MulticastPipe* sPipe =0); // Creates a pipe over an existing TCP socket
	ClusterPipe(std::string hostname,int portId,MulticastPipe* sPipe =0); // Creates a pipe connected to a remote host; assumes ownership of pipe
	private:
	ClusterPipe(const ClusterPipe& source); // Prohibit copy constructor
	ClusterPipe& operator=(const ClusterPipe& source); // Prohibit assignment operator
	public:
	~ClusterPipe(void); // Closes the socket and deletes the pipe
	
	/* Methods: */
	Threads::Mutex& getMutex(void) // Returns the write access mutex
		{
		return socketMutex;
		}
	int getPeerPortId(void) const; // Returns port ID of remote socket
	std::string getPeerAddress(void) const; // Returns internet address of remote socket in dotted notation; returns dummy string on slave nodes
	std::string getPeerHostname(void) const; // Returns host name of remote socket (or dotted address if host name cannot be resolved); returns dummy string on slave nodes
	void shutdown(bool shutdownRead,bool shutdownWrite) // Shuts down the read or write part of the socket; further reads or writes are not allowed
		{
		/* Forward the call to the socket: */
		if(socket!=0)
			socket->shutdown(shutdownRead,shutdownWrite);
		}
	bool waitForData(long timeoutSeconds,long timeoutMicroseconds,bool throwException =true) const // Waits for incoming data on TCP socket; returns true if data is ready; (optionally) throws exception if wait times out
		{
		int flag;
		if(socket!=0)
			flag=socket->waitForData(timeoutSeconds,timeoutMicroseconds,false)?1:0;
		if(pipe!=0)
			{
			pipe->broadcastRaw(&flag,sizeof(int));
			pipe->finishMessage();
			}
		if(throwException&&flag==0)
			throw TCPSocket::TimeOut("TCPSocket: Time-out while waiting for data");
		
		return flag!=0;
		}
	bool waitForData(const Misc::Time& timeout,bool throwException =true) const // Waits for incoming data on TCP socket; returns true if data is ready; (optionally) throws exception if wait times out
		{
		int flag;
		if(socket!=0)
			flag=socket->waitForData(timeout,false)?1:0;
		if(pipe!=0)
			{
			pipe->broadcastRaw(&flag,sizeof(int));
			pipe->finishMessage();
			}
		if(throwException&&flag==0)
			throw TCPSocket::TimeOut("TCPSocket: Time-out while waiting for data");
		
		return flag!=0;
		}
	template <class DataParam>
	void write(const DataParam& data) // Writes an element of the given data type to the pipe
		{
		if(socket!=0)
			socket->blockingWrite(&data,sizeof(DataParam));
		}
	template <class DataParam>
	void read(DataParam& data) // Reads an element of the given data type from the pipe
		{
		if(socket!=0)
			{
			socket->blockingRead(&data,sizeof(DataParam));
			
			/* Swap endianness if necessary: */
			if(mustSwapEndianness)
				Misc::swapEndianness(data);
			
			if(pipe!=0)
				pipe->writeRaw(&data,sizeof(DataParam));
			}
		else
			pipe->readRaw(&data,sizeof(DataParam));
		}
	template <class DataParam>
	DataParam read(void) // Reads an element of the given data type from the pipe
		{
		DataParam result;
		if(socket!=0)
			{
			socket->blockingRead(&result,sizeof(DataParam));
			
			/* Swap endianness if necessary: */
			if(mustSwapEndianness)
				Misc::swapEndianness(result);
			
			if(pipe!=0)
				pipe->writeRaw(&result,sizeof(DataParam));
			}
		else
			pipe->readRaw(&result,sizeof(DataParam));
		
		return result;
		}
	template <class DataParam>
	void write(const DataParam* elements,size_t numElements) // Writes an array of elements to the pipe
		{
		if(socket!=0)
			socket->blockingWrite(elements,numElements*sizeof(DataParam));
		}
	template <class DataParam>
	void read(DataParam* elements,size_t numElements) // Reads an array of elements from the pipe
		{
		if(socket!=0)
			{
			socket->blockingRead(elements,numElements*sizeof(DataParam));
			
			/* Swap endianness if necessary: */
			if(mustSwapEndianness)
				Misc::swapEndianness(elements,numElements);
			
			if(pipe!=0)
				pipe->writeRaw(elements,numElements*sizeof(DataParam));
			}
		else
			pipe->readRaw(elements,numElements*sizeof(DataParam));
		}
	void flushWrite(void) // Flushes the write buffer after a series of write operations
		{
		if(socket!=0)
			socket->flush();
		}
	void flushRead(void) // Flushes the read buffer after a series of read operations
		{
		if(pipe!=0)
			pipe->finishMessage();
		}
	};

/***********************************
Specializations of template methods:
***********************************/

template <>
inline
void
ClusterPipe::write<std::string>(
	const std::string& string)
	{
	if(socket!=0)
		{
		unsigned int length=string.length();
		socket->blockingWrite(&length,sizeof(unsigned int));
		socket->blockingWrite(string.data(),length*sizeof(char));
		}
	}

template <>
inline
std::string
ClusterPipe::read<std::string>(
	void)
	{
	std::string result;
	
	/* Read the string's length: */
	unsigned int length;
	if(socket!=0)
		socket->blockingRead(&length,sizeof(unsigned int));
	
	/* Swap endianness if necessary: */
	if(mustSwapEndianness)
		Misc::swapEndianness(length);
	
	/* Forward the length to the cluster nodes: */
	if(pipe!=0)
		pipe->broadcastRaw(&length,sizeof(unsigned int));
	
	/* Read the string in chunks (unfortunately, there is no API to read directly into the std::string): */
	result.reserve(length+1); // Specification is not clear whether reserve() includes room for the terminating NUL character
	unsigned int lengthLeft=length;
	while(lengthLeft>0)
		{
		char buffer[256];
		size_t readLength=lengthLeft;
		if(readLength>sizeof(buffer))
			readLength=sizeof(buffer);
		if(socket!=0)
			socket->blockingRead(buffer,readLength);
		if(pipe!=0)
			pipe->broadcastRaw(buffer,readLength);
		result.append(buffer,readLength);
		lengthLeft-=readLength;
		}
	
	return result;
	}

template <>
inline
void
ClusterPipe::read<std::string>(
	std::string& string)
	{
	/* Read the string's length: */
	unsigned int length;
	if(socket!=0)
		socket->blockingRead(&length,sizeof(unsigned int));
	
	/* Swap endianness if necessary: */
	if(mustSwapEndianness)
		Misc::swapEndianness(length);
	
	/* Forward the length to the cluster nodes: */
	if(pipe!=0)
		pipe->broadcastRaw(&length,sizeof(unsigned int));
	
	/* Read the string in chunks (unfortunately, there is no API to read directly into the std::string): */
	string.reserve(length+1); // Specification is not clear whether reserve() includes room for the terminating NUL character
	unsigned int lengthLeft=length;
	while(lengthLeft>0)
		{
		char buffer[256];
		size_t readLength=lengthLeft;
		if(readLength>sizeof(buffer))
			readLength=sizeof(buffer);
		if(socket!=0)
			socket->blockingRead(buffer,readLength);
		if(pipe!=0)
			pipe->broadcastRaw(buffer,readLength);
		string.append(buffer,readLength);
		lengthLeft-=readLength;
		}
	}

}

#endif
