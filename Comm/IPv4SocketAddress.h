/***********************************************************************
IPv4SocketAddress - Simple wrapper class for IP v4 socket addresses in
network byte order.
Copyright (c) 2015-2017 Oliver Kreylos

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

#ifndef IPV4SOCKETADDRESS_INCLUDED
#define IPV4SOCKETADDRESS_INCLUDED

#include <netinet/in.h>
#include <Comm/IPv4Address.h>

namespace Comm {

class IPv4SocketAddress:public sockaddr_in
	{
	/* Constructors and destructors: */
	public:
	IPv4SocketAddress(unsigned int sPort =0U) // Constructs the "any" IP address with the given port number
		{
		sin_family=AF_INET;
		sin_port=htons(in_port_t(sPort));
		sin_addr.s_addr=htonl(INADDR_ANY);
		}
	IPv4SocketAddress(unsigned int sPort,const IPv4Address& sAddress) // Constructs socket address from port number and IP address
		{
		sin_family=AF_INET;
		sin_port=htons(in_port_t(sPort));
		sin_addr.s_addr=sAddress.s_addr;
		}
	IPv4SocketAddress(const struct sockaddr_in& source) // Copies socket address; assumes it really is an IP v4 socket address
		{
		sin_family=source.sin_family;
		sin_port=source.sin_port;
		sin_addr.s_addr=source.sin_addr.s_addr;
		}
	
	/* Methods: */
	bool operator==(const IPv4SocketAddress& other) const
		{
		return sin_family==other.sin_family&&sin_port==other.sin_port&&sin_addr.s_addr==other.sin_addr.s_addr;
		}
	bool operator!=(const IPv4SocketAddress& other) const
		{
		return sin_family!=other.sin_family||sin_port!=other.sin_port||sin_addr.s_addr!=other.sin_addr.s_addr;
		}
	unsigned int getPort(void) const // Returns the socket's port ID
		{
		return ntohs(sin_port);
		}
	IPv4Address getAddress(void) const // Returns the socket's IP address
		{
		return IPv4Address(sin_addr);
		}
	};

}

#endif
