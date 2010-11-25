/***********************************************************************
ArrayValueCoders - Generic value coder classes for standard C-style
arrays, with fixed or dynamic array sizes.
Copyright (c) 2004-2010 Oliver Kreylos

This file is part of the Miscellaneous Support Library (Misc).

The Miscellaneous Support Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Miscellaneous Support Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Miscellaneous Support Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef MISC_ARRAYVALUECODERS_INCLUDED
#define MISC_ARRAYVALUECODERS_INCLUDED

#include <Misc/ValueCoder.h>

namespace Misc {

template <class ElementParam>
class FixedArrayValueCoder // Value coder class for arrays with a-priori known sizes
	{
	/* Methods: */
	public:
	static std::string encode(const ElementParam* elements,size_t numElements);
	static void decode(ElementParam* elements,size_t numElements,const char* start,const char* end,const char** decodeEnd =0);
	};

template <class ElementParam>
class DynamicArrayValueCoder // Value coder class for arrays with dynamic sizes
	{
	/* Methods: */
	public:
	static std::string encode(const ElementParam* elements,size_t numElements);
	static size_t decode(ElementParam* elements,size_t maxNumElements,const char* start,const char* end,const char** decodeEnd =0);
	};

#if 0

/**************************
Generic ValueCoder classes:
**************************/

template <class ElementParam>
class ValueCoderArray
	{
	/* Methods: */
	public:
	static std::string encode(int numElements,const ElementParam* elements);
	static int decode(int maxNumElements,ElementParam* elements,const char* start,const char* end,const char** decodeEnd =0);
	};

#endif

}

#if !defined(MISC_ARRAYVALUECODERS_IMPLEMENTATION)
#include <Misc/ArrayValueCoders.icpp>
#endif

#endif
