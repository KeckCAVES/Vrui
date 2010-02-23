/***********************************************************************
ElevationGridNode - Class for quad-based height fields as renderable
geometry.
Copyright (c) 2009-2010 Oliver Kreylos

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

#define NONSTANDARD_GLVERTEX_TEMPLATES

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

#include <SceneGraph/LoadElevationGrid.h>

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

Vector ElevationGridNode::calcVertexNormal(int x,int z) const
	{
	Vector result;
	int xDim=xDimension.getValue();
	int zDim=zDimension.getValue();
	size_t vInd=z*xDim+x;
	if(x==0)
		result[0]=-(height.getValue(vInd+1)-height.getValue(vInd))*zSpacing.getValue();
	else if(x==xDim-1)
		result[0]=-(height.getValue(vInd)-height.getValue(vInd-1))*zSpacing.getValue();
	else
		result[0]=-(height.getValue(vInd+1)-height.getValue(vInd-1))*Scalar(0.5)*zSpacing.getValue();
	result[1]=xSpacing.getValue()*zSpacing.getValue();
	if(z==0)
		result[2]=-(height.getValue(vInd+xDim)-height.getValue(vInd))*xSpacing.getValue();
	else if(z==zDim-1)
		result[2]=-(height.getValue(vInd)-height.getValue(vInd-xDim))*xSpacing.getValue();
	else
		result[2]=-(height.getValue(vInd+xDim)-height.getValue(vInd-xDim))*Scalar(0.5)*xSpacing.getValue();
	if(!ccw.getValue())
		result=-result;
	result.normalize();
	return result;
	}

void ElevationGridNode::uploadIndexedQuadStripSet(void) const
	{
	/* Define the vertex type used in the vertex array: */
	typedef GLGeometry::Vertex<Scalar,2,GLubyte,4,Scalar,Scalar,3> Vertex;
	
	/* Initialize the vertex buffer object: */
	int xDim=xDimension.getValue();
	int zDim=zDimension.getValue();
	Scalar xSp=xSpacing.getValue();
	Scalar zSp=zSpacing.getValue();
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,xDim*zDim*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
	
	/* Store all vertices: */
	Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	size_t vInd=0;
	for(int z=0;z<zDim;++z)
		for(int x=0;x<xDim;++x,++vPtr,++vInd)
			{
			/* Store the vertex' texture coordinate: */
			if(texCoord.getValue()!=0)
				vPtr->texCoord=texCoord.getValue()->point.getValue(vInd);
			else
				vPtr->texCoord=Vertex::TexCoord(Scalar(x)/Scalar(xDim-1),Scalar(z)/Scalar(zDim-1));
			
			/* Store the vertex' color: */
			if(color.getValue()!=0)
				vPtr->color=Vertex::Color(color.getValue()->color.getValue(vInd));
			else
				vPtr->color=Vertex::Color(255,255,255);
			
			/* Calculate the vertex' position and normal: */
			Point p;
			p[0]=origin.getValue()[0]+Scalar(x)*Scalar(xSp);
			p[1]=origin.getValue()[1]+height.getValue(z*xDim+x);
			p[2]=origin.getValue()[2]+Scalar(z)*Scalar(zSp);
			Vector n;
			if(normal.getValue()!=0)
				n=Geometry::normalize(normal.getValue()->vector.getValue(vInd));
			else
				n=calcVertexNormal(x,z);
			if(!heightIsY.getValue())
				{
				std::swap(p[1],p[2]);
				std::swap(n[1],n[2]);
				n=-n;
				}
			
			/* Store the vertex position and normal: */
			if(pointTransform.getValue()!=0)
				{
				vPtr->normal=Vertex::Normal(pointTransform.getValue()->transformNormal(p,n));
				vPtr->position=Vertex::Position(pointTransform.getValue()->transformPoint(p));
				}
			else
				{
				vPtr->normal=Vertex::Normal(n);
				vPtr->position=Vertex::Position(p);
				}
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
				iPtr[0]=GLuint(z*xDim+x);
				iPtr[1]=GLuint((z+1)*xDim+x);
				}
		}
	else
		{
		for(int z=0;z<zDim-1;++z)
			for(int x=0;x<xDim;++x,iPtr+=2)
				{
				iPtr[0]=GLuint((z+1)*xDim+x);
				iPtr[1]=GLuint(z*xDim+x);
				}
		}
	
	glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
	}

void ElevationGridNode::uploadQuadSet(void) const
	{
	/* Define the vertex type used in the vertex array: */
	typedef GLGeometry::Vertex<Scalar,2,GLubyte,4,Scalar,Scalar,3> Vertex;
	
	/* Initialize the vertex buffer object: */
	int xDim=xDimension.getValue();
	int zDim=zDimension.getValue();
	Scalar xSp=xSpacing.getValue();
	Scalar zSp=zSpacing.getValue();
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,(xDim-1)*(zDim-1)*4*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
	
	/* Store all vertices: */
	Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	size_t qInd=0;
	for(int z=0;z<zDim-1;++z)
		for(int x=0;x<xDim-1;++x,vPtr+=4,++qInd)
			{
			size_t vInd=z*xDim+x;
			Vertex v[4];
			
			/* Calculate the corner texture coordinates of the current quad: */
			if(texCoord.getValue()!=0)
				{
				v[0].texCoord=Vertex::TexCoord(texCoord.getValue()->point.getValue(vInd));
				v[1].texCoord=Vertex::TexCoord(texCoord.getValue()->point.getValue(vInd+1));
				v[2].texCoord=Vertex::TexCoord(texCoord.getValue()->point.getValue(vInd+xDim+1));
				v[3].texCoord=Vertex::TexCoord(texCoord.getValue()->point.getValue(vInd+xDim));
				}
			else
				{
				v[0].texCoord=Vertex::TexCoord(Scalar(x)/Scalar(xDim-1),Scalar(z)/Scalar(zDim-1));
				v[1].texCoord=Vertex::TexCoord(Scalar(x+1)/Scalar(xDim-1),Scalar(z)/Scalar(zDim-1));
				v[2].texCoord=Vertex::TexCoord(Scalar(x+1)/Scalar(xDim-1),Scalar(z+1)/Scalar(zDim-1));
				v[3].texCoord=Vertex::TexCoord(Scalar(x)/Scalar(xDim-1),Scalar(z+1)/Scalar(zDim-1));
				}
			
			/* Get the corner colors of the current quad: */
			if(color.getValue()!=0)
				{
				if(colorPerVertex.getValue())
					{
					v[0].color=Vertex::Color(color.getValue()->color.getValue(vInd));
					v[1].color=Vertex::Color(color.getValue()->color.getValue(vInd+1));
					v[2].color=Vertex::Color(color.getValue()->color.getValue(vInd+xDim+1));
					v[3].color=Vertex::Color(color.getValue()->color.getValue(vInd+xDim));
					}
				else
					{
					Vertex::Color c=Vertex::Color(color.getValue()->color.getValue(qInd));
					for(int i=0;i<4;++i)
						v[i].color=c;
					}
				}
			else
				{
				for(int i=0;i<4;++i)
					v[i].color=Vertex::Color(255,255,255);
				}
			
			/* Calculate the corner positions of the current quad: */
			Scalar x0=origin.getValue()[0]+Scalar(x)*xSp;
			Scalar z0=origin.getValue()[2]+Scalar(z)*zSp;
			Point cp[4];
			cp[0]=Point(x0,origin.getValue()[1]+height.getValue(vInd),z0);
			cp[1]=Point(x0+xSp,origin.getValue()[1]+height.getValue(vInd+1),z0);
			cp[2]=Point(x0+xSp,origin.getValue()[1]+height.getValue(vInd+xDim+1),z0+zSp);
			cp[3]=Point(x0,origin.getValue()[1]+height.getValue(vInd+xDim),z0+zSp);
			
			/* Calculate the corner normal vectors of the current quad: */
			Vector cn[4];
			if(normalPerVertex.getValue())
				{
				if(normal.getValue()!=0)
					{
					cn[0]=Geometry::normalize(normal.getValue()->vector.getValue(vInd));
					cn[1]=Geometry::normalize(normal.getValue()->vector.getValue(vInd+1));
					cn[2]=Geometry::normalize(normal.getValue()->vector.getValue(vInd+xDim+1));
					cn[3]=Geometry::normalize(normal.getValue()->vector.getValue(vInd+xDim));
					}
				else
					{
					cn[0]=calcVertexNormal(x,z);
					cn[1]=calcVertexNormal(x+1,z);
					cn[2]=calcVertexNormal(x+1,z+1);
					cn[3]=calcVertexNormal(x,z+1);
					}
				}
			else
				{
				Vector n;
				if(normal.getValue()!=0)
					n=normal.getValue()->vector.getValue(qInd);
				else
					{
					n[0]=(cp[0][1]-cp[1][1]-cp[2][1]+cp[3][1])*zSp;
					n[1]=Scalar(2)*xSp*zSp;
					n[2]=(cp[0][1]+cp[1][1]-cp[2][1]-cp[3][1])*xSp;
					if(!ccw.getValue())
						n=-n;
					}
				n.normalize();
				for(int i=0;i<4;++i)
					cn[i]=n;
				}
			
			/* Set the corner positions and normal vectors of the current quad: */
			for(int i=0;i<4;++i)
				{
				if(!heightIsY.getValue())
					{
					std::swap(cp[i][1],cp[i][2]);
					std::swap(cn[i][1],cn[i][2]);
					cn[i]=-cn[i];
					}
				if(pointTransform.getValue()!=0)
					{
					v[i].normal=Vertex::Normal(pointTransform.getValue()->transformNormal(cp[i],cn[i]));
					v[i].position=Vertex::Position(pointTransform.getValue()->transformPoint(cp[i]));
					}
				else
					{
					v[i].normal=Vertex::Normal(cn[i]);
					v[i].position=Vertex::Position(cp[i]);
					}
				}
			
			/* Store the corner vertices of the current quad: */
			if(ccw.getValue())
				{
				/* Store the corner vertices in counter-clockwise order: */
				for(int i=0;i<4;++i)
					vPtr[i]=v[3-i];
				}
			else
				{
				/* Store the corner vertices in clockwise order: */
				for(int i=0;i<4;++i)
					vPtr[i]=v[i];
				}
			}
	
	glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
	}

ElevationGridNode::ElevationGridNode(void)
	:colorPerVertex(true),normalPerVertex(true),
	 creaseAngle(0),
	 origin(Point::origin),
	 xDimension(0),xSpacing(0),
	 zDimension(0),zSpacing(0),
	 heightIsY(true),
	 ccw(true),solid(true),
	 version(0)
	{
	}

const char* ElevationGridNode::getStaticClassName(void)
	{
	return "ElevationGrid";
	}

const char* ElevationGridNode::getClassName(void) const
	{
	return "ElevationGrid";
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
	else if(strcmp(fieldName,"origin")==0)
		{
		vrmlFile.parseField(origin);
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
	else if(strcmp(fieldName,"heightUrl")==0)
		{
		vrmlFile.parseField(heightUrl);
		
		/* Fully qualify all URLs: */
		for(size_t i=0;i<heightUrl.getNumValues();++i)
			heightUrl.setValue(i,vrmlFile.getFullUrl(heightUrl.getValue(i)));
		}
	else if(strcmp(fieldName,"heightIsY")==0)
		{
		vrmlFile.parseField(heightIsY);
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
	/* Check whether the height field should be loaded from a file: */
	if(heightUrl.getNumValues()>0)
		{
		try
			{
			/* Load the elevation grid's height values: */
			loadElevationGrid(*this);
			}
		catch(std::runtime_error err)
			{
			/* Carry on... */
			}
		}
	
	/* Check whether the elevation grid is valid: */
	valid=xDimension.getValue()>0&&zDimension.getValue()>0&&height.getNumValues()>=size_t(xDimension.getValue())*size_t(zDimension.getValue());
	
	/* Check whether the elevation grid can be represented by a set of indexed triangle strips: */
	indexed=(color.getValue()==0||colorPerVertex.getValue())&&normalPerVertex.getValue();
	
	/* Bump up the elevation grid's version number: */
	++version;
	}

Box ElevationGridNode::calcBoundingBox(void) const
	{
	Box result=Box::empty;
	
	if(valid)
		{
		if(pointTransform.getValue()!=0)
			{
			/* Return the bounding box of the transformed point coordinates: */
			if(heightIsY.getValue())
				{
				MFFloat::ValueList::const_iterator hIt=height.getValues().begin();
				Point p;
				p[2]=origin.getValue()[2];
				for(int z=0;z<zDimension.getValue();++z,p[2]+=zSpacing.getValue())
					{
					p[0]=origin.getValue()[0];
					for(int x=0;x<xDimension.getValue();++x,p[0]+=xSpacing.getValue(),++hIt)
						{
						p[1]=origin.getValue()[1]+*hIt;
						result.addPoint(pointTransform.getValue()->transformPoint(p));
						}
					}
				}
			else
				{
				MFFloat::ValueList::const_iterator hIt=height.getValues().begin();
				Point p;
				p[1]=origin.getValue()[1];
				for(int z=0;z<zDimension.getValue();++z,p[1]+=zSpacing.getValue())
					{
					p[0]=origin.getValue()[0];
					for(int x=0;x<xDimension.getValue();++x,p[0]+=xSpacing.getValue(),++hIt)
						{
						p[2]=origin.getValue()[2]+*hIt;
						result.addPoint(pointTransform.getValue()->transformPoint(p));
						}
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
			if(heightIsY.getValue())
				result=Box(origin.getValue()+Vector(0,yMin,0),origin.getValue()+Vector(Scalar(xDimension.getValue()-1)*xSpacing.getValue(),yMax,Scalar(zDimension.getValue()-1)*zSpacing.getValue()));
			else
				result=Box(origin.getValue()+Vector(0,0,yMin),origin.getValue()+Vector(Scalar(xDimension.getValue()-1)*xSpacing.getValue(),Scalar(zDimension.getValue()-1)*zSpacing.getValue(),yMax));
			}
		}
	
	return result;
	}

void ElevationGridNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Bail out if the elevation grid is invalid: */
	if(!valid)
		return;
	
	/* Set up OpenGL state: */
	if(solid.getValue())
		renderState.enableCulling(GL_BACK);
	else
		renderState.disableCulling();
	
	/* Get the context data item: */
	DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
	
	typedef GLGeometry::Vertex<Scalar,2,GLubyte,4,Scalar,Scalar,3> Vertex;
	
	/* Bind the vertex buffer object: */
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferObjectId);
	
	/* Set up the vertex arrays: */
	int vertexArrayParts=Vertex::getPartsMask();
	if(color.getValue()==0)
		{
		/* Disable the color vertex array: */
		vertexArrayParts&=~GLVertexArrayParts::Color;
		}
	GLVertexArrayParts::enable(vertexArrayParts);
	glVertexPointer(static_cast<Vertex*>(0));
	
	if(indexed)
		{
		/* Bind the index buffer object: */
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,dataItem->indexBufferObjectId);
		
		/* Check if the buffers are current: */
		if(dataItem->version!=version)
			{
			/* Upload the set of indexed quad strips: */
			uploadIndexedQuadStripSet();
			
			/* Mark the buffers as up-to-date: */
			dataItem->version=version;
			}
		
		/* Draw the elevation grid as a set of indexed quad strips: */
		const GLuint* iPtr=0;
		for(int z=0;z<zDimension.getValue()-1;++z,iPtr+=xDimension.getValue()*2)
			glDrawElements(GL_QUAD_STRIP,xDimension.getValue()*2,GL_UNSIGNED_INT,iPtr);
		
		/* Protect the index buffer object: */
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
		}
	else
		{
		/* Check if the buffer is current: */
		if(dataItem->version!=version)
			{
			/* Upload the set of quads: */
			uploadQuadSet();
			
			/* Mark the buffers as up-to-date: */
			dataItem->version=version;
			}
		
		/* Draw the elevation grid as a set of quads: */
		glDrawArrays(GL_QUADS,0,(xDimension.getValue()-1)*(zDimension.getValue()-1)*4);
		}
	
	/* Reset the vertex arrays: */
	GLVertexArrayParts::disable(vertexArrayParts);
	
	/* Protect the vertex buffer object: */
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	}

void ElevationGridNode::initContext(GLContextData& contextData) const
	{
	/* Create a data item and store it in the context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

}
