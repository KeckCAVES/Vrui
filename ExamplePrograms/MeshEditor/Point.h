/***********************************************************************
Point - Class for points in 3-space
Copyright (c) 2000-2006 Oliver Kreylos

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

#ifndef POINT_INCLUDED
#define POINT_INCLUDED

#include <math.h>

template <class ScalarType =float> class Point
	{
	/* Embedded classes: */
	public:
	typedef ScalarType Scalar;
	
	/* Elements: */
	private:
	ScalarType position[3]; // Coordinates of point in space
	/* Constructors and destructors: */
	public:
	static Point zero(void) // The zero point (origin)
		{
		return Point(ScalarType(0),ScalarType(0),ScalarType(0));
		};
	Point(void) // Dummy constructor
		{
		};
	Point(ScalarType sX,ScalarType sY,ScalarType sZ) // With components
		{
		position[0]=sX;
		position[1]=sY;
		position[2]=sZ;
		};
	template <class InputScalarType> Point(const InputScalarType sPosition[3]) // With array
		{
		position[0]=ScalarType(sPosition[0]);
		position[1]=ScalarType(sPosition[1]);
		position[2]=ScalarType(sPosition[2]);
		};
	template <class InputScalarType> Point(const Point<InputScalarType>& source) // With other point
		{
		position[0]=ScalarType(source[0]);
		position[1]=ScalarType(source[1]);
		position[2]=ScalarType(source[2]);
		};
	/* Methods: */
	template <class InputScalarType> Point& operator=(const Point<InputScalarType>& source) // Assignment operator
		{
		position[0]=ScalarType(source[0]);
		position[1]=ScalarType(source[1]);
		position[2]=ScalarType(source[2]);
		return *this;
		};
	const ScalarType* pos(void) const // Returns pointer to array of components
		{
		return position;
		};
	ScalarType* pos(void) // Ditto
		{
		return position;
		};
	ScalarType operator[](int index) const // Returns specific component
		{
		return position[index];
		};
	ScalarType& operator[](int index) // Ditto
		{
		return position[index];
		};
	friend bool operator==(const Point& p1,const Point& p2) // Exact equality
		{
		return p1[0]==p2[0]&&p1[1]==p2[1]&&p1[2]==p2[2];
		};
	friend bool operator!=(const Point& p1,const Point& p2) // Exact inequality including
		{
		return p1[0]!=p2[0]||p1[1]!=p2[1]||p1[2]!=p2[2];
		};
	friend bool same(const Point& p1,const Point& p2,double epsilon) // Checks if two points have approximately the same position
		{
		return fabs(p1[0]-p2[0])<=epsilon&&fabs(p1[1]-p2[1])<=epsilon&&fabs(p1[2]-p2[2])<=epsilon;
		};
	template <class InputPoint> double sqrDist(const InputPoint& p) const // Returns squared distance to other point
		{
		double result=0.0;
		for(int i=0;i<3;++i)
			{
			double d=double(position[i])-double(p[i]);
			result+=d*d;
			}
		return result;
		};
	friend double sqrDist(const Point& p1,const Point& p2) // Returns squared distance between two points of same type
		{
		double result=0.0;
		for(int i=0;i<3;++i)
			{
			double d=double(p1[i])-double(p2[i]);
			result+=d*d;
			}
		return result;
		};
	/* Methods to calculate affine combinations of points: */
	Point& add(const Point& p) // Adds components of another point to the current one
		{
		for(int i=0;i<3;++i)
			position[i]+=p[i];
		return *this;
		};
	template <class InputScalarType>
	Point& add(const Point& p,InputScalarType weight) // Adds components of multiple of another point to the current one
		{
		for(int i=0;i<3;++i)
			position[i]+=p[i]*ScalarType(weight);
		return *this;
		};
	template <class InputScalarType>
	Point& normalize(InputScalarType sumWeights) // Divides the point sum by the sum of weights
		{
		for(int i=0;i<3;++i)
			position[i]/=ScalarType(sumWeights);
		return *this;
		};
	template <class OutputScalarType>
	friend OutputScalarType* planeNormal(const Point& p1,const Point& p2,const Point& p3,OutputScalarType normal[3]) // Returns (non-normalized) normal vector of three points
		{
		/* Calculate the two distance vectors: */
		double d1[3],d2[3];
		for(int i=0;i<3;++i)
			{
			d1[i]=p2[i]-p1[i];
			d2[i]=p3[i]-p1[i];
			}
		
		/* Calculate their cross product: */
		normal[0]=OutputScalarType(d1[1]*d2[2]-d1[2]*d2[1]);
		normal[1]=OutputScalarType(d1[2]*d2[0]-d1[0]*d2[2]);
		normal[2]=OutputScalarType(d1[0]*d2[1]-d1[1]*d2[0]);
		
		return normal;
		};
	};

#endif
