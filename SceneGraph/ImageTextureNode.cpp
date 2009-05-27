/***********************************************************************
ImageTextureNode - Class for textures loaded from external image files.
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

#include <SceneGraph/ImageTextureNode.h>

#include <string.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <Images/RGBImage.h>
#include <Images/ReadImageFile.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/*******************************************
Methods of class ImageTextureNode::DataItem:
*******************************************/

ImageTextureNode::DataItem::DataItem(void)
	:textureObjectId(0)
	{
	glGenTextures(1,&textureObjectId);
	}

ImageTextureNode::DataItem::~DataItem(void)
	{
	glDeleteTextures(1,&textureObjectId);
	}

/*********************************
Methods of class ImageTextureNode:
*********************************/

ImageTextureNode::ImageTextureNode(void)
	:repeatS(true),repeatT(true)
	{
	}

void ImageTextureNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"url")==0)
		{
		vrmlFile.parseField(url);
		
		/* Fully qualify all URLs: */
		for(size_t i=0;i<url.getNumValues();++i)
			url.setValue(i,vrmlFile.getFullUrl(url.getValue(i)));
		}
	else if(strcmp(fieldName,"repeatS")==0)
		{
		vrmlFile.parseField(repeatS);
		}
	else if(strcmp(fieldName,"repeatT")==0)
		{
		vrmlFile.parseField(repeatT);
		}
	else
		TextureNode::parseField(fieldName,vrmlFile);
	}

void ImageTextureNode::update(void)
	{
	}

void ImageTextureNode::setGLState(GLRenderState& renderState) const
	{
	if(url.getNumValues()>0)
		{
		/* Enable 2D textures: */
		renderState.enableTexture2D();
		
		/* Get the data item: */
		DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
		
		/* Bind the texture object: */
		glBindTexture(GL_TEXTURE_2D,dataItem->textureObjectId);
		}
	else
		{
		/* Disable texture mapping: */
		renderState.disableTextures();
		}
	}

void ImageTextureNode::resetGLState(GLRenderState& renderState) const
	{
	if(url.getNumValues()>0)
		{
		/* Unbind the texture object: */
		glBindTexture(GL_TEXTURE_2D,0);
		}
	
	/* Don't do anything else; next guy cleans up */
	}

void ImageTextureNode::initContext(GLContextData& contextData) const
	{
	if(url.getNumValues()>0)
		{
		/* Create a data item and store it in the GL context: */
		DataItem* dataItem=new DataItem;
		contextData.addDataItem(this,dataItem);
		
		/* Load the texture image: */
		Images::RGBImage texture=Images::readImageFile(url.getValue(0).c_str());
		
		/* Upload the texture image: */
		glBindTexture(GL_TEXTURE_2D,dataItem->textureObjectId);
		
		texture.glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,false);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,repeatS.getValue()?GL_REPEAT:GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,repeatT.getValue()?GL_REPEAT:GL_CLAMP);
		
		glBindTexture(GL_TEXTURE_2D,0);
		}
	}

}
