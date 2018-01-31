/***********************************************************************
Texture - Base class for widgets displaying dynamically-generated
textures.
Copyright (c) 2011-2017 Oliver Kreylos

This file is part of the GLMotif Widget Library (GLMotif).

The GLMotif Widget Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GLMotif Widget Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the GLMotif Widget Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef GLMOTIF_TEXTURE_INCLUDED
#define GLMOTIF_TEXTURE_INCLUDED

#include <GL/gl.h>
#include <GL/GLObject.h>
#include <GLMotif/Widget.h>

namespace GLMotif {

class Texture:public Widget,public GLObject
	{
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		bool npotdtSupported; // Flag if the OpenGL context supports non-power-of-two-dimension textures
		GLuint textureObjectId; // ID of texture object
		unsigned int textureSize[2]; // Current width and height of texture
		unsigned int version; // Version number of image data in texture object
		GLfloat regionTex[4]; // Texture coordinates to display current texture region
		unsigned int regionVersion; // Version number of displayed texture region
		unsigned int settingsVersion; // Version number of texture display settings
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	private:
	unsigned int size[2]; // Current size of the texture
	unsigned int version; // Version number of image data in texture
	GLfloat resolution[2]; // The horizontal and vertical resolution of the image in pixels per GLMotif length unit
	GLfloat region[4]; // Region of the image currently mapped to the widget's interior in pixel units
	Box textureBox; // Extents of texture display area inside the widget's interior
	unsigned int regionVersion; // Version number of displayed texture region
	GLenum interpolationMode; // Interpolation mode for texture display
	unsigned int settingsVersion; // Version number of texture display settings
	bool illuminated; // Flag whether the texture is illuminated by light sources, or emits its own light
	
	/* Protected methods: */
	virtual void uploadTexture(GLuint textureObjectId,bool npotdtSupported,const unsigned int textureSize[2],GLContextData& contextData) const =0; // Called when an image needs to be uploaded to the texture object of the given (padded) texture size; texture object is bound when this method is called
	
	/* Constructors and destructors: */
	protected:
	Texture(const char* sName,Container* sParent); // Creates a texture widget with uninitialized image; expects those to be set before managing itself
	public:
	Texture(const char* sName,Container* sParent,const unsigned int sSize[2],const GLfloat sResolution[2],bool sManageChild =true); // Creates a texture widget of the given image size at the given resolution
	
	/* Methods from Widget: */
	virtual Vector calcNaturalSize(void) const;
	virtual void resize(const Box& newExterior);
	virtual void draw(GLContextData& contextData) const;
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New methods: */
	void updateTexture(void); // Marks the texture image as outdated after a change from outside the widget
	const unsigned int* getSize(void) const // Returns the current texture size
		{
		return size;
		}
	unsigned int getSize(int dimension) const // Returns the current texture width or height
		{
		return size[dimension];
		}
	void setSize(const unsigned int newSize[2]); // Sets a new texture image size; resets region to display the entire image, and requests a resize of the widget
	const GLfloat* getResolution(void) const // Returns the current image's resolution
		{
		return resolution;
		}
	GLfloat getResolution(int dimension) const // Ditto
		{
		return resolution[dimension];
		}
	void setResolution(const GLfloat newResolution[2]); // Sets the image's resolution, and requests a resize of the widget based on the current display region
	const GLfloat* getRegion(void) const // Returns the currently displayed image region
		{
		return region;
		}
	GLfloat getRegionMin(int dimension) const // Returns the minimum of the currently displayed image region in the given dimension
		{
		return region[dimension];
		}
	GLfloat getRegionMax(int dimension) const // Returns the maximum of the currently displayed image region in the given dimension
		{
		return region[2+dimension];
		}
	void setRegion(const GLfloat newRegion[4]); // Sets the displayed image region and adapts it to the current widget aspect ratio
	GLenum getInterpolationMode(void) const // Returns the current interpolation mode for image display
		{
		return interpolationMode;
		}
	void setInterpolationMode(GLenum newInterpolationMode); // Sets the interpolation mode for image display
	bool getIlluminated(void) const // Returns true if the image is illuminated by light sources
		{
		return illuminated;
		}
	void setIlluminated(bool newIlluminated); // Sets the image illumination flag
	Point widgetToImage(const Point& widgetPoint) const // Converts a point from widget coordinates to texel coordinates
		{
		Point imagePoint;
		for(int i=0;i<2;++i)
			imagePoint[i]=(widgetPoint[i]-getInterior().origin[i])*(region[2+i]-region[0+i])/getInterior().size[i]+region[0+i];
		imagePoint[2]=widgetPoint[2];
		return imagePoint;
		}
	Point imageToWidget(const Point& imagePoint) const // Converts a point from texel coordinates to widget coordinates
		{
		Point widgetPoint;
		for(int i=0;i<2;++i)
			widgetPoint[i]=(imagePoint[i]-region[0+i])*getInterior().size[i]/(region[2+i]-region[0+i])+getInterior().origin[i];
		widgetPoint[2]=imagePoint[2];
		return widgetPoint;
		}
	};

}

#endif
