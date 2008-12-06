/***********************************************************************
OrthogonalTransformation - Class for transformations constructed from
only translations, rotations and uniform scalings.
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

#define GEOMETRY_ORTHOGONALTRANSFORMATION_IMPLEMENTATION

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
#include <Geometry/UniformScalingTransformation.h>

#include <Geometry/OrthogonalTransformation.h>

namespace Geometry {

/*************************************************
Static elements of class OrthogonalTransformation:
*************************************************/

template <class ScalarParam,int dimensionParam>
const OrthogonalTransformation<ScalarParam,dimensionParam> OrthogonalTransformation<ScalarParam,dimensionParam>::identity; // Default constructor creates identity transformation!

/*****************************************
Methods of class OrthogonalTransformation:
*****************************************/

template <class ScalarParam,int dimensionParam>
template <class SourceScalarParam>
METHODPREFIX OrthogonalTransformation<ScalarParam,dimensionParam>::OrthogonalTransformation(const TranslationTransformation<SourceScalarParam,dimensionParam>& source)
	:translation(source.getTranslation()),scaling(1)
	{
	}

template <class ScalarParam,int dimensionParam>
template <class SourceScalarParam>
METHODPREFIX OrthogonalTransformation<ScalarParam,dimensionParam>::OrthogonalTransformation(const RotationTransformation<SourceScalarParam,dimensionParam>& source)
	:translation(Scalar(0)),rotation(source.getRotation()),scaling(1)
	{
	}

template <class ScalarParam,int dimensionParam>
template <class SourceScalarParam>
METHODPREFIX OrthogonalTransformation<ScalarParam,dimensionParam>::OrthogonalTransformation(const OrthonormalTransformation<SourceScalarParam,dimensionParam>& source)
	:translation(source.getTranslation()),rotation(source.getRotation()),scaling(1)
	{
	}

template <class ScalarParam,int dimensionParam>
template <class SourceScalarParam>
METHODPREFIX OrthogonalTransformation<ScalarParam,dimensionParam>::OrthogonalTransformation(const UniformScalingTransformation<SourceScalarParam,dimensionParam>& source)
	:translation(Scalar(0)),scaling(source.getScaling())
	{
	}

#if !defined(NONSTANDARD_TEMPLATES)

/**********************************************************************************
Force instantiation of all standard OrthogonalTransformation classes and functions:
**********************************************************************************/

template class OrthogonalTransformation<float,2>;
template OrthogonalTransformation<float,2>::OrthogonalTransformation(const TranslationTransformation<float,2>&);
template OrthogonalTransformation<float,2>::OrthogonalTransformation(const RotationTransformation<float,2>&);
template OrthogonalTransformation<float,2>::OrthogonalTransformation(const OrthonormalTransformation<float,2>&);
template OrthogonalTransformation<float,2>::OrthogonalTransformation(const UniformScalingTransformation<float,2>&);
template OrthogonalTransformation<float,2>::OrthogonalTransformation(const TranslationTransformation<double,2>&);
template OrthogonalTransformation<float,2>::OrthogonalTransformation(const RotationTransformation<double,2>&);
template OrthogonalTransformation<float,2>::OrthogonalTransformation(const OrthonormalTransformation<double,2>&);
template OrthogonalTransformation<float,2>::OrthogonalTransformation(const UniformScalingTransformation<double,2>&);

template class OrthogonalTransformation<double,2>;
template OrthogonalTransformation<double,2>::OrthogonalTransformation(const TranslationTransformation<float,2>&);
template OrthogonalTransformation<double,2>::OrthogonalTransformation(const RotationTransformation<float,2>&);
template OrthogonalTransformation<double,2>::OrthogonalTransformation(const OrthonormalTransformation<float,2>&);
template OrthogonalTransformation<double,2>::OrthogonalTransformation(const UniformScalingTransformation<float,2>&);
template OrthogonalTransformation<double,2>::OrthogonalTransformation(const TranslationTransformation<double,2>&);
template OrthogonalTransformation<double,2>::OrthogonalTransformation(const RotationTransformation<double,2>&);
template OrthogonalTransformation<double,2>::OrthogonalTransformation(const OrthonormalTransformation<double,2>&);
template OrthogonalTransformation<double,2>::OrthogonalTransformation(const UniformScalingTransformation<double,2>&);

template class OrthogonalTransformation<float,3>;
template OrthogonalTransformation<float,3>::OrthogonalTransformation(const TranslationTransformation<float,3>&);
template OrthogonalTransformation<float,3>::OrthogonalTransformation(const RotationTransformation<float,3>&);
template OrthogonalTransformation<float,3>::OrthogonalTransformation(const OrthonormalTransformation<float,3>&);
template OrthogonalTransformation<float,3>::OrthogonalTransformation(const UniformScalingTransformation<float,3>&);
template OrthogonalTransformation<float,3>::OrthogonalTransformation(const TranslationTransformation<double,3>&);
template OrthogonalTransformation<float,3>::OrthogonalTransformation(const RotationTransformation<double,3>&);
template OrthogonalTransformation<float,3>::OrthogonalTransformation(const OrthonormalTransformation<double,3>&);
template OrthogonalTransformation<float,3>::OrthogonalTransformation(const UniformScalingTransformation<double,3>&);

template class OrthogonalTransformation<double,3>;
template OrthogonalTransformation<double,3>::OrthogonalTransformation(const TranslationTransformation<float,3>&);
template OrthogonalTransformation<double,3>::OrthogonalTransformation(const RotationTransformation<float,3>&);
template OrthogonalTransformation<double,3>::OrthogonalTransformation(const OrthonormalTransformation<float,3>&);
template OrthogonalTransformation<double,3>::OrthogonalTransformation(const UniformScalingTransformation<float,3>&);
template OrthogonalTransformation<double,3>::OrthogonalTransformation(const TranslationTransformation<double,3>&);
template OrthogonalTransformation<double,3>::OrthogonalTransformation(const RotationTransformation<double,3>&);
template OrthogonalTransformation<double,3>::OrthogonalTransformation(const OrthonormalTransformation<double,3>&);
template OrthogonalTransformation<double,3>::OrthogonalTransformation(const UniformScalingTransformation<double,3>&);

#endif

}
