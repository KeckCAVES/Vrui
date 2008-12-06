/***********************************************************************
SixDofInputDeviceTool - Class for tools using a 6-DOF input device to
interact with virtual input devices.
Copyright (c) 2004-2008 Oliver Kreylos

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

#ifndef VRUI_SIXDOFINPUTDEVICETOOL_INCLUDED
#define VRUI_SIXDOFINPUTDEVICETOOL_INCLUDED

#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/Tools/InputDeviceTool.h>

namespace Vrui {

class SixDofInputDeviceTool;

class SixDofInputDeviceToolFactory:public ToolFactory
	{
	friend class SixDofInputDeviceTool;
	
	/* Constructors and destructors: */
	public:
	SixDofInputDeviceToolFactory(ToolManager& toolManager);
	virtual ~SixDofInputDeviceToolFactory(void);
	
	/* Methods: */
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class SixDofInputDeviceTool:public InputDeviceTool
	{
	friend class SixDofInputDeviceToolFactory;
	
	/* Elements: */
	private:
	static SixDofInputDeviceToolFactory* factory; // Pointer to the factory object for this class
	
	/* Transient dragging state: */
	bool deactivating; // Flag that tool activation has been initiated (second button press)
	TrackerState preScale; // Transformation to be applied to the current transformation before scaling
	
	/* Constructors and destructors: */
	public:
	SixDofInputDeviceTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods: */
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int buttonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	};

}

#endif
