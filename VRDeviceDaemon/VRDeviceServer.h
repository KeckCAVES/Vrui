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

#include <string>
#include <vector>
#include <Threads/EventDispatcher.h>
#include <Comm/ListeningTCPSocket.h>
#include <Vrui/Internal/VRDevicePipe.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFile;
}
namespace Vrui {
class BatteryState;
class HMDConfiguration;
}
class VRDeviceManager;

class VRDeviceServer
	{
	/* Embedded classes: */
	private:
	struct ClientState // Class containing state of connected client
		{
		/* Elements: */
		public:
		VRDeviceServer* server; // Pointer to server object handling this client, to simplify event handling
		Vrui::VRDevicePipe pipe; // Pipe connected to the client
		#ifdef VERBOSE
		std::string clientName; // Name of the client, to keep track of connections in verbose mode
		#endif
		Threads::EventDispatcher::ListenerKey listenerKey; // Key with which this client is listening for I/O events
		int state; // Client's current position in the VRDeviceServer protocol state machine
		unsigned int protocolVersion; // Version of the VR device daemon protocol to use with this client
		bool clientExpectsTimeStamps; // Flag whether the connected client expects to receive time stamp data
		bool clientExpectsValidFlags; // Flag whether the connected client expects to receive tracker valid flags
		bool active; // Flag whether the client is currently active
		bool streaming; // Flag whether client is currently in streaming mode
		
		/* Constructors and destructors: */
		ClientState(VRDeviceServer* sServer,Comm::ListeningTCPSocket& listenSocket); // Accepts next incoming connection on given listening socket and establishes VR device connection
		};
	
	typedef std::vector<ClientState*> ClientStateList; // Data type for lists of states of connected clients
	
	struct BatteryStateVersions // Structure to hold version numbers for a device battery state
		{
		/* Elements: */
		public:
		unsigned int managerVersion; // Battery state's version in device manager
		unsigned int streamingVersion; // Battery state version most recently sent to streaming clients
		
		/* Constructors and destructors: */
		BatteryStateVersions(void)
			:managerVersion(0U),streamingVersion(0U)
			{
			}
		};
	
	struct HMDConfigurationVersions // Structure to hold version numbers for an HMD configuration
		{
		/* Elements: */
		public:
		Vrui::HMDConfiguration* hmdConfiguration; // Pointer to the HMD configuration
		unsigned int eyePosVersion,eyeVersion,distortionMeshVersion; // Version numbers for the three HMD configuration components most recently sent to streaming clients
		
		/* Constructors and destructors: */
		HMDConfigurationVersions(void)
			:hmdConfiguration(0),
			 eyePosVersion(0U),eyeVersion(0U),distortionMeshVersion(0U)
			{
			}
		};
	
	/* Elements: */
	private:
	VRDeviceManager* deviceManager; // Pointer to device manager running in server
	Threads::EventDispatcher dispatcher; // Event dispatcher to handle communication with multiple clients in parallel
	Comm::ListeningTCPSocket listenSocket; // Main socket the server listens on for incoming connections
	ClientStateList clientStates; // List of currently connected clients
	int numActiveClients; // Number of clients that are currently active
	int numStreamingClients; // Number of clients that are currently streaming
	unsigned int managerTrackerStateVersion; // Version number of tracker states in device manager
	unsigned int streamingTrackerStateVersion; // Version number of tracker states most recently sent to streaming clients
	unsigned int managerBatteryStateVersion; // Version number of device battery states in device manager
	unsigned int streamingBatteryStateVersion; // Version number of device battery states most recently sent to streaming clients
	BatteryStateVersions* batteryStateVersions; // Array of battery state version numbers
	unsigned int managerHmdConfigurationVersion; // Version number of HMD configurations in device manager
	unsigned int streamingHmdConfigurationVersion; // Version number of HMD configurations most recently sent to streaming clients
	unsigned int numHmdConfigurations; // Number of HMD configurations in the device manager
	HMDConfigurationVersions* hmdConfigurationVersions; // Array of HMD configuration version numbers
	
	/* Private methods: */
	static bool newConnectionCallback(Threads::EventDispatcher::ListenerKey eventKey,int eventType,void* userData); // Callback called when a connection attempt is made at the listening socket
	void disconnectClient(ClientState* client,bool removeListener,bool removeFromList); // Disconnects the given client due to a communication error; removes listener and/or dead client from list if respective flags are true
	static bool clientMessageCallback(Threads::EventDispatcher::ListenerKey eventKey,int eventType,void* userData); // Callback called when a message from a client arrives
	static void trackerUpdateNotificationCallback(VRDeviceManager* manager,void* userData); // Callback called when tracking device states are updated
	static void batteryStateUpdatedCallback(VRDeviceManager* manager,unsigned int deviceIndex,const Vrui::BatteryState& batteryState,void* userData); // Callback called when a virtual device's battery state has been updated
	static void hmdConfigurationUpdatedCallback(VRDeviceManager* manager,const Vrui::HMDConfiguration* hmdConfiguration,void* userData); // Callback called when the given HMD configuration has been updated
	void disconnectClientOnError(ClientStateList::iterator csIt,const std::runtime_error& err); // Forcefully disconnects a client after a communication error
	bool writeServerState(ClientStateList::iterator csIt); // Writes the device manager's current (locked) state to the given client; returns false on error
	bool writeBatteryState(ClientStateList::iterator csIt,unsigned int deviceIndex); // Writes the device manager's given battery state to the given client; returns false on error
	bool writeHmdConfiguration(ClientStateList::iterator csIt,HMDConfigurationVersions& hmdConfigurationVersions); // Writes the given HMD configuration to the given client; returns false on error
	
	/* Constructors and destructors: */
	public:
	VRDeviceServer(VRDeviceManager* sDeviceManager,const Misc::ConfigurationFile& configFile); // Creates server associated with device manager
	~VRDeviceServer(void);
	
	/* Methods: */
	void run(void); // Runs the server state machine
	void stop(void) // Stops the server state machine; can be called asynchronously
		{
		/* Stop the dispatcher's event handling: */
		dispatcher.stop();
		}
	};
