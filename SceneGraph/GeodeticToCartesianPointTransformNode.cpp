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

#include <SceneGraph/GeodeticToCartesianPointTransformNode.h>

#include <string.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Box.h>
#include <Geometry/Rotation.h>
#include <SceneGraph/Geometry.h>
#include <SceneGraph/VRMLFile.h>

namespace SceneGraph {

/******************************************************
Methods of class GeodeticToCartesianPointTransformNode:
******************************************************/

GeodeticToCartesianPointTransformNode::GeodeticToCartesianPointTransformNode(void)
	:longitudeFirst(true),
	 degrees(false),
	 colatitude(false),
	 re(0)
	{
	}

void GeodeticToCartesianPointTransformNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"referenceEllipsoid")==0)
		{
		vrmlFile.parseSFNode(referenceEllipsoid);
		}
	else if(strcmp(fieldName,"longitudeFirst")==0)
		{
		vrmlFile.parseField(longitudeFirst);
		}
	else if(strcmp(fieldName,"degrees")==0)
		{
		vrmlFile.parseField(degrees);
		}
	else if(strcmp(fieldName,"colatitude")==0)
		{
		vrmlFile.parseField(colatitude);
		}
	else
		PointTransformNode::parseField(fieldName,vrmlFile);
	}

void GeodeticToCartesianPointTransformNode::update(void)
	{
	/* Create a default reference ellipsoid if none was given: */
	if(referenceEllipsoid.getValue()==0)
		{
		referenceEllipsoid.setValue(new ReferenceEllipsoidNode);
		referenceEllipsoid.getValue()->update();
		}
	
	/* Get a pointer to the low-level reference ellipsoid: */
	re=&referenceEllipsoid.getValue()->getRE();
	}

Point GeodeticToCartesianPointTransformNode::transformPoint(const Point& point) const
	{
	/* Convert the geodetic point to longitude and latitude in radians and elevation in meters: */
	ReferenceEllipsoidNode::Geoid::Point geodetic;
	geodetic[0]=ReferenceEllipsoidNode::Geoid::Scalar(longitudeFirst.getValue()?point[0]:point[1]);
	geodetic[1]=ReferenceEllipsoidNode::Geoid::Scalar(longitudeFirst.getValue()?point[1]:point[0]);
	if(degrees.getValue())
		{
		geodetic[0]=Math::rad(geodetic[0]);
		geodetic[1]=Math::rad(geodetic[1]);
		}
	if(colatitude.getValue())
		geodetic[1]=Math::div2(Math::Constants<ReferenceEllipsoidNode::Geoid::Scalar>::pi)-geodetic[1];
	geodetic[2]=ReferenceEllipsoidNode::Geoid::Scalar(point[2]);
	
	/* Return the transformed point: */
	return re->geodeticToCartesian(geodetic);
	}

Box GeodeticToCartesianPointTransformNode::calcBoundingBox(const std::vector<Point>& points) const
	{
	Box result=Box::empty;
	for(std::vector<Point>::const_iterator pIt=points.begin();pIt!=points.end();++pIt)
		result.addPoint(transformPoint(*pIt));
	return result;
	}

Vector GeodeticToCartesianPointTransformNode::transformNormal(const Point& basePoint,const Vector& normal) const
	{
	/* Convert the geodetic base point to longitude and latitude in radians: */
	ReferenceEllipsoidNode::Geoid::Point geodetic;
	geodetic[0]=ReferenceEllipsoidNode::Geoid::Scalar(longitudeFirst.getValue()?basePoint[0]:basePoint[1]);
	geodetic[1]=ReferenceEllipsoidNode::Geoid::Scalar(longitudeFirst.getValue()?basePoint[1]:basePoint[0]);
	if(degrees.getValue())
		{
		geodetic[0]=Math::rad(geodetic[0]);
		geodetic[1]=Math::rad(geodetic[1]);
		}
	if(colatitude.getValue())
		geodetic[1]=Math::div2(Math::Constants<ReferenceEllipsoidNode::Geoid::Scalar>::pi)-geodetic[1];
	
	/* Rotate the normal: */
	ReferenceEllipsoidNode::Geoid::Orientation o=re->geodeticToCartesianOrientation(geodetic);
	return o.transform(normal);
	}

}
