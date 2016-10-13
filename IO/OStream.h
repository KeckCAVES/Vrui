/***********************************************************************
OStream - Class to layer a std::ostream over an IO::File.
Copyright (c) 2015 Oliver Kreylos

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

#ifndef IO_OSTREAM_INCLUDED
#define IO_OSTREAM_INCLUDED

#include <iostream>
#include <IO/File.h>

namespace IO {

class OStream:public std::ostream
	{
	/* Embedded classes: */
	private:
	class FileBuf:public std::streambuf // Custom stream buffer class to redirect output from a std::ostream to an IO::File
		{
		/* Elements: */
		private:
		IO::FilePtr file; // Pointer to underlying file object
		
		/* Protected methods from std::streambuf: */
		protected:
		virtual int sync(void);
		virtual std::streamsize xsputn(const char* s,std::streamsize n);
		virtual int overflow(int c);
		
		/* Constructors and destructors: */
		public:
		FileBuf(IO::FilePtr sFile); // Creates file buffer for given open file
		virtual ~FileBuf(void); // Destroys the file buffer
		};
	
	/* Elements: */
	private:
	FileBuf fb; // The stream buffer
	
	/* Constructors and destructors: */
	public:
	OStream(IO::FilePtr file); // Creates a std::ostream wrapper around the given file
	~OStream(void);
	};

}

#endif
