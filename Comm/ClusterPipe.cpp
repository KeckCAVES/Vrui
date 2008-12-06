/***********************************************************************
ClusterPipe - Class layering a pipe abstraction over a TCPSocket
connected to a remote process and a MulticastPipe to forward traffic to
a cluster connected to the local process.
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
#include <Comm/ClusterPipe.h>

namespace Comm {

/****************************
Methods of class ClusterPipe:
****************************/

ClusterPipe::ClusterPipe(const TCPSocket* sSocket,MulticastPipe* sPipe)
	:socket(sSocket!=0?new TCPSocket(*sSocket):0),pipe(sPipe)
	{
	if(socket!=0)
		{
		/* Enable TCP_CORK: */
		socket->setCork(true);
		
		/* Receive a test packet from the other side to check the need for endianness conversion: */
		unsigned int testPacket;
		socket->blockingRead(&testPacket,sizeof(unsigned int));
		if(pipe!=0)
			{
			pipe->writeRaw(&testPacket,sizeof(unsigned int));
			pipe->finishMessage();
			}
		if(testPacket==0x01020304U)
			mustSwapEndianness=false;
		else if(testPacket==0x04030201U)
			mustSwapEndianness=true;
		else
			Misc::throwStdErr("ClusterPipe::ClusterPipe: Could not establish connection with host %s on port %d",socket->getPeerHostname().c_str(),socket->getPeerPortId());
		
		/* Reply to the other side: */
		testPacket=0x01020304U;
		socket->blockingWrite(&testPacket,sizeof(unsigned int));
		socket->flush();
		}
	else
		{
		/* Receive a test packet from the master node: */
		unsigned int testPacket;
		pipe->readRaw(&testPacket,sizeof(unsigned int));
		if(testPacket==0x01020304U)
			mustSwapEndianness=false;
		else if(testPacket==0x04030201U)
			mustSwapEndianness=true;
		else
			Misc::throwStdErr("ClusterPipe::ClusterPipe: Could not establish connection");
		}
	}

ClusterPipe::ClusterPipe(std::string hostname,int portId,MulticastPipe* sPipe)
	:socket(sPipe==0||sPipe->isMaster()?new TCPSocket(hostname,portId):0),pipe(sPipe)
	{
	if(socket!=0)
		{
		/* Enable TCP_CORK: */
		socket->setCork(true);
		
		/* Send a test packet to the other side to check the need for endianness conversion: */
		unsigned int testPacket=0x01020304U;
		socket->blockingWrite(&testPacket,sizeof(unsigned int));
		socket->flush();
		
		/* Receive the other side's reply: */
		socket->blockingRead(&testPacket,sizeof(unsigned int));
		if(pipe!=0)
			{
			pipe->writeRaw(&testPacket,sizeof(unsigned int));
			pipe->finishMessage();
			}
		if(testPacket==0x01020304U)
			mustSwapEndianness=false;
		else if(testPacket==0x04030201U)
			mustSwapEndianness=true;
		else
			Misc::throwStdErr("ClusterPipe::ClusterPipe: Could not establish connection with host %s on port %d",socket->getPeerHostname().c_str(),socket->getPeerPortId());
		}
	else
		{
		/* Receive a test packet from the master node: */
		unsigned int testPacket;
		pipe->readRaw(&testPacket,sizeof(unsigned int));
		if(testPacket==0x01020304U)
			mustSwapEndianness=false;
		else if(testPacket==0x04030201U)
			mustSwapEndianness=true;
		else
			Misc::throwStdErr("ClusterPipe::ClusterPipe: Could not establish connection");
		}
	}

ClusterPipe::~ClusterPipe(void)
	{
	delete socket;
	delete pipe; // The cluster pipe assumed ownership of the pipe on construction
	}

int ClusterPipe::getPeerPortId(void) const
	{
	if(socket!=0)
		return socket->getPeerPortId();
	else
		return -1;
	}

std::string ClusterPipe::getPeerAddress(void) const
	{
	if(socket!=0)
		return socket->getPeerAddress();
	else
		return "0.0.0.0";
	}

std::string ClusterPipe::getPeerHostname(void) const
	{
	if(socket!=0)
		return socket->getPeerHostname(false);
	else
		return "0.0.0.0";
	}

}
