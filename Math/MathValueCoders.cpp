/***********************************************************************
MathValueCoders - Value coder classes for math objects.
Copyright (c) 2009 Oliver Kreylos

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

#define MATH_MATHVALUECODERS_IMPLEMENTATION

#ifndef METHODPREFIX
	#ifdef NONSTANDARD_TEMPLATES
		#define METHODPREFIX inline
	#else
		#define METHODPREFIX
	#endif
#endif

#include <Math/MathValueCoders.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <Math/BrokenLine.h>

namespace Misc {

/***********************************************************
Methods of class ValueCoder<Math::BrokenLine<ScalarParam> >:
***********************************************************/

template <class ScalarParam>
METHODPREFIX
std::string
ValueCoder<Math::BrokenLine<ScalarParam> >::encode(
	const Math::BrokenLine<ScalarParam>& value)
	{
	/* Encode the broken line as a vector with four elements: */
	ScalarParam components[4];
	components[0]=value.min;
	components[1]=value.deadMin;
	components[2]=value.deadMax;
	components[3]=value.max;
	return ValueCoderArray<ScalarParam>::encode(4,components);
	}

template <class ScalarParam>
METHODPREFIX
Math::BrokenLine<ScalarParam>
ValueCoder<Math::BrokenLine<ScalarParam> >::decode(
	const char* start,
	const char* end,
	const char** decodeEnd)
	{
	try
		{
		/* Decode the broken line as a vector with four elements: */
		ScalarParam components[4];
		int numComponents=ValueCoderArray<ScalarParam>::decode(4,components,start,end,decodeEnd);
		if(numComponents!=4)
			throw 42;
		Math::BrokenLine<ScalarParam> result;
		result.min=components[0];
		result.deadMin=components[1];
		result.deadMax=components[2];
		result.max=components[3];
		return result;
		}
	catch(...)
		{
		throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to Math::BrokenLine<Scalar>"));
		}
	}


#if !defined(NONSTANDARD_TEMPLATES)

/******************************************************
Force instantiation of all standard ValueCoder classes:
******************************************************/

template class ValueCoder<Math::BrokenLine<float> >;
template class ValueCoder<Math::BrokenLine<double> >;

#endif

}
