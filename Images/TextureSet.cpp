/***********************************************************************
TextureSet - Class to manage a set of RGB images as OpenGL textures.
Copyright (c) 2016 Oliver Kreylos

This file is part of the Image Handling Library (Images).

The Image Handling Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Image Handling Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Image Handling Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Images/TextureSet.h>

#include <Misc/ThrowStdErr.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBTextureNonPowerOfTwo.h>
#include <GL/Extensions/GLARBTextureRectangle.h>
#include <GL/Extensions/GLEXTFramebufferObject.h>

namespace Images {

/************************************
Methods of class TextureSet::GLState:
************************************/

void TextureSet::GLState::insertTexture(unsigned int position,const TextureSet::Texture& source,GLuint textureObjectId)
	{
	/* Create a new texture entry: */
	Texture newTex(source.key,&source.image);
	
	/* Check if the given texture object is valid: */
	if(textureObjectId!=0U)
		{
		/* Use the given texture object: */
		newTex.textureObjectId=textureObjectId;
		}
	else
		{
		/* Allocate a new texture object: */
		glGenTextures(1,&newTex.textureObjectId);
		}
		
	/* Calculate texture coordinate ranges: */
	newTex.texCoordMin[0]=0.0f;
	newTex.texCoordMin[1]=0.0f;
	if(source.target==GL_TEXTURE_RECTANGLE_ARB)
		{
		/* Use integer texture coordinates: */
		newTex.texCoordMax[0]=GLfloat(source.image.getWidth());
		newTex.texCoordMax[1]=GLfloat(source.image.getHeight());
		}
	else if(haveNpotdTextures)
		{
		/* Use normalized texture coordinates: */
		newTex.texCoordMax[0]=1.0f;
		newTex.texCoordMax[1]=1.0f;
		}
	else
		{
		for(int i=0;i<2;++i)
			{
			/* Calculate the power-of-two padded texture size: */
			unsigned int paddedSize;
			for(paddedSize=1U;paddedSize<source.image.getSize(i);paddedSize<<=1)
				;
			
			/* Calculate the ratio of original to padded texture size: */
			newTex.texCoordMax[i]=GLfloat(source.image.getSize(i))/GLfloat(paddedSize);
			}
		}
	
	/* Insert the new texture into the set: */
	textures.insert(textures.begin()+position,newTex);
	}

void TextureSet::GLState::removeTexture(unsigned int position)
	{
	/* Release the texture's OpenGL texture object: */
	glDeleteTextures(1,&textures[position].textureObjectId);
	
	/* Remove the texture from the set: */
	textures.erase(textures.begin()+position);
	}

TextureSet::GLState::GLState(const TextureSet& sTextureSet)
	:textureSet(sTextureSet),
	 haveNpotdTextures(GLARBTextureNonPowerOfTwo::isSupported()),
	 haveGenerateMipmap(GLEXTFramebufferObject::isSupported())
	{
	/* Initialize all supported OpenGL extensions: */
	if(haveNpotdTextures)
		GLARBTextureNonPowerOfTwo::initExtension();
	if(haveGenerateMipmap)
		GLEXTFramebufferObject::initExtension();
	
	/* Copy the texture set's texture set: */
	unsigned int numTextures=textureSet.textures.size();
	GLuint* textureObjectIds=new GLuint[numTextures];
	glGenTextures(numTextures,textureObjectIds);
	textures.reserve(textureSet.textures.size());
	for(unsigned int i=0;i<numTextures;++i)
		{
		/* Insert a new texture at the end of the list: */
		insertTexture(i,textureSet.textures[i],textureObjectIds[i]);
		}
	delete[] textureObjectIds;
	
	/* Mark the texture set as up-to-date: */
	textureSetVersion=textureSet.textureSetVersion;
	}

TextureSet::GLState::~GLState(void)
	{
	/* Release all allocated texture objects: */
	unsigned int numTextures=textures.size();
	GLuint* textureObjectIds=new GLuint[numTextures];
	for(unsigned int i=0;i<numTextures;++i)
		textureObjectIds[i]=textures[i].textureObjectId;
	glDeleteTextures(numTextures,textureObjectIds);
	delete[] textureObjectIds;
	}

const TextureSet::GLState::Texture& TextureSet::GLState::bindTexture(TextureSet::Key key)
	{
	/* Find the texture of the given key via binary search: */
	unsigned int l=0;
	unsigned int r=textures.size();
	while(r-l>1)
		{
		unsigned int m=(l+r)>>1;
		if(textures[m].key<=key)
			l=m;
		else
			r=m;
		}
	if(l==r||textures[l].key!=key)
		Misc::throwStdErr("Images::TextureSet::GLState::bindTexture: Key %u not found in texture set",(unsigned int)(key));
	
	/* Bind the texture: */
	Texture& t=textures[l];
	const TextureSet::Texture& s=textureSet.textures[l];
	glBindTexture(s.target,t.textureObjectId);
	
	/* Check if the texture settings are outdated: */
	if(t.settingsVersion!=s.settingsVersion)
		{
		/* Upload new settings: */
		glTexParameteri(s.target,GL_TEXTURE_BASE_LEVEL,s.mipmapRange[0]);
		glTexParameteri(s.target,GL_TEXTURE_MAX_LEVEL,s.mipmapRange[1]);
		glTexParameteri(s.target,GL_TEXTURE_WRAP_S,s.wrapModes[0]);
		glTexParameteri(s.target,GL_TEXTURE_WRAP_T,s.wrapModes[1]);
		glTexParameteri(s.target,GL_TEXTURE_MIN_FILTER,s.filterModes[0]);
		glTexParameteri(s.target,GL_TEXTURE_MAG_FILTER,s.filterModes[1]);
		
		/* Mark cached settings as up-to-date: */
		t.settingsVersion=s.settingsVersion;
		}
	
	/* Check if the texture image is outdated: */
	if(t.imageVersion!=s.imageVersion)
		{
		/* Upload the new texture image into the base mipmap level: */
		s.image.glTexImage2D(s.target,s.mipmapRange[0],s.internalFormat,!haveNpotdTextures);
		
		/* Check if mipmap levels are requested and automatic mipmap generation is supported: */
		if(s.mipmapRange[1]>s.mipmapRange[0]&&haveGenerateMipmap)
			{
			/* Auto-generate all requested mipmap levels: */
			glGenerateMipmapEXT(s.target);
			}
		
		/* Mark the cached image as up-to-date: */
		t.imageVersion=s.imageVersion;
		}
	
	/* Return the texture state: */
	return t;
	}

/***************************
Methods of class TextureSet:
***************************/

TextureSet::TextureSet(void)
	:textureSetVersion(0U)
	{
	}

TextureSet::~TextureSet(void)
	{
	}

void TextureSet::initContext(GLContextData& contextData) const
	{
	/* Create a new GL state item and associate it with the OpenGL context: */
	GLState* glState=new GLState(*this);
	contextData.addDataItem(this,glState);
	}

TextureSet::Texture& TextureSet::addTexture(const Images::BaseImage& newImage,GLenum newTarget,GLenum newInternalFormat)
	{
	/* Find an unused key for the new texture: */
	Key newKey=textures.empty()?0:textures.back().key+1U;
	
	/* Create a new texture set entry: */
	textures.push_back(Texture());
	Texture& newTexture=textures.back();
	newTexture.key=newKey;
	newTexture.image=newImage;
	newTexture.target=newTarget;
	newTexture.internalFormat=newInternalFormat;
	newTexture.imageVersion=1U;
	newTexture.wrapModes[0]=GL_CLAMP;
	newTexture.wrapModes[1]=GL_CLAMP;
	newTexture.filterModes[0]=GL_NEAREST;
	newTexture.filterModes[1]=GL_NEAREST;
	newTexture.settingsVersion=1U;
	
	/* Invalidate the cached texture set: */
	++textureSetVersion;
	
	/* Return the new texture: */
	return newTexture;
	}

TextureSet::Texture& TextureSet::addTexture(const Images::BaseImage& newImage,GLenum newTarget,GLenum newInternalFormat,TextureSet::Key newKey)
	{
	/* Find a place for the given key in the texture set: */
	unsigned int l=0; // Actually -1, but we subtract 1 mentally
	unsigned int r=textures.size()+1; // Actually textures.size(), but we subtract 1 mentally
	while(r-l>1)
		{
		unsigned int m=(l+r)>>1;
		if(textures[m-1].key<=newKey)
			l=m;
		else
			r=m;
		}
	
	/* Check if the requested key already exists: */
	if(l>0&&l<textures.size()+1&&textures[l-1].key==newKey)
		Misc::throwStdErr("Images::TextureSet:addTexture: Requested key %u already exists in set",(unsigned int)(newKey));
	
	/* Insert a new texture: */
	textures.insert(textures.begin()+(r-1),Texture());
	Texture& newTexture=textures[r-1];
	newTexture.key=newKey;
	newTexture.image=newImage;
	newTexture.target=newTarget;
	newTexture.internalFormat=newInternalFormat;
	newTexture.imageVersion=1U;
	newTexture.wrapModes[0]=GL_CLAMP;
	newTexture.wrapModes[1]=GL_CLAMP;
	newTexture.filterModes[0]=GL_NEAREST;
	newTexture.filterModes[1]=GL_NEAREST;
	newTexture.settingsVersion=1U;
	
	/* Invalidate the cached texture set: */
	++textureSetVersion;
	
	/* Return the new texture: */
	return newTexture;
	}

const TextureSet::Texture& TextureSet::getTexture(TextureSet::Key key) const
	{
	/* Find the texture of the given key via binary search: */
	unsigned int l=0;
	unsigned int r=textures.size();
	while(r-l>1)
		{
		unsigned int m=(l+r)>>1;
		if(textures[m].key<=key)
			l=m;
		else
			r=m;
		}
	if(l==r||textures[l].key!=key)
		Misc::throwStdErr("Images::TextureSet::getTexture: Key %u not found in texture set",(unsigned int)(key));
	
	/* Return the found texture: */
	return textures[l];
	}

TextureSet::Texture& TextureSet::getTexture(TextureSet::Key key)
	{
	/* Find the texture of the given key via binary search: */
	unsigned int l=0;
	unsigned int r=textures.size();
	while(r-l>1)
		{
		unsigned int m=(l+r)>>1;
		if(textures[m].key<=key)
			l=m;
		else
			r=m;
		}
	if(l==r||textures[l].key!=key)
		Misc::throwStdErr("Images::TextureSet::getTexture: Key %u not found in texture set",(unsigned int)(key));
	
	/* Return the found textur: */
	return textures[l];
	}

void TextureSet::deleteTexture(TextureSet::Key key)
	{
	/* Find the texture of the given key via binary search: */
	unsigned int l=0;
	unsigned int r=textures.size();
	while(r-l>1)
		{
		unsigned int m=(l+r)>>1;
		if(textures[m].key<=key)
			l=m;
		else
			r=m;
		}
	if(l==r||textures[l].key!=key)
		Misc::throwStdErr("Images::TextureSet::deleteTexture: Key %u not found in texture set",(unsigned int)(key));
	
	/* Delete the found texture: */
	textures.erase(textures.begin()+l);
	
	/* Invalidate the cached texture set: */
	++textureSetVersion;
	}

TextureSet::GLState* TextureSet::getGLState(GLContextData& contextData) const
	{
	/* Retrieve the context's GL state item: */
	GLState* glState=contextData.retrieveDataItem<GLState>(this);
	
	/* Check if the per-context texture set is outdated: */
	if(glState->textureSetVersion!=textureSetVersion)
		{
		/* Compare the two texture sets and insert or remove elements as needed: */
		unsigned int s0=textures.size();
		unsigned int s1=glState->textures.size();
		unsigned int i=0;
		while(i<s0||i<s1)
			{
			if(i>=s1||(i<s0&&textures[i].key<glState->textures[i].key))
				{
				/* Insert a new texture into the per-context set: */
				glState->insertTexture(i,textures[i]);
				++s1;
				
				/* Two entries are now identical; move on: */
				++i;
				}
			else if(i>=s0||(i<s1&&textures[i].key>glState->textures[i].key))
				{
				/* Remove a texture from the per-context set: */
				glState->removeTexture(i);
				--s1;
				}
			else
				++i;
			}
		
		/* Mark the per-context texture set as up-to-date: */
		glState->textureSetVersion=textureSetVersion;
		}
	
	return glState;
	}

}
