/***********************************************************************
GLTriangleStripSet - Class to allow fast incremental creation of
triangle strip sets of various types
Copyright (c) 2001-2006 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef GLTRIANGLESTRIPSET_INCLUDED
#define GLTRIANGLESTRIPSET_INCLUDED

#include <string.h>
#include <vector>
#include <GL/gl.h>

template <class VertexType,int vertexChunkSize =1024>
class GLTriangleStripSet
	{
	/* Embedded classes: */
	public:
	typedef VertexType Vertex;
	
	private:
	struct VertexChunk // Chunk of vertex data
		{
		/* Elements: */
		public:
		VertexChunk* succ; // Pointer to next chunk
		int numVertices; // Number of vertices in chunk
		VertexType vertices[vertexChunkSize]; // Array of vertices in chunk
		
		/* Constructors and destructors: */
		VertexChunk(void)
			:succ(0),numVertices(0)
			{
			};
		};
	
	struct TriangleStrip // Triangle strip identifier
		{
		/* Elements: */
		public:
		GLint firstVertex; // Index of first vertex in strip
		GLsizei numVertices; // Number of vertices in strip
		
		/* Constructors and destructors: */
		TriangleStrip(GLint sFirstVertex)
			:firstVertex(sFirstVertex),numVertices(0)
			{
			};
		};
	
	typedef std::vector<TriangleStrip> TriangleStripSet;
	
	/* Elements: */
	private:
	int numVertices; // Total number of vertices
	VertexChunk* chunks; // Pointer to first vertex chunk
	VertexChunk* lastChunk; // Pointer to last vertex chunk
	TriangleStripSet triangleStrips; // Vector of triangle strips
	VertexType* vertexArray; // Vertex array to render triangle strips
	
	/* Constructors and destructors: */
	public:
	GLTriangleStripSet(void) // Creates an empty triangle strip set
		:numVertices(0),chunks(new VertexChunk),lastChunk(chunks),vertexArray(0)
		{
		};
	~GLTriangleStripSet(void)
		{
		while(chunks!=0)
			{
			VertexChunk* next=chunks->succ;
			delete chunks;
			chunks=next;
			}
		delete[] vertexArray;
		};
	
	/* Methods: */
	void clear(void) // Returns a triangle strip set to its pristine state
		{
		numVertices=0;
		while(chunks!=0)
			{
			VertexChunk* next=chunks->succ;
			delete chunks;
			chunks=next;
			}
		chunks=new VertexChunk;
		lastChunk=chunks;
		triangleStrips.clear();
		delete[] vertexArray;
		vertexArray=0;
		};
	void addVertex(const VertexType& newVertex) // Adds a new vertex
		{
		++numVertices;
		if(lastChunk->numVertices==vertexChunkSize)
			{
			/* Add a new chunk: */
			VertexChunk* newChunk=new VertexChunk;
			lastChunk->succ=newChunk;
			lastChunk=newChunk;
			}
		
		/* Add the new vertex to the last chunk: */
		lastChunk->vertices[lastChunk->numVertices]=newVertex;
		++lastChunk->numVertices;
		};
	void beginStrip(void) // Starts a new triangle strip
		{
		triangleStrips.push_back(TriangleStrip(numVertices));
		};
	void endStrip(void) // Finishes the current triangle strip
		{
		triangleStrips[triangleStrips.size()-1].numVertices=numVertices-triangleStrips[triangleStrips.size()-1].firstVertex;
		};
	void finalize(void) // Creates a vertex array and deletes all vertex chunks. No more adding afterwards!
		{
		/* Copy all vertices into one consecutive vertex array: */
		vertexArray=new VertexType[numVertices];
		VertexType* vaPtr=vertexArray;
		while(chunks!=0)
			{
			/* Copy the chunk's contents: */
			memcpy(vaPtr,chunks->vertices,chunks->numVertices*sizeof(VertexType));
			vaPtr+=chunks->numVertices;
			
			/* Delete the chunk and go to the next: */
			VertexChunk* next=chunks->succ;
			delete chunks;
			chunks=next;
			}
		chunks=lastChunk=0;
		};
	void glRenderAction(void) const // Renders all triangle strips
		{
		/* Enable the vertex array: */
		glVertexPointer(vertexArray);
		
		/* Render all triangle strips: */
		for(typename TriangleStripSet::const_iterator tsIt=triangleStrips.begin();tsIt!=triangleStrips.end();++tsIt)
			glDrawArrays(GL_TRIANGLE_STRIP,tsIt->firstVertex,tsIt->numVertices);
		};
	};

#endif
