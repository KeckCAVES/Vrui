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

#ifndef IO_XMLSOURCE_INCLUDED
#define IO_XMLSOURCE_INCLUDED

#include <utility>
#include <string>
#include <stdexcept>
#include <IO/File.h>

namespace IO {

class XMLSource
	{
	/* Embedded classes: */
	public:
	class Error:public std::runtime_error // Base class for XML processing errors
		{
		/* Elements: */
		public:
		size_t line,column; // Line number and column index where the error occurred
		
		/* Constructors and destructors: */
		Error(const XMLSource& source,const char* errorType,const char* sWhat); // Creates an error of the given type with the given message and retrieves line number and column index from the given XML source
		};
	
	class SyntaxError:public Error // Exception class reporting a violation of the XML syntax
		{
		/* Constructors and destructors: */
		public:
		SyntaxError(const XMLSource& source,const char* sWhat)
			:Error(source,"Syntax",sWhat)
			{
			}
		};
	
	class WellFormedError:public Error // Exception class reporting a violation of the XML well-formedness constraint
		{
		/* Constructors and destructors: */
		public:
		WellFormedError(const XMLSource& source,const char* sWhat)
			:Error(source,"Well-formedness",sWhat)
			{
			}
		};
	
	class ValidError:public Error // Exception class reporting a violation of the XML validity constraint
		{
		/* Constructors and destructors: */
		public:
		ValidError(const XMLSource& source,const char* sWhat)
			:Error(source,"Validity",sWhat)
			{
			}
		};
	
	private:
	typedef int (*ReadNextCharFunction)(File& source); // Type for functions reading a single character from an input file; returns -1 at end of file and throws exception on decoding error
	
	enum SyntaxType // Enumerated type for states in the syntactic state machine
		{
		Comment,
		ProcessingInstructionTarget,ProcessingInstructionContent,
		TagName,AttributeName,AttributeValue,
		Content,CData,
		EndOfFile
		};
	
	/* Elements: */
	private:
	FilePtr source; // Data source for XML processor
	ReadNextCharFunction readNextChar; // Function to read the next character based on the source file's character encoding
	size_t charBufferSize; // Size of a buffer to hold Unicode characters decoded from the source
	int* charBuffer; // Pointer to Unicode character buffer
	int* cbEnd; // Pointer past last character in buffer
	int* cbNext; // Pointer to next character to be read from buffer
	size_t line,column; // Line number and column index of the character located at the midpoint of the Unicode buffer
	int minorVersion; // Minor XML version of the XML source
	bool standalone; // Flag whether the XML source is a standalone entity
	bool hadCarriageReturn; // Flag if the last character decoded from the source was a carriage return (#xD)
	SyntaxType syntaxType; // Type of syntactic element currently being reported to user
	bool openTag; // Flag if the current tag is an opening tag
	int quote; // The quotation mark that started the quoted string currently being reported to user
	bool nmtokens; // Flag whether the current quoted string is to be stripped of extra spaces
	bool selfCloseTag; // Flag whether the tag that was just read was a self-closing tag
	
	/* Private methods: */
	void decodeFromSource(void); // Decodes more data from the source into the Unicode buffer
	void updateFilePos(void); // Scans forward in the current input buffer to calculate the line number and column index of the next character to be read
	bool fillEmptyBuffer(void); // Re-fills the Unicode character buffer by reading more data from the source; returns true if there is no more data in the source
	int getChar(void) // Returns the next decoded Unicode character
		{
		/* Check if the buffer is empty: */
		if(cbNext==cbEnd)
			{
			/* Re-fill the buffer from the source file and check for end-of-file: */
			if(fillEmptyBuffer())
				return -1;
			}
		
		/* Return the next character and advance the read position: */
		return *(cbNext++);
		}
	void ungetChar(void) // Puts a just-read character back into the input buffer; this can only be used once after each getChar()
		{
		/* Rewind the read pointer: */
		--cbNext;
		}
	void growPutbackBuffer(void); // Grows the input buffer to be able to put back at least one more character
	void ungetChar(int putbackChar) // Puts the given character back into the input buffer; this can be called as many times as needed
		{
		/* Grow the put-back buffer if there is no room to put back the character: */
		if(cbNext==charBuffer)
			growPutbackBuffer();
		
		/* Put the character back: */
		*(--cbNext)=putbackChar;
		}
	size_t growReadAhead(size_t readAheadSize,size_t numChars); // Decodes more characters from the source; returns number of characters available to read ahead
	bool readAhead(size_t numChars) // Fills the Unicode character buffer such that the given number of characters can be read ahead; returns true if there are enough characters (might fail at end-of-file)
		{
		/* Calculate the number of read-ahead characters already in the buffer: */
		size_t readAheadSize=cbEnd-cbNext;
		
		/* Read more data if the current number is too small: */
		if(readAheadSize<numChars)
			readAheadSize=growReadAhead(readAheadSize,numChars);
		
		return readAheadSize>=numChars;
		}
	int peekChar(size_t offset) const // Returns the Unicode character at the given offset after the current read position; assumes that readAhead was called with a result >= offset
		{
		/* Return a character from the buffer: */
		return cbNext[offset];
		}
	bool matchString(const char* match,bool consumeOnMatch =true) // Checks if the given string matches a sequence of read-ahead characters; assumes that readAhead was called with a result >= match string length; skips the string if matched and consumeOnMatch is true
		{
		/* Compare characters until the string is over or there is a mismatch: */
		int* cbPtr;
		for(cbPtr=cbNext;*match!='\0'&&int(*match)==*cbPtr;++match,++cbPtr)
			;
		
		/* Check if the string was completely matched: */
		if(*match=='\0'&&consumeOnMatch)
			{
			/* Read to the end of the string: */
			cbNext=cbPtr;
			}
		return *match=='\0';
		}
	bool matchStringNoCase(const char* match,bool consumeOnMatch =true) // Checks if the given string matches a sequence of read-ahead characters up to case (assuming only ASCII characters); assumes that readAhead was called with a result >= match string length; skips the string if matched and consumeOnMatch is true
		{
		/* Compare characters until the string is over or there is a mismatch: */
		int* cbPtr;
		for(cbPtr=cbNext;*match!='\0'&&tolower(*match)==tolower(*cbPtr);++match,++cbPtr)
			;
		
		/* Check if the string was completely matched: */
		if(*match=='\0'&&consumeOnMatch)
			{
			/* Read to the end of the string: */
			cbNext=cbPtr;
			}
		return *match=='\0';
		}
	void consumeChars(size_t numChars) // Removes a number of characters from the input buffer; assumes that readAhead was called with a result >= numChars
		{
		/* Advance the read pointer: */
		cbNext+=numChars;
		}
	int parseReference(void); // Returns the code point of a single-character reference with the initial "&" already read; throws exception on error
	void detectNextSyntaxType(void); // Detects the next syntax type in the source at the closing of the current top-level syntax type
	void closeAttributeValue(void); // Detects the next syntax type in the source at the closing of an attribute value
	void processHeader(void); // Parses the first part of the XML header before any client-relevant elements
	
	/* Constructors and destructors: */
	public:
	XMLSource(FilePtr sSource); // Creates an XML processor for the given input file
	~XMLSource(void); // Destroys the XML processor
	
	/* Methods: */
	bool eof(void) const // Returns true if the entire source has been read
		{
		return syntaxType==EndOfFile;
		}
	std::pair<size_t,size_t> getFilePosition(void) const; // Returns the line number and column index (in that order) of the next character that will be returned by getChar()
	bool isComment(void) const // Returns true while processing a comment
		{
		return syntaxType==Comment;
		}
	bool isPITarget(void) const // Returns true while processing the target name of a processing instruction
		{
		return syntaxType==ProcessingInstructionTarget;
		}
	bool isPIContent(void) // Returns true while processing the content of a processing instruction
		{
		return syntaxType==ProcessingInstructionTarget;
		}
	bool isTagName(void) const // Returns true while processing the name of a tag
		{
		return syntaxType==TagName;
		}
	bool isAttributeName(void) const // Returns true while processing the name of one of a tag's attributes
		{
		return syntaxType==AttributeName;
		}
	bool isAttributeValue(void) const // Returns true while processing the value of one of a tag's attributes
		{
		return syntaxType==AttributeValue;
		}
	bool isOpeningTag(void) const // Returns true while processing an opening tag's name or attribute/value pairs
		{
		return openTag;
		}
	bool wasSelfClosingTag(void) const // Returns true if the most recently processed tag was a self-closing (empty) tag
		{
		return selfCloseTag;
		}
	bool isCharacterData(void) const // Returns true while processing character data
		{
		return syntaxType==Content||syntaxType==CData;
		}
	int readComment(void); // Returns the next comment character, or -1 at end of comment
	int readName(void); // Returns the next character of a name (tag name, attribute name, or processing instruction target), or -1 at end of name
	int readProcessingInstruction(void); // Returns the next character of a processing instruction, or -1 at end of processing instruction
	int readAttributeValue(void); // Returns the next attribute value character, or -1 at end of attribute value
	int readCharacterData(void); // Returns the next character data character, or -1 at end of character data
	std::string& readUTF8(std::string& string); // Appends the current syntax element to the given UTF-8 encoded string
	std::string readUTF8(void) // Returns the current syntax element as a UTF-8 encoded string
		{
		/* Read the current syntax element into a new string and return it: */
		std::string result;
		readUTF8(result);
		return result;
		}
	};

}

#endif
