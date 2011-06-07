/***********************************************************************
ClusterPipe - Class layering an endianness-safe pipe abstraction with
buffered typed read/writes over a TCPSocket connected to a remote
process and a MulticastPipe to forward traffic to a cluster connected to
the local process.
Copyright (c) 2007-2010 Oliver Kreylos

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

#include <string.h>
#include <string>
#include <Misc/Endianness.h>
#include <Threads/Mutex.h>
#include <Comm/TCPSocket.h>
#include <Comm/MulticastPipe.h>

namespace Comm {

class ClusterPipe
	{
	/* Embedded classes: */
	public:
	enum Endianness // Enumerated type to enforce pipe endianness
		{
		DontCare,LittleEndian,BigEndian,Automatic
		};
	
	/* Elements: */
	private:
	Threads::Mutex socketMutex; // Mutex protecting write access to the socket
	TCPSocket* socket; // TCP socket from the cluster's head node to the outside world
	MulticastPipe* pipe; // Multicast pipe to communicate between cluster nodes (pipe is deleted on socket closure)
	bool readMustSwapEndianness; // Flag if incoming data has to be endianness-swapped
	bool writeMustSwapEndianness; // Flag if outgoing data has to be endianness-swapped
	size_t bufferSize; // Size of read and write buffers in bytes
	char* readBuffer; // Buffer to store incoming data between read<> calls
	char* rbPos; // Current position in read buffer
	size_t readSize; // Number of bytes left in read buffer
	char* writeBuffer; // Buffer to store outgoing data between write<> calls
	char* wbPos; // Current position in write buffer
	size_t writeSize; // Number of bytes left in write buffer
	
	/* Private methods: */
	void directRead(void* data,size_t dataSize)
		{
		memcpy(data,rbPos,dataSize);
		rbPos+=dataSize;
		readSize-=dataSize;
		}
	void directWrite(const void* data,size_t dataSize)
		{
		memcpy(wbPos,data,dataSize);
		wbPos+=dataSize;
		writeSize-=dataSize;
		}
	void bufferedRead(void* data,size_t dataSize);
	void bufferedWrite(const void* data,size_t dataSize);
	void initializePipe(Endianness sEndianness);
	
	/* Constructors and destructors: */
	public:
	ClusterPipe(const TCPSocket* sSocket,MulticastPipe* sPipe,Endianness sEndianness =Automatic); // Creates a pipe over an existing TCP socket; assumes ownership of pipe
	ClusterPipe(std::string hostname,int portId,MulticastPipe* sPipe,Endianness sEndianness =Automatic); // Creates a pipe connected to a remote host; assumes ownership of pipe
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
	
	/* Basic socket-level methods: */
	int getPeerPortId(void) const; // Returns port ID of remote socket
	std::string getPeerAddress(void) const; // Returns internet address of remote socket in dotted notation
	std::string getPeerHostname(void) const; // Returns host name of remote socket (or dotted address if host name cannot be resolved)
	void shutdown(bool shutdownRead,bool shutdownWrite) // Shuts down the read or write part of the socket; further reads or writes are not allowed
		{
		/* Forward the call to the socket: */
		if(socket!=0)
			socket->shutdown(shutdownRead,shutdownWrite);
		}
	bool waitForData(long timeoutSeconds,long timeoutMicroseconds,bool throwException =true) const; // Waits for incoming data on TCP socket; returns true if data is ready; (optionally) throws exception if wait times out
	bool waitForData(const Misc::Time& timeout,bool throwException =true) const; // Waits for incoming data on TCP socket; returns true if data is ready; (optionally) throws exception if wait times out
	size_t readUpto(void* buffer,size_t count); // Replacement for TCPSocket's raw read method; reads between one and count bytes and returns number of bytes read
	void flush(void) // Flushes the write buffer after a series of write operations
		{
		if(socket!=0&&writeSize<bufferSize)
			{
			/* Send leftover data: */
			socket->blockingWrite(writeBuffer,bufferSize-writeSize);
			
			/* Reset the write buffer: */
			wbPos=writeBuffer;
			writeSize=bufferSize;
			}
		}
	
	/* New methods: */
	size_t getBufferSize(void) const // Returns the size of the read and write buffers
		{
		return bufferSize;
		}
	
	/* Endianness-safe binary I/O interface: */
	bool mustSwapOnRead(void) // Retusn true if the pipe must endianness-swap data on read
		{
		return readMustSwapEndianness;
		}
	void readRaw(void* data,size_t dataSize) // Reads raw data without endianness conversion
		{
		if(dataSize<=readSize)
			directRead(data,dataSize);
		else
			bufferedRead(data,dataSize);
		}
	template <class DataParam>
	DataParam read(void) // Reads an element of the given data type from the pipe
		{
		DataParam result;
		if(sizeof(DataParam)<=readSize)
			directRead(&result,sizeof(DataParam));
		else
			bufferedRead(&result,sizeof(DataParam));
		if(readMustSwapEndianness)
			Misc::swapEndianness(result);
		return result;
		}
	template <class DataParam>
	DataParam& read(DataParam& data) // Reads an element of the given data type from the pipe
		{
		if(sizeof(DataParam)<=readSize)
			directRead(&data,sizeof(DataParam));
		else
			bufferedRead(&data,sizeof(DataParam));
		if(readMustSwapEndianness)
			Misc::swapEndianness(data);
		return data;
		}
	template <class DataParam>
	void read(DataParam* data,size_t numItems) // Reads an array of elements from the pipe
		{
		if(numItems*sizeof(DataParam)<=readSize)
			directRead(data,numItems*sizeof(DataParam));
		else
			bufferedRead(data,numItems*sizeof(DataParam));
		if(readMustSwapEndianness)
			Misc::swapEndianness(data,numItems);
		}
	bool mustSwapOnWrite(void) // Returns true if the pipe must endianness-swap data on write
		{
		return writeMustSwapEndianness;
		}
	void writeRaw(const void* data,size_t dataSize) // Writes raw data without endianness conversion
		{
		if(socket!=0)
			{
			if(dataSize<=writeSize)
				directWrite(data,dataSize);
			else
				bufferedWrite(data,dataSize);
			}
		}
	template <class DataParam>
	void write(const DataParam& data) // Writes an element of the given data type to the pipe
		{
		if(socket!=0)
			{
			if(writeMustSwapEndianness)
				{
				DataParam temp=data;
				Misc::swapEndianness(temp);
				if(sizeof(DataParam)<=writeSize)
					directWrite(&temp,sizeof(DataParam));
				else
					bufferedWrite(&temp,sizeof(DataParam));
				}
			else
				{
				if(sizeof(DataParam)<=writeSize)
					directWrite(&data,sizeof(DataParam));
				else
					bufferedWrite(&data,sizeof(DataParam));
				}
			}
		}
	template <class DataParam>
	void write(const DataParam* data,size_t numItems) // Writes an array of elements to the pipe
		{
		if(socket!=0)
			{
			if(writeMustSwapEndianness)
				{
				for(size_t i=0;i<numItems;++i)
					{
					DataParam temp=data[i];
					Misc::swapEndianness(temp);
					if(sizeof(DataParam)<=writeSize)
						directWrite(&temp,sizeof(DataParam));
					else
						bufferedWrite(&temp,sizeof(DataParam));
					}
				}
			else
				{
				if(numItems*sizeof(DataParam)<writeSize)
					directWrite(data,numItems*sizeof(DataParam));
				else
					bufferedWrite(data,numItems*sizeof(DataParam));
				}
			}
		}
	};

/***********************************
Specializations of template methods:
***********************************/

template <>
std::string ClusterPipe::read<std::string>(void);
template <>
std::string& ClusterPipe::read<std::string>(std::string&);
template <>
void ClusterPipe::write<std::string>(const std::string&);

}

#endif
