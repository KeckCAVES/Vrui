/***********************************************************************
CatmullClark - Functions to perform Catmull-Clark subdivision on polygon
meshes.
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

#define CATMULLCLARK_IMPLEMENTATION

#include <utility>
#include <Misc/HashTable.h>

#include "CatmullClark.h"

template <class PointType>
PolygonMesh<PointType>& subdividePolyhedron(PolygonMesh<PointType>& mesh)
	{
	typename PolygonMesh<PointType>::FaceIterator fIt;
	typename PolygonMesh<PointType>::FaceEdgeIterator feIt;
	
	typedef Misc::HashTable<typename PolygonMesh<PointType>::VertexPair,std::pair<PointType,typename PolygonMesh<PointType>::EdgeIterator>,typename PolygonMesh<PointType>::VertexPair> EdgeHashTable;
	typedef Misc::HashTable<typename PolygonMesh<PointType>::FaceIterator,PointType,typename PolygonMesh<PointType>::FaceIterator> FaceHashTable;
	
	typename EdgeHashTable::Iterator epIt;
	typename FaceHashTable::Iterator fpIt;
	
	/* Calculate all face points and associate them with the original faces: */
	FaceHashTable facePoints((mesh.getNumFaces()*3)/2);
	int numEdges=0;
	for(fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		{
		/* Average all the face's vertices to calculate the face point: */
		PointType facePoint=PointType::zero();
		int numVertices=0;
		for(feIt=fIt.beginEdges();feIt!=fIt.endEdges();++feIt,++numVertices)
			facePoint.add(*feIt->getStart());
		facePoint.normalize(numVertices);
		
		/* Associate the face point with the face: */
		facePoints.setEntry(FaceHashTable::Entry(fIt,facePoint));
		
		/* Keep track of the total number of edges in the mesh: */
		numEdges+=numVertices;
		}
	
	/* Calculate all edge midpoints and associate them with the original edges: */
	EdgeHashTable edgePoints(numEdges);
	for(fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		{
		for(feIt=fIt.beginEdges();feIt!=fIt.endEdges();++feIt)
			{
			typename PolygonMesh<PointType>::VertexPair vp=feIt.getVertexPair();
			if(!edgePoints.isEntry(vp)) // Did we already create an edge point for this edge?
				{
				/* Calculate the edge midpoint: */
				PointType midPoint=PointType::zero();
				midPoint.add(*feIt->getStart());
				midPoint.add(*feIt->getEnd());
				midPoint.normalize(2);
				
				/* Associate the edge midpoint with the edge: */
				edgePoints.setEntry(typename EdgeHashTable::Entry(vp,std::pair<PointType,typename PolygonMesh<PointType>::EdgeIterator>(midPoint,feIt)));
				}
			}
		}
	
	/* Now insert all edge points into the mesh: */
	for(epIt=edgePoints.begin();!epIt.isFinished();++epIt)
		{
		/* Insert the edge point into the mesh by splitting the edge it is associated with: */
		mesh.splitEdge(epIt->getDest().second,epIt->getDest().first);
		}
	
	/* Now insert all face points into the mesh, splitting all faces into quad fans: */
	for(fpIt=facePoints.begin();!fpIt.isFinished();++fpIt)
		mesh.splitFaceCatmullClark(fpIt->getSource(),fpIt->getDest());
	
	return mesh;
	}

template <class PointType>
PolygonMesh<PointType>& subdividePolyhedronWithLists(PolygonMesh<PointType>& mesh,std::list<typename PolygonMesh<PointType>::VertexIterator>& vertexPointList,std::list<typename PolygonMesh<PointType>::VertexIterator>& facePointList)
	{
	typename PolygonMesh<PointType>::FaceIterator fIt;
	typename PolygonMesh<PointType>::FaceEdgeIterator feIt;
	
	typedef Misc::HashTable<typename PolygonMesh<PointType>::VertexPair,std::pair<PointType,typename PolygonMesh<PointType>::EdgeIterator>,typename PolygonMesh<PointType>::VertexPair> EdgeHashTable;
	typedef Misc::HashTable<typename PolygonMesh<PointType>::FaceIterator,PointType,typename PolygonMesh<PointType>::FaceIterator> FaceHashTable;
	
	typename EdgeHashTable::Iterator epIt;
	typename FaceHashTable::Iterator fpIt;
	
	/* Calculate all face points and associate them with the original faces: */
	FaceHashTable facePoints((mesh.getNumFaces()*3)/2);
	int numEdges=0;
	for(fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		{
		/* Average all the face's vertices to calculate the face point: */
		PointType facePoint=PointType::zero();
		int numVertices=0;
		for(feIt=fIt.beginEdges();feIt!=fIt.endEdges();++feIt,++numVertices)
			facePoint.add(*feIt->getStart());
		facePoint.normalize(numVertices);
		
		/* Associate the face point with the face: */
		facePoints.setEntry(FaceHashTable::Entry(fIt,facePoint));
		
		/* Keep track of the total number of edges in the mesh: */
		numEdges+=numVertices;
		}
	
	/* Calculate all edge midpoints and associate them with the original edges: */
	EdgeHashTable edgePoints(numEdges);
	for(fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		{
		for(feIt=fIt.beginEdges();feIt!=fIt.endEdges();++feIt)
			{
			typename PolygonMesh<PointType>::VertexPair vp=feIt.getVertexPair();
			if(!edgePoints.isEntry(vp)) // Did we already create an edge point for this edge?
				{
				/* Calculate the edge midpoint: */
				PointType midPoint=PointType::zero();
				midPoint.add(*feIt->getStart());
				midPoint.add(*feIt->getEnd());
				midPoint.normalize(2);
				
				/* Associate the edge midpoint with the edge: */
				edgePoints.setEntry(typename EdgeHashTable::Entry(vp,std::pair<PointType,typename PolygonMesh<PointType>::EdgeIterator>(midPoint,feIt)));
				}
			}
		}
	
	/* Create a list of vertex points: */
	vertexPointList.clear();
	for(typename PolygonMesh<PointType>::VertexIterator vIt=mesh.beginVertices();vIt!=mesh.endVertices();++vIt)
		vertexPointList.push_back(vIt);
	
	/* Now insert all edge points into the mesh: */
	for(epIt=edgePoints.begin();!epIt.isFinished();++epIt)
		{
		/* Insert the edge point into the mesh by splitting the edge it is associated with: */
		mesh.splitEdge(epIt->getDest().second,epIt->getDest().first);
		}
	
	/* Now insert all face points into the mesh, splitting all faces into quad fans: */
	facePointList.clear();
	for(fpIt=facePoints.begin();!fpIt.isFinished();++fpIt)
		facePointList.push_back(mesh.splitFaceCatmullClark(fpIt->getSource(),fpIt->getDest()));
	
	return mesh;
	}

template <class PointType>
PolygonMesh<PointType>& liftingStep(PolygonMesh<PointType>& mesh,std::list<typename PolygonMesh<PointType>::VertexIterator>& vertices,double a,double b)
	{
	typename PolygonMesh<PointType>::VertexEdgeIterator veIt;
	
	typedef Misc::HashTable<typename PolygonMesh<PointType>::VertexIterator,double,typename PolygonMesh<PointType>::VertexIterator> VertexHashTable;
	
	VertexHashTable edgePoints(mesh.getNumVertices());
	
	/* Perform a lifting step for all supplied vertices: */
	for(typename std::list<typename PolygonMesh<PointType>::VertexIterator>::iterator vIt=vertices.begin();vIt!=vertices.end();++vIt)
		{
		/* Average the surrounding face and edge points: */
		double fAvg=0.0;
		// PointType fAvg=PointType::zero();
		double eAvg=0.0;
		// PointType eAvg=PointType::zero();
		int numFaces=0;
		for(veIt=vIt->beginEdges();veIt!=vIt->endEdges();++veIt,++numFaces)
			{
			typename PolygonMesh<PointType>::VertexIterator edgePointIt=veIt->getEnd();
			typename PolygonMesh<PointType>::VertexIterator facePoint1It=veIt->getOpposite()->getFacePred()->getStart();
			typename PolygonMesh<PointType>::VertexIterator facePoint2It=veIt->getFaceSucc()->getEnd();
			if(!edgePoints.isEntry(edgePointIt))
				{
				/* Calculate the new edge point: */
				double e=0.0;
				// PointType e=PointType::zero();
				e+=(*edgePointIt)[2]*b;
				// e.add(*edgePointIt,b);
				e+=(*facePoint1It)[2]*a;
				// e.add(*facePoint1It,a);
				e+=(*facePoint2It)[2]*a;
				// e.add(*facePoint2It,a);
				// e/=b+2.0*a;
				// e.normalize(b+2.0*a);
				edgePoints.setEntry(typename VertexHashTable::Entry(edgePointIt,e));
				}
			eAvg+=(*edgePointIt)[2];
			// eAvg.add(*edgePointIt);
			fAvg+=(*facePoint2It)[2];
			// fAvg.add(*facePoint2It);
			}
		fAvg/=numFaces;
		// fAvg.normalize(numFaces);
		eAvg/=numFaces;
		// eAvg.normalize(numFaces);
		
		/* Calculate the vertex' new position: */
		double v=0.0;
		// PointType v=PointType::zero();
		v+=(**vIt)[2]*b*b;
		// v.add(**vIt,b*b);
		v+=eAvg*4.0*a*b;
		// v.add(eAvg,4.0*a*b);
		v+=fAvg*4.0*a*a;
		// v.add(fAvg,4.0*a*a);
		// v/=b*b+4.0*a*b+4.0*a*a;
		// v.normalize(b*b+4.0*a*b+4.0*a*a);
		(**vIt)[2]=v;
		// (*vIt)->setPoint(v);
		}
	
	/* Set the positions of all edge points: */
	for(typename Misc::HashTable<typename PolygonMesh<PointType>::VertexIterator,double,typename PolygonMesh<PointType>::VertexIterator>::Iterator eIt=edgePoints.begin();!eIt.isFinished();++eIt)
		{
		(*eIt->getSource())[2]=eIt->getDest();
		// eIt->getSource()->setPoint(eIt->getDest());
		}
	
	return mesh;
	}

#if 0

/* Catmull-Clark using hash tables to associate face and edge points: */

template <class PointType>
PolygonMesh<PointType>& subdivideCatmullClark(PolygonMesh<PointType>& mesh)
	{
	typename PolygonMesh<PointType>::FaceIterator fIt;
	typename PolygonMesh<PointType>::FaceEdgeIterator feIt;
	Misc::HashTable<typename PolygonMesh<PointType>::VertexPair,std::pair<PointType,typename PolygonMesh<PointType>::EdgeIterator>,typename PolygonMesh<PointType>::VertexPair>::Iterator epIt;
	Misc::HashTable<typename PolygonMesh<PointType>::FaceIterator,PointType,typename PolygonMesh<PointType>::FaceIterator>::Iterator fpIt;
	
	/* Calculate all face points and associate them with the original faces: */
	Misc::HashTable<typename PolygonMesh<PointType>::FaceIterator,PointType,typename PolygonMesh<PointType>::FaceIterator> facePoints((mesh.getNumFaces()*3)/2);
	int numEdges=0;
	for(fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		{
		/* Average all the face's vertices to calculate the face point: */
		PointType facePoint=PointType::zero();
		int numVertices=0;
		for(feIt=fIt.beginEdges();feIt!=fIt.endEdges();++feIt,++numVertices)
			facePoint.add(*feIt->getStart());
		facePoint.normalize(numVertices);
		
		/* Associate the face point with the face: */
		facePoints.setEntry(fIt,facePoint);
		
		/* Keep track of the total number of edges in the mesh: */
		numEdges+=numVertices;
		}
	
	/* Calculate all edge midpoints and associate them with the original edges: */
	Misc::HashTable<typename PolygonMesh<PointType>::VertexPair,std::pair<PointType,typename PolygonMesh<PointType>::EdgeIterator>,typename PolygonMesh<PointType>::VertexPair> edgePoints(numEdges);
	for(fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		{
		for(feIt=fIt.beginEdges();feIt!=fIt.endEdges();++feIt)
			{
			typename PolygonMesh<PointType>::VertexPair vp=feIt.getVertexPair();
			if(!edgePoints.isEntry(vp)) // Did we already create an edge point for this edge?
				{
				/* Calculate the edge midpoint: */
				PointType midPoint=PointType::zero();
				midPoint.add(*feIt->getStart());
				midPoint.add(*feIt->getEnd());
				midPoint.normalize(2);
				
				/* Associate the edge midpoint with the edge: */
				edgePoints.setEntry(vp,std::pair<PointType,typename PolygonMesh<PointType>::EdgeIterator>(midPoint,feIt));
				}
			}
		}
	
	/* Now adjust all vertices to be the new vertex points: */
	for(typename PolygonMesh<PointType>::VertexIterator vIt=mesh.beginVertices();vIt!=mesh.endVertices();++vIt)
		{
		PointType vertexPoint=PointType::zero();
		int numEdges=0;
		int numSharpEdges=0;
		typename PolygonMesh<PointType>::EdgeIterator sharpEdges[2];
		for(typename PolygonMesh<PointType>::VertexEdgeIterator veIt=vIt.beginEdges();veIt!=vIt.endEdges();++veIt,++numEdges)
			{
			/* Add the edge's midpoint and the next face's face point to the vertex point: */
			vertexPoint.add(facePoints.getEntry(typename PolygonMesh<PointType>::FaceIterator(veIt->getFace())));
			vertexPoint.add(edgePoints.getEntry(veIt.getVertexPair()).first,2);
			if(veIt->sharpness!=0)
				{
				if(numSharpEdges<2)
					sharpEdges[numSharpEdges]=veIt;
				++numSharpEdges;
				}
			}
		
		if(numSharpEdges<2)
			{
			/* Add the original vertex to the vertex point and normalize: */
			vertexPoint.add(*vIt,numEdges*(numEdges-3));
			vertexPoint.normalize(numEdges*numEdges);
			vIt->setPoint(vertexPoint);
			}
		else if(numSharpEdges==2)
			{
			/* Forget what we calculated, use the crease vertex rule: */
			vertexPoint=PointType::zero();
			vertexPoint.add(*vIt,2);
			vertexPoint.add(edgePoints.getEntry(sharpEdges[0].getVertexPair()).first);
			vertexPoint.add(edgePoints.getEntry(sharpEdges[1].getVertexPair()).first);
			vertexPoint.normalize(4);
			vIt->setPoint(vertexPoint);
			}
		}
	
	/* Now adjust all edge midpoints to be the new edge points: */
	for(epIt=edgePoints.begin();!epIt.isFinished();++epIt)
		{
		/* Add the edge midpoint and the two face points to the edge point: */
		typename PolygonMesh<PointType>::EdgeIterator edgeIt=epIt.getDest().second;
		if(edgeIt->sharpness==0)
			{
			PointType edgePoint=PointType::zero();
			edgePoint.add(epIt.getDest().first,2);
			edgePoint.add(facePoints.getEntry(edgeIt->getFace()));
			edgePoint.add(facePoints.getEntry(edgeIt->getOpposite()->getFace()));
			edgePoint.normalize(4);
			epIt.getDest().first=edgePoint;
			}
		else if(edgeIt->sharpness>0)
			{
			--edgeIt->sharpness;
			--edgeIt->getOpposite()->sharpness;
			}
		}
	
	/* Now insert all edge points into the mesh: */
	for(epIt=edgePoints.begin();!epIt.isFinished();++epIt)
		{
		/* Insert the edge point into the mesh by splitting the edge it is associated with: */
		mesh.splitEdge(epIt->getDest().second,epIt->getDest().first);
		}
	
	/* Now insert all face points into the mesh, splitting all faces into quad fans: */
	for(fpIt=facePoints.begin();!fpIt.isFinished();++fpIt)
		mesh.splitFaceCatmullClark(fpIt->getSource(),fpIt->getDest());
	
	return mesh;
	}

#else

/* Catmull-Clark using pointers to associate face and edge points: */

template <class PointType>
PolygonMesh<PointType>& subdivideCatmullClark(PolygonMesh<PointType>& mesh)
	{
	typename PolygonMesh<PointType>::FaceIterator fIt;
	typename PolygonMesh<PointType>::FaceEdgeIterator feIt;
	
	/* Calculate all face points and associate them with the original faces: */
	typename PolygonMesh<PointType>::Vertex* facePoints=0;
	for(fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		{
		/* Average all the face's vertices to calculate the face point: */
		PointType facePoint=PointType::zero();
		int numVertices=0;
		for(feIt=fIt.beginEdges();feIt!=fIt.endEdges();++feIt,++numVertices)
			facePoint.add(*feIt->getStart());
		facePoint.normalize(numVertices);
		
		/* Associate the face point with the face: */
		facePoints=mesh.createVertex(facePoint,facePoints);
		facePoints->setEdge(fIt->getEdge());
		fIt->facePoint=facePoints;
		
		#if 0
		/* Reset the edge point pointers of all the face's edges: */
		for(feIt=fIt.beginEdges();feIt!=fIt.endEdges();++feIt)
			feIt->edgePoint=0;
		#endif
		}
	
	/* Calculate all edge midpoints and associate them with the original edges: */
	typename PolygonMesh<PointType>::Vertex* edgePoints=0;
	for(fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		{
		for(feIt=fIt.beginEdges();feIt!=fIt.endEdges();++feIt)
			{
			// if(feIt->edgePoint==0) // Did we already create an edge point for this edge?
			if(feIt.isUpperHalf()) // Only process "dominant" half edges
				{
				/* Calculate the edge midpoint: */
				PointType midPoint=PointType::zero();
				midPoint.add(*feIt->getStart());
				midPoint.add(*feIt->getEnd());
				midPoint.normalize(2);
				
				/* Associate the edge midpoint with both half edges: */
				edgePoints=mesh.createVertex(midPoint,edgePoints);
				edgePoints->setEdge(&(*feIt));
				feIt->edgePoint=feIt->getOpposite()->edgePoint=edgePoints;
				}
			}
		}
	
	/* Now adjust all vertices to be the new vertex points: */
	for(typename PolygonMesh<PointType>::VertexIterator vIt=mesh.beginVertices();vIt!=mesh.endVertices();++vIt)
		{
		PointType vertexPoint=PointType::zero();
		int numEdges=0;
		int numSharpEdges=0;
		typename PolygonMesh<PointType>::EdgeIterator sharpEdges[2];
		for(typename PolygonMesh<PointType>::VertexEdgeIterator veIt=vIt.beginEdges();veIt!=vIt.endEdges();++veIt,++numEdges)
			{
			/* Add the edge's midpoint and the next face's face point to the vertex point: */
			vertexPoint.add(*veIt->getFace()->facePoint);
			vertexPoint.add(*veIt->edgePoint,2);
			if(veIt->sharpness!=0)
				{
				if(numSharpEdges<2)
					sharpEdges[numSharpEdges]=veIt;
				++numSharpEdges;
				}
			}
		
		if(numSharpEdges<2)
			{
			/* Add the original vertex to the vertex point and normalize: */
			vertexPoint.add(*vIt,numEdges*(numEdges-3));
			vertexPoint.normalize(numEdges*numEdges);
			vIt->setPoint(vertexPoint);
			}
		else if(numSharpEdges==2)
			{
			/* Forget what we calculated, use the crease vertex rule: */
			vertexPoint=PointType::zero();
			vertexPoint.add(*vIt,2);
			vertexPoint.add(*sharpEdges[0]->edgePoint);
			vertexPoint.add(*sharpEdges[1]->edgePoint);
			vertexPoint.normalize(4);
			vIt->setPoint(vertexPoint);
			}
		}
	
	/* Now adjust all edge midpoints to be the new edge points: */
	for(typename PolygonMesh<PointType>::Vertex* epIt=edgePoints;epIt!=0;epIt=epIt->getSucc())
		{
		typename PolygonMesh<PointType>::Edge* edge=epIt->getEdge();
		if(edge->sharpness==0)
			{
			PointType edgePoint=PointType::zero();
			edgePoint.add(*epIt,2);
			edgePoint.add(*edge->getFace()->facePoint);
			edgePoint.add(*edge->getOpposite()->getFace()->facePoint);
			edgePoint.normalize(4);
			epIt->setPoint(edgePoint);
			}
		else if(edge->sharpness>0)
			{
			--edge->sharpness;
			--edge->getOpposite()->sharpness;
			}
		}
	
	/* Now insert all edge points into the mesh: */
	for(typename PolygonMesh<PointType>::Vertex* epIt=edgePoints;epIt!=0;)
		{
		typename PolygonMesh<PointType>::Vertex* nextEpIt=epIt->getSucc();
		
		/* Insert the edge point into the mesh by splitting the edge it is associated with: */
		mesh.splitEdge(epIt->getEdge(),epIt);
		
		/* Go to the next edge point: */
		epIt=nextEpIt;
		}
	
	/* Now insert all face points into the mesh, splitting all faces into quad fans: */
	for(typename PolygonMesh<PointType>::Vertex* fpIt=facePoints;fpIt!=0;)
		{
		typename PolygonMesh<PointType>::Vertex* nextFpIt=fpIt->getSucc();
		
		/* Insert the face point into the mesh by splitting the face it is associated with into a quad fan: */
		mesh.splitFaceCatmullClark(fpIt->getEdge()->getFace(),fpIt);
		
		/* Go to the next face point: */
		fpIt=nextFpIt;
		}
	
	return mesh;
	}

#endif

template <class PointType>
PolygonMesh<PointType>& snapCatmullClark(PolygonMesh<PointType>& mesh)
	{
	static const int denominators[]={0,6,14,24,36,50,66,84,104,126,150};
	typename PolygonMesh<PointType>::VertexIterator vIt;
	
	/* Calculate all new vertex points and associate them with the existing vertex points: */
	typedef Misc::HashTable<typename PolygonMesh<PointType>::VertexIterator,PointType,typename PolygonMesh<PointType>::VertexIterator> VertexHashTable;
	VertexHashTable vertexPoints((mesh.getNumVertices()*3)/2);
	for(vIt=mesh.beginVertices();vIt!=mesh.endVertices();++vIt)
		{
		int valence=0;
		PointType vertexPoint=PointType::zero();
		for(typename PolygonMesh<PointType>::VertexEdgeIterator veIt=vIt.beginEdges();veIt!=vIt.endEdges();++veIt,++valence)
			{
			vertexPoint.add(*veIt->getEnd(),4);
			vertexPoint.add(*veIt->getFaceSucc()->getEnd());
			}
		vertexPoint.add(*vIt,denominators[valence]-5*valence);
		vertexPoint.normalize(denominators[valence]);
		vertexPoints.setEntry(typename VertexHashTable::Entry(vIt,vertexPoint));
		}
	
	/* Now set all vertices to their new positions: */
	for(vIt=mesh.beginVertices();vIt!=mesh.endVertices();++vIt)
		vIt->setPoint(vertexPoints.getEntry(vIt).getDest());
	
	return mesh;
	}
