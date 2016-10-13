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

#ifndef VRUI_INTERNAL_SCREENSAVERINHIBITORDBUS_INCLUDED
#define VRUI_INTERNAL_SCREENSAVERINHIBITORDBUS_INCLUDED

#include <dbus/dbus.h>
#include <string>
#include <Vrui/Internal/ScreenSaverInhibitor.h>

namespace Vrui {

class ScreenSaverInhibitorDBus:public ScreenSaverInhibitor
	{
	/* Elements: */
	private:
	DBusError error; // Error handler for DBus communication
	DBusConnection* connection; // Persistent connection to the session DBus
	std::string screenSaverService; // Destination object providing the screen saver service
	std::string screenSaverPath; // Path name of the screen saver service object
	std::string screenSaverInterface; // Name of the screen saver interface
	unsigned int inhibitCookie; // Cookie identifying the inhibit request
	
	/* Constructors and destructors: */
	public:
	ScreenSaverInhibitorDBus(void);
	virtual ~ScreenSaverInhibitorDBus(void);
	};

}

#endif
