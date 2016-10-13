/***********************************************************************
OGTransformCalculator - Helper function to calculate an orthogonal
transformation (translation + rotation + uniform scaling) that
transforms one ordered point set into another ordered point set.
Copyright (c) 2015 Oliver Kreylos

This file is part of the Vrui calibration utility package.

The Vrui calibration utility package is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Vrui calibration utility package is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui calibration utility package; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef OGTRANSFORMCALCULATOR_INCLUDED
#define OGTRANSFORMCALCULATOR_INCLUDED

#include <utility>
#include <vector>
#include <Geometry/Point.h>
#include <Geometry/OrthogonalTransformation.h>

template <class ScalarParam>
std::pair<Geometry::OrthogonalTransformation<double,3>,double>
calculateOGTransform(
	const std::vector<Geometry::Point<ScalarParam,3> >& points0,
	const std::vector<Geometry::Point<ScalarParam,3> >& points1);

#ifndef OGTRANSFORMCALCULATOR_IMPLEMENTATION
#include "OGTransformCalculator.icpp"
#endif

#endif
