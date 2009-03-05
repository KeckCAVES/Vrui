/***********************************************************************
Random - Functions to create random points or vectors according to
several probability distributions.
Copyright (c) 2007 Oliver Kreylos

This file is part of the Templatized Geometry Library (TGL).

The Templatized Geometry Library is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Templatized Geometry Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Templatized Geometry Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#define GEOMETRY_RANDOM_IMPLEMENTATION

#ifndef METHODPREFIX
	#ifdef NONSTANDARD_TEMPLATES
		#define METHODPREFIX inline
	#else
		#define METHODPREFIX
	#endif
#endif

#include <Math/Math.h>
#include <Math/Random.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Box.h>

#include <Geometry/Random.h>

namespace Geometry {

template <class ScalarParam,int dimensionParam>
METHODPREFIX
Point<ScalarParam,dimensionParam>
randPointUniformCO(
	const Point<ScalarParam,dimensionParam>& min,
	const Point<ScalarParam,dimensionParam>& max)
	{
	Point<ScalarParam,dimensionParam> p;
	for(int i=0;i<dimensionParam;++i)
		p[i]=ScalarParam(Math::randUniformCO(double(min[i]),double(max[i])));
	return p;
	}

template <class ScalarParam,int dimensionParam>
METHODPREFIX
Point<ScalarParam,dimensionParam>
randPointUniformCC(
	const Point<ScalarParam,dimensionParam>& min,
	const Point<ScalarParam,dimensionParam>& max)
	{
	Point<ScalarParam,dimensionParam> p;
	for(int i=0;i<dimensionParam;++i)
		p[i]=ScalarParam(Math::randUniformCC(double(min[i]),double(max[i])));
	return p;
	}

template <class ScalarParam,int dimensionParam>
METHODPREFIX
Point<ScalarParam,dimensionParam>
randPointUniformCO(
	const Box<ScalarParam,dimensionParam>& box)
	{
	Point<ScalarParam,dimensionParam> p;
	for(int i=0;i<dimensionParam;++i)
		p[i]=ScalarParam(Math::randUniformCO(double(box.min[i]),double(box.max[i])));
	return p;
	}

template <class ScalarParam,int dimensionParam>
METHODPREFIX
Point<ScalarParam,dimensionParam>
randPointUniformCC(
	const Box<ScalarParam,dimensionParam>& box)
	{
	Point<ScalarParam,dimensionParam> p;
	for(int i=0;i<dimensionParam;++i)
		p[i]=ScalarParam(Math::randUniformCC(double(box.min[i]),double(box.max[i])));
	return p;
	}

template <class ScalarParam,int dimensionParam>
METHODPREFIX
Vector<ScalarParam,dimensionParam>
randVectorUniform(
	ScalarParam length)
	{
	/* Create random vectors in [-1,1]^n until one is inside a spherical shell: */
	Vector<ScalarParam,dimensionParam> v;
	ScalarParam vLen;
	do
		{
		vLen=ScalarParam(0);
		for(int i=0;i<dimensionParam;++i)
			{
			v[i]=ScalarParam(Math::randUniformCC(-1.0,1.0));
			vLen+=Math::sqr(v[i]);
			}
		}
	while(vLen<ScalarParam(0.25)||vLen>ScalarParam(1));
	
	/* Scale and return the result vector: */
	ScalarParam scale=length/Math::sqrt(vLen);
	for(int i=0;i<dimensionParam;++i)
		v[i]*=scale;
	return v;
	}

template <class ScalarParam,int dimensionParam>
METHODPREFIX
Vector<ScalarParam,dimensionParam>
randUnitVectorUniform(
	void)
	{
	/* Create random vectors in [-1,1]^n until one is inside a spherical shell: */
	Vector<ScalarParam,dimensionParam> v;
	ScalarParam vLen;
	do
		{
		vLen=ScalarParam(0);
		for(int i=0;i<dimensionParam;++i)
			{
			v[i]=ScalarParam(Math::randUniformCC(-1.0,1.0));
			vLen+=Math::sqr(v[i]);
			}
		}
	while(vLen<ScalarParam(0.25)||vLen>ScalarParam(1));
	
	/* Normalize and return the result vector: */
	vLen=Math::sqrt(vLen);
	for(int i=0;i<dimensionParam;++i)
		v[i]/=vLen;
	return v;
	}

template <class ScalarParam,int dimensionParam>
METHODPREFIX
Vector<ScalarParam,dimensionParam>
randVectorNormal(
	ScalarParam stddev)
	{
	/* Create random vectors in [-1,1]^n until one is inside a spherical shell: */
	Vector<ScalarParam,dimensionParam> v;
	ScalarParam vLen;
	do
		{
		vLen=ScalarParam(0);
		for(int i=0;i<dimensionParam;++i)
			{
			v[i]=ScalarParam(Math::randUniformCC(-1.0,1.0));
			vLen+=Math::sqr(v[i]);
			}
		}
	while(vLen<ScalarParam(0.25)||vLen>ScalarParam(1));
	
	/* Calculate the length of the result vector: */
	ScalarParam len=ScalarParam(Math::randNormal(0.0,double(stddev)));
	
	/* Scale and return the result vector: */
	ScalarParam scale=len/Math::sqrt(vLen);
	for(int i=0;i<dimensionParam;++i)
		v[i]*=scale;
	return v;
	}

#if !defined(NONSTANDARD_TEMPLATES)

/*********************************************
Force instantiation of all standard functions:
*********************************************/

template Point<float,2> randPointUniformCO<float,2>(const Point<float,2>&,const Point<float,2>&);
template Point<double,2> randPointUniformCO<double,2>(const Point<double,2>&,const Point<double,2>&);
template Point<float,3> randPointUniformCO<float,3>(const Point<float,3>&,const Point<float,3>&);
template Point<double,3> randPointUniformCO<double,3>(const Point<double,3>&,const Point<double,3>&);

template Point<float,2> randPointUniformCC<float,2>(const Point<float,2>&,const Point<float,2>&);
template Point<double,2> randPointUniformCC<double,2>(const Point<double,2>&,const Point<double,2>&);
template Point<float,3> randPointUniformCC<float,3>(const Point<float,3>&,const Point<float,3>&);
template Point<double,3> randPointUniformCC<double,3>(const Point<double,3>&,const Point<double,3>&);

template Point<float,2> randPointUniformCO<float,2>(const Box<float,2>&);
template Point<double,2> randPointUniformCO<double,2>(const Box<double,2>&);
template Point<float,3> randPointUniformCO<float,3>(const Box<float,3>&);
template Point<double,3> randPointUniformCO<double,3>(const Box<double,3>&);

template Point<float,2> randPointUniformCC<float,2>(const Box<float,2>&);
template Point<double,2> randPointUniformCC<double,2>(const Box<double,2>&);
template Point<float,3> randPointUniformCC<float,3>(const Box<float,3>&);
template Point<double,3> randPointUniformCC<double,3>(const Box<double,3>&);

template Vector<float,2> randVectorUniform<float,2>(float);
template Vector<double,2> randVectorUniform<double,2>(double);
template Vector<float,3> randVectorUniform<float,3>(float);
template Vector<double,3> randVectorUniform<double,3>(double);

template Vector<float,2> randUnitVectorUniform<float,2>(void);
template Vector<double,2> randUnitVectorUniform<double,2>(void);
template Vector<float,3> randUnitVectorUniform<float,3>(void);
template Vector<double,3> randUnitVectorUniform<double,3>(void);

template Vector<float,2> randVectorNormal<float,2>(float);
template Vector<double,2> randVectorNormal<double,2>(double);
template Vector<float,3> randVectorNormal<float,3>(float);
template Vector<double,3> randVectorNormal<double,3>(double);

#endif

}
