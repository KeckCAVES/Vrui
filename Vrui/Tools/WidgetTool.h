/***********************************************************************
WidgetTool - Class for tools that can interact with GLMotif GUI widgets.
WidgetTool objects are cascadable and preempt button events if they
would fall into the area of interest of mapped widgets.
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

#ifndef VRUI_WIDGETTOOL_INCLUDED
#define VRUI_WIDGETTOOL_INCLUDED

#include <Geometry/Ray.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Geometry.h>
#include <Vrui/Tools/UserInterfaceTool.h>

/* Forward declarations: */
namespace GLMotif {
class Widget;
}
namespace Vrui {
class ToolManager;
}

namespace Vrui {

class WidgetTool;

class WidgetToolFactory:public ToolFactory
	{
	friend class WidgetTool;
	
	/* Constructors and destructors: */
	public:
	WidgetToolFactory(ToolManager& toolManager);
	virtual ~WidgetToolFactory(void);
	
	/* Methods: */
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class WidgetTool:public UserInterfaceTool
	{
	friend class WidgetToolFactory;
	
	/* Elements: */
	private:
	static WidgetToolFactory* factory; // Pointer to the factory object for this class
	
	/* Transient state: */
	bool insideWidget; // Flag if the tool is currently able to interact with a widget
	bool active; // Flag if the tool is currently active
	Ray selectionRay; // Current selection ray
	bool dragging; // Flag if the tool is currently dragging a primary top-level widget
	GLMotif::Widget* draggedWidget; // Pointer to currently dragged widget
	NavTrackerState preScale; // Current dragging transformation
	
	/* Private methods: */
	Ray calcSelectionRay(void) const; // Calculates the selection ray based on current device position/orientation
	
	/* Constructors and destructors: */
	public:
	WidgetTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods: */
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int buttonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

}

#endif
