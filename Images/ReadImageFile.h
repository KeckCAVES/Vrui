/***********************************************************************
ReadImageFile - Functions to read RGB images from a variety of file
formats.
Copyright (c) 2005-2018 Oliver Kreylos

This file is part of the Image Handling Library (Images).

The Image Handling Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Image Handling Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Image Handling Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef IMAGES_READIMAGEFILE_INCLUDED
#define IMAGES_READIMAGEFILE_INCLUDED

#include <IO/File.h>
#include <Images/BaseImage.h>
#include <Images/RGBImage.h>
#include <Images/RGBAImage.h>

/* Forward declarations: */
namespace IO {
class Directory;
}

namespace Images {

bool canReadImageFileType(const char* imageFileName); // Returns true if the image reader supports the image file's type

RGBImage readImageFile(const char* imageFileName,IO::FilePtr file); // Reads an RGB image from an already-open file; auto-detects file format based on file name extension
RGBImage readImageFile(const IO::Directory& directory,const char* imageFileName); // Ditto, but opens the file itself relative to the given directory
RGBImage readImageFile(const char* imageFileName); // Ditto, but opens the given file itself relative to the current directory

RGBAImage readTransparentImageFile(const char* imageFileName,IO::FilePtr file); // Reads an RGB image with alpha layer from an already-open file; auto-detects file format based on file name extension
RGBAImage readTransparentImageFile(const char* imageFileName); // Ditto, but opens the given file itself relative to the current directory

BaseImage readGenericImageFile(const char* imageFileName,IO::FilePtr file); // Reads a generic image of arbitrary number of channels or channel types from an already-open file; auto-detects file format based on file name extension
BaseImage readGenericImageFile(const IO::Directory& directory,const char* imageFileName); // Ditto, but opens the file itself relative to the given directory
BaseImage readGenericImageFile(const char* imageFileName); // Ditto, but opens the file itself relative to the current directory

RGBAImage readCursorFile(const char* cursorFileName,IO::FilePtr file,unsigned int nominalSize,unsigned int* hotspot =0); // Reads an RGBA image from a cursor file in Xcursor format
RGBAImage readCursorFile(const IO::Directory& directory,const char* cursorFileName,unsigned int nominalSize,unsigned int* hotspot =0); // Ditto, but opens the file itself relative to the given directory
RGBAImage readCursorFile(const char* cursorFileName,unsigned int nominalSize,unsigned int* hotspot =0); // Ditto, but opens the file itself relative to the current directory

}

#endif
