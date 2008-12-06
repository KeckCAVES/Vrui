/***********************************************************************
ArrayValueCoders - Generic value coder classes standard C-style arrays.
Copyright (c) 2004-2005 Oliver Kreylos

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

}

#if !defined(MISC_ARRAYVALUECODERS_IMPLEMENTATION)
#include <Misc/ArrayValueCoders.cpp>
#endif

#endif
