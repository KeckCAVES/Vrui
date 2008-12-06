/***********************************************************************
Container - Base class for GLMotif UI components that contain other
components.
Copyright (c) 2001-2005 Oliver Kreylos

This file is part of the GLMotif Widget Library (GLMotif).

The GLMotif Widget Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GLMotif Widget Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the GLMotif Widget Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <GLMotif/Container.h>

namespace GLMotif {

/**************************
Methods of class Container:
**************************/

Container::Container(const char* sName,Container* sParent,bool sManageChild)
	:Widget(sName,sParent,false)
	{
	/* Manage me: */
	if(sManageChild)
		manageChild();
	}

}
