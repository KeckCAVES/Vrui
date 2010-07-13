/***********************************************************************
ValueCoder - Generic class to encode/decode values into/from human-
readable strings.
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

#include <ctype.h>

#include <Misc/ValueCoder.h>

namespace Misc {

/****************
Helper functions:
****************/

const char* skipWhitespace(const char* start,const char* end)
	{
	/* Skip whitespace characters: */
	while(start!=end&&isspace(*start))
		++start;
	
	/* Return pointer to end of whitespace: */
	return start;
	}

const char* skipSeparator(char separator,const char* start,const char* end)
	{
	const char* cPtr=start;
	
	/* Skip whitespace characters: */
	while(cPtr!=end&&isspace(*cPtr))
		++cPtr;
	
	/* Check if separator character is found: */
	if(cPtr==end||*cPtr!=separator)
		throw DecodingError("Missing separator character");
	++cPtr;
	
	/* Skip whitespace characters: */
	while(cPtr!=end&&isspace(*cPtr))
		++cPtr;
	
	/* Return pointer to end of whitespace: */
	return cPtr;
	}

}
