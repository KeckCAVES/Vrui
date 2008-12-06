/***********************************************************************
Math - Genericized versions of standard C math functions.
Copyright (c) 2001-2005 Oliver Kreylos

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

#ifndef MATH_MATH_INCLUDED
#define MATH_MATH_INCLUDED

#include <math.h>
#include <stdlib.h>

/* Check if the implementation provides float versions of math calls: */
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#define MATH_HAVE_FLOAT_CALLS
#endif

namespace Math {

template <class ScalarParam>
inline ScalarParam mul2(ScalarParam value)
	{
	return value+value;
	}

template <class ScalarParam>
inline ScalarParam div2(ScalarParam value)
	{
	return value/ScalarParam(2);
	}

template <>
inline float div2(float value)
	{
	return value*0.5f;
	}

template <>
inline double div2(double value)
	{
	return value*0.5;
	}

template <class ScalarParam>
inline ScalarParam mid(ScalarParam value1,ScalarParam value2)
	{
	return (value1+value2)/ScalarParam(2);
	}

inline float mid(float value1,float value2)
	{
	return (value1+value2)*0.5f;
	}

inline double mid(double value1,double value2)
	{
	return (value1+value2)*0.5;
	}

inline int abs(int value)
	{
	return ::abs(value);
	}

inline float abs(float value)
	{
	#ifdef __ppc__
	return float(fabs(double(value))); // Mac OS doesn't seem to support the float calls
	#else
	return fabsf(value);
	#endif
	}

inline double abs(double value)
	{
	return fabs(value);
	}

inline float floor(float value)
	{
	#ifdef MATH_HAVE_FLOAT_CALLS
	return floorf(value);
	#else
	return float(::floor(double(value)));
	#endif
	}

inline double floor(double value)
	{
	return ::floor(value);
	}

inline float ceil(float value)
	{
	#ifdef MATH_HAVE_FLOAT_CALLS
	return ceilf(value);
	#else
	return float(::ceil(double(value)));
	#endif
	}

inline double ceil(double value)
	{
	return ::ceil(value);
	}

inline int mod(int counter,int denominator)
	{
	return counter%denominator;
	}

inline float mod(float counter,float denominator)
	{
	return float(fmod(counter,denominator));
	}

inline double mod(double counter,double denominator)
	{
	return fmod(counter,denominator);
	}

template <class ScalarParam>
inline ScalarParam rem(ScalarParam counter,ScalarParam denominator)
	{
	ScalarParam result=mod(counter,denominator);
	if(result<ScalarParam(0))
		result+=denominator;
	return result;
	}

template <class ScalarParam>
inline ScalarParam sqr(ScalarParam value)
	{
	return value*value;
	}

inline float sqrt(float value)
	{
	return float(::sqrt(double(value)));
	}

inline double sqrt(double value)
	{
	return ::sqrt(value);
	}

inline float deg(float radians)
	{
	return radians*(180.0f/3.14159265358979323846f);
	}

inline double deg(double radians)
	{
	return radians*(180.0/3.14159265358979323846);
	}

inline float rad(float degrees)
	{
	return degrees*(3.14159265358979323846f/180.0f);
	}

inline double rad(double degrees)
	{
	return degrees*(3.14159265358979323846/180.0);
	}

inline float wrapRad(float radians)
	{
	return radians-float(floor(double(radians)/(2.0*3.14159265358979323846))*(2.0*3.14159265358979323846));
	}

inline double wrapRad(double radians)
	{
	return radians-floor(radians/(2.0*3.14159265358979323846))*(2.0*3.14159265358979323846);
	}

inline float sin(float radians)
	{
	#ifdef MATH_HAVE_FLOAT_CALLS
	return sinf(radians);
	#else
	return float(::sin(double(radians)));
	#endif
	}

inline double sin(double radians)
	{
	return ::sin(radians);
	}

inline float cos(float radians)
	{
	#ifdef MATH_HAVE_FLOAT_CALLS
	return cosf(radians);
	#else
	return float(::cos(double(radians)));
	#endif
	}

inline double tan(double radians)
	{
	return ::tan(radians);
	}

inline float tan(float radians)
	{
	#ifdef MATH_HAVE_FLOAT_CALLS
	return tanf(radians);
	#else
	return float(::tan(double(radians)));
	#endif
	}

inline double cos(double radians)
	{
	return ::cos(radians);
	}

inline float asin(float value)
	{
	#ifdef MATH_HAVE_FLOAT_CALLS
	return asinf(value);
	#else
	return float(::asin(double(value)));
	#endif
	}

inline double asin(double value)
	{
	return ::asin(value);
	}

inline float acos(float value)
	{
	#ifdef MATH_HAVE_FLOAT_CALLS
	return acosf(value);
	#else
	return float(::acos(double(value)));
	#endif
	}

inline double acos(double value)
	{
	return ::acos(value);
	}

inline float atan(float value)
	{
	#ifdef MATH_HAVE_FLOAT_CALLS
	return atanf(value);
	#else
	return float(::atan(double(value)));
	#endif
	}

inline double atan(double value)
	{
	return ::atan(value);
	}

inline float atan2(float counter,float denominator)
	{
	#ifdef MATH_HAVE_FLOAT_CALLS
	return atan2f(counter,denominator);
	#else
	return float(::atan2(double(counter),double(denominator)));
	#endif
	}

inline double atan2(double counter,double denominator)
	{
	return ::atan2(counter,denominator);
	}

inline float log(float value)
	{
	#ifdef MATH_HAVE_FLOAT_CALLS
	return logf(value);
	#else
	return float(::log(double(value)));
	#endif
	}

inline double log(double value)
	{
	return ::log(value);
	}

inline float log10(float value)
	{
	#ifdef MATH_HAVE_FLOAT_CALLS
	return log10f(value);
	#else
	return float(::log10(double(value)));
	#endif
	}

inline double log10(double value)
	{
	return ::log10(value);
	}

inline float exp(float value)
	{
	#ifdef MATH_HAVE_FLOAT_CALLS
	return expf(value);
	#else
	return float(::exp(double(value)));
	#endif
	}

inline double exp(double value)
	{
	return ::exp(value);
	}

inline float pow(float base,float exponent)
	{
	return float(::pow(double(base),double(exponent)));
	}

inline double pow(double base,double exponent)
	{
	return ::pow(base,exponent);
	}

}

#endif
