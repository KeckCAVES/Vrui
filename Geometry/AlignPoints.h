/***********************************************************************
AlignPoints - Functions to align to sets of matched points using several
types of alignment transformations by minimizing RMS residual error.
These functions aim for high alignment quality and might therefore be
slow.
Copyright (c) 2009-2017 Oliver Kreylos

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

#ifndef GEOMETRY_ALIGNPOINTS_INCLUDED
#define GEOMETRY_ALIGNPOINTS_INCLUDED

#include <vector>

/* Forward declarations: */
namespace Geometry {
template <class ScalarParam,int dimensionParam>
class Point;
template <class ScalarParam,int dimensionParam>
class OrthonormalTransformation;
template <class ScalarParam,int dimensionParam>
class OrthogonalTransformation;
template <class ScalarParam,int dimensionParam>
class ProjectiveTransformation;
}

namespace Geometry {

/***************************************************
Structure to report results of alignment procedures:
***************************************************/

template <class TransformParam>
struct AlignResult
	{
	/* Embedded classes: */
	public:
	typedef TransformParam Transform; // Type of alignment transformation
	
	/* Elements: */
	Transform transform; // The calculated alignment transformation
	double rms; // Root mean squared error residual between point sets after alignment
	double linf; // Maximum distance between any point pair in the point sets after alignment
	};

/*******************
Alignment functions:
*******************/

template <class ScalarParam>
AlignResult<Geometry::OrthonormalTransformation<double,3> > alignPointsONTransform(const std::vector<Point<ScalarParam,3> >& points0,const std::vector<Point<ScalarParam,3> >& points1,unsigned int numIterations); // Aligns point sets using a rigid body transformation
template <class ScalarParam>
AlignResult<Geometry::OrthogonalTransformation<double,3> > alignPointsOGTransform(const std::vector<Point<ScalarParam,3> >& points0,const std::vector<Point<ScalarParam,3> >& points1,unsigned int numIterations); // Aligns point sets using an uniformly-scaled rigid body transformation
template <class ScalarParam>
AlignResult<Geometry::ProjectiveTransformation<double,3> > alignPointsPTransform(const std::vector<Point<ScalarParam,3> >& points0,const std::vector<Point<ScalarParam,3> >& points1,unsigned int numIterations); // Aligns point sets using a projective transformation

}

#if defined(GEOMETRY_NONSTANDARD_TEMPLATES) && !defined(GEOMETRY_ALIGNPOINTS_IMPLEMENTATION)
#include <Geometry/AlignPoints.icpp>
#endif

#endif
