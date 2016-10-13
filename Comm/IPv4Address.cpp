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

#include <Comm/IPv4Address.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/PrintInteger.h>

namespace Comm {

/****************************
Methods of class IPv4Address:
****************************/

IPv4Address::IPv4Address(const char* hostname)
	{
	/* Lookup host's IP address: */
	struct hostent* hostEntry=gethostbyname(hostname);
	if(hostEntry==0)
		Misc::throwStdErr("Comm::IPv4Address: Unable to resolve host name %s",hostname);
	s_addr=((struct in_addr*)hostEntry->h_addr_list[0])->s_addr;
	}

std::string IPv4Address::getAddress(void) const
	{
	/* Convert the address to dotted notation: */
	char addressBuffer[INET_ADDRSTRLEN];
	inet_ntop(AF_INET,this,addressBuffer,INET_ADDRSTRLEN);
	return addressBuffer;
	}

std::string IPv4Address::getHostname(void) const
	{
	struct hostent* hostEntry=gethostbyaddr(this,sizeof(IPv4Address),AF_INET);
	if(hostEntry==0)
		{
		/* Fall back to returning address in dotted notation: */
		char addressBuffer[INET_ADDRSTRLEN];
		inet_ntop(AF_INET,this,addressBuffer,INET_ADDRSTRLEN);
		return addressBuffer;
		}
	else
		return hostEntry->h_name;
	}

}
