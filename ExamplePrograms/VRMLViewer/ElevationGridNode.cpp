/***********************************************************************
ElevationGridNode - Class for shapes represented as regular grids of
height values.
Copyright (c) 2006-2008 Oliver Kreylos

This file is part of the Virtual Reality VRML viewer (VRMLViewer).

The Virtual Reality VRML viewer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Virtual Reality VRML viewer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality VRML viewer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <vector>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#define NONSTANDARD_GLVERTEX_TEMPLATES
#include <GL/GLVertex.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/GLGeometryWrappers.h>

#include "SFBool.h"
#include "SFFloat.h"
#include "MFFloat.h"
#include "SFInt32.h"
#include "VRMLParser.h"
#include "TextureCoordinateNode.h"
#include "ColorNode.h"
#include "NormalNode.h"

#include "ElevationGridNode.h"

/********************************************
Methods of class ElevationGridNode::DataItem:
********************************************/

ElevationGridNode::DataItem::DataItem(void)
	:vertexBufferObjectId(0),
	 indexBufferObjectId(0)
	{
	if(GLARBVertexBufferObject::isSupported())
		{
		/* Initialize the vertex buffer object extension: */
		GLARBVertexBufferObject::initExtension();
		
		/* Create vertex and index buffer objects: */
		glGenBuffersARB(1,&vertexBufferObjectId);
		glGenBuffersARB(1,&indexBufferObjectId);
		}
	}

ElevationGridNode::DataItem::~DataItem(void)
	{
	/* Destroy the buffer objects: */
	glDeleteBuffersARB(1,&vertexBufferObjectId);
	glDeleteBuffersARB(1,&indexBufferObjectId);
	}

/**********************************
Methods of class ElevationGridNode:
**********************************/

ElevationGridNode::ElevationGridNode(VRMLParser& parser)
	:VRMLNode(parser),
	 ccw(true),
	 solid(true),
	 colorPerVertex(true),
	 normalPerVertex(true)
	{
	/* Check for the opening brace: */
	if(!parser.isToken("{"))
		Misc::throwStdErr("ElevationGridNode::ElevationGridNode: Missing opening brace in node definition");
	parser.getNextToken();
	
	/* Process attributes until closing brace: */
	dimension[0]=dimension[1]=0;
	spacing[0]=spacing[1]=0;
	while(!parser.isToken("}"))
		{
		if(parser.isToken("ccw"))
			{
			parser.getNextToken();
			ccw=SFBool::parse(parser);
			}
		else if(parser.isToken("solid"))
			{
			parser.getNextToken();
			solid=SFBool::parse(parser);
			}
		else if(parser.isToken("colorPerVertex"))
			{
			parser.getNextToken();
			colorPerVertex=SFBool::parse(parser);
			}
		else if(parser.isToken("normalPerVertex"))
			{
			parser.getNextToken();
			normalPerVertex=SFBool::parse(parser);
			}
		else if(parser.isToken("xDimension"))
			{
			parser.getNextToken();
			dimension[0]=SFInt32::parse(parser);
			}
		else if(parser.isToken("zDimension"))
			{
			parser.getNextToken();
			dimension[1]=SFInt32::parse(parser);
			}
		else if(parser.isToken("xSpacing"))
			{
			parser.getNextToken();
			spacing[0]=SFFloat::parse(parser);
			}
		else if(parser.isToken("zSpacing"))
			{
			parser.getNextToken();
			spacing[1]=SFFloat::parse(parser);
			}
		else if(parser.isToken("height"))
			{
			/* Parse the height array: */
			parser.getNextToken();
			height=MFFloat::parse(parser);
			}
		else if(parser.isToken("texCoord"))
			{
			/* Parse the texture coordinate node: */
			parser.getNextToken();
			texCoord=parser.getNextNode();
			}
		else if(parser.isToken("color"))
			{
			/* Parse the color node: */
			parser.getNextToken();
			color=parser.getNextNode();
			}
		else if(parser.isToken("normal"))
			{
			/* Parse the normal node: */
			parser.getNextToken();
			normal=parser.getNextNode();
			}
		else
			Misc::throwStdErr("ElevationGridNode::ElevationGridNode: unknown attribute \"%s\" in node definition",parser.getToken());
		}
	
	/* Skip the closing brace: */
	parser.getNextToken();
	
	/* Check height field for consistency: */
	if(dimension[0]<=1||dimension[1]<=1)
		Misc::throwStdErr("ElevationGridNode::ElevationGridNode: invalid grid dimensions %d x %d",dimension[0],dimension[1]);
	if(dimension[0]*dimension[1]!=height.size())
		Misc::throwStdErr("ElevationGridNode::ElevationGridNode: grid dimensions do not match number of height values");
	}

ElevationGridNode::~ElevationGridNode(void)
	{
	}

void ElevationGridNode::initContext(GLContextData& contextData) const
	{
	/* Create a data item and store it in the context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Do nothing if the vertex buffer object extension is not supported: */
	if(dataItem->vertexBufferObjectId==0||dataItem->indexBufferObjectId==0)
		return;
	
	const TextureCoordinateNode* texCoordNode=dynamic_cast<const TextureCoordinateNode*>(texCoord.ptr());
	const ColorNode* colorNode=dynamic_cast<const ColorNode*>(color.ptr());
	const NormalNode* normalNode=dynamic_cast<const NormalNode*>(normal.ptr());
	
	/* Check whether vertices can be shared between quads: */
	bool shareVertices=(colorPerVertex||colorNode==0)&&normalPerVertex;
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferObjectId);
	if(shareVertices)
		{
		/* Upload all vertices into the vertex buffer: */
		size_t numVertices=size_t(dimension[0])*size_t(dimension[1]);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,numVertices*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
		Vertex* vertices=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
		size_t vertexIndex=0;
		for(Int32 z=0;z<dimension[1];++z)
			for(Int32 x=0;x<dimension[0];++x,++vertexIndex,++vertices)
				{
				/* Create the vertex: */
				if(texCoordNode!=0)
					vertices->texCoord=Vertex::TexCoord(texCoordNode->getPoint(i).getComponents());
				if(colorNode!=0)
					vertices->color=colorNode->getColor(i);
				if(normalNode!=0)
					vertices->normal=Vertex::Normal(normalNode->getVector(i).getComponents());
				else
					{
					/* Calculate normal vector based on central differences: */
					...
					}
				vertices->position[0]=Float(x)*spacing[0];
				vertices->position[1]=height[vertexIndex];
				vertices->position[2]=Float(z)*spacing[1];
				}
		}
	else
		{
		/* Upload all vertices into the vertex buffer: */
		size_t numVertices=size_t(dimension[0]-1)*size_t(dimension[1]-1)*4;
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,numVertices*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
		Vertex* vertices=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
		size_t vertexIndex=0;
		for(Int32 z=1;z<dimension[1];++z)
			for(Int32 x=1;x<dimension[0];++x,++vertexIndex,++vertices)
				{
				/* Create the vertex: */
				if(texCoordNode!=0)
					vertices->texCoord=Vertex::TexCoord(texCoordNode->getPoint(i).getComponents());
				if(colorNode!=0)
					vertices->color=colorNode->getColor(i);
				if(normalNode!=0)
					vertices->normal=Vertex::Normal(normalNode->getVector(i).getComponents());
				else
					{
					/* Calculate normal vector based on central differences: */
					...
					}
				vertices->position[0]=Float(x)*spacing[0];
				vertices->position[1]=height[vertexIndex];
				vertices->position[2]=Float(z)*spacing[1];
				}
		}
	
	/* Unmap and protect the vertex buffer: */
	glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	
	
	/* Upload all vertex indices into the index buffers: */
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,dataItem->indexBufferObjectId);
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,dataItem->numTriangles*3*sizeof(GLuint),0,GL_STATIC_DRAW_ARB);
	GLuint* indices=static_cast<GLuint*>(glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	
	texCoordIt=texCoordIndices.empty()?coordIndices.begin():texCoordIndices.begin();
	colorIt=colorIndices.empty()?coordIndices.begin():colorIndices.begin();
	normalIt=normalIndices.empty()?coordIndices.begin():normalIndices.begin();
	coordIt=coordIndices.begin();
	currentVertex=VertexIndices(0,0,0,0);
	while(coordIt!=coordIndices.end())
		{
		/* Process the vertices of this face: */
		GLuint triangleIndices[2];
		size_t numFaceVertices=0;
		while(coordIt!=coordIndices.end()&&*coordIt>=0)
			{
			/* Create the current compound vertex index: */
			if(texCoordNode!=0)
				currentVertex.texCoord=*texCoordIt;
			if(colorNode!=0)
				currentVertex.color=*colorIt;
			if(normalNode!=0)
				currentVertex.normal=*normalIt;
			currentVertex.coord=*coordIt;
			
			/* Get the array index of the assembled vertex: */
			GLuint index=vertexHasher.getEntry(currentVertex).getDest();
			
			/* Assemble triangles: */
			if(numFaceVertices<2)
				triangleIndices[numFaceVertices]=index;
			else
				{
				for(int i=0;i<2;++i)
					indices[i]=triangleIndices[i];
				indices[2]=index;
				triangleIndices[1]=index;
				indices+=3;
				}
			
			/* Go to the next vertex in the same face: */
			++texCoordIt;
			if(colorPerVertex)
				++colorIt;
			if(normalPerVertex)
				++normalIt;
			++coordIt;
			++numFaceVertices;
			}
		
		/* Go to the next face: */
		++texCoordIt;
		++colorIt;
		++normalIt;
		++coordIt;
		}
	
	/* Unmap and protect the index buffer: */
	glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
	}

VRMLNode::Box ElevationGridNode::calcBoundingBox(void) const
	{
	/* Get a pointer to the coord node: */
	CoordinateNode* coordNode=dynamic_cast<CoordinateNode*>(coord.ptr());
	
	/* Calculate the bounding box of all used vertex coordinates: */
	Box result=Box::empty;
	for(std::vector<int>::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();++ciIt)
		if(*ciIt>=0)
			result.addPoint(coordNode->getPoint(*ciIt));
	return result;
	}

void ElevationGridNode::glRenderAction(GLContextData& contextData) const
	{
	/* Retrieve the data item from the context: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	const TextureCoordinateNode* texCoordNode=dynamic_cast<const TextureCoordinateNode*>(texCoord.ptr());
	const ColorNode* colorNode=dynamic_cast<const ColorNode*>(color.ptr());
	const NormalNode* normalNode=dynamic_cast<const NormalNode*>(normal.ptr());
	const CoordinateNode* coordNode=dynamic_cast<const CoordinateNode*>(coord.ptr());
	
	/* Set up OpenGL: */
	if(ccw)
		glFrontFace(GL_CCW);
	else
		glFrontFace(GL_CW);
	if(solid)
		{
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
		}
	else
		{
		glDisable(GL_CULL_FACE);
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
		}
	
	if(dataItem->vertexBufferObjectId!=0&&dataItem->indexBufferObjectId!=0)
		{
		/* Determine which parts of the vertex array to enable: */
		int vertexPartsMask=0;
		if(texCoordNode!=0)
			vertexPartsMask|=GLVertexArrayParts::TexCoord;
		if(colorNode!=0)
			vertexPartsMask|=GLVertexArrayParts::Color;
		if(normalNode!=0)
			vertexPartsMask|=GLVertexArrayParts::Normal;
		vertexPartsMask|=GLVertexArrayParts::Position;
		
		/* Draw the indexed triangle set: */
		GLVertexArrayParts::enable(vertexPartsMask);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferObjectId);
		glVertexPointer(vertexPartsMask,static_cast<const Vertex*>(0));
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,dataItem->indexBufferObjectId);
		glDrawElements(GL_TRIANGLES,dataItem->numTriangles*3,GL_UNSIGNED_INT,static_cast<const GLuint*>(0));
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
		GLVertexArrayParts::disable(vertexPartsMask);
		}
	else
		{
		/* Process all faces: */
		std::vector<int>::const_iterator texCoordIt=texCoordIndices.begin();
		std::vector<int>::const_iterator colorIt=colorIndices.begin();
		std::vector<int>::const_iterator normalIt=normalIndices.begin();
		std::vector<int>::const_iterator coordIt=coordIndices.begin();
		while(coordIt!=coordIndices.end())
			{
			glBegin(GL_POLYGON);
			while(*coordIt>=0)
				{
				if(texCoordNode!=0)
					glTexCoord(texCoordNode->getPoint(*texCoordIt));
				if(colorNode!=0)
					glColor(colorNode->getColor(*colorIt));
				if(normalNode!=0)
					glNormal(normalNode->getVector(*normalIt));
				glVertex(coordNode->getPoint(*coordIt));
				++texCoordIt;
				if(colorPerVertex)
					++colorIt;
				if(normalPerVertex)
					++normalIt;
				++coordIt;
				}
			glEnd();
			
			++texCoordIt;
			++colorIt;
			++normalIt;
			++coordIt;
			}
		}
	}
