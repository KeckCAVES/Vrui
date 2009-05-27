/***********************************************************************
DaisyWheelTool - Class for tools to enter text by pointing at characters
on a dynamic daisy wheel.
Copyright (c) 2008-2009 Oliver Kreylos

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

#ifndef VRUI_DAISYWHEELTOOL_INCLUDED
#define VRUI_DAISYWHEELTOOL_INCLUDED

#include <Geometry/Ray.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/Geometry.h>
#include <Vrui/Tools/UserInterfaceTool.h>

/* Forward declarations: */
namespace Vrui {
class ToolManager;
}

namespace Vrui {

class DaisyWheelTool;

class DaisyWheelToolFactory:public ToolFactory
	{
	friend class DaisyWheelTool;
	
	/* Elements: */
	private:
	Scalar innerRadius,outerRadius; // Inner and outer radius of daisy wheel in physical coordinate units
	
	/* Constructors and destructors: */
	public:
	DaisyWheelToolFactory(ToolManager& toolManager);
	virtual ~DaisyWheelToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class DaisyWheelTool:public UserInterfaceTool
	{
	friend class DaisyWheelToolFactory;
	
	/* Elements: */
	private:
	static DaisyWheelToolFactory* factory; // Pointer to the factory object for this class
	
	/* Transient state: */
	int numCharacters; // Number of characters available on the daisy wheel
	Scalar* baseWeights; // Array of weights for each character before dynamic scaling
	Scalar baseWeightSum; // Current sum of base weights for all characters
	Scalar* dynamicWeights; // Array of dynamic weights for each character
	Scalar dynamicWeightSum; // Current sum of dynamic weights for all characters
	bool active; // Flag if the tool is currently active
	ONTransform wheelTransform; // Transformation from wheel coordinates to physical coordinates
	Ray selectionRay; // Current selection ray
	Scalar zoomAngle; // Center angle of zoom region in radians
	Scalar zoomStrength; // Zooming strength factor
	
	/* Constructors and destructors: */
	public:
	DaisyWheelTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~DaisyWheelTool(void);
	
	/* Methods from Tool: */
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int buttonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

}

#endif
