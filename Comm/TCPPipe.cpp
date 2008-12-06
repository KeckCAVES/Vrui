/***********************************************************************
TCPPipe - Class layering an endianness-safe pipe abstraction over a
TCPSocket.
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

#include <Misc/ThrowStdErr.h>
#include <Comm/TCPPipe.h>

namespace Comm {

/************************
Methods of class TCPPipe:
************************/

TCPPipe::TCPPipe(std::string hostname,int portId,TCPPipe::Endianness sEndianness)
	:TCPSocket(hostname,portId),
	 readMustSwapEndianness(false),
	 writeMustSwapEndianness(false)
	{
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
	}

TCPPipe::TCPPipe(const TCPSocket& socket,TCPPipe::Endianness sEndianness)
	:TCPSocket(socket),
	 readMustSwapEndianness(false),
	 writeMustSwapEndianness(false)
	{
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
	blockingRead(&length,sizeof(unsigned int));
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
		blockingRead(buffer,readLength);
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
	blockingRead(&length,sizeof(unsigned int));
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
		blockingRead(buffer,readLength);
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
	blockingWrite(&length,sizeof(unsigned int));
	
	/* Write the string's characters: */
	blockingWrite(string.data(),string.length()*sizeof(char));
	}

}
