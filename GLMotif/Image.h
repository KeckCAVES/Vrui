/***********************************************************************
Image - Class for widgets displaying image as textures.
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

#ifndef GLMOTIF_IMAGE_INCLUDED
#define GLMOTIF_IMAGE_INCLUDED

#include <Images/BaseImage.h>
#include <GLMotif/Texture.h>

namespace GLMotif {

class Image:public Texture
	{
	/* Elements: */
	private:
	Images::BaseImage image; // The displayed image
	
	/* Protected methods from Texture: */
	virtual void uploadTexture(GLuint textureObjectId,bool npotdtSupported,const unsigned int textureSize[2],GLContextData& contextData) const;
	
	/* Constructors and destructors: */
	public:
	Image(const char* sName,Container* sParent,const Images::BaseImage& sImage,const GLfloat sResolution[2],bool sManageChild =true); // Creates an image widget displaying the given image at the given resolution
	Image(const char* sName,Container* sParent,const char* imageFileName,const GLfloat sResolution[2],bool sManageChild =true); // Creates an image widget displaying the given image file at the given resolution
	
	/* New methods: */
	const Images::BaseImage& getImage(void) const // Returns the current image
		{
		return image;
		}
	Images::BaseImage& getImage(void) // Ditto
		{
		return image;
		}
	};

}

#endif
