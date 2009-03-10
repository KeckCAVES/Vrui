/***********************************************************************
GLARBGeometryShader4 - OpenGL extension class for the
GL_ARB_geometry_shader4 extension.
Copyright (c) 2007-2009 Tony Bernardin, Oliver Kreylos

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

#include <GL/Extensions/GLARBGeometryShader4.h>

/*********************************************
Static elements of class GLARBGeometryShader4:
*********************************************/

GL_THREAD_LOCAL(GLARBGeometryShader4*) GLARBGeometryShader4::current=0;

/*************************************
Methods of class GLARBGeometryShader4:
*************************************/

GLARBGeometryShader4::GLARBGeometryShader4(void)
	:glProgramParameteriARBProc(GLExtensionManager::getFunction<PFNGLPROGRAMPARAMETERIARBPROC>("glProgramParameteriARB")),
	 glFramebufferTextureARBProc(GLExtensionManager::getFunction<PFNGLFRAMEBUFFERTEXTUREARBPROC>("glFramebufferTextureARB")),
	 glFramebufferTextureLayerARBProc(GLExtensionManager::getFunction<PFNGLFRAMEBUFFERTEXTURELAYERARBPROC>("glFramebufferTextureLayerARB")),
	 glFramebufferTextureFaceARBProc(GLExtensionManager::getFunction<PFNGLFRAMEBUFFERTEXTUREFACEARBPROC>("glFramebufferTextureFaceARB"))
	{
	}

GLARBGeometryShader4::~GLARBGeometryShader4(void)
	{
	}

const char* GLARBGeometryShader4::getExtensionName(void) const
	{
	return "GL_ARB_geometry_shader4";
	}

void GLARBGeometryShader4::activate(void)
	{
	current=this;
	}

void GLARBGeometryShader4::deactivate(void)
	{
	current=0;
	}

bool GLARBGeometryShader4::isSupported(void)
	{
	/* Ask the current extension manager whether the extension is supported in the current OpenGL context: */
	return GLExtensionManager::isExtensionSupported("GL_ARB_geometry_shader4");
	}

void GLARBGeometryShader4::initExtension(void)
	{
	/* Check if the extension is already initialized: */
	if(!GLExtensionManager::isExtensionRegistered("GL_ARB_geometry_shader4"))
		{
		/* Create a new extension object: */
		GLARBGeometryShader4* newExtension=new GLARBGeometryShader4;
		
		/* Register the extension with the current extension manager: */
		GLExtensionManager::registerExtension(newExtension);
		}
	}

GLhandleARB glCompileARBGeometryShader4FromString(const char* shaderSource)
	{
	/* Create a new geometry shader: */
	GLhandleARB geometryShaderObject=glCreateShaderObjectARB(GL_GEOMETRY_SHADER_ARB);
	
	try
		{
		/* Load and compile the shader source: */
		glCompileShaderFromString(geometryShaderObject,shaderSource);
		}
	catch(std::runtime_error err)
		{
		/* Clean up and re-throw the exception: */
		glDeleteObjectARB(geometryShaderObject);
		
		throw;
		}
	
	return geometryShaderObject;
	}

GLhandleARB glCompileARBGeometryShader4FromFile(const char* shaderSourceFileName)
	{
	/* Create a new vertex shader: */
	GLhandleARB geometryShaderObject=glCreateShaderObjectARB(GL_GEOMETRY_SHADER_ARB);
	
	try
		{
		/* Load and compile the shader source: */
		glCompileShaderFromFile(geometryShaderObject,shaderSourceFileName);
		}
	catch(std::runtime_error err)
		{
		/* Clean up and re-throw the exception: */
		glDeleteObjectARB(geometryShaderObject);
		
		throw;
		}
	
	return geometryShaderObject;
	}
