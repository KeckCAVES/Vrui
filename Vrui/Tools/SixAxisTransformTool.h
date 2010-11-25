/***********************************************************************
SixAxisTransformTool - Class to convert an input device with six
valuators into a virtual 6-DOF input device.
Copyright (c) 2010 Oliver Kreylos

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

#ifndef VRUI_SIXAXISTRANSFORMTOOL_INCLUDED
#define VRUI_SIXAXISTRANSFORMTOOL_INCLUDED

#include <Geometry/Vector.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/TransformTool.h>

namespace Vrui {

class SixAxisTransformTool;

class SixAxisTransformToolFactory:public ToolFactory
	{
	friend class SixAxisTransformTool;
	
	/* Elements: */
	private:
	Point homePosition; // Position at which to create the device, and to which to return it when the home button is pressed
	Vector translations[3]; // Translation vectors
	Vector rotations[3]; // Scaled rotation axes
	Glyph deviceGlyph; // Glyph used to visualize the device's position and orientation
	
	/* Constructors and destructors: */
	public:
	SixAxisTransformToolFactory(ToolManager& toolManager);
	virtual ~SixAxisTransformToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual const char* getValuatorFunction(int valuatorSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class SixAxisTransformTool:public TransformTool
	{
	friend class SixAxisTransformToolFactory;
	
	/* Elements: */
	private:
	static SixAxisTransformToolFactory* factory; // Pointer to the factory object for this class
	
	/* Constructors and destructors: */
	public:
	SixAxisTransformTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods from Tool: */
	virtual void initialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	};

}

#endif
