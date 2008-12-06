/***********************************************************************
Influence - Class to encapsulate influence shapes and modification
actions.
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

#include <vector>
#include <GL/gl.h>
#include <GL/GLModels.h>
#include <GL/GLTransformationWrappers.h>

#include "Point.h"
#include "AutoTriangleMesh.h"

#include "Influence.h"

/************************************
Methods of class Influence::DataItem:
************************************/

Influence::DataItem::DataItem(void)
	:displayListId(glGenLists(1))
	{
	}

Influence::DataItem::~DataItem(void)
	{
	glDeleteLists(displayListId,1);
	}

/**************************
Methods of class Influence:
**************************/

void Influence::initContext(GLContextData& contextData) const
	{
	/* Create a context data item and add it to the context data: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Render the influence model: */
	glNewList(dataItem->displayListId,GL_COMPILE);
	glPushAttrib(GL_COLOR_BUFFER_BIT|GL_ENABLE_BIT|GL_LINE_BIT|GL_POLYGON_BIT);
	#if 0
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	glLineWidth(1.0f);
	glColor3f(1.0f,1.0f,1.0f);
	glDrawSphereIcosahedron(1.0f,5);
	#else
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	glLineWidth(1.0f);
	glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	glColor4f(0.0f,1.0f,0.0f,0.33f);
	glDrawSphereIcosahedron(1.0f,5);
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glColor4f(0.1f,0.5f,0.1f,0.33f);
	glDrawSphereIcosahedron(1.0f,5);
	glDepthMask(GL_TRUE);
	#endif
	glPopAttrib();
	glEndList();
	}

void Influence::setPositionOrientation(const Influence::ONTransform& newTransformation)
	{
	/* Calculate velocities, i.e., distances to old position and orientation: */
	linearVelocity=newTransformation.getTranslation()-transformation.getTranslation();
	angularVelocity=(newTransformation.getRotation()*Geometry::invert(transformation.getRotation())).getScaledAxis();
	
	/* Set new position and orientation: */
	transformation=newTransformation;
	}

void Influence::glRenderAction(GLContextData& contextData) const
	{
	/* Get a pointer to the context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	glPushMatrix();
	
	/* Translate coordinate system to influence's position and orientation: */
	glMultMatrix(transformation);
	glScaled(radius,radius,radius);
	
	/* Draw the influence sphere: */
	glCallList(dataItem->displayListId);
	
	glPopMatrix();
	}

void Influence::actOnMesh(Influence::Mesh& mesh) const
	{
	/* Limit mesh's triangle edge lengths inside region of influence: */
	Mesh::BasePoint center(transformation.getOrigin().getComponents());
	mesh.limitEdgeLength(center,radius,radius*0.1);
	mesh.ensureEdgeLength(center,radius,radius*0.03);
	
	/* Perform influence's action: */
	switch(action)
		{
		case EXPLODE:
			for(Mesh::VertexIterator vIt=mesh.beginVertices();vIt!=mesh.endVertices();++vIt)
				{
				Vector r;
				for(int i=0;i<3;++i)
					r[i]=(*vIt)[i]-center[i];
				double dist2=Geometry::sqr(r);
				if(dist2>0.0&&dist2<=radius2)
					{
					double dist=Math::sqrt(dist2);
					double factor=((radius-dist)*pressureFunction(dist/radius))/dist;
					for(int i=0;i<3;++i)
						(*vIt)[i]+=r[i]*factor;
					}
				}
			break;
		
		case DRAG:
			for(Mesh::VertexIterator vIt=mesh.beginVertices();vIt!=mesh.endVertices();++vIt)
				{
				double dist2=sqrDist(*vIt,center)/radius2;
				if(dist2<=1.0)
					{
					/* Move this vertex: */
					double factor=pressureFunction(sqrt(dist2));
					Vector r;
					for(int i=0;i<3;++i)
						r[i]=(*vIt)[i]-center[i];
					Vector displacement=linearVelocity+Geometry::cross(angularVelocity,r);
					for(int i=0;i<3;++i)
						(*vIt)[i]+=displacement[i]*factor;
					}
				}
			break;
		
		case WHITTLE:
			{
			std::vector<VertexMotion> verts;
			
			/* Apply fairing operation to vertices: */
			for(Mesh::VertexIterator vIt=mesh.beginVertices();vIt!=mesh.endVertices();++vIt)
				{
				double dist2=sqrDist(*vIt,center)/radius2;
				if(dist2<=1.0)
					{
					/* Calculate average position for this vertex: */
					Mesh::Point vAvg=Mesh::Point::zero();
					float weightSum=0.0f;
					const Mesh::Edge* e=vIt->getEdge();
					do
						{
						float weight=1.0f; // +float(sqrt(sqrDist(*e->getEnd(),*vIt)));
						vAvg.add(*e->getEnd(),weight);
						weightSum+=weight;
						e=e->getVertexSucc();
						}
					while(e!=vIt->getEdge());
					vAvg.normalize(weightSum);
					
					/* Move this vertex towards the average: */
					double dist=Math::sqrt(dist2);
					double factor=pressureFunction(dist);
					float offset[3];
					for(int i=0;i<3;++i)
						offset[i]=(vAvg[i]-(*vIt)[i])*factor;
					verts.push_back(VertexMotion(vIt,offset));
					}
				}

			for(std::vector<VertexMotion>::const_iterator vIt=verts.begin();vIt!=verts.end();++vIt)
				{
				for(int i=0;i<3;++i)
					(*vIt->vIt)[i]+=vIt->vec[i];
				}
			break;
			}
		}
	}
