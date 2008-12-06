/***********************************************************************
Constants - Classes providing generic access to type-specific math-
relevant information.
Copyright (c) 2003-2005 Oliver Kreylos

This file is part of the Templatized Math Library (Math).

The Templatized Math Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Templatized Math Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Templatized Math Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <float.h>

#include <Math/Constants.h>

namespace Math {

/****************************************
Static members of class Constants<float>:
****************************************/

const Constants<float>::Scalar Constants<float>::zero=0.0f;
const Constants<float>::Scalar Constants<float>::one=1.0f;
const Constants<float>::Scalar Constants<float>::min=-FLT_MAX;
const Constants<float>::Scalar Constants<float>::max=FLT_MAX;
const Constants<float>::Scalar Constants<float>::smallest=FLT_MIN;
const Constants<float>::Scalar Constants<float>::epsilon=FLT_EPSILON;
const Constants<float>::Scalar Constants<float>::e=2.7182818284590452354f;
const Constants<float>::Scalar Constants<float>::pi=3.14159265358979323846f;

/*****************************************
Static members of class Constants<double>:
*****************************************/

const Constants<double>::Scalar Constants<double>::zero=0.0;
const Constants<double>::Scalar Constants<double>::one=1.0;
const Constants<double>::Scalar Constants<double>::min=-DBL_MAX;
const Constants<double>::Scalar Constants<double>::max=DBL_MAX;
const Constants<double>::Scalar Constants<double>::smallest=DBL_MIN;
const Constants<double>::Scalar Constants<double>::epsilon=DBL_EPSILON;
const Constants<double>::Scalar Constants<double>::e=2.7182818284590452354;
const Constants<double>::Scalar Constants<double>::pi=3.14159265358979323846;

}
