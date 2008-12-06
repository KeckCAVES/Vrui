/***********************************************************************
RenderPolygonMeshGL - Functions to render polygon meshes using direct
OpenGL calls.
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

#define RENDERPOLYGONMESHGL_IMPLEMENTATION

#include <queue>
#include <GL/gl.h>
#include <GL/GLNormalTemplates.h>
#include <GL/GLVertexTemplates.h>

#include "PolygonMesh.h"
#include "AutoTriangleMesh.h"

#include "RenderPolygonMeshGL.h"

template <class PointType>
void renderMeshWireframe(const PolygonMesh<PointType>& mesh,GLfloat lineWidth,GLfloat pointSize)
	{
	/* Render all faces as line loops: */
	if(lineWidth>0.0f)
		{
		glLineWidth(lineWidth);
		for(typename PolygonMesh<PointType>::ConstFaceIterator fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
			{
			glBegin(GL_LINE_LOOP);
			for(typename PolygonMesh<PointType>::ConstFaceEdgeIterator feIt=fIt.beginEdges();feIt!=fIt.endEdges();++feIt)
				glVertex<3>(feIt->getStart()->pos());
			glEnd();
			}
		}
	
	/* Render all vertices as points: */
	if(pointSize>0.0f)
		{
		glPointSize(pointSize);
		glBegin(GL_POINTS);
		for(typename PolygonMesh<PointType>::ConstVertexIterator vIt=mesh.beginVertices();vIt!=mesh.endVertices();++vIt)
			glVertex<3>(vIt->pos());
		glEnd();
		}
	}

template <class PointType>
void renderMeshTriangles(const AutoTriangleMesh<PointType>& mesh)
	{
	/* Render all faces as triangles: */
	glBegin(GL_TRIANGLES);
	for(typename AutoTriangleMesh<PointType>::ConstFaceIterator fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		{
		/* Gather triangle's points: */
		const typename AutoTriangleMesh<PointType>::Vertex* v[3];
		const typename AutoTriangleMesh<PointType>::Edge* e=fIt->getEdge();
		for(int i=0;i<3;++i)
			{
			v[i]=e->getStart();
			e=e->getFaceSucc();
			}
		
		/* Calculate triangle's normal vector: */
		float normal[3];
		planeNormal(*v[0],*v[1],*v[2],normal);
		glNormal<3>(normal);
		for(int i=0;i<3;++i)
			glVertex3(v[i]->pos());
		}
	glEnd();
	}

template <class MeshType>
static float* calcVertexNormal(const typename MeshType::Vertex* vertex,float vertexNormal[3])
	{
	/* Calculate normal vector at vertex: */
	for(int i=0;i<3;++i)
		vertexNormal[i]=0.0f;

	/* Iterate through vertex' platelet: */
	const typename MeshType::Edge* ve=vertex->getEdge();
	do
		{
		const typename MeshType::Edge* ve2=ve->getFacePred()->getOpposite();
		float triangleNormal[3];
		planeNormal(*ve->getStart(),*ve->getEnd(),*ve2->getEnd(),triangleNormal);
		for(int i=0;i<3;++i)
			vertexNormal[i]+=triangleNormal[i];

		/* Go to next edge around vertex: */
		ve=ve2;
		}
	while(ve!=vertex->getEdge());

	/* Return computed normal: */
	return vertexNormal;
	}

template <class PointType>
void renderMeshTrianglesSmooth(const AutoTriangleMesh<PointType>& mesh)
	{
	/* Render all faces as triangles: */
	int numTriangles=0;
	int numVertices=0;
	glBegin(GL_TRIANGLES);
	for(typename AutoTriangleMesh<PointType>::ConstFaceIterator fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		{
		/* Iterate through triangle's vertices: */
		const typename AutoTriangleMesh<PointType>::Edge* e=fIt->getEdge();
		for(int i=0;i<3;++i,e=e->getFaceSucc())
			{
			/* Pass normal and vertex to OpenGL: */
			float vertexNormal[3];
			glNormal3(calcVertexNormal<AutoTriangleMesh<PointType> >(e->getStart(),vertexNormal));
			glVertex3(e->getStart()->pos());
			++numVertices;
			}
		++numTriangles;
		}
	glEnd();
	std::cout<<numTriangles<<", "<<numVertices<<std::endl;
	}

template <class PointType>
inline void addNormalContribution(typename PolygonMesh<PointType>::ConstEdgeIterator edge,GLVector<typename PointType::Scalar,3>& normal)
	{
	/* Calculate the two distance vectors: */
	typename PointType::Scalar d1[3],d2[3];
	for(int i=0;i<3;++i)
		{
		d1[i]=(*edge->getEnd())[i]-(*edge->getStart())[i];
		d2[i]=(*edge->getFacePred()->getStart())[i]-(*edge->getStart())[i];
		}
	
	/* Add their cross product to the normal vector: */
	normal[0]+=d1[1]*d2[2]-d1[2]*d2[1];
	normal[1]+=d1[2]*d2[0]-d1[0]*d2[2];
	normal[2]+=d1[0]*d2[1]-d1[1]*d2[0];
	}

inline void addNormalContribution(PolygonMesh<Point<float> >::ConstEdgeIterator edge,GLVector<float,3>& normal)
	{
	/* Calculate the two distance vectors: */
	float d1[3],d2[3];
	for(int i=0;i<3;++i)
		{
		d1[i]=(*edge->getEnd())[i]-(*edge->getStart())[i];
		d2[i]=(*edge->getFacePred()->getStart())[i]-(*edge->getStart())[i];
		}
	
	/* Add their cross product to the normal vector: */
	normal[0]+=d1[1]*d2[2]-d1[2]*d2[1];
	normal[1]+=d1[2]*d2[0]-d1[0]*d2[2];
	normal[2]+=d1[0]*d2[1]-d1[1]*d2[0];
	}

template <class PointType>
void renderVertex(typename PolygonMesh<PointType>::ConstEdgeIterator edge)
	{
	/* Calculate the vertex' normal vector: */
	GLVector<typename PointType::Scalar,3> normal(0.0,0.0,0.0);
	typename PolygonMesh<PointType>::ConstEdgeIterator veIt=edge;
	do
		{
		addNormalContribution(veIt,normal);
		
		/* Go to the next edge pair in counter-clockwise direction: */
		veIt=veIt->getVertexSucc();
		if(veIt->sharpness!=0||veIt==0) // Is the vertex non-smooth?
			{
			/* Go clockwise from the initial edge until a crease is encountered: */
			while(edge->sharpness==0)
				{
				edge=edge->getVertexPred();
				if(edge==0)
					break;
				
				addNormalContribution(edge,normal);
				}
			
			break;
			}
		}
	while(veIt!=edge);
	double normalLen=sqrt(normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2]);
	normal[0]/=normalLen;
	normal[1]/=normalLen;
	normal[2]/=normalLen;
	
	/* Set the vertex' normal vector and position: */
	glNormal(normal);
	glVertex3(edge->getStart()->pos());
	}

void renderVertex(PolygonMesh<Point<float> >::ConstEdgeIterator edge)
	{
	/* Calculate the vertex' normal vector: */
	GLVector<float,3> normal(0.0,0.0,0.0);
	PolygonMesh<Point<float> >::ConstEdgeIterator veIt=edge;
	do
		{
		addNormalContribution(veIt,normal);
		
		/* Go to the next edge pair in counter-clockwise direction: */
		veIt=veIt->getVertexSucc();
		if(veIt->sharpness!=0||veIt==0) // Is the vertex non-smooth?
			{
			/* Go clockwise from the initial edge until a crease is encountered: */
			while(edge->sharpness==0)
				{
				edge=edge->getVertexPred();
				if(edge==0)
					break;
				
				addNormalContribution(edge,normal);
				}
			
			break;
			}
		}
	while(veIt!=edge);
	double normalLen=sqrt(normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2]);
	normal[0]/=normalLen;
	normal[1]/=normalLen;
	normal[2]/=normalLen;
	
	/* Set the vertex' normal vector and position: */
	glNormal(normal);
	glVertex<3>(edge->getStart()->pos());
	}

template <class PointType>
void renderMeshTriangleStrips(const PolygonMesh<PointType>& mesh)
	{
	/* Create triangle strips until all faces are visited: */
	int numFaces=0;
	for(typename PolygonMesh<PointType>::ConstFaceIterator fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		{
		fIt->visited=false;
		++numFaces;
		}
	
	/* Start generating triangle strips around the first extraordinary vertex: */
	std::queue<typename PolygonMesh<PointType>::ConstEdgeIterator> crossEdges;
	typename PolygonMesh<PointType>::ConstVertexIterator vIt=mesh.beginVertices();
	while(vIt->getNumEdges()==4)
		++vIt;
	typename PolygonMesh<PointType>::ConstVertexEdgeIterator veIt=vIt.beginEdges();
	
	/* Keep generating strips until all faces are triangulated: */
	while(numFaces>0)
		{
		/* Queue the next edge around the next extraordinary vertex: */
		if(veIt==vIt.endEdges())
			{
			/* Find the next extraordinary vertex: */
			do
				{
				++vIt;
				}
			while(vIt->getNumEdges()==4);
			veIt=vIt.beginEdges();
			}
		crossEdges.push(veIt);
		++veIt;
		
		/* Process edges from the queue until it is empty: */
		while(!crossEdges.empty())
			{
			/* Grab the next edge from the queue: */
			typename PolygonMesh<PointType>::ConstEdgeIterator edge1=crossEdges.front();
			crossEdges.pop();
			typename PolygonMesh<PointType>::ConstEdgeIterator edge2;
			if(!edge1->getFace()->visited) // Has its face not yet been visited?
				{
				/* Go backwards until an obstacle is hit: */
				edge1=edge1->getFaceSucc();
				bool even=true;
				int numBackwardsFaces=0;
				#if 1
				edge1->getFace()->visited=true;
				while(edge1->sharpness==0&&edge1->getOpposite()!=0&&!edge1->getOpposite()->getFace()->visited)
					{
					/* Go to the previos face: */
					edge2=edge1->getOpposite();
					edge1=edge2->getFaceSucc();
					while(edge2!=edge1->getFaceSucc())
						{
						if(even)
							edge2=edge2->getFacePred();
						else
							edge1=edge1->getFaceSucc();
						even=!even;
						}
					++numBackwardsFaces;
					edge1->getFace()->visited=true;
					}
				#endif

				/* Start a new triangle strip at the last visited face: */
				edge2=edge1->getFaceSucc();
				if(edge2->getOpposite()!=0)
					crossEdges.push(edge2->getOpposite());
				#if 0
				GLubyte r=GLubyte(((rand()>>12)&0x1f)+112);
				GLubyte g=GLubyte(((rand()>>12)&0x1f)+112);
				GLubyte b=GLubyte(((rand()>>12)&0x1f)+112);
				glColor3ub(r,g,b);
				#endif
				glBegin(GL_TRIANGLE_STRIP);
				if(even)
					{
					renderVertex(edge1);
					renderVertex(edge2);
					}
				else
					{
					renderVertex(edge2);
					renderVertex(edge1);
					}
				while(true)
					{
					/* Triangulate the current face: */
					while(edge1!=edge2->getFaceSucc())
						{
						if(even)
							{
							/* Advance edge1: */
							edge1=edge1->getFacePred();
							renderVertex(edge1);
							}
						else
							{
							/* Advance edge2: */
							edge2=edge2->getFaceSucc();
							renderVertex(edge2);
							}
						even=!even;
						}

					/* Mark the face visited and go to the next face: */
					--numFaces;
					if(numBackwardsFaces==0)
						{
						edge1->getFace()->visited=true;
						if(edge2->sharpness!=0||edge2->getOpposite()==0) // Did we hit a non-smooth edge?
							break;
						if(edge2->getOpposite()->getFace()->visited) // Did we hit a face already visited?
							break;
						}
					else
						--numBackwardsFaces;
					edge1=edge2->getOpposite();
					edge2=edge1->getFaceSucc();
					}
				glEnd();
				}
			}
		}
	}

void storeVertex(PolygonMesh<Point<float> >::ConstEdgeIterator edge,GLTriangleStripSet<GLVertex<void,0,void,0,GLfloat,GLfloat,3> >& triangleStripSet)
	{
	GLVertex<void,0,void,0,GLfloat,GLfloat,3> newVertex;
	
	/* Calculate the vertex' normal vector: */
	newVertex.normal=GLVector<float,3>(0.0,0.0,0.0);
	PolygonMesh<Point<float> >::ConstEdgeIterator veIt=edge;
	do
		{
		addNormalContribution(veIt,newVertex.normal);
		
		/* Go to the next edge pair in counter-clockwise direction: */
		veIt=veIt->getVertexSucc();
		if(veIt->sharpness!=0||veIt==0) // Is the vertex non-smooth?
			{
			/* Go clockwise from the initial edge until a crease is encountered: */
			while(edge->sharpness==0)
				{
				edge=edge->getVertexPred();
				if(edge==0)
					break;
				
				addNormalContribution(edge,newVertex.normal);
				}
			
			break;
			}
		}
	while(veIt!=edge);
	double normalLen=sqrt(newVertex.normal[0]*newVertex.normal[0]+newVertex.normal[1]*newVertex.normal[1]+newVertex.normal[2]*newVertex.normal[2]);
	newVertex.normal[0]/=normalLen;
	newVertex.normal[1]/=normalLen;
	newVertex.normal[2]/=normalLen;
	newVertex.position=GLVector<GLfloat,3>(edge->getStart()->pos());
	triangleStripSet.addVertex(newVertex);
	}

template <class PointType>
void renderMeshTriangleStrips(const PolygonMesh<PointType>& mesh,GLTriangleStripSet<GLVertex<void,0,void,0,GLfloat,GLfloat,3> >& triangleStripSet)
	{
	/* Create triangle strips until all faces are visited: */
	triangleStripSet.clear();
	int numFaces=0;
	for(typename PolygonMesh<PointType>::ConstFaceIterator fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		{
		fIt->visited=false;
		++numFaces;
		}
	
	/* Start generating triangle strips around the first extraordinary vertex: */
	std::queue<typename PolygonMesh<PointType>::ConstEdgeIterator> crossEdges;
	typename PolygonMesh<PointType>::ConstVertexIterator vIt=mesh.beginVertices();
	while(vIt->getNumEdges()==4)
		++vIt;
	typename PolygonMesh<PointType>::ConstVertexEdgeIterator veIt=vIt.beginEdges();
	
	/* Keep generating strips until all faces are triangulated: */
	while(numFaces>0)
		{
		/* Queue the next edge around the next extraordinary vertex: */
		if(veIt==vIt.endEdges())
			{
			/* Find the next extraordinary vertex: */
			do
				{
				++vIt;
				}
			while(vIt->getNumEdges()==4);
			veIt=vIt.beginEdges();
			}
		crossEdges.push(veIt);
		++veIt;
		
		/* Process edges from the queue until it is empty: */
		while(!crossEdges.empty())
			{
			/* Grab the next edge from the queue: */
			typename PolygonMesh<PointType>::ConstEdgeIterator edge1=crossEdges.front();
			crossEdges.pop();
			typename PolygonMesh<PointType>::ConstEdgeIterator edge2;
			if(!edge1->getFace()->visited) // Has its face not yet been visited?
				{
				/* Go backwards until an obstacle is hit: */
				edge1=edge1->getFaceSucc();
				bool even=true;
				int numBackwardsFaces=0;
				edge1->getFace()->visited=true;
				while(edge1->sharpness==0&&edge1->getOpposite()!=0&&!edge1->getOpposite()->getFace()->visited)
					{
					/* Go to the previos face: */
					edge2=edge1->getOpposite();
					edge1=edge2->getFaceSucc();
					while(edge2!=edge1->getFaceSucc())
						{
						if(even)
							edge2=edge2->getFacePred();
						else
							edge1=edge1->getFaceSucc();
						even=!even;
						}
					++numBackwardsFaces;
					edge1->getFace()->visited=true;
					}

				/* Start a new triangle strip at the last visited face: */
				edge2=edge1->getFaceSucc();
				if(edge2->getOpposite()!=0)
					crossEdges.push(edge2->getOpposite());
				triangleStripSet.beginStrip();
				if(even)
					{
					storeVertex(edge1,triangleStripSet);
					storeVertex(edge2,triangleStripSet);
					}
				else
					{
					storeVertex(edge2,triangleStripSet);
					storeVertex(edge1,triangleStripSet);
					}
				while(true)
					{
					/* Triangulate the current face: */
					while(edge1!=edge2->getFaceSucc())
						{
						if(even)
							{
							/* Advance edge1: */
							edge1=edge1->getFacePred();
							storeVertex(edge1,triangleStripSet);
							}
						else
							{
							/* Advance edge2: */
							edge2=edge2->getFaceSucc();
							storeVertex(edge2,triangleStripSet);
							}
						even=!even;
						}

					/* Mark the face visited and go to the next face: */
					--numFaces;
					if(numBackwardsFaces==0)
						{
						edge1->getFace()->visited=true;
						if(edge2->sharpness!=0||edge2->getOpposite()==0) // Did we hit a non-smooth edge?
							break;
						if(edge2->getOpposite()->getFace()->visited) // Did we hit a face already visited?
							break;
						}
					else
						--numBackwardsFaces;
					edge1=edge2->getOpposite();
					edge2=edge1->getFaceSucc();
					}
				triangleStripSet.endStrip();
				}
			}
		}
	
	/* Finalize the created triangle strip set: */
	triangleStripSet.finalize();
	}

template <class PointType>
void renderMeshTriangleStripsOnTheFly(const AutoTriangleMesh<PointType>& mesh,int maxNumStrips)
	{
	srand(0);
	
	/* Mark all faces as non-visited: */
	for(typename AutoTriangleMesh<PointType>::ConstFaceIterator fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		fIt->visited=false;
	
	/* Traverse the list of faces and start a triangle strip from each unvisited face: */
	int numStrips=0;
	int numTriangles=0;
	int numVertices=0;
	for(typename AutoTriangleMesh<PointType>::ConstFaceIterator fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		{
		if(!fIt->visited)
			{
			/* Find an edge that crosses into another unvisited triangle: */
			const typename AutoTriangleMesh<PointType>::Edge* crossEdge=fIt->getEdge();
			for(typename AutoTriangleMesh<PointType>::ConstFaceEdgeIterator feIt=fIt.beginEdges();feIt!=fIt.endEdges();++feIt)
				if(feIt->getOpposite()!=0&&!feIt->getOpposite()->getFace()->visited)
					crossEdge=&*feIt;
			
			GLubyte r=GLubyte(((rand()>>12)&0x1f)+112);
			GLubyte g=GLubyte(((rand()>>12)&0x1f)+112);
			GLubyte b=GLubyte(((rand()>>12)&0x1f)+112);
			glColor3ub(r,g,b);
			
			/* Start a triangle strip: */
			float vertexNormal[3];
			glBegin(GL_TRIANGLE_STRIP);
			crossEdge->getFace()->visited=true;
			glNormal3(calcVertexNormal<AutoTriangleMesh<PointType> >(crossEdge->getFacePred()->getStart(),vertexNormal));
			glVertex3(crossEdge->getFacePred()->getStart()->pos());
			++numVertices;
			glNormal3(calcVertexNormal<AutoTriangleMesh<PointType> >(crossEdge->getStart(),vertexNormal));
			glVertex3(crossEdge->getStart()->pos());
			++numVertices;
			glNormal3(calcVertexNormal<AutoTriangleMesh<PointType> >(crossEdge->getEnd(),vertexNormal));
			glVertex3(crossEdge->getEnd()->pos());
			++numVertices;
			++numTriangles;
			crossEdge=crossEdge->getOpposite();
			bool crossLeft=true;
			
			/* Continue the triangle strip: */
			while(true)
				{
				crossEdge->getFace()->visited=true;
				const typename AutoTriangleMesh<PointType>::Edge* nextCrossEdge;
				if(crossLeft)
					{
					/* Check whether we should go straight, take a turn, or stop: */
					nextCrossEdge=crossEdge->getFacePred()->getOpposite();
					if(nextCrossEdge!=0&&!nextCrossEdge->getFace()->visited)
						{
						/* Go straight: */
						glNormal3(calcVertexNormal<AutoTriangleMesh<PointType> >(crossEdge->getFacePred()->getStart(),vertexNormal));
						glVertex3(crossEdge->getFacePred()->getStart()->pos());
						++numVertices;
						++numTriangles;
						crossEdge=nextCrossEdge;
						crossLeft=false;
						}
					else
						{
						nextCrossEdge=crossEdge->getFaceSucc()->getOpposite();
						if(nextCrossEdge!=0&&!nextCrossEdge->getFace()->visited)
							{
							/* Take a turn: */
							glNormal3(calcVertexNormal<AutoTriangleMesh<PointType> >(crossEdge->getEnd(),vertexNormal));
							glVertex3(crossEdge->getEnd()->pos());
							++numVertices;
							glNormal3(calcVertexNormal<AutoTriangleMesh<PointType> >(crossEdge->getFacePred()->getStart(),vertexNormal));
							glVertex3(crossEdge->getFacePred()->getStart()->pos());
							++numVertices;
							++numTriangles;
							crossEdge=nextCrossEdge;
							}
						else
							{
							/* Finish this triangle and stop: */
							glNormal3(calcVertexNormal<AutoTriangleMesh<PointType> >(crossEdge->getFacePred()->getStart(),vertexNormal));
							glVertex3(crossEdge->getFacePred()->getStart()->pos());
							++numVertices;
							++numTriangles;
							break;
							}
						}
					}
				else
					{
					/* Check whether we should go straight, take a turn, or stop: */
					nextCrossEdge=crossEdge->getFaceSucc()->getOpposite();
					if(nextCrossEdge!=0&&!nextCrossEdge->getFace()->visited)
						{
						/* Go straight: */
						glNormal3(calcVertexNormal<AutoTriangleMesh<PointType> >(crossEdge->getFaceSucc()->getEnd(),vertexNormal));
						glVertex3(crossEdge->getFaceSucc()->getEnd()->pos());
						++numVertices;
						++numTriangles;
						crossEdge=nextCrossEdge;
						crossLeft=true;
						}
					else
						{
						nextCrossEdge=crossEdge->getFacePred()->getOpposite();
						if(nextCrossEdge!=0&&!nextCrossEdge->getFace()->visited)
							{
							/* Take a turn: */
							glNormal3(calcVertexNormal<AutoTriangleMesh<PointType> >(crossEdge->getStart(),vertexNormal));
							glVertex3(crossEdge->getStart()->pos());
							++numVertices;
							glNormal3(calcVertexNormal<AutoTriangleMesh<PointType> >(crossEdge->getFaceSucc()->getEnd(),vertexNormal));
							glVertex3(crossEdge->getFaceSucc()->getEnd()->pos());
							++numVertices;
							++numTriangles;
							crossEdge=nextCrossEdge;
							}
						else
							{
							/* Finish this triangle and stop: */
							glNormal3(calcVertexNormal<AutoTriangleMesh<PointType> >(crossEdge->getFaceSucc()->getEnd(),vertexNormal));
							glVertex3(crossEdge->getFaceSucc()->getEnd()->pos());
							++numVertices;
							++numTriangles;
							break;
							}
						}
					}
				}
			glEnd();
			++numStrips;
			if(numStrips==maxNumStrips)
				break;
			}
		}
	std::cout<<numStrips<<", "<<numTriangles<<", "<<numVertices<<std::endl;
	}
