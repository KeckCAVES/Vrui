/***********************************************************************
TCPPipe - Class layering an endianness-safe pipe abstraction with
buffered typed read/writes over a TCPSocket.
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

#ifndef COMM_TCPPIPE_INCLUDED
#define COMM_TCPPIPE_INCLUDED

#include <string.h>
#include <string>
#include <Misc/Endianness.h>
#include <Comm/TCPSocket.h>

namespace Comm {

class TCPPipe:public TCPSocket
	{
	/* Embedded classes: */
	public:
	enum Endianness // Enumerated type to enforce pipe endianness
		{
		DontCare,LittleEndian,BigEndian,Automatic
		};
	
	/* Elements: */
	private:
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
	TCPPipe(std::string hostname,int portId,Endianness sEndianness =Automatic); // Creates a pipe connected to a remote host
	TCPPipe(const TCPSocket& socket,Endianness sEndianness =Automatic); // Creates a pipe layered over an existing (receiving) TCP socket
	private:
	TCPPipe(const TCPPipe& source); // Prohibit copy constructor
	TCPPipe& operator=(const TCPPipe& source); // Prohibit assignment operator
	public:
	~TCPPipe(void); // Destroys pipe
	
	/* Override methods to adapt behavior of base TCPSocket: */
	void setNoDelay(bool enable) // Dummy override function to prohibit disabling TCP_NODELAY, which interferes with new stream handling algorithm
		{
		/* Do nothing! */
		}
	void setCork(bool enable) // Dummy override function to prohibit enabling TCP_CORK, which interferes with new stream handling algorithm
		{
		/* Do nothing! */
		}
	bool waitForData(long timeoutSeconds,long timeoutMicroseconds,bool throwException =true) const // Overrides TCPSocket's waitForData method
		{
		/* Check the read buffer before checking the TCP socket: */
		if(readSize==0)
			return TCPSocket::waitForData(timeoutSeconds,timeoutMicroseconds,throwException);
		else
			return true;
		}
	bool waitForData(const Misc::Time& timeout,bool throwException =true) const // Overrides TCPSocket's waitForData method
		{
		/* Check the read buffer before checking the TCP socket: */
		if(readSize==0)
			return TCPSocket::waitForData(timeout,throwException);
		else
			return true;
		}
	size_t readUpto(void* buffer,size_t count) // Replacement for TCPSocket's raw read method; reads between one and count bytes and returns number of bytes read
		{
		if(readSize==0)
			{
			/* Read available data from the TCP socket: */
			readSize=TCPSocket::read(readBuffer,bufferSize);
			
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
	void flush(void) // Sends any data left in the write buffer
		{
		if(writeSize<bufferSize)
			{
			/* Send leftover data: */
			blockingWrite(writeBuffer,bufferSize-writeSize);
			
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
	DataParam read(void) // Reads single value
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
	DataParam& read(DataParam& data) // Reads single value through reference
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
	void read(DataParam* data,size_t numItems) // Reads array of values
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
		if(dataSize<=writeSize)
			directWrite(data,dataSize);
		else
			bufferedWrite(data,dataSize);
		}
	template <class DataParam>
	void write(const DataParam& data) // Writes single value
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
	template <class DataParam>
	void write(const DataParam* data,size_t numItems) // Writes array of values
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
			if(numItems*sizeof(DataParam)<=writeSize)
				directWrite(data,numItems*sizeof(DataParam));
			else
				bufferedWrite(data,numItems*sizeof(DataParam));
			}
		}
	};

/***********************************
Specializations of template methods:
***********************************/

template <>
std::string TCPPipe::read<std::string>(void);
template <>
std::string& TCPPipe::read<std::string>(std::string&);
template <>
void TCPPipe::write<std::string>(const std::string&);

}

#endif
