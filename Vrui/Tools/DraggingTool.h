/***********************************************************************
DraggingTool - Base class for tools encapsulating 6-DOF dragging
operations.
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

#ifndef VRUI_DRAGGINGTOOL_INCLUDED
#define VRUI_DRAGGINGTOOL_INCLUDED

#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/Ray.h>
#include <Vrui/Geometry.h>
#include <Vrui/Tools/Tool.h>

/* Forward declarations: */
namespace Vrui {
class ToolManager;
}

namespace Vrui {

class DraggingTool;

class DraggingToolFactory:public ToolFactory
	{
	friend class DraggingTool;
	
	/* Constructors and destructors: */
	public:
	DraggingToolFactory(ToolManager& toolManager);
	};

class DraggingTool:public Tool
	{
	friend class DraggingToolFactory;
	
	/* Embedded classes: */
	public:
	class IdleMotionCallbackData:public Misc::CallbackData
		{
		/* Elements: */
		public:
		DraggingTool* tool; // Pointer to the tool causing the event
		const NavTrackerState& currentTransformation; // Current motion transformation
		
		/* Constructors and destructors: */
		IdleMotionCallbackData(DraggingTool* sTool,const NavTrackerState& sCurrentTransformation)
			:tool(sTool),
			 currentTransformation(sCurrentTransformation)
			{
			}
		};
	
	class DragStartCallbackData:public Misc::CallbackData
		{
		/* Elements: */
		public:
		DraggingTool* tool; // Pointer to the tool causing the event
		const NavTrackerState& startTransformation; // Transformation before dragging starts
		bool rayBased; // True if the dragging selection should be based on a ray
		Ray ray; // Ray for the dragging selection
		
		/* Constructors and destructors: */
		DragStartCallbackData(DraggingTool* sTool,const NavTrackerState& sStartTransformation)
			:tool(sTool),
			 startTransformation(sStartTransformation),
			 rayBased(false)
			{
			}
		
		/* Methods: */
		void setRay(const Ray& newRay)
			{
			rayBased=true;
			ray=newRay;
			}
		};
	
	class DragCallbackData:public Misc::CallbackData
		{
		/* Elements: */
		public:
		DraggingTool* tool; // Pointer to the tool causing the event
		const NavTrackerState& currentTransformation; // Current dragging transformation
		const NavTrackerState& incrementTransformation; // Incremental transformation from initial to current transformation
		
		/* Constructors and destructors: */
		DragCallbackData(DraggingTool* sTool,const NavTrackerState& sCurrentTransformation,const NavTrackerState& sIncrementTransformation)
			:tool(sTool),
			 currentTransformation(sCurrentTransformation),
			 incrementTransformation(sIncrementTransformation)
			{
			}
		};
	
	class DragEndCallbackData:public Misc::CallbackData
		{
		/* Elements: */
		public:
		DraggingTool* tool; // Pointer to the tool causing the event
		const NavTrackerState& finalTransformation; // Final dragging transformation
		const NavTrackerState& incrementTransformation; // Incremental transformation from initial to final transformation
		
		/* Constructors and destructors: */
		DragEndCallbackData(DraggingTool* sTool,const NavTrackerState& sFinalTransformation,const NavTrackerState& sIncrementTransformation)
			:tool(sTool),
			 finalTransformation(sFinalTransformation),
			 incrementTransformation(sIncrementTransformation)
			{
			}
		};
	
	/* Elements: */
	protected:
	Misc::CallbackList idleMotionCallbacks; // List of callbacks to be called when not dragging
	Misc::CallbackList dragStartCallbacks; // List of callbacks to be called when dragging starts
	Misc::CallbackList dragCallbacks; // List of callbacks to be called during dragging
	Misc::CallbackList dragEndCallbacks; // List of callbacks to be called when dragging ends
	
	/* Constructors and destructors: */
	public:
	DraggingTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* New methods: */
	Misc::CallbackList& getIdleMotionCallbacks(void) // Returns list of idle motion callbacks
		{
		return idleMotionCallbacks;
		}
	Misc::CallbackList& getDragStartCallbacks(void) // Returns list of drag start callbacks
		{
		return dragStartCallbacks;
		}
	Misc::CallbackList& getDragCallbacks(void) // Returns list of drag callbacks
		{
		return dragCallbacks;
		}
	Misc::CallbackList& getDragEndCallbacks(void) // Returns list of drag end callbacks
		{
		return dragEndCallbacks;
		}
	};

}

#endif
