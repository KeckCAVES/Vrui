/***********************************************************************
PolygonMesh - Class providing the infrastructure for algorithms working
on meshes of convex polygons.
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

#define POLYGONMESH_IMPLEMENTATION

#include <new>
#include <iostream>
#include <Misc/HashTable.h>

#include "PolygonMesh.h"

/************************************
Methods of class PolygonMesh::Vertex:
************************************/

template <class PointType>
int PolygonMesh<PointType>::Vertex::getNumEdges(void) const
	{
	int result=0;
	const Edge* ePtr=edge;
	do
		{
		++result;
		ePtr=ePtr->getVertexSucc();
		}
	while(ePtr!=edge);

	return result;
	}

template <class PointType>
bool PolygonMesh<PointType>::Vertex::isInterior(void) const
	{
	if(edge==0)
		return false;
	
	const Edge* ePtr=edge;
	do
		{
		ePtr=ePtr->getVertexSucc();
		}
	while(ePtr!=edge&&ePtr!=0);
	
	return ePtr!=0;
	}

template <class PointType>
void PolygonMesh<PointType>::Vertex::checkVertex(void) const
	{
	assert(edge!=0);
	
	const Edge* ePtr=edge;
	do
		{
		assert(ePtr->getStart()==this);
		assert(ePtr->getFacePred()->getFaceSucc()==ePtr);
		assert(ePtr->getOpposite()!=0);
		assert(ePtr->getOpposite()->getOpposite()==ePtr);
		assert(ePtr->sharpness==ePtr->getOpposite()->sharpness);
		assert(ePtr->getVertexSucc()->getVertexPred()==ePtr);
		assert(ePtr->getVertexPred()->getVertexSucc()==ePtr);
		
		ePtr=ePtr->getVertexSucc();
		}
	while(ePtr!=edge);
	}

/**********************************
Methods of class PolygonMesh::Face:
**********************************/

template <class PointType>
int PolygonMesh<PointType>::Face::getNumEdges(void) const
	{
	int result=0;
	const Edge* ePtr=edge;
	do
		{
		++result;
		ePtr=ePtr->getFaceSucc();
		}
	while(ePtr!=edge);

	return result;
	}

template <class PointType>
void PolygonMesh<PointType>::Face::checkFace(void) const
	{
	assert(edge!=0);
	
	const Edge* ePtr=edge;
	do
		{
		assert(ePtr->getFace()==this);
		assert(ePtr->getFaceSucc()->getFacePred()==ePtr);
		assert(ePtr->getFacePred()->getFaceSucc()==ePtr);
		
		ePtr=ePtr->getFaceSucc();
		}
	while(ePtr!=edge);
	}

/****************************
Methods of class PolygonMesh:
****************************/

template <class PointType>
template <class InputPointType>
typename PolygonMesh<PointType>::Vertex* PolygonMesh<PointType>::newVertex(const InputPointType& p)
	{
	/* Create a new vertex and link it to the vertex list: */
	Vertex* newVertex=new(vertexAllocator.allocate()) Vertex(p,0);
	newVertex->pred=lastVertex;
	if(lastVertex!=0)
		lastVertex->succ=newVertex;
	else
		vertices=newVertex;
	lastVertex=newVertex;
	return newVertex;
	}

template <class PointType>
void PolygonMesh<PointType>::deleteVertex(typename PolygonMesh<PointType>::Vertex* vertex)
	{
	/* Unlink the vertex from the vertex list: */
	if(vertex->pred!=0)
		vertex->pred->succ=vertex->succ;
	else
		vertices=vertex->succ;
	if(vertex->succ!=0)
		vertex->succ->pred=vertex->pred;
	else
		lastVertex=vertex->pred;
	
	/* Delete the vertex: */
	vertex->~Vertex();
	vertexAllocator.free(vertex);
	}

template <class PointType>
typename PolygonMesh<PointType>::Edge* PolygonMesh<PointType>::newEdge(void)
	{
	/* Create a new edge: */
	return new(edgeAllocator.allocate()) Edge;
	}

template <class PointType>
void PolygonMesh<PointType>::deleteEdge(typename PolygonMesh<PointType>::Edge* edge)
	{
	/* Delete the edge: */
	edge->~Edge();
	edgeAllocator.free(edge);
	}

template <class PointType>
typename PolygonMesh<PointType>::Face* PolygonMesh<PointType>::newFace(void)
	{
	/* Create a new face and link it to the face list: */
	Face* newFace=new(faceAllocator.allocate()) Face(0);
	newFace->pred=lastFace;
	if(lastFace!=0)
		lastFace->succ=newFace;
	else
		faces=newFace;
	lastFace=newFace;
	return newFace;
	}

template <class PointType>
void PolygonMesh<PointType>::deleteFace(typename PolygonMesh<PointType>::Face* face)
	{
	/* Unlink the face from the face list: */
	if(face->pred!=0)
		face->pred->succ=face->succ;
	else
		faces=face->succ;
	if(face->succ!=0)
		face->succ->pred=face->pred;
	else
		lastFace=face->pred;
	
	/* Delete the face: */
	face->~Face();
	faceAllocator.free(face);
	}

template <class PointType>
template <class InputPointType>
PolygonMesh<PointType>::PolygonMesh(int numPoints,const InputPointType* points,const int* vertexIndices,int numSharpEdges,const int* sharpEdgeIndices)
	:vertices(0),lastVertex(0),faces(0),lastFace(0)
	{
	/* Local data type for reasons why a face does not conform to a mesh: */
	enum NonconformanceReason
		{
		NONE,NON_MANIFOLD,WRONG_ORIENTATION
		};
	
	/* Create vertices for all given points: */
	Vertex** vertexArray=new Vertex*[numPoints];
	for(int i=0;i<numPoints;++i)
		vertexArray[i]=newVertex(points[i]);
	
	/* Count the number of edges (to estimate the hash table size): */
	int numEdges=0;
	for(int i=0;vertexIndices[i]>=0||vertexIndices[i+1]>=0;++i)
		if(vertexIndices[i]>=0)
			++numEdges;
	Misc::HashTable<VertexPair,Edge*,VertexPair> companions(numEdges);
	
	/* Create (and connect) all polygons: */
	int faceIndex=0;
	const int* indexPtr=vertexIndices;
	while(*indexPtr>=0) // End of face list is denoted by -1
		{
		/* Check whether the current polygon conforms with the mesh: */
		bool faceConforms=true;
		NonconformanceReason nonconformanceReason=NONE;
		const int* checkIndexPtr=indexPtr;
		do
			{
			/* Get indices of vertices of current edge: */
			int i1=checkIndexPtr[0];
			int i2=checkIndexPtr[1];
			if(i2<0)
				i2=indexPtr[0];
			
			/* Look for the current edge in the edge hash table: */
			VertexPair vp(vertexArray[i1],vertexArray[i2]);
			typename Misc::HashTable<VertexPair,Edge*,VertexPair>::Iterator companionsIt=companions.findEntry(vp);
			if(companionsIt!=companions.end())
				{
				Edge* companion=companionsIt->getDest();
				
				/* Test if the companion edge already has an opposite: */
				if(companion->getOpposite()!=0)
					{
					nonconformanceReason=NON_MANIFOLD;
					faceConforms=false;
					}
				
				/* Test if the two edges are properly oriented: */
				if(companion->getStart()!=vertexArray[i2]||companion->getEnd()!=vertexArray[i1])
					{
					nonconformanceReason=WRONG_ORIENTATION;
					faceConforms=false;
					}
				}
			
			/* Go to the next edge: */
			++checkIndexPtr;
			}
		while(*checkIndexPtr>=0);
		
		/* Only create the new face if it conforms with the mesh: */
		if(faceConforms)
			{
			/* Create the new polygon: */
			Face* face=newFace();
			
			/* Create the face without connecting it to neighbours yet: */
			Edge* firstEdge;
			Edge* lastEdge=0;
			do
				{
				Edge* edge=newEdge();
				vertexArray[*indexPtr]->setEdge(edge);
				edge->set(vertexArray[*indexPtr],face,lastEdge,0,0);
				edge->sharpness=0;
				if(lastEdge!=0)
					lastEdge->setFaceSucc(edge);
				else
					firstEdge=edge;
				
				lastEdge=edge;
				++indexPtr;
				}
			while(*indexPtr>=0);
			lastEdge->setFaceSucc(firstEdge);
			firstEdge->setFacePred(lastEdge);
			face->setEdge(firstEdge);
			
			/* Walk around the face again and connect it to its neighbours: */
			lastEdge=firstEdge;
			do
				{
				VertexPair vp(*lastEdge);
				typename Misc::HashTable<VertexPair,Edge*,VertexPair>::Iterator companionsIt=companions.findEntry(vp);
				if(companionsIt!=companions.end())
					{
					/* Connect the edge to its companion: */
					Edge* companion=companionsIt->getDest();
					assert(companion->getOpposite()==0);
					assert(companion->getEnd()==lastEdge->getStart());
					lastEdge->setOpposite(companion);
					companion->setOpposite(lastEdge);
					}
				else
					{
					/* Add the edge to the companion table: */
					companions.setEntry(typename Misc::HashTable<VertexPair,Edge*,VertexPair>::Entry(vp,lastEdge));
					}
				
				lastEdge=lastEdge->getFaceSucc();
				}
			while(lastEdge!=firstEdge);
			}
		else
			{
			/* Print an error message and skip the offending face in the process: */
			switch(nonconformanceReason)
				{
				case NON_MANIFOLD:
					std::cout<<"Non-manifold edge in face "<<faceIndex<<" with vertex indices ["<<*indexPtr;
					break;
				
				case WRONG_ORIENTATION:
					std::cout<<"Wrong orientation in face "<<faceIndex<<" with vertex indices ["<<*indexPtr;
					break;
				
				default:
					;
				}
			++indexPtr;
			while(*indexPtr>=0)
				{
				std::cerr<<", "<<*indexPtr;
				++indexPtr;
				}
			std::cout<<"]."<<std::endl;
			}
		
		/* Skip the -1 denoting the end of the current face's vertex list and go to the next face: */
		++indexPtr;
		++faceIndex;
		}
	
	/* Sharpify all given edges: */
	for(int i=0;i<numSharpEdges;++i)
		{
		VertexPair vp(vertexArray[sharpEdgeIndices[i*3+0]],vertexArray[sharpEdgeIndices[i*3+1]]);
		typename Misc::HashTable<VertexPair,Edge*,VertexPair>::Iterator companionsIt=companions.findEntry(vp);
		if(companionsIt!=companions.end())
			{
			/* Set the sharpness of the edge: */
			companionsIt->getDest()->sharpness=sharpEdgeIndices[i*3+2];
			companionsIt->getDest()->getOpposite()->sharpness=sharpEdgeIndices[i*3+2];
			}
		}
	
	/* Delete temporary array of vertices: */
	delete[] vertexArray;
	}

template <class PointType>
PolygonMesh<PointType>::PolygonMesh(const PolygonMesh<PointType>& source)
	:vertices(0),lastVertex(0),faces(0),lastFace(0)
	{
	/* Copy all vertices and associate the copies with their originals: */
	Misc::HashTable<const Vertex*,Vertex*> vertexMap((source.getNumVertices()*3)/2);
	for(const Vertex* vPtr=source.vertices;vPtr!=0;vPtr=vPtr->succ)
		vertexMap.setEntry(typename Misc::HashTable<const Vertex*,Vertex*>::Entry(vPtr,newVertex(*vPtr)));
	
	/* Count the number of edges in the source mesh to estimate the needed hash table size: */
	const Face* fPtr;
	int numEdges=0;
	for(fPtr=source.faces;fPtr!=0;fPtr=fPtr->succ)
		{
		const Edge* firstSourceEdge=fPtr->getEdge();
		const Edge* fePtr=firstSourceEdge;
		do
			{
			++numEdges;
			fePtr=fePtr->getFaceSucc();
			}
		while(fePtr!=firstSourceEdge);
		}
	Misc::HashTable<VertexPair,Edge*,VertexPair> companions(numEdges);
	
	/* Copy faces one at a time: */
	for(fPtr=source.faces;fPtr!=0;fPtr=fPtr->succ)
		{
		Face* face=newFace();
		
		/* Copy all edges of the face (don't connect them to other faces yet): */
		const Edge* firstSourceEdge=fPtr->getEdge();
		const Edge* fePtr=firstSourceEdge;
		Edge* firstEdge;
		Edge* lastEdge=0;
		do
			{
			/* Create a new edge: */
			Edge* edge=newEdge();
			Vertex* vPtr=vertexMap.getEntry(fePtr->getStart()).getDest();
			edge->set(vPtr,face,lastEdge,0,0);
			edge->sharpness=fePtr->sharpness;
			vPtr->setEdge(edge);
			if(lastEdge!=0)
				lastEdge->setFaceSucc(edge);
			else
				firstEdge=edge;
			
			lastEdge=edge;
			fePtr=fePtr->getFaceSucc();
			}
		while(fePtr!=firstSourceEdge);
		lastEdge->setFaceSucc(firstEdge);
		firstEdge->setFacePred(lastEdge);
		face->setEdge(firstEdge);
		
		/* Now go around the edge loop again to connect the face to its neighbours: */
		Edge* edge=firstEdge;
		do
			{
			VertexPair vp(*edge);
			typename Misc::HashTable<VertexPair,Edge*,VertexPair>::Iterator companionsIt=companions.findEntry(vp);
			if(companionsIt!=companions.end())
				{
				/* Connect the edge to its companion: */
				edge->setOpposite(companionsIt->getDest());
				companionsIt->getDest()->setOpposite(edge);
				}
			else
				{
				/* Add the edge to the companion table: */
				companions.setEntry(typename Misc::HashTable<VertexPair,Edge*,VertexPair>::Entry(vp,edge));
				}
			
			edge=edge->getFaceSucc();
			}
		while(edge!=firstEdge);
		}
	}

template <class PointType>
PolygonMesh<PointType>::~PolygonMesh(void)
	{
	/* Delete all faces and their associated half-edges: */
	Face* fPtr=faces;
	while(fPtr!=0)
		{
		/* Delete all the face's half-edges: */
		Edge* ePtr=fPtr->edge;
		do
			{
			Edge* next=ePtr->getFaceSucc();
			deleteEdge(ePtr);
			ePtr=next;
			}
		while(ePtr!=fPtr->edge);
		
		/* Delete the face: */
		Face* next=fPtr->succ;
		fPtr->~Face();
		faceAllocator.free(fPtr);
		
		fPtr=next;
		}
	
	/* Delete all vertices: */
	Vertex* vPtr=vertices;
	while(vPtr!=0)
		{
		Vertex* next=vPtr->succ;
		vPtr->~Vertex();
		vertexAllocator.free(vPtr);
		
		vPtr=next;
		}
	}

template <class PointType>
int PolygonMesh<PointType>::getNumVertices(void) const
	{
	int result=0;
	for(const Vertex* vPtr=vertices;vPtr!=0;vPtr=vPtr->succ)
		++result;
	
	return result;
	}

template <class PointType>
int PolygonMesh<PointType>::getNumFaces(void) const
	{
	int result=0;
	for(const Face* fPtr=faces;fPtr!=0;fPtr=fPtr->succ)
		++result;
	
	return result;
	}

/******************************************************************
Do not use this function - put in but abandoned and probably broken
******************************************************************/

template <class PointType>
typename PolygonMesh<PointType>::FaceIterator PolygonMesh<PointType>::removeVertex(const typename PolygonMesh<PointType>::VertexIterator& vertexIt)
	{
	if(vertexIt->isInterior())
		{
		/* Combine all surrounding faces into a single face: */
		Face* vertexFace=newFace();
		Edge* lastEdge=0;
		Edge* ePtr=vertexIt->getEdge();
		ePtr->getOpposite()->setOpposite(0);
		while(ePtr!=0)
			{
			/* Re-arrange all face pointers: */
			Edge* vePtr=ePtr->getFaceSucc();
			while(vePtr!=ePtr->getFacePred())
				{
				vePtr->setFace(vertexFace);
				vePtr=vePtr->getFaceSucc();
				}
			
			/* Fix up the vertex: */
			ePtr->getEnd()->setEdge(ePtr->getFaceSucc());
			
			/* Delete the outgoing and incoming edges: */
			Edge* nextEptr=ePtr->getVertexSucc();
			ePtr->getFaceSucc()->setFacePred(lastEdge);
			if(lastEdge!=0)
				lastEdge->setFacePred(ePtr->getFaceSucc());
			else
				vertexFace->setEdge(ePtr->getFaceSucc());
			lastEdge=ePtr->getFacePred()->getFacePred();
			deleteFace(ePtr->getFace());
			deleteEdge(ePtr->getFacePred());
			deleteEdge(ePtr);
			
			/* Go to the next face: */
			ePtr=nextEptr;
			}
		
		/* Close the face loop: */
		lastEdge->setFaceSucc(vertexFace->getEdge());
		vertexFace->getEdge()->setFacePred(lastEdge);
		
		deleteVertex(vertexIt.vertex);
		return vertexFace;
		}
	else if(vertexIt->getEdge()!=0)
		{
		/* Go backwards until border edge is hit: */
		Edge* ePtr;
		for(ePtr=vertexIt->getEdge();ePtr->getOpposite()!=0;ePtr=ePtr->getVertexPred())
			;
		
		/* Remove all surrounding faces: */
		while(ePtr!=0)
			{
			Edge* nextEptr=ePtr->getVertexSucc();
			deleteFace(ePtr->getFace());
			Edge* fePtr=ePtr;
			do
				{
				Edge* nextFePtr=fePtr->getFaceSucc();
				
				/* Fix up the vertex: */
				if(nextFePtr!=ePtr)
					{
					if(nextFePtr->getVertexPred()!=0)
						nextFePtr->getStart()->setEdge(nextFePtr->getVertexPred());
					else
						nextFePtr->getStart()->setEdge(fePtr->getOpposite()->getFacePred());
					}
				
				/* Remove the edge: */
				if(fePtr->getOpposite()!=0)
					fePtr->getOpposite()->setOpposite(0);
				deleteEdge(fePtr);
				}
			while(fePtr!=ePtr);
			
			ePtr=nextEptr;
			}
		
		deleteVertex(vertexIt.vertex);
		return FaceIterator(0);
		}
	else // Dangling vertex
		{
		deleteVertex(vertexIt.vertex);
		return FaceIterator(0);
		}
	}

template <class PointType>
typename PolygonMesh<PointType>::FaceIterator PolygonMesh<PointType>::vertexToFace(const typename PolygonMesh<PointType>::VertexIterator& vertexIt)
	{
	/* Remove solitary vertices: */
	if(vertexIt->getEdge()==0)
		{
		deleteVertex(vertexIt.vertex);
		return FaceIterator(0);
		}
	
	/* Walk around the vertex and flip its edges: */
	Face* vertexFace=newFace();
	Edge* lastEdge=0;
	Edge* ePtr=vertexIt->getEdge();
	do
		{
		Edge* nextEdge=ePtr->getFacePred()->getOpposite();
		
		/* Remove the edge from its current face and add it to the vertex face: */
		Edge* pred=ePtr->getFacePred();
		Edge* succ=ePtr->getFaceSucc();
		
		/* Test for the dangerous special case of a triangle: */
		if(succ->getFaceSucc()==pred)
			{
			/* Remove the triangle completely: */
			deleteFace(succ->getFace());
			deleteEdge(ePtr);
			deleteEdge(pred);
			
			/* Put the outside edge into the new face loop: */
			succ->set(succ->getStart(),vertexFace,lastEdge,0,succ->getOpposite());
			ePtr=succ;
			}
		else
			{
			pred->setFaceSucc(succ);
			succ->setFacePred(pred);
			ePtr->set(succ->getStart(),vertexFace,lastEdge,0,pred);
			pred->setOpposite(ePtr);
			ePtr->sharpness=pred->sharpness=0;
			pred->getFace()->setEdge(pred);

			#ifndef NDEBUG
			pred->getFace()->checkFace();
			#endif
			}
		
		if(lastEdge!=0)
			{
			lastEdge->setFaceSucc(ePtr);
			
			#ifndef NDEBUG
			ePtr->getStart()->checkVertex();
			#endif
			}
		else
			vertexFace->setEdge(ePtr);
		lastEdge=ePtr;
		
		ePtr=nextEdge;
		}
	while(ePtr!=vertexIt->getEdge());
	lastEdge->setFaceSucc(vertexFace->getEdge());
	vertexFace->getEdge()->setFacePred(lastEdge);
	
	#ifndef NDEBUG
	ePtr->getStart()->checkVertex();
	vertexFace->checkFace();
	#endif
	
	/* Delete the vertex and return the new face: */
	vertexIt->setEdge(0);
	deleteVertex(vertexIt.vertex);
	
	return FaceIterator(vertexFace);
	}

template <class PointType>
typename PolygonMesh<PointType>::VertexIterator PolygonMesh<PointType>::splitEdge(const typename PolygonMesh<PointType>::EdgeIterator& edgeIt,typename PolygonMesh<PointType>::Vertex* edgePoint)
	{
	/* Link vertex to mesh: */
	edgePoint->pred=lastVertex;
	edgePoint->succ=0;
	if(lastVertex!=0)
		lastVertex->succ=edgePoint;
	else
		vertices=edgePoint;
	lastVertex=edgePoint;
	
	Edge* edge1=edgeIt.edge;
	Vertex* vertex1=edge1->getStart();
	Edge* edge2=edge1->getOpposite();
	Vertex* vertex2=edge2->getStart();
	Edge* edge3=newEdge();
	Edge* edge4=newEdge();
	
	/* Split edge1 and edge2: */
	edgePoint->setEdge(edge3);
	edge3->set(edgePoint,edge1->getFace(),edge1,edge1->getFaceSucc(),edge2);
	edge3->sharpness=edge1->sharpness;
	edge4->set(edgePoint,edge2->getFace(),edge2,edge2->getFaceSucc(),edge1);
	edge4->sharpness=edge2->sharpness;
	edge1->setFaceSucc(edge3);
	edge1->setOpposite(edge4);
	edge2->setFaceSucc(edge4);
	edge2->setOpposite(edge3);
	edge3->getFaceSucc()->setFacePred(edge3);
	edge4->getFaceSucc()->setFacePred(edge4);
	
	#ifndef NDEBUG
	vertex1->checkVertex();
	vertex2->checkVertex();
	edgePoint->checkVertex();
	edge1->getFace()->checkFace();
	edge2->getFace()->checkFace();
	#endif
	
	return VertexIterator(edgePoint);
	}

template <class PointType>
void PolygonMesh<PointType>::rotateEdge(const typename PolygonMesh<PointType>::EdgeIterator& edgeIt)
	{
	/* Collect environment of the edge to rotate: */
	Edge* edge1=edgeIt.edge;
	Vertex* vertex1=edge1->getStart();
	Face* face1=edge1->getFace();
	Edge* edge3=edge1->getFacePred();
	Edge* edge4=edge1->getFaceSucc();
	Edge* edge2=edge1->getOpposite();
	Vertex* vertex2=edge2->getStart();
	Face* face2=edge2->getFace();
	Edge* edge5=edge2->getFacePred();
	Edge* edge6=edge2->getFaceSucc();
	
	/* Rotate the edge: */
	vertex1->setEdge(edge6);
	vertex2->setEdge(edge4);
	face1->setEdge(edge1);
	face2->setEdge(edge2);
	edge1->set(edge6->getEnd(),face1,edge6,edge4->getFaceSucc(),edge2);
	edge2->set(edge4->getEnd(),face2,edge4,edge6->getFaceSucc(),edge1);
	edge3->setFaceSucc(edge6);
	edge4->set(vertex2,face2,edge5,edge2,edge4->getOpposite());
	edge5->setFaceSucc(edge4);
	edge6->set(vertex1,face1,edge3,edge1,edge6->getOpposite());
	}

template <class PointType>
typename PolygonMesh<PointType>::FaceIterator PolygonMesh<PointType>::removeEdge(const typename PolygonMesh<PointType>::EdgeIterator& edgeIt)
	{
	Edge* edge2=edgeIt->getOpposite();
	if(edge2!=0)
		{
		/* Have all edges of the second face point to the first instead: */
		Face* newFace=edgeIt->getFace();
		for(Edge* ePtr=edge2->getFaceSucc();ePtr!=edge2;ePtr=ePtr->getFaceSucc())
			ePtr->setFace(newFace);
		
		/* Fix up the edge's start vertex: */
		edgeIt->getFacePred()->setFaceSucc(edge2->getFaceSucc());
		edge2->getFaceSucc()->setFacePred(edgeIt->getFacePred());
		edgeIt->getStart()->setEdge(edge2->getFaceSucc());
		
		/* Fix up the edge's end vertex: */
		edgeIt->getFaceSucc()->setFacePred(edge2->getFacePred());
		edge2->getFacePred()->setFaceSucc(edgeIt->getFaceSucc());
		edge2->getStart()->setEdge(edgeIt->getFaceSucc());
		
		/* Remove the edge and the second face: */
		newFace->setEdge(edgeIt->getFaceSucc());
		deleteFace(edge2->getFace());
		deleteEdge(edgeIt.edge);
		deleteEdge(edge2);
		
		return FaceIterator(newFace);
		}
	else
		{
		/* Fix up all vertices around the face: */
		Edge* ePtr=edgeIt.edge;
		do
			{
			/* Fix up the edge's start vertex: */
			if(ePtr->getVertexSucc()!=0)
				ePtr->getStart()->setEdge(ePtr->getVertexSucc());
			else
				ePtr->getStart()->setEdge(ePtr->getVertexPred());
			
			/* Go to the next edge: */
			ePtr=ePtr->getFaceSucc();
			}
		while(ePtr!=edgeIt.edge);
		
		/* Delete the face: */
		deleteFace(ePtr->getFace());
		
		/* Delete all edges: */
		ePtr=edgeIt.edge;
		ePtr->getFacePred()->setFaceSucc(0);
		while(ePtr!=0)
			{
			Edge* next=ePtr->getFaceSucc();
			deleteEdge(ePtr);
			ePtr=next;
			}
		
		return FaceIterator(0);
		}
	}

template <class PointType>
void PolygonMesh<PointType>::triangulateFace(const typename PolygonMesh<PointType>::FaceIterator& fIt)
	{
	/* Set up an initial triangle at the face's base point: */
	Face* f=&(*fIt);
	Edge* e1=f->getEdge();
	Vertex* v0=e1->getStart();
	Edge* e2=e1->getFaceSucc();
	Vertex* v1=e2->getStart();
	Edge* e3=e2->getFaceSucc();
	Vertex* v2=e3->getStart();
	Edge* lastEdge=e1->getFacePred();
	
	/* Walk around face: */
	while(e3!=lastEdge)
		{
		/* Chop triangle (v0,v1,v2) off face: */
		Edge* ne1=newEdge();
		Edge* ne2=newEdge();
		Face* nf=newFace();
		nf->setEdge(e1);
		e1->setFace(nf);
		e1->setFacePred(ne1);
		e2->setFace(nf);
		e2->setFaceSucc(ne1);
		ne1->set(v2,nf,e2,e1,ne2);
		ne1->sharpness=0;
		f->setEdge(ne2);
		
		/* Reconnect old face: */
		ne2->set(v0,f,lastEdge,e3,ne1);
		ne2->sharpness=0;
		e3->setFacePred(ne2);
		lastEdge->setFaceSucc(ne2);
		
		/* Move to next triangle: */
		e1=ne2;
		v1=v2;
		e2=e3;
		e3=e3->getFaceSucc();
		v2=e3->getStart();
		}
	}

template <class PointType>
typename PolygonMesh<PointType>::EdgeIterator PolygonMesh<PointType>::splitFace(const typename PolygonMesh<PointType>::VertexIterator& vIt1,const PolygonMesh<PointType>::VertexIterator& vIt2)
	{
	/* Find the face sharing both vertices: */
	
	}

template <class PointType>
typename PolygonMesh<PointType>::VertexIterator PolygonMesh<PointType>::splitFace(const typename PolygonMesh<PointType>::FaceIterator& faceIt,typename PolygonMesh<PointType>::Vertex* facePoint)
	{
	/* Link the face point to the mesh: */
	facePoint->pred=lastVertex;
	facePoint->succ=0;
	if(lastVertex!=0)
		lastVertex->succ=facePoint;
	else
		vertices=facePoint;
	lastVertex=facePoint;

	/* Create a fan of triangles around the face point: */
	Edge* firstOuterEdge=faceIt->getEdge();
	deleteFace(faceIt.face);
	Edge* outerEdge=firstOuterEdge;
	Edge* firstInnerEdge;
	Edge* lastInnerEdge=0;
	do
		{
		Edge* nextOuterEdge=outerEdge->getFaceSucc();
		
		/* Create a new triangle: */
		Face* triangle=newFace();
		Edge* innerEdge1=newEdge();
		Edge* innerEdge2=newEdge();
		facePoint->setEdge(innerEdge1);
		innerEdge1->set(facePoint,triangle,innerEdge2,outerEdge,lastInnerEdge);
		innerEdge1->sharpness=0;
		if(lastInnerEdge!=0)
			lastInnerEdge->setOpposite(innerEdge1);
		else
			firstInnerEdge=innerEdge1;
		innerEdge2->set(outerEdge->getEnd(),triangle,outerEdge,innerEdge1,0);
		innerEdge2->sharpness=0;
		outerEdge->setFace(triangle);
		outerEdge->setFacePred(innerEdge1);
		outerEdge->setFaceSucc(innerEdge2);
		triangle->setEdge(outerEdge);
		
		#ifndef NDEBUG
		triangle->checkFace();
		#endif
		
		lastInnerEdge=innerEdge2;
		outerEdge=nextOuterEdge;
		}
	while(outerEdge!=firstOuterEdge);
	
	/* Close the fan by connecting the first and last inner edges: */
	lastInnerEdge->setOpposite(firstInnerEdge);
	firstInnerEdge->setOpposite(lastInnerEdge);
	
	#ifndef NDEBUG
	facePoint->checkVertex();
	#endif
	
	return VertexIterator(facePoint);
	}

template <class PointType>
typename PolygonMesh<PointType>::VertexIterator PolygonMesh<PointType>::splitFaceCatmullClark(const typename PolygonMesh<PointType>::FaceIterator& faceIt,typename PolygonMesh<PointType>::Vertex* facePoint)
	{
	assert(faceIt->getNumEdges()%2==0);
	
	/* Link the face point to the mesh: */
	facePoint->pred=lastVertex;
	facePoint->succ=0;
	if(lastVertex!=0)
		lastVertex->succ=facePoint;
	else
		vertices=facePoint;
	lastVertex=facePoint;
	
	/* Create a fan of quadrilaterals around the face point: */
	Edge* firstOuterEdge=faceIt->getEdge()->getFaceSucc();
	deleteFace(faceIt.face);
	Edge* outerEdge=firstOuterEdge;
	Edge* firstInnerEdge;
	Edge* lastInnerEdge=0;
	do
		{
		Edge* nextOuterEdge=outerEdge->getFaceSucc()->getFaceSucc();
		
		/* Create a new quadrilateral: */
		Face* quad=newFace();
		Edge* innerEdge1=newEdge();
		Edge* innerEdge2=newEdge();
		facePoint->setEdge(innerEdge1);
		innerEdge1->set(facePoint,quad,innerEdge2,outerEdge,lastInnerEdge);
		innerEdge1->sharpness=0;
		if(lastInnerEdge!=0)
			lastInnerEdge->setOpposite(innerEdge1);
		else
			firstInnerEdge=innerEdge1;
		outerEdge->setFace(quad);
		outerEdge->setFacePred(innerEdge1);
		outerEdge=outerEdge->getFaceSucc();
		innerEdge2->set(outerEdge->getEnd(),quad,outerEdge,innerEdge1,0);
		innerEdge2->sharpness=0;
		outerEdge->setFace(quad);
		outerEdge->setFaceSucc(innerEdge2);
		quad->setEdge(innerEdge1);
		
		#ifndef NDEBUG
		quad->checkFace();
		#endif
		
		lastInnerEdge=innerEdge2;
		outerEdge=nextOuterEdge;
		}
	while(outerEdge!=firstOuterEdge);
	
	/* Close the fan by connecting the first and last inner edges: */
	lastInnerEdge->setOpposite(firstInnerEdge);
	firstInnerEdge->setOpposite(lastInnerEdge);
	
	#ifndef NDEBUG
	facePoint->checkVertex();
	#endif
	
	return VertexIterator(facePoint);
	}

template <class PointType>
typename PolygonMesh<PointType>::FaceIterator PolygonMesh<PointType>::splitFaceDooSabin(const typename PolygonMesh<PointType>::FaceIterator& faceIt)
	{
	/* Calculate the face's centroid: */
	PointType centroid=PointType::zero();
	int numVertices=0;
	for(FaceEdgeIterator feIt=faceIt.beginEdges();feIt!=faceIt.endEdges();++feIt,++numVertices)
		centroid.add(*feIt->getStart());
	centroid.normalize(numVertices);
	
	/* Walk around the face again and create the inner face: */
	Face* innerFace=newFace();
	Edge* lastInnerEdge=0;
	Edge* outerEdge=faceIt->getEdge();
	for(int i=0;i<numVertices;++i,outerEdge=outerEdge->getFaceSucc())
		{
		/* Create a new vertex and a new edge: */
		PointType newPoint=PointType::zero();
		newPoint.add(centroid);
		newPoint.add(*outerEdge->getStart());
		newPoint.normalize(2);
		Vertex* newV=newVertex(newPoint);
		Edge* newE=newEdge();
		newV->setEdge(newE);
		newE->set(newV,innerFace,lastInnerEdge,0,0);
		newE->sharpness=0;
		if(lastInnerEdge!=0)
			lastInnerEdge->setFaceSucc(newE);
		else
			innerFace->setEdge(newE);
		lastInnerEdge=newE;
		}
	lastInnerEdge->setFaceSucc(innerFace->getEdge());
	innerFace->getEdge()->setFacePred(lastInnerEdge);
	
	/* Walk around the face again and create one quad face for each edge: */
	Edge* innerEdge=innerFace->getEdge();
	outerEdge=faceIt->getEdge();
	Edge* lastCrossEdge=0;
	Edge* firstCrossEdge;
	for(int i=0;i<numVertices;++i,innerEdge=innerEdge->getFaceSucc())
		{
		Edge* nextOuterEdge=outerEdge->getFaceSucc();
		
		/* Create a new face and three new edges: */
		Face* quad=newFace();
		quad->setEdge(outerEdge);
		Edge* e1=newEdge();
		Edge* e2=newEdge();
		Edge* e3=newEdge();
		e1->set(innerEdge->getEnd(),quad,e3,e2,innerEdge);
		e1->sharpness=0;
		innerEdge->setOpposite(e1);
		e2->set(innerEdge->getStart(),quad,e1,outerEdge,lastCrossEdge);
		e2->sharpness=0;
		if(lastCrossEdge!=0)
			lastCrossEdge->setOpposite(e2);
		else
			firstCrossEdge=e2;
		e3->set(outerEdge->getEnd(),quad,outerEdge,e1,0);
		e3->sharpness=0;
		lastCrossEdge=e3;
		outerEdge->set(outerEdge->getStart(),quad,e2,e3,outerEdge->getOpposite());
		
		outerEdge=nextOuterEdge;
		}
	lastCrossEdge->setOpposite(firstCrossEdge);
	firstCrossEdge->setOpposite(lastCrossEdge);
	
	/* Delete the old face and return the inner one: */
	deleteFace(faceIt.face);
	
	return FaceIterator(innerFace);
	}

template <class PointType>
void PolygonMesh<PointType>::checkMesh(void) const
	{
	/* Check all vertices: */
	for(const Vertex* vPtr=vertices;vPtr!=0;vPtr=vPtr->succ)
		vPtr->checkVertex();
	
	/* Check all faces: */
	for(const Face* fPtr=faces;fPtr!=0;fPtr=fPtr->succ)
		fPtr->checkFace();
	}
