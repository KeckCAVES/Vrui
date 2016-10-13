/***********************************************************************
GLEXTTextureArray - OpenGL extension class for the
GL_EXT_texture_array extension.
Copyright (c) 2016 Oliver Kreylos

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

#ifndef GLEXTENSIONS_GLEXTTEXTUREARRAY_INCLUDED
#define GLEXTENSIONS_GLEXTTEXTUREARRAY_INCLUDED

#include <GL/gl.h>
#include <GL/TLSHelper.h>
#include <GL/Extensions/GLExtension.h>

/********************************
Extension-specific parts of gl.h:
********************************/

#ifndef GL_EXT_texture_array
#define GL_EXT_texture_array 1

/* Extension-specific functions: */
typedef void (APIENTRY * PFNGLFRAMEBUFFERTEXTURELAYEREXTPROC) (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);

/* Extension-specific constants: */
#define GL_TEXTURE_1D_ARRAY_EXT           0x8C18
#define GL_PROXY_TEXTURE_1D_ARRAY_EXT     0x8C19
#define GL_TEXTURE_2D_ARRAY_EXT           0x8C1A
#define GL_PROXY_TEXTURE_2D_ARRAY_EXT     0x8C1B
#define GL_TEXTURE_BINDING_1D_ARRAY_EXT   0x8C1C
#define GL_TEXTURE_BINDING_2D_ARRAY_EXT   0x8C1D
#define GL_MAX_ARRAY_TEXTURE_LAYERS_EXT   0x88FF
#define GL_COMPARE_REF_DEPTH_TO_TEXTURE_EXT 0x884E

#endif

/* Forward declarations of friend functions: */
void glFramebufferTextureLayerEXT(GLenum target,GLenum attachment,GLuint texture,GLint level,GLint layer);


class GLEXTTextureArray:public GLExtension
	{
	/* Elements: */
	private:
	static GL_THREAD_LOCAL(GLEXTTextureArray*) current; // Pointer to extension object for current OpenGL context
	static const char* name; // Extension name
	PFNGLFRAMEBUFFERTEXTURELAYEREXTPROC glFramebufferTextureLayerEXTProc;
	
	/* Constructors and destructors: */
	private:
	GLEXTTextureArray(void);
	public:
	virtual ~GLEXTTextureArray(void);
	
	/* Methods: */
	public:
	virtual const char* getExtensionName(void) const;
	virtual void activate(void);
	virtual void deactivate(void);
	static bool isSupported(void); // Returns true if the extension is supported in the current OpenGL context
	static void initExtension(void); // Initializes the extension in the current OpenGL context
	
	/* Extension entry points: */
	inline friend void glFramebufferTextureLayerEXT(GLenum target,GLenum attachment,GLuint texture,GLint level,GLint layer)
		{
		GLEXTTextureArray::current->glFramebufferTextureLayerEXTProc(target,attachment,texture,level,layer);
		}
	};

/*******************************
Extension-specific entry points:
*******************************/

#endif
