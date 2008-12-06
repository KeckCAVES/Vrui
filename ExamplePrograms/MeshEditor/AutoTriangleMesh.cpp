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

#define AUTOTRIANGLEMESH_IMPLEMENTATION

#include <stdio.h>

#include "AutoTriangleMesh.h"

/*********************************
Methods of class AutoTriangleMesh:
*********************************/

template <class PointType>
void AutoTriangleMesh<PointType>::triangulateAllFaces(void)
	{
	/* Find all non-triangular faces and triangulate them: */
	for(FaceIterator faceIt=BaseMesh::beginFaces();faceIt!=BaseMesh::endFaces();++faceIt)
		{
		/* Triangulate the face if it is not a triangle: */
		if(faceIt->getNumEdges()>3)
			triangulateFace(faceIt);
		}
	}

template <class PointType>
void AutoTriangleMesh<PointType>::createVertexIndices(void)
	{
	/* Reset vertex index counter and mesh version number: */
	nextVertexIndex=0;
	version=1;
	
	/* Assign vertex indices and version numbers to all vertices: */
	for(VertexIterator vIt=BaseMesh::beginVertices();vIt!=BaseMesh::endVertices();++vIt)
		{
		vIt->index=nextVertexIndex;
		++nextVertexIndex;
		vIt->version=version;
		}
	}

template <class PointType>
void AutoTriangleMesh<PointType>::calcNormal(const typename AutoTriangleMesh<PointType>::Vertex* vPtr,float normal[3]) const
	{
	/* Set normal vector to zero: */
	for(int i=0;i<3;++i)
		normal[i]=0.0f;
	
	/* Iterate through vertex' platelet: */
	const Edge* ve=vPtr->getEdge();
	do
		{
		const Edge* ve2=ve->getFacePred()->getOpposite();
		float triangleNormal[3];
		planeNormal(*ve->getStart(),*ve->getEnd(),*ve2->getEnd(),triangleNormal);
		for(int i=0;i<3;++i)
			normal[i]+=triangleNormal[i];

		/* Go to next edge around vertex: */
		ve=ve2;
		}
	while(ve!=vPtr->getEdge());
	}

template <class PointType>
template <class InputPointType>
AutoTriangleMesh<PointType>::AutoTriangleMesh(int numPoints,const InputPointType* points,const int* vertexIndices,int numSharpEdges,const int* sharpEdgeIndices)
	:BaseMesh(numPoints,points,vertexIndices,numSharpEdges,sharpEdgeIndices)
	{
	/* Polygon mesh is already created; now triangulate it: */
	triangulateAllFaces();
	
	/* Number all vertices: */
	createVertexIndices();
	}

template <class PointType>
AutoTriangleMesh<PointType>::AutoTriangleMesh(const AutoTriangleMesh<PointType>::BaseMesh& source)
	:BaseMesh(source)
	{
	/* Polygon mesh is already created; now triangulate it: */
	triangulateAllFaces();
	
	/* Number all vertices: */
	createVertexIndices();
	}

template <class PointType>
AutoTriangleMesh<PointType>& AutoTriangleMesh<PointType>::operator=(const AutoTriangleMesh<PointType>::BaseMesh& source)
	{
	if(this!=&source)
		{
		/* Copy polygon mesh: */
		*this=source;
		
		/* Polygon mesh is already created; now triangulate it: */
		triangulateAllFaces();
		
		/* Number all vertices: */
		createVertexIndices();
		}
	
	return *this;
	}

template <class PointType>
void AutoTriangleMesh<PointType>::splitEdge(const typename AutoTriangleMesh<PointType>::EdgeIterator& edge)
	{
	/* Get triangle topology: */
	Edge* e1=&(*edge);
	Edge* e2=e1->getFaceSucc();
	Edge* e3=e1->getFacePred();
	Vertex* v1=e1->getStart();
	Vertex* v2=e2->getStart();
	Vertex* v3=e3->getStart();
	Face* f1=e1->getFace();
	
	assert(e2->getFaceSucc()==e3&&e3->getFacePred()==e2);
	assert(e2->getFace()==f1);
	assert(e3->getFace()==f1);
	assert(f1->getEdge()==e1||f1->getEdge()==e2||f1->getEdge()==e3);
	
	Edge* e4=e1->getOpposite();
	if(e4!=0)
		{
		Edge* e5=e4->getFaceSucc();
		Edge* e6=e4->getFacePred();
		Vertex* v4=e6->getStart();
		Face* f2=e4->getFace();
		
		assert(e5->getFaceSucc()==e6&&e6->getFacePred()==e5);
		assert(e4->getStart()==v2);
		assert(e5->getStart()==v1);
		assert(e5->getFace()==f2);
		assert(e6->getFace()==f2);
		assert(f2->getEdge()==e4||f2->getEdge()==e5||f2->getEdge()==e6);
		
		/* Don't increase aspect ratio of triangles when splitting: */
		double e4Len2=sqrDist(*v1,*v2);
		double e5Len2=sqrDist(*v1,*v4);
		double e6Len2=sqrDist(*v2,*v4);
		if(e4Len2<e5Len2||e4Len2<e6Len2)
			{
			/* Split longest edge in neighbouring triangle first: */
			if(e5Len2>e6Len2)
				splitEdge(e5);
			else
				splitEdge(e6);
			
			/* Re-get triangle topology: */
			e4=e1->getOpposite();
			e5=e4->getFaceSucc();
			e6=e4->getFacePred();
			v4=e6->getStart();
			f2=e4->getFace();
			}
		
		/* Create new vertex for edge midpoint: */
		Point p=Point::zero();
		p.add(*edge->getStart());
		p.add(*edge->getEnd());
		p.normalize(2);
		p.index=nextVertexIndex;
		++nextVertexIndex;
		typename BaseMesh::Vertex* nv=newVertex(p);
		
		/* Create two quadrilaterals: */
		Edge* ne1=BaseMesh::newEdge();
		Edge* ne2=BaseMesh::newEdge();
		nv->setEdge(ne1);
		e1->setFaceSucc(ne1);
		e1->setOpposite(ne2);
		e2->setFacePred(ne1);
		e4->setFaceSucc(ne2);
		e4->setOpposite(ne1);
		e5->setFacePred(ne2);
		ne1->set(nv,f1,e1,e2,e4);
		ne1->sharpness=0;
		ne2->set(nv,f2,e4,e5,e1);
		ne2->sharpness=0;
		f1->setEdge(e1);
		f2->setEdge(e4);
		
		/* Triangulate first quadrilateral: */
		Edge* ne3=BaseMesh::newEdge();
		Edge* ne4=BaseMesh::newEdge();
		Face* nf1=BaseMesh::newFace();
		e1->setFaceSucc(ne3);
		e3->setFacePred(ne3);
		e2->setFace(nf1);
		e2->setFaceSucc(ne4);
		ne1->setFace(nf1);
		ne1->setFacePred(ne4);
		ne3->set(nv,f1,e1,e3,ne4);
		ne3->sharpness=0;
		ne4->set(v3,nf1,e2,ne1,ne3);
		ne4->sharpness=0;
		nf1->setEdge(ne1);
		
		/* Triangulate second quadrilateral: */
		Edge* ne5=BaseMesh::newEdge();
		Edge* ne6=BaseMesh::newEdge();
		Face* nf2=BaseMesh::newFace();
		e4->setFaceSucc(ne5);
		e6->setFacePred(ne5);
		e5->setFace(nf2);
		e5->setFaceSucc(ne6);
		ne2->setFace(nf2);
		ne2->setFacePred(ne6);
		ne5->set(nv,f2,e4,e6,ne6);
		ne5->sharpness=0;
		ne6->set(v4,nf2,e5,ne2,ne5);
		ne6->sharpness=0;
		nf2->setEdge(ne2);
		
		/* Update version numbers of all involved vertices: */
		++version;
		v1->version=version;
		v2->version=version;
		v3->version=version;
		v4->version=version;
		nv->version=version;
		}
	else
		{
		/* Create new vertex for edge midpoint: */
		Point p=Point::zero();
		p.add(*edge->getStart());
		p.add(*edge->getEnd());
		p.normalize(2);
		p.index=nextVertexIndex;
		++nextVertexIndex;
		typename BaseMesh::Vertex* nv=newVertex(p);
		
		/* Create one quadrilateral: */
		Edge* ne=BaseMesh::newEdge();
		nv->setEdge(ne);
		e1->setFaceSucc(ne);
		e2->setFacePred(ne);
		ne->set(nv,f1,e1,e2,0);
		ne->sharpness=0;
		f1->setEdge(e1);
		
		/* Triangulate quadrilateral: */
		Edge* ne3=BaseMesh::newEdge();
		Edge* ne4=BaseMesh::newEdge();
		Face* nf1=BaseMesh::newFace();
		e1->setFaceSucc(ne3);
		e3->setFacePred(ne3);
		e2->setFace(nf1);
		e2->setFaceSucc(ne4);
		ne->setFace(nf1);
		ne->setFacePred(ne4);
		ne3->set(nv,f1,e1,e3,ne4);
		ne3->sharpness=0;
		ne4->set(v3,nf1,e2,ne,ne3);
		ne4->sharpness=0;
		nf1->setEdge(ne);
		
		/* Update version numbers of all involved vertices: */
		++version;
		v1->version=version;
		v2->version=version;
		v3->version=version;
		nv->version=version;
		}
	}

template <class PointType>
bool AutoTriangleMesh<PointType>::canCollapseEdge(const typename AutoTriangleMesh<PointType>::ConstEdgeIterator& edge) const
	{
	/* Get triangle topology: */
	const Edge* e1=&(*edge);
	const Edge* e2=e1->getFaceSucc();
	const Edge* e3=e1->getFacePred();
	const Edge* e4=e1->getOpposite();
	if(e4==0)
		return false;
	
	const Edge* e5=e4->getFaceSucc();
	const Edge* e6=e4->getFacePred();
	const Edge* e7=e2->getOpposite();
	const Edge* e8=e3->getOpposite();
	const Edge* e9=e5->getOpposite();
	const Edge* e10=e6->getOpposite();
	
	/* Check if v3 has valence of at least 4: */
	if(e7->getFacePred()->getOpposite()->getFacePred()==e8)
		return false;
	
	/* Check if v4 has valence of at least 4: */
	if(e9->getFacePred()->getOpposite()->getFacePred()==e10)
		return false;
	
	/* Check if v1 and v2 together have at least valence 7 (then collapsed v1 will have at least valence 3): */
	if(e7->getFaceSucc()==e10&&e9->getFaceSucc()==e8)
		return false;
	
	/* Check if platelets of v1 and v2 have common vertices (except each other, of course): */
	/* Currently highly inefficient at O(n^2); need to optimize! */
	for(const Edge* ve1=e10->getFacePred();ve1!=e7;ve1=ve1->getOpposite()->getFacePred())
		for(const Edge* ve2=e8->getFacePred();ve2!=e9;ve2=ve2->getOpposite()->getFacePred())
			if(ve1->getStart()==ve2->getStart())
				return false;
	
	return true;
	}

template <class PointType>
bool AutoTriangleMesh<PointType>::collapseEdge(const typename AutoTriangleMesh<PointType>::EdgeIterator& edge)
	{
	/* Get triangle topology: */
	Edge* e1=&(*edge);
	Edge* e2=e1->getFaceSucc();
	Edge* e3=e1->getFacePred();
	Edge* e4=e1->getOpposite();
	if(e4==0)
		return false;
	
	Edge* e5=e4->getFaceSucc();
	Edge* e6=e4->getFacePred();
	Edge* e7=e2->getOpposite();
	Edge* e8=e3->getOpposite();
	Edge* e9=e5->getOpposite();
	Edge* e10=e6->getOpposite();
	Vertex* v1=e1->getStart();
	Vertex* v2=e2->getStart();
	Vertex* v3=e3->getStart();
	Vertex* v4=e6->getStart();
	Face* f1=e1->getFace();
	Face* f2=e4->getFace();
	
	/* Check if v3 has valence of at least 4: */
	if(e7->getFacePred()->getOpposite()->getFacePred()==e8)
		return false;
	
	/* Check if v4 has valence of at least 4: */
	if(e9->getFacePred()->getOpposite()->getFacePred()==e10)
		return false;
	
	/* Check if v1 and v2 together have at least valence 7 (then collapsed v1 will have at least valence 3): */
	if(e7->getFaceSucc()==e10&&e9->getFaceSucc()==e8)
		return false;
	
	/* Check if platelets of v1 and v2 have common vertices (except each other, of course): */
	/* Currently highly inefficient at O(n^2); need to optimize! */
	for(Edge* ve1=e10->getFacePred();ve1!=e7;ve1=ve1->getOpposite()->getFacePred())
		for(Edge* ve2=e8->getFacePred();ve2!=e9;ve2=ve2->getOpposite()->getFacePred())
			if(ve1->getStart()==ve2->getStart())
				return false;
	
	assert(v2->getEdge()!=0);
	assert(f1->getEdge()!=0);
	assert(f2->getEdge()!=0);
	
	assert(e2->getFaceSucc()==e3&&e3->getFacePred()==e2);
	assert(e5->getFaceSucc()==e6&&e6->getFacePred()==e5);
	assert(e4->getStart()==v2);
	assert(e5->getStart()==v1);
	assert(e7->getOpposite()==e2);
	assert(e8->getOpposite()==e3);
	assert(e9->getOpposite()==e5);
	assert(e10->getOpposite()==e6);
	assert(e7->getStart()==v3);
	assert(e8->getStart()==v1);
	assert(e9->getStart()==v4);
	assert(e10->getStart()==v2);
	assert(e2->getFace()==f1);
	assert(e3->getFace()==f1);
	assert(e5->getFace()==f2);
	assert(e6->getFace()==f2);
	assert(f1->getEdge()==e1||f1->getEdge()==e2||f1->getEdge()==e3);
	assert(f2->getEdge()==e4||f2->getEdge()==e5||f2->getEdge()==e6);
	
	/* Move v1 to edge midpoint: */
	Point p=Point::zero();
	p.add(*v1);
	p.add(*v2);
	p.normalize(2);
	p.index=v1->index;
	v1->setPoint(p);
	
	/* Remove both faces from mesh: */
	e7->setOpposite(e8);
	e8->setOpposite(e7);
	if(e7->sharpness<e8->sharpness)
		e7->sharpness=e8->sharpness;
	else
		e8->sharpness=e7->sharpness;
	e9->setOpposite(e10);
	e10->setOpposite(e9);
	if(e9->sharpness<e10->sharpness)
		e9->sharpness=e10->sharpness;
	else
		e10->sharpness=e9->sharpness;
	v1->setEdge(e8);
	v3->setEdge(e7);
	v4->setEdge(e9);
	
	/* Remove v2 from mesh: */
	/* Note: Works currently only for closed meshes! */
	for(Edge* e=e10;e!=e8;e=e->getVertexSucc())
		{
		assert(e->getStart()==v2);
		e->setStart(v1);
		}
	
	assert(e7->getOpposite()==e8);
	assert(e8->getOpposite()==e7);
	assert(e9->getOpposite()==e10);
	assert(e10->getOpposite()==e9);
	assert(e7->getStart()==v3);
	assert(e8->getStart()==v1);
	assert(e9->getStart()==v4);
	assert(e10->getStart()==v1);
	
	/* Delete removed objects: */
	v2->setEdge(0);
	f1->setEdge(0);
	f2->setEdge(0);
	
	deleteEdge(e1);
	deleteEdge(e2);
	deleteEdge(e3);
	deleteEdge(e4);
	deleteEdge(e5);
	deleteEdge(e6);
	deleteVertex(v2);
	deleteFace(f1);
	deleteFace(f2);
	
	/* Update version numbers of all involved vertices: */
	++version;
	v1->version=version;
	Edge* e=v1->getEdge();
	do
		{
		e->getEnd()->version=version;
		e=e->getVertexSucc();
		}
	while(e!=v1->getEdge());
	
	return true;
	}

template <class PointType>
void AutoTriangleMesh<PointType>::limitEdgeLength(const typename AutoTriangleMesh<PointType>::BasePoint& center,double radius,double maxEdgeLength)
	{
	/* Iterate through all triangles: */
	FaceIterator faceIt=BaseMesh::beginFaces();
	while(faceIt!=BaseMesh::endFaces())
		{
		/* Check whether face overlaps area of influence and calculate face's maximum edge length: */
		bool overlaps=false;
		Edge* longestEdge=0;
		double longestEdgeLength2=maxEdgeLength*maxEdgeLength;
		Edge* e=faceIt->getEdge();
		for(int i=0;i<3;++i)
			{
			overlaps=overlaps||sqrDist(*e->getStart(),center)<=radius*radius;
			
			/* Calculate edge's squared length: */
			double edgeLength2=sqrDist(*e->getStart(),*e->getEnd());
			if(longestEdgeLength2<edgeLength2)
				{
				longestEdge=e;
				longestEdgeLength2=edgeLength2;
				}
			
			/* Go to next edge: */
			e=e->getFaceSucc();
			}
		
		/* Check whether the longest triangle edge is too long: */
		if(overlaps&&longestEdge!=0)
			{
			/* Split longest edge: */
			splitEdge(longestEdge);
			}
		else
			{
			/* Go to next triangle: */
			++faceIt;
			}
		}
	}

template <class PointType>
void AutoTriangleMesh<PointType>::ensureEdgeLength(const typename AutoTriangleMesh<PointType>::BasePoint& center,double radius,double minEdgeLength)
	{
	double radius2=radius*radius;
	
	/* Iterate through all triangles: */
	FaceIterator faceIt=BaseMesh::beginFaces();
	while(faceIt!=BaseMesh::endFaces())
		{
		/* Check quickly (ha!) if face overlaps area of influence: */
		bool overlaps=false;
		Edge* e=faceIt->getEdge();
		do
			{
			if(sqrDist(*e->getStart(),center)<=radius2)
				{
				overlaps=true;
				break;
				}
			
			/* Go to next edge: */
			e=e->getFaceSucc();
			}
		while(e!=faceIt->getEdge());
		
		if(overlaps)
			{
			/* Calculate face's minimum edge length: */
			Edge* shortestEdge=0;
			double shortestEdgeLength2=minEdgeLength*minEdgeLength;
			Edge* e=faceIt->getEdge();
			do
				{
				/* Calculate edge's squared length: */
				#if 1
				double edgeLength2=sqrDist(*e->getStart(),*e->getEnd());
				#else
				/* Calculate normal vectors for edge's vertices: */
				float normal1[3],normal2[3];
				calcNormal(e->getStart(),normal1);
				calcNormal(e->getEnd(),normal2);
				float dist1=0.0f;
				float dist2=0.0f;
				float edgeLength2=0.0f;
				float normal1Length2=0.0f;
				float normal2Length2=0.0f;
				for(int i=0;i<3;++i)
					{
					float dist=(*e->getEnd())[i]-(*e->getStart())[i];
					normal1Length2+=normal1[i]*normal1[i];
					normal2Length2+=normal2[i]*normal2[i];
					dist1+=dist*normal1[i];
					dist2+=dist*normal2[i];
					edgeLength2+=dist*dist;
					}
				dist1=fabsf(dist1)/sqrtf(normal1Length2);
				dist2=fabsf(dist2)/sqrtf(normal2Length2);
				float edgeLength=sqrtf(edgeLength2)+5.0f*(dist1+dist2);
				edgeLength2=edgeLength*edgeLength;
				#endif
				if(shortestEdgeLength2>edgeLength2&&canCollapseEdge(e))
					{
					shortestEdge=e;
					shortestEdgeLength2=edgeLength2;
					}
				
				/* Go to next edge: */
				e=e->getFaceSucc();
				}
			while(e!=faceIt->getEdge());
			
			/* Go to next triangle: */
			++faceIt;
			
			/* Check whether the shortest collapsible triangle edge is too short: */
			if(shortestEdge!=0)
				{
				/* Skip next face if it will be removed by edge collapse: */
				if(faceIt==shortestEdge->getOpposite()->getFace())
					++faceIt;
				
				/* Collapse shortest collapsible edge: */
				collapseEdge(shortestEdge);
				}
			}
		else
			{
			/* Go to the next triangle: */
			++faceIt;
			}
		}
	}
