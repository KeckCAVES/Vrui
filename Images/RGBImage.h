/***********************************************************************
RGBImage - Specialized image class to represent RGB images with 8-bit
color depth.
Copyright (c) 2005-2016 Oliver Kreylos

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

#ifndef IMAGES_RGBIMAGE_INCLUDED
#define IMAGES_RGBIMAGE_INCLUDED

#include <GL/gl.h>
#include <Images/Image.h>

namespace Images {

class RGBImage:public Image<GLubyte,3>
	{
	/* Embedded classes: */
	public:
	typedef Image<GLubyte,3> Base; // Type of base class
	
	/* Constructors and destructors: */
	public:
	RGBImage(void) // Creates an invalid image
		{
		}
	RGBImage(unsigned int sWidth,unsigned int sHeight) // Creates an uninitialized image of the given size
		:Base(sWidth,sHeight,GL_RGB)
		{
		}
	explicit RGBImage(const BaseImage& source) // Copies an existing base image (does not copy image representation); throws exception if base image format does not match pixel type
		:Base(source)
		{
		}
	RGBImage(const RGBImage& source) // Copies an existing image (does not copy image representation)
		:Base(source)
		{
		}
	RGBImage& operator=(const BaseImage& source) // Assigns an existing base image (does not copy image representation); throws exception if base image format does not match pixel type
		{
		Base::operator=(source);
		return *this;
		}
	RGBImage& operator=(const RGBImage& source) // Assigns an existing image (does not copy image representation)
		{
		Base::operator=(source);
		return *this;
		}
	
	/* Methods: */
	using Base::glReadPixels;
	static RGBImage glReadPixels(GLint x,GLint y,GLsizei width,GLsizei height) // Returns a new image created by reading from the frame buffer
		{
		/* Create a new image of the requested size: */
		RGBImage result(width,height);
		
		/* Read from the frame buffer: */
		result.glReadPixels(x,y);
		
		return result;
		}
	};

}

#endif
