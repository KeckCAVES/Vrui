/***********************************************************************
GLARBPointSprite - OpenGL extension class for the GL_ARB_point_sprite
extension.
Copyright (c) 2007 Oliver Kreylos

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

#include <GL/Extensions/GLARBPointSprite.h>

/*****************************************
Static elements of class GLARBPointSprite:
*****************************************/

GL_THREAD_LOCAL(GLARBPointSprite*) GLARBPointSprite::current=0;

/*********************************
Methods of class GLARBPointSprite:
*********************************/

GLARBPointSprite::GLARBPointSprite(void)
	{
	}

GLARBPointSprite::~GLARBPointSprite(void)
	{
	}

const char* GLARBPointSprite::getExtensionName(void) const
	{
	return "GL_ARB_point_sprite";
	}

void GLARBPointSprite::activate(void)
	{
	current=this;
	}

void GLARBPointSprite::deactivate(void)
	{
	current=0;
	}

bool GLARBPointSprite::isSupported(void)
	{
	/* Ask the current extension manager whether the extension is supported in the current OpenGL context: */
	return GLExtensionManager::isExtensionSupported("GL_ARB_point_sprite");
	}

void GLARBPointSprite::initExtension(void)
	{
	/* Check if the extension is already initialized: */
	if(!GLExtensionManager::isExtensionRegistered("GL_ARB_point_sprite"))
		{
		/* Create a new extension object: */
		GLARBPointSprite* newExtension=new GLARBPointSprite;
		
		/* Register the extension with the current extension manager: */
		GLExtensionManager::registerExtension(newExtension);
		}
	}
