/***********************************************************************
ArtDTrack - Class for ART DTrack tracking devices.
Copyright (c) 2004-2005 Oliver Kreylos

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

#ifndef ARTDTRACK_INCLUDED
#define ARTDTRACK_INCLUDED

#include <Comm/UDPSocket.h>

#include "VRDevice.h"

class ArtDTrack:public VRDevice
	{
	/* Embedded classes: */
	private:
	typedef Vrui::VRDeviceState::TrackerState::PositionOrientation PositionOrientation;
	typedef PositionOrientation::Vector Vector;
	typedef Vector::Scalar VScalar;
	typedef PositionOrientation::Rotation Rotation;
	typedef Rotation::Scalar RScalar;
	
	public:
	enum DataFormat // Enumerated type for data formats
		{
		ASCII,BINARY
		};
	
	private:
	struct Device // Structure describing DTrack devices
		{
		/* Elements: */
		public:
		int id; // Device's DTrack ID
		int numButtons; // Number of buttons associated with the device
		int firstButtonIndex; // Index of first button on device
		int numValuators; // Number of valuators associated with the device
		int firstValuatorIndex; // Index of first valuator on device
		};
	
	/* Elements: */
	Comm::UDPSocket controlSocket; // DTrack control socket
	Comm::UDPSocket dataSocket; // DTrack data socket
	DataFormat dataFormat; // Format of tracking data stream
	Device* devices; // Array of tracked devices
	int maxDeviceId; // Largest ID of any configured tracked device
	int* deviceIdToIndex; // Array mapping from device IDs to device indices
	Vrui::VRDeviceState::TrackerState* trackerStates; // Local copy of all tracker states to fill in missing data
	
	/* Private methods: */
	void processAsciiData(void); // Processes tracking data in ASCII format
	void processBinaryData(void); // Processes tracking data in binary format
	
	/* Protected methods: */
	protected:
	virtual void deviceThreadMethod(void);
	
	/* Constructors and destructors: */
	public:
	ArtDTrack(VRDevice::Factory* sFactory,VRDeviceManager* sDeviceManager,Misc::ConfigurationFile& configFile);
	virtual ~ArtDTrack(void);
	
	/* Methods: */
	virtual void start(void);
	virtual void stop(void);
	};

#endif
