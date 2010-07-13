/***********************************************************************
ClusterPipe - Class layering an endianness-safe pipe abstraction with
buffered typed read/writes over a TCPSocket connected to a remote
process and a MulticastPipe to forward traffic to a cluster connected to
the local process.
Copyright (c) 2007-2009 Oliver Kreylos

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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <Misc/ThrowStdErr.h>

#include <Comm/ClusterPipe.h>

namespace Comm {

/****************************
Methods of class ClusterPipe:
****************************/

void ClusterPipe::bufferedRead(void* data,size_t dataSize)
	{
	char* dPtr=static_cast<char*>(data);
	while(dataSize>0)
		{
		/* Receive more data if the read buffer is empty: */
		if(readSize==0)
			{
			if(socket!=0)
				{
				/* Read available data from the TCP socket: */
				readSize=socket->read(readBuffer,bufferSize);
				
				if(pipe!=0)
					{
					/* Forward the read data to the cluster: */
					unsigned int tReadSize=(unsigned int)readSize;
					pipe->writeRaw(&tReadSize,sizeof(unsigned int));
					pipe->writeRaw(readBuffer,readSize);
					pipe->finishMessage();
					}
				}
			else
				{
				/* Read data from the master: */
				unsigned int tReadSize;
				pipe->readRaw(&tReadSize,sizeof(unsigned int));
				readSize=size_t(tReadSize);
				pipe->readRaw(readBuffer,readSize);
				}
			
			/* Reset the read buffer: */
			rbPos=readBuffer;
			}
		
		/* Determine the number of bytes to read in a single go: */
		size_t bytes=dataSize;
		if(bytes>readSize)
			bytes=readSize;
		
		/* Copy bytes from the read buffer: */
		memcpy(dPtr,rbPos,bytes);
		rbPos+=bytes;
		readSize-=bytes;
		dPtr+=bytes;
		dataSize-=bytes;
		}
	}

void ClusterPipe::bufferedWrite(const void* data,size_t dataSize)
	{
	const char* dPtr=static_cast<const char*>(data);
	while(dataSize>0)
		{
		/* Determine the number of bytes to write in a single go: */
		size_t bytes=dataSize;
		if(bytes>writeSize)
			bytes=writeSize;
		
		/* Copy bytes into the write buffer: */
		memcpy(wbPos,dPtr,bytes);
		wbPos+=bytes;
		writeSize-=bytes;
		dPtr+=bytes;
		dataSize-=bytes;
		
		/* Send the write buffer if full: */
		if(writeSize==0)
			{
			/* Send data across the TCP socket: */
			socket->blockingWrite(writeBuffer,bufferSize);
			
			/* Reset the write buffer: */
			wbPos=writeBuffer;
			writeSize=bufferSize;
			}
		}
	}

void ClusterPipe::initializePipe(ClusterPipe::Endianness sEndianness)
	{
	/* Check for fixed network byte order: */
	if(sEndianness==LittleEndian)
		{
		#if __BYTE_ORDER==__BIG_ENDIAN
		readMustSwapEndianness=writeMustSwapEndianness=true;
		#endif
		}
	else if(sEndianness==BigEndian)
		{
		#if __BYTE_ORDER==__LITTLE_ENDIAN
		readMustSwapEndianness=writeMustSwapEndianness=true;
		#endif
		}
	
	if(socket!=0)
		{
		/* Set socket options: */
		socket->setNoDelay(true);
		
		/* Check for automatic receiver-makes-right endianness conversion: */
		if(sEndianness==Automatic)
			{
			/* Exchange a magic value to test for endianness on the other end: */
			unsigned int magic=0x12345678U;
			socket->blockingWrite(&magic,sizeof(unsigned int));
			socket->blockingRead(&magic,sizeof(unsigned int));
			if(pipe!=0)
				{
				/* Forward the received magic value to the cluster: */
				pipe->writeRaw(&magic,sizeof(unsigned int));
				pipe->finishMessage();
				}
			if(magic==0x78563412U)
				readMustSwapEndianness=true;
			else if(magic!=0x12345678U)
				Misc::throwStdErr("ClusterPipe::ClusterPipe: Could not establish connection with host %s on port %d",socket->getPeerHostname().c_str(),socket->getPeerPortId());
			}
		
		/* Determine the TCP segment size: */
		bufferSize=1448; // Seems to be a reasonable default
		int maxSegSize=-1;
		socklen_t maxSegLen=sizeof(int);
		if(getsockopt(socket->getFd(),IPPROTO_TCP,TCP_MAXSEG,&maxSegSize,&maxSegLen)==0)
			bufferSize=size_t(maxSegSize);
		
		if(pipe!=0)
			{
			/* Forward the buffer size to the cluster: */
			unsigned int tBufferSize=(unsigned int)bufferSize;
			pipe->writeRaw(&tBufferSize,sizeof(unsigned int));
			pipe->finishMessage();
			}
		}
	else
		{
		/* Check for automatic receiver-makes-right endianness conversion: */
		if(sEndianness==Automatic)
			{
			/* Receive a magic value to test for endianness on the other end: */
			unsigned int magic;
			pipe->readRaw(&magic,sizeof(unsigned int));
			if(magic==0x78563412U)
				readMustSwapEndianness=true;
			else if(magic!=0x12345678U)
				Misc::throwStdErr("ClusterPipe::ClusterPipe: Could not establish connection");
			}
		
		/* Receive the buffer size from the master: */
		unsigned int tBufferSize;
		pipe->readRaw(&tBufferSize,sizeof(unsigned int));
		bufferSize=size_t(tBufferSize);
		}
	
	/* Allocate the read and write buffers: */
	readBuffer=new char[bufferSize];
	rbPos=readBuffer;
	readSize=0;
	if(socket!=0)
		{
		writeBuffer=new char[bufferSize];
		wbPos=writeBuffer;
		writeSize=bufferSize;
		}
	}

ClusterPipe::ClusterPipe(const TCPSocket* sSocket,MulticastPipe* sPipe,ClusterPipe::Endianness sEndianness)
	:socket(sSocket!=0?new TCPSocket(*sSocket):0),pipe(sPipe),
	 readMustSwapEndianness(false),writeMustSwapEndianness(false),
	 readBuffer(0),writeBuffer(0)
	{
	/* Set up the socket, multicast pipe, endianness conversion, and read/write buffers: */
	initializePipe(sEndianness);
	}

ClusterPipe::ClusterPipe(std::string hostname,int portId,MulticastPipe* sPipe,ClusterPipe::Endianness sEndianness)
	:socket(sPipe==0||sPipe->isMaster()?new TCPSocket(hostname,portId):0),pipe(sPipe),
	 readMustSwapEndianness(false),writeMustSwapEndianness(false),
	 readBuffer(0),writeBuffer(0)
	{
	/* Set up the socket, multicast pipe, endianness conversion, and read/write buffers: */
	initializePipe(sEndianness);
	}

ClusterPipe::~ClusterPipe(void)
	{
	try
		{
		/* Send any leftover data still in the write buffer: */
		if(socket!=0&&writeSize<bufferSize)
			socket->blockingWrite(writeBuffer,bufferSize-writeSize);
		}
	catch(...)
		{
		/* This was a best effort to flush the pipe; if it fails, the pipe was probably already dead anyways... */
		}
	
	/* Delete the buffers: */
	delete[] readBuffer;
	delete[] writeBuffer;
	
	/* Delete the socket and pipe: */
	delete socket;
	delete pipe; // The cluster pipe assumed ownership of the pipe on construction
	}

int ClusterPipe::getPeerPortId(void) const
	{
	int portId;
	
	if(socket!=0)
		{
		/* Get the port ID from the TCP socket: */
		portId=socket->getPeerPortId();
		
		if(pipe!=0)
			{
			/* Forward the port ID to the slaves: */
			pipe->writeRaw(&portId,sizeof(int));
			pipe->finishMessage();
			}
		}
	else
		{
		/* Get the port ID from the master: */
		pipe->readRaw(&portId,sizeof(int));
		}
	
	return portId;
	}

std::string ClusterPipe::getPeerAddress(void) const
	{
	std::string address;
	
	if(socket!=0)
		{
		/* Get the peer address from the TCP socket: */
		address=socket->getPeerAddress();
		
		if(pipe!=0)
			{
			/* Forward the address to the slaves: */
			unsigned int addressLength=address.length();
			pipe->writeRaw(&addressLength,sizeof(unsigned int));
			pipe->writeRaw(address.c_str(),addressLength+1);
			pipe->finishMessage();
			}
		}
	else
		{
		/* Get the address from the master: */
		unsigned int addressLength;
		pipe->readRaw(&addressLength,sizeof(unsigned int));
		char addressBuffer[16];
		pipe->readRaw(addressBuffer,addressLength+1);
		address=addressBuffer;
		}
	
	return address;
	}

std::string ClusterPipe::getPeerHostname(void) const
	{
	std::string hostname;
	
	if(socket!=0)
		{
		/* Get the peer hostname from the TCP socket: */
		hostname=socket->getPeerHostname(false);
		
		if(pipe!=0)
			{
			/* Forward the hostname to the slaves: */
			unsigned int hostnameLength=hostname.length();
			pipe->writeRaw(&hostnameLength,sizeof(unsigned int));
			pipe->writeRaw(hostname.c_str(),hostnameLength+1);
			pipe->finishMessage();
			}
		}
	else
		{
		/* Get the hostname from the master: */
		unsigned int hostnameLength;
		pipe->readRaw(&hostnameLength,sizeof(unsigned int));
		char* hostnameBuffer=new char[hostnameLength+1];
		pipe->readRaw(hostnameBuffer,hostnameLength+1);
		hostname=hostnameBuffer;
		delete[] hostnameBuffer;
		}
	
	return hostname;
	}

bool ClusterPipe::waitForData(long timeoutSeconds,long timeoutMicroseconds,bool throwException) const
	{
	/* Check if there is any data in the read buffer: */
	if(readSize>0)
		return true;
	
	/* Wait for data on the TCP socket: */
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

bool ClusterPipe::waitForData(const Misc::Time& timeout,bool throwException) const
	{
	/* Check if there is any data in the read buffer: */
	if(readSize>0)
		return true;
	
	/* Wait for data on the TCP socket: */
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

size_t ClusterPipe::readRaw(void* buffer,size_t count)
	{
	if(readSize==0)
		{
		if(socket!=0)
			{
			/* Read available data from the TCP socket: */
			readSize=socket->read(readBuffer,bufferSize);
			
			if(pipe!=0)
				{
				/* Forward the read data to the cluster: */
				unsigned int tReadSize=(unsigned int)readSize;
				pipe->writeRaw(&tReadSize,sizeof(unsigned int));
				pipe->writeRaw(readBuffer,readSize);
				pipe->finishMessage();
				}
			}
		else
			{
			/* Read data from the master: */
			unsigned int tReadSize;
			pipe->readRaw(&tReadSize,sizeof(unsigned int));
			readSize=size_t(tReadSize);
			pipe->readRaw(readBuffer,readSize);
			}

		/* Reset the read buffer: */
		rbPos=readBuffer;
		}
	
	/* Return data from the buffer: */
	if(count>readSize)
		count=readSize;
	memcpy(buffer,rbPos,count);
	rbPos+=count;
	readSize-=count;
	
	return count;
	}

/***********************************
Specializations of template methods:
***********************************/

template <>
std::string
ClusterPipe::read<std::string>(
	void)
	{
	std::string result;
	
	/* Read the string's length: */
	unsigned int length;
	bufferedRead(&length,sizeof(unsigned int));
	if(readMustSwapEndianness)
		Misc::swapEndianness(length);
	
	/* Read the string in chunks (unfortunately, there is no API to read directly into the std::string): */
	result.reserve(length+1); // Specification is not clear whether reserve() includes room for the terminating NUL character
	unsigned int lengthLeft=length;
	while(lengthLeft>0)
		{
		char buffer[256];
		size_t readLength=lengthLeft;
		if(readLength>sizeof(buffer))
			readLength=sizeof(buffer);
		bufferedRead(buffer,readLength);
		result.append(buffer,readLength);
		lengthLeft-=readLength;
		}
	
	return result;
	}

template <>
std::string&
ClusterPipe::read<std::string>(
	std::string& string)
	{
	/* Read the string's length: */
	unsigned int length;
	bufferedRead(&length,sizeof(unsigned int));
	if(readMustSwapEndianness)
		Misc::swapEndianness(length);
	
	/* Read the string in chunks (unfortunately, there is no API to read directly into the std::string): */
	string.reserve(length+1); // Specification is not clear whether reserve() includes room for the terminating NUL character
	unsigned int lengthLeft=length;
	while(lengthLeft>0)
		{
		char buffer[256];
		size_t readLength=lengthLeft;
		if(readLength>sizeof(buffer))
			readLength=sizeof(buffer);
		bufferedRead(buffer,readLength);
		string.append(buffer,readLength);
		lengthLeft-=readLength;
		}
	return string;
	}

template <>
void
ClusterPipe::write<std::string>(
	const std::string& string)
	{
	if(socket!=0)
		{
		/* Write the string's length: */
		unsigned int length=string.length();
		if(writeMustSwapEndianness)
			Misc::swapEndianness(length);
		bufferedWrite(&length,sizeof(unsigned int));
		
		/* Write the string's characters: */
		bufferedWrite(string.data(),string.length()*sizeof(char));
		}
	}

}
