/***********************************************************************
QuadSetNode - Class for sets of quadrilaterals as renderable
geometry.
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

#include <SceneGraph/QuadSetNode.h>

#include <string.h>
#include <utility>
#include <GL/gl.h>
#include <GL/GLGeometryWrappers.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/****************************
Methods of class QuadSetNode:
****************************/

QuadSetNode::QuadSetNode(void)
	:ccw(true),solid(true)
	{
	}

const char* QuadSetNode::getStaticClassName(void)
	{
	return "QuadSet";
	}

const char* QuadSetNode::getClassName(void) const
	{
	return "QuadSet";
	}

void QuadSetNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"coord")==0)
		{
		vrmlFile.parseSFNode(coord);
		}
	else if(strcmp(fieldName,"ccw")==0)
		{
		vrmlFile.parseField(ccw);
		}
	else if(strcmp(fieldName,"solid")==0)
		{
		vrmlFile.parseField(solid);
		}
	else
		GeometryNode::parseField(fieldName,vrmlFile);
	}

void QuadSetNode::update(void)
	{
	/* Initialize the quad corner texture coordinates: */
	quadTexCoords[0]=Point(0,0,0);
	quadTexCoords[1]=Point(1,0,0);
	quadTexCoords[2]=Point(1,1,0);
	quadTexCoords[3]=Point(0,1,0);
	
	/* Calculate normal vectors for all quads: */
	if(coord.getValue()!=0)
		{
		size_t numPoints=coord.getValue()->point.getNumValues();
		std::vector<Vector> newQuadNormals;
		for(size_t q=0;q+4<=numPoints;q+=4)
			{
			/* Get the quad's four corner points in counter-clockwise order: */
			Point ps[4];
			if(ccw.getValue())
				{
				for(size_t i=0;i<4;++i)
					ps[i]=coord.getValue()->point.getValue(q+i);
				}
			else
				{
				for(size_t i=0;i<4;++i)
					ps[i]=coord.getValue()->point.getValue(q+3-i);
				}
			
			if(pointTransform.getValue()!=0)
				{
				/* Transform the quad's corner points: */
				for(int i=0;i<4;++i)
					ps[i]=pointTransform.getValue()->transformPoint(ps[i]);
				}
			
			/* Calculate the quad's normal vector: */
			Vector normal=Geometry::cross(ps[1]-ps[0],ps[3]-ps[0])+Geometry::cross(ps[2]-ps[1],ps[0]-ps[1])+Geometry::cross(ps[3]-ps[2],ps[1]-ps[2])+Geometry::cross(ps[0]-ps[3],ps[2]-ps[3]);
			normal.normalize();
			newQuadNormals.push_back(normal);
			}
		std::swap(quadNormals,newQuadNormals);
		}
	}

Box QuadSetNode::calcBoundingBox(void) const
	{
	if(coord.getValue()!=0)
		{
		if(pointTransform.getValue()!=0)
			{
			/* Return the bounding box of the transformed point coordinates: */
			return pointTransform.getValue()->calcBoundingBox(coord.getValue()->point.getValues());
			}
		else
			{
			/* Return the bounding box of the untransformed point coordinates: */
			return coord.getValue()->calcBoundingBox();
			}
		}
	else
		return Box::empty;
	}

void QuadSetNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Bail out if there are less than 4 points: */
	if(coord.getValue()==0||coord.getValue()->point.getNumValues()<4)
		return;
	
	/* Set up OpenGL state: */
	renderState.enableCulling(GL_BACK);
	
	/* Render the quad set: */
	size_t numPoints=coord.getValue()->point.getNumValues();
	glBegin(GL_QUADS);
	std::vector<Vector>::const_iterator qnIt=quadNormals.begin();
	for(size_t q=0;q+4<=numPoints;q+=4,++qnIt)
		{
		/* Get the quad's four corner points in counter-clockwise order: */
		Point ps[4];
		if(ccw.getValue())
			{
			for(size_t i=0;i<4;++i)
				ps[i]=coord.getValue()->point.getValue(q+i);
			}
		else
			{
			for(size_t i=0;i<4;++i)
				ps[i]=coord.getValue()->point.getValue(q+3-i);
			}
		
		if(pointTransform.getValue()!=0)
			{
			/* Transform the quad's corner points: */
			for(int i=0;i<4;++i)
				ps[i]=pointTransform.getValue()->transformPoint(ps[i]);
			}
		
		/* Draw the quad's front: */
		glNormal(*qnIt);
		for(int i=0;i<4;++i)
			{
			glTexCoord(quadTexCoords[i]);
			glVertex(ps[i]);
			}
		
		if(!solid.getValue())
			{
			/* Draw the quad's back: */
			glNormal(-*qnIt);
			for(int i=3;i>=0;--i)
				{
				glTexCoord(quadTexCoords[i]);
				glVertex(ps[i]);
				}
			}
		}
	glEnd();
	}

}
