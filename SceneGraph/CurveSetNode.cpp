/***********************************************************************
CurveSetNode - Class for sets of curves written by curve tracing
application.
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

#include <SceneGraph/CurveSetNode.h>

#include <string.h>
#include <Misc/ValueSource.h>
#include <Threads/GzippedFileCharacterSource.h>
#include <Geometry/Box.h>
#include <GL/gl.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLGeometryVertex.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/***************************************
Methods of class CurveSetNode::DataItem:
***************************************/

CurveSetNode::DataItem::DataItem(void)
	:vertexBufferObjectId(0),
	 version(0)
	{
	if(GLARBVertexBufferObject::isSupported())
		{
		/* Initialize the vertex buffer object extension: */
		GLARBVertexBufferObject::initExtension();
		
		/* Create the vertex buffer object: */
		glGenBuffersARB(1,&vertexBufferObjectId);
		}
	}

CurveSetNode::DataItem::~DataItem(void)
	{
	/* Destroy the vertex buffer object: */
	if(vertexBufferObjectId!=0)
		glDeleteBuffersARB(1,&vertexBufferObjectId);
	}

/*****************************
Methods of class CurveSetNode:
*****************************/

CurveSetNode::CurveSetNode(void)
	:version(0)
	{
	}

const char* CurveSetNode::getStaticClassName(void)
	{
	return "CurveSet";
	}

const char* CurveSetNode::getClassName(void) const
	{
	return "CurveSet";
	}

void CurveSetNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"url")==0)
		{
		vrmlFile.parseField(url);
		
		/* Fully qualify all URLs: */
		for(size_t i=0;i<url.getNumValues();++i)
			url.setValue(i,vrmlFile.getFullUrl(url.getValue(i)));
		}
	else if(strcmp(fieldName,"lineWidth")==0)
		{
		vrmlFile.parseField(lineWidth);
		}
	else if(strcmp(fieldName,"pointSize")==0)
		{
		vrmlFile.parseField(pointSize);
		}
	else
		GeometryNode::parseField(fieldName,vrmlFile);
	}

void CurveSetNode::update(void)
	{
	for(size_t fileIndex=0;fileIndex<url.getNumValues();++fileIndex)
		{
		/* Open the curve file: */
		Threads::GzippedFileCharacterSource curveFile(url.getValue(fileIndex).c_str());
		Misc::ValueSource source(curveFile);
		source.skipWs();
		
		/* Read the number of curves: */
		size_t numCurves=source.readUnsignedInteger();
		
		/* Read all curves: */
		for(size_t i=0;i<numCurves;++i)
			{
			/* Read the number of vertices in the curve: */
			GLsizei nv=GLsizei(source.readUnsignedInteger());
			numVertices.push_back(nv);
			
			/* Read all vertices: */
			for(GLsizei j=0;j<nv;++j)
				{
				/* Read the next vertex: */
				Point v;
				for(int k=0;k<3;++k)
					v[k]=Scalar(source.readNumber());
				
				/* Store the vertex: */
				vertices.push_back(v);
				}
			}
		}
	
	/* Bump up the indexed line set's version number: */
	++version;
	}

Box CurveSetNode::calcBoundingBox(void) const
	{
	Box result=Box::empty;
	
	if(pointTransform.getValue()!=0)
		{
		/* Return the bounding box of the transformed point coordinates: */
		return pointTransform.getValue()->calcBoundingBox(vertices);
		}
	else
		{
		/* Add all vertices to the box: */
		for(std::vector<Point>::const_iterator vIt=vertices.begin();vIt!=vertices.end();++vIt)
			result.addPoint(*vIt);
		}
	
	return result;
	}

void CurveSetNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Set up OpenGL state: */
	renderState.disableMaterials();
	renderState.disableTextures();
	glLineWidth(lineWidth.getValue());
	glPointSize(pointSize.getValue());
	
	/* Use the current emissive color: */
	glColor(renderState.emissiveColor);
	
	/* Get the context data item: */
	DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
	
	if(dataItem->vertexBufferObjectId!=0)
		{
		/*******************************************************************
		Render the curve set from the vertex buffer:
		*******************************************************************/
		
		typedef GLGeometry::Vertex<void,0,void,0,void,Scalar,3> Vertex;
		
		/* Bind the curve set's vertex buffer object: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferObjectId);
		
		if(dataItem->version!=version)
			{
			/* Allocate the vertex buffer: */
			glBufferDataARB(GL_ARRAY_BUFFER_ARB,(vertices.size()+numVertices.size()*2)*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
			Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
			
			/* Copy curve vertices into the vertex buffer: */
			for(std::vector<Point>::const_iterator vIt=vertices.begin();vIt!=vertices.end();++vIt)
				{
				if(pointTransform.getValue()!=0)
					vPtr->position=pointTransform.getValue()->transformPoint(*vIt);
				else
					vPtr->position=*vIt;
				++vPtr;
				}
			
			/* Copy curve end points into the vertex buffer: */
			GLsizei baseIndex=0;
			for(std::vector<GLsizei>::const_iterator nvIt=numVertices.begin();nvIt!=numVertices.end();++nvIt)
				{
				if(pointTransform.getValue()!=0)
					{
					vPtr[0].position=pointTransform.getValue()->transformPoint(vertices[baseIndex]);
					vPtr[1].position=pointTransform.getValue()->transformPoint(vertices[baseIndex+*nvIt-1]);
					}
				else
					{
					vPtr[0].position=vertices[baseIndex];
					vPtr[1].position=vertices[baseIndex+*nvIt-1];
					}
				vPtr+=2;
				baseIndex+=*nvIt;
				}
			
			/* Finish uploading into the buffer: */
			glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
			
			/* Mark the vertex and index buffer objects as up-to-date: */
			dataItem->version=version;
			}
		
		/* Set up the vertex array: */
		GLVertexArrayParts::enable(Vertex::getPartsMask());
		glVertexPointer(static_cast<Vertex*>(0));
		
		/* Draw all curves: */
		GLint baseVertexIndex=0;
		for(std::vector<GLsizei>::const_iterator nvIt=numVertices.begin();nvIt!=numVertices.end();++nvIt)
			{
			if(*nvIt>=2)
				{
				/* Render the curve: */
				glDrawArrays(GL_LINE_STRIP,baseVertexIndex,*nvIt);
				}
			
			/* Go to the next curve: */
			baseVertexIndex+=*nvIt;
			}
		
		/* Draw the endpoints of all curves: */
		glDrawArrays(GL_POINTS,baseVertexIndex,numVertices.size()*2);
		
		/* Disable the vertex array: */
		GLVertexArrayParts::disable(Vertex::getPartsMask());
		
		/* Protect the buffer: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		}
	else
		{
		/*******************************************************************
		Render the curve set directly:
		*******************************************************************/
		
		/* Draw all curves: */
		std::vector<Point>::const_iterator vIt=vertices.begin();
		for(std::vector<GLsizei>::const_iterator nvIt=numVertices.begin();nvIt!=numVertices.end();++nvIt)
			{
			/* Render the curve: */
			glBegin(GL_LINE_STRIP);
			for(GLsizei i=0;i<*nvIt;++i,++vIt)
				{
				if(pointTransform.getValue()!=0)
					glVertex(pointTransform.getValue()->transformPoint(*vIt));
				else
					glVertex(*vIt);
				}
			glEnd();
			}
		
		/* Draw the endpoints of all curves: */
		GLint baseVertexIndex=0;
		for(std::vector<GLsizei>::const_iterator nvIt=numVertices.begin();nvIt!=numVertices.end();++nvIt)
			{
			if(pointTransform.getValue()!=0)
				{
				glVertex(pointTransform.getValue()->transformPoint(vertices[baseVertexIndex]));
				glVertex(pointTransform.getValue()->transformPoint(vertices[baseVertexIndex+*nvIt-1]));
				}
			else
				{
				glVertex(vertices[baseVertexIndex]);
				glVertex(vertices[baseVertexIndex+*nvIt-1]);
				}
			
			/* Go to the next curve: */
			baseVertexIndex+=*nvIt;
			}
		}
	}

void CurveSetNode::initContext(GLContextData& contextData) const
	{
	/* Create a data item and store it in the context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

}
