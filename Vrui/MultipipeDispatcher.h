/***********************************************************************
MultipipeDispatcher - Class to distribute input device and ancillary
data between the nodes in a multipipe VR environment.
Copyright (c) 2004-2005 Oliver Kreylos

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

#ifndef VRUI_MULTIPIPEDISPATCHER_INCLUDED
#define VRUI_MULTIPIPEDISPATCHER_INCLUDED

#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/Geometry.h>

/* Forward declarations: */
namespace Comm {
class MulticastPipe;
}
namespace Vrui {
class InputDeviceManager;
}

namespace Vrui {

class MultipipeDispatcher
	{
	/* Embedded classes: */
	private:
	struct InputDeviceTrackingState // Structure for current input device tracking states
		{
		/* Elements: */
		public:
		TrackerState transformation;
		Vector linearVelocity;
		Vector angularVelocity;
		};
	
	/* Elements: */
	private:
	Comm::MulticastPipe* pipe; // Multicast pipe connecting the master node to all slave nodes
	InputDeviceManager* inputDeviceManager; // Pointer to input device manager on this node
	int numInputDevices; // Number of dispatched input devices
	int totalNumButtons; // Total number of buttons on all dispatched input devices
	int totalNumValuators; // Total number of valuators on all dispatched input devices
	InputDeviceTrackingState* trackingStates; // Array of input device tracking states
	bool* buttonStates; // Array of input device button states
	double* valuatorStates; // Array of input device valuator states
	
	/* Constructors and destructors: */
	public:
	MultipipeDispatcher(Comm::MulticastPipe* sPipe,InputDeviceManager* sInputDeviceManager);
	~MultipipeDispatcher(void);
	
	/* Methods: */
	void dispatchState(void); // Dispatches input device states to all nodes
	};

}

#endif
