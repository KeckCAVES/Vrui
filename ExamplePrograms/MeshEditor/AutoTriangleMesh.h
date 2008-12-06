/***********************************************************************
AutoTriangleMesh - Class for triangular meshes that enforce triangle
shape constraints under mesh transformations.
Copyright (c) 2003-2006 Oliver Kreylos

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

#ifndef AUTOTRIANGLEMESH_INCLUDED
#define AUTOTRIANGLEMESH_INCLUDED

#include "PolygonMesh.h"

template <class PointType>
class IndexedPoint:public PointType
	{
	/* Elements: */
	public:
	unsigned int index; // Point's index in global vertex array
	unsigned int version; // Point's free-running version counter
	
	/* Constructors and destructors: */
	IndexedPoint(const PointType& sp)
		:PointType(sp),index(0),version(0)
		{
		};
	};

template <class PointType>
class AutoTriangleMesh:public PolygonMesh<IndexedPoint<PointType> >
	{
	/* Embedded classes: */
	public:
	typedef PointType BasePoint; // Basic type for affine points
	typedef IndexedPoint<BasePoint> Point; // Type for points with vertex indices
	typedef typename Point::Scalar Scalar; // Scalar type of points
	typedef PolygonMesh<Point> BaseMesh; // Base class type
	typedef typename BaseMesh::Vertex Vertex;
	typedef typename BaseMesh::Edge Edge;
	typedef typename BaseMesh::Face Face;
	typedef typename BaseMesh::VertexIterator VertexIterator;
	typedef typename BaseMesh::EdgeIterator EdgeIterator;
	typedef typename BaseMesh::ConstEdgeIterator ConstEdgeIterator;
	typedef typename BaseMesh::FaceIterator FaceIterator;
	
	/* Elements: */
	protected:
	unsigned int nextVertexIndex; // Index assigned to next created vertex
	unsigned int version; // Mesh's free-running version counter
	
	/* Protected methods: */
	void triangulateAllFaces(void); // Converts polygon mesh to triangle mesh
	void createVertexIndices(void); // Assigns indices and version numbers to all vertices
	void calcNormal(const Vertex* vPtr,float normal[3]) const; // Calculates (non-normalized) normal vector of a vertex
	
	/* Constructors and destructors: */
	public:
	AutoTriangleMesh(void) // Creates empty mesh
		:nextVertexIndex(0),version(1)
		{
		};
	template <class InputPointType>
	AutoTriangleMesh(int numPoints,const InputPointType* points,const int* vertexIndices,int numSharpEdges,const int* sharpEdgeIndices); // Creates triangle mesh from face list
	AutoTriangleMesh(const BaseMesh& source); // Copies a polygon mesh and converts it into an automatic triangle mesh
	AutoTriangleMesh(const AutoTriangleMesh& source) // Copies an automatic triangle mesh
		:BaseMesh(source)
		{
		/* Create vertex indices and reset vertex versions: */
		createVertexIndices();
		};
	AutoTriangleMesh& operator=(const BaseMesh& source); // Assigns a polygon mesh and triangulates it
	
	/* Methods: */
	unsigned int getNextVertexIndex(void) const // Returns next assigned vertex index
		{
		return nextVertexIndex;
		};
	unsigned int getVersion(void) const // Returns current version number of mesh
		{
		return version;
		};
	void update(void) // Bumps up mesh's version number by one
		{
		++version;
		};
	void splitEdge(const EdgeIterator& edge); // Splits an edge at its midpoint
	bool canCollapseEdge(const ConstEdgeIterator& edge) const; // Tests if an edge can be collapsed
	bool collapseEdge(const EdgeIterator& edge); // Collapses an edge to its midpoint; returns false if edge is not collapsible
	void limitEdgeLength(const BasePoint& center,double radius,double maxEdgeLength);
	void ensureEdgeLength(const BasePoint& center,double radius,double minEdgeLength);
	};

#ifndef AUTOTRIANGLEMESH_IMPLEMENTATION
#include "AutoTriangleMesh.cpp"
#endif

#endif
