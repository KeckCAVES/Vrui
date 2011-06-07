/***********************************************************************
MulticastPipeMultiplexer - Class to share several multicast pipes across
a single UDP socket connection.
Copyright (c) 2005-2010 Oliver Kreylos

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

#define DEBUGGING 0

#include <Comm/MulticastPipeMultiplexer.h>

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#if DEBUGGING
#include <iostream>
#endif
#include <Misc/ThrowStdErr.h>
#include <Comm/MulticastPacket.h>

namespace Comm {

namespace {

/****************
Helper functions:
****************/

inline bool isMulticast(const struct in_addr& netAddress)
	{
	int address=netAddress.s_addr;
	return address>=(0xe0<<24)&&address<(0xf0<<24);
	}

#if DEBUGGING

Misc::HashTable<unsigned int,size_t> pipeNumResentBytes(17);

#endif

}

/****************************************************************
Methods of class MulticastPipeMultiplexer::PipeState::PacketList:
****************************************************************/

MulticastPipeMultiplexer::PipeState::PacketList::PacketList(void)
	:numPackets(0),head(0),tail(0)
	{
	}

MulticastPipeMultiplexer::PipeState::PacketList::~PacketList(void)
	{
	/* Delete all packets in the queue: */
	while(head!=0)
		{
		MulticastPacket* succ=head->succ;
		delete head;
		head=succ;
		}
	}

void MulticastPipeMultiplexer::PipeState::PacketList::push_back(MulticastPacket* packet)
	{
	/* Mark the packet as the last in the list: */
	packet->succ=0;
	
	/* Append it to the end of the list: */
	if(tail!=0)
		tail->succ=packet;
	else
		head=packet;
	
	/* Update the tail pointer: */
	tail=packet;
	
	/* Increase number of packets: */
	++numPackets;
	}

MulticastPacket* MulticastPipeMultiplexer::PipeState::PacketList::pop_front(void)
	{
	/* Store pointer to first packet: */
	MulticastPacket* result=head;
	
	/* Update the head pointer: */
	head=result->succ;
	
	/* Reset the tail pointer if the list is now empty: */
	if(head==0)
		tail=0;
	
	/* Decrease number of packets: */
	--numPackets;
	
	/* Mark the result packet as singleton and return it: */
	result->succ=0;
	return result;
	}

/****************************************************
Methods of class MulticastPipeMultiplexer::PipeState:
****************************************************/

MulticastPipeMultiplexer::PipeState::PipeState(void)
	:streamPos(0),packetLossMode(false),
	 headStreamPos(0),
	 slaveStreamPosOffsets(0),numHeadSlaves(0),
	 barrierId(0),slaveBarrierIds(0),minSlaveBarrierId(0),
	 slaveGatherValues(0)
	{
	}

MulticastPipeMultiplexer::PipeState::~PipeState(void)
	{
	{
	Threads::Mutex::Lock stateLock(stateMutex);
	
	/* Destroy stream position array: */
	delete[] slaveStreamPosOffsets;
	
	/* Destroy barrier ID array: */
	delete[] slaveBarrierIds;
	
	/* Destroy slave gather value array: */
	delete[] slaveGatherValues;
	}
	}

/*****************************************
Methods of class MulticastPipeMultiplexer:
*****************************************/

void MulticastPipeMultiplexer::processAcknowledgment(MulticastPipeMultiplexer::LockedPipe& pipeState,int slaveIndex,unsigned int streamPos)
	{
	/* Check if the reported stream position points into the packet queue: */
	unsigned int streamPosOffset=streamPos-pipeState->headStreamPos;
	if(streamPosOffset>0)
		{
		/* Check if the slave had not yet acknowledged the head of the packet list: */
		if(pipeState->slaveStreamPosOffsets[slaveIndex]==0)
			{
			/* Update the slave's stream position offset: */
			pipeState->slaveStreamPosOffsets[slaveIndex]=streamPosOffset;
			
			/* Reduce the number of slaves that are still pending acknowledgment for the head of the packet list: */
			--pipeState->numHeadSlaves;
			
			/* Check if the last acknowledgment for the head of the packet list has come in: */
			if(pipeState->numHeadSlaves==0)
				{
				/* Calculate the minimal stream position offset: */
				unsigned int minStreamPosOffset=pipeState->slaveStreamPosOffsets[0];
				for(unsigned int i=1;i<numSlaves;++i)
					if(minStreamPosOffset>pipeState->slaveStreamPosOffsets[i])
						minStreamPosOffset=pipeState->slaveStreamPosOffsets[i];
				
				#if DEBUGGING2
				std::cerr<<"Attempting to discard "<<minStreamPosOffset<<" bytes from beginning of packet list"<<std::endl;
				#endif
				
				/* Discard all acknowledged packets from the head of the packet list: */
				unsigned int numDiscarded=0;
				while(!pipeState->packetList.empty()&&minStreamPosOffset>=pipeState->packetList.front()->packetSize)
					{
					/* Discard the first packet: */
					MulticastPacket* front=pipeState->packetList.pop_front();
					numDiscarded+=front->packetSize;
					minStreamPosOffset-=front->packetSize;
					deletePacket(front);
					}
				
				#if DEBUGGING2
				std::cerr<<"Discarded "<<numDiscarded<<" bytes from beginning of packet list"<<std::endl;
				#endif
				
				/* Update the stream position of the head of the packet list: */
				pipeState->headStreamPos+=numDiscarded;
				
				/* Update all slaves' stream position offsets: */
				for(unsigned int i=0;i<numSlaves;++i)
					{
					pipeState->slaveStreamPosOffsets[i]-=numDiscarded;
					if(pipeState->slaveStreamPosOffsets[i]==0)
						++pipeState->numHeadSlaves;
					}
				
				/* Wake up any callers that might be blocking on a full send queue: */
				pipeState->receiveCond.broadcast();
				}
			}
		else
			{
			/* Remember the slave's stream position offset: */
			pipeState->slaveStreamPosOffsets[slaveIndex]=streamPosOffset;
			}
		}
	}

void* MulticastPipeMultiplexer::packetHandlingThreadMaster(void)
	{
	Threads::Thread::setCancelState(Threads::Thread::CANCEL_ENABLE);
	Threads::Thread::setCancelType(Threads::Thread::CANCEL_ASYNCHRONOUS);
	
	/* Handle message exchange during multiplexer initialization: */
	bool* slaveConnecteds=new bool[numSlaves];
	for(unsigned int i=0;i<numSlaves;++i)
		slaveConnecteds[i]=false;
	unsigned int numConnectedSlaves=0;
	while(numConnectedSlaves<numSlaves)
		{
		/* Wait for a connection initialization packet: */
		SlaveMessage msg;
		ssize_t numBytesReceived=recv(socketFd,&msg,sizeof(SlaveMessage),0);
		if(numBytesReceived==sizeof(SlaveMessage))
			{
			unsigned int slaveIndex=msg.nodeIndex-1;
			if(msg.messageId==SlaveMessage::CONNECTION&&slaveIndex<numSlaves&&!slaveConnecteds[slaveIndex])
				{
				/* Mark the slave as connected: */
				slaveConnecteds[slaveIndex]=true;
				++numConnectedSlaves;
				}
			}
		}
	delete[] slaveConnecteds;
	
	/* Send connection message to slaves: */
	MasterMessage msg(MasterMessage::CONNECTION);
	{
	Threads::Mutex::Lock socketLock(socketMutex);
	for(int i=0;i<masterMessageBurstSize;++i)
		sendto(socketFd,&msg,sizeof(MasterMessage),0,(const sockaddr*)otherAddress,sizeof(sockaddr_in));
	}
	
	/* Signal connection establishment: */
	{
	Threads::MutexCond::Lock connectionCondLock(connectionCond);
	connected=true;
	connectionCond.broadcast(connectionCondLock);
	}
	
	/* Handle messages from the slaves: */
	while(true)
		{
		/* Wait for a slave message: */
		SlaveMessage msg;
		ssize_t numBytesReceived=recv(socketFd,&msg,sizeof(SlaveMessage),0);
		if(numBytesReceived==sizeof(SlaveMessage))
			{
			switch(msg.messageId)
				{
				case SlaveMessage::CONNECTION:
					{
					/* One slave must have missed the connection establishment packet; send another one: */
					MasterMessage msg(MasterMessage::CONNECTION);
					{
					Threads::Mutex::Lock socketLock(socketMutex);
					sendto(socketFd,&msg,sizeof(MasterMessage),0,(const sockaddr*)otherAddress,sizeof(sockaddr_in));
					}
					break;
					}
				
				case SlaveMessage::PING:
					{
					/* Broadcast a ping reply to all slaves: */
					MasterMessage msg(MasterMessage::PING);
					{
					Threads::Mutex::Lock socketLock(socketMutex);
					sendto(socketFd,&msg,sizeof(MasterMessage),0,(const sockaddr*)otherAddress,sizeof(sockaddr_in));
					}
					break;
					}
				
				case SlaveMessage::CREATEPIPE:
					{
					/* Get a handle on the state object of the pipe the packet is meant for: */
					LockedPipe pipeState(pipeStateTable,pipeStateTableMutex,msg.pipeId);
					
					/* If the pipe has already been created on the master, treat this like a barrier; otherwise, ignore: */
					if(pipeState.isValid())
						{
						/* Update the barrier ID array: */
						if(pipeState->barrierId>=1)
							{
							/* One slave must have missed a pipe creation completion message; send another one: */
							MasterMessage msg2(MasterMessage::CREATEPIPE);
							msg2.pipeId=msg.pipeId;
							{
							Threads::Mutex::Lock socketLock(socketMutex);
							sendto(socketFd,&msg2,sizeof(MasterMessage),0,(const sockaddr*)otherAddress,sizeof(sockaddr_in));
							}
							}
						else
							{
							pipeState->slaveBarrierIds[msg.nodeIndex-1]=1;
							
							/* Check if the current barrier is complete: */
							pipeState->minSlaveBarrierId=pipeState->slaveBarrierIds[0];
							for(unsigned int i=1;i<numSlaves;++i)
								if(pipeState->minSlaveBarrierId>pipeState->slaveBarrierIds[i])
									pipeState->minSlaveBarrierId=pipeState->slaveBarrierIds[i];
							if(pipeState->minSlaveBarrierId>0)
								{
								/* Wake up thread waiting on barrier: */
								pipeState->barrierCond.broadcast();
								}
							}
						}
					break;
					}
				
				case SlaveMessage::ACKNOWLEDGMENT:
					{
					/* Get a handle on the state object of the pipe the packet is meant for: */
					LockedPipe pipeState(pipeStateTable,pipeStateTableMutex,msg.pipeId);
					
					if(pipeState.isValid())
						{
						/* Process the acknowledgment packet: */
						processAcknowledgment(pipeState,msg.nodeIndex-1,msg.streamPos);
						}
					break;
					}
				
				case SlaveMessage::PACKETLOSS:
					{
					/* Get a handle on the state object of the pipe the packet is meant for: */
					LockedPipe pipeState(pipeStateTable,pipeStateTableMutex,msg.pipeId);
					
					if(pipeState.isValid())
						{
						/* Use the stream position reported by the client as positive acknowledgment: */
						processAcknowledgment(pipeState,msg.nodeIndex-1,msg.streamPos);
						
						//std::cerr<<"Node "<<msg.nodeIndex<<": Packet loss, "<<pipeState->streamPos-msg.streamPos<<" bytes, "<<(pipeState->streamPos-msg.streamPos+1463)/1464<<" packets"<<std::endl;
						//std::cerr<<"Packet loss of "<<msg.packetPos-msg.streamPos<<" bytes from "<<msg.streamPos<<" detected by node "<<msg.nodeIndex<<", stream pos is "<<pipeState->streamPos<<", buffer starts at "<<pipeState->headStreamPos<<std::endl;
						
						/* Do nothing if there is no more data to send (i.e., master is busy): */
						if(msg.streamPos<pipeState->streamPos)
							{
							/* Find the first recently sent packet after the slave's current stream position: */
							MulticastPacket* packet;
							for(packet=pipeState->packetList.front();packet!=0&&packet->streamPos<msg.streamPos;packet=packet->succ)
								;
							
							/* Signal a fatal error if the required packet has already been discarded: */
							if(packet->streamPos!=msg.streamPos)
								Misc::throwStdErr("MulticastPipeMultiplexer: Fatal packet loss detected by %u bytes",packet->streamPos-msg.streamPos);
							
							{
							/* Resend all recent packets in order: */
							Threads::Mutex::Lock socketLock(socketMutex);
							for(;packet!=0;packet=packet->succ)
								{
								sendto(socketFd,&packet->pipeId,packet->packetSize+2*sizeof(unsigned int),0,(const sockaddr*)otherAddress,sizeof(sockaddr_in));
								#if DEBUGGING
								pipeNumResentBytes.getEntry(msg.pipeId).getDest()+=packet->packetSize;
								#endif
								}
							}
							}
						}
					break;
					}
				
				case SlaveMessage::BARRIER:
					{
					/* Get a handle on the state object of the pipe the packet is meant for: */
					LockedPipe pipeState(pipeStateTable,pipeStateTableMutex,msg.pipeId);
					
					if(pipeState.isValid())
						{
						/* Update the barrier ID array: */
						if(msg.barrierId<=pipeState->barrierId)
							{
							/* One slave must have missed a barrier completion message; send another one: */
							MasterMessage msg2(MasterMessage::BARRIER);
							msg2.pipeId=msg.pipeId;
							msg2.barrierId=msg.barrierId;
							{
							Threads::Mutex::Lock socketLock(socketMutex);
							sendto(socketFd,&msg2,sizeof(MasterMessage),0,(const sockaddr*)otherAddress,sizeof(sockaddr_in));
							}
							}
						else
							{
							pipeState->slaveBarrierIds[msg.nodeIndex-1]=msg.barrierId;
							
							/* Check if the current barrier is complete: */
							pipeState->minSlaveBarrierId=pipeState->slaveBarrierIds[0];
							for(unsigned int i=1;i<numSlaves;++i)
								if(pipeState->minSlaveBarrierId>pipeState->slaveBarrierIds[i])
									pipeState->minSlaveBarrierId=pipeState->slaveBarrierIds[i];
							if(pipeState->minSlaveBarrierId>pipeState->barrierId)
								{
								/* Wake up thread waiting on barrier: */
								pipeState->barrierCond.broadcast();
								}
							}
						}
					break;
					}
				
				case SlaveMessage::GATHER:
					{
					/* Get a handle on the state object of the pipe the packet is meant for: */
					LockedPipe pipeState(pipeStateTable,pipeStateTableMutex,msg.pipeId);
					
					if(pipeState.isValid())
						{
						/* Update the barrier ID array: */
						if(msg.barrierId<=pipeState->barrierId)
							{
							/* One slave must have missed a gather completion message; send another one: */
							MasterMessage msg2(MasterMessage::GATHER);
							msg2.pipeId=msg.pipeId;
							msg2.barrierId=msg.barrierId;
							msg2.masterValue=pipeState->masterGatherValue;
							{
							Threads::Mutex::Lock socketLock(socketMutex);
							sendto(socketFd,&msg2,sizeof(MasterMessage),0,(const sockaddr*)otherAddress,sizeof(sockaddr_in));
							}
							}
						else
							{
							pipeState->slaveBarrierIds[msg.nodeIndex-1]=msg.barrierId;
							pipeState->slaveGatherValues[msg.nodeIndex-1]=msg.slaveValue;
							
							/* Check if the current gather operation is complete: */
							pipeState->minSlaveBarrierId=pipeState->slaveBarrierIds[0];
							for(unsigned int i=1;i<numSlaves;++i)
								if(pipeState->minSlaveBarrierId>pipeState->slaveBarrierIds[i])
									pipeState->minSlaveBarrierId=pipeState->slaveBarrierIds[i];
							if(pipeState->minSlaveBarrierId>pipeState->barrierId)
								{
								/* Wake up thread waiting on barrier: */
								pipeState->barrierCond.broadcast();
								}
							}
						}
					break;
					}
				}
			}
		}
	
	return 0;
	}

void* MulticastPipeMultiplexer::packetHandlingThreadSlave(void)
	{
	Threads::Thread::setCancelState(Threads::Thread::CANCEL_ENABLE);
	Threads::Thread::setCancelType(Threads::Thread::CANCEL_ASYNCHRONOUS);
	
	/* Keep sending connection initiation packets to the master until connection is established: */
	while(true)
		{
		/* Send connection initiation packet to master: */
		SlaveMessage msg;
		msg.nodeIndex=nodeIndex;
		msg.messageId=SlaveMessage::CONNECTION;
		{
		Threads::Mutex::Lock socketLock(socketMutex);
		for(int i=0;i<slaveMessageBurstSize;++i)
			sendto(socketFd,&msg,sizeof(SlaveMessage),0,(const sockaddr*)otherAddress,sizeof(struct sockaddr_in));
		}
		
		/* Wait for a connection packet from the master (but don't wait for too long): */
		fd_set readFdSet;
		FD_ZERO(&readFdSet);
		FD_SET(socketFd,&readFdSet);
		struct timeval timeout=connectionWaitTimeout;
		if(select(socketFd+1,&readFdSet,0,0,&timeout)>=0&&FD_ISSET(socketFd,&readFdSet))
			break;
		}
	
	unsigned int sendAckIn=nodeIndex-1;
	
	/* Handle messages from the master: */
	while(true)
		{
		/* Wait for the next packet, and request a ping packet if no data arrives during the timeout: */
		bool havePacket=false;
		for(int i=0;i<maxPingRequests&&!havePacket;++i)
			{
			/* Wait until the "silence period" is over: */
			fd_set readFdSet;
			FD_ZERO(&readFdSet);
			FD_SET(socketFd,&readFdSet);
			struct timeval timeout=pingTimeout;
			if(select(socketFd+1,&readFdSet,0,0,&timeout)>=0&&FD_ISSET(socketFd,&readFdSet))
				havePacket=true;
			else
				{
				/* Send a ping request packet: */
				SlaveMessage msg;
				msg.nodeIndex=nodeIndex;
				msg.messageId=SlaveMessage::PING;
				{
				Threads::Mutex::Lock socketLock(socketMutex);
				for(int i=0;i<slaveMessageBurstSize;++i)
					sendto(socketFd,&msg,sizeof(SlaveMessage),0,(const sockaddr*)otherAddress,sizeof(struct sockaddr_in));
				}
				}
			}
		if(!havePacket)
			{
			/* Signal an error: */
			Misc::throwStdErr("MulticastPipeMultiplexer: Communication error");
			}
		
		/* Read the waiting packet: */
		ssize_t numBytesReceived=recv(socketFd,&slaveThreadPacket->pipeId,MulticastPacket::maxPacketSize+2*sizeof(unsigned int),0);
		if(numBytesReceived<0)
			{
			/* Consider this a fatal error: */
			Misc::throwStdErr("MulticastPipeMultiplexer: Fatal error during receive");
			}
		slaveThreadPacket->packetSize=size_t(numBytesReceived-2*sizeof(unsigned int));
		
		if(slaveThreadPacket->pipeId==0)
			{
			/* It's a message for the pipe multiplexer itself: */
			MasterMessage* msg=reinterpret_cast<MasterMessage*>(&slaveThreadPacket->pipeId);
			switch(msg->messageId)
				{
				case MasterMessage::CONNECTION:
					/* Signal connection establishment: */
					{
					Threads::MutexCond::Lock connectionCondLock(connectionCond);
					if(!connected)
						{
						connected=true;
						connectionCond.broadcast(connectionCondLock);
						}
					}
					break;
				
				case MasterMessage::PING:
					/* Just ignore the packet... */
					break;
				
				case MasterMessage::CREATEPIPE:
					{
					/* Get a handle on the state object of the pipe the packet is meant for: */
					LockedPipe pipeState(pipeStateTable,pipeStateTableMutex,msg->pipeId);
					
					if(pipeState.isValid())
						{
						/* Signal barrier completion: */
						if(pipeState->barrierId==0)
							pipeState->barrierCond.broadcast();
						}
					break;
					}
				
				case MasterMessage::BARRIER:
					{
					/* Get a handle on the state object of the pipe the packet is meant for: */
					LockedPipe pipeState(pipeStateTable,pipeStateTableMutex,msg->pipeId);
					
					if(pipeState.isValid())
						{
						/* Signal barrier completion if the completion message is for the current barrier: */
						if(msg->barrierId>pipeState->barrierId)
							pipeState->barrierCond.broadcast();
						}
					break;
					}
				
				case MasterMessage::GATHER:
					{
					/* Get a handle on the state object of the pipe the packet is meant for: */
					LockedPipe pipeState(pipeStateTable,pipeStateTableMutex,msg->pipeId);
					
					if(pipeState.isValid())
						{
						/* Signal barrier completion if the completion message is for the current barrier: */
						if(msg->barrierId>pipeState->barrierId)
							{
							pipeState->masterGatherValue=msg->masterValue;
							pipeState->barrierCond.broadcast();
							}
						}
					break;
					}
				}
			}
		else
			{
			/* Get a handle on the state object of the pipe the packet is meant for: */
			LockedPipe pipeState(pipeStateTable,pipeStateTableMutex,slaveThreadPacket->pipeId);
			
			if(pipeState.isValid())
				{
				/* Check if all previous data has been received by the pipe: */
				if(pipeState->streamPos!=slaveThreadPacket->streamPos)
					{
					if(pipeState->streamPos<slaveThreadPacket->streamPos&&!pipeState->packetLossMode)
						{
						/* At least one packet must have been lost; send negative acknowledgment to the master: */
						SlaveMessage msg;
						msg.nodeIndex=nodeIndex;
						msg.messageId=SlaveMessage::PACKETLOSS;
						msg.pipeId=slaveThreadPacket->pipeId;
						msg.streamPos=pipeState->streamPos;
						msg.packetPos=slaveThreadPacket->streamPos;
						{
						Threads::Mutex::Lock socketLock(socketMutex);
						for(int i=0;i<slaveMessageBurstSize;++i)
							sendto(socketFd,&msg,sizeof(SlaveMessage),0,(const sockaddr*)otherAddress,sizeof(struct sockaddr_in));
						}
						
						/* Enable packet loss mode to prohibit sending further loss messages until the missing packet arrives: */
						pipeState->packetLossMode=true;
						}
					}
				else
					{
					/* Disable packet loss mode: */
					pipeState->packetLossMode=false;
					
					++sendAckIn;
					if(sendAckIn==numSlaves)
						{
						/* Send positive acknowledgment to the master: */
						SlaveMessage msg;
						msg.nodeIndex=nodeIndex;
						msg.messageId=SlaveMessage::ACKNOWLEDGMENT;
						msg.pipeId=slaveThreadPacket->pipeId;
						msg.streamPos=pipeState->streamPos;
						msg.packetPos=slaveThreadPacket->streamPos;
						{
						Threads::Mutex::Lock socketLock(socketMutex);
						sendto(socketFd,&msg,sizeof(SlaveMessage),0,(const sockaddr*)otherAddress,sizeof(struct sockaddr_in));
						}
						sendAckIn=0;
						}
					
					/* Wake up sleeping receivers if the delivery queue is currently empty: */
					if(pipeState->packetList.empty())
						pipeState->receiveCond.signal();

					/* Append the packet to the pipe state's delivery queue: */
					pipeState->streamPos+=slaveThreadPacket->packetSize;
					pipeState->packetList.push_back(slaveThreadPacket);
					
					/* Get a new packet: */
					slaveThreadPacket=newPacket();
					}
				}
			}
		}
	
	return 0;
	}

MulticastPipeMultiplexer::MulticastPipeMultiplexer(unsigned int sNumSlaves,unsigned int sNodeIndex,std::string masterHostName,int masterPortNumber,std::string slaveMulticastGroup,int slavePortNumber)
	:numSlaves(sNumSlaves),nodeIndex(sNodeIndex),
	 otherAddress(new sockaddr_in),
	 socketFd(0),
	 connected(false),
	 nextPipeId(1),
	 pipeStateTable(17),
	 slaveThreadPacket(0),
	 masterMessageBurstSize(1),slaveMessageBurstSize(1),
	 connectionWaitTimeout(0.5),
	 pingTimeout(10.0),maxPingRequests(3),
	 receiveWaitTimeout(0.25),
	 barrierWaitTimeout(0.1),
	 sendBufferSize(50),
	 packetPoolHead(0)
	{
	/* Lookup master's IP address: */
	struct hostent* masterEntry=gethostbyname(masterHostName.c_str());
	if(masterEntry==0)
		{
		close(socketFd);
		Misc::throwStdErr("MulticastPipeMultiplexer: Unable to resolve master %s",masterHostName.c_str());
		}
	struct in_addr masterNetAddress;
	masterNetAddress.s_addr=ntohl(((struct in_addr*)masterEntry->h_addr_list[0])->s_addr);
	
	/* Lookup slave multicast group's IP address: */
	struct hostent* slaveEntry=gethostbyname(slaveMulticastGroup.c_str());
	if(slaveEntry==0)
		{
		close(socketFd);
		Misc::throwStdErr("MulticastPipeMultiplexer: Unable to resolve slave multicast group %s",slaveMulticastGroup.c_str());
		}
	struct in_addr slaveNetAddress;
	slaveNetAddress.s_addr=ntohl(((struct in_addr*)slaveEntry->h_addr_list[0])->s_addr);
	
	/* Create a UDP socket: */
	socketFd=socket(PF_INET,SOCK_DGRAM,0);
	if(socketFd<0)
		Misc::throwStdErr("MulticastPipeMultiplexer: Unable to create socket");
	
	/* Bind the socket to the local address/port number: */
	int localPortNumber=nodeIndex==0?masterPortNumber:slavePortNumber;
	struct sockaddr_in socketAddress;
	socketAddress.sin_family=AF_INET;
	socketAddress.sin_port=htons(localPortNumber);
	socketAddress.sin_addr.s_addr=htonl(INADDR_ANY);
	if(bind(socketFd,(struct sockaddr*)&socketAddress,sizeof(struct sockaddr_in))==-1)
		{
		close(socketFd);
		Misc::throwStdErr("MulticastPipeMultiplexer: Unable to bind socket to port number %d",localPortNumber);
		}
	
	if(!isMulticast(slaveNetAddress))
		{
		/* Enable broadcast handling for the socket: */
		int broadcastFlag=1;
		setsockopt(socketFd,SOL_SOCKET,SO_BROADCAST,&broadcastFlag,sizeof(int));
		}
	
	/* Connect the socket to the other end: */
	if(nodeIndex==0)
		{
		if(isMulticast(slaveNetAddress))
			{
			/* Set the outgoing network interface for the slave multicast group: */
			struct in_addr multicastInterfaceAddress;
			multicastInterfaceAddress.s_addr=htonl(masterNetAddress.s_addr);
			if(setsockopt(socketFd,IPPROTO_IP,IP_MULTICAST_IF,&multicastInterfaceAddress,sizeof(struct in_addr))<0)
				{
				int myerrno=errno;
				close(socketFd);
				Misc::throwStdErr("MulticastMultiplexer::MulticastMultiplexer: Node %u: error %s during setsockopt",nodeIndex,strerror(myerrno));
				}
			}
		
		/* Store the slaves' address: */
		memset(otherAddress,0,sizeof(sockaddr_in));
		otherAddress->sin_family=AF_INET;
		otherAddress->sin_port=htons(slavePortNumber);
		otherAddress->sin_addr.s_addr=htonl(slaveNetAddress.s_addr);
		}
	else
		{
		if(isMulticast(slaveNetAddress))
			{
			/* Join the slave multicast group: */
			struct ip_mreq addGroupRequest;
			addGroupRequest.imr_multiaddr.s_addr=htonl(slaveNetAddress.s_addr);
			addGroupRequest.imr_interface.s_addr=htonl(INADDR_ANY);
			if(setsockopt(socketFd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&addGroupRequest,sizeof(struct ip_mreq))<0)
				{
				int myerrno=errno;
				close(socketFd);
				Misc::throwStdErr("MulticastMultiplexer::MulticastMultiplexer: Node %u: error %s during setsockopt",nodeIndex,strerror(myerrno));
				}
			}
		
		/* Store the master's address: */
		otherAddress->sin_family=AF_INET;
		otherAddress->sin_port=htons(masterPortNumber);
		otherAddress->sin_addr.s_addr=htonl(masterNetAddress.s_addr);
		}
	
	/* Create the packet handling thread: */
	if(nodeIndex==0)
		packetHandlingThread.start(this,&MulticastPipeMultiplexer::packetHandlingThreadMaster);
	else
		{
		slaveThreadPacket=newPacket();
		packetHandlingThread.start(this,&MulticastPipeMultiplexer::packetHandlingThreadSlave);
		}
	}

MulticastPipeMultiplexer::~MulticastPipeMultiplexer(void)
	{
	/* Stop the packet handling thread: */
	packetHandlingThread.cancel();
	packetHandlingThread.join();
	
	/* Delete the packet handling thread's receive packet: */
	if(slaveThreadPacket!=0)
		delete slaveThreadPacket;
	
	/* Close all leftover pipes: */
	for(PipeHasher::Iterator psIt=pipeStateTable.begin();psIt!=pipeStateTable.end();++psIt)
		delete psIt->getDest();
	
	/* Close the UDP socket: */
	close(socketFd);
	
	/* Delete address of multicast connection's other end: */
	delete otherAddress;
	
	/* Delete all multicast packets in the packet pool: */
	while(packetPoolHead!=0)
		{
		MulticastPacket* succ=packetPoolHead->succ;
		delete packetPoolHead;
		packetPoolHead=succ;
		}
	}

int MulticastPipeMultiplexer::getLocalPortNumber(void) const
	{
	/* Query the communication socket's bound address: */
	struct sockaddr_in socketAddress;
	#ifdef __SGI_IRIX__
	int socketAddressLen=sizeof(struct sockaddr_in);
	#else
	socklen_t socketAddressLen=sizeof(struct sockaddr_in);
	#endif
	getsockname(socketFd,(struct sockaddr*)&socketAddress,&socketAddressLen);
	
	/* Return the socket's port number: */
	return ntohs(socketAddress.sin_port);
	}

void MulticastPipeMultiplexer::setConnectionWaitTimeout(Misc::Time newConnectionWaitTimeout)
	{
	connectionWaitTimeout=newConnectionWaitTimeout;
	}

void MulticastPipeMultiplexer::setPingTimeout(Misc::Time newPingTimeout,int newMaxPingRequests)
	{
	pingTimeout=newPingTimeout;
	maxPingRequests=newMaxPingRequests;
	if(maxPingRequests<2) // Need at least two
		maxPingRequests=2;
	}

void MulticastPipeMultiplexer::setReceiveWaitTimeout(Misc::Time newReceiveWaitTimeout)
	{
	receiveWaitTimeout=newReceiveWaitTimeout;
	}

void MulticastPipeMultiplexer::setBarrierWaitTimeout(Misc::Time newBarrierWaitTimeout)
	{
	barrierWaitTimeout=newBarrierWaitTimeout;
	}

void MulticastPipeMultiplexer::waitForConnection(void)
	{
	{
	Threads::MutexCond::Lock connectionCondLock(connectionCond);
	while(!connected)
		{
		/* Sleep until connection is established: */
		connectionCond.wait(connectionCondLock);
		}
	}
	}

unsigned int MulticastPipeMultiplexer::openPipe(void)
	{
	/* Add new pipe state to the pipe state table: */
	unsigned int newPipeId;
	PipeState* newPipeState;
	{
	Threads::Mutex::Lock pipeStateTableLock(pipeStateTableMutex);
	newPipeId=nextPipeId;
	++nextPipeId;
	newPipeState=new PipeState;
	if(nodeIndex==0)
		{
		/* Initialize the slave stream position offset array: */
		newPipeState->slaveStreamPosOffsets=new unsigned int[numSlaves];
		for(unsigned int i=0;i<numSlaves;++i)
			newPipeState->slaveStreamPosOffsets[i]=0;
		newPipeState->numHeadSlaves=numSlaves;
		
		/* Initialize the slave barrier ID array: */
		newPipeState->slaveBarrierIds=new unsigned int[numSlaves];
		for(unsigned int i=0;i<numSlaves;++i)
			newPipeState->slaveBarrierIds[i]=0;
		
		/* Initialize the slave gather value array: */
		newPipeState->slaveGatherValues=new unsigned int[numSlaves];
		for(unsigned int i=0;i<numSlaves;++i)
			newPipeState->slaveBarrierIds[i]=0;
		}
	pipeStateTable.setEntry(PipeHasher::Entry(newPipeId,newPipeState));
	}
	
	#if DEBUGGING
	if(nodeIndex==0)
		std::cerr<<"Opening pipe "<<newPipeId<<std::endl;
	
	/* Create a resent byte counter entry for this pipe: */
	pipeNumResentBytes.setEntry(Misc::HashTable<unsigned int,size_t>::Entry(newPipeId,0));
	#endif
	
	/* Synchronize until all nodes have created the new pipe: */
	{
	Threads::Mutex::Lock pipeStateLock(newPipeState->stateMutex);
	
	if(nodeIndex==0)
		{
		/* Wait until barrier messages from all slaves have been received: */
		while(newPipeState->minSlaveBarrierId==0)
			{
			/* Wait until the next barrier message: */
			newPipeState->barrierCond.wait(newPipeState->stateMutex);
			}
		
		/* Send pipe creation completion message to all slaves: */
		MasterMessage msg(MasterMessage::CREATEPIPE);
		msg.pipeId=newPipeId;
		{
		Threads::Mutex::Lock socketLock(socketMutex);
		for(int i=0;i<masterMessageBurstSize;++i)
			sendto(socketFd,&msg,sizeof(MasterMessage),0,(const sockaddr*)otherAddress,sizeof(sockaddr_in));
		}
		}
	else
		{
		/* Continue sending pipe creation messages to master until pipe creation completion message is received: */
		Misc::Time waitTimeout=Misc::Time::now();
		while(true)
			{
			/* Send pipe creation message to master: */
			SlaveMessage msg;
			msg.nodeIndex=nodeIndex;
			msg.messageId=SlaveMessage::CREATEPIPE;
			msg.pipeId=newPipeId;
			{
			Threads::Mutex::Lock socketLock(socketMutex);
			for(int i=0;i<slaveMessageBurstSize;++i)
				sendto(socketFd,&msg,sizeof(SlaveMessage),0,(const sockaddr*)otherAddress,sizeof(struct sockaddr_in));
			}
			
			/* Wait for arrival of pipe creation completion message: */
			waitTimeout+=barrierWaitTimeout;
			if(newPipeState->barrierCond.timedWait(newPipeState->stateMutex,waitTimeout))
				break;
			}
		}
	
	/* Mark the pipe as created: */
	newPipeState->barrierId=1;
	}
	
	/* Return new pipe's ID: */
	return newPipeId;
	}

void MulticastPipeMultiplexer::closePipe(unsigned int pipeId)
	{
	/* Execute a barrier to synchronize and flush the pipe before closing it: */
	barrier(pipeId);
	
	/* Remove the pipe's state from the state table: */
	PipeState* pipeState;
	{
	Threads::Mutex::Lock pipeStateTableLock(pipeStateTableMutex);
	PipeHasher::Iterator psIt=pipeStateTable.findEntry(pipeId);
	if(psIt.isFinished())
		Misc::throwStdErr("MulticastPipeMultiplexer: Attempt to close already-closed pipe");
	pipeState=psIt->getDest();
	pipeStateTable.removeEntry(psIt);
	}
	
	#if DEBUGGING
	if(nodeIndex==0)
		{
		std::cerr<<"Closing pipe "<<pipeId;
		std::cerr<<". Total number of resent bytes: "<<pipeNumResentBytes.getEntry(pipeId).getDest()<<std::endl;
		}
	#endif
	
	/* Add all packets in the list to the list of free packets: */
	if(pipeState->packetList.numPackets>0)
		{
		{
		Threads::Mutex::Lock packetPoolLock(packetPoolMutex);
		pipeState->packetList.tail->succ=packetPoolHead;
		packetPoolHead=pipeState->packetList.head;
		pipeState->packetList.numPackets=0;
		pipeState->packetList.head=0;
		pipeState->packetList.tail=0;
		}
		}
	
	/* Destroy the pipe state: */
	delete pipeState;
	}

void MulticastPipeMultiplexer::sendPacket(unsigned int pipeId,MulticastPacket* packet)
	{
	/* Get a handle on the state object for the given pipe: */
	LockedPipe pipeState(pipeStateTable,pipeStateTableMutex,pipeId);
	if(!pipeState.isValid())
		Misc::throwStdErr("MulticastPipeMultiplexer: Attempt to write to closed pipe");
	
	/* Block until there's room in the send queue, and append the packet: */
	packet->pipeId=pipeId;
	
	/* Block if the pipe's send queue is full: */
	#if DEBUGGING
	bool amBlocking=pipeState->packetList.size()==sendBufferSize;
	if(amBlocking)
		std::cerr<<"Blocking on full send buffer"<<std::endl;
	#endif
	
	while(pipeState->packetList.size()==sendBufferSize)
		pipeState->receiveCond.wait(pipeState->stateMutex);
	
	#if DEBUGGING
	if(amBlocking)
		std::cerr<<"Woke up after blocking on full send buffer"<<std::endl;
	#endif
	
	/* Append the packet to the "recently sent" list: */
	packet->streamPos=pipeState->streamPos;
	pipeState->streamPos+=packet->packetSize;
	pipeState->packetList.push_back(packet);
	
	/* It's safe to unlock the pipe state now: */
	pipeState.unlock();
	
	/* Send the packet across the UDP connection: */
	{
	Threads::Mutex::Lock socketLock(socketMutex);
	sendto(socketFd,&packet->pipeId,packet->packetSize+2*sizeof(unsigned int),0,(const sockaddr*)otherAddress,sizeof(sockaddr_in));
	}
	}

MulticastPacket* MulticastPipeMultiplexer::receivePacket(unsigned int pipeId)
	{
	/* Get a handle on the state object for the given pipe: */
	LockedPipe pipeState(pipeStateTable,pipeStateTableMutex,pipeId);
	if(!pipeState.isValid())
		Misc::throwStdErr("MulticastPipeMultiplexer: Attempt to read from closed pipe");
	
	/* Sleep while there are no packets in the delivery queue: */
	Misc::Time waitTimeout=Misc::Time::now();
	while(pipeState->packetList.empty())
		{
		/* Wait for arrival of the next packet: */
		waitTimeout+=receiveWaitTimeout;
		if(!pipeState->receiveCond.timedWait(pipeState->stateMutex,waitTimeout))
			{
			/* Send a packet loss message to the master, just to be sure: */
			SlaveMessage msg;
			msg.nodeIndex=nodeIndex;
			msg.messageId=SlaveMessage::PACKETLOSS;
			msg.pipeId=pipeId;
			msg.streamPos=pipeState->streamPos;
			msg.packetPos=pipeState->streamPos;
			{
			Threads::Mutex::Lock socketLock(socketMutex);
			for(int i=0;i<slaveMessageBurstSize;++i)
				sendto(socketFd,&msg,sizeof(SlaveMessage),0,(const sockaddr*)otherAddress,sizeof(struct sockaddr_in));
			}
			}
		}
	
	/* Remove the first packet from the queue: */
	MulticastPacket* packet=pipeState->packetList.pop_front();
	
	/* Return the received packet: */
	return packet;
	}

void MulticastPipeMultiplexer::barrier(unsigned int pipeId)
	{
	/* Get a handle on the state object for the given pipe: */
	LockedPipe pipeState(pipeStateTable,pipeStateTableMutex,pipeId);
	if(!pipeState.isValid())
		Misc::throwStdErr("MulticastPipeMultiplexer: Attempt to synchronize closed pipe");
		
	/* Bump up barrier ID: */
	unsigned int nextBarrierId=pipeState->barrierId+1;
	
	if(nodeIndex==0)
		{
		/* Wait until barrier messages from all slaves have been received: */
		while(pipeState->minSlaveBarrierId<nextBarrierId)
			{
			/* Wait until the next barrier message: */
			pipeState->barrierCond.wait(pipeState->stateMutex);
			}
		
		/*******************************************************************
		Flush the list of sent packets:
		*******************************************************************/
		
		/* Add all packets in the list to the list of free packets: */
		if(pipeState->packetList.numPackets>0)
			{
			{
			Threads::Mutex::Lock packetPoolLock(packetPoolMutex);
			pipeState->packetList.tail->succ=packetPoolHead;
			packetPoolHead=pipeState->packetList.head;
			pipeState->packetList.numPackets=0;
			pipeState->packetList.head=0;
			pipeState->packetList.tail=0;
			}
			}
		
		/* Reset the pipe's flow control state: */
		pipeState->headStreamPos=pipeState->streamPos;
		for(unsigned int i=0;i<numSlaves;++i)
			pipeState->slaveStreamPosOffsets[i]=0;
		pipeState->numHeadSlaves=numSlaves;
		
		/* Send barrier completion message to all slaves: */
		MasterMessage msg(MasterMessage::BARRIER);
		msg.pipeId=pipeId;
		msg.barrierId=nextBarrierId;
		{
		Threads::Mutex::Lock socketLock(socketMutex);
		sendto(socketFd,&msg,sizeof(MasterMessage),0,(const sockaddr*)otherAddress,sizeof(sockaddr_in));
		}
		}
	else
		{
		/* Continue sending barrier messages to master until barrier completion message is received: */
		Misc::Time waitTimeout=Misc::Time::now();
		while(true)
			{
			/* Send barrier message to master: */
			SlaveMessage msg;
			msg.nodeIndex=nodeIndex;
			msg.messageId=SlaveMessage::BARRIER;
			msg.pipeId=pipeId;
			msg.barrierId=nextBarrierId;
			for(int i=0;i<1;++i)
				{
				Threads::Mutex::Lock socketLock(socketMutex);
				sendto(socketFd,&msg,sizeof(SlaveMessage),0,(const sockaddr*)otherAddress,sizeof(struct sockaddr_in));
				}
			
			/* Wait for arrival of barrier completion message: */
			waitTimeout+=barrierWaitTimeout;
			if(pipeState->barrierCond.timedWait(pipeState->stateMutex,waitTimeout))
				break;
			}
		}
	
	/* Mark the barrier as completed: */
	pipeState->barrierId=nextBarrierId;
	}

unsigned int MulticastPipeMultiplexer::gather(unsigned int pipeId,unsigned int value,GatherOperation::OpCode op)
	{
	unsigned int result;
	
	/* Get a handle on the state object for the given pipe: */
	LockedPipe pipeState(pipeStateTable,pipeStateTableMutex,pipeId);
	if(!pipeState.isValid())
		Misc::throwStdErr("MulticastPipeMultiplexer: Attempt to gather on closed pipe");
	
	/* Bump up barrier ID: */
	unsigned int nextBarrierId=pipeState->barrierId+1;
	
	if(nodeIndex==0)
		{
		/* Wait until gather messages from all slaves have been received: */
		while(pipeState->minSlaveBarrierId<nextBarrierId)
			{
			/* Wait until the next barrier message: */
			pipeState->barrierCond.wait(pipeState->stateMutex);
			}
		
		/* Calculate the final gather value: */
		pipeState->masterGatherValue=value;
		switch(op)
			{
			case GatherOperation::AND:
				for(unsigned int i=0;i<numSlaves;++i)
					pipeState->masterGatherValue=pipeState->masterGatherValue&&pipeState->slaveGatherValues[i];
				break;
			
			case GatherOperation::OR:
				for(unsigned int i=0;i<numSlaves;++i)
					pipeState->masterGatherValue=pipeState->masterGatherValue||pipeState->slaveGatherValues[i];
				break;
			
			case GatherOperation::MIN:
				for(unsigned int i=0;i<numSlaves;++i)
					if(pipeState->masterGatherValue>pipeState->slaveGatherValues[i])
						pipeState->masterGatherValue=pipeState->slaveGatherValues[i];
				break;
			
			case GatherOperation::MAX:
				for(unsigned int i=0;i<numSlaves;++i)
					if(pipeState->masterGatherValue<pipeState->slaveGatherValues[i])
						pipeState->masterGatherValue=pipeState->slaveGatherValues[i];
				break;
			
			case GatherOperation::SUM:
				for(unsigned int i=0;i<numSlaves;++i)
					pipeState->masterGatherValue+=pipeState->slaveGatherValues[i];
				break;
			
			case GatherOperation::PRODUCT:
				for(unsigned int i=0;i<numSlaves;++i)
					pipeState->masterGatherValue*=pipeState->slaveGatherValues[i];
				break;
			}
		
		/*******************************************************************
		Flush the list of sent packets:
		*******************************************************************/
		
		/* Add all packets in the list to the list of free packets: */
		if(pipeState->packetList.numPackets>0)
			{
			{
			Threads::Mutex::Lock packetPoolLock(packetPoolMutex);
			pipeState->packetList.tail->succ=packetPoolHead;
			packetPoolHead=pipeState->packetList.head;
			pipeState->packetList.numPackets=0;
			pipeState->packetList.head=0;
			pipeState->packetList.tail=0;
			}
			}
		
		/* Reset the pipe's flow control state: */
		pipeState->headStreamPos=pipeState->streamPos;
		for(unsigned int i=0;i<numSlaves;++i)
			pipeState->slaveStreamPosOffsets[i]=0;
		pipeState->numHeadSlaves=numSlaves;
		
		/* Send gather completion message to all slaves: */
		MasterMessage msg(MasterMessage::GATHER);
		msg.pipeId=pipeId;
		msg.barrierId=nextBarrierId;
		msg.masterValue=pipeState->masterGatherValue;
		{
		Threads::Mutex::Lock socketLock(socketMutex);
		sendto(socketFd,&msg,sizeof(MasterMessage),0,(const sockaddr*)otherAddress,sizeof(sockaddr_in));
		}
		}
	else
		{
		/* Continue sending barrier messages to master until barrier completion message is received: */
		Misc::Time waitTimeout=Misc::Time::now();
		while(true)
			{
			/* Send gather message to master: */
			SlaveMessage msg;
			msg.nodeIndex=nodeIndex;
			msg.messageId=SlaveMessage::GATHER;
			msg.pipeId=pipeId;
			msg.barrierId=nextBarrierId;
			msg.slaveValue=value;
			for(int i=0;i<1;++i)
				{
				Threads::Mutex::Lock socketLock(socketMutex);
				sendto(socketFd,&msg,sizeof(SlaveMessage),0,(const sockaddr*)otherAddress,sizeof(struct sockaddr_in));
				}
			
			/* Wait for arrival of barrier completion message: */
			waitTimeout+=barrierWaitTimeout;
			if(pipeState->barrierCond.timedWait(pipeState->stateMutex,waitTimeout))
				break;
			}
		}
	
	/* Return the master gather value: */
	result=pipeState->masterGatherValue;
	
	/* Mark the gathering operation as completed: */
	pipeState->barrierId=nextBarrierId;
	
	return result;
	}

}
