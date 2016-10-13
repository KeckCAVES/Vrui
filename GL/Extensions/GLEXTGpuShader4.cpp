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

#include <GL/Extensions/GLEXTGpuShader4.h>

#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>

/****************************************
Static elements of class GLEXTGpuShader4:
****************************************/

GL_THREAD_LOCAL(GLEXTGpuShader4*) GLEXTGpuShader4::current=0;
const char* GLEXTGpuShader4::name="GL_EXT_gpu_shader4";

/********************************
Methods of class GLEXTGpuShader4:
********************************/

GLEXTGpuShader4::GLEXTGpuShader4(void)
	:glVertexAttribI1iEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI1IEXTPROC>("glVertexAttribI1iEXT")),
	 glVertexAttribI2iEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI2IEXTPROC>("glVertexAttribI2iEXT")),
	 glVertexAttribI3iEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI3IEXTPROC>("glVertexAttribI3iEXT")),
	 glVertexAttribI4iEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI4IEXTPROC>("glVertexAttribI4iEXT")),
	 glVertexAttribI1uiEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI1UIEXTPROC>("glVertexAttribI1uiEXT")),
	 glVertexAttribI2uiEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI2UIEXTPROC>("glVertexAttribI2uiEXT")),
	 glVertexAttribI3uiEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI3UIEXTPROC>("glVertexAttribI3uiEXT")),
	 glVertexAttribI4uiEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI4UIEXTPROC>("glVertexAttribI4uiEXT")),
	 glVertexAttribI1ivEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI1IVEXTPROC>("glVertexAttribI1ivEXT")),
	 glVertexAttribI2ivEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI2IVEXTPROC>("glVertexAttribI2ivEXT")),
	 glVertexAttribI3ivEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI3IVEXTPROC>("glVertexAttribI3ivEXT")),
	 glVertexAttribI4ivEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI4IVEXTPROC>("glVertexAttribI4ivEXT")),
	 glVertexAttribI1uivEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI1UIVEXTPROC>("glVertexAttribI1uivEXT")),
	 glVertexAttribI2uivEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI2UIVEXTPROC>("glVertexAttribI2uivEXT")),
	 glVertexAttribI3uivEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI3UIVEXTPROC>("glVertexAttribI3uivEXT")),
	 glVertexAttribI4uivEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI4UIVEXTPROC>("glVertexAttribI4uivEXT")),
	 glVertexAttribI4bvEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI4BVEXTPROC>("glVertexAttribI4bvEXT")),
	 glVertexAttribI4svEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI4SVEXTPROC>("glVertexAttribI4svEXT")),
	 glVertexAttribI4ubvEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI4UBVEXTPROC>("glVertexAttribI4ubvEXT")),
	 glVertexAttribI4usvEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBI4USVEXTPROC>("glVertexAttribI4usvEXT")),
	 glVertexAttribIPointerEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXATTRIBIPOINTEREXTPROC>("glVertexAttribIPointerEXT")),
	 glGetVertexAttribIivEXTProc(GLExtensionManager::getFunction<PFNGLGETVERTEXATTRIBIIVEXTPROC>("glGetVertexAttribIivEXT")),
	 glGetVertexAttribIuivEXTProc(GLExtensionManager::getFunction<PFNGLGETVERTEXATTRIBIUIVEXTPROC>("glGetVertexAttribIuivEXT")),
	 glGetUniformuivEXTProc(GLExtensionManager::getFunction<PFNGLGETUNIFORMUIVEXTPROC>("glGetUniformuivEXT")),
	 glBindFragDataLocationEXTProc(GLExtensionManager::getFunction<PFNGLBINDFRAGDATALOCATIONEXTPROC>("glBindFragDataLocationEXT")),
	 glGetFragDataLocationEXTProc(GLExtensionManager::getFunction<PFNGLGETFRAGDATALOCATIONEXTPROC>("glGetFragDataLocationEXT")),
	 glUniform1uiEXTProc(GLExtensionManager::getFunction<PFNGLUNIFORM1UIEXTPROC>("glUniform1uiEXT")),
	 glUniform2uiEXTProc(GLExtensionManager::getFunction<PFNGLUNIFORM2UIEXTPROC>("glUniform2uiEXT")),
	 glUniform3uiEXTProc(GLExtensionManager::getFunction<PFNGLUNIFORM3UIEXTPROC>("glUniform3uiEXT")),
	 glUniform4uiEXTProc(GLExtensionManager::getFunction<PFNGLUNIFORM4UIEXTPROC>("glUniform4uiEXT")),
	 glUniform1uivEXTProc(GLExtensionManager::getFunction<PFNGLUNIFORM1UIVEXTPROC>("glUniform1uivEXT")),
	 glUniform2uivEXTProc(GLExtensionManager::getFunction<PFNGLUNIFORM2UIVEXTPROC>("glUniform2uivEXT")),
	 glUniform3uivEXTProc(GLExtensionManager::getFunction<PFNGLUNIFORM3UIVEXTPROC>("glUniform3uivEXT")),
	 glUniform4uivEXTProc(GLExtensionManager::getFunction<PFNGLUNIFORM4UIVEXTPROC>("glUniform4uivEXT"))
	{
	}

GLEXTGpuShader4::~GLEXTGpuShader4(void)
	{
	}

const char* GLEXTGpuShader4::getExtensionName(void) const
	{
	return name;
	}

void GLEXTGpuShader4::activate(void)
	{
	current=this;
	}

void GLEXTGpuShader4::deactivate(void)
	{
	current=0;
	}

bool GLEXTGpuShader4::isSupported(void)
	{
	/* Ask the current extension manager whether the extension is supported in the current OpenGL context: */
	return GLExtensionManager::isExtensionSupported(name);
	}

void GLEXTGpuShader4::initExtension(void)
	{
	/* Check if the extension is already initialized: */
	if(!GLExtensionManager::isExtensionRegistered(name))
		{
		/* Create a new extension object: */
		GLEXTGpuShader4* newExtension=new GLEXTGpuShader4;
		
		/* Register the extension with the current extension manager: */
		GLExtensionManager::registerExtension(newExtension);
		}
	}
