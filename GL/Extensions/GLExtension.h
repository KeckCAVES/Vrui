/***********************************************************************
GLExtension - Abstract base class for OpenGL extension classes managed
by the OpenGL extension manager.
Copyright (c) 2005-2006 Oliver Kreylos

This file is part of the OpenGL Support Library (GLSupport).

The OpenGL Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenGL Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenGL Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef GLEXTENSIONS_GLEXTENSION_INCLUDED
#define GLEXTENSIONS_GLEXTENSION_INCLUDED

/* Forward declarations: */
class GLExtensionManager;

class GLExtension
	{
	friend class GLExtensionManager;
	
	/* Elements: */
	private:
	GLExtension* succ; // Pointer to next extension in list
	
	/* Constructors and destructors: */
	public:
	GLExtension(void) // Initializes the extension in the current OpenGL context
		:succ(0)
		{
		}
	virtual ~GLExtension(void) // Uninitializes the extension in the current OpenGL context
		{
		}
	
	/* Methods: */
	virtual const char* getExtensionName(void) const =0; // Returns the OpenGL name of the extension
	virtual void activate(void) =0; // Activates the extension in the current OpenGL context
	virtual void deactivate(void) =0; // Deactivates the extension in the current OpenGL context
	};

#endif
