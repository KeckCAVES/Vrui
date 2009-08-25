/***********************************************************************
ElevationGridNode - Class for quad-based height fields as renderable
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

#include <SceneGraph/ElevationGridNode.h>

#include <string.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLGeometryVertex.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/********************************************
Methods of class ElevationGridNode::DataItem:
********************************************/

ElevationGridNode::DataItem::DataItem(void)
	:vertexBufferObjectId(0),indexBufferObjectId(0),
	 version(0)
	{
	if(GLARBVertexBufferObject::isSupported())
		{
		/* Initialize the vertex buffer object extension: */
		GLARBVertexBufferObject::initExtension();
		
		/* Create the vertex buffer object: */
		glGenBuffersARB(1,&vertexBufferObjectId);
		
		/* Create the index buffer object: */
		glGenBuffersARB(1,&indexBufferObjectId);
		}
	}

ElevationGridNode::DataItem::~DataItem(void)
	{
	/* Destroy the vertex buffer object: */
	if(vertexBufferObjectId!=0)
		glDeleteBuffersARB(1,&vertexBufferObjectId);
	
	/* Destroy the index buffer object: */
	if(indexBufferObjectId!=0)
		glDeleteBuffersARB(1,&indexBufferObjectId);
	}

/**********************************
Methods of class ElevationGridNode:
**********************************/

void ElevationGridNode::uploadVerticesSmooth(void) const
	{
	/* Define the vertex type used in the vertex array: */
	typedef GLGeometry::Vertex<Scalar,2,Scalar,4,Scalar,Scalar,3> Vertex;
	
	/* Initialize the vertex buffer object: */
	int xDim=xDimension.getValue();
	int zDim=zDimension.getValue();
	Scalar xSp=xSpacing.getValue();
	Scalar zSp=zSpacing.getValue();
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,xDim*zDim*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
	
	/* Store all vertices: */
	Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	for(int z=0;z<zDim;++z)
		for(int x=0;x<xDim;++x,++vPtr)
			{
			/* Store the vertex' texture coordinate: */
			vPtr->texCoord=texCoord.getValue()!=0?texCoord.getValue()->point.getValue(z*xDim+x):Vertex::TexCoord(Scalar(x)/Scalar(xDim-1),Scalar(z)/Scalar(zDim-1));
			
			/* Store the vertex' color: */
			if(color.getValue()!=0)
				vPtr->color=color.getValue()->color.getValue(z*xDim+x);
			
			/* Store the vertex' normal: */
			Vertex::Normal normal;
			if(x==0)
				normal[0]=(height.getValue(z*xDim+(x+1))-height.getValue(z*xDim+x))/xSp;
			else if(x==xDim-1)
				normal[0]=(height.getValue(z*xDim+x)-height.getValue(z*xDim+(x-1)))/xSp;
			else
				normal[0]=(height.getValue(z*xDim+(x+1))-height.getValue(z*xDim+(x-1)))/(Scalar(2)*xSp);
			normal[1]=Scalar(1);
			if(z==0)
				normal[2]=(height.getValue((z+1)*xDim+x)-height.getValue(z*xDim+x))/zSp;
			else if(z==zDim-1)
				normal[2]=(height.getValue(z*xDim+x)-height.getValue((z-1)*xDim+x))/zSp;
			else
				normal[2]=(height.getValue((z+1)*xDim+x)-height.getValue((z-1)*xDim+x))/(Scalar(2)*zSp);
			if(!ccw.getValue())
				normal=-normal;
			normal.normalize();
			vPtr->normal=normal;
			
			/* Store the vertex' position: */
			vPtr->position[0]=Scalar(x)*Scalar(xSp);
			vPtr->position[1]=height.getValue(z*xDim+x);
			vPtr->position[2]=Scalar(z)*Scalar(zSp);
			}
	
	glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
	
	/* Initialize the index buffer object: */
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,(zDim-1)*xDim*2*sizeof(GLuint),0,GL_STATIC_DRAW_ARB);
	
	/* Store all vertex indices: */
	GLuint* iPtr=static_cast<GLuint*>(glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,GL_WRITE_ONLY));
	if(ccw.getValue())
		{
		for(int z=0;z<zDim-1;++z)
			for(int x=0;x<xDim;++x,iPtr+=2)
				{
				iPtr[0]=GLuint((z+1)*xDim+x);
				iPtr[1]=GLuint(z*xDim+x);
				}
		}
	else
		{
		for(int z=0;z<zDim-1;++z)
			for(int x=0;x<xDim;++x,iPtr+=2)
				{
				iPtr[0]=GLuint(z*xDim+x);
				iPtr[1]=GLuint((z+1)*xDim+x);
				}
		}
	
	glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
	}

void ElevationGridNode::uploadVerticesFaceted(void) const
	{
	/* Define the vertex type used in the vertex array: */
	typedef GLGeometry::Vertex<Scalar,2,Scalar,4,Scalar,Scalar,3> Vertex;
	
	/* Initialize the vertex buffer object: */
	int xDim=xDimension.getValue();
	int zDim=zDimension.getValue();
	Scalar xSp=xSpacing.getValue();
	Scalar zSp=zSpacing.getValue();
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,(xDim-1)*(zDim-1)*4*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
	
	/* Store all vertices: */
	Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	for(int z=0;z<zDim-1;++z)
		for(int x=0;x<xDim-1;++x,vPtr+=4)
			{
			/* Get the corner heights of the current quad: */
			Scalar h0=height.getValue(z*xDim+x);
			Scalar h1=height.getValue(z*xDim+(x+1));
			Scalar h2=height.getValue((z+1)*xDim+x);
			Scalar h3=height.getValue((z+1)*xDim+(x+1));
			
			/* Calculate the normal vector of the current quad: */
			Vertex::Normal normal;
			normal=Vertex::Normal((h0-h1+h2-h3)*zSp,Scalar(2)*xSp*zSp,(h0+h1-h2-h3)*xSp);
			normal.normalize();
			
			/* Calculate the corner texture coordinates of the current quad: */
			Vertex::TexCoord tc[4];
			if(texCoord.getValue()!=0)
				{
				tc[0]=texCoord.getValue()->point.getValue(z*xDim+x);
				tc[1]=texCoord.getValue()->point.getValue(z*xDim+(x+1));
				tc[2]=texCoord.getValue()->point.getValue((z+1)*xDim+x);
				tc[3]=texCoord.getValue()->point.getValue((z+1)*xDim+(x+1));
				}
			else
				{
				tc[0]=Vertex::TexCoord(Scalar(x)/Scalar(xDim-1),Scalar(z)/Scalar(zDim-1));
				tc[1]=Vertex::TexCoord(Scalar(x+1)/Scalar(xDim-1),Scalar(z)/Scalar(zDim-1));
				tc[2]=Vertex::TexCoord(Scalar(x)/Scalar(xDim-1),Scalar(z+1)/Scalar(zDim-1));
				tc[3]=Vertex::TexCoord(Scalar(x+1)/Scalar(xDim-1),Scalar(z+1)/Scalar(zDim-1));
				}
			
			/* Store the corner vertices of the current quad: */
			if(ccw.getValue())
				{
				/* Store the corner vertices in counter-clockwise order: */
				vPtr[0].texCoord=tc[0];
				if(color.getValue()!=0)
					vPtr[0].color=colorPerVertex.getValue()?color.getValue()->color.getValue(z*xDim+x):color.getValue()->color.getValue(z*(xDim-1)+x);
				vPtr[0].normal=normal;
				vPtr[0].position=Vertex::Position(Scalar(x)*xSp,h0,Scalar(z)*zSp);
				vPtr[1].texCoord=tc[1];
				if(color.getValue()!=0)
					vPtr[1].color=colorPerVertex.getValue()?color.getValue()->color.getValue(z*xDim+(x+1)):color.getValue()->color.getValue(z*(xDim-1)+x);
				vPtr[1].normal=normal;
				vPtr[1].position=Vertex::Position(Scalar(x+1)*xSp,h1,Scalar(z)*zSp);
				vPtr[2].texCoord=tc[3];
				if(color.getValue()!=0)
					vPtr[2].color=colorPerVertex.getValue()?color.getValue()->color.getValue((z+1)*xDim+(x+1)):color.getValue()->color.getValue(z*(xDim-1)+x);
				vPtr[2].normal=normal;
				vPtr[2].position=Vertex::Position(Scalar(x+1)*xSp,h3,Scalar(z+1)*zSp);
				vPtr[3].texCoord=tc[2];
				if(color.getValue()!=0)
					vPtr[3].color=colorPerVertex.getValue()?color.getValue()->color.getValue((z+1)*xDim+x):color.getValue()->color.getValue(z*(xDim-1)+x);
				vPtr[3].normal=normal;
				vPtr[3].position=Vertex::Position(Scalar(x)*xSp,h2,Scalar(z+1)*zSp);
				}
			else
				{
				/* Flip the normal vector: */
				normal=-normal;
				
				/* Store the corner vertices in clockwise order: */
				vPtr[0].texCoord=tc[0];
				if(color.getValue()!=0)
					vPtr[0].color=colorPerVertex.getValue()?color.getValue()->color.getValue(z*xDim+x):color.getValue()->color.getValue(z*(xDim-1)+x);
				vPtr[0].normal=normal;
				vPtr[0].position=Vertex::Position(Scalar(x)*xSp,h0,Scalar(z)*zSp);
				vPtr[1].texCoord=tc[2];
				if(color.getValue()!=0)
					vPtr[1].color=colorPerVertex.getValue()?color.getValue()->color.getValue((z+1)*xDim+x):color.getValue()->color.getValue(z*(xDim-1)+x);
				vPtr[1].normal=normal;
				vPtr[1].position=Vertex::Position(Scalar(x)*xSp,h2,Scalar(z+1)*zSp);
				vPtr[2].texCoord=tc[3];
				if(color.getValue()!=0)
					vPtr[2].color=colorPerVertex.getValue()?color.getValue()->color.getValue((z+1)*xDim+(x+1)):color.getValue()->color.getValue(z*(xDim-1)+x);
				vPtr[2].normal=normal;
				vPtr[2].position=Vertex::Position(Scalar(x+1)*xSp,h3,Scalar(z+1)*zSp);
				vPtr[3].texCoord=tc[1];
				if(color.getValue()!=0)
					vPtr[3].color=colorPerVertex.getValue()?color.getValue()->color.getValue(z*xDim+(x+1)):color.getValue()->color.getValue(z*(xDim-1)+x);
				vPtr[3].normal=normal;
				vPtr[3].position=Vertex::Position(Scalar(x+1)*xSp,h1,Scalar(z)*zSp);
				}
			}
	
	glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
	
	/* Clear the index buffer object (not needed for faceted elevation grids): */
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0,0,GL_STATIC_DRAW_ARB);
	}

ElevationGridNode::ElevationGridNode(void)
	:colorPerVertex(true),normalPerVertex(true),
	 creaseAngle(0),
	 xDimension(0),xSpacing(0),
	 zDimension(0),zSpacing(0),
	 ccw(true),solid(true)
	{
	}

void ElevationGridNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"texCoord")==0)
		{
		vrmlFile.parseSFNode(texCoord);
		}
	else if(strcmp(fieldName,"color")==0)
		{
		vrmlFile.parseSFNode(color);
		}
	else if(strcmp(fieldName,"colorPerVertex")==0)
		{
		vrmlFile.parseField(colorPerVertex);
		}
	else if(strcmp(fieldName,"normal")==0)
		{
		vrmlFile.parseSFNode(normal);
		}
	else if(strcmp(fieldName,"normalPerVertex")==0)
		{
		vrmlFile.parseField(normalPerVertex);
		}
	else if(strcmp(fieldName,"creaseAngle")==0)
		{
		vrmlFile.parseField(creaseAngle);
		}
	else if(strcmp(fieldName,"xDimension")==0)
		{
		vrmlFile.parseField(xDimension);
		}
	else if(strcmp(fieldName,"xSpacing")==0)
		{
		vrmlFile.parseField(xSpacing);
		}
	else if(strcmp(fieldName,"zDimension")==0)
		{
		vrmlFile.parseField(zDimension);
		}
	else if(strcmp(fieldName,"zSpacing")==0)
		{
		vrmlFile.parseField(zSpacing);
		}
	else if(strcmp(fieldName,"height")==0)
		{
		vrmlFile.parseField(height);
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

void ElevationGridNode::update(void)
	{
	/* Bump up the elevation grid's version number: */
	++version;
	}

Box ElevationGridNode::calcBoundingBox(void) const
	{
	Box result=Box::empty;
	
	if(height.getNumValues()==size_t(xDimension.getValue())*size_t(zDimension.getValue()))
		{
		if(pointTransform.getValue()!=0)
			{
			/* Return the bounding box of the transformed point coordinates: */
			MFFloat::ValueList::const_iterator hIt=height.getValues().begin();
			Point p;
			p[2]=Scalar(0);
			for(int z=0;z<zDimension.getValue();++z,p[2]+=zSpacing.getValue())
				{
				p[0]=Scalar(0);
				for(int x=0;x<xDimension.getValue();++x,p[0]+=xSpacing.getValue(),++hIt)
					{
					p[1]=*hIt;
					result.addPoint(pointTransform.getValue()->transformPoint(p));
					}
				}
			}
		else
			{
			/* Return the bounding box of the untransformed point coordinates: */
			MFFloat::ValueList::const_iterator hIt=height.getValues().begin();
			Scalar yMin,yMax;
			yMin=yMax=*hIt;
			for(++hIt;hIt!=height.getValues().end();++hIt)
				{
				if(yMin>*hIt)
					yMin=*hIt;
				else if(yMax<*hIt)
					yMax=*hIt;
				}
			result=Box(Point(0,yMin,0),Point(Scalar(xDimension.getValue()-1)*xSpacing.getValue(),yMax,Scalar(zDimension.getValue()-1)*zSpacing.getValue()));
			}
		}
	
	return result;
	}

void ElevationGridNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Get the context data item: */
	DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
	
	if(dataItem->vertexBufferObjectId!=0)
		{
		/*******************************************************************
		Render the elevation grid from the vertex buffer:
		*******************************************************************/
		
		typedef GLGeometry::Vertex<Scalar,2,Scalar,4,Scalar,Scalar,3> Vertex;
		
		/* Bind the vertex and index buffer objects: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferObjectId);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,dataItem->indexBufferObjectId);
		
		/* Check if the buffers need to be updated: */
		if(dataItem->version!=version)
			{
			/* Upload the new elevation grid geometry: */
			if(normalPerVertex.getValue()&&(color.getValue()==0||colorPerVertex.getValue()))
				uploadVerticesSmooth();
			else
				uploadVerticesFaceted();
			
			dataItem->version=version;
			}
		
		/* Set up the vertex arrays: */
		int vertexArrayParts=Vertex::getPartsMask();
		if(color.getValue()==0)
			{
			/* Disable the color vertex array: */
			vertexArrayParts&=~GLVertexArrayParts::Color;
			
			/* Use the current emissive color: */
			glColor(renderState.emissiveColor);
			}
		GLVertexArrayParts::enable(vertexArrayParts);
		glVertexPointer(static_cast<Vertex*>(0));
		
		/* Draw the elevation grid: */
		if(normalPerVertex.getValue()&&(color.getValue()==0||colorPerVertex.getValue()))
			{
			/* Draw the elevation grid as a series of indexed quad strips: */
			const GLuint* iPtr=0;
			for(int z=0;z<zDimension.getValue();++z,iPtr+=xDimension.getValue()*2)
				glDrawElements(GL_QUAD_STRIP,xDimension.getValue()*2,GL_UNSIGNED_INT,iPtr);
			}
		else
			{
			/* Draw the elevation grid as a series of quads: */
			glDrawArrays(GL_QUADS,0,(xDimension.getValue()-1)*(zDimension.getValue()-1)*4);
			}
		
		/* Reset the vertex arrays: */
		GLVertexArrayParts::disable(vertexArrayParts);
		
		/* Protect the buffer objects: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
		}
	else
		{
		/*******************************************************************
		Render the elevation grid directly:
		*******************************************************************/
		}
	}

void ElevationGridNode::initContext(GLContextData& contextData) const
	{
	/* Create a data item and store it in the context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

}
