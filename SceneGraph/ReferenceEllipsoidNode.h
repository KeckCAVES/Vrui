/***********************************************************************
ReferenceEllipsoidNode - Class for nodes defining reference ellipsoid
(geodesic data) for geodesic coordinate systems and transformations
between them.
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

#ifndef SCENEGRAPH_REFERENCEELLIPSOIDNODE_INCLUDED
#define SCENEGRAPH_REFERENCEELLIPSOIDNODE_INCLUDED

#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <SceneGraph/Geometry.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/Node.h>

namespace SceneGraph {

class ReferenceEllipsoidNode:public Node
	{
	/* Embedded classes: */
	public:
	typedef SF<double> SFDouble;
	
	/* Elements: */
	
	/* Fields: */
	public:
	SFDouble radius;
	SFDouble flattening;
	SFDouble scale;
	
	/* Derived state: */
	protected:
	double r; // Radius in meters after scaling
	double f; // Flattening factor
	double e2; // Eccentricity
	
	/* Constructors and destructors: */
	public:
	ReferenceEllipsoidNode(void); // Creates reference ellipsoid node with default settings
	
	/* Methods from Node: */
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	
	/* New methods: */
	Point geodeticToCartesianPoint(double longitude,double latitude,double height) const // Transforms a point from geodetic coordinates in radians and meters to scaled Cartesian coordinates
		{
		double sLon=Math::sin(longitude);
		double cLon=Math::cos(longitude);
		double sLat=Math::sin(latitude);
		double cLat=Math::cos(latitude);
		double chi=Math::sqrt(1.0-e2*sLat*sLat);
		return Point(Scalar((r/chi+height)*cLat*cLon),Scalar((r/chi+height)*cLat*sLon),Scalar((r*(1.0-e2)/chi+height)*sLat));
		}
	OGTransform geodeticToCartesianFrame(double longitude,double latitude,double height) const // Returns a scaled Cartesian coordinate frame at the geodetic coordinates in radians and meters; x points east, y north, z up
		{
		double sLon=Math::sin(longitude);
		double cLon=Math::cos(longitude);
		double sLat=Math::sin(latitude);
		double cLat=Math::cos(latitude);
		double chi=Math::sqrt(1.0-e2*sLat*sLat);
		Vector translation(Scalar((r/chi+height)*cLat*cLon),Scalar((r/chi+height)*cLat*sLon),Scalar((r*(1.0-e2)/chi+height)*sLat));
		
		Rotation rotation=Rotation::rotateZ(Scalar(0.5*Math::Constants<double>::pi+longitude));
		rotation*=Rotation::rotateX(Scalar(0.5*Math::Constants<double>::pi-latitude));
		
		return OGTransform(translation,rotation,Scalar(scale.getValue()));
		}
	};

typedef Misc::Autopointer<ReferenceEllipsoidNode> ReferenceEllipsoidNodePointer;

}

#endif
