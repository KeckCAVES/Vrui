/***********************************************************************
MessageLogger - Base class for objects that receive and log messages.
Copyright (c) 2015 Oliver Kreylos

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

#include <Misc/MessageLogger.h>

#include <stdio.h>
#include <unistd.h>
#include <string>

namespace Misc {

/**************************************
Static elements of class MessageLogger:
**************************************/

Autopointer<MessageLogger> MessageLogger::theMessageLogger(new MessageLogger);

/******************************
Methods of class MessageLogger:
******************************/

void MessageLogger::logMessageInternal(MessageLogger::Target target,int messageLevel,const char* message)
	{
	/* Append a newline to the message: */
	std::string paddedMessage=message;
	paddedMessage.append("\n");
	
	/* Write message directly to stderr, bypassing any buffers: */
	if(write(STDERR_FILENO,paddedMessage.data(),paddedMessage.size())!=ssize_t(paddedMessage.size()))
		{
		/* Whatcha gonna do? */
		}
	}

MessageLogger::MessageLogger(void)
	:minMessageLevel(Note)
	{
	}

MessageLogger::~MessageLogger(void)
	{
	}

void MessageLogger::setMessageLogger(Autopointer<MessageLogger> newMessageLogger)
	{
	theMessageLogger=newMessageLogger;
	}

void MessageLogger::setMinMessageLevel(int newMinMessageLevel)
	{
	minMessageLevel=newMinMessageLevel;
	}

void MessageLogger::logMessage(MessageLogger::Target target,int messageLevel,const char* message)
	{
	/* Log the message if there is a logger and the message exceeds the minimum severity level: */
	if(theMessageLogger!=0&&messageLevel>=theMessageLogger->minMessageLevel)
		theMessageLogger->logMessageInternal(target,messageLevel,message);
	}

void MessageLogger::logFormattedMessage(MessageLogger::Target target,int messageLevel,const char* formatString,...)
	{
	/* Log the message if there is a logger and the message exceeds the minimum severity level: */
	if(theMessageLogger!=0&&messageLevel>=theMessageLogger->minMessageLevel)
		{
		/* Print the message into a local buffer: */
		char message[1024]; // Buffer for error messages - hopefully long enough...
		va_list ap;
		va_start(ap,formatString);
		vsnprintf(message,sizeof(message),formatString,ap);
		va_end(ap);
		
		/* Log the message: */
		theMessageLogger->logMessageInternal(target,messageLevel,message);
		}
	}

void MessageLogger::logFormattedMessage(MessageLogger::Target target,int messageLevel,const char* formatString,va_list args)
	{
	/* Log the message if there is a logger and the message exceeds the minimum severity level: */
	if(theMessageLogger!=0&&messageLevel>=theMessageLogger->minMessageLevel)
		{
		/* Print the message into a local buffer: */
		char message[1024]; // Buffer for error messages - hopefully long enough...
		vsnprintf(message,sizeof(message),formatString,args);
		
		/* Log the message: */
		theMessageLogger->logMessageInternal(target,messageLevel,message);
		}
	}

}
