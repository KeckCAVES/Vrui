/***********************************************************************
HVector - Class for homogenuous vectors.
Copyright (c) 2001-2005 Oliver Kreylos

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

#define GEOMETRY_HVECTOR_IMPLEMENTATION

#ifndef METHODPREFIX
	#ifdef NONSTANDARD_TEMPLATES
		#define METHODPREFIX inline
	#else
		#define METHODPREFIX
	#endif
#endif

#include <Geometry/HVector.h>

namespace Geometry {

/*******************************
Static elements of class HVector:
*******************************/

template <class ScalarParam,int dimensionParam>
const HVector<ScalarParam,dimensionParam> HVector<ScalarParam,dimensionParam>::zero(ScalarParam(0),ScalarParam(0));
template <class ScalarParam,int dimensionParam>
const HVector<ScalarParam,dimensionParam> HVector<ScalarParam,dimensionParam>::origin(ScalarParam(0),ScalarParam(1));

#if !defined(NONSTANDARD_TEMPLATES)

/*****************************************************************
Force instantiation of all standard HVector classes and functions:
*****************************************************************/

template const HVector<int,2> HVector<int,2>::zero;
template const HVector<int,2> HVector<int,2>::origin;

template const HVector<int,3> HVector<int,3>::zero;
template const HVector<int,3> HVector<int,3>::origin;

template const HVector<float,2> HVector<float,2>::zero;
template const HVector<float,2> HVector<float,2>::origin;

template const HVector<float,3> HVector<float,3>::zero;
template const HVector<float,3> HVector<float,3>::origin;

template const HVector<double,2> HVector<double,2>::zero;
template const HVector<double,2> HVector<double,2>::origin;

template const HVector<double,3> HVector<double,3>::zero;
template const HVector<double,3> HVector<double,3>::origin;

#endif

}
