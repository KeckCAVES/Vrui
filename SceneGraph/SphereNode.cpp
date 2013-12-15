/***********************************************************************
SphereNode - Class for spheres as renderable geometry.
Copyright (c) 2013 Oliver Kreylos

This file is part of the Simple Scene Graph Renderer (SceneGraph).

The Simple Scene Graph Renderer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Simple Scene Graph Renderer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Simple Scene Graph Renderer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <SceneGraph/SphereNode.h>

#include <string.h>
#include <GL/gl.h>
#include <Math/Math.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/***************************
Methods of class SphereNode:
***************************/

void SphereNode::createList(GLContextData& renderState) const
	{
	const GLfloat pi=Math::Constants<GLfloat>::pi;
	const GLfloat ns=GLfloat(numSegments.getValue());
	const GLfloat nq=GLfloat(numSegments.getValue()*2);
	const GLfloat cx=center.getValue()[0];
	const GLfloat cy=center.getValue()[0];
	const GLfloat cz=center.getValue()[0];
	const GLfloat r=radius.getValue();
	
	GLfloat texY1=1.0f/ns;
	GLfloat lat1=1.0f*pi/ns-0.5f*pi;
	GLfloat r1=cosf(lat1);
	GLfloat z1=sinf(lat1);
	
	/* Draw "southern polar cap": */
	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(0.0f,0.0f,-1.0f);
	glTexCoord2f(0.5f,0.0f);
	glVertex3f(cx,cy,cz-r);
	for(int j=numSegments.getValue()*2;j>=0;--j)
		{
		GLfloat texX=GLfloat(j)/nq;
		GLfloat lng=GLfloat(j)*(2.0f*pi)/nq;
		GLfloat x1=cosf(lng)*r1;
		GLfloat y1=sinf(lng)*r1;
		glNormal3f(x1,y1,z1);
		glTexCoord2f(texX,texY1);
		glVertex3f(cx+x1*r,cy+y1*r,cz+z1*r);
		}
	glEnd();
	
	/* Draw quad strips: */
	for(int i=2;i<numSegments.getValue();++i)
		{
		GLfloat r0=r1;
		GLfloat z0=z1;
		GLfloat texY0=texY1;
		texY1=GLfloat(i)/ns;
		lat1=GLfloat(i)*pi/ns-0.5f*pi;
		r1=cosf(lat1);
		z1=sinf(lat1);
		
		glBegin(GL_QUAD_STRIP);
		for(int j=0;j<=numSegments.getValue()*2;++j)
			{
			GLfloat texX=GLfloat(j)/nq;
			GLfloat lng=GLfloat(j)*(2.0f*pi)/nq;
			GLfloat x1=cosf(lng)*r1;
			GLfloat y1=sinf(lng)*r1;
			glNormal3f(x1,y1,z1);
			glTexCoord2f(texX,texY1);
			glVertex3f(cx+x1*r,cy+y1*r,cz+z1*r);
			GLfloat x0=cosf(lng)*r0;
			GLfloat y0=sinf(lng)*r0;
			glNormal3f(x0,y0,z0);
			glTexCoord2f(texX,texY0);
			glVertex3f(cx+x0*r,cy+y0*r,cz+z0*r);
			}
		glEnd();
		}
	
	/* Draw "northern polar cap": */
	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(0.0f,0.0f,1.0f);
	glTexCoord2f(0.5f,1.0f);
	glVertex3f(cx,cy,cz+r);
	for(int j=0;j<=numSegments.getValue()*2;++j)
		{
		GLfloat texX=GLfloat(j)/nq;
		GLfloat lng=GLfloat(j)*(2.0f*pi)/nq;
		GLfloat x1=cosf(lng)*r1;
		GLfloat y1=sinf(lng)*r1;
		glNormal3f(x1,y1,z1);
		glTexCoord2f(texX,texY1);
		glVertex3f(cx+x1*r,cy+y1*r,cz+z1*r);
		}
	glEnd();
	}

SphereNode::SphereNode(void)
	:center(Point::origin),
	 radius(1.0f),
	 numSegments(32)
	{
	}

const char* SphereNode::getStaticClassName(void)
	{
	return "Sphere";
	}

const char* SphereNode::getClassName(void) const
	{
	return "Sphere";
	}

EventOut* SphereNode::getEventOut(const char* fieldName) const
	{
	if(strcmp(fieldName,"center")==0)
		return makeEventOut(this,center);
	else if(strcmp(fieldName,"radius")==0)
		return makeEventOut(this,radius);
	else if(strcmp(fieldName,"numSegments")==0)
		return makeEventOut(this,numSegments);
	else
		return GeometryNode::getEventOut(fieldName);
	}

EventIn* SphereNode::getEventIn(const char* fieldName)
	{
	if(strcmp(fieldName,"center")==0)
		return makeEventIn(this,center);
	else if(strcmp(fieldName,"radius")==0)
		return makeEventIn(this,radius);
	else if(strcmp(fieldName,"numSegments")==0)
		return makeEventIn(this,numSegments);
	else
		return GeometryNode::getEventIn(fieldName);
	}

void SphereNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"center")==0)
		vrmlFile.parseField(center);
	else if(strcmp(fieldName,"radius")==0)
		vrmlFile.parseField(radius);
	else if(strcmp(fieldName,"numSegments")==0)
		vrmlFile.parseField(numSegments);
	else
		GeometryNode::parseField(fieldName,vrmlFile);
	}

void SphereNode::update(void)
	{
	/* Invalidate the display list: */
	DisplayList::update();
	}

Box SphereNode::calcBoundingBox(void) const
	{
	/* Return a box around the sphere: */
	Point pmin=center.getValue();
	Point pmax=center.getValue();
	for(int i=0;i<3;++i)
		{
		pmin[i]-=radius.getValue();
		pmax[i]+=radius.getValue();
		}
	return Box(pmin,pmax);
	}

void SphereNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Set up OpenGL state: */
	renderState.enableCulling(GL_BACK);
	
	/* Render the display list: */
	DisplayList::glRenderAction(renderState.contextData);
	}

}
