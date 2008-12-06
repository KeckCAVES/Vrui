/***********************************************************************
GLEXTPalettedTexture - OpenGL extension class for the
GL_EXT_paletted_texture extension.
Copyright (c) 2006 Oliver Kreylos

This file is part of the OpenGL Support Library (GLSupport).

The OpenGL Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenGL Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenGL Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>

#include <GL/Extensions/GLEXTPalettedTexture.h>

/*********************************************
Static elements of class GLEXTPalettedTexture:
*********************************************/

GL_THREAD_LOCAL(GLEXTPalettedTexture*) GLEXTPalettedTexture::current=0;

/*************************************
Methods of class GLEXTPalettedTexture:
*************************************/

GLEXTPalettedTexture::GLEXTPalettedTexture(void)
	{
	/* Get all function pointers: */
	glColorTableEXTProc=GLExtensionManager::getFunction<PFNGLCOLORTABLEEXTPROC>("gllColorTableEXT");
	glColorSubTableEXTProc=GLExtensionManager::getFunction<PFNGLCOLORSUBTABLEEXTPROC>("glColorSubTableEXT");
	glGetColorTableEXTProc=GLExtensionManager::getFunction<PFNGLGETCOLORTABLEEXTPROC>("glGetColorTableEXT");
	glGetColorTableParameterivEXTProc=GLExtensionManager::getFunction<PFNGLGETCOLORTABLEPARAMETERIVEXTPROC>("glGetColorTableParameterivEXT");
	glGetColorTableParameterfvEXTProc=GLExtensionManager::getFunction<PFNGLGETCOLORTABLEPARAMETERFVEXTPROC>("glGetColorTableParameterfvEXT");
	}

GLEXTPalettedTexture::~GLEXTPalettedTexture(void)
	{
	}

const char* GLEXTPalettedTexture::getExtensionName(void) const
	{
	return "GL_EXT_paletted_texture";
	}

void GLEXTPalettedTexture::activate(void)
	{
	current=this;
	}

void GLEXTPalettedTexture::deactivate(void)
	{
	current=0;
	}

bool GLEXTPalettedTexture::isSupported(void)
	{
	/* Ask the current extension manager whether the extension is supported in the current OpenGL context: */
	return GLExtensionManager::isExtensionSupported("GL_EXT_paletted_texture");
	}

void GLEXTPalettedTexture::initExtension(void)
	{
	/* Check if the extension is already initialized: */
	if(!GLExtensionManager::isExtensionRegistered("GL_EXT_paletted_texture"))
		{
		/* Create a new extension object: */
		GLEXTPalettedTexture* newExtension=new GLEXTPalettedTexture;
		
		/* Register the extension with the current extension manager: */
		GLExtensionManager::registerExtension(newExtension);
		}
	}
