/***********************************************************************
ScreenSaverInhibitor - Abstract base class to prevent a screen saver
from blanking the display or entering low-power mode while a VR
application is running.
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

#ifndef VRUI_INTERNAL_SCREENSAVERINHIBITOR_INCLUDED
#define VRUI_INTERNAL_SCREENSAVERINHIBITOR_INCLUDED

namespace Vrui {

class ScreenSaverInhibitor
	{
	/* Constructors and destructors: */
	public:
	ScreenSaverInhibitor(void) // Creates an inhibitor and inhibits the screen saver
		{
		}
	virtual ~ScreenSaverInhibitor(void) // Un-inhibits the screen saver and destroys the inhibitor
		{
		}
	};

}

#endif
