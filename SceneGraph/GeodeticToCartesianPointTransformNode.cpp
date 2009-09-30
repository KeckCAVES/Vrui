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

#include <utility>
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
	:longitude("X"),latitude("Y"),elevation("Z"),
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
	else if(strcmp(fieldName,"longitude")==0)
		{
		vrmlFile.parseField(longitude);
		}
	else if(strcmp(fieldName,"latitude")==0)
		{
		vrmlFile.parseField(latitude);
		}
	else if(strcmp(fieldName,"elevation")==0)
		{
		vrmlFile.parseField(elevation);
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
	
	/* Get the point component indices: */
	if(longitude.getValue()=="X")
		componentIndices[0]=0;
	else if(longitude.getValue()=="Y")
		componentIndices[0]=1;
	else if(longitude.getValue()=="Z")
		componentIndices[0]=2;
	if(latitude.getValue()=="X")
		componentIndices[1]=0;
	else if(latitude.getValue()=="Y")
		componentIndices[1]=1;
	else if(latitude.getValue()=="Z")
		componentIndices[1]=2;
	if(elevation.getValue()=="X")
		componentIndices[2]=0;
	else if(elevation.getValue()=="Y")
		componentIndices[2]=1;
	else if(elevation.getValue()=="Z")
		componentIndices[2]=2;
	
	/* Check whether normal vectors need to be flipped: */
	int ci[3];
	for(int i=0;i<3;++i)
		ci[i]=componentIndices[i];
	int numSwaps=0;
	if(ci[0]>ci[1])
		{
		std::swap(ci[0],ci[1]);
		++numSwaps;
		}
	if(ci[1]>ci[2])
		{
		std::swap(ci[1],ci[2]);
		++numSwaps;
		}
	if(ci[0]>ci[1])
		{
		std::swap(ci[0],ci[1]);
		++numSwaps;
		}
	flipNormals=numSwaps%2==1;
	}

Point GeodeticToCartesianPointTransformNode::transformPoint(const Point& point) const
	{
	/* Convert the geodetic point to longitude and latitude in radians and elevation in meters: */
	ReferenceEllipsoidNode::Geoid::Point geodetic;
	for(int i=0;i<3;++i)
		geodetic[i]=ReferenceEllipsoidNode::Geoid::Scalar(point[componentIndices[i]]);
	if(degrees.getValue())
		{
		geodetic[0]=Math::rad(geodetic[0]);
		geodetic[1]=Math::rad(geodetic[1]);
		}
	if(colatitude.getValue())
		geodetic[1]=Math::div2(Math::Constants<ReferenceEllipsoidNode::Geoid::Scalar>::pi)-geodetic[1];
	
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
	for(int i=0;i<3;++i)
		geodetic[i]=ReferenceEllipsoidNode::Geoid::Scalar(basePoint[componentIndices[i]]);
	if(degrees.getValue())
		{
		geodetic[0]=Math::rad(geodetic[0]);
		geodetic[1]=Math::rad(geodetic[1]);
		}
	if(colatitude.getValue())
		geodetic[1]=Math::div2(Math::Constants<ReferenceEllipsoidNode::Geoid::Scalar>::pi)-geodetic[1];
	
	/* Rotate the normal: */
	ReferenceEllipsoidNode::Geoid::Orientation o=re->geodeticToCartesianOrientation(geodetic);
	Vector geodeticNormal;
	for(int i=0;i<3;++i)
		geodeticNormal[i]=ReferenceEllipsoidNode::Geoid::Scalar(normal[componentIndices[i]]);
	if(flipNormals)
		geodeticNormal=-geodeticNormal;
	return o.transform(geodeticNormal);
	}

}
