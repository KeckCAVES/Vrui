/***********************************************************************
OrthonormalTransformation - Class for transformations constructed from
only translations and rotations.
Copyright (c) 2002-2005 Oliver Kreylos

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

#define GEOMETRY_ORTHONORMALTRANSFORMATION_IMPLEMENTATION

#ifndef METHODPREFIX
	#ifdef NONSTANDARD_TEMPLATES
		#define METHODPREFIX inline
	#else
		#define METHODPREFIX
	#endif
#endif

#include <Geometry/TranslationTransformation.h>
#include <Geometry/RotationTransformation.h>

#include <Geometry/OrthonormalTransformation.h>

namespace Geometry {

/**************************************************
Static elements of class OrthonormalTransformation:
**************************************************/

template <class ScalarParam,int dimensionParam>
const int OrthonormalTransformation<ScalarParam,dimensionParam>::dimension;
template <class ScalarParam,int dimensionParam>
const OrthonormalTransformation<ScalarParam,dimensionParam> OrthonormalTransformation<ScalarParam,dimensionParam>::identity; // Default constructor creates identity transformation!

/******************************************
Methods of class OrthonormalTransformation:
******************************************/

template <class ScalarParam,int dimensionParam>
template <class SourceScalarParam>
METHODPREFIX OrthonormalTransformation<ScalarParam,dimensionParam>::OrthonormalTransformation(const TranslationTransformation<SourceScalarParam,dimensionParam>& source)
	:translation(source.getTranslation())
	{
	}

template <class ScalarParam,int dimensionParam>
template <class SourceScalarParam>
METHODPREFIX OrthonormalTransformation<ScalarParam,dimensionParam>::OrthonormalTransformation(const RotationTransformation<SourceScalarParam,dimensionParam>& source)
	:translation(ScalarParam(0)),rotation(source.getRotation())
	{
	}

/****************************************************
Concatenation operators with built-in generalization:
****************************************************/

template <class ScalarParam,int dimensionParam>
OrthonormalTransformation<ScalarParam,dimensionParam> operator*(const TranslationTransformation<ScalarParam,dimensionParam>& t1,const RotationTransformation<ScalarParam,dimensionParam>& t2)
	{
	return OrthonormalTransformation<ScalarParam,dimensionParam>(t1.getTranslation(),t2.getRotation());
	}

template <class ScalarParam,int dimensionParam>
OrthonormalTransformation<ScalarParam,dimensionParam> operator*(const RotationTransformation<ScalarParam,dimensionParam>& t1,const TranslationTransformation<ScalarParam,dimensionParam>& t2)
	{
	return OrthonormalTransformation<ScalarParam,dimensionParam>(t1.transform(t2.getTranslation()),t1.getRotation());
	}

#if !defined(NONSTANDARD_TEMPLATES)

/***********************************************************************************
Force instantiation of all standard OrthonormalTransformation classes and functions:
***********************************************************************************/

template class OrthonormalTransformation<float,2>;
template OrthonormalTransformation<float,2>::OrthonormalTransformation(const TranslationTransformation<float,2>&);
template OrthonormalTransformation<float,2>::OrthonormalTransformation(const RotationTransformation<float,2>&);
template OrthonormalTransformation<float,2>::OrthonormalTransformation(const TranslationTransformation<double,2>&);
template OrthonormalTransformation<float,2>::OrthonormalTransformation(const RotationTransformation<double,2>&);
template OrthonormalTransformation<float,2> operator*(const TranslationTransformation<float,2>&,const RotationTransformation<float,2>&);
template OrthonormalTransformation<float,2> operator*(const RotationTransformation<float,2>&,const TranslationTransformation<float,2>&);

template class OrthonormalTransformation<double,2>;
template OrthonormalTransformation<double,2>::OrthonormalTransformation(const TranslationTransformation<float,2>&);
template OrthonormalTransformation<double,2>::OrthonormalTransformation(const RotationTransformation<float,2>&);
template OrthonormalTransformation<double,2>::OrthonormalTransformation(const TranslationTransformation<double,2>&);
template OrthonormalTransformation<double,2>::OrthonormalTransformation(const RotationTransformation<double,2>&);
template OrthonormalTransformation<double,2> operator*(const TranslationTransformation<double,2>&,const RotationTransformation<double,2>&);
template OrthonormalTransformation<double,2> operator*(const RotationTransformation<double,2>&,const TranslationTransformation<double,2>&);

template class OrthonormalTransformation<float,3>;
template OrthonormalTransformation<float,3>::OrthonormalTransformation(const TranslationTransformation<float,3>&);
template OrthonormalTransformation<float,3>::OrthonormalTransformation(const RotationTransformation<float,3>&);
template OrthonormalTransformation<float,3>::OrthonormalTransformation(const TranslationTransformation<double,3>&);
template OrthonormalTransformation<float,3>::OrthonormalTransformation(const RotationTransformation<double,3>&);
template OrthonormalTransformation<float,3> operator*(const TranslationTransformation<float,3>&,const RotationTransformation<float,3>&);
template OrthonormalTransformation<float,3> operator*(const RotationTransformation<float,3>&,const TranslationTransformation<float,3>&);

template class OrthonormalTransformation<double,3>;
template OrthonormalTransformation<double,3>::OrthonormalTransformation(const TranslationTransformation<float,3>&);
template OrthonormalTransformation<double,3>::OrthonormalTransformation(const RotationTransformation<float,3>&);
template OrthonormalTransformation<double,3>::OrthonormalTransformation(const TranslationTransformation<double,3>&);
template OrthonormalTransformation<double,3>::OrthonormalTransformation(const RotationTransformation<double,3>&);
template OrthonormalTransformation<double,3> operator*(const TranslationTransformation<double,3>&,const RotationTransformation<double,3>&);
template OrthonormalTransformation<double,3> operator*(const RotationTransformation<double,3>&,const TranslationTransformation<double,3>&);

#endif

}
