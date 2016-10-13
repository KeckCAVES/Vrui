/***********************************************************************
GLEXTGpuShader4 - OpenGL extension class for the
GL_EXT_gpu_shader4 extension.
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

#ifndef GLEXTENSIONS_GLEXTGPUSHADER4_INCLUDED
#define GLEXTENSIONS_GLEXTGPUSHADER4_INCLUDED

#include <GL/gl.h>
#include <GL/TLSHelper.h>
#include <GL/Extensions/GLExtension.h>

/********************************
Extension-specific parts of gl.h:
********************************/

#ifndef GL_EXT_gpu_shader4
#define GL_EXT_gpu_shader4 1

/* Extension-specific functions: */
typedef void (APIENTRY * PFNGLVERTEXATTRIBI1IEXTPROC) (GLuint index, GLint x);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI2IEXTPROC) (GLuint index, GLint x, GLint y);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI3IEXTPROC) (GLuint index, GLint x, GLint y, GLint z);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI4IEXTPROC) (GLuint index, GLint x, GLint y, GLint z, GLint w);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI1UIEXTPROC) (GLuint index, GLuint x);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI2UIEXTPROC) (GLuint index, GLuint x, GLuint y);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI3UIEXTPROC) (GLuint index, GLuint x, GLuint y, GLuint z);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI4UIEXTPROC) (GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI1IVEXTPROC) (GLuint index, const GLint *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI2IVEXTPROC) (GLuint index, const GLint *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI3IVEXTPROC) (GLuint index, const GLint *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI4IVEXTPROC) (GLuint index, const GLint *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI1UIVEXTPROC) (GLuint index, const GLuint *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI2UIVEXTPROC) (GLuint index, const GLuint *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI3UIVEXTPROC) (GLuint index, const GLuint *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI4UIVEXTPROC) (GLuint index, const GLuint *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI4BVEXTPROC) (GLuint index, const GLbyte *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI4SVEXTPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI4UBVEXTPROC) (GLuint index, const GLubyte *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBI4USVEXTPROC) (GLuint index, const GLushort *v);
typedef void (APIENTRY * PFNGLVERTEXATTRIBIPOINTEREXTPROC) (GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
typedef void (APIENTRY * PFNGLGETVERTEXATTRIBIIVEXTPROC) (GLuint index, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETVERTEXATTRIBIUIVEXTPROC) (GLuint index, GLenum pname, GLuint *params);
typedef void (APIENTRY * PFNGLGETUNIFORMUIVEXTPROC) (GLuint program, GLint location, GLuint *params);
typedef void (APIENTRY * PFNGLBINDFRAGDATALOCATIONEXTPROC) (GLuint program, GLuint color, const GLchar *name);
typedef GLint (APIENTRY * PFNGLGETFRAGDATALOCATIONEXTPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRY * PFNGLUNIFORM1UIEXTPROC) (GLint location, GLuint v0);
typedef void (APIENTRY * PFNGLUNIFORM2UIEXTPROC) (GLint location, GLuint v0, GLuint v1);
typedef void (APIENTRY * PFNGLUNIFORM3UIEXTPROC) (GLint location, GLuint v0, GLuint v1, GLuint v2);
typedef void (APIENTRY * PFNGLUNIFORM4UIEXTPROC) (GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
typedef void (APIENTRY * PFNGLUNIFORM1UIVEXTPROC) (GLint location, GLsizei count, const GLuint *value);
typedef void (APIENTRY * PFNGLUNIFORM2UIVEXTPROC) (GLint location, GLsizei count, const GLuint *value);
typedef void (APIENTRY * PFNGLUNIFORM3UIVEXTPROC) (GLint location, GLsizei count, const GLuint *value);
typedef void (APIENTRY * PFNGLUNIFORM4UIVEXTPROC) (GLint location, GLsizei count, const GLuint *value);

/* Extension-specific constants: */
#define GL_VERTEX_ATTRIB_ARRAY_INTEGER_EXT 0x88FD
#define GL_SAMPLER_1D_ARRAY_EXT           0x8DC0
#define GL_SAMPLER_2D_ARRAY_EXT           0x8DC1
#define GL_SAMPLER_BUFFER_EXT             0x8DC2
#define GL_SAMPLER_1D_ARRAY_SHADOW_EXT    0x8DC3
#define GL_SAMPLER_2D_ARRAY_SHADOW_EXT    0x8DC4
#define GL_SAMPLER_CUBE_SHADOW_EXT        0x8DC5
#define GL_UNSIGNED_INT_VEC2_EXT          0x8DC6
#define GL_UNSIGNED_INT_VEC3_EXT          0x8DC7
#define GL_UNSIGNED_INT_VEC4_EXT          0x8DC8
#define GL_INT_SAMPLER_1D_EXT             0x8DC9
#define GL_INT_SAMPLER_2D_EXT             0x8DCA
#define GL_INT_SAMPLER_3D_EXT             0x8DCB
#define GL_INT_SAMPLER_CUBE_EXT           0x8DCC
#define GL_INT_SAMPLER_2D_RECT_EXT        0x8DCD
#define GL_INT_SAMPLER_1D_ARRAY_EXT       0x8DCE
#define GL_INT_SAMPLER_2D_ARRAY_EXT       0x8DCF
#define GL_INT_SAMPLER_BUFFER_EXT         0x8DD0
#define GL_UNSIGNED_INT_SAMPLER_1D_EXT    0x8DD1
#define GL_UNSIGNED_INT_SAMPLER_2D_EXT    0x8DD2
#define GL_UNSIGNED_INT_SAMPLER_3D_EXT    0x8DD3
#define GL_UNSIGNED_INT_SAMPLER_CUBE_EXT  0x8DD4
#define GL_UNSIGNED_INT_SAMPLER_2D_RECT_EXT 0x8DD5
#define GL_UNSIGNED_INT_SAMPLER_1D_ARRAY_EXT 0x8DD6
#define GL_UNSIGNED_INT_SAMPLER_2D_ARRAY_EXT 0x8DD7
#define GL_UNSIGNED_INT_SAMPLER_BUFFER_EXT 0x8DD8
#define GL_MIN_PROGRAM_TEXEL_OFFSET_EXT   0x8904
#define GL_MAX_PROGRAM_TEXEL_OFFSET_EXT   0x8905

#endif

/* Forward declarations of friend functions: */
void glVertexAttribI1iEXT(GLuint index,GLint x);
void glVertexAttribI2iEXT(GLuint index,GLint x,GLint y);
void glVertexAttribI3iEXT(GLuint index,GLint x,GLint y,GLint z);
void glVertexAttribI4iEXT(GLuint index,GLint x,GLint y,GLint z,GLint w);
void glVertexAttribI1uiEXT(GLuint index,GLuint x);
void glVertexAttribI2uiEXT(GLuint index,GLuint x,GLuint y);
void glVertexAttribI3uiEXT(GLuint index,GLuint x,GLuint y,GLuint z);
void glVertexAttribI4uiEXT(GLuint index,GLuint x,GLuint y,GLuint z,GLuint w);
void glVertexAttribI1ivEXT(GLuint index,const GLint* v);
void glVertexAttribI2ivEXT(GLuint index,const GLint* v);
void glVertexAttribI3ivEXT(GLuint index,const GLint* v);
void glVertexAttribI4ivEXT(GLuint index,const GLint* v);
void glVertexAttribI1uivEXT(GLuint index,const GLuint* v);
void glVertexAttribI2uivEXT(GLuint index,const GLuint* v);
void glVertexAttribI3uivEXT(GLuint index,const GLuint* v);
void glVertexAttribI4uivEXT(GLuint index,const GLuint* v);
void glVertexAttribI4bvEXT(GLuint index,const GLbyte* v);
void glVertexAttribI4svEXT(GLuint index,const GLshort* v);
void glVertexAttribI4ubvEXT(GLuint index,const GLubyte* v);
void glVertexAttribI4usvEXT(GLuint index,const GLushort* v);
void glVertexAttribIPointerEXT(GLuint index,GLint size,GLenum type,GLsizei stride,const void* pointer);
void glGetVertexAttribIivEXT(GLuint index,GLenum pname,GLint* params);
void glGetVertexAttribIuivEXT(GLuint index,GLenum pname,GLuint* params);
void glGetUniformuivEXT(GLuint program,GLint location,GLuint* params);
void glBindFragDataLocationEXT(GLuint program,GLuint color,const GLchar* name);
GLint glGetFragDataLocationEXT(GLuint program,const GLchar* name);
void glUniform1uiEXT(GLint location,GLuint v0);
void glUniform2uiEXT(GLint location,GLuint v0,GLuint v1);
void glUniform3uiEXT(GLint location,GLuint v0,GLuint v1,GLuint v2);
void glUniform4uiEXT(GLint location,GLuint v0,GLuint v1,GLuint v2,GLuint v3);
void glUniform1uivEXT(GLint location,GLsizei count,const GLuint* value);
void glUniform2uivEXT(GLint location,GLsizei count,const GLuint* value);
void glUniform3uivEXT(GLint location,GLsizei count,const GLuint* value);
void glUniform4uivEXT(GLint location,GLsizei count,const GLuint* value);

class GLEXTGpuShader4:public GLExtension
	{
	/* Elements: */
	private:
	static GL_THREAD_LOCAL(GLEXTGpuShader4*) current; // Pointer to extension object for current OpenGL context
	static const char* name; // Extension name
	PFNGLVERTEXATTRIBI1IEXTPROC glVertexAttribI1iEXTProc;
	PFNGLVERTEXATTRIBI2IEXTPROC glVertexAttribI2iEXTProc;
	PFNGLVERTEXATTRIBI3IEXTPROC glVertexAttribI3iEXTProc;
	PFNGLVERTEXATTRIBI4IEXTPROC glVertexAttribI4iEXTProc;
	PFNGLVERTEXATTRIBI1UIEXTPROC glVertexAttribI1uiEXTProc;
	PFNGLVERTEXATTRIBI2UIEXTPROC glVertexAttribI2uiEXTProc;
	PFNGLVERTEXATTRIBI3UIEXTPROC glVertexAttribI3uiEXTProc;
	PFNGLVERTEXATTRIBI4UIEXTPROC glVertexAttribI4uiEXTProc;
	PFNGLVERTEXATTRIBI1IVEXTPROC glVertexAttribI1ivEXTProc;
	PFNGLVERTEXATTRIBI2IVEXTPROC glVertexAttribI2ivEXTProc;
	PFNGLVERTEXATTRIBI3IVEXTPROC glVertexAttribI3ivEXTProc;
	PFNGLVERTEXATTRIBI4IVEXTPROC glVertexAttribI4ivEXTProc;
	PFNGLVERTEXATTRIBI1UIVEXTPROC glVertexAttribI1uivEXTProc;
	PFNGLVERTEXATTRIBI2UIVEXTPROC glVertexAttribI2uivEXTProc;
	PFNGLVERTEXATTRIBI3UIVEXTPROC glVertexAttribI3uivEXTProc;
	PFNGLVERTEXATTRIBI4UIVEXTPROC glVertexAttribI4uivEXTProc;
	PFNGLVERTEXATTRIBI4BVEXTPROC glVertexAttribI4bvEXTProc;
	PFNGLVERTEXATTRIBI4SVEXTPROC glVertexAttribI4svEXTProc;
	PFNGLVERTEXATTRIBI4UBVEXTPROC glVertexAttribI4ubvEXTProc;
	PFNGLVERTEXATTRIBI4USVEXTPROC glVertexAttribI4usvEXTProc;
	PFNGLVERTEXATTRIBIPOINTEREXTPROC glVertexAttribIPointerEXTProc;
	PFNGLGETVERTEXATTRIBIIVEXTPROC glGetVertexAttribIivEXTProc;
	PFNGLGETVERTEXATTRIBIUIVEXTPROC glGetVertexAttribIuivEXTProc;
	PFNGLGETUNIFORMUIVEXTPROC glGetUniformuivEXTProc;
	PFNGLBINDFRAGDATALOCATIONEXTPROC glBindFragDataLocationEXTProc;
	PFNGLGETFRAGDATALOCATIONEXTPROC glGetFragDataLocationEXTProc;
	PFNGLUNIFORM1UIEXTPROC glUniform1uiEXTProc;
	PFNGLUNIFORM2UIEXTPROC glUniform2uiEXTProc;
	PFNGLUNIFORM3UIEXTPROC glUniform3uiEXTProc;
	PFNGLUNIFORM4UIEXTPROC glUniform4uiEXTProc;
	PFNGLUNIFORM1UIVEXTPROC glUniform1uivEXTProc;
	PFNGLUNIFORM2UIVEXTPROC glUniform2uivEXTProc;
	PFNGLUNIFORM3UIVEXTPROC glUniform3uivEXTProc;
	PFNGLUNIFORM4UIVEXTPROC glUniform4uivEXTProc;
	
	/* Constructors and destructors: */
	private:
	GLEXTGpuShader4(void);
	public:
	virtual ~GLEXTGpuShader4(void);
	
	/* Methods: */
	public:
	virtual const char* getExtensionName(void) const;
	virtual void activate(void);
	virtual void deactivate(void);
	static bool isSupported(void); // Returns true if the extension is supported in the current OpenGL context
	static void initExtension(void); // Initializes the extension in the current OpenGL context
	
	/* Extension entry points: */
	inline friend void glVertexAttribI1iEXT(GLuint index,GLint x)
		{
		GLEXTGpuShader4::current->glVertexAttribI1iEXTProc(index,x);
		}
	inline friend void glVertexAttribI2iEXT(GLuint index,GLint x,GLint y)
		{
		GLEXTGpuShader4::current->glVertexAttribI2iEXTProc(index,x,y);
		}
	inline friend void glVertexAttribI3iEXT(GLuint index,GLint x,GLint y,GLint z)
		{
		GLEXTGpuShader4::current->glVertexAttribI3iEXTProc(index,x,y,z);
		}
	inline friend void glVertexAttribI4iEXT(GLuint index,GLint x,GLint y,GLint z,GLint w)
		{
		GLEXTGpuShader4::current->glVertexAttribI4iEXTProc(index,x,y,z,w);
		}
	inline friend void glVertexAttribI1uiEXT(GLuint index,GLuint x)
		{
		GLEXTGpuShader4::current->glVertexAttribI1uiEXTProc(index,x);
		}
	inline friend void glVertexAttribI2uiEXT(GLuint index,GLuint x,GLuint y)
		{
		GLEXTGpuShader4::current->glVertexAttribI2uiEXTProc(index,x,y);
		}
	inline friend void glVertexAttribI3uiEXT(GLuint index,GLuint x,GLuint y,GLuint z)
		{
		GLEXTGpuShader4::current->glVertexAttribI3uiEXTProc(index,x,y,z);
		}
	inline friend void glVertexAttribI4uiEXT(GLuint index,GLuint x,GLuint y,GLuint z,GLuint w)
		{
		GLEXTGpuShader4::current->glVertexAttribI4uiEXTProc(index,x,y,z,w);
		}
	inline friend void glVertexAttribI1ivEXT(GLuint index,const GLint* v)
		{
		GLEXTGpuShader4::current->glVertexAttribI1ivEXTProc(index,v);
		}
	inline friend void glVertexAttribI2ivEXT(GLuint index,const GLint* v)
		{
		GLEXTGpuShader4::current->glVertexAttribI2ivEXTProc(index,v);
		}
	inline friend void glVertexAttribI3ivEXT(GLuint index,const GLint* v)
		{
		GLEXTGpuShader4::current->glVertexAttribI3ivEXTProc(index,v);
		}
	inline friend void glVertexAttribI4ivEXT(GLuint index,const GLint* v)
		{
		GLEXTGpuShader4::current->glVertexAttribI4ivEXTProc(index,v);
		}
	inline friend void glVertexAttribI1uivEXT(GLuint index,const GLuint* v)
		{
		GLEXTGpuShader4::current->glVertexAttribI1uivEXTProc(index,v);
		}
	inline friend void glVertexAttribI2uivEXT(GLuint index,const GLuint* v)
		{
		GLEXTGpuShader4::current->glVertexAttribI2uivEXTProc(index,v);
		}
	inline friend void glVertexAttribI3uivEXT(GLuint index,const GLuint* v)
		{
		GLEXTGpuShader4::current->glVertexAttribI3uivEXTProc(index,v);
		}
	inline friend void glVertexAttribI4uivEXT(GLuint index,const GLuint* v)
		{
		GLEXTGpuShader4::current->glVertexAttribI4uivEXTProc(index,v);
		}
	inline friend void glVertexAttribI4bvEXT(GLuint index,const GLbyte* v)
		{
		GLEXTGpuShader4::current->glVertexAttribI4bvEXTProc(index,v);
		}
	inline friend void glVertexAttribI4svEXT(GLuint index,const GLshort* v)
		{
		GLEXTGpuShader4::current->glVertexAttribI4svEXTProc(index,v);
		}
	inline friend void glVertexAttribI4ubvEXT(GLuint index,const GLubyte* v)
		{
		GLEXTGpuShader4::current->glVertexAttribI4ubvEXTProc(index,v);
		}
	inline friend void glVertexAttribI4usvEXT(GLuint index,const GLushort* v)
		{
		GLEXTGpuShader4::current->glVertexAttribI4usvEXTProc(index,v);
		}
	inline friend void glVertexAttribIPointerEXT(GLuint index,GLint size,GLenum type,GLsizei stride,const void* pointer)
		{
		GLEXTGpuShader4::current->glVertexAttribIPointerEXTProc(index,size,type,stride,pointer);
		}
	inline friend void glGetVertexAttribIivEXT(GLuint index,GLenum pname,GLint* params)
		{
		GLEXTGpuShader4::current->glGetVertexAttribIivEXTProc(index,pname,params);
		}
	inline friend void glGetVertexAttribIuivEXT(GLuint index,GLenum pname,GLuint* params)
		{
		GLEXTGpuShader4::current->glGetVertexAttribIuivEXTProc(index,pname,params);
		}
	inline friend void glGetUniformuivEXT(GLuint program,GLint location,GLuint* params)
		{
		GLEXTGpuShader4::current->glGetUniformuivEXTProc(program,location,params);
		}
	inline friend void glBindFragDataLocationEXT(GLuint program,GLuint color,const GLchar* name)
		{
		GLEXTGpuShader4::current->glBindFragDataLocationEXTProc(program,color,name);
		}
	inline friend GLint glGetFragDataLocationEXT(GLuint program,const GLchar* name)
		{
		return GLEXTGpuShader4::current->glGetFragDataLocationEXTProc(program,name);
		}
	inline friend void glUniform1uiEXT(GLint location,GLuint v0)
		{
		GLEXTGpuShader4::current->glUniform1uiEXTProc(location,v0);
		}
	inline friend void glUniform2uiEXT(GLint location,GLuint v0,GLuint v1)
		{
		GLEXTGpuShader4::current->glUniform2uiEXTProc(location,v0,v1);
		}
	inline friend void glUniform3uiEXT(GLint location,GLuint v0,GLuint v1,GLuint v2)
		{
		GLEXTGpuShader4::current->glUniform3uiEXTProc(location,v0,v1,v2);
		}
	inline friend void glUniform4uiEXT(GLint location,GLuint v0,GLuint v1,GLuint v2,GLuint v3)
		{
		GLEXTGpuShader4::current->glUniform4uiEXTProc(location,v0,v1,v2,v3);
		}
	inline friend void glUniform1uivEXT(GLint location,GLsizei count,const GLuint* value)
		{
		GLEXTGpuShader4::current->glUniform1uivEXTProc(location,count,value);
		}
	inline friend void glUniform2uivEXT(GLint location,GLsizei count,const GLuint* value)
		{
		GLEXTGpuShader4::current->glUniform2uivEXTProc(location,count,value);
		}
	inline friend void glUniform3uivEXT(GLint location,GLsizei count,const GLuint* value)
		{
		GLEXTGpuShader4::current->glUniform3uivEXTProc(location,count,value);
		}
	inline friend void glUniform4uivEXT(GLint location,GLsizei count,const GLuint* value)
		{
		GLEXTGpuShader4::current->glUniform4uivEXTProc(location,count,value);
		}
	};

/*******************************
Extension-specific entry points:
*******************************/

/***************************************************************
Overloaded versions of component-based glVertexAttribIEXT calls:
***************************************************************/

inline void glVertexAttribIEXT(GLuint index,GLint x)
	{
	glVertexAttribI1iEXT(index,x);
	}

inline void glVertexAttribIEXT(GLuint index,GLuint x)
	{
	glVertexAttribI1uiEXT(index,x);
	}

inline void glVertexAttribIEXT(GLuint index,GLint x,GLint y)
	{
	glVertexAttribI2iEXT(index,x,y);
	}

inline void glVertexAttribIEXT(GLuint index,GLuint x,GLuint y)
	{
	glVertexAttribI2uiEXT(index,x,y);
	}

inline void glVertexAttribIEXT(GLuint index,GLint x,GLint y,GLint z)
	{
	glVertexAttribI3iEXT(index,x,y,z);
	}

inline void glVertexAttribIEXT(GLuint index,GLuint x,GLuint y,GLuint z)
	{
	glVertexAttribI3uiEXT(index,x,y,z);
	}

inline void glVertexAttribIEXT(GLuint index,GLint x,GLint y,GLint z,GLint w)
	{
	glVertexAttribI4iEXT(index,x,y,z,w);
	}

inline void glVertexAttribIEXT(GLuint index,GLuint x,GLuint y,GLuint z,GLuint w)
	{
	glVertexAttribI4uiEXT(index,x,y,z,w);
	}

/************************************************************
Dummy generic version of array-based glVertexAttribIEXT call:
************************************************************/

template <GLsizei numComponents,class ScalarParam>
void glVertexAttribIEXT(GLuint index,const ScalarParam components[numComponents]);

/***********************************************************
Specialized versions of array-based glVertexAttribIEXT calls:
***********************************************************/

template <>
inline void glVertexAttribIEXT<1,GLint>(GLuint index,const GLint components[1])
	{
	glVertexAttribI1ivEXT(index,components);
	}

template <>
inline void glVertexAttribIEXT<1,GLuint>(GLuint index,const GLuint components[1])
	{
	glVertexAttribI1uivEXT(index,components);
	}

template <>
inline void glVertexAttribIEXT<2,GLint>(GLuint index,const GLint components[2])
	{
	glVertexAttribI2ivEXT(index,components);
	}

template <>
inline void glVertexAttribIEXT<2,GLuint>(GLuint index,const GLuint components[2])
	{
	glVertexAttribI2uivEXT(index,components);
	}

template <>
inline void glVertexAttribIEXT<3,GLint>(GLuint index,const GLint components[3])
	{
	glVertexAttribI3ivEXT(index,components);
	}

template <>
inline void glVertexAttribIEXT<3,GLuint>(GLuint index,const GLuint components[3])
	{
	glVertexAttribI3uivEXT(index,components);
	}

template <>
inline void glVertexAttribIEXT<4,GLbyte>(GLuint index,const GLbyte components[4])
	{
	glVertexAttribI4bvEXT(index,components);
	}

template <>
inline void glVertexAttribIEXT<4,GLubyte>(GLuint index,const GLubyte components[4])
	{
	glVertexAttribI4ubvEXT(index,components);
	}

template <>
inline void glVertexAttribIEXT<4,GLshort>(GLuint index,const GLshort components[4])
	{
	glVertexAttribI4svEXT(index,components);
	}

template <>
inline void glVertexAttribIEXT<4,GLushort>(GLuint index,const GLushort components[4])
	{
	glVertexAttribI4usvEXT(index,components);
	}

template <>
inline void glVertexAttribIEXT<4,GLint>(GLuint index,const GLint components[4])
	{
	glVertexAttribI4ivEXT(index,components);
	}

template <>
inline void glVertexAttribIEXT<4,GLuint>(GLuint index,const GLuint components[4])
	{
	glVertexAttribI4uivEXT(index,components);
	}

/*****************************************************
Overloaded versions of glVertexAttribIPointerEXT calls:
*****************************************************/

inline void glVertexAttribIPointerEXT(GLuint index,GLint size,GLsizei stride,const GLbyte* pointer)
	{
	glVertexAttribIPointerEXT(index,size,GL_BYTE,stride,pointer);
	}

inline void glVertexAttribIPointerEXT(GLuint index,GLint size,GLsizei stride,const GLubyte* pointer)
	{
	glVertexAttribIPointerEXT(index,size,GL_UNSIGNED_BYTE,stride,pointer);
	}

inline void glVertexAttribIPointerEXT(GLuint index,GLint size,GLsizei stride,const GLshort* pointer)
	{
	glVertexAttribIPointerEXT(index,size,GL_SHORT,stride,pointer);
	}

inline void glVertexAttribIPointerEXT(GLuint index,GLint size,GLsizei stride,const GLushort* pointer)
	{
	glVertexAttribIPointerEXT(index,size,GL_UNSIGNED_SHORT,stride,pointer);
	}

inline void glVertexAttribIPointerEXT(GLuint index,GLint size,GLsizei stride,const GLint* pointer)
	{
	glVertexAttribIPointerEXT(index,size,GL_INT,stride,pointer);
	}

inline void glVertexAttribIPointerEXT(GLuint index,GLint size,GLsizei stride,const GLuint* pointer)
	{
	glVertexAttribIPointerEXT(index,size,GL_UNSIGNED_INT,stride,pointer);
	}

/**************************************************
Overloaded versions of glGetVertexAttribIEXT calls:
**************************************************/

inline void glGetVertexAttribIEXT(GLuint index,GLenum pname,GLint* params)
	{
	glGetVertexAttribIivEXT(index,pname,params);
	}

inline void glGetVertexAttribIEXT(GLuint index,GLenum pname,GLuint* params)
	{
	glGetVertexAttribIuivEXT(index,pname,params);
	}

/*********************************************************
Overloaded versions of component-based glUniformEXT calls:
*********************************************************/

inline void glUniformEXT(GLint location,GLuint v0)
	{
	glUniform1uiEXT(location,v0);
	}

inline void glUniformEXT(GLint location,GLuint v0,GLuint v1)
	{
	glUniform2uiEXT(location,v0,v1);
	}

inline void glUniformEXT(GLint location,GLuint v0,GLuint v1,GLuint v2)
	{
	glUniform3uiEXT(location,v0,v1,v2);
	}

inline void glUniformEXT(GLint location,GLuint v0,GLuint v1,GLuint v2,GLuint v3)
	{
	glUniform4uiEXT(location,v0,v1,v2,v3);
	}

/******************************************************
Dummy generic version of array-based glUniformEXT call:
******************************************************/

template <GLsizei numComponents,class ScalarParam>
void glUniformEXT(GLint location,GLsizei count,const ScalarParam components[numComponents]);

/******************************************************
Specialized versions of array-based glUniformEXT calls:
******************************************************/

template <>
inline void glUniformEXT<1,GLuint>(GLint location,GLsizei count,const GLuint components[1])
	{
	glUniform1uivEXT(location,count,components);
	}

template <>
inline void glUniformEXT<2,GLuint>(GLint location,GLsizei count,const GLuint components[2])
	{
	glUniform2uivEXT(location,count,components);
	}

template <>
inline void glUniformEXT<3,GLuint>(GLint location,GLsizei count,const GLuint components[3])
	{
	glUniform3uivEXT(location,count,components);
	}

template <>
inline void glUniformEXT<4,GLuint>(GLint location,GLsizei count,const GLuint components[4])
	{
	glUniform4uivEXT(location,count,components);
	}

#endif
