/***********************************************************************
RayScreenMenuTool - Class for menu selection tools using ray selection
that align menus to screen planes.
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

#ifndef VRUI_RAYSCREENMENUTOOL_INCLUDED
#define VRUI_RAYSCREENMENUTOOL_INCLUDED

#include <Geometry/Ray.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Geometry.h>
#include <Vrui/Tools/MenuTool.h>

/* Forward declarations: */
namespace GLMotif {
class Widget;
}
namespace Vrui {
class Viewer;
}

namespace Vrui {

class RayScreenMenuTool;

class RayScreenMenuToolFactory:public ToolFactory
	{
	friend class RayScreenMenuTool;
	
	/* Elements: */
	private:
	bool useEyeRay; // Flag whether to use an eyeline from the main viewer or the device's ray direction
	bool interactWithWidgets; // Flag if the menu tool doubles as a widget interaction tool
	
	/* Constructors and destructors: */
	public:
	RayScreenMenuToolFactory(ToolManager& toolManager);
	virtual ~RayScreenMenuToolFactory(void);
	
	/* Methods: */
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class RayScreenMenuTool:public MenuTool
	{
	friend class RayScreenMenuToolFactory;
	
	/* Elements: */
	private:
	static RayScreenMenuToolFactory* factory; // Pointer to the factory object for this class
	const Viewer* viewer; // Viewer associated with the menu tool
	
	/* Transient state: */
	Ray selectionRay; // Current selection ray
	bool insideWidget; // Flag if the tool is currently able to interact with a widget
	bool widgetActive; // Flag if the widget tool is currently active
	bool dragging; // Flag if the widget tool is currently dragging a primary top-level widget
	GLMotif::Widget* draggedWidget; // Pointer to currently dragged widget
	NavTrackerState preScale; // Current dragging transformation
	
	/* Private methods: */
	Ray calcSelectionRay(void) const; // Calculates the selection ray based on current device position/orientation
	
	/* Constructors and destructors: */
	public:
	RayScreenMenuTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods: */
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int buttonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

}

#endif
