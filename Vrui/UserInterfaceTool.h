/***********************************************************************
UserInterfaceTool - Base class for tools related to user interfaces
(interaction with dialog boxes, context menus, virtual input devices).
Copyright (c) 2008-2010 Oliver Kreylos

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

#ifndef VRUI_USERINTERFACETOOL_INCLUDED
#define VRUI_USERINTERFACETOOL_INCLUDED

#include <Geometry/Ray.h>
#include <Vrui/Geometry.h>
#include <Vrui/Tool.h>

namespace Vrui {

/* Forward declarations: */
class UserInterfaceTool;

class UserInterfaceToolFactory:public ToolFactory
	{
	friend class UserInterfaceTool;
	
	/* Elements: */
	private:
	bool useEyeRay; // Flag whether to use an eyeline from the main viewer or the device's ray direction for ray-based interaction
	Scalar rayOffset; // Amount by which to shift the selection ray backwards to simplify interaction
	
	/* Constructors and destructors: */
	public:
	UserInterfaceToolFactory(ToolManager& toolManager);
	virtual ~UserInterfaceToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	};

class UserInterfaceTool:public Tool
	{
	friend class UserInterfaceToolFactory;
	
	/* Elements: */
	private:
	static UserInterfaceToolFactory* factory; // Pointer to the factory object for this class
	protected:
	InputDevice* interactionDevice; // Pointer to the input device used for user interface interaction
	
	/* Protected methods: */
	protected:
	bool isUseEyeRay(void) const // Returns true if input devices use eyelines for ray-based interaction
		{
		return factory->useEyeRay;
		}
	Scalar getRayOffset(void) const // Returns the ray origin offset for ray-based interaction
		{
		return factory->rayOffset;
		}
	Point getInteractionPosition(void) const // Returns a position for point-based interaction
		{
		return interactionDevice->getPosition();
		}
	Ray calcInteractionRay(void) const; // Returns a ray for ray-based interaction
	ONTransform calcScreenTransform(const Ray& ray) const; // Returns a screen-aligned transformation where the given ray intersects a screen
	
	/* Constructors and destructors: */
	public:
	UserInterfaceTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	};

}

#endif
