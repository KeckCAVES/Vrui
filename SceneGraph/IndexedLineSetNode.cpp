/***********************************************************************
IndexedLineSetNode - Class for sets of lines or polylines as renderable
geometry.
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

#include <SceneGraph/IndexedLineSetNode.h>

#include <string.h>
#include <Geometry/Box.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/***********************************
Methods of class IndexedLineSetNode:
***********************************/

IndexedLineSetNode::IndexedLineSetNode(void)
	:colorPerVertex(true),
	 lineWidth(Scalar(1))
	{
	}

void IndexedLineSetNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"color")==0)
		{
		vrmlFile.parseSFNode(color);
		}
	else if(strcmp(fieldName,"coord")==0)
		{
		vrmlFile.parseSFNode(coord);
		}
	else if(strcmp(fieldName,"colorIndex")==0)
		{
		vrmlFile.parseField(colorIndex);
		}
	else if(strcmp(fieldName,"colorPerVertex")==0)
		{
		vrmlFile.parseField(colorPerVertex);
		}
	else if(strcmp(fieldName,"coordIndex")==0)
		{
		vrmlFile.parseField(coordIndex);
		}
	else if(strcmp(fieldName,"lineWidth")==0)
		{
		vrmlFile.parseField(lineWidth);
		}
	else
		GeometryNode::parseField(fieldName,vrmlFile);
	}

void IndexedLineSetNode::update(void)
	{
	}

Box IndexedLineSetNode::calcBoundingBox(void) const
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

void IndexedLineSetNode::glRenderAction(GLRenderState& renderState) const
	{
	if(coord.getValue()!=0)
		{
		/* Set up OpenGL state: */
		renderState.disableMaterials();
		renderState.disableTextures();
		glLineWidth(lineWidth.getValue());
		
		/* Draw the line set: */
		const std::vector<Point>& points=coord.getValue()->point.getValues();
		const MFInt::ValueList& coordIndices=coordIndex.getValues();
		if(color.getValue()!=0)
			{
			const std::vector<Color>& colors=color.getValue()->color.getValues();
			const MFInt::ValueList& colorIndices=colorIndex.getValues();
			MFInt::ValueList::const_iterator colorIt=colorIndices.empty()?coordIndices.begin():colorIndices.begin();
			bool countColors=!colorPerVertex.getValue()&&colorIndices.empty();
			int colorCounter=0;
			MFInt::ValueList::const_iterator coordIt=coordIndices.begin();
			while(coordIt!=coordIndices.end())
				{
				glBegin(GL_LINE_STRIP);
				while(coordIt!=coordIndices.end()&&*coordIt>=0)
					{
					if(countColors)
						glColor(colors[colorCounter]);
					else
						glColor(colors[*colorIt]);
					if(pointTransform.getValue()!=0)
						glVertex(pointTransform.getValue()->transformPoint(points[*coordIt]));
					else
						glVertex(points[*coordIt]);
					if(colorPerVertex.getValue())
						++colorIt;
					++coordIt;
					}
				glEnd();
				
				if(countColors)
					++colorCounter;
				else
					++colorIt;
				if(coordIt!=coordIndices.end())
					++coordIt;
				}
			}
		else
			{
			/* Use the current emissive color: */
			glColor(renderState.emissiveColor);
			
			MFInt::ValueList::const_iterator coordIt=coordIndices.begin();
			while(coordIt!=coordIndices.end())
				{
				glBegin(GL_LINE_STRIP);
				while(*coordIt>=0)
					{
					if(pointTransform.getValue()!=0)
						glVertex(pointTransform.getValue()->transformPoint(points[*coordIt]));
					else
						glVertex(points[*coordIt]);
					++coordIt;
					}
				glEnd();
				
				if(coordIt!=coordIndices.end())
					++coordIt;
				}
			}
		}
	}

}
