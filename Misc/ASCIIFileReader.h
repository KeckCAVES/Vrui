/***********************************************************************
ASCIIFileReader - Helper class to read ASCII files and support higher-
level file parsers. Can unzip files on-the-fly.
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

#ifndef MISC_ASCIIFILEREADER_INCLUDED
#define MISC_ASCIIFILEREADER_INCLUDED

#include <string>
#include <stdexcept>

namespace Misc {

class ASCIIFileReader
	{
	/* Embedded classes: */
	public:
	enum CharacterClasses // Enumerated type for character class bit masks to speed up tokenization
		{
		NONE=0x0,
		WHITESPACE=0x1, // Class for characters skipped by skipWs()
		PUNCTUATION=0x2, // User-definable class for syntactic punctuation, i.e., single-character tokens
		TOKEN=0x4, // Class for characters allowed in generic tokens
		STRING=0x8, // Class for characters allowed in non-quoted strings
		QUOTEDSTRING=0x10, // Class for characters allowed in quoted strings
		UNSIGNEDINTEGER=0x20, // Class for characters allowed in unsigned integer values
		INTEGER=0x40, // Class for characters allowed in integer values
		FLOATINGPOINT=0x80 // Class for characters allowed in floating-point values
		};
	
	class ReadError:public std::runtime_error // Exception class to report errors while reading from input files
		{
		/* Constructors and destructors: */
		public:
		ReadError(void)
			:std::runtime_error("Misc::ASCIIFileReader: Error while reading from input file")
			{
			}
		};
	
	class ConversionError:public std::runtime_error // Exception class to report that no characters could be read for a numeric conversion
		{
		/* Constructors and destructors: */
		public:
		ConversionError(void)
			:std::runtime_error("Misc::ASCIIFileReader: No characters matching requested type were read")
			{
			}
		};
	
	class EndOfFile:public std::runtime_error // Exception class to report that end-of-file has been reached before a requested item could be read
		{
		/* Constructors and destructors: */
		public:
		EndOfFile(void)
			:std::runtime_error("Misc::ASCIIFileReader: End of file encountered while reading from input file")
			{
			}
		};
	
	/* Elements: */
	private:
	int inputFd; // Low-level file descriptor for the ASCII input file
	void* gzInputFile; // ZLIB compressed file structure to read gzipped files
	unsigned char characterClasses[257]; // Array of character type bit flags for quicker classification, with extra space for EOF
	protected:
	unsigned char* cc; // Pointer into character classes array to account for EOF==-1
	int escape; // Escape character to insert special characters into strings, defaults to 256 (none)
	private:
	size_t bufferSize; // Size of file access buffer
	bool privateBuffer; // Flag if the input buffer was allocated by the reader itself
	char* buffer; // Buffer for efficient file access
	char* bufferEnd; // Pointer behind last valid character in buffer
	char* eofPtr; // Pointer behind last character in file, if it is actually in the buffer
	char* rPtr; // Current reading position in buffer
	protected:
	int lastChar; // Last character read from file
	
	/* Special state to read and unread tokens: */
	private:
	size_t tokenBufferSize; // Current size of token buffer
	char* tokenBuffer; // Pointer to token buffer
	size_t tokenSize; // Length of the current token excluding terminator character
	bool haveUnreadToken; // Flag if the token buffer contains an "unread" token
	
	/* Private methods: */
	void initCharacterClasses(void); // Initializes the character classes array
	void fillBuffer(void); // Fills the read buffer with more data from the input file
	void resizeTokenBuffer(void); // Creates additional room in the token buffer
	
	/* Protected methods: */
	protected:
	void readNextChar(void) // Reads the next character from the input file, or -1 (EOF) on end-of-file
		{
		/* Check for end of file: */
		if(rPtr==eofPtr)
			{
			/* Store EOF marker: */
			lastChar=-1;
			}
		else
			{
			/* Read more data from the file if the buffer is empty: */
			if(rPtr==bufferEnd)
				fillBuffer();
			
			/* Store the next character and advance the read pointer: */
			lastChar=*(rPtr++);
			}
		}
	
	/* Constructors and destructors: */
	public:
	ASCIIFileReader(const char* inputFileName); // Creates a reader for the given input file
	ASCIIFileReader(int sInputFd); // Creates a reader for the already-open low-level file descriptor
	ASCIIFileReader(const void* inputBuffer,size_t inputBufferSize); // Creates a reader for the given chunk of memory
	~ASCIIFileReader(void); // Destroys the reader and closes the input file
	
	/* Methods: */
	void setWhiteSpace(int character,bool whiteSpace); // Sets the given character's whitespace flag
	void setPunctuation(const char* newPunctuations); // Marks all characters in the given string as punctuation characters
	void setEscape(int newEscape); // Sets the escape character for special characters in strings
	
	/* Low-level read methods: */
	bool eof(void) const // Returns true if the entire input file has been read
		{
		return lastChar<0;
		}
	int getcLookAhead(void) // Returns the next character that will be read from the input file
		{
		return lastChar;
		}
	int getc(void) // Returns the next character from the input file, or -1 (EOF)
		{
		/* Return the current and read the next character: */
		int result=lastChar;
		readNextChar();
		return result;
		}
	void skipWs(void); // Skips whitespace in the input file
	void skipWsLine(void); // Skips whitespace in the input file until the next newline character, and reads the newline
	void skipLine(void); // Skips the rest of the current line, i.e., until the next newline character, and reads the newline
	const char* readNextToken(void); // Skips whitespace, and reads the next token, i.e., either a single punctuation character, or a sequence of non-whitespace and non-punctuation characters
	size_t getTokenSize(void) const // Returns the length of the most recently read token
		{
		return tokenSize;
		}
	const char* getToken(void) // Returns the most recently read token; readNextToken must be called first
		{
		return tokenBuffer;
		}
	bool isToken(const char* token) const; // Returns true if the most recently read token matches the given string
	bool isCaseToken(const char* token) const; // Returns true if the most recently read token matches the given string up to case
	void unreadToken(void); // Unreads the most recently read token; can only be called once after each call to readNextToken
	
	/* Higher-level read methods: */
	std::string readString(void); // Skips whitespace, and reads the next (quoted or non-quoted) string; throws exception if end-of-file is encountered before string is read
	unsigned int readUInt(void); // Skips whitespace, and reads the next unsigned integer; throws exception if end-of-file is encountered before number is read, or no characters could be read
	int readInt(void); // Skips whitespace, and reads the next integer; throws exception if end-of-file is encountered before number is read, or no characters could be read
	double readDouble(void); // Skips whitespace, and reads the next floating-point value; throws exception if end-of-file is encountered before number is read, or no characters could be read
	void readUInts(size_t numValues,unsigned int values[]); // Reads an array of whitespace-separated unsigned integer values
	void readInts(size_t numValues,int values[]); // Reads an array of whitespace-separated integer values
	void readDoubles(size_t numValues,double values[]); // Reads an array of whitespace-separated double values
	};

}

#endif
