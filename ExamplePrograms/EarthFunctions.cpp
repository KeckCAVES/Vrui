/***********************************************************************
EarthFunctions - Helper functions to display models of Earth and other
Earth-related stuff.
Copyright (c) 2005 Oliver Kreylos

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

#include <Math/Math.h>
#include <Math/Constants.h>
#include <GL/gl.h>
#include <GL/GLVertexArrayParts.h>
#define NONSTANDARD_GLVERTEX_TEMPLATES
#include <GL/GLVertex.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>

/***********************************************************************
Function to calculate radius of Earth at given latitude (geoid formula):
***********************************************************************/

double calcRadius(double latitude)
	{
	/* Constant parameters for geoid formula: */
	const double a=6378.14e3; // Equatorial radius in m
	const double f=1.0/298.247; // Geoid flattening factor
	
	/* Calculate radius at latitude according to geoid formula: */
	return a*(1.0-f*Math::sqr(Math::sin(latitude)));
	}

/***********************************************************************
Function to calculate the Cartesian coordinates of a point on the
Earth's surface:
***********************************************************************/

template <class ScalarParam>
void calcSurfacePos(ScalarParam latitude,ScalarParam longitude,double scaleFactor,ScalarParam pos[3])
	{
	/* Constant parameters for geoid formula: */
	const double a=6378.14e3; // Equatorial radius in m
	const double f=1.0/298.247; // Geoid flattening factor
	
	double s0=Math::sin(double(latitude));
	double c0=Math::cos(double(latitude));
	double r=a*(1.0-f*Math::sqr(s0))*scaleFactor;
	double xy=r*c0;
	double s1=Math::sin(double(longitude));
	double c1=Math::cos(double(longitude));
	pos[0]=ScalarParam(xy*c1);
	pos[1]=ScalarParam(xy*s1);
	pos[2]=ScalarParam(r*s0);
	}

/***********************************************************************
Function to calculate the Cartesian coordinates of a point in the
Earth's interior, given a depth:
***********************************************************************/

template <class ScalarParam>
void calcDepthPos(ScalarParam latitude,ScalarParam longitude,ScalarParam depth,double scaleFactor,ScalarParam pos[3])
	{
	/* Constant parameters for geoid formula: */
	const double a=6378.14e3; // Equatorial radius in m
	const double f=1.0/298.247; // Geoid flattening factor
	
	double s0=Math::sin(double(latitude));
	double c0=Math::cos(double(latitude));
	double r=(a*(1.0-f*Math::sqr(s0))-depth)*scaleFactor;
	double xy=r*c0;
	double s1=Math::sin(double(longitude));
	double c1=Math::cos(double(longitude));
	pos[0]=ScalarParam(xy*c1);
	pos[1]=ScalarParam(xy*s1);
	pos[2]=ScalarParam(r*s0);
	}

/***********************************************************************
Function to calculate the Cartesian coordinates of a point in the
Earth's interior, given a radius:
***********************************************************************/

template <class ScalarParam>
void calcRadiusPos(ScalarParam latitude,ScalarParam longitude,ScalarParam radius,double scaleFactor,ScalarParam pos[3])
	{
	double s0=Math::sin(double(latitude));
	double c0=Math::cos(double(latitude));
	double r=radius*scaleFactor;
	double xy=r*c0;
	double s1=Math::sin(double(longitude));
	double c1=Math::cos(double(longitude));
	pos[0]=ScalarParam(xy*c1);
	pos[1]=ScalarParam(xy*s1);
	pos[2]=ScalarParam(r*s0);
	}

/***********************************************************************
Function to draw a model of Earth using texture-mapped quad strips:
***********************************************************************/

void drawEarth(int numStrips,int numQuads,double scaleFactor)
	{
	const double a=6378.14e3; // Equatorial radius in m
	const double f=1.0/298.247; // Geoid flattening factor
	
	float texY1=float(0)/float(numStrips);
	double lat1=(Math::Constants<double>::pi*double(0))/double(numStrips)-0.5*Math::Constants<double>::pi;
	double s1=Math::sin(lat1);
	double c1=Math::cos(lat1);
	double r1=a*(1.0-f*s1*s1)*scaleFactor;
	double xy1=r1*c1;
	double z1=r1*s1;
	
	/* Draw latitude quad strips: */
	for(int i=1;i<numStrips+1;++i)
		{
		float texY0=texY1;
		double lat0=lat1;
		double s0=s1;
		double c0=c1;
		double r0=r1;
		double xy0=xy1;
		double z0=z1;
		texY1=float(i)/float(numStrips);
		lat1=(Math::Constants<double>::pi*double(i))/double(numStrips)-0.5*Math::Constants<double>::pi;
		s1=Math::sin(lat1);
		c1=Math::cos(lat1);
		r1=a*(1.0-f*s1*s1)*scaleFactor;
		xy1=r1*c1;
		z1=r1*s1;
		
		glBegin(GL_QUAD_STRIP);
		for(int j=0;j<=numQuads;++j)
			{
			float texX=float(j)/float(numQuads)+0.5f;
			glTexCoord2f(texX,texY1);
			double lng=(2.0*Math::Constants<double>::pi*double(j))/double(numQuads);
			double cl=Math::cos(lng);
			double sl=Math::sin(lng);
			double nx1=(1.0-3.0*f*s1*s1)*c1*cl;
			double ny1=(1.0-3.0*f*s1*s1)*c1*sl;
			double nz1=(1.0+3.0*f*c1*c1-f)*s1;
			double nl1=sqrt(nx1*nx1+ny1*ny1+nz1*nz1);
			glNormal3f(float(nx1/nl1),float(ny1/nl1),float(nz1/nl1));
			double x1=xy1*cl;
			double y1=xy1*sl;
			glVertex3f(float(x1),float(y1),float(z1));
			glTexCoord2f(texX,texY0);
			double nx0=(1.0-3.0*f*s0*s0)*c0*cl;
			double ny0=(1.0-3.0*f*s0*s0)*c0*sl;
			double nz0=(1.0+3.0*f*c0*c0-f)*s0;
			double nl0=sqrt(nx0*nx0+ny0*ny0+nz0*nz0);
			glNormal3f(float(nx0/nl0),float(ny0/nl0),float(nz0/nl0));
			double x0=xy0*cl;
			double y0=xy0*sl;
			glVertex3f(float(x0),float(y0),float(z0));
			}
		glEnd();
		}
	}

/***********************************************************************
Function to draw a model of Earth using texture-mapped quad strips:
***********************************************************************/

void drawEarth(int numStrips,int numQuads,double scaleFactor,unsigned int vertexBufferObjectId,unsigned int indexBufferObjectId)
	{
	typedef GLVertex<GLfloat,2,void,0,GLfloat,GLfloat,3> Vertex;
	
	const double a=6378.14e3; // Equatorial radius in m
	const double f=1.0/298.247; // Geoid flattening factor
	
	GLVertexArrayParts::enable(Vertex::getPartsMask());
	
	/* Upload the vertex data into the vertex buffer: */
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,vertexBufferObjectId);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,(numStrips+1)*(numQuads+1)*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
	Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	for(int i=0;i<=numStrips;++i)
		{
		float texY=float(i)/float(numStrips);
		double lat=(double(i)/double(numStrips)-0.5)*Math::Constants<double>::pi;
		double s0=Math::sin(lat);
		double c0=Math::cos(lat);
		double r=a*(1.0-f*s0*s0)*scaleFactor;
		double xy=r*c0;
		float z=r*s0;
		for(int j=0;j<=numQuads;++j,++vPtr)
			{
			float texX=float(j)/float(numQuads)+0.5f;
			vPtr->texCoord[0]=texX;
			vPtr->texCoord[1]=texY;
			double lng=(2.0*Math::Constants<double>::pi*double(j))/double(numQuads);
			double s1=Math::sin(lng);
			double c1=Math::cos(lng);
			double nx=(1.0-3.0*f*s0*s0)*c0*c1;
			double ny=(1.0-3.0*f*s0*s0)*c0*s1;
			double nz=(1.0+3.0*f*c0*c0-f)*s0;
			double nl=Math::sqrt(nx*nx+ny*ny+nz*nz);
			vPtr->normal[0]=float(nx/nl);
			vPtr->normal[1]=float(ny/nl);
			vPtr->normal[2]=float(nz/nl);
			vPtr->position[0]=float(xy*c1);
			vPtr->position[1]=float(xy*s1);
			vPtr->position[2]=z;
			}
		}
	glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
	
	/* Upload the index data into the index buffer: */
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,indexBufferObjectId);
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,numStrips*(numQuads+1)*2*sizeof(GLuint),0,GL_STATIC_DRAW_ARB);
	GLuint* iPtr=static_cast<GLuint*>(glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	for(int i=0;i<numStrips;++i)
		{
		for(int j=0;j<=numQuads;++j,iPtr+=2)
			{
			iPtr[0]=(i+1)*(numQuads+1)+j;
			iPtr[1]=(i+0)*(numQuads+1)+j;
			}
		}
	glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
	
	/* Render the quad strips: */
	glVertexPointer(static_cast<Vertex*>(0));
	GLubyte* stripBaseIndexPtr=0;
	for(int i=0;i<numStrips;++i)
		{
		glDrawElements(GL_QUAD_STRIP,(numQuads+1)*2,GL_UNSIGNED_INT,stripBaseIndexPtr);
		stripBaseIndexPtr+=(numQuads+1)*2*sizeof(GLuint);
		}
	
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
	GLVertexArrayParts::disable(Vertex::getPartsMask());
	}

/***********************************************************************
Function to draw a latitude/longitude grid on the Earth's surface:
***********************************************************************/

void drawGrid(int numStrips,int numQuads,int overSample,double scaleFactor)
	{
	const double a=6378.14e3; // Equatorial radius in m
	const double f=1.0/298.247; // Geoid flattening factor
	
	/* Draw circles of constant latitude (what are they called?): */
	for(int i=1;i<numStrips;++i)
		{
		double lat=(Math::Constants<double>::pi*double(i))/double(numStrips)-0.5*Math::Constants<double>::pi;
		double s=Math::sin(lat);
		double c=Math::cos(lat);
		double r=a*(1.0-f*s*s)*scaleFactor;
		double xy=r*c;
		double z=r*s;
		
		glBegin(GL_LINE_LOOP);
		for(int j=0;j<numQuads*overSample;++j)
			{
			double lng=(2.0*Math::Constants<double>::pi*double(j))/double(numQuads*overSample);
			double cl=Math::cos(lng);
			double sl=Math::sin(lng);
			double x=xy*cl;
			double y=xy*sl;
			glVertex3f(float(x),float(y),float(z));
			}
		glEnd();
		}
	
	/* Draw meridians: */
	for(int i=0;i<numQuads;++i)
		{
		double lng=(2.0*Math::Constants<double>::pi*double(i))/double(numQuads);
		double cl=Math::cos(lng);
		double sl=Math::sin(lng);
		
		glBegin(GL_LINE_STRIP);
		glVertex3f(0.0f,0.0f,-a*(1.0-f)*scaleFactor);
		for(int j=1;j<numStrips*overSample;++j)
			{
			double lat=(Math::Constants<double>::pi*double(j))/double(numStrips*overSample)-0.5*Math::Constants<double>::pi;
			double s=Math::sin(lat);
			double c=Math::cos(lat);
			double r=a*(1.0-f*s*s)*scaleFactor;
			double xy=r*c;
			double x=xy*cl;
			double y=xy*sl;
			double z=r*s;
			glVertex3f(float(x),float(y),float(z));
			}
		glVertex3f(0.0f,0.0f,a*(1.0-f)*scaleFactor);
		glEnd();
		}
	}

/***********************************************************************
Function to draw a "pin" sticking out of the Earth's surface:
***********************************************************************/

void drawPin(double latitude,double longitude,double height,double scaleFactor)
	{
	const double a=6378.14e3; // Equatorial radius in m
	const double f=1.0/298.247; // Geoid flattening factor
	
	double lat=(latitude*Math::Constants<double>::pi)/180.0;
	double s=Math::sin(lat);
	double c=Math::cos(lat);
	double r=a*(1.0-f*s*s)*scaleFactor;
	double xy=r*c;
	double lng=(longitude*Math::Constants<double>::pi)/180.0;
	double cl=Math::cos(lng);
	double sl=Math::sin(lng);
	double x1=xy*cl;
	double y1=xy*sl;
	double z1=r*s;
	double nx=(1.0-3.0*f*s*s)*c*cl;
	double ny=(1.0-3.0*f*s*s)*c*sl;
	double nz=(1.0+3.0*f*c*c-f)*s;
	double nl=sqrt(nx*nx+ny*ny+nz*nz);
	double x2=x1+nx*height/nl;
	double y2=y1+ny*height/nl;
	double z2=z1+nz*height/nl;
	glLineWidth(1.0f);
	glBegin(GL_LINES);
	glVertex3f(float(x1),float(y1),float(z1));
	glVertex3f(float(x2),float(y2),float(z2));
	glEnd();
	glPointSize(3.0f);
	glBegin(GL_POINTS);
	glVertex3f(float(x2),float(y2),float(z2));
	glEnd();
	}

/***********************************************************************
Instantiate all template function for common template parameters:
***********************************************************************/

template void calcSurfacePos<float>(float,float,double,float[3]);
template void calcSurfacePos<double>(double,double,double,double[3]);

template void calcDepthPos<float>(float,float,float,double,float[3]);
template void calcDepthPos<double>(double,double,double,double,double[3]);

template void calcRadiusPos<float>(float,float,float,double,float[3]);
template void calcRadiusPos<double>(double,double,double,double,double[3]);
