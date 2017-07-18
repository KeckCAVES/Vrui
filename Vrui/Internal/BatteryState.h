/***********************************************************************
BatteryState - Class to represent the battery state of a virtual device.
Copyright (c) 2017 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef VRUI_INTERNAL_BATTERYSTATE_INCLUDED
#define VRUI_INTERNAL_BATTERYSTATE_INCLUDED

#include <Misc/SizedTypes.h>
#include <Vrui/Internal/VRDevicePipe.h>

namespace Vrui {

class BatteryState
	{
	/* Elements: */
	public:
	bool charging; // Flag if the device is currently charging
	unsigned int batteryLevel; // Device's current battery level in percent
	
	/* Constructors and destructors: */
	BatteryState(void) // Creates the battery state of a plugged-in device
		:charging(true),batteryLevel(100U)
		{
		}
	
	/* Methods: */
	void write(VRDevicePipe& sink) const // Writes battery state to the given sink
		{
		sink.write<Misc::UInt8>(charging?1:0);
		sink.write<Misc::UInt8>(Misc::UInt8(batteryLevel));
		}
	void read(VRDevicePipe& source) // Reads battery state from the given source
		{
		charging=source.read<Misc::UInt8>()!=0;
		batteryLevel=source.read<Misc::UInt8>();
		}
	};

}

#endif
