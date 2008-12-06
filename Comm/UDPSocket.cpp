/***********************************************************************
UDPSocket - Wrapper class for UDP sockets ensuring exception safety.
Copyright (c) 2004-2005 Oliver Kreylos

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

#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <Misc/ThrowStdErr.h>

#include <Comm/UDPSocket.h>

namespace Comm {

/**************************
Methods of class UPDSocket:
**************************/

UDPSocket::UDPSocket(int localPortId,int)
	{
	/* Create the socket file descriptor: */
	socketFd=socket(PF_INET,SOCK_DGRAM,0);
	if(socketFd<0)
		Misc::throwStdErr("UDPSocket: Unable to create socket");
	
	/* Bind the socket file descriptor to the local port ID: */
	struct sockaddr_in socketAddress;
	socketAddress.sin_family=AF_INET;
	socketAddress.sin_port=localPortId>=0?htons(localPortId):0;
	socketAddress.sin_addr.s_addr=htonl(INADDR_ANY);
	if(bind(socketFd,(struct sockaddr*)&socketAddress,sizeof(struct sockaddr_in))==-1)
		{
		close(socketFd);
		Misc::throwStdErr("UDPSocket: Unable to bind socket to port %d",localPortId);
		}
	}

UDPSocket::UDPSocket(int localPortId,std::string hostname,int hostPortId)
	{
	/* Create the socket file descriptor: */
	socketFd=socket(PF_INET,SOCK_DGRAM,0);
	if(socketFd<0)
		Misc::throwStdErr("UDPSocket: Unable to create socket");
	
	/* Bind the socket file descriptor to the local port ID: */
	struct sockaddr_in mySocketAddress;
	mySocketAddress.sin_family=AF_INET;
	mySocketAddress.sin_port=localPortId>=0?htons(localPortId):0;
	mySocketAddress.sin_addr.s_addr=htonl(INADDR_ANY);
	if(bind(socketFd,(struct sockaddr*)&mySocketAddress,sizeof(struct sockaddr_in))==-1)
		{
		close(socketFd);
		Misc::throwStdErr("UDPSocket: Unable to bind socket to port %d",localPortId);
		}
	
	/* Lookup host's IP address: */
	struct hostent* hostEntry=gethostbyname(hostname.c_str());
	if(hostEntry==0)
		{
		close(socketFd);
		Misc::throwStdErr("UDPSocket: Unable to resolve host name %s",hostname.c_str());
		}
	struct in_addr hostNetAddress;
	hostNetAddress.s_addr=ntohl(((struct in_addr*)hostEntry->h_addr_list[0])->s_addr);
	
	/* Connect to the remote host: */
	struct sockaddr_in hostAddress;
	hostAddress.sin_family=AF_INET;
	hostAddress.sin_port=htons(hostPortId);
	hostAddress.sin_addr.s_addr=htonl(hostNetAddress.s_addr);
	if(::connect(socketFd,(const struct sockaddr*)&hostAddress,sizeof(struct sockaddr_in))==-1)
		{
		close(socketFd);
		Misc::throwStdErr("UDPSocket: Unable to connect to host %s on port %d",hostname.c_str(),hostPortId);
		}
	}

UDPSocket::UDPSocket(const UDPSocket& source)
	:socketFd(dup(source.socketFd))
	{
	}

UDPSocket::~UDPSocket(void)
	{
	close(socketFd);
	}

UDPSocket& UDPSocket::operator=(const UDPSocket& source)
	{
	if(this!=&source)
		{
		close(socketFd);
		socketFd=dup(source.socketFd);
		}
	return *this;
	}

int UDPSocket::getPortId(void) const
	{
	struct sockaddr_in socketAddress;
	#ifdef __SGI_IRIX__
	int socketAddressLen=sizeof(struct sockaddr_in);
	#else
	socklen_t socketAddressLen=sizeof(struct sockaddr_in);
	#endif
	getsockname(socketFd,(struct sockaddr*)&socketAddress,&socketAddressLen);
	return ntohs(socketAddress.sin_port);
	}

void UDPSocket::connect(std::string hostname,int hostPortId)
	{
	/* Lookup host's IP address: */
	struct hostent* hostEntry=gethostbyname(hostname.c_str());
	if(hostEntry==0)
		Misc::throwStdErr("UDPSocket: Unable to resolve host name %s",hostname.c_str());
	struct in_addr hostNetAddress;
	hostNetAddress.s_addr=ntohl(((struct in_addr*)hostEntry->h_addr_list[0])->s_addr);
	
	/* Connect to the remote host: */
	struct sockaddr_in hostAddress;
	hostAddress.sin_family=AF_INET;
	hostAddress.sin_port=htons(hostPortId);
	hostAddress.sin_addr.s_addr=htonl(hostNetAddress.s_addr);
	if(::connect(socketFd,(const struct sockaddr*)&hostAddress,sizeof(struct sockaddr_in))==-1)
		Misc::throwStdErr("UDPSocket: Unable to connect to host %s on port %d",hostname.c_str(),hostPortId);
	}

void UDPSocket::sendMessage(const void* messageBuffer,size_t messageSize)
	{
	ssize_t numBytesSent=send(socketFd,messageBuffer,messageSize,0);
	if(numBytesSent<0||numBytesSent!=ssize_t(messageSize))
		{
		/* Consider this a fatal error: */
		Misc::throwStdErr("UDPSocket: Fatal error during sendMessage");
		}
	}

size_t UDPSocket::receiveMessage(void* messageBuffer,size_t messageSize)
	{
	ssize_t numBytesReceived=recv(socketFd,messageBuffer,messageSize,0);
	if(numBytesReceived<0)
		{
		/* Consider this a fatal error: */
		Misc::throwStdErr("UDPSocket: Fatal error during receiveMessage");
		}
	return size_t(numBytesReceived);
	}

}
