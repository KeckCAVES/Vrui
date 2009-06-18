/***********************************************************************
ASCIIFileReader - Multithreaded version of Misc::ASCIIFileReader; uses a
background thread to read ahead in the input file.
Copyright (c) 2009 Oliver Kreylos

This file is part of the Portable Threading Library (Threads).

The Portable Threading Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Portable Threading Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Portable Threading Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <zlib.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/FileNameExtensions.h>

#include <Threads/ASCIIFileReader.h>

namespace Threads {

/********************************
Methods of class ASCIIFileReader:
********************************/

void ASCIIFileReader::initCharacterClasses(void)
	{
	cc[-1]=NONE; // EOF is nothing; nothing, I tell you!
	
	/* Set up the basic character classes: */
	for(int i=0;i<256;++i)
		{
		cc[i]=NONE;
		if(isspace(i))
			cc[i]|=WHITESPACE;
		else
			cc[i]|=TOKEN|STRING; // Everything that's not a space can be a token or string for now
		cc[i]|=QUOTEDSTRING; // Pretty much everything is a quoted string
		if(isdigit(i))
			cc[i]|=UNSIGNEDINTEGER|INTEGER|FLOATINGPOINT;
		}
	
	/* Set string terminators: */
	cc['\"']&=~(STRING|QUOTEDSTRING); // Quotes terminate strings and quoted strings
	cc['\n']&=~QUOTEDSTRING; // Newlines terminate quoted strings
	
	/* Add sign characters to integer and floating-point class: */
	cc['+']|=UNSIGNEDINTEGER|INTEGER|FLOATINGPOINT;
	cc['-']|=INTEGER|FLOATINGPOINT;
	
	/* Add period and exponent characters to floating-point class: */
	cc['.']|=FLOATINGPOINT;
	cc['e']|=FLOATINGPOINT;
	cc['E']|=FLOATINGPOINT;
	}

void* ASCIIFileReader::readAheadThreadMethod(void)
	{
	Thread::setCancelState(Thread::CANCEL_ENABLE);
	Thread::setCancelType(Thread::CANCEL_ASYNCHRONOUS);
	
	unsigned int lastReadRequest=~0;
	while(true)
		{
		{
		MutexCond::Lock readRequestLock(readRequestCond);
		
		/* Signal completion of the current read request: */
		readComplete=lastReadRequest;
		readRequestCond.signal(readRequestLock);
		
		/* Wait for a new read request: */
		while(lastReadRequest==readRequest)
			readRequestCond.wait(readRequestLock);
		lastReadRequest=readRequest;
		}
		
		/* Read the next chunk of data from the input file: */
		char* readBuffer=buffer;
		if(lastReadRequest&0x1)
			readBuffer+=bufferSize;
		ssize_t readSize;
		if(gzInputFile!=0)
			{
			/* Read at most one buffer's worth of data from the compressed input file: */
			readSize=gzread(gzInputFile,readBuffer,bufferSize);
			}
		else
			{
			/* Read at most one buffer's worth of data from the input file: */
			readSize=read(inputFd,readBuffer,bufferSize);
			}
		
		if(readSize<0) // That's an error condition
			{
			readError=true;
			break;
			}
		else if(size_t(readSize)<bufferSize)
			{
			/* Must have read end-of-file: */
			eofPtr=readBuffer+readSize;
			break;
			}
		}
	
	/* Wake up main thread on error or end-of-file: */
	{
	MutexCond::Lock readRequestLock(readRequestCond);
	readComplete=lastReadRequest;
	readRequestCond.signal(readRequestLock);
	}
	
	return 0;
	}

void ASCIIFileReader::fillBuffer(void)
	{
	unsigned int lastReadComplete;
	{
	MutexCond::Lock readRequestLock(readRequestCond);
	
	/* Wait until current read request is finished: */
	while(readComplete!=readRequest)
		readRequestCond.wait(readRequestLock);
	lastReadComplete=readComplete;
	
	/* Immediately post the next read request: */
	++readRequest;
	readRequestCond.signal(readRequestLock);
	}
	
	/* Check for error condition: */
	if(readError)
		throw ReadError();
	
	/* Reset the buffer end and read pointers: */
	bufferEnd=buffer+bufferSize;
	rPtr=buffer;
	if(lastReadComplete&0x1)
		{
		bufferEnd+=bufferSize;
		rPtr+=bufferSize;
		}
	}

void ASCIIFileReader::resizeTokenBuffer(void)
	{
	tokenBufferSize=(tokenBufferSize*5)/4+10;
	char* newTokenBuffer=new char[tokenBufferSize];
	if(tokenSize>0)
		memcpy(newTokenBuffer,tokenBuffer,tokenSize);
	delete[] tokenBuffer;
	tokenBuffer=newTokenBuffer;
	}

ASCIIFileReader::ASCIIFileReader(const char* inputFileName)
	:inputFd(-1),
	 gzInputFile(0),
	 cc(characterClasses+1),
	 escape(256),
	 readRequest(0),readComplete(~0),readError(false),
	 bufferSize(16384),privateBuffer(true),buffer(0),
	 tokenBufferSize(0),
	 tokenBuffer(0),
	 tokenSize(0),
	 haveUnreadToken(false)
	{
	/* Check if the input file is gzipped: */
	if(Misc::hasCaseExtension(inputFileName,".gz"))
		{
		/* Open the compressed input file: */
		if((gzInputFile=gzopen(inputFileName,"r"))==0)
			Misc::throwStdErr("ASCIIFileReader::ASCIIFileReader: Could not open compressed input file %s",inputFileName);
		}
	else
		{
		/* Open the uncompressed input file: */
		if((inputFd=open(inputFileName,O_RDONLY))<0)
			Misc::throwStdErr("ASCIIFileReader::ASCIIFileReader: Could not open input file %s",inputFileName);
		}
	
	/* Initialize the character classes: */
	initCharacterClasses();
	
	/* Initialize the read buffer: */
	buffer=new char[bufferSize*2]; // This is a double buffer!
	bufferEnd=buffer+bufferSize;
	eofPtr=0;
	rPtr=bufferEnd;
	
	/* Start the readahead thread: */
	readAheadThread.start(this,&ASCIIFileReader::readAheadThreadMethod);
	
	/* Read the first chunk of data from the file: */
	readNextChar();
	}

ASCIIFileReader::ASCIIFileReader(int sInputFd)
	:inputFd(-1),
	 gzInputFile(0),
	 cc(characterClasses+1),
	 escape(256),
	 readRequest(0),readComplete(~0),readError(false),
	 bufferSize(16384),privateBuffer(true),buffer(0),
	 tokenBufferSize(0),
	 tokenBuffer(0),
	 tokenSize(0),
	 haveUnreadToken(false)
	{
	/* Attach to the file descriptor: */
	if((inputFd=dup(sInputFd))<0)
		Misc::throwStdErr("ASCIIFileReader::ASCIIFileReader: Could not attach to input file descriptor %d",sInputFd);
	
	/* Initialize the character classes: */
	initCharacterClasses();
	
	/* Initialize the read buffer: */
	buffer=new char[bufferSize*2]; // This is a double buffer!
	bufferEnd=buffer+bufferSize;
	eofPtr=0;
	rPtr=bufferEnd;
	
	/* Start the readahead thread: */
	readAheadThread.start(this,&ASCIIFileReader::readAheadThreadMethod);
	
	/* Read the first chunk of data from the file: */
	readNextChar();
	}

ASCIIFileReader::ASCIIFileReader(const void* inputBuffer,size_t inputBufferSize)
	:inputFd(-1),
	 gzInputFile(0),
	 cc(characterClasses+1),
	 escape(256),
	 readRequest(0),readComplete(~0),readError(false),
	 bufferSize(inputBufferSize),privateBuffer(false),buffer(reinterpret_cast<char*>(const_cast<void*>(inputBuffer))),
	 bufferEnd(buffer+bufferSize),
	 eofPtr(bufferEnd),
	 rPtr(buffer),
	 tokenBufferSize(0),
	 tokenBuffer(0),
	 tokenSize(0),
	 haveUnreadToken(false)
	{
	/* Initialize the character classes: */
	initCharacterClasses();
	
	/* Read the first character from the file: */
	if(rPtr==eofPtr)
		lastChar=-1;
	else
		lastChar=*(rPtr++);
	}

ASCIIFileReader::~ASCIIFileReader(void)
	{
	if(inputFd>=0||gzInputFile!=0)
		{
		/* Cancel the readahead thread: */
		readAheadThread.cancel();
		readAheadThread.join();
		}
	
	/* Close the compressed or uncompressed input file: */
	if(inputFd>=0)
		close(inputFd);
	if(gzInputFile!=0)
		gzclose(gzInputFile);
	
	if(privateBuffer)
		{
		/* Delete the read buffer: */
		delete[] buffer;
		}
	delete[] tokenBuffer;
	}

void ASCIIFileReader::setWhiteSpace(int character,bool newWhiteSpace)
	{
	if(newWhiteSpace)
		{
		cc[character]|=WHITESPACE;
		cc[character]&=~(TOKEN|STRING);
		}
	else
		{
		cc[character]&=~WHITESPACE;
		cc[character]|=TOKEN|STRING;
		}
	}

void ASCIIFileReader::setPunctuation(const char* newPunctuation)
	{
	/* Unmark all previous punctuation characters: */
	for(int i=0;i<256;++i)
		if(cc[i]&PUNCTUATION)
			{
			cc[i]&=~PUNCTUATION;
			cc[i]|=TOKEN|STRING;
			}
	
	/* Mark all characters in the given string as punctuation characters: */
	for(const char* pPtr=newPunctuation;*pPtr!='\0';++pPtr)
		{
		cc[int(*pPtr)]|=PUNCTUATION;
		cc[int(*pPtr)]&=~WHITESPACE; // Punctuation characters are not whitespace
		cc[int(*pPtr)]&=~(TOKEN|STRING); // Punctuation characters terminate tokens and non-quoted strings
		}
	}

void ASCIIFileReader::setEscape(int newEscape)
	{
	if(escape>=0&&escape<256)
		escape=newEscape;
	else
		escape=256;
	}

void ASCIIFileReader::skipWs(void)
	{
	while(cc[lastChar]&WHITESPACE) // Breaks on non-space or EOF
		readNextChar();
	}

void ASCIIFileReader::skipWsLine(void)
	{
	while(lastChar!='\n'&&(cc[lastChar]&WHITESPACE)) // Breaks on newline, non-space, or EOF
		readNextChar();
	}

void ASCIIFileReader::skipLine(void)
	{
	while(lastChar>=0&&lastChar!='\n')
		readNextChar();
	if(lastChar=='\n')
		readNextChar();
	}

const char* ASCIIFileReader::readNextToken(void)
	{
	if(haveUnreadToken)
		{
		/* Use the "unread" token: */
		haveUnreadToken=false;
		return tokenBuffer;
		}
	
	/* Initialize the token: */
	tokenSize=0;
	
	/* Check for punctuation characters: */
	if(cc[lastChar]&PUNCTUATION)
		{
		/* Return the punctuation character: */
		if(tokenSize>=tokenBufferSize)
			resizeTokenBuffer();
		tokenBuffer[tokenSize++]=lastChar;
		readNextChar();
		}
	else
		{
		/* Read characters until EOF, whitespace, or punctuation: */
		while(cc[lastChar]&TOKEN)
			{
			if(tokenSize>=tokenBufferSize)
				resizeTokenBuffer();
			tokenBuffer[tokenSize++]=lastChar;
			readNextChar();
			}
		}
	
	/* Terminate the token: */
	if(tokenSize>=tokenBufferSize)
		resizeTokenBuffer();
	tokenBuffer[tokenSize]='\0';
	
	return tokenBuffer;
	}

bool ASCIIFileReader::isToken(const char* token) const
	{
	return strcmp(tokenBuffer,token)==0;
	}

bool ASCIIFileReader::isCaseToken(const char* token) const
	{
	return strcasecmp(tokenBuffer,token)==0;
	}

void ASCIIFileReader::unreadToken(void)
	{
	/* Mark the current token as "unread": */
	haveUnreadToken=true;
	}

std::string ASCIIFileReader::readString(void)
	{
	/* Skip whitespace: */
	while(cc[lastChar]&WHITESPACE) // Breaks on non-space or EOF
		readNextChar();
	if(lastChar<0)
		throw EndOfFile();
	
	unsigned char stringClass=STRING;
	
	/* Check for opening quote: */
	if(lastChar=='\"')
		{
		/* Go to quoted string class: */
		stringClass=QUOTEDSTRING;
		
		/* Skip the opening quote: */
		readNextChar();
		}
	
	/* Parse the string: */
	std::string result;
	while(cc[lastChar]&stringClass)
		{
		/* Handle escaped characters: */
		if(lastChar==escape)
			{
			/* Read the escaped character: */
			readNextChar();
			if(lastChar<0)
				{
				/* Append the backslash itself and quit: */
				result.push_back('\\');
				break;
				}
			else
				{
				/* Translate escape sequence according to C standard: */
				switch(lastChar)
					{
					case 'a':
						lastChar='\a';
						break;
					
					case 'b':
						lastChar='\b';
						break;
					
					case 'f':
						lastChar='\f';
						break;
					
					case 'n':
						lastChar='\n';
						break;
					
					case 'r':
						lastChar='\r';
						break;
					
					case 't':
						lastChar='\t';
						break;
					
					case 'v':
						lastChar='\v';
						break;
					}
				}
			}
		
		/* Store the character: */
		result.push_back(lastChar);
		
		/* Read the next character: */
		readNextChar();
		}
	
	if(stringClass==QUOTEDSTRING&&lastChar=='\"')
		{
		/* Skip the closing quote: */
		readNextChar();
		}
	
	return result;
	}

unsigned int ASCIIFileReader::readUInt(void)
	{
	/* Skip whitespace: */
	while(cc[lastChar]&WHITESPACE) // Breaks on non-space or EOF
		readNextChar();
	if(lastChar<0)
		throw EndOfFile();
	
	/* Read unsigned integer-class characters from the input file: */
	char valueBuffer[64];
	char* valuePtr=valueBuffer;
	while(cc[lastChar]&UNSIGNEDINTEGER)
		{
		*(valuePtr++)=lastChar;
		readNextChar();
		}
	if(valuePtr==valueBuffer)
		throw ConversionError();
	*valuePtr='\0';
	
	/* Translate and return the unsigned integer value: */
	return strtoul(valueBuffer,0,10);
	}

int ASCIIFileReader::readInt(void)
	{
	/* Skip whitespace: */
	while(cc[lastChar]&WHITESPACE) // Breaks on non-space or EOF
		readNextChar();
	if(lastChar<0)
		throw EndOfFile();
	
	/* Read integer-class characters from the input file: */
	char valueBuffer[64];
	char* valuePtr=valueBuffer;
	while(cc[lastChar]&INTEGER)
		{
		*(valuePtr++)=lastChar;
		readNextChar();
		}
	if(valuePtr==valueBuffer)
		throw ConversionError();
	*valuePtr='\0';
	
	/* Translate and return the integer value: */
	return atoi(valueBuffer);
	}

double ASCIIFileReader::readDouble(void)
	{
	/* Skip whitespace: */
	while(cc[lastChar]&WHITESPACE) // Breaks on non-space or EOF
		readNextChar();
	if(lastChar<0)
		throw EndOfFile();
	
	/* Read floating-point-class characters from the input file: */
	char valueBuffer[64];
	char* valuePtr=valueBuffer;
	while(cc[lastChar]&FLOATINGPOINT)
		{
		*(valuePtr++)=lastChar;
		readNextChar();
		}
	if(valuePtr==valueBuffer)
		throw ConversionError();
	*valuePtr='\0';
	
	/* Translate and return the floating-point value: */
	return atof(valueBuffer);
	}

void ASCIIFileReader::readUInts(size_t numValues,unsigned int values[])
	{
	char valueBuffer[64];
	for(size_t i=0;i<numValues;++i)
		{
		/* Skip whitespace: */
		while(cc[lastChar]&WHITESPACE) // Breaks on non-space or EOF
			readNextChar();
		if(lastChar<0)
			throw EndOfFile();
		
		/* Read unsigned integer-class characters from the input file: */
		char* valuePtr=valueBuffer;
		while(cc[lastChar]&UNSIGNEDINTEGER)
			{
			*(valuePtr++)=lastChar;
			readNextChar();
			}
		if(valuePtr==valueBuffer)
			throw ConversionError();
		*valuePtr='\0';
		
		/* Translate and return the unsigned integer value: */
		values[i]=strtoul(valueBuffer,0,10);
		}
	}

void ASCIIFileReader::readInts(size_t numValues,int values[])
	{
	char valueBuffer[64];
	for(size_t i=0;i<numValues;++i)
		{
		/* Skip whitespace: */
		while(cc[lastChar]&WHITESPACE) // Breaks on non-space or EOF
			readNextChar();
		if(lastChar<0)
			throw EndOfFile();
		
		/* Read unsigned integer-class characters from the input file: */
		char* valuePtr=valueBuffer;
		while(cc[lastChar]&INTEGER)
			{
			*(valuePtr++)=lastChar;
			readNextChar();
			}
		if(valuePtr==valueBuffer)
			throw ConversionError();
		*valuePtr='\0';
		
		/* Translate and return the integer value: */
		values[i]=atoi(valueBuffer);
		}
	}

void ASCIIFileReader::readDoubles(size_t numValues,double values[])
	{
	char valueBuffer[64];
	for(size_t i=0;i<numValues;++i)
		{
		/* Skip whitespace: */
		while(cc[lastChar]&WHITESPACE) // Breaks on non-space or EOF
			readNextChar();
		if(lastChar<0)
			throw EndOfFile();
		
		/* Read unsigned integer-class characters from the input file: */
		char* valuePtr=valueBuffer;
		while(cc[lastChar]&FLOATINGPOINT)
			{
			*(valuePtr++)=lastChar;
			readNextChar();
			}
		if(valuePtr==valueBuffer)
			throw ConversionError();
		*valuePtr='\0';
		
		/* Translate and return the floating-point value: */
		values[i]=atof(valueBuffer);
		}
	}

}
