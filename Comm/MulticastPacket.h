/***********************************************************************
MulticastPacket - Structure for packets that are sent across a multicast
link over UDP.
Copyright (c) 2005 Oliver Kreylos

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

#ifndef COMM_MULTICASTPACKET_INCLUDED
#define COMM_MULTICASTPACKET_INCLUDED

namespace Comm {

struct MulticastPacket
	{
	/* Embedded classes: */
	public:
	static const size_t maxPacketSize=1472-2*sizeof(unsigned int); // Maximum size of packet in bytes
	
	/* Elements: */
	MulticastPacket* succ; // Pointer to successor in packet queues
	size_t packetSize; // Actual size of packet
	unsigned int pipeId; // ID of the pipe this packet is intended for
	unsigned int streamPos; // Position of packet data in entire stream that has been sent on pipe so far
	char packet[maxPacketSize]; // Packet data
	
	/* Constructors and destructors: */
	MulticastPacket(void) // Creates empty packet
		:succ(0),packetSize(0)
		{
		}
	};

}

#endif
