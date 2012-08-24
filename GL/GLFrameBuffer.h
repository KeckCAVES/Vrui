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

#ifndef GLFRAMEBUFFER_INCLUDED
#define GLFRAMEBUFFER_INCLUDED

#include <GL/gl.h>

class GLFrameBuffer
	{
	/* Elements: */
	private:
	GLsizei size[2]; // The width and height of the frame buffer in pixels
	GLuint frameBufferId; // The ID of the frame buffer object
	bool haveDepthTextures; // Flag whether the local OpenGL supports depth textures
	bool depthIsTexture; // Flag if the depth buffer is a texture object
	GLuint depthBufferId; // The ID of the render buffer or texture object attached as depth buffer; 0 if no depth buffer is used
	GLsizei numColorAttachments; // Number of color buffers attachable to a single frame buffer
	bool* colorIsTextures; // Array of flags if each of the color buffers is a texture object
	GLuint colorBufferIds; // Array of IDs of the render buffer of texture objects attached as color buffers; 0 if no color buffer is used in a slot
	
	/* Constructors and destructors: */
	public:
	GLFrameBuffer(GLsizei width,GLsizei height); // Creates a frame buffer of the given size with no attachments
	private:
	GLFrameBuffer(const GLFrameBuffer& source); // Prohibit copy constructor
	GLFrameBuffer& operator=(const GLFrameBuffer& source); // Prohibit assignment operator
	public:
	~GLFrameBuffer(void); // Destroys the frame buffer and releases all allocated resources
	
	/* Methods: */
	static bool isSupported(void); // Returns true if the current OpenGL context supports frame buffer objects
	void attachDepthBuffer(void); // Attaches a render buffer as the frame buffer's depth buffer
	bool canAttachDepthTexture(void) const // Returns true if the frame buffer supports use of textures as depth buffers
		{
		return haveDepthTextures;
		}
	void attachDepthTexture(void); // Attaches a texture object as the frame buffer's depth buffer
	void bindDepthTexture(GLenum textureTarget) const // Binds the texture object attached as depth buffer to the given texture target; fails if depth buffer is not a texture
		{
		glBindTexture(textureTarget,depthBufferId);
		}
	GLsizei getNumColorBuffers(void) const // Returns the maximum number of color buffer attachments supported by this frame buffer
		{
		return numColorAttachments;
		}
	void attachColorBuffer(GLint colorBufferIndex); // Attaches a render buffer as the frame buffer's color buffer of the given index
	void attachColorTexture(GLint colorBufferIndex); // Attaches a texture object as the frame buffer's color buffer of the given index
	void bindColorTexture(GLenum textureTarget,GLint colorBufferIndex) const // Binds the texture object attached as color buffer of the given index to the given texture target; fails if selected color buffer is not a texture
		{
		glBindTexture(textureTarget,colorBufferIds[colorBufferIndex]);
		}
	void finish(void); // Finishes the frame buffer; throws exception if the frame buffer is inconsistent
	void bind(void) const; // Binds this frame buffer object in the current OpenGL context
	static void unbind(void); // Unbinds the currently bound frame buffer object in the current OpenGL context
	};

#endif
