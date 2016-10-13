/***********************************************************************
BaseImage - Generic base class to represent images of arbitrary pixel
formats. The image coordinate system is such that pixel (0,0) is in the
lower-left corner.
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

#include <Images/BaseImage.h>

#include <stddef.h>
#include <string.h>

namespace Images {

/***********************************************
Methods of class BaseImage::ImageRepresentation:
***********************************************/

BaseImage::ImageRepresentation::ImageRepresentation(unsigned int sWidth,unsigned int sHeight,unsigned int sNumChannels,unsigned int sChannelSize,GLenum sFormat,GLenum sScalarType)
	:refCount(1U),
	 numChannels(sNumChannels),channelSize(sChannelSize),
	 image(0),
	 format(sFormat),scalarType(sScalarType)
	{
	/* Copy the image size: */
	size[0]=sWidth;
	size[1]=sHeight;
	
	/* Allocate the image array: */
	image=new char[size_t(size[1])*size_t(size[0])*size_t(numChannels)*size_t(channelSize)];
	}

BaseImage::ImageRepresentation::ImageRepresentation(const ImageRepresentation& source)
	:refCount(1U),
	 numChannels(source.numChannels),channelSize(source.channelSize),
	 image(0),
	 format(source.format),scalarType(source.scalarType)
	{
	/* Copy the image size: */
	size[0]=source.size[0];
	size[1]=source.size[1];
	
	/* Allocate the image array: */
	image=new char[size_t(size[1])*size_t(size[0])*size_t(numChannels)*size_t(channelSize)];
	
	/* Copy the source image byte-by-byte: */
	memcpy(image,source.image,size_t(size[1])*size_t(size[0])*size_t(numChannels)*size_t(channelSize));
	}

BaseImage::ImageRepresentation::~ImageRepresentation(void)
	{
	/* Destroy the allocated image array: */
	delete[] static_cast<char*>(image);
	}

/**************************
Methods of class BaseImage:
**************************/

BaseImage& BaseImage::glReadPixels(GLint x,GLint y)
	{
	/* Un-share the image representation without retaining pixels: */
	ownRepresentation(false);
	
	/* Set up pixel processing pipeline: */
	glPixelStorei(GL_PACK_ALIGNMENT,1);
	glPixelStorei(GL_PACK_SKIP_PIXELS,0);
	glPixelStorei(GL_PACK_ROW_LENGTH,0);
	glPixelStorei(GL_PACK_SKIP_ROWS,0);
	
	/* Read image: */
	::glReadPixels(x,y,rep->size[0],rep->size[1],rep->format,rep->scalarType,rep->image);
	
	return *this;
	}

void BaseImage::glDrawPixels(void) const
	{
	/* Set up pixel processing pipeline: */
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
	
	/* Write image: */
	::glDrawPixels(rep->size[0],rep->size[1],rep->format,rep->scalarType,rep->image);
	}

void BaseImage::glTexImage2D(GLenum target,GLint level,GLint internalFormat,bool padImageSize) const
	{
	/* Set up pixel processing pipeline: */
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
	
	if(padImageSize)
		{
		/* Calculate the texture width and height as the next power of two: */
		unsigned int texSize[2];
		for(int i=0;i<2;++i)
			for(texSize[i]=1;texSize[i]<rep->size[i];texSize[i]<<=1)
				;
		
		if(texSize[0]!=rep->size[0]||texSize[1]!=rep->size[1])
			{
			/* Create the padded texture image: */
			::glTexImage2D(target,level,internalFormat,texSize[0],texSize[1],0,rep->format,rep->scalarType,0);
			
			/* Upload the image: */
			::glTexSubImage2D(target,level,0,0,rep->size[0],rep->size[1],rep->format,rep->scalarType,rep->image);
			}
		else
			{
			/* Upload the image: */
			::glTexImage2D(target,level,internalFormat,rep->size[0],rep->size[1],0,rep->format,rep->scalarType,rep->image);
			}
		}
	else
		{
		/* Upload the image: */
		::glTexImage2D(target,level,internalFormat,rep->size[0],rep->size[1],0,rep->format,rep->scalarType,rep->image);
		}
	}

void BaseImage::glTexSubImage2D(GLenum target,GLint level,GLint xOffset,GLint yOffset) const
	{
	/* Set up pixel processing pipeline: */
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
	
	/* Upload the image: */
	::glTexSubImage2D(target,level,xOffset,yOffset,rep->size[0],rep->size[1],rep->format,rep->scalarType,rep->image);
	}

void BaseImage::glTexSubImage3D(GLenum target,GLint level,GLint xOffset,GLint yOffset,GLint zOffset) const
	{
	/* Set up pixel processing pipeline: */
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
	
	/* Upload the image: */
	::glTexSubImage3D(target,level,xOffset,yOffset,zOffset,rep->size[0],rep->size[1],1,rep->format,rep->scalarType,rep->image);
	}

}
