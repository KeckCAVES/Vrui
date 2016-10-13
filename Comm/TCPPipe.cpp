/***********************************************************************
TCPPipe - Class for high-performance reading/writing from/to connected
TCP sockets.
Copyright (c) 2010-2016 Oliver Kreylos

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

#include <Comm/TCPPipe.h>

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <Misc/PrintInteger.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/MessageLogger.h>
#include <Misc/FdSet.h>
#include <Comm/ListeningTCPSocket.h>

namespace Comm {

namespace {

/****************
String constants:
****************/

const char pipeReadErrorString[]="Comm::TCPPipe: Fatal error %d (%s) while reading from source";
const char pipeHangupErrorString[]="Comm::TCPPipe: Connection terminated by peer";
const char pipeWriteErrorString[]="Comm::TCPPipe: Fatal error %d (%s) while writing to sink";

}

/************************
Methods of class TCPPipe:
************************/

size_t TCPPipe::readData(IO::File::Byte* buffer,size_t bufferSize)
	{
	/* Read more data from source: */
	ssize_t readResult;
	do
		{
		readResult=::read(fd,buffer,bufferSize);
		}
	while(readResult<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR));
	
	/* Handle the result from the read call: */
	if(readResult<0)
		{
		/* Unknown error; probably a bad thing: */
		int error=errno;
		throw Error(Misc::printStdErrMsg(pipeReadErrorString,error,strerror(error)));
		}
	
	return size_t(readResult);
	}

void TCPPipe::writeData(const IO::File::Byte* buffer,size_t bufferSize)
	{
	while(bufferSize>0)
		{
		/* Write data to the sink: */
		ssize_t writeResult=::write(fd,buffer,bufferSize);
		if(writeResult>0)
			{
			/* Prepare to write more data: */
			buffer+=writeResult;
			bufferSize-=writeResult;
			}
		else if(writeResult==0)
			{
			/* Sink has reached end-of-file: */
			throw WriteError(bufferSize);
			}
		else if(errno==EPIPE)
			{
			/* Other side hung up: */
			throw Error(pipeHangupErrorString);
			}
		else if(errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)
			{
			/* Unknown error; probably a bad thing: */
			int error=errno;
			throw Error(Misc::printStdErrMsg(pipeWriteErrorString,error,strerror(error)));
			}
		}
	}

size_t TCPPipe::writeDataUpTo(const IO::File::Byte* buffer,size_t bufferSize)
	{
	/* Write data to the sink: */
	ssize_t writeResult;
	do
		{
		writeResult=::write(fd,buffer,bufferSize);
		}
	while(writeResult<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR));
	if(writeResult>0)
		return size_t(writeResult);
	else if(writeResult==0)
		{
		/* Sink has reached end-of-file: */
		throw WriteError(bufferSize);
		}
	else if(errno==EPIPE)
		{
		/* Other side hung up: */
		throw Error(pipeHangupErrorString);
		}
	else
		{
		/* Unknown error; probably a bad thing: */
		int error=errno;
		throw Error(Misc::printStdErrMsg(pipeWriteErrorString,error,strerror(error)));
		}
	}

namespace {

/****************
Helper functions:
****************/

void disableNagle(int fd) // Disables Nagle's algorithm on the given socket
	{
	/* Set the TCP_NODELAY socket option: */
	int flag=1;
	if(setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,&flag,sizeof(flag))==-1)
		{
		/* Close the socket and throw an exception: */
		close(fd);
		throw IO::File::Error("Comm::TCPPipe::TCPPipe: Unable to disable Nagle's algorithm on socket");
		}
	}

}

TCPPipe::TCPPipe(const char* hostName,int portId)
	:NetPipe(ReadWrite),
	 fd(-1)
	{
	/* Convert port ID to string for getaddrinfo (awkward!): */
	if(portId<0||portId>65535)
		throw OpenError(Misc::printStdErrMsg("Comm::TCPPipe::TCPPipe: Invalid port %d",portId));
	char portIdBuffer[6];
	char* portIdString=Misc::print(portId,portIdBuffer+5);
	
	/* Lookup host's IP address: */
	struct addrinfo hints;
	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family=AF_UNSPEC;
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_flags=AI_NUMERICSERV|AI_ADDRCONFIG;
	hints.ai_protocol=0;
	struct addrinfo* addresses;
	int aiResult=getaddrinfo(hostName,portIdString,&hints,&addresses);
	if(aiResult!=0)
		throw OpenError(Misc::printStdErrMsg("Comm::TCPPipe::TCPPipe: Unable to resolve host name %s due to error %s",hostName,gai_strerror(aiResult)));
	
	/* Try all returned addresses in order until one successfully connects: */
	for(struct addrinfo* aiPtr=addresses;aiPtr!=0;aiPtr=aiPtr->ai_next)
		{
		/* Open a socket: */
		fd=socket(aiPtr->ai_family,aiPtr->ai_socktype,aiPtr->ai_protocol);
		if(fd<0)
			continue;
		
		/* Connect to the remote host and bail out if successful: */
		if(connect(fd,reinterpret_cast<struct sockaddr*>(aiPtr->ai_addr),aiPtr->ai_addrlen)>=0)
			break;
		
		/* Close the socket and try the next address: */
		close(fd);
		fd=-1;
		}
	
	/* Release the returned addresses: */
	freeaddrinfo(addresses);
	
	/* Check if connection setup failed: */
	if(fd<0)
		throw OpenError(Misc::printStdErrMsg("Comm::TCPPipe::TCPPipe: Unable to connect to host %s on port %d",hostName,portId));
	
	/* Turn off socket-level buffering: */
	disableNagle(fd);
	}

TCPPipe::TCPPipe(ListeningTCPSocket& listenSocket)
	:NetPipe(ReadWrite),
	 fd(-1)
	{
	/* Wait for a connection attempt on the listening socket: */
	fd=accept(listenSocket.getFd(),0,0);
	if(fd==-1)
		throw OpenError("Comm::TCPPipe::TCPPipe: Unable to accept connection");
	
	/* Turn off socket-level buffering: */
	disableNagle(fd);
	}

TCPPipe::~TCPPipe(void)
	{
	try
		{
		/* Flush the write buffer: */
		flush();
		}
	catch(std::runtime_error err)
		{
		/* Print an error message and carry on: */
		Misc::formattedUserError("Comm::TCPPipe: Caught exception \"%s\" while closing pipe",err.what());
		}
	
	/* Close the socket: */
	if(fd>=0)
		close(fd);
	}

int TCPPipe::getFd(void) const
	{
	return fd;
	}

bool TCPPipe::waitForData(void) const
	{
	/* Check if there is unread data in the buffer: */
	if(getUnreadDataSize()>0)
		return true;
	
	/* Wait for data on the socket and return whether data is available: */
	Misc::FdSet readFds(fd);
	return Misc::pselect(&readFds,0,0,0)>=0&&readFds.isSet(fd);
	}

bool TCPPipe::waitForData(const Misc::Time& timeout) const
	{
	/* Check if there is unread data in the buffer: */
	if(getUnreadDataSize()>0)
		return true;
	
	/* Wait for data on the socket and return whether data is available: */
	Misc::FdSet readFds(fd);
	return Misc::pselect(&readFds,0,0,timeout)>=0&&readFds.isSet(fd);
	}

void TCPPipe::shutdown(bool read,bool write)
	{
	/* Flush the write buffer: */
	flush();
	
	/* Shut down the socket: */
	if(read&&write)
		::shutdown(fd,SHUT_RDWR);
	else if(read)
		::shutdown(fd,SHUT_RD);
	else if(write)
		::shutdown(fd,SHUT_WR);
	}

int TCPPipe::getPortId(void) const
	{
	/* Get the socket's local address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getsockname(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		throw Error("Comm::TCPPipe::getPortId: Unable to query socket address");
	
	/* Extract a numeric port ID from the socket's address: */
	char portIdBuffer[NI_MAXSERV];
	int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,0,0,portIdBuffer,sizeof(portIdBuffer),NI_NUMERICSERV);
	if(niResult!=0)
		throw Error(Misc::printStdErrMsg("Comm::TCPPipe::getPortId: Unable to retrieve port ID due to error %s",gai_strerror(niResult)));
	
	/* Convert the port ID string to a number: */
	int result=0;
	for(int i=0;i<NI_MAXSERV&&portIdBuffer[i]!='\0';++i)
		result=result*10+int(portIdBuffer[i]-'0');
	
	return result;
	}

std::string TCPPipe::getAddress(void) const
	{
	/* Get the socket's local address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getsockname(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		throw Error("Comm::TCPPipe::getAddress: Unable to query socket address");
	
	/* Extract a numeric address from the socket's address: */
	char addressBuffer[NI_MAXHOST];
	int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,addressBuffer,sizeof(addressBuffer),0,0,NI_NUMERICHOST);
	if(niResult!=0)
		throw Error(Misc::printStdErrMsg("Comm::TCPPipe::getAddress: Unable to retrieve address due to error %s",gai_strerror(niResult)));
	
	return addressBuffer;
	}

std::string TCPPipe::getHostName(void) const
	{
	/* Get the socket's local address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getsockname(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		throw Error("Comm::TCPPipe::getHostName: Unable to query socket address");
	
	/* Extract a host name from the socket's address: */
	char hostNameBuffer[NI_MAXHOST];
	int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,hostNameBuffer,sizeof(hostNameBuffer),0,0,0);
	if(niResult!=0)
		throw Error(Misc::printStdErrMsg("Comm::TCPPipe::getHostName: Unable to retrieve host name due to error %s",gai_strerror(niResult)));
	
	return hostNameBuffer;
	}

int TCPPipe::getPeerPortId(void) const
	{
	/* Get the socket's peer address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getpeername(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		throw Error("Comm::TCPPipe::getPeerPortId: Unable to query socket's peer address");
	
	/* Extract a numeric port ID from the socket's peer address: */
	char portIdBuffer[NI_MAXSERV];
	int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,0,0,portIdBuffer,sizeof(portIdBuffer),NI_NUMERICSERV);
	if(niResult!=0)
		throw Error(Misc::printStdErrMsg("Comm::TCPPipe::getPeerPortId: Unable to retrieve peer port ID due to error %s",gai_strerror(niResult)));
	
	/* Convert the port ID string to a number: */
	int result=0;
	for(int i=0;i<NI_MAXSERV&&portIdBuffer[i]!='\0';++i)
		result=result*10+int(portIdBuffer[i]-'0');
	
	return result;
	}

std::string TCPPipe::getPeerAddress(void) const
	{
	/* Get the socket's peer address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getpeername(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		throw Error("Comm::TCPPipe::getPeerAddress: Unable to query socket's peer address");
	
	/* Extract a numeric address from the socket's peer address: */
	char addressBuffer[NI_MAXHOST];
	int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,addressBuffer,sizeof(addressBuffer),0,0,NI_NUMERICHOST);
	if(niResult!=0)
		throw Error(Misc::printStdErrMsg("Comm::TCPPipe::getPeerAddress: Unable to retrieve peer address due to error %s",gai_strerror(niResult)));
	
	return addressBuffer;
	}

std::string TCPPipe::getPeerHostName(void) const
	{
	/* Get the socket's peer address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getpeername(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		throw Error("Comm::TCPPipe::getPeerHostName: Unable to query socket's peer address");
	
	/* Extract a host name from the socket's peer address: */
	char hostNameBuffer[NI_MAXHOST];
	int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,hostNameBuffer,sizeof(hostNameBuffer),0,0,0);
	if(niResult!=0)
		throw Error(Misc::printStdErrMsg("Comm::TCPPipe::getPeerHostName: Unable to retrieve peer host name due to error %s",gai_strerror(niResult)));
	
	return hostNameBuffer;
	}

}
