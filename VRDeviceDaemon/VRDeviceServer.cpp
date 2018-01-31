/***********************************************************************
VRDeviceServer - Class encapsulating the VR device protocol's server
side.
Copyright (c) 2002-2017 Oliver Kreylos

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

#include <VRDeviceDaemon/VRDeviceServer.h>

#include <stdio.h>
#include <stdexcept>
#include <Misc/PrintInteger.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Vrui/Internal/VRDeviceDescriptor.h>
#include <Vrui/Internal/HMDConfiguration.h>

#include <VRDeviceDaemon/VRDeviceManager.h>

#define VRDEVICEDAEMON_DEBUG_PROTOCOL 0

namespace {

/**************
Helper classes:
**************/

enum State
	{
	START,CONNECTED,ACTIVE,STREAMING
	};

}

/********************************************
Methods of class VRDeviceServer::ClientState:
********************************************/

VRDeviceServer::ClientState::ClientState(VRDeviceServer* sServer,Comm::ListeningTCPSocket& listenSocket)
	:server(sServer),
	 pipe(listenSocket),
	 state(START),protocolVersion(Vrui::VRDevicePipe::protocolVersionNumber),clientExpectsTimeStamps(true),
	 active(false),streaming(false)
	{
	#ifdef VERBOSE
	/* Assemble the client name: */
	clientName=pipe.getPeerHostName();
	clientName.push_back(':');
	char portId[10];
	clientName.append(Misc::print(pipe.getPeerPortId(),portId+sizeof(portId)-1));
	#endif
	}

/*******************************
Methods of class VRDeviceServer:
*******************************/

bool VRDeviceServer::newConnectionCallback(Threads::EventDispatcher::ListenerKey eventKey,int eventType,void* userData)
	{
	VRDeviceServer* thisPtr=static_cast<VRDeviceServer*>(userData);
	
	#if VRDEVICEDAEMON_DEBUG_PROTOCOL
	printf("Creating new client state..."); fflush(stdout);
	#endif
	
	/* Create a new client state object and add it to the list: */
	ClientState* newClient=new ClientState(thisPtr,thisPtr->listenSocket);
	
	#if VRDEVICEDAEMON_DEBUG_PROTOCOL
	printf(" done\n");
	#endif
	
	#ifdef VERBOSE
	printf("VRDeviceServer: Connecting new client %s\n",newClient->clientName.c_str());
	fflush(stdout);
	#endif
	
	#if VRDEVICEDAEMON_DEBUG_PROTOCOL
	printf("Adding new client state to list\n");
	#endif
	
	thisPtr->clientStates.push_back(newClient);
	
	#if VRDEVICEDAEMON_DEBUG_PROTOCOL
	printf("Adding listener for client's socket\n");
	#endif
	
	/* Add an event listener for incoming messages from the client: */
	newClient->listenerKey=thisPtr->dispatcher.addIOEventListener(newClient->pipe.getFd(),Threads::EventDispatcher::Read,thisPtr->clientMessageCallback,newClient);
	
	#if VRDEVICEDAEMON_DEBUG_PROTOCOL
	printf("Client connected\n");
	#endif
	
	return false;
	}

void VRDeviceServer::disconnectClient(VRDeviceServer::ClientState* client,bool removeListener,bool removeFromList)
	{
	if(removeListener)
		{
		/* Stop listening on the client's pipe: */
		dispatcher.removeIOEventListener(client->listenerKey);
		}
	
	/* Check if the client is still streaming or active: */
	if(client->streaming)
		--numStreamingClients;
	if(client->active)
		{
		--numActiveClients;
		
		/* Stop VR devices if there are no more active clients: */
		if(numActiveClients==0)
			deviceManager->stop();
		}
	
	/* Disconnect the client: */
	delete client;
	
	if(removeFromList)
		{
		/* Remove the dead client from the list: */
		for(ClientStateList::iterator csIt=clientStates.begin();csIt!=clientStates.end();++csIt)
			if(*csIt==client)
				{
				/* Remove it and stop searching: */
				*csIt=clientStates.back();
				clientStates.pop_back();
				break;
				}
		}
	}

bool VRDeviceServer::clientMessageCallback(Threads::EventDispatcher::ListenerKey eventKey,int eventType,void* userData)
	{
	ClientState* client=static_cast<ClientState*>(userData);
	VRDeviceServer* thisPtr=client->server;
	
	bool result=false;
	
	try
		{
		/* Read some data from the socket into the socket's read buffer and check if client hung up: */
		if(client->pipe.readSomeData()==0)
			throw std::runtime_error("Client terminated connection");
		
		/* Process messages as long as there is data in the read buffer: */
		while(!result&&client->pipe.canReadImmediately())
			{
			#if VRDEVICEDAEMON_DEBUG_PROTOCOL
			printf("Reading message..."); fflush(stdout);
			#endif
			
			/* Read the next message from the client: */
			Vrui::VRDevicePipe::MessageIdType message=client->pipe.readMessage();
			
			#if VRDEVICEDAEMON_DEBUG_PROTOCOL
			printf(" done, %u\n",(unsigned int)message);
			#endif
			
			/* Run the client state machine: */
			switch(client->state)
				{
				case START:
					if(message==Vrui::VRDevicePipe::CONNECT_REQUEST)
						{
						#if VRDEVICEDAEMON_DEBUG_PROTOCOL
						printf("Reading protocol version..."); fflush(stdout);
						#endif
						
						/* Read client's protocol version number: */
						client->protocolVersion=client->pipe.read<Misc::UInt32>();
						
						#if VRDEVICEDAEMON_DEBUG_PROTOCOL
						printf(" done, %u\n",client->protocolVersion);
						#endif
						
						#if VRDEVICEDAEMON_DEBUG_PROTOCOL
						printf("Sending connect reply..."); fflush(stdout);
						#endif
						
						/* Send connect reply message: */
						client->pipe.writeMessage(Vrui::VRDevicePipe::CONNECT_REPLY);
						if(client->protocolVersion>Vrui::VRDevicePipe::protocolVersionNumber)
							client->protocolVersion=Vrui::VRDevicePipe::protocolVersionNumber;
						client->pipe.write<Misc::UInt32>(client->protocolVersion);
						
						/* Send server layout: */
						thisPtr->deviceManager->getState().writeLayout(client->pipe);
						
						/* Check if the client expects virtual device descriptors: */
						if(client->protocolVersion>=2U)
							{
							/* Send the layout of all virtual devices: */
							client->pipe.write<Misc::SInt32>(thisPtr->deviceManager->getNumVirtualDevices());
							for(int deviceIndex=0;deviceIndex<thisPtr->deviceManager->getNumVirtualDevices();++deviceIndex)
								thisPtr->deviceManager->getVirtualDevice(deviceIndex).write(client->pipe,client->protocolVersion);
							}
						
						/* Check if the client expects tracker state time stamps: */
						client->clientExpectsTimeStamps=client->protocolVersion>=3U;
						
						/* Check if the client expects device battery states: */
						if(client->protocolVersion>=5U)
							{
							/* Send all current device battery states: */
							thisPtr->deviceManager->lockBatteryStates();
							for(int deviceIndex=0;deviceIndex<thisPtr->deviceManager->getNumVirtualDevices();++deviceIndex)
								thisPtr->deviceManager->getBatteryState(deviceIndex).write(client->pipe);
							thisPtr->deviceManager->unlockBatteryStates();
							}
						
						/* Check if the client expects HMD configurations: */
						if(client->protocolVersion>=4U)
							{
							/* Send all current HMD configurations to the new client: */
							client->pipe.write<Misc::UInt32>(thisPtr->deviceManager->getNumHmdConfigurations());
							thisPtr->deviceManager->lockHmdConfigurations();
							for(unsigned int i=0;i<thisPtr->numHmdConfigurations;++i)
								{
								/* Send the full configuration to the client: */
								thisPtr->hmdConfigurationVersions[i].hmdConfiguration->write(0U,0U,0U,client->pipe);
								}
							thisPtr->deviceManager->unlockHmdConfigurations();
							}
						
						/* Check if the client expects tracker valid flags: */
						client->clientExpectsValidFlags=client->protocolVersion>=5;
						
						/* Check if the client knows about power and haptic features: */
						if(client->protocolVersion>=6U)
							{
							/* Send the number of power and haptic features: */
							client->pipe.write<Misc::UInt32>(thisPtr->deviceManager->getNumPowerFeatures());
							client->pipe.write<Misc::UInt32>(thisPtr->deviceManager->getNumHapticFeatures());
							}
						
						/* Finish the reply message: */
						client->pipe.flush();
						
						#if VRDEVICEDAEMON_DEBUG_PROTOCOL
						printf(" done\n");
						#endif
						
						/* Go to connected state: */
						client->state=CONNECTED;
						}
					else
						throw std::runtime_error("Protocol error in START state");
					break;
				
				case CONNECTED:
					if(message==Vrui::VRDevicePipe::ACTIVATE_REQUEST)
						{
						/* Start VR devices if this is the first active client: */
						if(thisPtr->numActiveClients==0)
							thisPtr->deviceManager->start();
						++thisPtr->numActiveClients;
						
						/* Go to active state: */
						client->active=true;
						client->state=ACTIVE;
						}
					else if(message==Vrui::VRDevicePipe::DISCONNECT_REQUEST)
						{
						/* Cleanly disconnect this client: */
						#ifdef VERBOSE
						printf("VRDeviceServer: Disconnecting client %s\n",client->clientName.c_str());
						fflush(stdout);
						#endif
						thisPtr->disconnectClient(client,false,true);
						result=true;
						}
					else
						throw std::runtime_error("Protocol error in CONNECTED state");
					break;
				
				case ACTIVE:
					if(message==Vrui::VRDevicePipe::PACKET_REQUEST||message==Vrui::VRDevicePipe::STARTSTREAM_REQUEST)
						{
						#if VRDEVICEDAEMON_DEBUG_PROTOCOL
						printf("Sending packet reply..."); fflush(stdout);
						#endif
						
						/* Send the current server state to the client: */
						client->pipe.writeMessage(Vrui::VRDevicePipe::PACKET_REPLY);
						try
							{
							/* Send server state: */
							thisPtr->deviceManager->lockState();
							thisPtr->deviceManager->getState().write(client->pipe,client->clientExpectsTimeStamps,client->clientExpectsValidFlags);
							thisPtr->deviceManager->unlockState();
							}
						catch(...)
							{
							/* Unlock the device manager's state and throw the exception again: */
							thisPtr->deviceManager->unlockState();
							throw;
							}
						
						/* Finish the reply message: */
						client->pipe.flush();
						
						#if VRDEVICEDAEMON_DEBUG_PROTOCOL
						printf(" done\n");
						#endif
						
						if(message==Vrui::VRDevicePipe::STARTSTREAM_REQUEST)
							{
							/* Increase the number of streaming clients: */
							++thisPtr->numStreamingClients;
							
							/* Go to streaming state: */
							client->streaming=true;
							client->state=STREAMING;
							}
						}
					else if(message==Vrui::VRDevicePipe::POWEROFF_REQUEST)
						{
						/* Read the index of the power feature to power off: */
						unsigned int powerFeatureIndex=client->pipe.read<Misc::UInt16>();
						
						/* Power off the requested feature: */
						thisPtr->deviceManager->powerOff(powerFeatureIndex);
						}
					else if(message==Vrui::VRDevicePipe::HAPTICTICK_REQUEST)
						{
						/* Read the index of the haptic feature and the duration of the haptic tick: */
						unsigned int hapticFeatureIndex=client->pipe.read<Misc::UInt16>();
						unsigned int duration=client->pipe.read<Misc::UInt16>();
						
						/* Request a haptic tick on the requested feature: */
						thisPtr->deviceManager->hapticTick(hapticFeatureIndex,duration);
						}
					else if(message==Vrui::VRDevicePipe::DEACTIVATE_REQUEST)
						{
						/* Stop VR devices if this was the last active clients: */
						--thisPtr->numActiveClients;
						if(thisPtr->numActiveClients==0)
							thisPtr->deviceManager->stop();
						
						/* Go to connected state: */
						client->active=false;
						client->state=CONNECTED;
						}
					else
						throw std::runtime_error("Protocol error in ACTIVE state");
					break;
				
				case STREAMING:
					if(message==Vrui::VRDevicePipe::POWEROFF_REQUEST)
						{
						/* Read the index of the power feature to power off: */
						unsigned int powerFeatureIndex=client->pipe.read<Misc::UInt16>();
						
						/* Power off the requested feature: */
						thisPtr->deviceManager->powerOff(powerFeatureIndex);
						}
					else if(message==Vrui::VRDevicePipe::HAPTICTICK_REQUEST)
						{
						/* Read the index of the haptic feature and the duration of the haptic tick: */
						unsigned int hapticFeatureIndex=client->pipe.read<Misc::UInt16>();
						unsigned int duration=client->pipe.read<Misc::UInt16>();
						
						/* Request a haptic tick on the requested feature: */
						thisPtr->deviceManager->hapticTick(hapticFeatureIndex,duration);
						}
					else if(message==Vrui::VRDevicePipe::STOPSTREAM_REQUEST)
						{
						/* Send stopstream reply message: */
						client->pipe.writeMessage(Vrui::VRDevicePipe::STOPSTREAM_REPLY);
						client->pipe.flush();
						
						/* Decrease the number of streaming clients: */
						--thisPtr->numStreamingClients;
						
						/* Go to active state: */
						client->streaming=false;
						client->state=ACTIVE;
						}
					else if(message!=Vrui::VRDevicePipe::PACKET_REQUEST)
						throw std::runtime_error("Protocol error in STREAMING state");
					break;
				}
			}
		}
	catch(std::runtime_error err)
		{
		#ifdef VERBOSE
		printf("VRDeviceServer: Disconnecting client %s due to exception \"%s\"\n",client->clientName.c_str(),err.what());
		fflush(stdout);
		#endif
		thisPtr->disconnectClient(client,false,true);
		result=true;
		}
	
	return result;
	}

void VRDeviceServer::trackerUpdateNotificationCallback(VRDeviceManager* manager,void* userData)
	{
	VRDeviceServer* thisPtr=static_cast<VRDeviceServer*>(userData);
	
	/* Update the version number of the device manager's tracking state and wake up the run loop: */
	++thisPtr->managerTrackerStateVersion;
	thisPtr->dispatcher.interrupt();
	}

void VRDeviceServer::batteryStateUpdatedCallback(VRDeviceManager* manager,unsigned int deviceIndex,const Vrui::BatteryState& batteryState,void* userData)
	{
	VRDeviceServer* thisPtr=static_cast<VRDeviceServer*>(userData);
	
	/* Update the version number of the device manager's device battery state and wake up the run loop: */
	++thisPtr->batteryStateVersions[deviceIndex].managerVersion;
	++thisPtr->managerBatteryStateVersion;
	thisPtr->dispatcher.interrupt();
	}

void VRDeviceServer::hmdConfigurationUpdatedCallback(VRDeviceManager* manager,const Vrui::HMDConfiguration* hmdConfiguration,void* userData)
	{
	VRDeviceServer* thisPtr=static_cast<VRDeviceServer*>(userData);
	
	/* Update the version number of the device manager's HMD configuration state and wake up the run loop: */
	++thisPtr->managerHmdConfigurationVersion;
	thisPtr->dispatcher.interrupt();
	}

void VRDeviceServer::disconnectClientOnError(VRDeviceServer::ClientStateList::iterator csIt,const std::runtime_error& err)
	{
	/* Print error message to stderr: */
	fprintf(stderr,"VRDeviceServer: Disconnecting client %s due to exception %s\n",(*csIt)->clientName.c_str(),err.what());
	fflush(stderr);
	
	/* Disconnect the client: */
	disconnectClient(*csIt,true,false);
	
	/* Remove the dead client from the list: */
	delete *csIt;
	*csIt=clientStates.back();
	clientStates.pop_back();
	}

bool VRDeviceServer::writeServerState(VRDeviceServer::ClientStateList::iterator csIt)
	{
	/* Bail out if the client is not streaming: */
	ClientState* client=*csIt;
	if(!client->streaming)
		return true;
	
	/* Send state to client: */
	try
		{
		/* Send packet reply message: */
		client->pipe.writeMessage(Vrui::VRDevicePipe::PACKET_REPLY);
		
		/* Send server state: */
		deviceManager->getState().write(client->pipe,client->clientExpectsTimeStamps,client->clientExpectsValidFlags);
		client->pipe.flush();
		}
	catch(std::runtime_error err)
		{
		/* Disconnect the client and signal an error: */
		disconnectClientOnError(csIt,err);
		return false;
		}
	
	return true;
	}

bool VRDeviceServer::writeBatteryState(VRDeviceServer::ClientStateList::iterator csIt,unsigned int deviceIndex)
	{
	/* Bail out if the client is not streaming or can't handle battery states: */
	ClientState* client=*csIt;
	if(!client->streaming||client->protocolVersion<5U)
		return true;
	
	/* Send battery state to client: */
	try
		{
		/* Send battery state update message: */
		client->pipe.writeMessage(Vrui::VRDevicePipe::BATTERYSTATE_UPDATE);
		
		/* Send virtual device index: */
		client->pipe.write<Misc::UInt16>(deviceIndex);
		
		/* Send battery state: */
		deviceManager->getBatteryState(deviceIndex).write(client->pipe);
		client->pipe.flush();
		}
	catch(std::runtime_error err)
		{
		/* Disconnect the client and signal an error: */
		disconnectClientOnError(csIt,err);
		return false;
		}
	
	return true;
	}

bool VRDeviceServer::writeHmdConfiguration(VRDeviceServer::ClientStateList::iterator csIt,VRDeviceServer::HMDConfigurationVersions& hmdConfigurationVersions)
	{
	/* Bail out if the client is not streaming or can't handle HMD configurations: */
	ClientState* client=*csIt;
	if(!client->streaming||client->protocolVersion<4U)
		return true;
	
	try
		{
		/* Send HMD configuration to client: */
		hmdConfigurationVersions.hmdConfiguration->write(hmdConfigurationVersions.eyePosVersion,hmdConfigurationVersions.eyeVersion,hmdConfigurationVersions.distortionMeshVersion,client->pipe);
		client->pipe.flush();
		}
	catch(std::runtime_error err)
		{
		/* Disconnect the client and signal an error: */
		disconnectClientOnError(csIt,err);
		return false;
		}
	
	return true;
	}

VRDeviceServer::VRDeviceServer(VRDeviceManager* sDeviceManager,const Misc::ConfigurationFile& configFile)
	:deviceManager(sDeviceManager),
	 listenSocket(configFile.retrieveValue<int>("./serverPort",-1),5),
	 numActiveClients(0),numStreamingClients(0),
	 managerTrackerStateVersion(0U),streamingTrackerStateVersion(0U),
	 managerBatteryStateVersion(0U),streamingBatteryStateVersion(0U),batteryStateVersions(0),
	 managerHmdConfigurationVersion(0U),streamingHmdConfigurationVersion(0U),
	 numHmdConfigurations(deviceManager->getNumHmdConfigurations()),hmdConfigurationVersions(0)
	{
	/* Add an event listener for incoming connections on the listening socket: */
	dispatcher.addIOEventListener(listenSocket.getFd(),Threads::EventDispatcher::Read,newConnectionCallback,this);
	
	/* Initialize the array of battery state version numbers: */
	batteryStateVersions=new BatteryStateVersions[deviceManager->getNumVirtualDevices()];
	
	/* Initialize the array of HMD configuration version numbers: */
	hmdConfigurationVersions=new HMDConfigurationVersions[numHmdConfigurations];
	for(unsigned int i=0;i<deviceManager->getNumHmdConfigurations();++i)
		hmdConfigurationVersions[i].hmdConfiguration=&deviceManager->getHmdConfiguration(i);
	}

VRDeviceServer::~VRDeviceServer(void)
	{
	/* Stop VR devices if there are still active clients: */
	if(numActiveClients>0)
		deviceManager->stop();
	
	/* Forcefully disconnect all clients: */
	for(ClientStateList::iterator csIt=clientStates.begin();csIt!=clientStates.end();++csIt)
		delete *csIt;
	
	/* Clean up: */
	delete[] batteryStateVersions;
	delete[] hmdConfigurationVersions;
	}

void VRDeviceServer::run(void)
	{
	#ifdef VERBOSE
	printf("VRDeviceServer: Listening for incoming connections on TCP port %d\n",listenSocket.getPortId());
	fflush(stdout);
	#endif
	
	/* Enable tracker update, battery state, and HMD configuration change notifications: */
	deviceManager->enableTrackerUpdateNotification(trackerUpdateNotificationCallback,this);
	deviceManager->setBatteryStateUpdatedCallback(batteryStateUpdatedCallback,this);
	deviceManager->setHmdConfigurationUpdatedCallback(hmdConfigurationUpdatedCallback,this);
	
	/* Run the main loop and dispatch events until stopped: */
	while(dispatcher.dispatchNextEvent())
		{
		/* Check if a streaming update needs to be sent: */
		if(numStreamingClients>0&&streamingTrackerStateVersion!=managerTrackerStateVersion)
			{
			/* Lock the current server state: */
			deviceManager->lockState();
			
			/* Send a state update to all clients in streaming mode: */
			for(ClientStateList::iterator csIt=clientStates.begin();csIt!=clientStates.end();++csIt)
				if(!writeServerState(csIt))
					--csIt;
			
			/* Unlock the current server state: */
			deviceManager->unlockState();
			
			/* Mark streaming state as up-to-date: */
			streamingTrackerStateVersion=managerTrackerStateVersion;
			}
		
		/* Check if any device battery states need to be sent: */
		if(streamingBatteryStateVersion!=managerBatteryStateVersion)
			{
			deviceManager->lockBatteryStates();
			
			for(int i=0;i<deviceManager->getNumVirtualDevices();++i)
				{
				if(batteryStateVersions[i].streamingVersion!=batteryStateVersions[i].managerVersion)
					{
					#ifdef VERBOSE
					printf("VRDeviceServer: Sending updated battery state %d to clients\n",i);
					fflush(stdout);
					#endif
					
					/* Send the new HMD configuration to all clients that are streaming and can handle it: */
					for(ClientStateList::iterator csIt=clientStates.begin();csIt!=clientStates.end();++csIt)
						if(!writeBatteryState(csIt,i))
							--csIt;
					
					/* Mark the battery state as up-to-date: */
					batteryStateVersions[i].streamingVersion=batteryStateVersions[i].managerVersion;
					}
				}
			
			deviceManager->unlockBatteryStates();
			
			/* Mark device battery state as up-to-date: */
			streamingBatteryStateVersion=managerBatteryStateVersion;
			}
		
		/* Check if any HMD configuration updates need to be sent: */
		if(streamingHmdConfigurationVersion!=managerHmdConfigurationVersion)
			{
			deviceManager->lockHmdConfigurations();
			
			for(unsigned int i=0;i<numHmdConfigurations;++i)
				{
				/* Check if this HMD configuration has been updated: */
				Vrui::HMDConfiguration& hc=*hmdConfigurationVersions[i].hmdConfiguration;
				if(hmdConfigurationVersions[i].eyePosVersion!=hc.getEyePosVersion()||hmdConfigurationVersions[i].eyeVersion!=hc.getEyeVersion()||hmdConfigurationVersions[i].distortionMeshVersion!=hc.getDistortionMeshVersion())
					{
					#ifdef VERBOSE
					printf("VRDeviceServer: Sending updated HMD configuration %u to clients\n",i);
					fflush(stdout);
					#endif
					
					/* Send the new HMD configuration to all clients that are streaming and can handle it: */
					for(ClientStateList::iterator csIt=clientStates.begin();csIt!=clientStates.end();++csIt)
						if(!writeHmdConfiguration(csIt,hmdConfigurationVersions[i]))
							--csIt;
					
					/* Mark the HMD configuration as up-to-date: */
					hmdConfigurationVersions[i].eyePosVersion=hc.getEyePosVersion();
					hmdConfigurationVersions[i].eyeVersion=hc.getEyeVersion();
					hmdConfigurationVersions[i].distortionMeshVersion=hc.getDistortionMeshVersion();
					}
				}
			
			deviceManager->unlockHmdConfigurations();
			
			/* Mark HMD configuration state as up-to-date: */
			streamingHmdConfigurationVersion=managerHmdConfigurationVersion;
			}
		}
	
	/* Disable tracker update notifications: */
	deviceManager->disableTrackerUpdateNotification();
	}
