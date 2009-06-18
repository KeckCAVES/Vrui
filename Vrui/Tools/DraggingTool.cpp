/***********************************************************************
DraggingTool - Base class for tools encapsulating 6-DOF dragging
operations.
Copyright (c) 2004-2009 Oliver Kreylos

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

#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

#include <Vrui/Tools/DraggingTool.h>

namespace Vrui {

/************************************
Methods of class DraggingToolFactory:
************************************/

DraggingToolFactory::DraggingToolFactory(ToolManager& toolManager)
	:ToolFactory("DraggingTool",toolManager)
	{
	#if 0
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("Tool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	#endif
	}

const char* DraggingToolFactory::getName(void) const
	{
	return "Dragger";
	}

/*****************************
Methods of class DraggingTool:
*****************************/

DraggingTool::DraggingTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:Tool(factory,inputAssignment)
	{
	}

}
