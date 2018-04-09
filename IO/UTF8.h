/***********************************************************************
UTF8 - Helper class to encode/decode Unicode characters to/from UTF-8.
Copyright (c) 2018 Oliver Kreylos

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

#ifndef UTF8_INCLUDED
#define UTF8_INCLUDED

#include <string>
#include <stdexcept>
#include <IO/File.h>

namespace IO {

class UTF8
	{
	/* Methods: */
	public:
	static int read(File& source) // Reads the next complete Unicode character from the given UTF-8 encoded file
		{
		/* Read the first byte and check for end-of-file: */
		int result=source.getChar();
		if(result<0)
			return result;
		
		/* Check if there are additional bytes encoding the current character: */
		if(result&0x80)
			{
			/* Calculate the number of additional bytes to read: */
			int numContinuationBytes;
			if((result&0xe0)==0xc0) // Byte starts with 110
				numContinuationBytes=1;
			else if((result&0xf0)==0xe0) // Byte starts with 1110
				numContinuationBytes=2;
			else if((result&0xf8)==0xf0) // Byte starts with 11110
				numContinuationBytes=3;
			else if((result&0xc0)==0x80) // Byte starts with 10
				throw std::runtime_error("IO::UTF8::read: Synchronization lost");
			else
				throw std::runtime_error("IO::UTF8::read: Invalid code byte");
			
			/* Read the continuation bytes: */
			while(numContinuationBytes>0)
				{
				/* Read the next byte and check for end-of-file: */
				int byte=source.getChar();
				if(byte<0)
					throw std::runtime_error("IO::UTF8::read: Truncated character");
				
				/* Check for a continuation byte: */
				if((result&0xc0)!=0x80)
					throw std::runtime_error("IO::UTF8::read: Invalid code byte");
				
				/* Append the byte to the character code: */
				result=(result<<6)|(byte&0x3f);
				}
			}
		
		return result;
		}
	static int decode(std::string::const_iterator begin,std::string::const_iterator end) // Reads the next complete Unicode character from the given UTF-8 encoded string
		{
		/* Read the first byte: */
		int result=*(begin++);
		
		/* Check if there are additional bytes encoding the current character: */
		if(result&0x80)
			{
			/* Calculate the number of additional bytes to read: */
			int numContinuationBytes;
			if((result&0xe0)==0xc0) // Byte starts with 110
				numContinuationBytes=1;
			else if((result&0xf0)==0xe0) // Byte starts with 1110
				numContinuationBytes=2;
			else if((result&0xf8)==0xf0) // Byte starts with 11110
				numContinuationBytes=3;
			else if((result&0xc0)==0x80) // Byte starts with 10
				throw std::runtime_error("IO::UTF8::decode: Synchronization lost");
			else
				throw std::runtime_error("IO::UTF8::decode: Invalid code byte");
			
			/* Read the continuation bytes: */
			while(numContinuationBytes>0)
				{
				/* Check for end-of-string: */
				if(begin==end)
					throw std::runtime_error("IO::UTF8::decode: Truncated character");
					
				/* Read the next byte: */
				int byte=*(begin++);
				
				/* Check for a continuation byte: */
				if((result&0xc0)!=0x80)
					throw std::runtime_error("IO::UTF8::decode: Invalid code byte");
				
				/* Append the byte to the character code: */
				result=(result<<6)|(byte&0x3f);
				}
			}
		
		return result;
		}
	static void write(int c,File& dest) // Encodes the given Unicode character into UTF-8 and writes the encoding to the given file
		{
		/* Determine the length of the encoded character: */
		if(c<0x80) // 7 significant bits
			{
			/* Encode as single byte: */
			dest.putChar(c); // Encode all 7 bits
			}
		else if(c<0x800) // 11 significant bits
			{
			/* Encode as two bytes: */
			char b1=(c&0x3f)|0x80; // Encode lowest 6 bits
			c>>=6;
			char b0=c|0xc0; // Encode highest 5 bits
			dest.putChar(b0);
			dest.putChar(b1);
			}
		else if(c<0x10000) // 16 significant bits
			{
			/* Encode as three bytes: */
			char b2=(c&0x3f)|0x80; // Encode lowest 6 bits
			c>>=6;
			char b1=(c&0x3f)|0x80; // Encode next 6 bits
			c>>=6;
			char b0=c|0xe0; // Encode highest 4 bits
			dest.putChar(b0);
			dest.putChar(b1);
			dest.putChar(b2);
			}
		else // c>=0x10000 // 21 significant bits
			{
			/* Encode as four bytes: */
			char b3=(c&0x3f)|0x80; // Encode lowest 6 bits
			c>>=6;
			char b2=(c&0x3f)|0x80; // Encode next 6 bits
			c>>=6;
			char b1=(c&0x3f)|0x80; // Encode next 6 bits
			c>>=6;
			char b0=c|0xf0; // Encode highest 3 bits
			dest.putChar(b0);
			dest.putChar(b1);
			dest.putChar(b2);
			dest.putChar(b3);
			}
		}
	static void encode(int c,std::string& string) // Encodes the given Unicode character into UTF-8 and appends the encoding to the given string
		{
		/* Determine the length of the encoded character: */
		if(c<0x80) // 7 significant bits
			{
			/* Encode as single byte: */
			string.push_back(c); // Encode all 7 bits
			}
		else if(c<0x800) // 11 significant bits
			{
			/* Encode as two bytes: */
			char b1=(c&0x3f)|0x80; // Encode lowest 6 bits
			c>>=6;
			char b0=c|0xc0; // Encode highest 5 bits
			string.push_back(b0);
			string.push_back(b1);
			}
		else if(c<0x10000) // 16 significant bits
			{
			/* Encode as three bytes: */
			char b2=(c&0x3f)|0x80; // Encode lowest 6 bits
			c>>=6;
			char b1=(c&0x3f)|0x80; // Encode next 6 bits
			c>>=6;
			char b0=c|0xe0; // Encode highest 4 bits
			string.push_back(b0);
			string.push_back(b1);
			string.push_back(b2);
			}
		else // c>=0x10000 // 21 significant bits
			{
			/* Encode as four bytes: */
			char b3=(c&0x3f)|0x80; // Encode lowest 6 bits
			c>>=6;
			char b2=(c&0x3f)|0x80; // Encode next 6 bits
			c>>=6;
			char b1=(c&0x3f)|0x80; // Encode next 6 bits
			c>>=6;
			char b0=c|0xf0; // Encode highest 3 bits
			string.push_back(b0);
			string.push_back(b1);
			string.push_back(b2);
			string.push_back(b3);
			}
		}
	};

}

#endif
