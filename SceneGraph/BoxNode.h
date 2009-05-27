/***********************************************************************
BoxNode - Class for axis-aligned boxes as renderable geometry.
Copyright (c) 2009 Oliver Kreylos

This file is part of the Simple Scene Graph Renderer (SceneGraph).

The Simple Scene Graph Renderer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Simple Scene Graph Renderer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Simple Scene Graph Renderer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SCENEGRAPH_BOXNODE_INCLUDED
#define SCENEGRAPH_BOXNODE_INCLUDED

#include <Geometry/ComponentArray.h>
#include <Geometry/Point.h>
#include <Geometry/Box.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GeometryNode.h>

namespace SceneGraph {

class BoxNode:public GeometryNode
	{
	/* Elements: */
	protected:
	
	/* Fields: */
	SFPoint center;
	SFSize size;
	
	/* Derived state: */
	Box box; // The position and size of the axis-aligned box
	
	/* Constructors and destructors: */
	public:
	BoxNode(void); // Creates a default box (centered at origin, edge length 2)
	
	/* Methods from Node: */
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	
	/* Methods from GeometryNode: */
	virtual Box calcBoundingBox(void) const;
	virtual void glRenderAction(GLRenderState& renderState) const;
	
	/* New methods: */
	void setBox(const Box& newBox); // Sets the position and size of the axis-aligned box
	void setBox(const Point& newCenter,const Size& newSize); // Ditto, with center and size
	void setBox(const Size& newSize); // Ditto, with size around the origin
	};

}

#endif
