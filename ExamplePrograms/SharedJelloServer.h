/***********************************************************************
SharedJelloServer - Dedicated server program to allow multiple clients
to collaboratively smack around a Jell-O crystal.
Copyright (c) 2007 Oliver Kreylos

This file is part of the Virtual Jell-O interactive VR demonstration.

Virtual Jell-O is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

Virtual Jell-O is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with Virtual Jell-O; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SHAREDJELLOSERVER_INCLUDED
#define SHAREDJELLOSERVER_INCLUDED

#include <vector>
#include <Threads/Thread.h>
#include <Threads/Mutex.h>
#include <Comm/TCPSocket.h>
#include <Geometry/Box.h>

#include "JelloAtom.h"
#include "JelloCrystal.h"
#include "SharedJelloPipe.h"

class SharedJelloServer
	{
	/* Embedded classes: */
	private:
	typedef JelloCrystal::Scalar Scalar;
	typedef JelloCrystal::Point Point;
	typedef JelloCrystal::Vector Vector;
	typedef JelloCrystal::Rotation Rotation;
	typedef JelloCrystal::ONTransform ONTransform;
	typedef JelloCrystal::Ray Ray;
	typedef JelloCrystal::Box Box;
	typedef JelloCrystal::AtomID AtomID;
	public:
	typedef JelloCrystal::Index Index; // Index type for the atom array
	
	private:
	struct ClientState // Structure to hold the input device state of a connected client
		{
		/* Embedded classes: */
		public:
		struct StateUpdate // Structure to hold the contents of a state update packet
			{
			/* Elements: */
			public:
			int numDraggers; // Number of dragging tools in the state update
			unsigned int* draggerIDs; // Array of unique (per client) IDs for each dragger, to detect dynamic creation/deletion
			bool* draggerRayBaseds; // Array of flags if a dragger has ray-based selection
			Ray* draggerRays; // Array of ray directions for each dragger
			ONTransform* draggerTransformations; // Array of dragger positions/orientations
			bool* draggerActives; // Array of active flags for each dragger
			
			/* Constructors and destructors: */
			StateUpdate(void)
				:numDraggers(0),draggerIDs(0),draggerRayBaseds(0),draggerRays(0),draggerTransformations(0),draggerActives(0)
				{
				};
			~StateUpdate(void)
				{
				delete[] draggerIDs;
				delete[] draggerRayBaseds;
				delete[] draggerRays;
				delete[] draggerTransformations;
				delete[] draggerActives;
				};
			};
		
		struct AtomLock // Structure to connect a client's dragger to a locked Jell-O atom
			{
			/* Elements: */
			public:
			unsigned int draggerID; // ID of the locking dragger
			AtomID draggedAtom; // ID of the locked atom
			ONTransform dragTransformation; // The dragging transformation applied to the locked atom
			};
		
		/* Elements: */
		public:
		SharedJelloPipe pipe; // Communication pipe connected to the client
		Threads::Thread communicationThread; // Thread receiving state updates from the client
		bool connected; // Flag if the client's connection protocol has finished
		unsigned int parameterVersion; // Version number of parameter set on the client side
		StateUpdate stateUpdates[3]; // Triple buffer of state update packets
		volatile int lockedIndex; // Buffer index currently locked by the consumer
		volatile int mostRecentIndex; // Buffer index of most recently produced value
		std::vector<AtomLock> atomLocks; // List of atom locks held by this client
		
		/* Constructors and destructors: */
		ClientState(const Comm::TCPSocket& socket)
			:pipe(&socket),
			 connected(false),
			 parameterVersion(0),
			 lockedIndex(0),mostRecentIndex(0)
			{
			};
		};
	
	typedef std::vector<ClientState*> ClientStateList; // Type for lists of pointers to client state structures
	
	/* Elements: */
	
	/* Jell-O state: */
	Threads::Mutex parameterMutex; // Mutex serializing write access to set of simulation parameters
	unsigned int newParameterVersion; // Version number of current set of simulation parameters
	Scalar newAtomMass;
	Scalar newAttenuation;
	Scalar newGravity;
	JelloCrystal crystal; // The virtual Jell-O crystal
	unsigned int parameterVersion; // Version number of simulation parameters set in Jell-O crystal
	
	/* Client communication state: */
	Comm::TCPSocket listenSocket; // Listening socket for incoming client connections
	Threads::Thread listenThread; // Thread listening for incoming connections on the listening port
	Threads::Mutex clientStateListMutex; // Mutex protecting the client state list (not the included structures)
	ClientStateList clientStates; // List of client state structures
	
	/* Private methods: */
	void* listenThreadMethod(void); // Thread method accepting connections from new clients
	void* clientCommunicationThreadMethod(ClientState* clientState); // Thread method receiving state updates from connected clients
	
	/* Constructors and destructors: */
	public:
	SharedJelloServer(const Index& numAtoms,int listenPortID); // Creates a shared Jell-O server with the given crystal size and listen port ID (assigns dynamic port if port ID is negative)
	~SharedJelloServer(void); // Destroys the shared Jell-O server
	
	/* Methods: */
	int getListenPortID(void) const // Returns the port ID assigned to the listening socket
		{
		return listenSocket.getPortId();
		};
	void simulate(double timeStep); // Updates the simulation state based on the real time since the last frame
	void sendServerUpdate(void); // Sends the most recent Jell-O crystal state to all connected clients
	};

#endif
