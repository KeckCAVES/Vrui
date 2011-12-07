/***********************************************************************
ClusterPipe - Base class providing a 1-to-n intra-cluster communication
pattern using a cluster multiplexer.
Copyright (c) 2011 Oliver Kreylos

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

#ifndef CLUSTER_CLUSTERPIPE_INCLUDED
#define CLUSTER_CLUSTERPIPE_INCLUDED

#include <Cluster/GatherOperation.h>
#include <Cluster/Multiplexer.h>

namespace Cluster {

class ClusterPipe
	{
	/* Elements: */
	protected:
	Multiplexer* multiplexer; // Pointer to the intra-cluster communication multiplexer
	unsigned int pipeId; // Unique identifier of this pipe for the same multiplexer
	
	/* Protected methods: */
	virtual void flushPipe(void); // Immediately sends any buffered data on the master side
	
	/* Constructors and destructors: */
	protected:
	ClusterPipe(Multiplexer* sMultiplexer); // Creates a new pipe on the given multiplexer
	virtual ~ClusterPipe(void); // Closes the pipe
	
	/* New methods: */
	public:
	Multiplexer* getMultiplexer(void) const // Returns the pipe's underlying intra-cluster communication multiplexer
		{
		return multiplexer;
		}
	bool isMaster(void) const // Convenience method returning whether this node is the cluster's head node
		{
		return multiplexer->isMaster();
		}
	unsigned int getNumNodes(void) const // Convenience method to get the cluster's total number of nodes
		{
		return multiplexer->getNumNodes();
		}
	unsigned int getNodeIndex(void) const // Convenience method to get this node's cluster index
		{
		return multiplexer->getNodeIndex();
		}
	virtual void barrier(void); // Blocks the calling thread until all nodes in a cluster pipe have reached the same point in the program
	virtual unsigned int gather(unsigned int value,GatherOperation::OpCode op); // Blocks the calling thread until all nodes in a cluster pipe have exchanged a value; returns final accumulated value
	};

}

#endif
