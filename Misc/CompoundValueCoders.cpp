/***********************************************************************
CompoundValueCoders - Generic value coder classes for vectors and lists
of other data types.
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

#define MISC_COMPOUNDVALUECODERS_IMPLEMENTATION

#include <ctype.h>

#include <Misc/CompoundValueCoders.h>

namespace Misc {

/***************************************************
Methods of class ValueCoder<std::list<ValueParam> >:
***************************************************/

template <class ValueParam>
inline std::string ValueCoder<std::list<ValueParam> >::encode(const std::list<ValueParam>& value)
	{
	std::string result="(";
	bool notFirstItem=false;
	for(typename std::list<ValueParam>::const_iterator it=value.begin();it!=value.end();++it)
		{
		/* Put a separator between items: */
		if(notFirstItem)
			result+=", ";
		notFirstItem=true;
		
		/* Concatenate the result of encoding the next item: */
		result+=ValueCoder<ValueParam>::encode(*it);
		}
	result+=")";
	
	return result;
	}

template <class ValueParam>
inline std::list<ValueParam> ValueCoder<std::list<ValueParam> >::decode(const char* start,const char* end,const char** decodeEnd)
	{
	std::list<ValueParam> result;
	const char* cPtr=start;
	
	/* Check for opening parenthesis: */
	if(cPtr==end||*cPtr!='(')
		throw DecodingError(std::string("Missing opening parenthesis in ")+std::string(start,end));
	++cPtr;
	
	cPtr=skipWhitespace(cPtr,end);
	
	bool notFirstItem=false;
	while(cPtr!=end&&*cPtr!=')')
		{
		if(notFirstItem)
			{
			/* Check for comma separator: */
			if(*cPtr!=',')
				throw DecodingError(std::string("Missing comma separator in ")+std::string(start,end));
			++cPtr;
			
			cPtr=skipWhitespace(cPtr,end);
			}
		notFirstItem=true;
		
		/* Decode next list item: */
		result.push_back(ValueCoder<ValueParam>::decode(cPtr,end,&cPtr));
		
		cPtr=skipWhitespace(cPtr,end);
		}
	
	/* Check for closing parenthesis: */
	if(cPtr==end)
		throw DecodingError(std::string("Missing closing parenthesis in ")+std::string(start,end));
	++cPtr;
	
	if(decodeEnd!=0)
		*decodeEnd=cPtr;
	return result;
	}

/*****************************************************
Methods of class ValueCoder<std::vector<ValueParam> >:
*****************************************************/

template <class ValueParam>
inline std::string ValueCoder<std::vector<ValueParam> >::encode(const std::vector<ValueParam>& value)
	{
	std::string result="(";
	bool notFirstItem=false;
	for(typename std::vector<ValueParam>::const_iterator it=value.begin();it!=value.end();++it)
		{
		/* Put a separator between items: */
		if(notFirstItem)
			result+=", ";
		notFirstItem=true;
		
		/* Concatenate the result of encoding the next item: */
		result+=ValueCoder<ValueParam>::encode(*it);
		}
	result+=")";
	
	return result;
	}

template <class ValueParam>
inline std::vector<ValueParam> ValueCoder<std::vector<ValueParam> >::decode(const char* start,const char* end,const char** decodeEnd)
	{
	std::vector<ValueParam> result;
	const char* cPtr=start;
	
	/* Check for opening parenthesis: */
	if(cPtr==end||*cPtr!='(')
		throw DecodingError(std::string("Missing opening parenthesis in ")+std::string(start,end));
	++cPtr;
	
	cPtr=skipWhitespace(cPtr,end);
	
	bool notFirstItem=false;
	while(cPtr!=end&&*cPtr!=')')
		{
		if(notFirstItem)
			{
			/* Check for comma separator: */
			if(*cPtr!=',')
				throw DecodingError(std::string("Missing comma separator in ")+std::string(start,end));
			++cPtr;
			
			cPtr=skipWhitespace(cPtr,end);
			}
		notFirstItem=true;
		
		/* Decode next list item: */
		result.push_back(ValueCoder<ValueParam>::decode(cPtr,end,&cPtr));
		
		cPtr=skipWhitespace(cPtr,end);
		}
	
	/* Check for closing parenthesis: */
	if(cPtr==end)
		throw DecodingError(std::string("Missing closing parenthesis in ")+std::string(start,end));
	++cPtr;
	
	if(decodeEnd!=0)
		*decodeEnd=cPtr;
	return result;
	}

}
