/***********************************************************************
GeodeticToCartesianPointTransformNode - Point transformation class to
convert geodetic coordinates (longitude/latitude/altitude on a reference
ellipsoid) to Cartesian coordinates.
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

#ifndef SCENEGRAPH_GEODETICTOCARTESIANPOINTTRANSFORMNODE_INCLUDED
#define SCENEGRAPH_GEODETICTOCARTESIANPOINTTRANSFORMNODE_INCLUDED

#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/PointTransformNode.h>
#include <SceneGraph/ReferenceEllipsoidNode.h>

namespace SceneGraph {

class GeodeticToCartesianPointTransformNode:public PointTransformNode
	{
	/* Embedded classes: */
	public:
	typedef SF<ReferenceEllipsoidNodePointer> SFReferenceEllipsoidNode;
	
	/* Elements: */
	
	/* Fields: */
	public:
	SFReferenceEllipsoidNode referenceEllipsoid;
	SFBool longitudeFirst;
	SFBool degrees;
	SFBool colatitude;
	
	/* Constructors and destructors: */
	public:
	GeodeticToCartesianPointTransformNode(void); // Creates a default node
	
	/* Methods from Node: */
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	
	/* Methods from PointTransformNode: */
	virtual Point transformPoint(const Point& point) const;
	virtual Box calcBoundingBox(const std::vector<Point>& points) const;
	virtual Vector transformVector(const Point& basePoint,const Vector& vector) const;
	virtual Vector transformNormal(const Point& basePoint,const Vector& normal) const;
	};

}

#endif
