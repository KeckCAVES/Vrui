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

#ifndef MISC_MESSAGELOGGER_INCLUDED
#define MISC_MESSAGELOGGER_INCLUDED

#include <stdarg.h>
#include <Misc/RefCounted.h>
#include <Misc/Autopointer.h>

namespace Misc {

class MessageLogger:public RefCounted
	{
	/* Embedded classes: */
	public:
	enum Target // Enumerated type for message targets
		{
		Log, // Message can go to some log to be viewed off-line
		Console, // Message can go to some form of console to be viewed asynchronously
		User // Message needs to be seen by the user right away
		};
	
	enum MessageLevel // Enumerated type for message severity levels
		{
		Note=0, // Something of interest
		Warning=10, // Some operation had a sub-optimal outcome, but succeeded
		Error=20 // Some operation failed
		};
	
	/* Elements: */
	private:
	static Autopointer<MessageLogger> theMessageLogger; // Pointer to the currently installed message logger
	protected:
	int minMessageLevel; // Minimum message severity level that gets logged
	
	/* Protected methods: */
	protected:
	virtual void logMessageInternal(Target target,int messageLevel,const char* message); // Implementation of static logMessage method
	
	/* Constructors and destructors: */
	public:
	MessageLogger(void);
	virtual ~MessageLogger(void);
	
	/* Methods: */
	static Autopointer<MessageLogger> getMessageLogger(void) // Returns the currently installed message logger
		{
		return theMessageLogger;
		}
	static void setMessageLogger(Autopointer<MessageLogger> newMessageLogger); // Installs the given message logger
	int getMinMessageLevel(void) const // Returns the minimum message severity level that gets logged
		{
		return minMessageLevel;
		}
	virtual void setMinMessageLevel(int newMinMessageLevel); // Sets the minimum logged message severity level
	static void logMessage(Target target,int messageLevel,const char* message); // Logs a message of the given level
	static void logFormattedMessage(Target target,int messageLevel,const char* formatString,...); // Ditto, using a printf-style interface
	static void logFormattedMessage(Target target,int messageLevel,const char* formatString,va_list args); // Ditto, using a va_list variable argument structure
	};
	
/*********************
Convenience functions:
*********************/

inline void logNote(const char* message) // Logs a note to the message log
	{
	MessageLogger::logMessage(MessageLogger::Log,MessageLogger::Note,message);
	}

inline void logWarning(const char* message) // Logs a warning to the message log
	{
	MessageLogger::logMessage(MessageLogger::Log,MessageLogger::Warning,message);
	}

inline void logError(const char* message) // Logs an error to the message log
	{
	MessageLogger::logMessage(MessageLogger::Log,MessageLogger::Error,message);
	}

inline void formattedLogNote(const char* formatString,...) // Logs a formatted note to the message log
	{
	va_list args;
	va_start(args,formatString);
	MessageLogger::logFormattedMessage(MessageLogger::Log,MessageLogger::Note,formatString,args);
	va_end(args);
	}

inline void formattedLogWarning(const char* formatString,...) // Logs a formatted warning to the message log
	{
	va_list args;
	va_start(args,formatString);
	MessageLogger::logFormattedMessage(MessageLogger::Log,MessageLogger::Warning,formatString,args);
	va_end(args);
	}

inline void formattedLogError(const char* formatString,...) // Logs a formatted error to the message log
	{
	va_list args;
	va_start(args,formatString);
	MessageLogger::logFormattedMessage(MessageLogger::Log,MessageLogger::Error,formatString,args);
	va_end(args);
	}

inline void consoleNote(const char* message) // Logs a note to the console
	{
	MessageLogger::logMessage(MessageLogger::Console,MessageLogger::Note,message);
	}

inline void consoleWarning(const char* message) // Logs a warning to the console
	{
	MessageLogger::logMessage(MessageLogger::Console,MessageLogger::Warning,message);
	}

inline void consoleError(const char* message) // Logs an error to the console
	{
	MessageLogger::logMessage(MessageLogger::Console,MessageLogger::Error,message);
	}

inline void formattedConsoleNote(const char* formatString,...) // Logs a formatted note to the console
	{
	va_list args;
	va_start(args,formatString);
	MessageLogger::logFormattedMessage(MessageLogger::Console,MessageLogger::Note,formatString,args);
	va_end(args);
	}

inline void formattedConsoleWarning(const char* formatString,...) // Logs a formatted warning to the console
	{
	va_list args;
	va_start(args,formatString);
	MessageLogger::logFormattedMessage(MessageLogger::Console,MessageLogger::Note,formatString,args);
	va_end(args);
	}

inline void formattedConsoleError(const char* formatString,...) // Logs a formatted error to the console
	{
	va_list args;
	va_start(args,formatString);
	MessageLogger::logFormattedMessage(MessageLogger::Console,MessageLogger::Error,formatString,args);
	va_end(args);
	}

inline void userNote(const char* message) // Logs a note to the user
	{
	MessageLogger::logMessage(MessageLogger::User,MessageLogger::Note,message);
	}

inline void userWarning(const char* message) // Logs a warning to the user
	{
	MessageLogger::logMessage(MessageLogger::User,MessageLogger::Warning,message);
	}

inline void userError(const char* message) // Logs an error to the user
	{
	MessageLogger::logMessage(MessageLogger::User,MessageLogger::Error,message);
	}

inline void formattedUserNote(const char* formatString,...) // Logs a formatted note to the user
	{
	va_list args;
	va_start(args,formatString);
	MessageLogger::logFormattedMessage(MessageLogger::User,MessageLogger::Note,formatString,args);
	va_end(args);
	}

inline void formattedUserWarning(const char* formatString,...) // Logs a formatted warning to the user
	{
	va_list args;
	va_start(args,formatString);
	MessageLogger::logFormattedMessage(MessageLogger::User,MessageLogger::Warning,formatString,args);
	va_end(args);
	}

inline void formattedUserError(const char* formatString,...) // Logs a formatted error to the user
	{
	va_list args;
	va_start(args,formatString);
	MessageLogger::logFormattedMessage(MessageLogger::User,MessageLogger::Error,formatString,args);
	va_end(args);
	}

}

#endif
