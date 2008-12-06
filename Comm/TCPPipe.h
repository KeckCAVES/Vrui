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

#ifndef COMM_TCPPIPE_INCLUDED
#define COMM_TCPPIPE_INCLUDED

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
	
	/* Constructors and destructors: */
	public:
	TCPPipe(std::string hostname,int portId,Endianness sEndianness =Automatic); // Creates a pipe connected to a remote host
	TCPPipe(const TCPSocket& socket,Endianness sEndianness =Automatic); // Creates a pipe layered over an existing (receiving) TCP socket
	private:
	TCPPipe(const TCPPipe& source); // Prohibit copy constructor
	TCPPipe& operator=(const TCPPipe& source); // Prohibit assignment operator
	
	/* Methods: */
	public:
	template <class DataParam>
	DataParam read(void) // Reads single value
		{
		DataParam result;
		blockingRead(&result,sizeof(DataParam));
		if(readMustSwapEndianness)
			Misc::swapEndianness(result);
		return result;
		}
	template <class DataParam>
	DataParam& read(DataParam& data) // Reads single value through reference
		{
		blockingRead(&data,sizeof(DataParam));
		if(readMustSwapEndianness)
			Misc::swapEndianness(data);
		return data;
		}
	template <class DataParam>
	size_t read(DataParam* data,size_t numItems) // Reads array of values
		{
		blockingRead(data,numItems*sizeof(DataParam));
		if(readMustSwapEndianness)
			Misc::swapEndianness(data,numItems);
		return numItems;
		}
	template <class DataParam>
	void write(const DataParam& data) // Writes single value
		{
		if(writeMustSwapEndianness)
			{
			DataParam temp=data;
			Misc::swapEndianness(temp);
			blockingWrite(&temp,sizeof(DataParam));
			}
		else
			blockingWrite(&data,sizeof(DataParam));
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
				blockingWrite(&temp,sizeof(DataParam));
				}
			}
		else
			blockingWrite(data,sizeof(DataParam)*numItems);
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
