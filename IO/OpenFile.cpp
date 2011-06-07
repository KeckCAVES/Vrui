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

#include <IO/OpenFile.h>

#include <Misc/ThrowStdErr.h>
#include <Misc/FileNameExtensions.h>
#include <IO/StandardFile.h>
#include <IO/GzippedFile.h>

namespace IO {

File* openFile(const char* fileName,File::AccessMode accessMode)
	{
	File* result=0;
	
	/* Check if the file name has the .gz extension: */
	if(Misc::hasCaseExtension(fileName,".gz"))
		{
		/* Check if the caller wants to write to the file: */
		if(accessMode!=File::ReadOnly)
			Misc::throwStdErr("IO::openFile: Cannot write to gzipped files");
		
		/* Open a gzip-compressed file: */
		result=new GzippedFile(fileName);
		}
	else
		{
		/* Open a standard file: */
		result=new StandardFile(fileName,accessMode);
		}
	
	/* Return the open file: */
	return result;
	}

SeekableFile* openSeekableFile(const char* fileName,File::AccessMode accessMode)
	{
	SeekableFile* result=0;
	
	/* Check if the file name has the .gz extension: */
	if(Misc::hasCaseExtension(fileName,".gz"))
		{
		/* Seeking in gzipped files not supported: */
		Misc::throwStdErr("IO::openSeekableFile: Cannot seek in gzipped files");
		}
	else
		{
		/* Open a standard file: */
		result=new StandardFile(fileName,accessMode);
		}
	
	/* Return the open file: */
	return result;
	}

}
