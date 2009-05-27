/***********************************************************************
PointTransformNode - Base class for nodes that define non-linear
transformations that can be applied to the point coordinates and normal
vectors of Geometry nodes.
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

#ifndef SCENEGRAPH_POINTTRANSFORMNODE_INCLUDED
#define SCENEGRAPH_POINTTRANSFORMNODE_INCLUDED

#include <vector>
#include <Misc/Autopointer.h>
#include <SceneGraph/Geometry.h>
#include <SceneGraph/Node.h>

namespace SceneGraph {

class PointTransformNode:public Node
	{
	/* New methods: */
	public:
	virtual Point transformPoint(const Point& point) const =0; // Transforms a point
	virtual Box calcBoundingBox(const std::vector<Point>& points) const =0; // Calculates transformed bounding box of a point list
	virtual Vector transformVector(const Point& basePoint,const Vector& vector) const =0; // Transforms a vector based at the given point
	virtual Vector transformNormal(const Point& basePoint,const Vector& normal) const =0; // Transforms a normal vector based at the given point
	};

typedef Misc::Autopointer<PointTransformNode> PointTransformNodePointer;

}

#endif
