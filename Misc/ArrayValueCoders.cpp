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

#define MISC_ARRAYVALUECODERS_IMPLEMENTATION

#include <ctype.h>

#include <Misc/ArrayValueCoders.h>

namespace Misc {

/**********************************************
Methods of class ValueCoderArray<ElementParam>:
**********************************************/

template <class ElementParam>
std::string ValueCoderArray<ElementParam>::encode(int numElements,const ElementParam* elements)
	{
	std::string result="(";
	bool notFirstItem=false;
	for(int i=0;i<numElements;++i)
		{
		/* Put a separator between items: */
		if(notFirstItem)
			result+=", ";
		notFirstItem=true;
		
		/* Concatenate the result of encoding the next item: */
		result+=ValueCoder<ElementParam>::encode(elements[i]);
		}
	result+=")";
	
	return result;
	}

template <class ElementParam>
inline int ValueCoderArray<ElementParam>::decode(int maxNumElements,ElementParam* elements,const char* start,const char* end,const char** decodeEnd)
	{
	const char* cPtr=start;
	
	/* Check for opening parenthesis: */
	if(cPtr==end||*cPtr!='(')
		throw DecodingError(std::string("Missing opening parenthesis in ")+std::string(start,end));
	++cPtr;
	
	cPtr=skipWhitespace(cPtr,end);
	
	int numElements=0;
	while(cPtr!=end&&*cPtr!=')')
		{
		if(numElements>0)
			{
			/* Check for comma separator: */
			if(*cPtr!=',')
				throw DecodingError(std::string("Missing comma separator in ")+std::string(start,end));
			++cPtr;
			
			cPtr=skipWhitespace(cPtr,end);
			}
		
		/* Decode next list item (but only store it if there is room): */
		if(numElements<maxNumElements)
			elements[numElements]=ValueCoder<ElementParam>::decode(cPtr,end,&cPtr);
		else
			ValueCoder<ElementParam>::decode(cPtr,end,&cPtr);
		
		cPtr=skipWhitespace(cPtr,end);
		
		++numElements;
		}
	
	/* Check for closing parenthesis: */
	if(cPtr==end)
		throw DecodingError(std::string("Missing closing parenthesis in ")+std::string(start,end));
	++cPtr;
	
	if(decodeEnd!=0)
		*decodeEnd=cPtr;
	return numElements;
	}

}
