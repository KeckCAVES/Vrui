/***********************************************************************
OpenFile - Convenience functions to open files of several types using
the File abstraction.
Copyright (c) 2011 Oliver Kreylos

This file is part of the I/O Support Library (IO).

The I/O Support Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The I/O Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the I/O Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef IO_OPENFILE_INCLUDED
#define IO_OPENFILE_INCLUDED

#include <Misc/SelfDestructPointer.h>
#include <IO/File.h>
#include <IO/SeekableFile.h>

namespace IO {

/* Type definition for self-destructing file objects: */
typedef Misc::SelfDestructPointer<File> AutoFile;
typedef Misc::SelfDestructPointer<SeekableFile> AutoSeekableFile;

File* openFile(const char* fileName,File::AccessMode accessMode =File::ReadOnly); // Opens a file of the given name
SeekableFile* openSeekableFile(const char* fileName,File::AccessMode accessMode =File::ReadOnly); // Opens a seekable file of the given name

}

#endif
