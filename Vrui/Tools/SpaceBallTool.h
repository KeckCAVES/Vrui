/***********************************************************************
SpaceBallTool - Class to abstract a raw SpaceBall relative 6-DOF device
into an absolute 6-DOF virtual input device.
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

#ifndef VRUI_SPACEBALLTOOL_INCLUDED
#define VRUI_SPACEBALLTOOL_INCLUDED

#include <Vrui/Tools/Tool.h>

/* Forward declarations: */
namespace Vrui {
class Glyph;
}

namespace Vrui {

class SpaceBallTool;

class SpaceBallToolFactory:public ToolFactory
	{
	friend class SpaceBallTool;
	
	/* Elements: */
	private:
	Scalar translateFactor; // Conversion factor from SpaceBall valuator values to physical units
	Scalar rotateFactor; // Conversion factor from SpaceBall valuator values to radians
	bool buttonToggleFlags[12]; // Flag whether each SpaceBall button acts as a toggle
	Glyph deviceGlyph; // Glyph to be used for virtual SpaceBall devices
	
	/* Constructors and destructors: */
	public:
	SpaceBallToolFactory(ToolManager& toolManager);
	virtual ~SpaceBallToolFactory(void);
	
	/* Methods: */
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class SpaceBallTool:public Tool
	{
	friend class SpaceBallToolFactory;
	
	/* Elements: */
	private:
	static SpaceBallToolFactory* factory; // Pointer to the factory object for this class
	InputDevice* spaceBall; // Pointer to the virtual SpaceBall input device
	bool toggleButtonStates[12]; // Current state of all simulated toggle buttons
	
	/* Constructors and destructors: */
	public:
	SpaceBallTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~SpaceBallTool(void);
	
	/* Methods: */
	virtual void initialize(void);
	virtual void deinitialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int buttonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	};

}

#endif
