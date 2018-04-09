/***********************************************************************
GLVertexBuffer - Class to represent OpenGL vertex buffer objects
containg typed vertices.
Copyright (c) 2018 Oliver Kreylos

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

#ifndef GLVERTEXBUFFER_INCLUDED
#define GLVERTEXBUFFER_INCLUDED

#include <GL/gl.h>
#include <GL/GLObject.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>

/* Forward declarations: */
class GLContextData;

template <class VertexParam>
class GLVertexBuffer:public GLObject
	{
	/* Embedded classes: */
	public:
	typedef VertexParam Vertex; // Type for vertices stored in the vertex buffer
	
	struct DataItem:public GLObject::DataItem
		{
		friend class GLVertexBuffer;
		
		/* Elements: */
		private:
		GLuint vertexBufferObjectId; // ID of the buffer object storing the vertex array in GPU memory
		unsigned int parameterVersion; // Version number of the buffer's parameters (size and usage pattern)
		unsigned int version; // Version number of the vertex data in GPU memory
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	private:
	size_t numVertices; // Number of vertices in the buffer
	const Vertex* sourceVertices; // Pointer to source vertex data in CPU memory
	GLenum bufferUsage; // Usage pattern for the vertex data buffer
	unsigned int parameterVersion; // Version number of buffer parameters (size and usage pattern)
	unsigned int version; // Version number of the vertex data in CPU memory
	
	/* Constructors and destructors: */
	public:
	GLVertexBuffer(void) // Creates zero-sized buffer with default parameters
		:numVertices(0),sourceVertices(0),
		 bufferUsage(GL_DYNAMIC_DRAW_ARB),
		 parameterVersion(0),version(0)
		{
		}
	GLVertexBuffer(size_t sNumVertices,const Vertex* sSourceVertices,GLenum sBufferUsage =GL_DYNAMIC_DRAW_ARB) // Creates a buffer for the given source vertex array and usage pattern
		:numVertices(sNumVertices),sourceVertices(sSourceVertices),
		 bufferUsage(sBufferUsage),
		 parameterVersion(0),version(0)
		{
		}
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New methods: */
	
	/* Methods to be called from application code: */
	size_t getNumVertices(void) const // Returns the number of vertices in the buffer
		{
		return numVertices;
		}
	GLenum getBufferUsage(void) const // Returns the buffer usage pattern
		{
		return bufferUsage;
		}
	void invalidate(void); // Invalidates the buffer when the source vertex array is changed externally
	void setSource(size_t newNumVertices,const Vertex* newSourceVertices); // Changes the source vertex data; causes a re-upload of buffer contents on the next bind()
	void setBufferUsage(GLenum newBufferUsage); // Changes the buffer usage pattern; causes a re-upload of buffer contents on next bind()
	
	/* Methods to be called from inside an active OpenGL context: */
	void bind(GLContextData& contextData) const; // Binds the vertex buffer to the current OpenGL context for subsequent drawing operations
	static void unbind(void); // Unbinds any active vertex buffer from the current OpenGL context
	DataItem* getDataItem(GLContextData& contextData) const; // Returns the buffer's per-context data item for further calls
	bool needsUpdate(DataItem* dataItem) const // Returns true if the buffer's parameters or content are outdated
		{
		return dataItem->parameterVersion!=parameterVersion||dataItem->version!=version;
		}
	Vertex* startUpdate(DataItem* dataItem) const; // Returns a pointer to upload vertex data into the buffer
	void finishUpdate(DataItem* dataItem) const; // Finishes updating the buffer and prepares it for subsequent drawing operations
	void draw(GLenum mode,GLContextData& contextData) const; // Draws the buffer's vertices using the given primitive mode
	void draw(GLenum mode,GLint first,GLsizei count,GLContextData& contextData) const; // Draws the given subset of the buffer's vertices using the given primitive mode
	};

#ifndef GLVERTEXBUFFER_IMPLEMENTATION
#include <GL/GLVertexBuffer.icpp>
#endif

#endif
