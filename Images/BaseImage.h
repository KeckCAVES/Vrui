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

#ifndef IMAGES_BASEIMAGE_INCLUDED
#define IMAGES_BASEIMAGE_INCLUDED

#include <Threads/Atomic.h>
#include <GL/gl.h>

namespace Images {

class BaseImage
	{
	private:
	struct ImageRepresentation // Structure to represent an image to allow non-copy sharing and passing of images
		{
		/* Elements: */
		public:
		Threads::Atomic<unsigned int> refCount; // Number of Image objects referencing this representation
		unsigned int size[2]; // Image size (width, height)
		unsigned int numChannels; // Number of interleaved channels in the image
		unsigned int channelSize; // Storage size of one pixel component in bytes
		void* image; // Untyped pointer to the image array
		GLenum format; // OpenGL texture format compatible with this image (GL_RGB etc.)
		GLenum scalarType; // OpenGL scalar type compatible with this image (GL_UNSIGNED_BYTE etc.)
		
		/* Constructors and destructors: */
		ImageRepresentation(unsigned int sWidth,unsigned int sHeight,unsigned int sNumChannels,unsigned int sChannelSize,GLenum sFormat,GLenum sScalarType); // Creates a new image representation with uninitialized data
		ImageRepresentation(const ImageRepresentation& source); // Copies an existing image representation
		~ImageRepresentation(void); // Destroys an image representation
		
		/* Methods: */
		ImageRepresentation* attach(void)
			{
			refCount.preAdd(1);
			return this;
			}
		void detach(void)
			{
			if(refCount.preSub(1)==0)
				delete this;
			}
		};
	
	/* Elements: */
	private:
	ImageRepresentation* rep; // Pointer to a (shared) image representation
	
	/* Private methods: */
	void ownRepresentation(bool copyPixels) // Ensures that image representation is not shared; copies current pixels if flag is true
		{
		/* Copy the image representation if it is shared: */
		if(rep->refCount.get()>1U)
			{
			ImageRepresentation* newRep=copyPixels?new ImageRepresentation(*rep):new ImageRepresentation(rep->size[0],rep->size[1],rep->numChannels,rep->channelSize,rep->format,rep->scalarType);
			rep->detach();
			rep=newRep;
			}
		}
	
	/* Constructors and destructors: */
	public:
	BaseImage(void) // Creates an invalid image
		:rep(0)
		{
		}
	private:
	BaseImage(ImageRepresentation* sRep) // Creates an image with a pre-attached representation
		:rep(sRep)
		{
		}
	protected:
	BaseImage(unsigned int sWidth,unsigned int sHeight,unsigned int sNumChannels,unsigned int sChannelSize,GLenum sFormat,GLenum sScalarType) // Creates an uninitialized image of the given size and format
		:rep(new ImageRepresentation(sWidth,sHeight,sNumChannels,sChannelSize,sFormat,sScalarType))
		{
		}
	public:
	BaseImage(const BaseImage& source) // Copies an existing image (does not copy image representation)
		:rep(source.rep!=0?source.rep->attach():0)
		{
		}
	BaseImage& operator=(const BaseImage& source) // Assigns an existing image (does not copy image representation)
		{
		if(rep!=source.rep)
			{
			if(rep!=0)
				rep->detach();
			rep=source.rep!=0?source.rep->attach():0;
			}
		return *this;
		}
	~BaseImage(void) // Destroys the image
		{
		if(rep!=0)
			rep->detach();
		}
	
	/* Methods: */
	bool isValid(void) const // Returns if the image has a valid representation
		{
		return rep!=0;
		}
	void invalidate(void) // Invalidates the image, i.e., detaches from any shared pixel buffers
		{
		if(rep!=0)
			rep->detach();
		rep=0;
		}
	
	/*****************************************************************************
	The following methods crash if the image does not have a valid representation!
	*****************************************************************************/
	
	const unsigned int* getSize(void) const // Returns image size
		{
		return rep->size;
		}
	unsigned int getSize(int dimension) const // Returns one dimension of image size
		{
		return rep->size[dimension];
		}
	unsigned int getWidth(void) const // Returns image width
		{
		return rep->size[0];
		}
	unsigned int getHeight(void) const // Returns image height
		{
		return rep->size[1];
		}
	unsigned int getNumChannels(void) const // Returns the number of image channels
		{
		return rep->numChannels;
		}
	unsigned int getChannelSize(void) const // Returns the storage size of one pixel component in bytes
		{
		return rep->channelSize;
		}
	const void* getPixels(void) const // Returns untyped pointer to the pixel array
		{
		return rep->image;
		}
	void* modifyPixels(void) // Ensures that the pixel array is private, and returns pointer to it
		{
		ownRepresentation(true);
		return rep->image;
		}
	void* replacePixels(void) // Ensures that the pixel array is private, but does not copy pixel values, and returns pointer to it
		{
		ownRepresentation(false);
		return rep->image;
		}
	GLenum getFormat(void) const // Returns the OpenGL texture format compatible with this image
		{
		return rep->format;
		}
	GLenum getScalarType(void) const // Returns the OpenGL scalar type compatible with this image
		{
		return rep->scalarType;
		}
	BaseImage& glReadPixels(GLint x,GLint y); // Reads the frame buffer contents into the image
	void glDrawPixels(void) const; // Writes image to the frame buffer at the current raster position
	void glTexImage2D(GLenum target,GLint level,GLint internalFormat,bool padImageSize =false) const; // Uploads an image as an OpenGL texture
	void glTexSubImage2D(GLenum target,GLint level,GLint xOffset,GLint yOffset) const; // Uploads an image as a part of a larger OpenGL texture
	void glTexSubImage3D(GLenum target,GLint level,GLint xOffset,GLint yOffset,GLint zOffset) const; // Uploads an image as a (part of) single slice of an OpenGL 3D texture
	};

}

#endif
