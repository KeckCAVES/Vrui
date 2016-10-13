/***********************************************************************
GLARBVertexArrayObject - OpenGL extension class for the
GL_ARB_vertex_array_object extension.
Copyright (c) 2015 Oliver Kreylos

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

#ifndef GLEXTENSIONS_GLARBVERTEXARRAYOBJECT_INCLUDED
#define GLEXTENSIONS_GLARBVERTEXARRAYOBJECT_INCLUDED

#include <stddef.h>
#include <GL/gl.h>
#include <GL/TLSHelper.h>
#include <GL/Extensions/GLExtension.h>

/********************************
Extension-specific parts of gl.h:
********************************/

// #ifndef GL_ARB_vertex_array_object
#define GL_ARB_vertex_array_object 1

/* Extension-specific functions: */
typedef void (APIENTRY * PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef void (APIENTRY * PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint *arrays);
typedef void (APIENTRY * PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
typedef GLboolean (APIENTRY * PFNGLISVERTEXARRAYPROC)(GLuint array);

/* Extension-specific constants: */
#define GL_VERTEX_ARRAY_BINDING 0x85B5

// #endif

/* Forward declarations of friend functions: */
void glBindVertexArray(GLuint array);
void glDeleteVertexArrays(GLsizei n,const GLuint* arrays);
void glGenVertexArrays(GLsizei n,GLuint* arrays);
GLboolean glIsVertexArray(GLuint array);

class GLARBVertexArrayObject:public GLExtension
	{
	/* Elements: */
	private:
	static GL_THREAD_LOCAL(GLARBVertexArrayObject*) current; // Pointer to extension object for current OpenGL context
	static const char* name; // Extension name
	PFNGLBINDVERTEXARRAYPROC glBindVertexArrayProc;
	PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArraysProc;
	PFNGLGENVERTEXARRAYSPROC glGenVertexArraysProc;
	PFNGLISVERTEXARRAYPROC glIsVertexArrayProc;
	
	/* Constructors and destructors: */
	private:
	GLARBVertexArrayObject(void);
	public:
	virtual ~GLARBVertexArrayObject(void);
	
	/* Methods: */
	public:
	virtual const char* getExtensionName(void) const;
	virtual void activate(void);
	virtual void deactivate(void);
	static bool isSupported(void); // Returns true if the extension is supported in the current OpenGL context
	static void initExtension(void); // Initializes the extension in the current OpenGL context
	
	/* Extension entry points: */
	inline friend void glBindVertexArray(GLuint array)
		{
		GLARBVertexArrayObject::current->glBindVertexArrayProc(array);
		}
	inline friend void glDeleteVertexArrays(GLsizei n,const GLuint* arrays)
		{
		GLARBVertexArrayObject::current->glDeleteVertexArraysProc(n,arrays);
		}
	inline friend void glGenVertexArrays(GLsizei n,GLuint* arrays)
		{
		GLARBVertexArrayObject::current->glGenVertexArraysProc(n,arrays);
		}
	inline friend GLboolean glIsVertexArray(GLuint array)
		{
		return GLARBVertexArrayObject::current->glIsVertexArrayProc(array);
		}
	};

/*******************************
Extension-specific entry points:
*******************************/

#endif
