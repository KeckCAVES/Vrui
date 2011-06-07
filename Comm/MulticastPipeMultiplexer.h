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

#ifndef COMM_MULTICASTPIPEMULTIPLEXER_INCLUDED
#define COMM_MULTICASTPIPEMULTIPLEXER_INCLUDED

#include <string>
#include <Misc/HashTable.h>
#include <Misc/Time.h>
#include <Threads/Thread.h>
#include <Threads/Mutex.h>
#include <Threads/Cond.h>
#include <Threads/MutexCond.h>
#include <Comm/MulticastPacket.h>
#include <Comm/GatherOperation.h>

/* Forward declarations: */
struct sockaddr_in;

namespace Comm {

class MulticastPipeMultiplexer
	{
	/* Embedded classes: */
	private:
	struct SlaveMessage // Structure for messages sent from the slaves to the master
		{
		/* Embedded classes: */
		public:
		enum MessageId // Enumerated type for slave message IDs
			{
			CONNECTION, // Signal that slave joined multicast group
			PING, // Ping request from slave to master
			CREATEPIPE, // Signal that slave created new pipe of given ID
			ACKNOWLEDGMENT, // Signal that slave has received some stream packets
			PACKETLOSS, // Signal that slave lost a stream packet
			BARRIER, // Barrier message sent from slaves to master
			GATHER // Message conveying a slave's gather value in a gather operation
			};
		
		/* Elements: */
		unsigned int nodeIndex; // Index of slave node that sent this message
		int messageId; // ID of message
		unsigned int pipeId; // ID of pipe related to message
		unsigned int streamPos; // Current stream position of slave when packet loss is detected
		unsigned int packetPos; // Stream position of packet after packet loss
		unsigned int barrierId; // ID of current barrier in barrier message or gather operation in gather message
		unsigned int slaveValue; // Slave's gather value in a gather operation
		};
	
	struct MasterMessage // Structure for messages sent from the master to the slaves (embedded in normal multicast packets with pipeId==0)
		{
		/* Embedded classes: */
		public:
		enum MessageId // Enumerated type for master message IDs
			{
			CONNECTION, // Signal that multicast group is complete
			PING, // Ping reply from master to slave
			CREATEPIPE, // Signal that pipe creation is complete
			BARRIER, // Signal that barrier is complete
			GATHER // Signal that a gather operation is complete; payload is final gather value
			};
		
		/* Elements: */
		public:
		unsigned int zeroPipeId; // Zero pipe ID to distinguish master messages from regular multicast packets
		int messageId; // ID of message
		unsigned int pipeId; // ID of affected pipe for barrier or gather completion messages
		unsigned int barrierId; // ID of completed barrier or gather operation
		unsigned int masterValue; // Master's final value in a gather operation
		
		/* Constructors and destructors: */
		public:
		MasterMessage(MessageId sMessageId) // Constructs a simple message structure
			:zeroPipeId(0),messageId(sMessageId),pipeId(0),barrierId(0),masterValue(0)
			{
			}
		};
	
	struct PipeState // Structure storing the current state of a pipe
		{
		/* Embedded classes: */
		public:
		struct PacketList // Structure for lists of multicast packets
			{
			/* Elements: */
			public:
			unsigned int numPackets; // Number of packets currently in list
			MulticastPacket* head; // Pointer to first packet in list
			MulticastPacket* tail; // Pointer to last packet in list
			
			/* Constructors and destructors: */
			PacketList(void); // Creates an empty packet list
			~PacketList(void); // Destroys a packet list and all packets in it
			
			/* Methods: */
			bool empty(void) const // Returns true if the list is empty
				{
				return numPackets==0;
				}
			unsigned int size(void) const // Returns number of packets in list
				{
				return numPackets;
				}
			MulticastPacket* front(void) const // Returns the first packet in the list
				{
				return head;
				}
			MulticastPacket* back(void) const // Returns the last packet in the list
				{
				return tail;
				}
			void push_back(MulticastPacket* packet); // Pushes the given packet on the back of the list
			MulticastPacket* pop_front(void); // Removes the packet at the front of the list and returns pointer to it
			};
		
		/* Elements: */
		public:
		Threads::Mutex stateMutex; // Mutex serializing access to the pipe state
		Threads::Cond receiveCond; // Condition variable receivers wait on when the delivery queue is empty
		Threads::Cond barrierCond; // Condition variable all nodes wait on while processing a barrier
		unsigned int streamPos; // Total amount of bytes that has been sent/received on this pipe so far
		bool packetLossMode; // True if the pipe is currently recovering from lost data
		PacketList packetList; // List of packets to be delivered to readers (on the slave side) or recently sent (on the master side)
		unsigned int headStreamPos; // Stream position currently at the head of the packet list
		unsigned int* slaveStreamPosOffsets; // Array of stream positions of the slaves relative to beginning of packet list
		unsigned int numHeadSlaves; // Number of slaves that still have not acknowledged the first packet in the packet list
		unsigned int barrierId; // Unique identifier of last completed barrier in pipe
		unsigned int* slaveBarrierIds; // Array of most recently received barrier messages from the slaves
		unsigned int minSlaveBarrierId; // Smallest barrier ID currently in the state array
		unsigned int* slaveGatherValues; // Array of most recently received gather values from the slaves
		unsigned int masterGatherValue; // Final value of last completed gather operation in pipe
		// DEBUGGING
		//unsigned int totalNumResentBytes;
		
		/* Constructors and destructors: */
		PipeState(void); // Creates empty pipe state
		~PipeState(void); // Destroys a pipe state and all buffers in its delivery queue
		};
	
	typedef Misc::HashTable<unsigned int,PipeState*> PipeHasher; // Hash table to map from pipe IDs to pipe state table entries
	
	class LockedPipe // Helper class to obtain locks on pipe state objects retrieved by pipe ID
		{
		/* Elements: */
		private:
		PipeState* pipeState; // Pointer to the locked pipe state object, or NULL
		
		/* Constructors and destructors: */
		public:
		LockedPipe(const PipeHasher& pipeStateTable,Threads::Mutex& pipeStateTableMutex,unsigned int pipeId) // Grabs and locks the pipe of the given ID, if it exists
			:pipeState(0)
			{
			/* Lock the pipe state table: */
			Threads::Mutex::Lock pipeStateTableLock(pipeStateTableMutex);
			
			/* Get the pipe: */
			PipeHasher::ConstIterator psIt=pipeStateTable.findEntry(pipeId);
			if(!psIt.isFinished())
				{
				/* Get the pipe state pointer, and lock it: */
				pipeState=psIt->getDest();
				pipeState->stateMutex.lock();
				}
			}
		~LockedPipe(void)
			{
			/* Unlock the pipe state if it is valid: */
			if(pipeState!=0)
				pipeState->stateMutex.unlock();
			}
		
		/* Methods: */
		bool isValid(void) const // Returns true if the pipe state is valid
			{
			return pipeState!=0;
			}
		PipeState& operator*(void) const
			{
			return *pipeState;
			}
		PipeState* operator->(void) const
			{
			return pipeState;
			}
		PipeState* unlock(void) // Explicitly unlocks the pipe state and returns it as a standard pointer
			{
			PipeState* result=pipeState;
			if(pipeState!=0)
				{
				/* Unlock the pipe state and invalidate it: */
				pipeState->stateMutex.unlock();
				pipeState=0;
				}
			return result;
			}
		};
	
	/* Elements: */
	private:
	unsigned int numSlaves; // Number of slaves in the multicast group
	unsigned int nodeIndex; // Index of this node; master node == 0
	struct sockaddr_in* otherAddress; // Pointer to socket address of other end of multicast connection
	Threads::Mutex socketMutex; // Mutex serializing (write) access to the UDP socket
	int socketFd; // File descriptor for the UDP socket
	bool connected; // Flag to indicate whether connection between master and all slaves has been established
	Threads::MutexCond connectionCond; // Condition variable to wait on for connection establishment
	Threads::Mutex pipeStateTableMutex; // Mutex serializing access to the the pipe state table
	unsigned int nextPipeId; // ID of the next pipe to be created
	PipeHasher pipeStateTable; // Hash table to map from pipe IDs to pipe state table entries
	Threads::Thread packetHandlingThread; // Packet handling thread
	MulticastPacket* slaveThreadPacket; // Pointer to the one packet always held by the packet handling thread on slave nodes
	int masterMessageBurstSize; // Number of server messages sent in a single burst
	int slaveMessageBurstSize; // Number of client messages sent in a single burst
	Misc::Time connectionWaitTimeout; // Timeout between connection messages from the slaves
	Misc::Time pingTimeout; // Timeout between ping requests from the slaves
	int maxPingRequests; // Maximum number of consecutive ping requests before the slave signals a communication error
	Misc::Time receiveWaitTimeout; // Timeout between packet loss messages from the slaves
	Misc::Time barrierWaitTimeout; // Timeout between barrier messages from the slaves
	unsigned int sendBufferSize; // Maximum number of packets buffered for each pipe
	Threads::Mutex packetPoolMutex; // Mutex protecting the free packet pool
	MulticastPacket* packetPoolHead; // Pool of recently deleted packets to minimize number of new/delete calls
	
	/* Private methods: */
	void processAcknowledgment(LockedPipe& pipeState,int slaveIndex,unsigned int streamPos); // Processes an acknowlegment (positive or implied-positive) from a slave
	void* packetHandlingThreadMaster(void); // Packet handling thread method for the master
	void* packetHandlingThreadSlave(void); // Packet handling thread method for the slaves
	
	/* Constructors and destructors: */
	public:
	MulticastPipeMultiplexer(unsigned int sNumSlaves,unsigned int sNodeIndex,std::string masterHostName,int masterPortNumber,std::string slaveMulticastGroup,int slavePortNumber);
	~MulticastPipeMultiplexer(void);
	
	/* Methods: */
	MulticastPacket* newPacket(void) // Returns a new multicast packet
		{
		Threads::Mutex::Lock packetPoolLock(packetPoolMutex);
		if(packetPoolHead==0)
			return new MulticastPacket();
		else
			{
			MulticastPacket* result=packetPoolHead;
			packetPoolHead=packetPoolHead->succ;
			return result;
			}
		}
	void deletePacket(MulticastPacket* packet) // Deletes the given multicast packet
		{
		Threads::Mutex::Lock packetPoolLock(packetPoolMutex);
		packet->succ=packetPoolHead;
		packetPoolHead=packet;
		}
	bool isMaster(void) const // Returns true if the local multiplexer is the master node
		{
		return nodeIndex==0;
		}
	unsigned int getNumNodes(void) const // Returns the total number of nodes in the multicast group
		{
		return numSlaves+1;
		}
	unsigned int getNumSlaves(void) const // Returns the number of slave nodes in the multicast group
		{
		return numSlaves;
		}
	unsigned int getNodeIndex(void) const // Returns the index of this node in the multicast group (0: master node)
		{
		return nodeIndex;
		}
	int getLocalPortNumber(void) const; // Returns port number of local communication socket
	void setConnectionWaitTimeout(Misc::Time newConnectionWaitTimeout); // Sets the timeout when waiting for connection messages
	void setPingTimeout(Misc::Time newPingTimeout,int newMaxPingRequests); // Sets the time after which slaves request a ping packet when no data is received, and the maximum number of requests sent before a connection error is signaled
	void setReceiveWaitTimeout(Misc::Time newReceiveWaitTimeout); // Sets the timeout when waiting for data packages
	void setBarrierWaitTimeout(Misc::Time newBarrierWaitTimeout); // Sets the timeout when waiting for barrier messages
	void waitForConnection(void); // Waits until all slaves have connected to the master
	
	/* Pipe management interface: */
	unsigned int openPipe(void); // Creates a new multicast pipe and returns its pipe ID
	void closePipe(unsigned int pipeId); // Destroys the multicast pipe of the given ID
	
	/* Pipe communication interface: */
	void sendPacket(unsigned int pipeId,MulticastPacket* packet); // Sends a packet from the master to the slaves
	MulticastPacket* receivePacket(unsigned int pipeId); // Receives a packet from the master
	void barrier(unsigned int pipeId); // Waits until all nodes (master + slaves) have reached the same point in the program
	unsigned int gather(unsigned int pipeId,unsigned int value,GatherOperation::OpCode op); // Exchanges a single value between all nodes (master + slaves); implies a barrier
	};

}

#endif
