/***********************************************************************
AffinePointTransformNode - Point transformation class to transform
points by arbitrary affine transformations.
Copyright (c) 2011 Oliver Kreylos

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

#include <SceneGraph/AffinePointTransformNode.h>

#include <string.h>
#include <utility>
#include <SceneGraph/VRMLFile.h>

namespace SceneGraph {

/*****************************************
Methods of class AffinePointTransformNode:
*****************************************/

AffinePointTransformNode::AffinePointTransformNode(void)
	{
	/* Initialize the matrix to the identity transformation: */
	for(int i=0;i<3;++i)
		for(int j=0;j<4;++j)
			matrix.appendValue(i==j?Scalar(1):Scalar(0));
	}

const char* AffinePointTransformNode::getStaticClassName(void)
	{
	return "AffinePointTransform";
	}

const char* AffinePointTransformNode::getClassName(void) const
	{
	return "AffinePointTransform";
	}

void AffinePointTransformNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"matrix")==0)
		{
		vrmlFile.parseField(matrix);
		}
	else
		PointTransformNode::parseField(fieldName,vrmlFile);
	}

void AffinePointTransformNode::update(void)
	{
	/* Convert the matrix to an affine transformation: */
	transform=ATransform::identity;
	int index=0;
	for(MFFloat::ValueList::const_iterator mIt=matrix.getValues().begin();mIt!=matrix.getValues().end()&&index<12;++mIt,++index)
		transform.getMatrix()(index/4,index%4)=*mIt;
	
	/* Calculate the normal transformation: */
	normalTransform=Geometry::invert(transform);
	for(int i=0;i<3;++i)
		for(int j=i;j<3;++j)
			std::swap(normalTransform.getMatrix()(i,j),normalTransform.getMatrix()(j,i));
	}

Point AffinePointTransformNode::transformPoint(const Point& point) const
	{
	return transform.transform(point);
	}

Box AffinePointTransformNode::calcBoundingBox(const std::vector<Point>& points) const
	{
	Box result=Box::empty;
	for(std::vector<Point>::const_iterator pIt=points.begin();pIt!=points.end();++pIt)
		result.addPoint(transform.transform(*pIt));
	return result;
	}

Vector AffinePointTransformNode::transformNormal(const Point& basePoint,const Vector& normal) const
	{
	Vector result=normalTransform.transform(normal);
	result.normalize();
	return result;
	}

}
