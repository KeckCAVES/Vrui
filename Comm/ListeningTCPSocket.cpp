/***********************************************************************
ListeningTCPSocket - Class for TCP half-sockets that can accept incoming
connections.
Copyright (c) 2011-2015 Oliver Kreylos

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

#include <Comm/ListeningTCPSocket.h>

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <Misc/PrintInteger.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/FdSet.h>

#include <errno.h>
#include <unistd.h>
#include <sys/time.h>

namespace Comm {

/***********************************
Methods of class ListeningTCPSocket:
***********************************/

ListeningTCPSocket::ListeningTCPSocket(int portId,int backlog,ListeningTCPSocket::AddressFamily addressFamily)
	:fd(-1)
	{
	/* Convert port ID to string for getaddrinfo (awkward!): */
	if(portId<0||portId>65535)
		Misc::throwStdErr("Comm::ListeningTCPSocket: Invalid port %d",portId);
	char portIdBuffer[6];
	char* portIdString=Misc::print(portId,portIdBuffer+5);
	
	/* Create a local any-IP address: */
	struct addrinfo hints;
	memset(&hints,0,sizeof(struct addrinfo));
	switch(addressFamily)
		{
		case Any:
			hints.ai_family=AF_UNSPEC;
			break;
			
		case IPv4:
			hints.ai_family=AF_INET;
			break;
		
		case IPv6:
			hints.ai_family=AF_INET6;
			break;
		}
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_flags=AI_NUMERICSERV|AI_PASSIVE|AI_ADDRCONFIG;
	hints.ai_protocol=0;
	struct addrinfo* addresses;
	int aiResult=getaddrinfo(0,portIdString,&hints,&addresses);
	if(aiResult!=0)
		Misc::throwStdErr("Comm::ListeningTCPSocket: Unable to generate listening address on port %d due to error %s",portId,gai_strerror(aiResult));
	
	/* Try all returned addresses in order until one successfully binds: */
	struct addrinfo* aiPtr;
	for(aiPtr=addresses;aiPtr!=0;aiPtr=aiPtr->ai_next)
		{
		/* Open a socket: */
		fd=socket(aiPtr->ai_family,aiPtr->ai_socktype,aiPtr->ai_protocol);
		if(fd<0)
			continue;
		
		/* Bind the local address and bail out if successful: */
		if(bind(fd,reinterpret_cast<struct sockaddr*>(aiPtr->ai_addr),aiPtr->ai_addrlen)==0)
			break;
		
		/* Close the socket and try the next address: */
		close(fd);
		}
	
	/* Release the returned addresses: */
	freeaddrinfo(addresses);
	
	/* Check if socket setup failed: */
	if(aiPtr==0)
		Misc::throwStdErr("Comm::ListeningTCPSocket: Unable to create listening socket on port %d",portId);
	
	/* Start listening on the socket: */
	if(listen(fd,backlog)<0)
		{
		close(fd);
		Misc::throwStdErr("Comm::ListeningTCPSocket: Unable to start listening on port %d",portId);
		}
	}

ListeningTCPSocket::~ListeningTCPSocket(void)
	{
	if(fd>=0)
		close(fd);
	}

int ListeningTCPSocket::getPortId(void) const
	{
	/* Get the socket's local address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getsockname(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		Misc::throwStdErr("Comm::ListeningTCPSocket::getPortId: Unable to query socket address");
	
	/* Extract a numeric port ID from the socket's address: */
	char portIdBuffer[NI_MAXSERV];
	int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,0,0,portIdBuffer,sizeof(portIdBuffer),NI_NUMERICSERV);
	if(niResult!=0)
		Misc::throwStdErr("Comm::ListeningTCPSocket::getPortId: Unable to retrieve port ID due to error %s",gai_strerror(niResult));
	
	/* Convert the port ID string to a number: */
	int result=0;
	for(int i=0;i<NI_MAXSERV&&portIdBuffer[i]!='\0';++i)
		result=result*10+int(portIdBuffer[i]-'0');
	
	return result;
	}

std::string ListeningTCPSocket::getAddress(void) const
	{
	/* Get the socket's local address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getsockname(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		Misc::throwStdErr("Comm::ListeningTCPSocket::getAddress: Unable to query socket address");
	
	/* Extract a numeric address from the socket's address: */
	char addressBuffer[NI_MAXHOST];
	int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,addressBuffer,sizeof(addressBuffer),0,0,NI_NUMERICHOST);
	if(niResult!=0)
		Misc::throwStdErr("Comm::ListeningTCPSocket::getAddress: Unable to retrieve address due to error %s",gai_strerror(niResult));
	
	return addressBuffer;
	}

std::string ListeningTCPSocket::getInterfaceName(bool throwException) const
	{
	/* Get the socket's local address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getsockname(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		Misc::throwStdErr("Comm::ListeningTCPSocket::getInterfaceName: Unable to query socket address");
	
	/* Extract a numeric address from the socket's address: */
	char addressBuffer[NI_MAXHOST];
	int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,addressBuffer,sizeof(addressBuffer),0,0,0);
	if(niResult!=0)
		Misc::throwStdErr("Comm::ListeningTCPSocket::getInterfaceName: Unable to retrieve interface name due to error %s",gai_strerror(niResult));
	
	return addressBuffer;
	}

bool ListeningTCPSocket::waitForConnection(const Misc::Time& timeout) const
	{
	/* Wait for a connection (socket ready for reading) and return whether one is available: */
	Misc::FdSet readFds(fd);
	return Misc::pselect(&readFds,0,0,timeout)>=0&&readFds.isSet(fd);
	}

}
