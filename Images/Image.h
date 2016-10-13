/***********************************************************************
Image - Base class to represent images of arbitrary pixel formats. The
image coordinate system is such that pixel (0,0) is in the lower-left
corner.
Copyright (c) 2007-2016 Oliver Kreylos

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

#ifndef IMAGES_IMAGE_INCLUDED
#define IMAGES_IMAGE_INCLUDED

#include <GL/gl.h>
#include <GL/GLScalarLimits.h>
#include <GL/GLColor.h>
#include <Images/BaseImage.h>

namespace Images {

template <class ScalarParam,int numComponentsParam>
class Image:public BaseImage
	{
	/* Embedded classes: */
	public:
	typedef ScalarParam Scalar; // Data type for pixel color components
	static const int numComponents=numComponentsParam; // Number of pixel components
	typedef GLColor<Scalar,numComponentsParam> Color; // Data type for pixel colors
	
	/* Constructors and destructors: */
	public:
	Image(void) // Creates an invalid image
		{
		}
	Image(unsigned int sWidth,unsigned int sHeight,GLenum sFormat) // Creates an uninitialized image of the given size and format
		:BaseImage(sWidth,sHeight,numComponents,sizeof(Scalar),sFormat,GLScalarLimits<Scalar>::type)
		{
		}
	explicit Image(const BaseImage& source); // Copies an existing base image (does not copy image representation); throws exception if base image format does not match pixel type
	Image(const Image& source) // Copies an existing image (does not copy image representation)
		:BaseImage(source)
		{
		}
	Image& operator=(const BaseImage& source); // Assigns an existing base image (does not copy image representation); throws exception if base image format does not match pixel type
	Image& operator=(const Image& source) // Assigns an existing image (does not copy image representation)
		{
		/* Call base class assignment operator: */
		BaseImage::operator=(source);
		return *this;
		}
	
	/* Methods: */
	using BaseImage::glReadPixels;
	static Image glReadPixels(GLint x,GLint y,GLsizei width,GLsizei height,GLenum format) // Returns a new image created by reading from the frame buffer
		{
		/* Create a new image of the requested size: */
		Image result(width,height,format);
		
		/* Read from the frame buffer: */
		result.glReadPixels(x,y);
		
		return result;
		}
	
	/*****************************************************************************
	The following methods crash if the image does not have a valid representation!
	*****************************************************************************/
	
	const Color& getPixel(unsigned int x,unsigned int y) const // Returns color of an image pixel
		{
		/* Return the pixel value: */
		return static_cast<const Color*>(BaseImage::getPixels())[size_t(y)*size_t(getWidth())+size_t(x)];
		}
	Image& setPixel(unsigned int x,unsigned int y,const Color& c) // Sets an image pixel to the given color
		{
		/* Set the pixel value: */
		static_cast<Color*>(BaseImage::modifyPixels())[size_t(y)*size_t(getWidth())+size_t(x)]=c;
		return *this;
		}
	Image& clear(const Color& c); // Sets all image pixels to the given color
	const Color* getPixels(void) const // Returns pointer to the entire image data for reading
		{
		/* Return the type-correct base image pointer: */
		return static_cast<const Color*>(BaseImage::getPixels());
		};
	Color* modifyPixels(void) // Returns pointer to the entire image for writing
		{
		/* Return the type-correct base image pointer: */
		return static_cast<Color*>(BaseImage::modifyPixels());
		};
	Color* replacePixels(void) // Returns pointer to the entire image for writing, but does not necessarily retain current pixel values
		{
		/* Return the type-correct base image pointer: */
		return static_cast<Color*>(BaseImage::replacePixels());
		};
	const Color* getPixelRow(unsigned int y) const // Returns pointer to an image row for reading
		{
		/* Return the image row pointer: */
		return static_cast<const Color*>(BaseImage::getPixels())+(size_t(y)*size_t(getWidth()));
		}
	Color* modifyPixelRow(unsigned int y) // Returns pointer to an image row for writing
		{
		/* Return the image row pointer: */
		return static_cast<Color*>(BaseImage::modifyPixels())+(size_t(y)*size_t(getWidth()));
		}
	Color* replacePixelRow(unsigned int y) // Returns pointer to an image row for writing, but does not necessarily retain current pixel values
		{
		/* Return the image row pointer: */
		return static_cast<Color*>(BaseImage::replacePixels())+(size_t(y)*size_t(getWidth()));
		}
	Image& resize(unsigned int newWidth,unsigned int newHeight); // Resamples the image to the given size
	};

}

#endif
