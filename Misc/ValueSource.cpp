/***********************************************************************
ValueSource - Class to read strings or numbers from character sources.
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

#include <Misc/ValueSource.h>

#include <ctype.h>
#include <math.h>

namespace Misc {

/****************************
Methods of class ValueSource:
****************************/

void ValueSource::initCharacterClasses(void)
	{
	cc[-1]=NONE; // EOF is nothing; nothing, I tell you!
	
	/* Set up the basic character classes: */
	for(int i=0;i<256;++i)
		{
		cc[i]=NONE;
		if(isspace(i))
			cc[i]|=WHITESPACE;
		else
			cc[i]|=STRING; // Everything that's not a space can be a string for now
		if(isdigit(i))
			cc[i]|=DIGIT;
		cc[i]|=QUOTEDSTRING; // Pretty much everything is a quoted string
		}
	
	/* Set string terminators: */
	cc['\n']&=~QUOTEDSTRING; // Newlines terminate quoted strings
	}

ValueSource::ValueSource(Misc::CharacterSource& sSource)
	:source(sSource),
	 cc(characterClasses+1),
	 escapeChar(-1)
	{
	/* Initialize the character classes: */
	initCharacterClasses();
	
	/* Read the first character from the character source: */
	lastChar=source.getc();
	}

ValueSource::~ValueSource(void)
	{
	}

void ValueSource::setWhitespace(int character,bool whitespace)
	{
	if(whitespace)
		{
		cc[character]|=WHITESPACE;
		cc[character]&=~(PUNCTUATION|STRING|QUOTE);
		}
	else
		{
		cc[character]&=~WHITESPACE;
		if(!(cc[character]&(PUNCTUATION|QUOTE)))
			cc[character]|=STRING;
		}
	}

void ValueSource::setWhitespace(const char* whitespace)
	{
	/* Clear the whitespace set: */
	for(int i=0;i<256;++i)
		if(cc[i]&WHITESPACE)
			{
			cc[i]&=~WHITESPACE;
			if(!(cc[i]&(PUNCTUATION|QUOTE)))
				cc[i]|=STRING;
			}
	
	/* Mark all characters in the given string as whitespace: */
	for(const char* wPtr=whitespace;*wPtr!='\0';++wPtr)
		{
		cc[int(*wPtr)]|=WHITESPACE;
		cc[int(*wPtr)]&=~(PUNCTUATION|STRING|QUOTE);
		}
	}

void ValueSource::setPunctuation(int character,bool punctuation)
	{
	if(punctuation)
		{
		cc[character]|=PUNCTUATION;
		cc[character]&=~(WHITESPACE|STRING|QUOTE);
		}
	else
		{
		cc[character]&=~PUNCTUATION;
		if(!(cc[character]&(WHITESPACE|QUOTE)))
			cc[character]|=STRING;
		}
	}

void ValueSource::setPunctuation(const char* punctuation)
	{
	/* Clear the punctuation set: */
	for(int i=0;i<256;++i)
		if(cc[i]&PUNCTUATION)
			{
			cc[i]&=~PUNCTUATION;
			if(!(cc[i]&(WHITESPACE|QUOTE)))
				cc[i]|=STRING;
			}
	
	/* Mark all characters in the given string as punctuation: */
	for(const char* pPtr=punctuation;*pPtr!='\0';++pPtr)
		{
		cc[int(*pPtr)]|=PUNCTUATION;
		cc[int(*pPtr)]&=~(WHITESPACE|STRING|QUOTE);
		}
	}

void ValueSource::setQuote(int character,bool quote)
	{
	if(quote)
		{
		cc[character]|=QUOTE;
		cc[character]&=~(WHITESPACE|PUNCTUATION|STRING);
		}
	else
		{
		cc[character]&=~QUOTE;
		if(!(cc[character]&(WHITESPACE|PUNCTUATION)))
			cc[character]|=STRING;
		}
	}

void ValueSource::setQuotes(const char* quotes)
	{
	/* Clear the quote set: */
	for(int i=0;i<256;++i)
		if(cc[i]&QUOTE)
			{
			cc[i]&=~QUOTE;
			if(!(cc[i]&(WHITESPACE|PUNCTUATION)))
				cc[i]|=STRING;
			}
	
	/* Mark all characters in the given string as quotes: */
	for(const char* qPtr=quotes;*qPtr!='\0';++qPtr)
		{
		cc[int(*qPtr)]|=QUOTE;
		cc[int(*qPtr)]&=~(WHITESPACE|PUNCTUATION|STRING);
		}
	}

void ValueSource::setEscape(int newEscapeChar)
	{
	escapeChar=newEscapeChar;
	if(escapeChar>=0)
		{
		/* Mark the escape character as valid for strings and quoted strings: */
		cc[escapeChar]|=STRING|QUOTEDSTRING;
		}
	}

void ValueSource::skipWs(void)
	{
	/* Skip all whitespace characters: */
	while(cc[lastChar]&WHITESPACE)
		lastChar=source.getc();
	}

void ValueSource::skipLine(void)
	{
	/* Skip everything until the next newline: */
	while(lastChar>=0&&lastChar!='\n')
		lastChar=source.getc();
	
	/* Skip the newline: */
	if(lastChar=='\n')
		lastChar=source.getc();
	}

void ValueSource::skipString(void)
	{
	if(cc[lastChar]&PUNCTUATION)
		{
		/* Read a punctuation character: */
		lastChar=source.getc();
		}
	else if(cc[lastChar]&QUOTE)
		{
		/* Read the quote character and temporarily remove it from the set of quoted string characters: */
		int quote=lastChar;
		cc[quote]&=~QUOTEDSTRING;
		lastChar=source.getc();
		
		/* Read characters until the matching quote, endline, or EOF: */
		while(cc[lastChar]&QUOTEDSTRING)
			{
			if(lastChar==escapeChar)
				{
				/* Read the escaped character: */
				lastChar=source.getc();
				switch(lastChar)
					{
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
						/* Parse an octal character code: */
						lastChar=source.getc();
						for(int i=1;i<3&&lastChar>='0'&&lastChar<='7';++i)
							lastChar=source.getc();
						break;
					
					case 'x':
						/* Parse a hexadecimal character code: */
						lastChar=source.getc();
						while((lastChar>='0'&&lastChar<='9')||(lastChar>='A'&&lastChar<='F')||(lastChar>='a'&&lastChar<='f'))
							lastChar=source.getc();
						break;
					}
				}
			
			lastChar=source.getc();
			}
		
		/* Read the terminating quote, if there is one: */
		if(lastChar==quote)
			lastChar=source.getc();
		
		/* Add the quote character to the set of quoted string characters again: */
		cc[quote]|=QUOTEDSTRING;
		}
	else
		{
		/* Read a non-quoted string: */
		while(cc[lastChar]&STRING)
			{
			if(lastChar==escapeChar)
				{
				/* Read the escaped character: */
				lastChar=source.getc();
				switch(lastChar)
					{
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
						/* Parse an octal character code: */
						lastChar=source.getc();
						for(int i=1;i<3&&lastChar>='0'&&lastChar<='7';++i)
							lastChar=source.getc();
						break;
					
					case 'x':
						/* Parse a hexadecimal character code: */
						lastChar=source.getc();
						while((lastChar>='0'&&lastChar<='9')||(lastChar>='A'&&lastChar<='F')||(lastChar>='a'&&lastChar<='f'))
							lastChar=source.getc();
						break;
					}
				}
			
			lastChar=source.getc();
			}
		}
	
	/* Skip whitespace: */
	while(cc[lastChar]&WHITESPACE)
		lastChar=source.getc();
	}

std::string ValueSource::readString(void)
	{
	std::string result;
	
	if(cc[lastChar]&PUNCTUATION)
		{
		/* Read a punctuation character: */
		result.push_back(lastChar);
		lastChar=source.getc();
		}
	else if(cc[lastChar]&QUOTE)
		{
		/* Read the quote character and temporarily remove it from the set of quoted string characters: */
		int quote=lastChar;
		cc[quote]&=~QUOTEDSTRING;
		lastChar=source.getc();
		
		/* Read characters until the matching quote, endline, or EOF: */
		while(cc[lastChar]&QUOTEDSTRING)
			{
			if(lastChar==escapeChar)
				{
				/* Read the escaped character: */
				lastChar=source.getc();
				switch(lastChar)
					{
					case 'a':
						result.push_back('\a');
						break;
					
					case 'b':
						result.push_back('\b');
						break;
					
					case 'f':
						result.push_back('\f');
						break;
					
					case 'n':
						result.push_back('\n');
						break;
					
					case 'r':
						result.push_back('\r');
						break;
					
					case 't':
						result.push_back('\t');
						break;
					
					case 'v':
						result.push_back('\v');
						break;
					
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
						{
						/* Parse an octal character code: */
						int value=lastChar-'0';
						lastChar=source.getc();
						for(int i=1;i<3&&lastChar>='0'&&lastChar<='7';++i,lastChar=source.getc())
							value=(value<<3)+(lastChar-'0');
						
						result.push_back(value);
						break;
						}
					
					case 'x':
						{
						/* Parse a hexadecimal character code: */
						int value=0;
						lastChar=source.getc();
						while((lastChar>='0'&&lastChar<='9')||(lastChar>='A'&&lastChar<='F')||(lastChar>='a'&&lastChar<='f'))
							{
							if(lastChar>='0'&&lastChar<='9')
								value=(value<<4)+(lastChar-'0');
							else if(lastChar>='A'&&lastChar<='F')
								value=(value<<4)+(lastChar-'A'+10);
							else
								value=(value<<4)+(lastChar-'a'+10);
							lastChar=source.getc();
							}
						
						result.push_back(value);
						break;
						}
					
					default:
						/* Use the escaped character verbatim: */
						result.push_back(lastChar);
					}
				}
			else
				result.push_back(lastChar);
			
			lastChar=source.getc();
			}
		
		/* Read the terminating quote, if there is one: */
		if(lastChar==quote)
			lastChar=source.getc();
		
		/* Add the quote character to the set of quoted string characters again: */
		cc[quote]|=QUOTEDSTRING;
		}
	else
		{
		/* Read a non-quoted string: */
		while(cc[lastChar]&STRING)
			{
			if(lastChar==escapeChar)
				{
				/* Read the escaped character: */
				lastChar=source.getc();
				switch(lastChar)
					{
					case 'a':
						result.push_back('\a');
						break;
					
					case 'b':
						result.push_back('\b');
						break;
					
					case 'f':
						result.push_back('\f');
						break;
					
					case 'n':
						result.push_back('\n');
						break;
					
					case 'r':
						result.push_back('\r');
						break;
					
					case 't':
						result.push_back('\t');
						break;
					
					case 'v':
						result.push_back('\v');
						break;
					
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
						{
						/* Parse an octal character code: */
						int value=lastChar-'0';
						lastChar=source.getc();
						for(int i=1;i<3&&lastChar>='0'&&lastChar<='7';++i,lastChar=source.getc())
							value=(value<<3)+(lastChar-'0');
						
						result.push_back(value);
						break;
						}
					
					case 'x':
						{
						/* Parse a hexadecimal character code: */
						int value=0;
						lastChar=source.getc();
						while((lastChar>='0'&&lastChar<='9')||(lastChar>='A'&&lastChar<='F')||(lastChar>='a'&&lastChar<='f'))
							{
							if(lastChar>='0'&&lastChar<='9')
								value=(value<<4)+(lastChar-'0');
							else if(lastChar>='A'&&lastChar<='F')
								value=(value<<4)+(lastChar-'A'+10);
							else
								value=(value<<4)+(lastChar-'a'+10);
							lastChar=source.getc();
							}
						
						result.push_back(value);
						break;
						}
					
					default:
						/* Use the escaped character verbatim: */
						result.push_back(lastChar);
					}
				}
			else
				result.push_back(lastChar);
			
			lastChar=source.getc();
			}
		}
	
	/* Skip whitespace: */
	while(cc[lastChar]&WHITESPACE)
		lastChar=source.getc();
	
	return result;
	}

int ValueSource::readInteger(void)
	{
	/* Read a plus or minus sign: */
	bool negate=lastChar=='-';
	if(lastChar=='-'||lastChar=='+')
		lastChar=source.getc();
	
	/* Signal an error if the next character is not a digit: */
	if(!(cc[lastChar]&DIGIT))
		throw NumberError();
	
	/* Read an integral number part: */
	int result=0;
	while(cc[lastChar]&DIGIT)
		{
		result=result*10+int(lastChar-'0');
		lastChar=source.getc();
		}
	
	/* Negate the result if a minus sign was read: */
	if(negate)
		result=-result;
	
	/* Skip whitespace: */
	while(cc[lastChar]&WHITESPACE)
		lastChar=source.getc();
	
	return result;
	}

unsigned int ValueSource::readUnsignedInteger(void)
	{
	/* Signal an error if the next character is not a digit: */
	if(!(cc[lastChar]&DIGIT))
		throw NumberError();
	
	/* Read an integral number part: */
	unsigned int result=0;
	while(cc[lastChar]&DIGIT)
		{
		result=result*10+(unsigned int)(lastChar-'0');
		lastChar=source.getc();
		}
	
	/* Skip whitespace: */
	while(cc[lastChar]&WHITESPACE)
		lastChar=source.getc();
	
	return result;
	}

double ValueSource::readNumber(void)
	{
	/* Read a plus or minus sign: */
	bool negate=lastChar=='-';
	if(lastChar=='-'||lastChar=='+')
		lastChar=source.getc();
	
	/* Read an integral number part: */
	bool haveDigit=false;
	double result=0.0;
	while(cc[lastChar]&DIGIT)
		{
		haveDigit=true;
		result=result*10.0+double(lastChar-'0');
		lastChar=source.getc();
		}
	
	/* Check for a period: */
	if(lastChar=='.')
		{
		lastChar=source.getc();
		
		/* Read a fractional number part: */
		double fraction=0.0;
		double fractionBase=1.0;
		while(cc[lastChar]&DIGIT)
			{
			haveDigit=true;
			fraction=fraction*10.0+double(lastChar-'0');
			fractionBase*=10.0;
			lastChar=source.getc();
			}
		
		result+=fraction/fractionBase;
		}
	
	/* Signal an error if no digits were read in the integral or fractional part: */
	if(!haveDigit)
		throw NumberError();
	
	/* Negate the result if a minus sign was read: */
	if(negate)
		result=-result;
	
	/* Check for an exponent indicator: */
	if(lastChar=='e'||lastChar=='E')
		{
		lastChar=source.getc();
		
		/* Read a plus or minus sign: */
		bool negateExponent=lastChar=='-';
		if(lastChar=='-'||lastChar=='+')
			lastChar=source.getc();
		
		/* Check if there are any digits in the exponent: */
		if(!(cc[lastChar]&DIGIT))
			throw NumberError();
		
		/* Read the exponent: */
		double exponent=0.0;
		while(cc[lastChar]&DIGIT)
			{
			exponent=exponent*10.0+double(lastChar-'0');
			lastChar=source.getc();
			}
		
		/* Multiply the mantissa with the exponent: */
		result*=pow(10.0,negateExponent?-exponent:exponent);
		}
	
	/* Skip whitespace: */
	while(cc[lastChar]&WHITESPACE)
		lastChar=source.getc();
	
	return result;
	}

}
