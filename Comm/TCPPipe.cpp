/***********************************************************************
TCPPipe - Class layering an endianness-safe pipe abstraction with
buffered typed read/writes over a TCPSocket.
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

#include <Comm/TCPPipe.h>

namespace Comm {

/************************
Methods of class TCPPipe:
************************/

void TCPPipe::bufferedRead(void* data,size_t dataSize)
	{
	char* dPtr=static_cast<char*>(data);
	while(dataSize>0)
		{
		/* Receive more data if the read buffer is empty: */
		if(readSize==0)
			{
			/* Read available data from the TCP socket: */
			readSize=TCPSocket::read(readBuffer,bufferSize);
			
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

void TCPPipe::bufferedWrite(const void* data,size_t dataSize)
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
			blockingWrite(writeBuffer,bufferSize);
			
			/* Reset the write buffer: */
			wbPos=writeBuffer;
			writeSize=bufferSize;
			}
		}
	}

void TCPPipe::initializePipe(TCPPipe::Endianness sEndianness)
	{
	/* Set socket options: */
	TCPSocket::setNoDelay(true);
	
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
	else if(sEndianness==Automatic)
		{
		/* Exchange a magic value to test for endianness on the other end: */
		unsigned int magic=0x12345678U;
		blockingWrite(&magic,sizeof(unsigned int));
		blockingRead(&magic,sizeof(unsigned int));
		if(magic==0x78563412U)
			readMustSwapEndianness=true;
		else if(magic!=0x12345678U)
			Misc::throwStdErr("Comm::TCPPipe: Unable to establish connection with host %s on port %d",getPeerHostname().c_str(),getPeerPortId());
		}
	
	/* Allocate the read and write buffers: */
	int maxSegSize=-1;
	socklen_t maxSegLen=sizeof(int);
	if(getsockopt(getFd(),IPPROTO_TCP,TCP_MAXSEG,&maxSegSize,&maxSegLen)<0)
		Misc::throwStdErr("Comm::TCPPipe: Unable to determine maximum TCP segment size");
	bufferSize=size_t(maxSegSize);
	readBuffer=new char[bufferSize];
	rbPos=readBuffer;
	readSize=0;
	writeBuffer=new char[bufferSize];
	wbPos=writeBuffer;
	writeSize=bufferSize;
	}

TCPPipe::TCPPipe(std::string hostname,int portId,TCPPipe::Endianness sEndianness)
	:TCPSocket(hostname,portId),
	 readMustSwapEndianness(false),writeMustSwapEndianness(false),
	 readBuffer(0),writeBuffer(0)
	{
	/* Set up the socket, endianness conversion, and read/write buffers: */
	initializePipe(sEndianness);
	}

TCPPipe::TCPPipe(const TCPSocket& socket,TCPPipe::Endianness sEndianness)
	:TCPSocket(socket),
	 readMustSwapEndianness(false),
	 writeMustSwapEndianness(false),
	 readBuffer(0),writeBuffer(0)
	{
	/* Set up the socket, endianness conversion, and read/write buffers: */
	initializePipe(sEndianness);
	}

TCPPipe::~TCPPipe(void)
	{
	try
		{
		/* Send any leftover data still in the write buffer: */
		if(writeSize<bufferSize)
			blockingWrite(writeBuffer,bufferSize-writeSize);
		}
	catch(...)
		{
		/* This was a best effort to flush the pipe; if it fails, the pipe was probably already dead anyways... */
		}
	
	/* Delete the buffers: */
	delete[] readBuffer;
	delete[] writeBuffer;
	}

/***********************************
Specializations of template methods:
***********************************/

template <>
std::string
TCPPipe::read<std::string>(
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
TCPPipe::read<std::string>(
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
TCPPipe::write<std::string>(
	const std::string& string)
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
