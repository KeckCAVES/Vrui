/***********************************************************************
ScreenSaverInhibitorDBus - Screen saver inhibitor using the ScreenSaver
DBus interface exposed by most Linux desktop environments.
Copyright (c) 2016 Oliver Kreylos

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

#include <Vrui/Internal/Linux/ScreenSaverInhibitorDBus.h>

#include <string.h>
#include <stdexcept>
#include <Misc/ThrowStdErr.h>

namespace Vrui {

namespace {

/****************
Helper functions:
****************/

std::string findScreenSaverService(DBusConnection* connection)
	{
	std::string result;
	
	/* Create a new method call object: */
	DBusMessage* message=dbus_message_new_method_call("org.freedesktop.DBus",
	                                                  "/org/freedesktop/DBus",
	                                                  "org.freedesktop.DBus",
	                                                  "ListNames");
	if(message==0)
		return result;
	
	/* Send the message: */
	DBusPendingCall* pending=0;
	if(!dbus_connection_send_with_reply(connection,message,&pending,-1)||pending==0)
		{
		dbus_message_unref(message);
		return result;
		}
	dbus_connection_flush(connection);
	
	/* Release the message object: */
	dbus_message_unref(message);
	
	/* Block until the message reply arrives: */
	dbus_pending_call_block(pending);
	
	/* Get the message reply: */
	message=dbus_pending_call_steal_reply(pending);
	if(message==0)
		{
		dbus_pending_call_unref(pending);
		return result;
		}
	
	/* Release the pending message handle: */
	dbus_pending_call_unref(pending);
	
	/* Extract the method result: */
	DBusMessageIter arguments;
	if(!dbus_message_iter_init(message,&arguments))
		{
		dbus_message_unref(message);
		return result;
		}
	DBusMessageIter strings;
	dbus_message_iter_recurse(&arguments,&strings);
	do
		{
		if(dbus_message_iter_get_arg_type(&strings)==DBUS_TYPE_STRING)
			{
			const char* string;
			dbus_message_iter_get_basic(&strings,&string);
			
			/* Find the last component of the service name: */
			const char* lastCompPtr=string;
			for(const char* sPtr=string;*sPtr!='\0';++sPtr)
				if(*sPtr=='.')
					lastCompPtr=sPtr+1;
			
			/* Check if this service is a screen saver: */
			if(strcmp(lastCompPtr,"ScreenSaver")==0)
				result=string;
			}
		}
	while(dbus_message_iter_next(&strings));
	
	/* Release the reply message object: */
	dbus_message_unref(message);
	
	return result;
	}

bool inhibitScreenSaver(DBusConnection* connection,const std::string& service,const std::string& path,const std::string& interface,unsigned int& cookie)
	{
	/* Create a new method call object: */
	DBusMessage* message=dbus_message_new_method_call(service.c_str(),path.c_str(),interface.c_str(),"Inhibit");
	if(message==0)
		return false;
	
	/* Set method arguments: */
	DBusMessageIter arguments;
	dbus_message_iter_init_append(message,&arguments);
	bool argsOk=true;
	const char* appName="Vrui";
	argsOk=argsOk&&dbus_message_iter_append_basic(&arguments,DBUS_TYPE_STRING,&appName);
	const char* reason="VR application running";
	argsOk=argsOk&&dbus_message_iter_append_basic(&arguments,DBUS_TYPE_STRING,&reason);
	if(!argsOk)
		{
		dbus_message_unref(message);
		return false;
		}
	
	/* Send the message: */
	DBusPendingCall* pending=0;
	if(!dbus_connection_send_with_reply(connection,message,&pending,-1)||pending==0)
		{
		dbus_message_unref(message);
		return false;
		}
	dbus_connection_flush(connection);
	
	/* Release the message object: */
	dbus_message_unref(message);
	
	/* Block until the message reply arrives: */
	dbus_pending_call_block(pending);
	
	/* Get the message reply: */
	message=dbus_pending_call_steal_reply(pending);
	if(message==0)
		{
		dbus_pending_call_unref(pending);
		return false;
		}
	
	/* Release the pending message handle: */
	dbus_pending_call_unref(pending);
	
	/* Extract the method result: */
	dbus_uint32_t methodResult;
	if(!dbus_message_iter_init(message,&arguments))
		{
		dbus_message_unref(message);
		return false;
		}
	if(dbus_message_iter_get_arg_type(&arguments)!=DBUS_TYPE_UINT32)
		{
		dbus_message_unref(message);
		return false;
		}
	dbus_message_iter_get_basic(&arguments,&methodResult);
	cookie=methodResult;
	
	/* Release the reply message object: */
	dbus_message_unref(message);
	
	return true;
	}

bool uninhibitScreenSaver(DBusConnection* connection,const std::string& service,const std::string& path,const std::string& interface,unsigned int cookie)
	{
	/* Create a new method call object: */
	DBusMessage* message=dbus_message_new_method_call(service.c_str(),path.c_str(),interface.c_str(),"Uninhibit");
	if(message==0)
		return false;
	
	/* Set method arguments: */
	DBusMessageIter arguments;
	dbus_message_iter_init_append(message,&arguments);
	bool argsOk=true;
	dbus_uint32_t methodArgument=cookie;
	argsOk=argsOk&&dbus_message_iter_append_basic(&arguments,DBUS_TYPE_UINT32,&methodArgument);
	if(!argsOk)
		{
		dbus_message_unref(message);
		return false;
		}
	
	/* Send the message: */
	DBusPendingCall* pending=0;
	if(!dbus_connection_send_with_reply(connection,message,&pending,-1)||pending==0)
		{
		dbus_message_unref(message);
		return false;
		}
	dbus_connection_flush(connection);
	
	/* Release the message object: */
	dbus_message_unref(message);
	
	/* Block until the message reply arrives: */
	dbus_pending_call_block(pending);
	
	/* Get the message reply: */
	message=dbus_pending_call_steal_reply(pending);
	if(message==0)
		{
		dbus_pending_call_unref(pending);
		return false;
		}
	
	/* Release the pending message handle: */
	dbus_pending_call_unref(pending);
	
	/* Release the reply message object: */
	dbus_message_unref(message);
	
	return true;
	}

}

/*****************************************
Methods of class ScreenSaverInhibitorDBus:
*****************************************/

ScreenSaverInhibitorDBus::ScreenSaverInhibitorDBus(void)
	:connection(0)
	{
	/* Initialize the error handler: */
	dbus_error_init(&error);
	
	/* Connect to the session message bus: */
	connection=dbus_bus_get(DBUS_BUS_SESSION,&error);
	if(dbus_error_is_set(&error))
		{
		dbus_error_free(&error);
		Misc::throwStdErr("Vrui::ScreenSaverInhibitorDBus: Unable to connect to session message bus due to error %s",error.message);
		}
	else if(connection==0)
		{
		dbus_error_free(&error);
		Misc::throwStdErr("Vrui::ScreenSaverInhibitorDBus: Unable to connect to session message bus due to unknown error");
		}
	
	/* Find a screen saver object on the session DBus: */
	screenSaverService=findScreenSaverService(connection);
	if(screenSaverService.empty())
		{
		dbus_connection_unref(connection);
		dbus_error_free(&error);
		throw std::runtime_error("Vrui::ScreenSaverInhibitorDBus: No screen saver found on session DBus");
		}
	
	/* Create the appropriate object path and interface: */
	screenSaverPath.push_back('/');
	for(std::string::iterator sssIt=screenSaverService.begin();sssIt!=screenSaverService.end();++sssIt)
		screenSaverPath.push_back(*sssIt!='.'?*sssIt:'/');
	screenSaverInterface=screenSaverService;
	
	/* Inhibit the screen saver: */
	if(!inhibitScreenSaver(connection,screenSaverService,screenSaverPath,screenSaverInterface,inhibitCookie))
		{
		dbus_connection_unref(connection);
		dbus_error_free(&error);
		throw std::runtime_error("Vrui::ScreenSaverInhibitorDBus: Unable to inhibit screen saver");
		}
	}

ScreenSaverInhibitorDBus::~ScreenSaverInhibitorDBus(void)
	{
	/* Uninhibit the screen saver: */
	uninhibitScreenSaver(connection,screenSaverService,screenSaverPath,screenSaverInterface,inhibitCookie);
	
	/* Close the DBus connection: */
	dbus_connection_unref(connection);
	dbus_error_free(&error);
	}

}
