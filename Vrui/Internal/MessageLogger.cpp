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

#include <Vrui/Internal/MessageLogger.h>

#include <ctype.h>
#include <unistd.h>
#include <string>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Margin.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <Vrui/Vrui.h>

namespace Vrui {

namespace {

/****************
Helper functions:
****************/

void closeMessageDialog(Misc::CallbackData* cbData) // Helper function to close a message dialog when its confirmation button is pressed
	{
	GLMotif::Button::CallbackData* buttonCbData=dynamic_cast<GLMotif::Button::CallbackData*>(cbData);
	if(buttonCbData!=0)
		{
		/* Close the top-level widget to which the button belongs: */
		getWidgetManager()->deleteWidget(buttonCbData->button->getRoot());
		}
	}

}

/******************************
Methods of class MessageLogger:
******************************/

void MessageLogger::logMessageInternal(Target target,int messageLevel,const char* message)
	{
	/* Handle messages based on target: */
	if(target==Log)
		{
		/* For now, write the message directly to stdout if this is the master node: */
		if(Vrui::isMaster())
			{
			/* Append a newline to the message: */
			std::string paddedMessage=message;
			paddedMessage.append("\n");
			
			/* Write message directly to stdout, bypassing any buffers: */
			if(write(STDOUT_FILENO,paddedMessage.data(),paddedMessage.size())!=ssize_t(paddedMessage.size()))
				{
				/* Whatcha gonna do? */
				}
			}
		}
	else if(target==Console||userToConsole)
		{
		/* Write the message directly to stderr if this is the master node: */
		if(Vrui::isMaster())
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
		}
	else
		{
		/* Present a note/warning/error dialog to the user: */
		showMessageDialog(messageLevel,message);
		}
	}

void MessageLogger::showMessageDialog(int messageLevel,const char* messageString)
	{
	/* Assemble the dialog title: */
	std::string title=messageLevel<Warning?"Note":messageLevel<Error?"Warning":"Error";
	
	/* Check if the messageString starts with a source identifier: */
	const char* colonPtr=0;
	for(const char* mPtr=messageString;*mPtr!='\0'&&!isspace(*mPtr);++mPtr)
		if(*mPtr==':')
			colonPtr=mPtr;
	
	if(colonPtr!=0&&colonPtr[1]!='\0'&&isspace(colonPtr[1]))
		{
		/* Append the messageString source to the title: */
		title.append(" from ");
		title.append(messageString,colonPtr);
		
		/* Cut the source from the messageString: */
		messageString=colonPtr+1;
		}
	
	/* Create a popup window: */
	GLMotif::PopupWindow* messageDialog=new GLMotif::PopupWindow("VruiMessageLoggerMessage",getWidgetManager(),title.c_str());
	messageDialog->setResizableFlags(false,false);
	messageDialog->setHideButton(false);
	
	GLMotif::RowColumn* message=new GLMotif::RowColumn("Message",messageDialog,false);
	message->setOrientation(GLMotif::RowColumn::VERTICAL);
	message->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	
	/* Skip initial whitespace in the message message: */
	const char* linePtr=messageString;
	while(*linePtr!='\0'&&isspace(*linePtr))
		++linePtr;
	
	/* Break the message message into multiple lines: */
	while(*linePtr!='\0')
		{
		/* Find potential line break points: */
		const char* breakPtr=0;
		const char* cPtr=linePtr;
		do
			{
			/* Find the end of the current word: */
			while(!isspace(*cPtr)&&*cPtr!='-'&&*cPtr!='/'&&*cPtr!='\0')
				++cPtr;
			
			/* Skip past dashes and slashes: */
			while(*cPtr=='-'||*cPtr=='/')
				++cPtr;
			
			/* If the line is already too long, and there is a previous break point, break there: */
			if(cPtr-linePtr>=40&&breakPtr!=0)
				break;
			
			/* Mark the break point: */
			breakPtr=cPtr;
			
			/* Skip whitespace: */
			while(isspace(*cPtr)&&*cPtr!='\0')
				++cPtr;
			}
		while(cPtr-linePtr<40&&*breakPtr!='\n'&&*breakPtr!='\0');
		
		/* Add the current line: */
		new GLMotif::Label("messageLine",message,linePtr,breakPtr);
		
		/* Go to the beginning of the next line: */
		linePtr=breakPtr;
		while(isspace(*linePtr)&&*linePtr!='\0')
			++linePtr;
		}
	
	/* Add an acknowledgment button: */
	GLMotif::Margin* buttonMargin=new GLMotif::Margin("ButtonMargin",message,false);
	buttonMargin->setAlignment(GLMotif::Alignment::RIGHT);
	GLMotif::Button* okButton=new GLMotif::Button("OkButton",buttonMargin,messageLevel<Warning?"Gee, thanks":messageLevel<Error?"Alright then":"Darn it!");
	okButton->getSelectCallbacks().add(closeMessageDialog);
	
	buttonMargin->manageChild();
	
	message->manageChild();
	
	/* Show the popup window: */
	popupPrimaryWidget(messageDialog);
	}

MessageLogger::MessageLogger(void)
	:userToConsole(true)
	{
	}

MessageLogger::~MessageLogger(void)
	{
	}

void MessageLogger::setUserToConsole(bool newUserToConsole)
	{
	userToConsole=newUserToConsole;
	}

}
