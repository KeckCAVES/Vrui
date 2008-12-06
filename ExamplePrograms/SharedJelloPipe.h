/***********************************************************************
SharedJelloPipe - Class defining the communication protocol between a
shared Jell-O server and its clients.
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

#ifndef SHAREDJELLOPIPE_INCLUDED
#define SHAREDJELLOPIPE_INCLUDED

#include <Comm/ClusterPipe.h>
#include <Geometry/OrthonormalTransformation.h>

#include "JelloAtom.h"

class SharedJelloPipe:public Comm::ClusterPipe
	{
	/* Embedded classes: */
	public:
	typedef unsigned short int MessageIdType; // Network type for protocol messages
	typedef JelloAtom::Scalar Scalar;
	typedef JelloAtom::Point Point;
	typedef JelloAtom::Vector Vector;
	typedef JelloAtom::Rotation Rotation;
	typedef Geometry::OrthonormalTransformation<Scalar,3> ONTransform;
	
	enum MessageId // Enumerated type for protocol messages
		{
		CONNECT_REPLY, // Initiates connection by sending the Jell-O crystal's parameters to the client
		CLIENT_UPDATE, // Updates the connected client's state on the server side
		SERVER_UPDATE, // Sends current state of all other connected clients to a connected client
		CLIENT_PARAMUPDATE, // Sends new simulation parameters from client to server
		SERVER_PARAMUPDATE, // Sends new simulation parameters from server to client
		DISCONNECT_REQUEST, // Polite request to disconnect from the server
		DISCONNECT_REPLY // Reply to a disconnect request
		};
	
	/* Constructors and destructors: */
	public:
	SharedJelloPipe(const char* hostName,int portID,Comm::MulticastPipe* sPipe =0) // Creates a shared Jell-O pipe for the given server host name/port ID
		:Comm::ClusterPipe(hostName,portID,sPipe)
		{
		};
	SharedJelloPipe(const Comm::TCPSocket* sSocket,Comm::MulticastPipe* sPipe =0) // Creates a shared Jell-O pipe for the given TCP socket
		:Comm::ClusterPipe(sSocket,sPipe)
		{
		};
	
	/* Methods: */
	void writeMessage(MessageId messageId) // Writes a protocol message to the pipe
		{
		write<MessageIdType>(messageId);
		};
	MessageIdType readMessage(void) // Reads a protocol message from the pipe
		{
		return read<MessageIdType>();
		};
	void writePoint(const Point& p) // Writes a point to the pipe
		{
		write<Scalar>(p.getComponents(),3);
		};
	Point readPoint(void) // Reads a point from the pipe
		{
		Point result;
		read<Scalar>(result.getComponents(),3);
		return result;
		};
	void writeVector(const Vector& v) // Writes a vector to the pipe
		{
		write<Scalar>(v.getComponents(),3);
		};
	Vector readVector(void) // Reads a vector from the pipe
		{
		Vector result;
		read<Scalar>(result.getComponents(),3);
		return result;
		};
	void writeRotation(const Rotation& rotation) // Writes a rotation to the pipe
		{
		write<Scalar>(rotation.getQuaternion(),4);
		};
	Rotation readRotation(void) // Reads a rotation from the pipe
		{
		Scalar rotationQuaternion[4];
		read<Scalar>(rotationQuaternion,4);
		return Rotation::fromQuaternion(rotationQuaternion);
		};
	void writeONTransform(const ONTransform& transform) // Writes an orthonormal transformation to the pipe
		{
		writeVector(transform.getTranslation());
		writeRotation(transform.getRotation());
		};
	ONTransform readONTransform(void) // Reads an orthonormal transformation from the pipe
		{
		Vector translation=readVector();
		Rotation rotation=readRotation();
		return ONTransform(translation,rotation);
		};
	};

#endif
