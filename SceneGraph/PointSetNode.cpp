/***********************************************************************
PointSetNode - Class for sets of points as renderable geometry.
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

#include <SceneGraph/PointSetNode.h>

#include <string.h>
#include <Geometry/Box.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/*****************************
Methods of class PointSetNode:
*****************************/

PointSetNode::PointSetNode(void)
	:pointSize(Scalar(1))
	{
	}

void PointSetNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"color")==0)
		{
		vrmlFile.parseSFNode(color);
		}
	else if(strcmp(fieldName,"coord")==0)
		{
		vrmlFile.parseSFNode(coord);
		}
	else if(strcmp(fieldName,"pointSize")==0)
		{
		vrmlFile.parseField(pointSize);
		}
	else
		GeometryNode::parseField(fieldName,vrmlFile);
	}

void PointSetNode::update(void)
	{
	}

Box PointSetNode::calcBoundingBox(void) const
	{
	if(coord.getValue()!=0)
		{
		/* Return the bounding box of the point coordinates: */
		if(pointTransform.getValue()!=0)
			{
			/* Return the bounding box of the transformed point coordinates: */
			return pointTransform.getValue()->calcBoundingBox(coord.getValue()->getPoints());
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

void PointSetNode::glRenderAction(GLRenderState& renderState) const
	{
	if(coord.getValue()!=0)
		{
		/* Set up OpenGL state: */
		renderState.disableMaterials();
		renderState.disableTextures();
		glPointSize(pointSize.getValue());
		
		/* Draw the point set: */
		glBegin(GL_POINTS);
		const std::vector<Point>& points=coord.getValue()->getPoints();
		size_t numPoints=points.size();
		if(color.getValue()!=0)
			{
			/* Color each point: */
			const std::vector<Color>& colors=color.getValue()->getColors();
			size_t numColors=colors.size();
			for(size_t i=0;i<numPoints;++i)
				{
				if(i<numColors)
					glColor(colors[i]);
				if(pointTransform.getValue()!=0)
					glVertex(pointTransform.getValue()->transformPoint(points[i]));
				else
					glVertex(points[i]);
				}
			}
		else
			{
			/* Use the current emissive color: */
			glColor(renderState.emissiveColor);
			for(size_t i=0;i<numPoints;++i)
				{
				if(pointTransform.getValue()!=0)
					glVertex(pointTransform.getValue()->transformPoint(points[i]));
				else
					glVertex(points[i]);
				}
			}
		glEnd();
		}
	}

}
