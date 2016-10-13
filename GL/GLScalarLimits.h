/***********************************************************************
GLScalarLimits - Helper class to store limit values of integral scalar
data types used in OpenGL to enable automatic type conversion for range-
limited scalar values.
Copyright (c) 2004-2016 Oliver Kreylos

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

#ifndef GLSCALARLIMITS_INCLUDED
#define GLSCALARLIMITS_INCLUDED

#include <Math/Math.h>
#include <GL/gl.h>

struct GLScalarUnsignedTrait
	{
	};

struct GLScalarSignedTrait
	{
	};

struct GLScalarFloatTrait
	{
	};

/**********************************************
Dummy generic class for limits of scalar types:
**********************************************/

template <class ScalarParam>
class GLScalarLimits
	{
	};

/*****************************************************
Specialized versions for OpenGL integral scalar types:
*****************************************************/

template <>
class GLScalarLimits<GLubyte>
	{
	/* Embedded classes: */
	public:
	typedef GLScalarUnsignedTrait Trait; // GLubyte is unsigned
	typedef GLfloat AccumulatorScalar; // Type to use when accumulating or interpolating
	
	/* Elements: */
	static const GLenum type=GL_UNSIGNED_BYTE;
	static const GLubyte min=0U; // Minimum scalar value
	static const GLubyte max=255U; // Maximum scalar value
	static const GLubyte scale=255U; // Scale value for conversion from/to float
	
	/* Methods: */
	static GLubyte fromAccumulator(AccumulatorScalar s) // Convert back to original type after accumulating
		{
		return GLubyte(s+0.5f);
		}
	};

template <>
class GLScalarLimits<GLbyte>
	{
	/* Embedded classes: */
	public:
	typedef GLScalarSignedTrait Trait; // GLbyte is signed
	typedef GLfloat AccumulatorScalar; // Type to use when accumulating or interpolating
	
	/* Elements: */
	static const GLenum type=GL_BYTE;
	static const GLbyte min=-128; // Minimum scalar value
	static const GLbyte max=127; // Maximum scalar value
	static const GLubyte scale=255U; // Scale value for conversion from/to float
	
	/* Methods: */
	static GLbyte fromAccumulator(AccumulatorScalar s) // Convert back to original type after accumulating
		{
		return GLbyte(Math::floor(s+0.5f));
		}
	};

template <>
class GLScalarLimits<GLushort>
	{
	/* Embedded classes: */
	public:
	typedef GLScalarUnsignedTrait Trait; // GLushort is unsigned
	typedef GLfloat AccumulatorScalar; // Type to use when accumulating or interpolating
	
	/* Elements: */
	static const GLenum type=GL_UNSIGNED_SHORT;
	static const GLushort min=0U; // Minimum scalar value
	static const GLushort max=65535U; // Maximum scalar value
	static const GLushort scale=65535U; // Scale value for conversion from/to float
	
	/* Methods: */
	static GLushort fromAccumulator(AccumulatorScalar s) // Convert back to original type after accumulating
		{
		return GLushort(s+0.5f);
		}
	};

template <>
class GLScalarLimits<GLshort>
	{
	/* Embedded classes: */
	public:
	typedef GLScalarSignedTrait Trait; // GLshort is signed
	typedef GLfloat AccumulatorScalar; // Type to use when accumulating or interpolating
	
	/* Elements: */
	static const GLenum type=GL_SHORT;
	static const GLshort min=-32768; // Minimum scalar value
	static const GLshort max=32767; // Maximum scalar value
	static const GLushort scale=65535U; // Scale value for conversion from/to float
	
	/* Methods: */
	static GLshort fromAccumulator(AccumulatorScalar s) // Convert back to original type after accumulating
		{
		return GLshort(Math::floor(s+0.5f));
		}
	};

template <>
class GLScalarLimits<GLuint>
	{
	/* Embedded classes: */
	public:
	typedef GLScalarUnsignedTrait Trait; // GLuint is unsigned
	typedef GLfloat AccumulatorScalar; // Type to use when accumulating or interpolating
	
	/* Elements: */
	static const GLenum type=GL_UNSIGNED_INT;
	static const GLuint min=0U; // Minimum scalar value
	static const GLuint max=4294967295U; // Maximum scalar value
	static const GLuint scale=4294967295U; // Scale value for conversion from/to float
	
	/* Methods: */
	static GLuint fromAccumulator(AccumulatorScalar s) // Convert back to original type after accumulating
		{
		return GLuint(s+0.5f);
		}
	};

template <>
class GLScalarLimits<GLint>
	{
	/* Embedded classes: */
	public:
	typedef GLScalarSignedTrait Trait; // GLint is signed
	typedef GLfloat AccumulatorScalar; // Type to use when accumulating or interpolating
	
	/* Elements: */
	static const GLenum type=GL_INT;
	static const GLint min=-2147483647-1; // Minimum scalar value
	static const GLint max=2147483647; // Maximum scalar value
	static const GLuint scale=4294967295U; // Scale value for conversion from/to float
	
	/* Methods: */
	static GLint fromAccumulator(AccumulatorScalar s) // Convert back to original type after accumulating
		{
		return GLint(Math::floor(s+0.5f));
		}
	};

/**************************************************
Specialized versions for OpenGL float scalar types:
**************************************************/

template <>
class GLScalarLimits<GLfloat>
	{
	/* Embedded classes: */
	public:
	typedef GLScalarFloatTrait Trait; // GLfloat is float
	typedef GLfloat AccumulatorScalar; // Type to use when accumulating or interpolating
	
	/* Elements: */
	static const GLenum type=GL_FLOAT;
	static const GLfloat min; // Minimum scalar value
	static const GLfloat max; // Maximum scalar value
	
	/* Methods: */
	static GLfloat fromAccumulator(AccumulatorScalar s) // Convert back to original type after accumulating
		{
		return s;
		}
	};

template <>
class GLScalarLimits<GLdouble>
	{
	/* Embedded classes: */
	public:
	typedef GLScalarFloatTrait Trait; // GLdouble is float
	typedef GLdouble AccumulatorScalar; // Type to use when accumulating or interpolating
	
	/* Elements: */
	static const GLenum type=GL_DOUBLE;
	static const GLdouble min; // Minimum scalar value
	static const GLdouble max; // Maximum scalar value
	
	/* Methods: */
	static GLfloat fromAccumulator(AccumulatorScalar s) // Convert back to original type after accumulating
		{
		return GLfloat(s);
		}
	};

#endif
