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

#include <GLMotif/Image.h>

#include <GL/gl.h>
#include <Images/ReadImageFile.h>

namespace GLMotif {

/**********************
Methods of class Image:
**********************/

void Image::uploadTexture(GLuint textureObjectId,bool npotdtSupported,const unsigned int textureSize[2],GLContextData& contextData) const
	{
	/* Upload the image into the bound texture object: */
	image.glTexImage2D(GL_TEXTURE_2D,0,!npotdtSupported);
	}

Image::Image(const char* sName,Container* sParent,const Images::BaseImage& sImage,const GLfloat sResolution[2],bool sManageChild)
	:Texture(sName,sParent,sImage.getSize(),sResolution,sManageChild),
	 image(sImage)
	{
	}

Image::Image(const char* sName,Container* sParent,const char* imageFileName,const GLfloat sResolution[2],bool sManageChild)
	:Texture(sName,sParent)
	{
	/* Load the image file: */
	image=Images::readGenericImageFile(imageFileName);
	
	/* Set the image size and resolution in the base class: */
	setSize(image.getSize());
	setResolution(sResolution);
	
	/* Manage me: */
	if(sManageChild)
		manageChild();
	}

}
