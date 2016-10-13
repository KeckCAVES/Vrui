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

#include <IO/OStream.h>

namespace IO {

/*********************************
Methods of class OStream::FileBuf:
*********************************/

int OStream::FileBuf::sync(void)
	{
	/* Finish writing directly into the buffer: */
	file->writeInBufferFinish(pptr()-pbase());
	
	/* Flush the buffer: */
	file->flush();
	
	/* Reset the write buffer pointers: */
	void* buffer;
	size_t bufferSize=file->writeInBufferPrepare(buffer);
	char* cBuffer=static_cast<char*>(buffer);
	setp(cBuffer,cBuffer+bufferSize);
	
	return 0;
	}

std::streamsize OStream::FileBuf::xsputn(const char* s,std::streamsize n)
	{
	/* Finish writing directly into the buffer: */
	file->writeInBufferFinish(pptr()-pbase());
	
	/* Write the given string into the buffer or directly to file: */
	file->writeRaw(s,n);
	
	/* Reset the write buffer pointers: */
	void* buffer;
	size_t bufferSize=file->writeInBufferPrepare(buffer);
	char* cBuffer=static_cast<char*>(buffer);
	setp(cBuffer,cBuffer+bufferSize);
	
	return n;
	}

int OStream::FileBuf::overflow(int c)
	{
	/* Finish writing directly into the buffer: */
	file->writeInBufferFinish(pptr()-pbase());
	
	/* Reset the write buffer pointers: */
	void* buffer;
	size_t bufferSize=file->writeInBufferPrepare(buffer);
	char* cBuffer=static_cast<char*>(buffer);
	setp(cBuffer,cBuffer+bufferSize);
	
	/* Put the overflow character into the buffer: */
	cBuffer[0]=char(c);
	pbump(1);
	
	return traits_type::to_int_type(c);
	}

OStream::FileBuf::FileBuf(IO::FilePtr sFile)
	:file(sFile)
	{
	/* Set up the streambuf's buffer pointers: */
	void* buffer;
	size_t bufferSize=file->writeInBufferPrepare(buffer);
	char* cBuffer=static_cast<char*>(buffer);
	setp(cBuffer,cBuffer+bufferSize);
	}

OStream::FileBuf::~FileBuf(void)
	{
	/* Finish writing directly into the buffer: */
	file->writeInBufferFinish(pptr()-pbase());
	
	/* Flush the buffer: */
	file->flush();
	}

/************************
Methods of class OStream:
************************/

OStream::OStream(IO::FilePtr file)
	:std::ostream(0),
	 fb(file)
	{
	/* Set the ostream object's stream buffer: */
	rdbuf(&fb);
	}

OStream::~OStream(void)
	{
	/* Reset the ostream object's stream buffer: */
	rdbuf(0);
	}

}
