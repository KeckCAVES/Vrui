/***********************************************************************
InputDeviceAdapterHID - Class to directly connect USB human interface
devices (HID) to a set of Vrui input devices. Cross-platform class using
the event device API under Linux and the IOHIDLib API under Mac OSX.
Copyright (c) 2009 Oliver Kreylos

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

#ifndef VRUI_INPUTDEVICEADAPTERHID_INCLUDED
#define VRUI_INPUTDEVICEADAPTERHID_INCLUDED

/*****************************************
Include the OS-specific class declaration:
*****************************************/

#ifdef __LINUX__
#include <Vrui/InputDeviceAdapterHID.Linux.h>
#endif

#ifdef __DARWIN__
#include <Vrui/InputDeviceAdapterHID.MacOSX.h>
#endif

#endif
