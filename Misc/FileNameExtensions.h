/***********************************************************************
FileNameExtensions - Helper functions to extract or test extensions from
file names.
Copyright (c) 2009 Oliver Kreylos

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

#ifndef MISC_FILENAMEEXTENSIONS_INCLUDED
#define MISC_FILENAMEEXTENSIONS_INCLUDED

namespace Misc {

const char* getExtension(const char* fileName); // Returns a pointer to the extension of the last component of the given file / path name; returns NULL if no extension
bool hasExtension(const char* fileName,const char* extension); // Returns true if the extension of the last component of the given file / path name matches the given extension (period included, empty string matches against no extension)
bool hasCaseExtension(const char* fileName,const char* extension); // Ditto, with case-insensitive comparison

}

#endif
