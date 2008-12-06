/***********************************************************************
GLFogCoordTemplates - Overloaded versions of glFogCoord...() functions.
Copyright (c) 2004-2005 Oliver Kreylos

This file is part of the OpenGL C++ Wrapper Library (GLWrappers).

The OpenGL C++ Wrapper Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenGL C++ Wrapper Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenGL C++ Wrapper Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef GLFOGCOORDTEMPLATES_INCLUDED
#define GLFOGCOORDTEMPLATES_INCLUDED

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/GLVector.h>

#if defined(GL_VERSION_1_4) && GL_VERSION_1_4 == 1 && defined(GL_GLEXT_PROTOTYPES)

/*******************************************************
Overloaded versions of component-based glFogCoord calls:
*******************************************************/

inline void glFogCoord(GLfloat f)
	{
	glFogCoordf(f);
	}

inline void glFogCoord(GLdouble f)
	{
	glFogCoordd(f);
	}

/****************************************************
Dummy generic version of array-based glFogCoord call:
****************************************************/

template <GLsizei numComponents,class ScalarParam>
void glFogCoord(const ScalarParam components[numComponents]);

/****************************************************
Specialized versions of array-based glFogCoord calls:
****************************************************/

template <>
inline void glFogCoord<1,GLfloat>(const GLfloat components[1])
	{
	glFogCoordfv(components);
	}

template <>
inline void glFogCoord<1,GLdouble>(const GLdouble components[1])
	{
	glFogCoorddv(components);
	}

/**************************************
Overloaded versions of glFogCoord call:
**************************************/

template <class ScalarParam>
inline void glFogCoord(const GLVector<ScalarParam,1>& param)
	{
	glFogCoord<1>(param.getXyzw());
	}

#endif

#endif
