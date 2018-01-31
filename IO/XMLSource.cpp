/***********************************************************************
XMLSource - Class implementing a low-level XML file processor.
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

#include <IO/XMLSource.h>

#include <string.h>
#include <stdexcept>

namespace IO {

namespace {

/*********************************************************************
Helper functions to read encoded Unicode characters from source files:
*********************************************************************/

int readCharUTF8(File& source)
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
			throw std::runtime_error("readCharUTF8: Synchronization lost");
		else
			throw std::runtime_error("readCharUTF8: Invalid code byte");
		
		/* Read the continuation bytes: */
		while(numContinuationBytes>0)
			{
			/* Read the next byte and check for end-of-file: */
			int byte=source.getChar();
			if(byte<0)
				throw std::runtime_error("readCharUTF8: Truncated character");
			
			/* Check for a continuation byte: */
			if((result&0xc0)!=0x80)
				throw std::runtime_error("readCharUTF8: Invalid code byte");
			
			/* Append the byte to the character code: */
			result=(result<<6)|(byte&0x3f);
			}
		}
	
	return result;
	}

int readCharUTF16LE(File& source)
	{
	/* Read the first byte and check for end-of-file: */
	int byte0=source.getChar();
	if(byte0<0)
		return byte0;
	
	/* Read the second byte: */
	int byte1=source.getChar();
	bool ok=byte1>=0;
	
	if(ok)
		{
		/* Assemble the first 16-bit code unit: */
		int unit0=(byte1<<8)|byte0;
		
		/* Check for surrogate code units: */
		if(unit0<0xdc00)
			{
			if(unit0<0xd800)
				return unit0;
			else // unit0>=0xd800
				{
				/* Read the third and fourth bytes: */
				ok=ok&&(byte0=source.getChar())>=0;
				ok=ok&&(byte1=source.getChar())>=0;
				
				if(ok)
					{
					/* Assemble the second 16-bit code unit: */
					int unit1=(byte1<<8)|byte0;
					if(unit1<0xdc00||unit1>=0xe000)
						throw std::runtime_error("readCharUTF16LE: Invalid code unit");
					
					/* Assemble the code point: */
					return ((unit0-0xd800)<<10)|(unit1=0xdc00);
					}
				}
			}
		else // result>=0xdc00
			{
			if(unit0<0xe000)
				throw std::runtime_error("readCharUTF16LE: Synchronization lost");
			else // unit0>=0xe000
				return unit0;
			}
		}
	
	throw std::runtime_error("readCharUTF16LE: Truncated character");
	}

int readCharUTF16BE(File& source)
	{
	/* Read the first byte and check for end-of-file: */
	int byte0=source.getChar();
	if(byte0<0)
		return byte0;
	
	/* Read the second byte: */
	int byte1=source.getChar();
	bool ok=byte1>=0;
	
	if(ok)
		{
		/* Assemble the first 16-bit code unit: */
		int unit0=(byte0<<8)|byte1;
		
		/* Check for surrogate code units: */
		if(unit0<0xdc00)
			{
			if(unit0<0xd800)
				return unit0;
			else // unit0>=0xd800
				{
				/* Read the third and fourth bytes: */
				ok=ok&&(byte0=source.getChar())>=0;
				ok=ok&&(byte1=source.getChar())>=0;
				
				if(ok)
					{
					/* Assemble the second 16-bit code unit: */
					int unit1=(byte0<<8)|byte1;
					if(unit1<0xdc00||unit1>=0xe000)
						throw std::runtime_error("readCharUTF16LE: Invalid code unit");
					
					/* Assemble the code point: */
					return ((unit0-0xd800)<<10)|(unit1=0xdc00);
					}
				}
			}
		else // result>=0xdc00
			{
			if(unit0<0xe000)
				throw std::runtime_error("readCharUTF16LE: Synchronization lost");
			else // unit0>=0xe000
				return unit0;
			}
		}
	
	throw std::runtime_error("readCharUTF16LE: Truncated character");
	}

int readCharUCS4BE(File& source)
	{
	return -1;
	}

int readCharUCS4LE(File& source)
	{
	return -1;
	}

int readCharUCS4_2143(File& source)
	{
	return -1;
	}

int readCharUCS4_3412(File& source)
	{
	return -1;
	}

int readCharEBCDIC(File& source)
	{
	return -1;
	}

/***********************************************************************
Helper functions to classify Unicode characters according to XML syntax:
***********************************************************************/

inline bool isSpace(int c)
	{
	/* XML whitespace is space, horizontal tab, and line feed (carriage return is removed on input): */
	return c==0x20||c==0x09||c==0x0a;
	}

inline bool isDigit(int c)
	{
	return c>='0'&&c<='9';
	}

inline bool isHexDigit(int c)
	{
	return (c>='0'&&c<='9')||(c>='A'&&c<='F')||(c>='a'&&c<='f');
	}

inline bool isNameStartChar(int c)
	{
	/* There are sixteen discrete character ranges, so use binary search to keep it fast-ish: */
	if(c<0x37f)
		{
		if(c<0xc0)
			{
			if(c<'_')
				{
				if(c<'A')
					return c==':';
				else // c>='A'
					return c<='Z';
				}
			else // c>='_'
				{
				if(c<'a')
					return c=='_';
				else // c>='a'
					return c<='z';
				}
			}
		else // c>=0xc0
			{
			if(c<0xf8)
				{
				if(c<0xd8)
					return c<=0xd6;
				else // c>=0xd8
					return c<=0xf6;
				}
			else // c>=0xf8
				{
				if(c<0x370)
					return c<=0x2ff;
				else // c>=0x370
					return c<=0x37d;
				}
			}
		}
	else // c>=0x37f
		{
		if(c<0x3001)
			{
			if(c<0x2070)
				{
				if(c<0x200c)
					return c<=0x1fff;
				else
					return c<=0x200d;
				}
			else // c>=0x2070
				{
				if(c<0x2c00)
					return c<=0x218f;
				else // c>=0x2c00
					return c<=0x2fef;
				}
			}
		else // c>=0x3001
			{
			if(c<0xfdf0)
				{
				if(c<0xf900)
					return c<=0xd7ff;
				else // c>=0xf900
					return c<=0xfdcf;
				}
			else // c>=0xfdf0
				{
				if(c<0x10000)
					return c<=0xfffd;
				else // c>=0x10000
					return c<=0xeffff;
				}
			}
		}
	}

inline bool isNameChar(int c)
	{
	/* Check for a name start character first: */
	if(isNameStartChar(c))
		return true;
	
	/* Check the additional six character ranges using binary search: */
	if(c<0xb7)
		{
		if(c<'0')
			return c=='-'||c=='.';
		else // c>='0'
			return c<='9';
		}
	else // c>=0xb7
		{
		if(c<0x203f)
			{
			if(c<0x300)
				return c==0xb7;
			else // c>=0x300
				return c<=0x36f;
			}
		else // c>=0x203f
			return c<=0x2040;
		}
	}

inline bool isQuote(int c)
	{
	/* Single or double quotes allowed: */
	return c=='\''||c=='\"';
	}

}

/**************************
Methods of class XMLSource:
**************************/

void XMLSource::processHeader(void)
	{
	/* Read the first four bytes of the source to determine the initial character encoding: */
	unsigned char h[4];
	source->read(h,4);
	int putback=4; // Number of header bytes to put back into the source after detection
	bool haveBom=false; // Flag whether the XML source started with a Byte Order Mark
	if(h[0]==0x00&&h[1]==0x00&&h[2]==0xfe&&h[3]==0xff) // Byte Order Mark for UCS-4 big endian (1234 order)
		{
		readNextChar=readCharUCS4BE;
		putback=0; // Gobble up the BOM
		haveBom=true;
		}
	else if(h[0]==0xff&&h[1]==0xfe&&h[2]==0x00&&h[3]==0x00) // Byte Order Mark for UCS-4 little endian (4321 order)
		{
		readNextChar=readCharUCS4LE;
		putback=0; // Gobble up the BOM
		haveBom=true;
		}
	else if(h[0]==0x00&&h[1]==0x00&&h[2]==0xff&&h[3]==0xfe) // Byte Order Mark for UCS-4 (2143 order)
		{
		readNextChar=readCharUCS4_2143;
		putback=0; // Gobble up the BOM
		haveBom=true;
		}
	else if(h[0]==0xfe&&h[1]==0xff&&h[2]==0x00&&h[3]==0x00) // Byte Order Mark for UCS-4 (3412 order)
		{
		readNextChar=readCharUCS4_3412;
		putback=0; // Gobble up the BOM
		haveBom=true;
		}
	else if(h[0]==0xfe&&h[1]==0xff) // Byte order mark for UTF-16 big endian
		{
		readNextChar=readCharUTF16BE;
		putback=2; // Gobble up the BOM
		haveBom=true;
		}
	else if(h[0]==0xff&&h[1]==0xfe) // Byte order mark for UTF-16 little endian
		{
		readNextChar=readCharUTF16LE;
		putback=2; // Gobble up the BOM
		haveBom=true;
		}
	else if(h[0]==0xef&&h[1]==0xbb&&h[2]==0xbf) // Redundant Byte order mark for UTF-8
		{
		readNextChar=readCharUTF8;
		putback=1; // Gobble up the BOM
		haveBom=true;
		}
	else if(h[0]==0x00&&h[1]==0x00&&h[2]==0x00&&h[3]==0x3c) // XML tag for UCS-4 big endian (1234 order)
		readNextChar=readCharUCS4BE;
	else if(h[0]==0x3c&&h[1]==0x00&&h[2]==0x00&&h[3]==0x00) // XML tag for UCS-4 little endian (4321 order)
		readNextChar=readCharUCS4LE;
	else if(h[0]==0x00&&h[1]==0x00&&h[2]==0x3c&&h[3]==0x00) // XML tag for UCS-4 (2143 order)
		readNextChar=readCharUCS4_2143;
	else if(h[0]==0x00&&h[1]==0x3c&&h[2]==0x00&&h[3]==0x00) // XML tag for UCS-4 (3412 order)
		readNextChar=readCharUCS4_3412;
	else if(h[0]==0x00&&h[1]==0x3c&&h[2]==0x00&&h[3]==0x3f) // XML tag for UTF-16 big endian or compatible
		readNextChar=readCharUTF16BE;
	else if(h[0]==0x3c&&h[1]==0x00&&h[2]==0x3f&&h[3]==0x00) // XML tag for UTF-16 little endian or compatible
		readNextChar=readCharUTF16LE;
	else if(h[0]==0x3c&&h[1]==0x3f&&h[2]==0x78&&h[3]==0x6d) // XML tag for UTF-8 or compatible
		readNextChar=readCharUTF8;
	else if(h[0]==0x4c&&h[1]==0x6f&&h[2]==0xa7&&h[3]==0x94) // XML tag for some flavor of EBCDIC
		readNextChar=readCharEBCDIC;
	
	/* Put unused characters back into the source: */
	for(int i=0;i<putback;++i)
		source->ungetChar(h[3-i]);
	
	/* Check if the source begins with an XML declaration: */
	if(readAhead(6)&&matchString("<?xml")&&isSpace(peekChar(0)))
		{
		int attributeIndex=-1;
		
		/* Process attribute/value pairs until the closing marker: */
		while(true)
			{
			int c;
			
			/* Skip whitespace: */
			bool haveSpace=false;
			while(isSpace(c=getChar()))
				haveSpace=true;
			
			/* Check for attribute name start or closing marker: */
			if(haveSpace&&isNameStartChar(c))
				{
				/* Put the character back and check the expected attribute names: */
				ungetChar();
				if(attributeIndex<0&&readAhead(7)&&matchString("version"))
					attributeIndex=0;
				else if(attributeIndex<1&&readAhead(8)&&matchString("encoding"))
					attributeIndex=1;
				else if(attributeIndex<2&&readAhead(8)&&matchString("standalone"))
					attributeIndex=2;
				else
					throw std::runtime_error("XMLSource: Unrecognized attribute in XML declaration");
				
				/* Skip whitespace: */
				while(isSpace(c=getChar()))
					;
				
				/* Check for '=': */
				if(c!='=')
					throw std::runtime_error("XMLSource: Missing '=' in XML declaration");
				
				/* Skip whitespace: */
				while(isSpace(c=getChar()))
					;
				
				/* Check for opening quote: */
				if(!isQuote(c))
					throw std::runtime_error("XMLSource: Missing opening quote in XML declaration");
				int quote=c;
				
				/* Parse the attribute value: */
				switch(attributeIndex)
					{
					case 0:
						{
						/* Check the major version number: */
						if(!readAhead(3)||!matchString("1.")||!isDigit(peekChar(0)))
							throw std::runtime_error("XMLSource: Malformed version number in XML declaration");
						
						/* Parse the minor version number: */
						minorVersion=getChar()-'0';
						while(isDigit(c=getChar()))
							minorVersion=minorVersion*10+c-'0';
						
						break;
						}
					
					case 1:
						{
						/* Check known encodings: */
						if(readAhead(6)&&matchStringNoCase("utf-8",false)&&peekChar(5)==quote)
							{
							/* Check if the encoding matches: */
							if(readNextChar!=readCharUTF8)
								throw std::runtime_error("XMLSource: Mismatching character encoding in XML declaration");
							}
						else if(readAhead(7)&&matchStringNoCase("utf-16",false)&&peekChar(6)==quote)
							{
							/* Check if the encoding matches: */
							if(!haveBom)
								throw std::runtime_error("XMLSource: Missing Byte Order Mark for UTF-16 encoding");
							if(readNextChar!=readCharUTF16LE&&readNextChar!=readCharUTF16BE)
								throw std::runtime_error("XMLSource: Mismatching character encoding in XML declaration");
							}
						else
							throw std::runtime_error("XMLSource: Unrecognized character encoding in XML declaration");
						
						/* Consume value: */
						while((c=getChar())!=quote)
							;
						
						break;
						}
					
					case 2:
						{
						/* Check for yes or no: */
						if(readAhead(4)&&matchString("yes",false)&&peekChar(3)==quote)
							standalone=true;
						else if(readAhead(3)&&matchString("no",false)&&peekChar(2)==quote)
							standalone=false;
						else
							throw std::runtime_error("XMLSource: Malformed standalone flag in XML declaration");
						
						/* Consume value: */
						while((c=getChar())!=quote)
							;
						
						break;
						}
					}
				
				/* Check the closing quote: */
				if(c!=quote)
					throw std::runtime_error("XMLSource: Mismatching quotes in XML declaration");
				}
			else if(c=='?'&&getChar()=='>')
				break;
			else
				throw std::runtime_error("XMLSource: Malformed XML declaration");
			}
		}
	}

bool XMLSource::fillBuffer(void)
	{
	/* Re-initialize the buffer pointers: */
	size_t readAheadSize=cbEnd-cbNext;
	cbNext=charBuffer+charBufferSize/2; // Leave room at the beginning to support putback
	cbEnd=cbNext+readAheadSize;
	
	/* Decode characters until the buffer is full or the source is completely read: */
	int* bufferEnd=charBuffer+charBufferSize;
	while(cbEnd!=bufferEnd)
		{
		/* Decode the next character: */
		int c=readNextChar(*source);
		
		/* Check for end-of-file: */
		if(c<0)
			break;
		
		/* Normalize line breaks: */
		if(c!=0x0a||!hadCarriageReturn) // Not a line feed, or the last character was not a carriage return
			{
			/* Put the character into the buffer: */
			*(cbEnd++)=c;
			}
		else if(c==0x0d) // Carriage return
			{
			/* Put a line feed into the buffer: */
			*(cbEnd++)=0x0a;
			}
		
		/* Remember if the just-read character is a carriage return: */
		hadCarriageReturn=c==0x0d;
		}
	
	/* Return true if no characters were decoded, i.e., the source is completely read: */
	return cbNext==cbEnd;
	}

void XMLSource::growBuffer(void)
	{
	/* Double the buffer size and copy the current buffer contents into the second half of the new buffer: */
	size_t newCharBufferSize=charBufferSize*2;
	int* newCharBuffer=new int[newCharBufferSize];
	int* ncbNext=newCharBuffer+charBufferSize;
	size_t readAheadSize=cbEnd-cbNext;
	memcpy(ncbNext,cbNext,readAheadSize*sizeof(int));
	
	/* Delete the old buffer and install the new one: */
	delete[] charBuffer;
	charBufferSize=newCharBufferSize;
	charBuffer=newCharBuffer;
	cbNext=ncbNext;
	cbEnd=cbNext+readAheadSize;
	}

size_t XMLSource::readAheadBuffer(size_t readAheadSize,size_t numChars)
	{
	/* Check if there is enough space in the buffer: */
	if(numChars<=charBufferSize/2)
		{
		/* Move the current buffer contents to the beginning of the buffer: */
		int* ncbNext=charBuffer+charBufferSize/2;
		if(readAheadSize!=0&&ncbNext!=cbNext)
			memcpy(ncbNext,cbNext,readAheadSize*sizeof(int));
		}
	else
		{
		/* Increase the buffer size and copy the current buffer contents into the second half of the new buffer: */
		size_t newCharBufferSize=numChars*2;
		int* newCharBuffer=new int[newCharBufferSize];
		size_t readAheadSize=cbEnd-cbNext;
		memcpy(newCharBuffer+numChars,cbNext,readAheadSize*sizeof(int));
		
		/* Delete the old buffer and install the new one: */
		delete[] charBuffer;
		charBufferSize=newCharBufferSize;
		charBuffer=newCharBuffer;
		}
	
	/* Read more characters into the buffer (fillBuffer will correctly initialize buffer pointers): */
	fillBuffer();
	
	/* Return the amount of read-ahead data now in the buffer: */
	return cbEnd-cbNext;
	}

int XMLSource::parseReference(void)
	{
	/* Check if this is a character reference or an entity reference: */
	int c=getChar();
	if(c=='#')
		{
		/* Parse a character reference: */
		int code=0;
		
		/* Check if it's a hexadecimal character reference: */
		c=getChar();
		if(c=='x')
			{
			/* Parse a hexadecimal character reference: */
			while(isHexDigit(c=getChar()))
				code=code*16+(c<'A'?c-'0':(c<'a'?c-'A':c-'a')+10);
			}
		else
			{
			/* Parse a decimal character reference: */
			while(isDigit(c))
				{
				code=code*10+c-'0';
				c=getChar();
				}
			}
		
		/* Check for terminating semicolon: */
		if(c!=';')
			throw std::runtime_error("XMLSource: Missing ';' in character reference");
		
		/* Check character for validity: */
		bool codeValid=code==0x9||code==0xa||code==0xd||(code>=0x20&&code<=0xd7ff)||(code>=0xe000&&code<=0xfffd)||(code>=0x10000&&code<=0x10ffff);
		if(!codeValid)
			throw std::runtime_error("XMLSource: Illegal character reference");
		
		return code;
		}
	else if(isNameStartChar(c))
		{
		/* Put the character back and parse an entity reference name: */
		ungetChar();
		if(readAhead(3)&&matchString("amp"))
			return '&';
		if(readAhead(2)&&matchString("lt"))
			return '<';
		if(readAhead(2)&&matchString("gt"))
			return '>';
		if(readAhead(4)&&matchString("apos"))
			return '\'';
		if(readAhead(4)&&matchString("quot"))
			return '\"';
		throw std::runtime_error("XMLSource: Entity references not supported");
		}
	else
		throw std::runtime_error("XMLSource: Malformed reference");
	}

void XMLSource::detectNextSyntaxType(void)
	{
	/* Read the next character: */
	int c=getChar();
	
	/* Check for left angle bracket: */
	if(c=='<')
		{
		/* Determine the type of markup: */
		c=getChar();
		if(c=='!')
			{
			/* Distinguish between comments, CDATA sections, and entity declarations: */
			if(readAhead(2)&&matchString("--"))
				{
				/* Parse a comment: */
				syntaxType=Comment;
				}
			else if(readAhead(7)&&matchString("[CDATA["))
				{
				/* Parse a CDATA section: */
				syntaxType=CData;
				}
			else
				throw std::runtime_error("XMLSource: Entitity declarations not supported");
			}
		else if(c=='?')
			{
			/* Check if the next character starts a name: */
			if(isNameStartChar(getChar()))
				{
				/* Put the character back and parse a processing instruction target: */
				ungetChar();
				syntaxType=ProcessingInstructionTarget;
				}
			else
				throw std::runtime_error("XMLSource: Malformed closing tag");
			}
		else if(c=='/')
			{
			/* Check if the next character starts a name: */
			if(isNameStartChar(getChar()))
				{
				/* Put the character back and parse a closing tag name: */
				ungetChar();
				syntaxType=TagName;
				openTag=false;
				}
			else
				throw std::runtime_error("XMLSource: Malformed closing tag");
			}
		else if(isNameStartChar(c))
			{
			/* Put the character back and parse an opening tag name: */
			ungetChar();
			syntaxType=TagName;
			openTag=true;
			}
		else
			throw std::runtime_error("XMLSource: Malformed opening tag");
		}
	else if(c<0)
		{
		/* End of file reached: */
		syntaxType=EndOfFile;
		}
	else
		{
		/* Put the character back and parse character data: */
		ungetChar();
		syntaxType=Content;
		}
	}

void XMLSource::closeAttributeValue(void)
	{
	/* Skip whitespace: */
	int c;
	bool hadSpace=false;
	while(isSpace(c=getChar()))
		hadSpace=true;
	
	/* Detect the next syntax type: */
	if(c=='>')
		{
		/* This was not a self-closing tag: */
		selfCloseTag=false;
		
		/* Detect the next syntax type: */
		detectNextSyntaxType();
		}
	else if(openTag&&c=='/')
		{
		/* Check if the next character is a '>': */
		if(getChar()!='>')
			throw std::runtime_error("XMLSource: Illegal '/' in tag");
		else
			{
			/* Detect the next syntax type: */
			detectNextSyntaxType();
			}
		}
	else if(hadSpace&&isNameStartChar(c))
		{
		/* Put the character back and start parsing an attribute name: */
		ungetChar();
		syntaxType=AttributeName;
		}
	else
		throw std::runtime_error("XMLSource: Malformed tag");
	}

XMLSource::XMLSource(FilePtr sSource)
	:source(sSource),
	 readNextChar(readCharUTF8),
	 charBufferSize(32),charBuffer(new int[charBufferSize]),
	 cbEnd(charBuffer),cbNext(charBuffer),
	 minorVersion(-1),standalone(false),
	 hadCarriageReturn(false)
	{
	try
		{
		/* Process the XML header: */
		processHeader();
		}
	catch(...)
		{
		/* Delete the character buffer and re-throw exception: */
		delete[] charBuffer;
		throw;
		}
	}

XMLSource::~XMLSource(void)
	{
	/* Delete the character buffer: */
	delete[] charBuffer;
	}

int XMLSource::readComment(void)
	{
	/* Read the next character and return it if it's safe: */
	int c=getChar();
	if(c!='-'&&c>=0)
		return c;
	
	/* Check for end-of-file: */
	if(c<0)
		throw std::runtime_error("XMLSource: Unterminated comment at end of file");
	
	/* Check if the next character is another '-': */
	if(getChar()!='-')
		{
		/* False alarm; put the second charater back and return the '-': */
		ungetChar();
		return c;
		}
	else
		{
		/* Check for a proper comment tag close: */
		if(getChar()=='>')
			{
			/* Detect the next syntax type: */
			detectNextSyntaxType();
			return -1;
			}
		else
			throw std::runtime_error("XMLSource: Illegal -- in comment");
		}
	}

int XMLSource::readName(void)
	{
	/* Read the next character and return it if it's a valid name character: */
	int c=getChar();
	if(isNameChar(c))
		return c;
	
	/* Skip whitespace: */
	bool hadSpace=false;
	while(isSpace(c))
		{
		hadSpace=true;
		c=getChar();
		}
	
	/* Determine the next syntax type based on the current syntax type and first non-space character: */
	switch(syntaxType)
		{
		case ProcessingInstructionTarget:
			/* Check if this is the processing instruction's end: */
			if(readAhead(2)&&matchString("?>"))
				{
				/* Detect the next syntax type: */
				detectNextSyntaxType();
				}
			else if(c>=0)
				{
				/* Start reading the processing instruction's content: */
				syntaxType=ProcessingInstructionContent;
				}
			else
				throw std::runtime_error("XMLSource: Unterminated processing instruction at end of file");
			break;
		
		case TagName:
			/* Detect the next syntax type: */
			if(c=='>')
				{
				/* This was not a self-closing tag: */
				selfCloseTag=false;
				
				/* Detect the next syntax type: */
				detectNextSyntaxType();
				}
			else if(openTag&&c=='/')
				{
				/* Check if the next character is a '>': */
				if(getChar()!='>')
					throw std::runtime_error("XMLSource: Illegal '/' in tag");
				else
					{
					/* Detect the next syntax type: */
					detectNextSyntaxType();
					}
				}
			else if(hadSpace&&isNameStartChar(c))
				{
				/* Put the character back and start parsing an attribute name: */
				ungetChar();
				syntaxType=AttributeName;
				}
			else
				throw std::runtime_error("XMLSource: Malformed tag");
			break;
		
		case AttributeName:
			/* Check for assignment: */
			if(c!='=')
				throw std::runtime_error("XMLSource: Missing '=' in tag attribute");
			
			/* Skip whitespace: */
			while(isSpace(c=getChar()))
				;
			
			/* Check for quote: */
			if(isQuote(c))
				{
				/* Remember the quote character and start parsing an attribute value: */
				quote=c;
				syntaxType=AttributeValue;
				
				/* Attribute values are CDATA by default: */
				nmtokens=false;
				
				/* Check if the attribute value should be stripped of spaces: */
				if(nmtokens)
					{
					/* Skip initial whitespace: */
					while(isSpace(getChar()))
						;
					
					/* Put back the non-whitespace character: */
					ungetChar();
					}
				}
			else
				throw std::runtime_error("XMLSource: Missing tag attribute value");
			break;
		
		default:
			; // Just to make compiler happy
		}
	
	return -1;
	}

int XMLSource::readProcessingInstruction(void)
	{
	/* Read the next character and return it if it's safe: */
	int c=getChar();
	if(c!='?'&&c>=0)
		return c;
	
	/* Check for end-of-file: */
	if(c<0)
		throw std::runtime_error("XMLSource: Unterminated processing instruction at end of file");
	
	/* Check if the next character is a '>': */
	if(getChar()!='>')
		{
		/* False alarm; put the second charater back and return the '?': */
		ungetChar();
		return c;
		}
	else
		{
		/* Detect the next syntax type: */
		detectNextSyntaxType();
		return -1;
		}
	}

int XMLSource::readAttributeValue(void)
	{
	/* Read the next character and return it if it's safe: */
	int c=getChar();
	if(c!=quote&&!isSpace(c)&&c!='&'&&c!='<'&&c>=0)
		return c;
	
	/* Handle the special characters: */
	if(c==quote)
		{
		/* Close this attribute value and detect the next syntax type: */
		closeAttributeValue();
		
		return -1;
		}
	else if(isSpace(c))
		{
		/* Check if the attribute value should be stripped of spaces: */
		if(nmtokens)
			{
			/* Skip all subsequent whitespace: */
			while(isSpace(c=getChar()))
				;
			
			/* Check for the closing quote: */
			if(c==quote)
				{
				/* Close this attribute value and detect the next syntax type: */
				closeAttributeValue();
				
				return -1;
				}
			}
		
		/* Convert the whitespace character to an actual space: */
		return 0x20;
		}
	else if(c=='&')
		{
		/* Parse a character or entity reference: */
		int c=parseReference();
		if(c=='<')
			throw std::runtime_error("XMLSource: Illegal '<' in attribute value");
		
		return c;
		}
	else if(c=='<')
		throw std::runtime_error("XMLSource: Illegal '<' in attribute value");
	else
		throw std::runtime_error("XMLSource: Unterminated attribute value at end of file");
	}

int XMLSource::readCharacterData(void)
	{
	/* Read the next character and return it if it's safe: */
	int c=getChar();
	if((syntaxType==CData||(c!='<'&&c!='&'))&&c!=']'&&c>=0)
		return c;
	
	/* Handle the special characters: */
	if(syntaxType==CData&&c==']')
		{
		/* Check if this is a CDATA closing tag: */
		if(readAhead(2)&&matchString("]>"))
			{
			/* Detect the next syntax type: */
			detectNextSyntaxType();
			
			/* Check if the new syntax type is still character data: */
			if(syntaxType==Content||syntaxType==CData)
				{
				/* Return the first character of the character data: */
				return readCharacterData();
				}
			else
				{
				/* Character data is over: */
				return -1;
				}
			}
		else
			{
			/* False alarm; return the closing bracket: */
			return c;
			}
		}
	else if(c=='<')
		{
		/* Put the character back and detect the next syntax type: */
		ungetChar();
		detectNextSyntaxType();
		
		/* Check if the new syntax type is a CDATA section: */
		if(syntaxType==CData)
			{
			/* Return the first character of the CDATA section: */
			return readCharacterData();
			}
		else
			{
			/* Character data is over: */
			return -1;
			}
		}
	else if(c=='&')
		{
		/* Parse a character or entity reference: */
		return parseReference();
		}
	else if(c==']')
		{
		/* Check if this is a misplaced CDATA closing tag: */
		if(readAhead(2)&&matchString("]>"))
			throw std::runtime_error("XMLSource: Illegal ']]>' in character data");
		else
			{
			/* False alarm; return the closing bracket: */
			return c;
			}
		}
	else
		throw std::runtime_error("XMLSource: Unterminated character data at end of file");
	}

}
