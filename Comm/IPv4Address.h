/***********************************************************************
IPv4Address - Simple wrapper class for IP v4 addresses in network byte
order.
Copyright (c) 2015 Oliver Kreylos

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

#ifndef IPV4ADDRESS_INCLUDED
#define IPV4ADDRESS_INCLUDED

#include <netinet/in.h>
#include <string>

namespace Comm {

class IPv4Address:public in_addr
	{
	/* Constructors and destructors: */
	public:
	IPv4Address(void) // Constructs the "any" IP address
		{
		s_addr=htonl(INADDR_ANY);
		}
	IPv4Address(const struct in_addr& source) // Copies an existing IP address
		{
		s_addr=source.s_addr;
		}
	IPv4Address(const char* hostname); // Performs host lookup to convert an IP address in dotted decimal notation or a host name into an IP address
	
	/* Methods: */
	bool operator==(const IPv4Address& other) const
		{
		return s_addr==other.s_addr;
		}
	bool operator!=(const IPv4Address& other) const
		{
		return s_addr!=other.s_addr;
		}
	std::string getAddress(void) const; // Returns the address in dotted decimal notation
	std::string getHostname(void) const; // Returns a host name for the IP address, or the address in dotted notation if host name look-up fails
	};

}

#endif
