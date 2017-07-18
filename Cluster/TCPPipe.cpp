/***********************************************************************
TCPPipe - Pair of classes for high-performance cluster-transparent
reading/writing from/to TCP sockets.
Copyright (c) 2011-2017 Oliver Kreylos

This file is part of the Cluster Abstraction Library (Cluster).

The Cluster Abstraction Library is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Cluster Abstraction Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Cluster Abstraction Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Cluster/TCPPipe.h>

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
#include <Misc/StringMarshaller.h>
#include <Misc/FdSet.h>

namespace Cluster {

namespace {

/****************
String constants:
****************/

const char pipeGetFdErrorString[]="Cluster::TCPPipe::getFd: Cannot query file descriptor";

/****************
Helper functions:
****************/

void handleConstructionError(int errorType,int errorCode,const char* hostName,int portId)
	{
	switch(errorType)
		{
		case 1:
			throw IO::File::OpenError(Misc::printStdErrMsg("Cluster::TCPPipe::TCPPipe: Unable to resolve host name %s due to error %s",hostName,gai_strerror(errorCode)));
		
		case 2:
			throw IO::File::OpenError(Misc::printStdErrMsg("Cluster::TCPPipe::TCPPipe: Unable to connect to host %s on port %d",hostName,portId));
		
		case 3:
			throw IO::File::OpenError("Cluster::TCPPipe::TCPPipe: Unable to disable Nagle's algorithm on socket");
		}
	}

void handleReadError(int errorCode)
	{
	throw IO::File::Error(Misc::printStdErrMsg("Cluster::TCPPipe: Fatal error %d (%s) while reading from source",errorCode,strerror(errorCode)));
	}

void handleWriteError(int errorType,int errorCode)
	{
	switch(errorType)
		{
		case 1:
			throw IO::File::Error("Cluster::TCPPipe: Connection terminated by peer");
		
		case 2:
			throw IO::File::WriteError(errorCode);
		
		case 3:
			throw IO::File::Error(Misc::printStdErrMsg("Cluster::TCPPipe: Fatal error %d (%s) while writing to sink",errorCode,strerror(errorCode)));
		}
	}

void handleGetPortIdError(int errorType,int errorCode)
	{
	switch(errorType)
		{
		case 1:
			throw IO::File::Error("Cluster::TCPPipe::getPortId: Unable to query socket address");
		
		case 2:
			throw IO::File::Error(Misc::printStdErrMsg("Cluster::TCPPipe::getPortId: Unable to retrieve port ID due to error %s",gai_strerror(errorCode)));
		}
	}

void handleGetAddressError(int errorType,int errorCode)
	{
	switch(errorType)
		{
		case 1:
			throw IO::File::Error("Cluster::TCPPipe::getAddress: Unable to query socket address");
		
		case 2:
			throw IO::File::Error(Misc::printStdErrMsg("Cluster::TCPPipe::getAddress: Unable to retrieve address due to error %s",gai_strerror(errorCode)));
		}
	}

void handleGetHostNameError(int errorType,int errorCode)
	{
	switch(errorType)
		{
		case 1:
			throw IO::File::Error("Cluster::TCPPipe::getHostName: Unable to query socket address");
		
		case 2:
			throw IO::File::Error(Misc::printStdErrMsg("Cluster::TCPPipe::getHostName: Unable to retrieve host name due to error %s",gai_strerror(errorCode)));
		}
	}

void handleGetPeerPortIdError(int errorType,int errorCode)
	{
	switch(errorType)
		{
		case 1:
			throw IO::File::Error("Cluster::TCPPipe::getPeerPortId: Unable to query socket's peer address");
		
		case 2:
			throw IO::File::Error(Misc::printStdErrMsg("Cluster::TCPPipe::getPeerPortId: Unable to retrieve peer port ID due to error %s",gai_strerror(errorCode)));
		}
	}

void handleGetPeerAddressError(int errorType,int errorCode)
	{
	switch(errorType)
		{
		case 1:
			throw IO::File::Error("Cluster::TCPPipe::getPeerAddress: Unable to query socket's peer address");
		
		case 2:
			throw IO::File::Error(Misc::printStdErrMsg("Cluster::TCPPipe::getPeerAddress: Unable to retrieve peer address due to error %s",gai_strerror(errorCode)));
		}
	}

void handleGetPeerHostNameError(int errorType,int errorCode)
	{
	switch(errorType)
		{
		case 1:
			throw IO::File::Error("Cluster::TCPPipe::getPeerHostName: Unable to query socket's peer address");
		
		case 2:
			throw IO::File::Error(Misc::printStdErrMsg("Cluster::TCPPipe::getPeerHostName: Unable to retrieve peer host name due to error %s",gai_strerror(errorCode)));
		}
	}

}

/******************************
Methods of class TCPPipeMaster:
******************************/

size_t TCPPipeMaster::readData(IO::File::Byte* buffer,size_t bufferSize)
	{
	/* Read more data from source: */
	ssize_t readResult;
	do
		{
		readResult=::read(fd,buffer,bufferSize);
		}
	while(readResult<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR));
	
	/* Handle the result from the read call: */
	if(readResult>=0)
		{
		size_t readSize=size_t(readResult);
		
		if(isReadCoupled())
			{
			/* Forward the just-read data to the slaves: */
			Packet* packet=multiplexer->newPacket();
			packet->packetSize=readSize;
			memcpy(packet->packet,buffer,readSize);
			multiplexer->sendPacket(pipeId,packet);
			}
		
		return readSize;
		}
	else
		{
		int errorCode=errno;
		if(isReadCoupled())
			{
			/* Send an error indicator (empty packet followed by status packet) to the slaves: */
			Packet* packet=multiplexer->newPacket();
			packet->packetSize=0;
			multiplexer->sendPacket(pipeId,packet);
			packet=multiplexer->newPacket();
			{
			Packet::Writer writer(packet);
			writer.write<int>(errorCode);
			}
			multiplexer->sendPacket(pipeId,packet);
			}
		
		/* Throw an exception: */
		handleReadError(errorCode);
		}
	
	/* Never reached; just to make compiler happy: */
	return 0;
	}

void TCPPipeMaster::writeData(const IO::File::Byte* buffer,size_t bufferSize)
	{
	/* Collect error codes: */
	int errorType=0;
	int errorCode=0;
	
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
			errorType=2;
			errorCode=int(bufferSize);
			break;
			}
		else if(errno==EPIPE)
			{
			/* Other side hung up: */
			errorType=1;
			break;
			}
		else if(errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)
			{
			/* Unknown error; probably a bad thing: */
			errorType=3;
			errorCode=errno;
			break;
			}
		}
	
	if(isWriteCoupled())
		{
		/* Send a status packet to the slaves: */
		Packet* packet=multiplexer->newPacket();
		{
		Packet::Writer writer(packet);
		writer.write<int>(errorType);
		writer.write<int>(errorCode);
		}
		multiplexer->sendPacket(statusPipeId,packet);
		}
	
	if(errorType!=0)
		{
		/* Throw an exception: */
		handleWriteError(errorType,errorCode);
		}
	}

size_t TCPPipeMaster::writeDataUpTo(const IO::File::Byte* buffer,size_t bufferSize)
	{
	/* Collect error codes: */
	int errorType=0;
	int errorCode=0;
	size_t numBytesWritten=0;
	
	/* Write data to the sink: */
	ssize_t writeResult;
	do
		{
		writeResult=::write(fd,buffer,bufferSize);
		}
	while(writeResult<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR));
	if(writeResult>0)
		numBytesWritten=size_t(writeResult);
	else if(writeResult==0)
		{
		/* Sink has reached end-of-file: */
		errorType=2;
		errorCode=int(bufferSize);
		}
	else if(errno==EPIPE)
		{
		/* Other side hung up: */
		errorType=1;
		}
	else
		{
		/* Unknown error; probably a bad thing: */
		errorType=3;
		errorCode=errno;
		}
	
	if(isWriteCoupled())
		{
		/* Send a status packet to the slaves: */
		Packet* packet=multiplexer->newPacket();
		{
		Packet::Writer writer(packet);
		writer.write<int>(errorType);
		writer.write<int>(errorType!=0?errorCode:int(numBytesWritten));
		}
		multiplexer->sendPacket(statusPipeId,packet);
		}
	
	if(errorType!=0)
		{
		/* Throw an exception: */
		handleWriteError(errorType,errorCode);
		}
	
	return numBytesWritten;
	}

TCPPipeMaster::TCPPipeMaster(Multiplexer* sMultiplexer,const char* hostName,int portId)
	:Comm::NetPipe(WriteOnly),ClusterPipe(sMultiplexer),
	 fd(-1)
	{
	/* Convert port ID to string for getaddrinfo (awkward!): */
	if(portId<0||portId>65535)
		throw OpenError(Misc::printStdErrMsg("Cluster::TCPPipe::TCPPipe: Invalid port %d",portId));
	char portIdBuffer[6];
	char* portIdString=Misc::print(portId,portIdBuffer+5);
	
	/* Collect error indicators: */
	int errorType=0;
	int errorCode=0;
	
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
		{
		errorType=1;
		errorCode=aiResult;
		}
	
	if(errorType==0)
		{
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
			errorType=2;
		}
	
	if(errorType==0)
		{
		/* Set the TCP_NODELAY socket option: */
		int flag=1;
		if(setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,&flag,sizeof(flag))==-1)
			{
			/* Close the socket and signal an error: */
			close(fd);
			errorType=3;
			}
		}
	
	/* Send a status message to the slaves: */
	Packet* statusPacket=multiplexer->newPacket();
	{
	Packet::Writer writer(statusPacket);
	writer.write<int>(errorType);
	writer.write<int>(errorCode);
	}
	multiplexer->sendPacket(pipeId,statusPacket);
	
	if(errorType!=0)
		{
		/* Throw an exception: */
		handleConstructionError(errorType,errorCode,hostName,portId);
		}
	
	/* Open the status pipe: */
	statusPipeId=multiplexer->openPipe();
	
	/* Install a read buffer the size of a multicast packet: */
	Comm::Pipe::resizeReadBuffer(Packet::maxPacketSize);
	canReadThrough=false;
	}

TCPPipeMaster::~TCPPipeMaster(void)
	{
	/* Close the status pipe: */
	multiplexer->closePipe(statusPipeId);
	
	/* Flush the write buffer, and then close the socket: */
	flush();
	if(fd>=0)
		close(fd);
	}

int TCPPipeMaster::getFd(void) const
	{
	throw Error(pipeGetFdErrorString);
	
	/* Just to make compiler happy: */
	return -1;
	}

size_t TCPPipeMaster::resizeReadBuffer(size_t newReadBufferSize)
	{
	/* Ignore the change and return the size of a multicast packet: */
	return Packet::maxPacketSize;
	}

bool TCPPipeMaster::waitForData(void) const
	{
	/* Check if there is unread data in the buffer: */
	if(getUnreadDataSize()>0)
		return true;
	
	/* Wait for data on the socket and return whether data is available: */
	Misc::FdSet readFds(fd);
	bool result=Misc::pselect(&readFds,0,0,0)>=0&&readFds.isSet(fd);
	
	if(isReadCoupled())
		{
		/* Send a status message to the slaves: */
		Packet* statusPacket=multiplexer->newPacket();
		{
		Packet::Writer writer(statusPacket);
		writer.write<int>(result?1:0);
		}
		multiplexer->sendPacket(pipeId,statusPacket);
		}
	
	return result;
	}

bool TCPPipeMaster::waitForData(const Misc::Time& timeout) const
	{
	/* Check if there is unread data in the buffer: */
	if(getUnreadDataSize()>0)
		return true;
	
	/* Wait for data on the socket and return whether data is available: */
	Misc::FdSet readFds(fd);
	bool result=Misc::pselect(&readFds,0,0,timeout)>=0&&readFds.isSet(fd);
	
	if(isReadCoupled())
		{
		/* Send a status message to the slaves: */
		Packet* statusPacket=multiplexer->newPacket();
		{
		Packet::Writer writer(statusPacket);
		writer.write<int>(result?1:0);
		}
		multiplexer->sendPacket(pipeId,statusPacket);
		}
	
	return result;
	}

void TCPPipeMaster::shutdown(bool read,bool write)
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

int TCPPipeMaster::getPortId(void) const
	{
	/* Collect errors: */
	int errorType=0;
	int errorCode=0;
	
	/* Get the socket's local address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getsockname(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		errorType=1;
	
	int result=0;
	if(errorType==0)
		{
		/* Extract a numeric port ID from the socket's address: */
		char portIdBuffer[NI_MAXSERV];
		int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,0,0,portIdBuffer,sizeof(portIdBuffer),NI_NUMERICSERV);
		if(niResult!=0)
			{
			errorType=2;
			errorCode=niResult;
			}
		else
			{
			/* Convert the port ID string to a number: */
			for(int i=0;i<NI_MAXSERV&&portIdBuffer[i]!='\0';++i)
				result=result*10+int(portIdBuffer[i]-'0');
			}
		}
	
	if(isReadCoupled())
		{
		/* Send a status message to the slaves: */
		Packet* statusPacket=multiplexer->newPacket();
		{
		Packet::Writer writer(statusPacket);
		writer.write<int>(errorType);
		writer.write<int>(errorType!=0?errorCode:result);
		}
		multiplexer->sendPacket(statusPipeId,statusPacket);
		}
	
	if(errorType!=0)
		{
		/* Throw an exception: */
		handleGetPortIdError(errorType,errorCode);
		}
	
	return result;
	}

std::string TCPPipeMaster::getAddress(void) const
	{
	/* Collect errors: */
	int errorType=0;
	int errorCode=0;
	
	/* Get the socket's local address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getsockname(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		errorType=1;
	
	char addressBuffer[NI_MAXHOST];
	if(errorType==0)
		{
		/* Extract a numeric address from the socket's address: */
		int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,addressBuffer,sizeof(addressBuffer),0,0,NI_NUMERICHOST);
		if(niResult!=0)
			{
			errorType=2;
			errorCode=niResult;
			}
		}
	
	if(isReadCoupled())
		{
		/* Send a status message to the slaves: */
		Packet* statusPacket=multiplexer->newPacket();
		{
		Packet::Writer writer(statusPacket);
		writer.write<int>(errorType);
		if(errorType!=0)
			writer.write<int>(errorCode);
		else
			Misc::writeCString(addressBuffer,writer);
		}
		multiplexer->sendPacket(statusPipeId,statusPacket);
		}
	
	if(errorType!=0)
		{
		/* Throw an exception: */
		handleGetAddressError(errorType,errorCode);
		}
	
	return addressBuffer;
	}

std::string TCPPipeMaster::getHostName(void) const
	{
	/* Collect errors: */
	int errorType=0;
	int errorCode=0;
	
	/* Get the socket's local address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getsockname(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		errorType=1;
	
	char hostNameBuffer[NI_MAXHOST];
	if(errorType==0)
		{
		/* Extract a host name from the socket's address: */
		int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,hostNameBuffer,sizeof(hostNameBuffer),0,0,0);
		if(niResult!=0)
			{
			errorType=2;
			errorCode=niResult;
			}
		}
	
	if(isReadCoupled())
		{
		/* Send a status message to the slaves: */
		Packet* statusPacket=multiplexer->newPacket();
		{
		Packet::Writer writer(statusPacket);
		writer.write<int>(errorType);
		if(errorType!=0)
			writer.write<int>(errorCode);
		else
			Misc::writeCString(hostNameBuffer,writer);
		}
		multiplexer->sendPacket(statusPipeId,statusPacket);
		}
	
	if(errorType!=0)
		{
		/* Throw an exception: */
		handleGetHostNameError(errorType,errorCode);
		}
	
	return hostNameBuffer;
	}

int TCPPipeMaster::getPeerPortId(void) const
	{
	/* Collect errors: */
	int errorType=0;
	int errorCode=0;
	
	/* Get the socket's peer address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getpeername(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		errorType=1;
	
	int result=0;
	if(errorType==0)
		{
		/* Extract a numeric port ID from the socket's peer address: */
		char portIdBuffer[NI_MAXSERV];
		int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,0,0,portIdBuffer,sizeof(portIdBuffer),NI_NUMERICSERV);
		if(niResult!=0)
			{
			errorType=2;
			errorCode=niResult;
			}
		else
			{
			/* Convert the port ID string to a number: */
			for(int i=0;i<NI_MAXSERV&&portIdBuffer[i]!='\0';++i)
				result=result*10+int(portIdBuffer[i]-'0');
			}
		}
	
	if(isReadCoupled())
		{
		/* Send a status message to the slaves: */
		Packet* statusPacket=multiplexer->newPacket();
		{
		Packet::Writer writer(statusPacket);
		writer.write<int>(errorType);
		writer.write<int>(errorType!=0?errorCode:result);
		}
		multiplexer->sendPacket(statusPipeId,statusPacket);
		}
	
	if(errorType!=0)
		{
		/* Throw an exception: */
		handleGetPeerPortIdError(errorType,errorCode);
		}
	
	return result;
	}

std::string TCPPipeMaster::getPeerAddress(void) const
	{
	/* Collect errors: */
	int errorType=0;
	int errorCode=0;
	
	/* Get the socket's peer address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getpeername(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		errorType=1;
	
	char addressBuffer[NI_MAXHOST];
	if(errorType==0)
		{
		/* Extract a numeric address from the socket's peer address: */
		int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,addressBuffer,sizeof(addressBuffer),0,0,NI_NUMERICHOST);
		if(niResult!=0)
			{
			errorType=2;
			errorCode=niResult;
			}
		}
	
	if(isReadCoupled())
		{
		/* Send a status message to the slaves: */
		Packet* statusPacket=multiplexer->newPacket();
		{
		Packet::Writer writer(statusPacket);
		writer.write<int>(errorType);
		if(errorType!=0)
			writer.write<int>(errorCode);
		else
			Misc::writeCString(addressBuffer,writer);
		}
		multiplexer->sendPacket(statusPipeId,statusPacket);
		}
	
	if(errorType!=0)
		{
		/* Throw an exception: */
		handleGetPeerAddressError(errorType,errorCode);
		}
	
	return addressBuffer;
	}

std::string TCPPipeMaster::getPeerHostName(void) const
	{
	/* Collect errors: */
	int errorType=0;
	int errorCode=0;
	
	/* Get the socket's peer address: */
	struct sockaddr_storage socketAddress;
	socklen_t socketAddressLen=sizeof(socketAddress);
	if(getpeername(fd,reinterpret_cast<struct sockaddr*>(&socketAddress),&socketAddressLen)<0)
		errorType=1;
	
	char hostNameBuffer[NI_MAXHOST];
	if(errorType==0)
		{
		/* Extract a host name from the socket's address: */
		int niResult=getnameinfo(reinterpret_cast<const struct sockaddr*>(&socketAddress),socketAddressLen,hostNameBuffer,sizeof(hostNameBuffer),0,0,0);
		if(niResult!=0)
			{
			errorType=2;
			errorCode=niResult;
			}
		}
	
	if(isReadCoupled())
		{
		/* Send a status message to the slaves: */
		Packet* statusPacket=multiplexer->newPacket();
		{
		Packet::Writer writer(statusPacket);
		writer.write<int>(errorType);
		if(errorType!=0)
			writer.write<int>(errorCode);
		else
			Misc::writeCString(hostNameBuffer,writer);
		}
		multiplexer->sendPacket(statusPipeId,statusPacket);
		}
	
	if(errorType!=0)
		{
		/* Throw an exception: */
		handleGetPeerHostNameError(errorType,errorCode);
		}
	
	return hostNameBuffer;
	}

/*****************************
Methods of class TCPPipeSlave:
*****************************/

size_t TCPPipeSlave::readData(IO::File::Byte* buffer,size_t bufferSize)
	{
	if(isReadCoupled())
		{
		/* Receive a data packet from the master: */
		Packet* newPacket=multiplexer->receivePacket(pipeId);
		
		/* Check for error conditions: */
		if(newPacket->packetSize!=0)
			{
			/* Install the new packet as the pipe's read buffer: */
			if(packet!=0)
				multiplexer->deletePacket(packet);
			packet=newPacket;
			setReadBuffer(Packet::maxPacketSize,reinterpret_cast<Byte*>(packet->packet),false);
			
			return packet->packetSize;
			}
		else
			{
			/* Read the status packet: */
			multiplexer->deletePacket(newPacket);
			newPacket=multiplexer->receivePacket(pipeId);
			Packet::Reader reader(newPacket);
			int errorCode=reader.read<int>();
			multiplexer->deletePacket(newPacket);
			
			/* Throw an exception: */
			handleReadError(errorCode);
			}
		
		/* Never reached; just to make compiler happy: */
		return 0;
		}
	else
		{
		/* Return end-of-file; slave shouldn't have been reading in decoupled state: */
		return 0;
		}
	}

void TCPPipeSlave::writeData(const IO::File::Byte* buffer,size_t bufferSize)
	{
	if(isWriteCoupled())
		{
		/* Receive a status packet from the master: */
		Packet* statusPacket=multiplexer->receivePacket(statusPipeId);
		Packet::Reader reader(statusPacket);
		int errorType=reader.read<int>();
		int errorCode=reader.read<int>();
		multiplexer->deletePacket(statusPacket);
		
		if(errorType!=0)
			{
			/* Throw an exception: */
			handleWriteError(errorType,errorCode);
			}
		}
	}

size_t TCPPipeSlave::writeDataUpTo(const IO::File::Byte* buffer,size_t bufferSize)
	{
	if(isWriteCoupled())
		{
		/* Receive a status packet from the master: */
		Packet* statusPacket=multiplexer->receivePacket(statusPipeId);
		Packet::Reader reader(statusPacket);
		int errorType=reader.read<int>();
		int errorCode=reader.read<int>();
		multiplexer->deletePacket(statusPacket);
		
		if(errorType!=0)
			{
			/* Throw an exception: */
			handleWriteError(errorType,errorCode);
			}
		else
			return size_t(errorCode);
		}
	
	return 0;
	}

TCPPipeSlave::TCPPipeSlave(Multiplexer* sMultiplexer,const char* hostName,int portId)
	:Comm::NetPipe(WriteOnly),ClusterPipe(sMultiplexer),
	 packet(0)
	{
	/* Check the port ID: */
	if(portId<0||portId>65535)
		throw OpenError(Misc::printStdErrMsg("Comm::TCPPipe::TCPPipe: Invalid port %d",portId));
	
	/* Read the status packet from the master node: */
	Packet* statusPacket=multiplexer->receivePacket(pipeId);
	Packet::Reader reader(statusPacket);
	int errorType=reader.read<int>();
	int errorCode=reader.read<int>();
	multiplexer->deletePacket(statusPacket);
	
	if(errorType!=0)
		{
		/* Throw an exception: */
		handleConstructionError(errorType,errorCode,hostName,portId);
		}
	
	/* Open the status pipe: */
	statusPipeId=multiplexer->openPipe();
	
	/* Disable read-through: */
	canReadThrough=false;
	}

TCPPipeSlave::~TCPPipeSlave(void)
	{
	/* Close the status pipe: */
	multiplexer->closePipe(statusPipeId);
	
	/* Delete the current multicast packet: */
	if(packet!=0)
		{
		multiplexer->deletePacket(packet);
		setReadBuffer(0,0,false);
		}
	}

int TCPPipeSlave::getFd(void) const
	{
	throw Error(pipeGetFdErrorString);
	
	/* Just to make compiler happy: */
	return -1;
	}

size_t TCPPipeSlave::getReadBufferSize(void) const
	{
	/* Return the size of a multicast packet: */
	return Packet::maxPacketSize;
	}

size_t TCPPipeSlave::resizeReadBuffer(size_t newReadBufferSize)
	{
	/* Ignore the change and return the size of a multicast packet: */
	return Packet::maxPacketSize;
	}

bool TCPPipeSlave::waitForData(void) const
	{
	if(isReadCoupled())
		{
		/* Check if there is unread data in the buffer: */
		if(getUnreadDataSize()>0)
			return true;
		
		/* Read the status packet from the master node: */
		Packet* statusPacket=multiplexer->receivePacket(pipeId);
		Packet::Reader reader(statusPacket);
		int result=reader.read<int>();
		multiplexer->deletePacket(statusPacket);
		
		return result!=0;
		}
	else
		{
		/* Return no new data; slave shouldn't be reading in decoupled state: */
		return false;
		}
	}

bool TCPPipeSlave::waitForData(const Misc::Time& timeout) const
	{
	if(isReadCoupled())
		{
		/* Check if there is unread data in the buffer: */
		if(getUnreadDataSize()>0)
			return true;
		
		/* Read the status packet from the master node: */
		Packet* statusPacket=multiplexer->receivePacket(pipeId);
		Packet::Reader reader(statusPacket);
		int result=reader.read<int>();
		multiplexer->deletePacket(statusPacket);
		
		return result!=0;
		}
	else
		{
		/* Return no new data; slave shouldn't be reading in decoupled state: */
		return false;
		}
	}

void TCPPipeSlave::shutdown(bool read,bool write)
	{
	/* Do nothing */
	}

int TCPPipeSlave::getPortId(void) const
	{
	if(isReadCoupled())
		{
		/* Read the status packet from the master node: */
		Packet* statusPacket=multiplexer->receivePacket(statusPipeId);
		Packet::Reader reader(statusPacket);
		int errorType=reader.read<int>();
		int errorCode=reader.read<int>();
		multiplexer->deletePacket(statusPacket);
		
		if(errorType!=0)
			handleGetPortIdError(errorType,errorCode);
		
		return errorCode;
		}
	else
		{
		/* Return bogus port ID; slave shouldn't be reading in decoupled state: */
		return -1;
		}
	}

std::string TCPPipeSlave::getAddress(void) const
	{
	if(isReadCoupled())
		{
		/* Read the status packet from the master node: */
		Packet* statusPacket=multiplexer->receivePacket(statusPipeId);
		Packet::Reader reader(statusPacket);
		int errorType=reader.read<int>();
		int errorCode;
		std::string result;
		if(errorType!=0)
			errorCode=reader.read<int>();
		else
			result=Misc::readCppString(reader);
		multiplexer->deletePacket(statusPacket);
		
		if(errorType!=0)
			handleGetAddressError(errorType,errorCode);
		
		return result;
		}
	else
		{
		/* Return bogus address; slave shouldn't be reading in decoupled state: */
		return std::string();
		}
	}

std::string TCPPipeSlave::getHostName(void) const
	{
	if(isReadCoupled())
		{
		/* Read the status packet from the master node: */
		Packet* statusPacket=multiplexer->receivePacket(statusPipeId);
		Packet::Reader reader(statusPacket);
		int errorType=reader.read<int>();
		int errorCode;
		std::string result;
		if(errorType!=0)
			errorCode=reader.read<int>();
		else
			result=Misc::readCppString(reader);
		multiplexer->deletePacket(statusPacket);
		
		if(errorType!=0)
			handleGetHostNameError(errorType,errorCode);
		
		return result;
		}
	else
		{
		/* Return bogus host name; slave shouldn't be reading in decoupled state: */
		return std::string();
		}
	}

int TCPPipeSlave::getPeerPortId(void) const
	{
	if(isReadCoupled())
		{
		/* Read the status packet from the master node: */
		Packet* statusPacket=multiplexer->receivePacket(statusPipeId);
		Packet::Reader reader(statusPacket);
		int errorType=reader.read<int>();
		int errorCode=reader.read<int>();
		multiplexer->deletePacket(statusPacket);
		
		if(errorType!=0)
			handleGetPeerPortIdError(errorType,errorCode);
		
		return errorCode;
		}
	else
		{
		/* Return bogus port ID; slave shouldn't be reading in decoupled state: */
		return -1;
		}
	}

std::string TCPPipeSlave::getPeerAddress(void) const
	{
	if(isReadCoupled())
		{
		/* Read the status packet from the master node: */
		Packet* statusPacket=multiplexer->receivePacket(statusPipeId);
		Packet::Reader reader(statusPacket);
		int errorType=reader.read<int>();
		int errorCode;
		std::string result;
		if(errorType!=0)
			errorCode=reader.read<int>();
		else
			result=Misc::readCppString(reader);
		multiplexer->deletePacket(statusPacket);
		
		if(errorType!=0)
			handleGetPeerAddressError(errorType,errorCode);
		
		return result;
		}
	else
		{
		/* Return bogus address; slave shouldn't be reading in decoupled state: */
		return std::string();
		}
	}

std::string TCPPipeSlave::getPeerHostName(void) const
	{
	if(isReadCoupled())
		{
		/* Read the status packet from the master node: */
		Packet* statusPacket=multiplexer->receivePacket(statusPipeId);
		Packet::Reader reader(statusPacket);
		int errorType=reader.read<int>();
		int errorCode;
		std::string result;
		if(errorType!=0)
			errorCode=reader.read<int>();
		else
			result=Misc::readCppString(reader);
		multiplexer->deletePacket(statusPacket);
		
		if(errorType!=0)
			handleGetPeerHostNameError(errorType,errorCode);
		
		return result;
		}
	else
		{
		/* Return bogus host name; slave shouldn't be reading in decoupled state: */
		return std::string();
		}
	}

}
