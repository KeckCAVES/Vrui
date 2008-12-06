/***********************************************************************
MorphBox - Data structure to embed polygon meshes into upright boxes
that can be subsequently deformed to morph the embedded mesh.
Copyright (c) 2004-2006 Oliver Kreylos

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

#ifndef MORPHBOX_INCLUDED
#define MORPHBOX_INCLUDED

#include <vector>
#include <Geometry/Point.h>
#include <Geometry/OrthogonalTransformation.h>

/* Forward declarations: */
class GLContextData;

template <class MeshType>
class MorphBox
	{
	/* Embedded classes: */
	public:
	typedef MeshType Mesh;
	typedef typename Mesh::Point::Scalar Scalar;
	typedef Geometry::Point<Scalar,3> Point;
	typedef Geometry::OrthogonalTransformation<Scalar,3> OGTransform;
	private:
	typedef typename Mesh::Vertex MVertex;
	typedef typename Mesh::VertexIterator MVertexIterator;
	
	struct MorphVertex // Data structure for morphed vertices
		{
		/* Elements: */
		public:
		MVertex* v; // Pointer to the morphed vertex
		Scalar boxCoords[3]; // Box coordinates of the morphed vertex
		};
	
	/* Elements: */
	Mesh* mesh; // Mesh this morph box is manipulating
	Point boxVertices[8]; // Array of current positions of box vertices
	std::vector<MorphVertex> morphedVertices; // List of morphed mesh vertices
	int numDraggedVertices; // Number of dragged box vertices
	int draggedVertexIndices[8]; // Indices of dragged box vertices
	Point draggedVertices[8]; // Initial positions of dragged box vertices
	
	/* Constructors and destructors: */
	public:
	MorphBox(Mesh* sMesh,const Point& origin,const Scalar size[3]); // Creates an upright morph box
	
	/* Methods: */
	bool pickBox(Scalar vertexDist,Scalar edgeDist,Scalar faceDist,const Point& pickPoint); // Picks the morph box using a point; returns true if pick successful
	void startDragBox(const OGTransform& startTransformation); // Starts dragging the morph box
	void dragBox(const OGTransform& currentTransformation); // Drags the morph box and applies morph to morphed mesh vertices
	void stopDragBox(void); // Stops dragging the morph box
	void glRenderAction(const GLContextData& contextData) const;
	};

#ifndef MORPHBOX_IMPLEMENTATION
#include "MorphBox.cpp"
#endif

#endif
