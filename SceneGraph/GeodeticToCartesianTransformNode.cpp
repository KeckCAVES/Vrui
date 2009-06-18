/***********************************************************************
GeodeticToCartesianTransformNode - Point transformation class to convert
geodetic coordinates (longitude/latitude/altitude on a reference
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

#include <SceneGraph/GeodeticToCartesianTransformNode.h>

#include <string.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/*************************************************
Methods of class GeodeticToCartesianTransformNode:
*************************************************/

GeodeticToCartesianTransformNode::GeodeticToCartesianTransformNode(void)
	:longitudeFirst(true),
	 degrees(false),
	 colatitude(false),
	 geodetic(Point::origin)
	{
	}

void GeodeticToCartesianTransformNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
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
	else if(strcmp(fieldName,"geodetic")==0)
		{
		vrmlFile.parseField(geodetic);
		}
	else
		GroupNode::parseField(fieldName,vrmlFile);
	}

void GeodeticToCartesianTransformNode::update(void)
	{
	/* Create a default reference ellipsoid if none was given: */
	if(referenceEllipsoid.getValue()==0)
		{
		referenceEllipsoid.setValue(new ReferenceEllipsoidNode);
		referenceEllipsoid.getValue()->update();
		}
	
	/* Convert the geodetic point to longitude and latitude in radians and height in meters: */
	double longitude=longitudeFirst.getValue()?geodetic.getValue()[0]:geodetic.getValue()[1];
	double latitude=longitudeFirst.getValue()?geodetic.getValue()[1]:geodetic.getValue()[0];
	if(degrees.getValue())
		{
		longitude=Math::rad(longitude);
		latitude=Math::rad(latitude);
		}
	if(colatitude.getValue())
		latitude=0.5*Math::Constants<double>::pi-latitude;
	double height=geodetic.getValue()[2];
	
	/* Calculate the current transformation: */
	transform=referenceEllipsoid.getValue()->geodeticToCartesianFrame(longitude,latitude,height);
	}

Box GeodeticToCartesianTransformNode::calcBoundingBox(void) const
	{
	/* Return the explicit bounding box if there is one: */
	if(haveExplicitBoundingBox)
		return explicitBoundingBox;
	else
		{
		/* Calculate the group's bounding box as the union of the transformed children's boxes: */
		Box result=Box::empty;
		for(MFGraphNode::ValueList::const_iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
			{
			Box childBox=(*chIt)->calcBoundingBox();
			childBox.transform(transform);
			result.addBox(childBox);
			}
		return result;
		}
	}

void GeodeticToCartesianTransformNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Push the transformation onto the matrix stack: */
	OGTransform previousTransform=renderState.pushTransform(transform);
	
	/* Call the render actions of all children in order: */
	for(MFGraphNode::ValueList::const_iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
		(*chIt)->glRenderAction(renderState);
		
	/* Pop the transformation off the matrix stack: */
	renderState.popTransform(previousTransform);
	}

}
