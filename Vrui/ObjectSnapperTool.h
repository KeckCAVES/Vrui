/***********************************************************************
ObjectSnapperTool - Tool class to snap a virtual input device's position
and/or orientation to application-specified objects using a callback
mechanism.
Copyright (c) 2017-2018 Oliver Kreylos

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

#ifndef VRUI_OBJECTSNAPPERTOOL_INCLUDED
#define VRUI_OBJECTSNAPPERTOOL_INCLUDED

#include <vector>
#include <Geometry/Point.h>
#include <Geometry/Ray.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/TransformTool.h>

/* Forward declarations: */
namespace Misc {
template <class ParameterParam>
class FunctionCall;
}

namespace Vrui {

class ObjectSnapperTool; // Forward declaration of the object snapper tool class

class ObjectSnapperToolFactory:public ToolFactory // Class for factories that create/destroy object snapper tool objects
	{
	friend class ObjectSnapperTool;
	
	/* Embedded classes: */
	public:
	struct SnapRequest // Request structure passed to snap callbacks
		{
		/* Elements: */
		public:
		ObjectSnapperTool* tool; // Pointer to the tool that caused the request
		ONTransform toolTransform; // Position and orientation of the requesting tool in navigational coordinates
		bool rayBased; // Flag whether this snap request should use ray-based selection instead of point-based selection
		Ray snapRay; // Equation of selection ray in navigational coordinates
		Scalar snapRayCosine; // Cosine of opening angle of cone around selection ray
		Scalar snapRayMax; // Length of selection ray; updated by callee after successful snap
		Point snapPosition; // Position of selection point in navigational coordinates
		Scalar snapRadius; // Radius of selection sphere around selection point in navigational coordinate units; updated by callee after successful snap
		bool snapped; // Flag whether the snap request was successful and snapResult is filled
		ONTransform snapResult; // Result of a successful snap request
		
		/* Methods: */
		bool snapPoint(const Point& p); // Convenience method to snap against a point; returns true if snap succeeded
		};
	
	typedef Misc::FunctionCall<SnapRequest&> SnapFunction; // Type for snapping function calls
	private:
	typedef std::vector<SnapFunction*> SnapFunctionList; // Type for lists of snapping function calls
	
	/* Elements: */
	SnapFunctionList snapCallbacks; // List of currently registered snap callbacks
	
	/* Constructors and destructors: */
	public:
	ObjectSnapperToolFactory(ToolManager& toolManager);
	virtual ~ObjectSnapperToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class ObjectSnapperTool:public TransformTool
	{
	friend class ObjectSnapperToolFactory;
	
	/* Elements: */
	private:
	static ObjectSnapperToolFactory* factory; // Pointer to the factory object for this class
	
	/* Constructors and destructors: */
	public:
	ObjectSnapperTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~ObjectSnapperTool(void);
	
	/* Methods from Tool: */
	virtual void initialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void frame(void);
	
	/* New methods: */
	static void addSnapCallback(ObjectSnapperToolFactory::SnapFunction* newSnapFunction); // Registers additional snap callback with all object snapper tools; inherits function call object
	};

}

#endif
