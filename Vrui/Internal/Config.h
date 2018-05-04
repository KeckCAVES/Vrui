/***********************************************************************
Config - Internal configuration header file for the Vrui Library.
Copyright (c) 2014-2018 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_CONFIG_INCLUDED
#define VRUI_INTERNAL_CONFIG_INCLUDED

#define VRUI_INTERNAL_CONFIG_HAVE_XRANDR 1
#define VRUI_INTERNAL_CONFIG_HAVE_XINPUT2 1
#define VRUI_INTERNAL_CONFIG_HAVE_LIBDBUS 1

#define VRUI_INTERNAL_CONFIG_VRWINDOW_USE_SWAPGROUPS 0

#define VRUI_INTERNAL_CONFIG_INPUT_H_HAS_STRUCTS 1

#define VRUI_INTERNAL_CONFIG_CONFIGFILENAME "Vrui"
#define VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX ".cfg"
#define VRUI_INTERNAL_CONFIG_SYSCONFIGDIR "/home/okreylos/Share"
#define VRUI_INTERNAL_CONFIG_HAVE_USERCONFIGFILE 1
#define VRUI_INTERNAL_CONFIG_USERCONFIGDIR ".config/Vrui-devel"
#define VRUI_INTERNAL_CONFIG_APPCONFIGDIR "Applications"
#define VRUI_INTERNAL_CONFIG_DEFAULTROOTSECTION "Desktop"

#define VRUI_INTERNAL_CONFIG_VERSION 4005003
#define VRUI_INTERNAL_CONFIG_ETCDIR "/home/okreylos/Share"
#define VRUI_INTERNAL_CONFIG_SHAREDIR "/home/okreylos/Share"

#endif
