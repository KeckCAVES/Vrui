/***********************************************************************
ScreenProtectorArea - Class describing an area of physical space that
needs to be protected from penetration by input devices.
Copyright (c) 2000-2016 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Vrui/Internal/ScreenProtectorArea.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Math/Math.h>
#include <Geometry/Point.h>
#include <Geometry/AffineCombiner.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLGeometryWrappers.h>
#include <Vrui/VRScreen.h>

namespace Vrui {

/************************************
Methods of class ScreenProtectorArea:
************************************/

void ScreenProtectorArea::init(void)
	{
	/* Determine the primary axes most aligned with the polygon's plane: */
	Scalar maxNormalComponent=Math::abs(plane.getNormal()[0]);
	int maxAxis=0;
	for(int axis=1;axis<3;++axis)
		{
		Scalar normalComponent=Math::abs(plane.getNormal()[axis]);
		if(maxNormalComponent<normalComponent)
			{
			maxNormalComponent=normalComponent;
			maxAxis=axis;
			}
		}
	axis0=(maxAxis+1)%3;
	axis1=(maxAxis+2)%3;
	
	/* Calculate the polygon's edge directions and lengths: */
	for(unsigned int i=0U;i<4U;++i)
		{
		edges[i]=vertices[(i+1U)%4U]-vertices[i];
		edgeLengths[i]=edges[i].mag();
		edges[i]/=edgeLengths[i];
		}
	}

ScreenProtectorArea::ScreenProtectorArea(void)
	:numVertices(0U),
	 vertices(0),edges(0),edgeLengths(0)
	{
	}

ScreenProtectorArea::ScreenProtectorArea(const VRScreen& screen)
	:numVertices(4U),
	 vertices(new Point[numVertices]),edges(new Vector[numVertices]),edgeLengths(new Scalar[numVertices])
	{
	/* Calculate the screen's four corners in screen coordinates: */
	vertices[0]=Point(0,0,0);
	vertices[1]=Point(screen.getWidth(),0,0);
	vertices[2]=Point(screen.getWidth(),screen.getHeight(),0);
	vertices[3]=Point(0,screen.getHeight(),0);
	
	/* Transform the corners to physical space: */
	ONTransform screenT=screen.getScreenTransformation();
	for(unsigned int i=0U;i<4U;++i)
		vertices[i]=screenT.transform(vertices[i]);
	
	/* Calculate the screen's plane equation: */
	plane=Plane(screenT.getDirection(2),Geometry::mid(Geometry::mid(vertices[0],vertices[1]),Geometry::mid(vertices[2],vertices[3])));
	
	/* Finish initialization: */
	init();
	}

ScreenProtectorArea::ScreenProtectorArea(const std::vector<Point>& sVertices)
	:numVertices(sVertices.size()),
	 vertices(new Point[numVertices]),edges(new Vector[numVertices]),edgeLengths(new Scalar[numVertices])
	{
	/* Copy the polygon vertices: */
	for(unsigned int i=0U;i<numVertices;++i)
		vertices[i]=sVertices[i];
	
	/* Calculate the polygon's centroid and normal vector by adding cross products of each pair of adjacent edges: */
	Point::AffineCombiner ac;
	Vector normal=Vector::zero;
	Point* p2=&vertices[numVertices-1U];
	Vector d1=*p2-vertices[numVertices-2U];
	for(unsigned int i=0U;i<numVertices;++i)
		{
		/* Calculate the cross product of the vertex' two incident edges: */
		Point* p3=&vertices[i];
		ac.addPoint(*p3);
		Vector d2=*p3-*p2;
		normal+=d1^d2;
		
		/* Go to the next vertex: */
		p2=p3;
		d1=d2;
		}
	normal.normalize();
	plane=Plane(normal,ac.getPoint());
	
	/* Project all vertices into the plane: */
	for(unsigned int i=0U;i<numVertices;++i)
		vertices[i]=plane.project(vertices[i]);
	
	/* Finish initialization: */
	init();
	}

ScreenProtectorArea::ScreenProtectorArea(const ScreenProtectorArea& source)
	:plane(source.plane),axis0(source.axis0),axis1(source.axis1),
	 numVertices(source.numVertices),
	 vertices(new Point[numVertices]),edges(new Vector[numVertices]),edgeLengths(new Scalar[numVertices])
	{
	/* Copy vertices, edges, and edge lengths: */
	for(unsigned int i=0U;i<numVertices;++i)
		{
		vertices[i]=source.vertices[i];
		edges[i]=source.edges[i];
		edgeLengths[i]=source.edgeLengths[i];
		}
	}

ScreenProtectorArea& ScreenProtectorArea::operator=(const ScreenProtectorArea& source)
	{
	/* Check for aliasing: */
	if(this!=&source)
		{
		/* Delete the polygon arrays: */
		delete[] vertices;
		delete[] edges;
		delete[] edgeLengths;
		
		/* Copy source state: */
		plane=source.plane;
		axis0=source.axis0;
		axis1=source.axis1;
		numVertices=source.numVertices;
		
		/* Copy vertices, edges, and edge lengths: */
		vertices=new Point[numVertices];
		edges=new Vector[numVertices];
		edgeLengths=new Scalar[numVertices];
		for(unsigned int i=0U;i<numVertices;++i)
			{
			vertices[i]=source.vertices[i];
			edges[i]=source.edges[i];
			edgeLengths[i]=source.edgeLengths[i];
			}
		}
	
	return *this;
	}

ScreenProtectorArea::~ScreenProtectorArea(void)
	{
	/* Delete the polygon arrays: */
	delete[] vertices;
	delete[] edges;
	delete[] edgeLengths;
	}

Scalar ScreenProtectorArea::calcPenetrationDepth(const Point& center,Scalar radius) const
	{
	/* Check the sphere's center against the area's plane: */
	Scalar centerDist=Math::abs(plane.calcDistance(center));
	if(centerDist>=radius)
		return Scalar(0); // Sphere is too far away from plane; does not penetrate
	
	/* Check the projection of the sphere's center into the area's plane against the area's polygon: */
	Point pc=plane.project(center);
	unsigned int numRegionChanges=0U;
	const Point* p0=vertices+(numVertices-1U);
	for(const Point* p1=vertices;p1!=vertices+numVertices;p0=p1,++p1)
		{
		/* Check if a ray from the sphere's center crosses the edge from p0 to p1: */
		if((*p0)[axis1]<=pc[axis1]&&(*p1)[axis1]>pc[axis1]) // Edge crosses ray from bottom to top
			{
			/* Check if the ray's intersection with the edge is to the right of the center point: */
			if((*p0)[axis0]*((*p1)[axis1]-pc[axis1])+(*p1)[axis0]*(pc[axis1]-(*p0)[axis1])>=pc[axis0]*((*p1)[axis1]-(*p0)[axis1]))
				++numRegionChanges;
			}
		else if((*p1)[axis1]<=pc[axis1]&&(*p0)[axis1]>pc[axis1]) // Edge crosses ray from top to bottom
			{
			/* Check if the ray's intersection with the edge is to the right of the center point: */
			if((*p1)[axis0]*((*p0)[axis1]-pc[axis1])+(*p0)[axis0]*(pc[axis1]-(*p1)[axis1])>=pc[axis0]*((*p0)[axis1]-(*p1)[axis1]))
				++numRegionChanges;
			}
		}
	if(numRegionChanges%2U==1U)
		return (radius-centerDist)/radius; // Sphere center is inside area's polygon
	
	/* Check the sphere's center against all edges of the area's polygon: */
	for(unsigned int i=0U;i<numVertices;++i)
		{
		/* Calculate the position of the sphere's center along the edge: */
		Vector pc=center-vertices[i];
		Scalar x=pc*edges[i];
		if(x<=edgeLengths[i])
			{
			Scalar l2=pc.sqr();
			if(x>=Scalar(0))
				{
				/* Check the sphere's center against the edge's cylinder: */
				Scalar d2=l2-x*x;
				if(d2<=radius*radius)
					return (radius-Math::sqrt(d2))/radius;
				}
			else
				{
				/* Check the sphere's center against the edge's start vertex: */
				if(l2<=radius*radius)
					return (radius-Math::sqrt(l2))/radius;
				}
			}
		}
	
	/* Sphere does not penetrate area: */
	return Scalar(0);
	}

void ScreenProtectorArea::glRenderAction(Scalar gridLineDist) const
	{
	/* Draw the polygon's boundary: */
	glBegin(GL_LINE_LOOP);
	for(unsigned int i=0U;i<numVertices;++i)
		glVertex(vertices[i]);
	glEnd();
	
	/* Draw grid lines inside the polygon: */
	glBegin(GL_LINES);
	Point* intersections=new Point[numVertices];
	for(int axisIndex=0;axisIndex<2;++axisIndex)
		{
		/* Determine the primary axis with which to align these grid lines: */
		int axis=axisIndex==0?axis0:axis1;
		int sort=axisIndex==0?axis1:axis0;
		
		/* Calculate the extent of the polygon along the selected primary axis: */
		Scalar min=vertices[0U][axis];
		Scalar max=vertices[0U][axis];
		for(unsigned int i=1U;i<numVertices;++i)
			{
			if(min>vertices[i][axis])
				min=vertices[i][axis];
			if(max<vertices[i][axis])
				max=vertices[i][axis];
			}
		
		/* Intersect the polygon with a sequence of planes aligned to a primary axis: */
		Scalar step=Math::sqrt(Scalar(1)-Math::sqr(plane.getNormal()[axis]))*gridLineDist;
		int numLines=Math::ceil((max-min)/step)-1;
		Scalar level=min+((max-min)-step*Scalar(numLines-1))*Scalar(0.5);
		for(int line=0;line<numLines;++line,level+=step)
			{
			/* Find all intersections between the grid plane and the polygon's edges: */
			unsigned int numIntersections=0U;
			const Point* p0=vertices+(numVertices-1U);
			for(const Point* p1=vertices;p1!=vertices+numVertices;p0=p1,++p1)
				{
				if(((*p0)[axis]<=level&&(*p1)[axis]>level)||((*p1)[axis]<=level&&(*p0)[axis]>level)) // Edge crosses plane
					{
					/* Calculate the intersection point: */
					intersections[numIntersections]=Geometry::affineCombination(*p0,*p1,(level-(*p0)[axis])/((*p1)[axis]-(*p0)[axis]));
					++numIntersections;
					
					/* Sort the array of intersections by position along the sorting axis: */
					for(unsigned int i=numIntersections-1U;i>0U&&intersections[i][sort]<intersections[i-1U][sort];--i)
						{
						Point t=intersections[i-1U];
						intersections[i-1U]=intersections[i];
						intersections[i]=t;
						}
					}
				}
			
			/* Number of intersections is always even; draw line segments between all pairs: */
			for(unsigned int i=0U;i<numIntersections;++i)
				glVertex(intersections[i]);
			}
		}
	delete[] intersections;
	glEnd();
	}

}

namespace Misc {

/******************************************************
Methods of class ValueCoder<Vrui::ScreenProtectorArea>:
******************************************************/

std::string
ValueCoder<Vrui::ScreenProtectorArea>::encode(
	const Vrui::ScreenProtectorArea& value)
	{
	std::string result;
	result.push_back('(');
	result.append(ValueCoder<Vrui::Point>::encode(value.vertices[0]));
	for(unsigned int i=1U;i<value.numVertices;++i)
		{
		result.push_back(',');
		result.push_back(' ');
		result.append(ValueCoder<Vrui::Point>::encode(value.vertices[i]));
		}
	result.push_back(')');
	return result;
	}

Vrui::ScreenProtectorArea
ValueCoder<Vrui::ScreenProtectorArea>::decode(
	const char* start,
	const char* end,
	const char** decodeEnd)
	{
	/* Decode a list of points from the given string: */
	std::vector<Vrui::Point> vertices=Misc::ValueCoder<std::vector<Vrui::Point> >::decode(start,end,decodeEnd);
	
	/* Create a screen protector area from the decoded list: */
	return Vrui::ScreenProtectorArea(vertices);
	}

}
