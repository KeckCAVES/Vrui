/***********************************************************************
VRDeviceServer - Class encapsulating the VR device protocol's server
side.
Copyright (c) 2002-2005 Oliver Kreylos

This file is part of the Vrui VR Device Driver Daemon (VRDeviceDaemon).

The Vrui VR Device Driver Daemon is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vrui VR Device Driver Daemon is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui VR Device Driver Daemon; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include "VRDeviceServer.h"

#include <stdio.h>
#include <stdexcept>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>

#include "VRDeviceManager.h"

/*******************************
Methods of class VRDeviceServer:
*******************************/

void* VRDeviceServer::listenThreadMethod(void)
	{
	/* Enable immediate cancellation of this thread: */
	Threads::Thread::setCancelState(Threads::Thread::CANCEL_ENABLE);
	Threads::Thread::setCancelType(Threads::Thread::CANCEL_ASYNCHRONOUS);
	
	while(true)
		{
		/* Wait for the next incoming connection: */
		#ifdef VERBOSE
		printf("VRDeviceServer: Waiting for client connection\n");
		fflush(stdout);
		#endif
		Comm::TCPSocket clientSocket=listenSocket.accept();
		
		/* Connect the new client: */
		#ifdef VERBOSE
		try
			{
			printf("VRDeviceServer: Connecting new client from %s, port %d\n",clientSocket.getPeerHostname().c_str(),clientSocket.getPeerPortId());
			fflush(stdout);
			}
		catch(std::runtime_error error)
			{
			printf("VRDeviceServer: Connecting new client from %s, port %d\n",clientSocket.getPeerAddress().c_str(),clientSocket.getPeerPortId());
			fflush(stdout);
			}
		#endif
		{
		Threads::Mutex::Lock clientListLock(clientListMutex);
		ClientData* newClient=new ClientData(clientSocket);
		clientList.push_back(newClient);
		newClient->communicationThread.start(this,&VRDeviceServer::clientCommunicationThreadMethod,newClient);
		}
		}
	
	return 0;
	}

void* VRDeviceServer::clientCommunicationThreadMethod(VRDeviceServer::ClientData* clientData)
	{
	/* Enable immediate cancellation of this thread: */
	Threads::Thread::setCancelState(Threads::Thread::CANCEL_ENABLE);
	Threads::Thread::setCancelType(Threads::Thread::CANCEL_ASYNCHRONOUS);
	
	Vrui::VRDevicePipe& pipe=clientData->pipe;
	
	enum State
		{
		START,CONNECTED,ACTIVE,STREAMING,FINISH
		};
	
	try
		{
		/* Execute client communication protocol state machine: */
		State state=START; // Current client state
		while(state!=FINISH)
			{
			if(clientData->streaming)
				{
				/* Wait for next tracker update: */
				trackerUpdateCompleteCond.wait();
				
				deviceManager->lockState();
				try
					{
					/* Send packet reply message: */
					pipe.writeMessage(Vrui::VRDevicePipe::PACKET_REPLY);
					
					/* Send server state: */
					pipe.writeState(deviceManager->getState());
					}
				catch(std::runtime_error err)
					{
					/* Unlock the device manager's state and throw the exception again: */
					deviceManager->unlockState();
					throw;
					}
				deviceManager->unlockState();
				
				/* Check for messages: */
				if(pipe.getSocket().waitForData(0,0,false))
					{
					switch(pipe.readMessage())
						{
						case Vrui::VRDevicePipe::PACKET_REQUEST:
							/* Ignore message: */
							break;
						
						case Vrui::VRDevicePipe::STOPSTREAM_REQUEST:
							/* Send stopstream reply message: */
							pipe.writeMessage(Vrui::VRDevicePipe::STOPSTREAM_REPLY);
							
							/* Go to active state: */
							clientData->streaming=false;
							state=ACTIVE;
							break;
						
						default:
							state=FINISH;
						}
					}
				}
			else
				{
				Vrui::VRDevicePipe::MessageIdType message=pipe.readMessage();
				switch(state)
					{
					case START:
						switch(message)
							{
							case Vrui::VRDevicePipe::CONNECT_REQUEST:
								/* Send connect reply message: */
								pipe.writeMessage(Vrui::VRDevicePipe::CONNECT_REPLY);
								
								/* Send server layout: */
								pipe.writeLayout(deviceManager->getState());
								
								/* Go to connected state: */
								state=CONNECTED;
								break;
							
							default:
								state=FINISH;
							}
						break;
					
					case CONNECTED:
						switch(message)
							{
							case Vrui::VRDevicePipe::ACTIVATE_REQUEST:
								{
								Threads::Mutex::Lock clientListLock(clientListMutex);
								
								/* Start VR devices if this is the first active client: */
								if(numActiveClients==0)
									deviceManager->start();
								
								/* Activate client: */
								++numActiveClients;
								}
								clientData->active=true;
								
								/* Go to active state: */
								state=ACTIVE;
								break;
							
							default:
								state=FINISH;
							}
						break;
					
					case ACTIVE:
						switch(message)
							{
							case Vrui::VRDevicePipe::PACKET_REQUEST:
								deviceManager->lockState();
								try
									{
									/* Send packet reply message: */
									pipe.writeMessage(Vrui::VRDevicePipe::PACKET_REPLY);
									
									/* Send server state: */
									pipe.writeState(deviceManager->getState());
									}
								catch(std::runtime_error err)
									{
									/* Unlock the device manager's state and throw the exception again: */
									deviceManager->unlockState();
									throw;
									}
								deviceManager->unlockState();
								break;
							
							case Vrui::VRDevicePipe::STARTSTREAM_REQUEST:
								deviceManager->lockState();
								try
									{
									/* Send packet reply message: */
									pipe.writeMessage(Vrui::VRDevicePipe::PACKET_REPLY);
									
									/* Send server state: */
									pipe.writeState(deviceManager->getState());
									}
								catch(std::runtime_error err)
									{
									/* Unlock the device manager's state and throw the exception again: */
									deviceManager->unlockState();
									throw;
									}
								deviceManager->unlockState();
								
								/* Go to streaming state: */
								clientData->streaming=true;
								state=STREAMING;
								break;
							
							case Vrui::VRDevicePipe::DEACTIVATE_REQUEST:
								/* Deactivate client: */
								clientData->active=false;
								{
								Threads::Mutex::Lock clientListLock(clientListMutex);
								--numActiveClients;
								
								/* Stop VR devices if this was the last active client: */
								if(numActiveClients==0)
									deviceManager->stop();
								}
								
								/* Go to connected state: */
								state=CONNECTED;
								break;
							
							default:
								state=FINISH;
							}
						break;
					
					default:
						/* Just to make g++ happy... */
						;
					}
				}
			}
		}
	catch(std::runtime_error err)
		{
		/* Print error message to stderr, but ignore exception otherwise: */
		fprintf(stderr,"VRDeviceServer: Terminating client connection due to exception\n  %s\n",err.what());
		fflush(stderr);
		}
	catch(...)
		{
		/* Just ignore the exception: */
		fprintf(stderr,"VRDeviceServer: Terminating client connection due to spurious exception\n");
		fflush(stderr);
		}
	
	/* Cleanly deactivate client: */
	{
	Threads::Mutex::Lock clientListLock(clientListMutex);
	if(clientData->streaming)
		{
		/* Leave streaming mode: */
		clientData->streaming=false;
		}
	if(clientData->active)
		{
		/* Deactivate client: */
		clientData->active=false;
		--numActiveClients;
		
		/* Stop VR devices if this was the last active client: */
		if(numActiveClients==0)
			deviceManager->stop();
		}
	
	/* Remove client from list: */
	ClientList::iterator clIt;
	for(clIt=clientList.begin();clIt!=clientList.end()&&*clIt!=clientData;++clIt)
		;
	clientList.erase(clIt);
	
	/* Disconnect client: */
	delete clientData;
	}
	
	/* Terminate: */
	#ifdef VERBOSE
	printf("VRDeviceServer: Disconnected client\n");
	fflush(stdout);
	#endif
	
	return 0;
	}

VRDeviceServer::VRDeviceServer(VRDeviceManager* sDeviceManager,const Misc::ConfigurationFile& configFile)
	:deviceManager(sDeviceManager),
	 listenSocket(configFile.retrieveValue<int>("./serverPort"),0),
	 numActiveClients(0)
	{
	/* Enable tracker update notification: */
	deviceManager->enableTrackerUpdateNotification(&trackerUpdateCompleteCond);
	
	/* Start connection initiating thread: */
	listenThread.start(this,&VRDeviceServer::listenThreadMethod);
	}

VRDeviceServer::~VRDeviceServer(void)
	{
	/* Lock client list: */
	{
	Threads::Mutex::Lock clientListLock(clientListMutex);
	
	/* Stop connection initiating thread: */
	listenThread.cancel();
	listenThread.join();
	
	/* Disconnect all clients: */
	deviceManager->lockState();
	for(ClientList::iterator clIt=clientList.begin();clIt!=clientList.end();++clIt)
		{
		/* Stop client communication thread: */
		(*clIt)->communicationThread.cancel();
		(*clIt)->communicationThread.join();
		
		/* Delete client data object (closing TCP socket): */
		delete *clIt;
		}
	deviceManager->unlockState();
	
	/* Stop VR devices: */
	if(numActiveClients>0)
		deviceManager->stop();
	}
	
	/* Disable tracker update notification: */
	deviceManager->disableTrackerUpdateNotification();
	}
