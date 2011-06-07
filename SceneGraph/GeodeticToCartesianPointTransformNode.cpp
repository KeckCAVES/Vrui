/***********************************************************************
GeodeticToCartesianPointTransformNode - Point transformation class to
convert geodetic coordinates (longitude/latitude/altitude on a reference
ellipsoid) to Cartesian coordinates.
Copyright (c) 2009-2011 Oliver Kreylos

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
	 elevationScale(1),
	 re(0)
	{
	}

const char* GeodeticToCartesianPointTransformNode::getStaticClassName(void)
	{
	return "GeodeticToCartesianPointTransform";
	}

const char* GeodeticToCartesianPointTransformNode::getClassName(void) const
	{
	return "GeodeticToCartesianPointTransform";
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
	else if(strcmp(fieldName,"elevationScale")==0)
		{
		vrmlFile.parseField(elevationScale);
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
	
	/* Calculate the geodetic point transformation: */
	for(int i=0;i<3;++i)
		{
		componentScales[i]=GScalar(1);
		componentOffsets[i]=GScalar(0);
		}
	if(degrees.getValue())
		{
		/* Scale longitude and latitude to radians: */
		componentScales[0]=componentScales[1]=Math::Constants<GScalar>::pi/GScalar(180);
		}
	if(colatitude.getValue())
		{
		/* Subtract latitude in radians from pi/2: */
		componentScales[1]=-componentScales[1];
		componentOffsets[1]=Math::div2(Math::Constants<GScalar>::pi);
		}
	componentScales[2]=elevationScale.getValue();
	
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
		geodetic[i]=GScalar(point[componentIndices[i]])*componentScales[i]+componentOffsets[i];
	
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
	/* Convert the geodetic base point and normal vector to longitude and latitude in radians: */
	ReferenceEllipsoidNode::Geoid::Point geodetic;
	Geometry::Vector<GScalar,3> geonorm;
	for(int i=0;i<3;++i)
		{
		geodetic[i]=GScalar(basePoint[componentIndices[i]])*componentScales[i]+componentOffsets[i];
		geonorm[i]=GScalar(normal[componentIndices[i]])/componentScales[i];
		}
	
	/* Calculate the geoid transformation's derivative at the base point: */
	ReferenceEllipsoidNode::Geoid::Derivative deriv=re->geodeticToCartesianDerivative(geodetic);
	
	/* Calculate the normal transformation matrix: */
	GScalar a=deriv(1,1)*deriv(2,2)-deriv(1,2)*deriv(2,1);
	GScalar b=deriv(1,2)*deriv(2,0)-deriv(1,0)*deriv(2,2);
	GScalar c=deriv(1,0)*deriv(2,1)-deriv(1,1)*deriv(2,0);
	GScalar d=deriv(0,2)*deriv(2,1)-deriv(0,1)*deriv(2,2);
	GScalar e=deriv(0,0)*deriv(2,2)-deriv(0,2)*deriv(2,0);
	GScalar f=deriv(0,1)*deriv(2,0)-deriv(0,0)*deriv(2,1);
	GScalar g=deriv(0,1)*deriv(1,2)-deriv(0,2)*deriv(1,1);
	GScalar h=deriv(0,2)*deriv(1,0)-deriv(0,0)*deriv(1,2);
	GScalar i=deriv(0,0)*deriv(1,1)-deriv(0,1)*deriv(1,0);
	
	/* Calculate the result normal: */
	Geometry::Vector<GScalar,3> norm;
	norm[0]=a*geonorm[0]+b*geonorm[1]+c*geonorm[2];
	norm[1]=d*geonorm[0]+e*geonorm[1]+f*geonorm[2];
	norm[2]=g*geonorm[0]+h*geonorm[1]+i*geonorm[2];
	GScalar normLen=Geometry::mag(norm);
	if(flipNormals)
		normLen=-normLen;
	return Vector(Scalar(norm[0]/normLen),Scalar(norm[1]/normLen),Scalar(norm[2]/normLen));
	}

}
