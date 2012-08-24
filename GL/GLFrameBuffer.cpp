/***********************************************************************
GLFrameBuffer - Simple class to encapsulate the state of and operations
on OpenGL frame buffer objects.
Copyright (c) 2012 Oliver Kreylos

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

#include <GL/GLFrameBuffer.h>

#include <Misc/ThrowStdErr.h>
#include <GL/gl.h>
#include <GL/Extensions/GLEXTFramebufferObject.h>
#include <GL/Extensions/GLARBDepthTexture.h>

/******************************
Methods of class GLFrameBuffer:
******************************/

GLFrameBuffer::GLFrameBuffer(GLsizei width,GLsizei height)
	:frameBufferId(0),
	 haveDepthTextures(GLARBDepthTexture::isSupported()),
	 depthIsTexture(false),depthBufferId(0),
	 numColorAttachments(0),colorIsTextures(0),colorBufferIds(0)
	{
	/* Check for the required OpenGL extensions: */
	if(!GLEXTFramebufferObject::isSupported())
		Misc::throwStdErr("GLFrameBuffer::GLFrameBuffer: GL_EXT_framebuffer_object not supported");
	
	/* Initialize the required extensions: */
	GLEXTFramebufferObject::initExtension();
	if(haveDepthTextures)
		GLARBDepthTexture::initExtension();
	
	/* Create the frame buffer object: */
	glGenFramebuffersEXT(1,&frameBufferId);
	
	/* Query the number of supported color attachments: */
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS_EXT,&numColorAttachments);
	
	/* Initialize the color buffer attachment arrays: */
	colorIsTextures=new bool[numColorAttachments];
	colorBufferIds=new GLuint[numColorAttachments];
	for(GLsizei i=0;i<numColorAttachments;++i)
		{
		colorIsTextures[i]=false;
		colorBufferIds[i]=0;
		}
	}

GLFrameBuffer::~GLFrameBuffer(void)
	{
	/* Destroy the color buffer attachment arrays: */
	delete[] colorIsTextures;
	delete[] colorBufferIds;
	
	/* Destroy the frame buffer object: */
	glDestroyFramebuffersEXT(1,&frameBufferId);
	}

bool GLFrameBuffer::isSupported(void)
	{
	return GLEXTFramebufferObject::isSupported();
	}

void GLFrameBuffer::attachDepthBuffer(void)
	{
	/* Check if there already is a depth buffer: */
	if(depthBufferId!=0)
		{
		/* Detach and destroy the depth buffer: */
		if(depthIsTexture)
			glDeleteTextures(1,&depthBufferId);
		else
			glDeleteRenderbuffersEXT(1,&depthBufferId);
		}
	
	/* Create a new render buffer: */
	depthIsTexture=false;
	glGenRenderBuffersEXT(1,&depthBufferId);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,depthBufferId);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH_COMPONENT,size[0],size[1]);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
	}

void GLFrameBuffer::attachDepthTexture(void)
	{
	/* Check for the required OpenGL extensions: */
	if(!haveDepthTextures)
		Misc::throwStdErr("GLFrameBuffer::attachDepthTexture: GL_ARB_depth_texture not supported");
	
	/* Check if there already is a depth buffer: */
	if(depthBufferId!=0)
		{
		/* Detach and destroy the depth buffer: */
		if(depthIsTexture)
			glDeleteTextures(1,&depthBufferId);
		else
			glDeleteRenderbuffersEXT(1,&depthBufferId);
		}
	
	/* Create a new depth texture object: */
	depthIsTexture=true;
	glGenTextures(1,&depthBufferId);
	glBindTexture(GL_TEXTURE_2D,depthBufferId);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_DEPTH_TEXTURE_MODE_ARB,GL_INTENSITY);
	glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24_ARB,size[0],size[1],0,GL_DEPTH_COMPONENT,GL_UNSIGNED_BYTE,0);
	glBindTexture(GL_TEXTURE_2D,0);
	}

void GLFrameBuffer::attachColorBuffer(GLint colorBufferIndex)
	{
	/* Check if there already is a color buffer in the given slot: */
	if(colorBufferIds[colorBufferIndex]!=0)
		{
		/* Detach and destroy the depth buffer: */
		if(colorIsTextures[colorBufferIndex])
			glDeleteTextures(1,&colorBufferIds[colorBufferIndex]);
		else
			glDeleteRenderbuffersEXT(1,&colorBufferIds[colorBufferIndex]);
		}
	
	/* Create a new render buffer: */
	colorIsTextures[colorBufferIndex]=false;
	glGenRenderBuffersEXT(1,&colorBufferIds[colorBufferIndex]);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,colorBufferIds[colorBufferIndex]);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH_COMPONENT,size[0],size[1]);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
	}

void GLFrameBuffer::finish(void)
	{
	/* Bind the frame buffer: */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,frameBufferId);
	
	/* Attach all requested buffers: */
	if(depthBufferId!=0)
		{
		/* Attach the depth buffer to the frame buffer: */
		if(depthIsTexture)
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,depthBufferId,0);
		else
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,depthBufferId);
		}
	
	/* Check the frame buffer for consistency: */
	GLenum status=glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	
	/* Unbind the frame buffer: */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	
	if(status!=GL_FRAMEBUFFER_COMPLETE_EXT)
		{
		switch(status)
			{
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
				Misc::throwStdErr("GLFrameBuffer::finish: attachment");
			
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
				Misc::throwStdErr("GLFrameBuffer::finish: missing attachment");
			
			case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT:
				Misc::throwStdErr("GLFrameBuffer::finish: duplicate attachment");
			
			case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
				Misc::throwStdErr("GLFrameBuffer::finish: dimensions");
			
			case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
				Misc::throwStdErr("GLFrameBuffer::finish: formats");
			
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
				Misc::throwStdErr("GLFrameBuffer::finish: draw buffer");
			
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
				Misc::throwStdErr("GLFrameBuffer::finish: read buffer");
			
			case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
				Misc::throwStdErr("GLFrameBuffer::finish: unsupported");
			}
		}
	}

void GLFrameBuffer::bind(void)
	{
	/* Bind the frame buffer object: */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,frameBufferId);
	}

void GLFrameBuffer::unbind(void)
	{
	/* Unbind all frame buffer objects: */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	}
