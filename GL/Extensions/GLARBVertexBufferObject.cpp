/***********************************************************************
GLARBVertexBufferObject - OpenGL extension class for the
GL_ARB_vertex_buffer_object extension.
Copyright (c) 2005-2006 Oliver Kreylos

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

#include <GL/Extensions/GLARBVertexBufferObject.h>

/************************************************
Static elements of class GLARBVertexBufferObject:
************************************************/

GL_THREAD_LOCAL(GLARBVertexBufferObject*) GLARBVertexBufferObject::current=0;

/****************************************
Methods of class GLARBVertexBufferObject:
****************************************/

GLARBVertexBufferObject::GLARBVertexBufferObject(void)
	:glBindBufferARBProc(GLExtensionManager::getFunction<PFNGLBINDBUFFERARBPROC>("glBindBufferARB")),
	 glDeleteBuffersARBProc(GLExtensionManager::getFunction<PFNGLDELETEBUFFERSARBPROC>("glDeleteBuffersARB")),
	 glGenBuffersARBProc(GLExtensionManager::getFunction<PFNGLGENBUFFERSARBPROC>("glGenBuffersARB")),
	 glIsBufferARBProc(GLExtensionManager::getFunction<PFNGLISBUFFERARBPROC>("glIsBufferARB")),
	 glBufferDataARBProc(GLExtensionManager::getFunction<PFNGLBUFFERDATAARBPROC>("glBufferDataARB")),
	 glBufferSubDataARBProc(GLExtensionManager::getFunction<PFNGLBUFFERSUBDATAARBPROC>("glBufferSubDataARB")),
	 glGetBufferSubDataARBProc(GLExtensionManager::getFunction<PFNGLGETBUFFERSUBDATAARBPROC>("glGetBufferSubDataARB")),
	 glMapBufferARBProc(GLExtensionManager::getFunction<PFNGLMAPBUFFERARBPROC>("glMapBufferARB")),
	 glUnmapBufferARBProc(GLExtensionManager::getFunction<PFNGLUNMAPBUFFERARBPROC>("glUnmapBufferARB")),
	 glGetBufferParameterivARBProc(GLExtensionManager::getFunction<PFNGLGETBUFFERPARAMETERIVARBPROC>("glGetBufferParameterivARB")),
	 glGetBufferPointervARBProc(GLExtensionManager::getFunction<PFNGLGETBUFFERPOINTERVARBPROC>("glGetBufferPointervARB"))
	{
	}

GLARBVertexBufferObject::~GLARBVertexBufferObject(void)
	{
	}

const char* GLARBVertexBufferObject::getExtensionName(void) const
	{
	return "GL_ARB_vertex_buffer_object";
	}

void GLARBVertexBufferObject::activate(void)
	{
	current=this;
	}

void GLARBVertexBufferObject::deactivate(void)
	{
	current=0;
	}

bool GLARBVertexBufferObject::isSupported(void)
	{
	/* Ask the current extension manager whether the extension is supported in the current OpenGL context: */
	return GLExtensionManager::isExtensionSupported("GL_ARB_vertex_buffer_object");
	}

void GLARBVertexBufferObject::initExtension(void)
	{
	/* Check if the extension is already initialized: */
	if(!GLExtensionManager::isExtensionRegistered("GL_ARB_vertex_buffer_object"))
		{
		/* Create a new extension object: */
		GLARBVertexBufferObject* newExtension=new GLARBVertexBufferObject;
		
		/* Register the extension with the current extension manager: */
		GLExtensionManager::registerExtension(newExtension);
		}
	}
