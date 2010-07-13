/***********************************************************************
alc.h - Wrapper around OpenAL's alc.h include file to protect client
code from missing OpenAL installations, or different include file
locations.
Copyright (c) 2010 Oliver Kreylos

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

#ifndef VRUI_ALC_INCLUDED
#define VRUI_ALC_INCLUDED

#ifdef VRUI_USE_OPENAL
#ifdef __DARWIN__
#include <OpenAL/alc.h>
#else
#include <AL/alc.h>
#endif
#endif

#endif
