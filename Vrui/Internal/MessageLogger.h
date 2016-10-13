/***********************************************************************
MessageLogger - Class derived from Misc::MessageLogger to log and
present messages inside a Vrui application.
Copyright (c) 2015 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef VRUI_MESSAGELOGGER_INCLUDED
#define VRUI_MESSAGELOGGER_INCLUDED

#include <Misc/MessageLogger.h>

namespace Vrui {

class MessageLogger:public Misc::MessageLogger
	{
	/* Elements: */
	private:
	bool userToConsole; // Flag whether to route user messages to the console
	
	/* Protected methods from Misc::MessageLogger: */
	protected:
	virtual void logMessageInternal(Target target,int messageLevel,const char* message);
	
	/* Private methods: */
	void showMessageDialog(int messageLevel,const char* messageString); // Displays a message as a GLMotif dialog
	
	/* Constructors and destructors: */
	public:
	MessageLogger(void);
	virtual ~MessageLogger(void);
	
	/* New methods: */
	void setUserToConsole(bool newUserToConsole); // If true, user messages are re-routed to the console
	};

}

#endif
