/***********************************************************************
GLValueCoders - Value coder classes for OpenGL abstraction classes.
Copyright (c) 2004-2005 Oliver Kreylos

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

#include <ctype.h>
#include <stdio.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLMaterial.h>

#include <GL/GLValueCoders.h>

namespace Misc {

/****************************************************
Methods of class ValueCoder<GLColor<ScalarParam,3> >:
****************************************************/

template <class ScalarParam>
std::string ValueCoder<GLColor<ScalarParam,3> >::encode(const GLColor<ScalarParam,3>& value)
	{
	/* Convert color into vector of GLdoubles: */
	GLColor<GLdouble,3> dv(value);
	
	/* Return the encoded vector: */
	return ValueCoderArray<GLdouble>::encode(3,dv.getRgba());
	}

template <class ScalarParam>
GLColor<ScalarParam,3> ValueCoder<GLColor<ScalarParam,3> >::decode(const char* start,const char* end,const char** decodeEnd)
	{
	/* Decode string into array of GLdoubles: */
	GLdouble components[3];
	int numComponents=ValueCoderArray<GLdouble>::decode(3,components,start,end,decodeEnd);
	
	/* Check for correct vector size: */
	if(numComponents!=3)
		throw DecodingError(std::string("Wrong number of components in ")+std::string(start,end));
	
	/* Return result color: */
	return GLColor<ScalarParam,3>(components);
	}

/****************************************************
Methods of class ValueCoder<GLColor<ScalarParam,4> >:
****************************************************/

template <class ScalarParam>
std::string ValueCoder<GLColor<ScalarParam,4> >::encode(const GLColor<ScalarParam,4>& value)
	{
	/* Convert color into vector of GLdoubles: */
	GLColor<GLdouble,4> dv(value);
	
	/* Only encode three components if alpha is default value: */
	int numComponents=dv[3]==1.0?3:4;
	
	/* Return the encoded vector: */
	return ValueCoderArray<GLdouble>::encode(numComponents,dv.getRgba());
	}

template <class ScalarParam>
GLColor<ScalarParam,4> ValueCoder<GLColor<ScalarParam,4> >::decode(const char* start,const char* end,const char** decodeEnd)
	{
	/* Decode string into array of GLdoubles: */
	GLdouble components[4];
	int numComponents=ValueCoderArray<GLdouble>::decode(4,components,start,end,decodeEnd);
	
	/* Check for correct vector size: */
	if(numComponents<3||numComponents>4)
		throw DecodingError(std::string("Wrong number of components in ")+std::string(start,end));
	
	/* Set default alpha value for three-component colors: */
	if(numComponents==3)
		components[3]=1.0;
	
	/* Return result color: */
	return GLColor<ScalarParam,4>(components);
	}

/***************************************
Methods of class ValueCoder<GLMaterial>:
***************************************/

std::string ValueCoder<GLMaterial>::encode(const GLMaterial& value)
	{
	std::string result="{ ";
	result+="Ambient = ";
	result+=ValueCoder<GLMaterial::Color>::encode(value.ambient);
	result+="; ";
	result+="Diffuse = ";
	result+=ValueCoder<GLMaterial::Color>::encode(value.diffuse);
	result+="; ";
	result+="Specular = ";
	result+=ValueCoder<GLMaterial::Color>::encode(value.specular);
	result+="; ";
	result+="Shininess = ";
	result+=ValueCoder<GLfloat>::encode(value.shininess);
	result+="; ";
	result+="Emission = ";
	result+=ValueCoder<GLMaterial::Color>::encode(value.emission);
	result+="; }";
	return result;
	}

GLMaterial ValueCoder<GLMaterial>::decode(const char* start,const char* end,const char** decodeEnd)
	{
	GLMaterial result;
	
	const char* cPtr=start;
	
	try
		{
		/* Check if the string starts with an opening brace: */
		if(*cPtr=='{')
			{
			/* It's the compound value notation of materials: */
			++cPtr;
			while(true)
				{
				cPtr=skipWhitespace(cPtr,end);
				if(*cPtr=='}')
					break;
				std::string tag=ValueCoder<std::string>::decode(cPtr,end,&cPtr);
				cPtr=skipSeparator('=',cPtr,end);
				if(tag=="Ambient")
					result.ambient=ValueCoder<GLMaterial::Color>::decode(cPtr,end,&cPtr);
				else if(tag=="Diffuse")
					result.diffuse=ValueCoder<GLMaterial::Color>::decode(cPtr,end,&cPtr);
				else if(tag=="AmbientDiffuse")
					result.ambient=result.diffuse=ValueCoder<GLMaterial::Color>::decode(cPtr,end,&cPtr);
				else if(tag=="Specular")
					result.specular=ValueCoder<GLMaterial::Color>::decode(cPtr,end,&cPtr);
				else if(tag=="Shininess")
					result.shininess=ValueCoder<GLfloat>::decode(cPtr,end,&cPtr);
				else if(tag=="Emission")
					result.emission=ValueCoder<GLMaterial::Color>::decode(cPtr,end,&cPtr);
				else
					throw DecodingError(std::string("Unknown tag ")+tag);
				cPtr=skipWhitespace(cPtr,end);
				if(*cPtr!=';')
					throw DecodingError(std::string("Missing semicolon after tag value ")+tag);
				++cPtr;
				}
			++cPtr;
			}
		else if(*cPtr=='(')
			{
			/* It's the old-style notation of materials: */
			++cPtr;
			cPtr=skipWhitespace(cPtr,end);
			result.ambient=ValueCoder<GLMaterial::Color>::decode(cPtr,end,&cPtr);
			cPtr=skipSeparator(',',cPtr,end);
			result.diffuse=ValueCoder<GLMaterial::Color>::decode(cPtr,end,&cPtr);
			cPtr=skipSeparator(',',cPtr,end);
			result.specular=ValueCoder<GLMaterial::Color>::decode(cPtr,end,&cPtr);
			cPtr=skipSeparator(',',cPtr,end);
			result.shininess=ValueCoder<GLfloat>::decode(cPtr,end,&cPtr);
			cPtr=skipWhitespace(cPtr,end);
			if(*cPtr!=')')
				throw DecodingError("Missing closing delimiter");
			++cPtr;
			}
		else
			throw DecodingError("Missing opening delimiter");
		}
	catch(std::runtime_error err)
		{
		throw DecodingError(std::string("Unable to convert \"")+std::string(start,end)+std::string("\" to GLMaterial due to ")+err.what());
		}
	
	/* Return result material: */
	if(decodeEnd!=0)
		*decodeEnd=cPtr;
	return result;
	}

/**********************************************
Force instantiation of all value coder classes:
**********************************************/

template class ValueCoder<GLColor<GLbyte,3> >;
template class ValueCoder<GLColor<GLubyte,3> >;
template class ValueCoder<GLColor<GLint,3> >;
template class ValueCoder<GLColor<GLuint,3> >;
template class ValueCoder<GLColor<GLfloat,3> >;
template class ValueCoder<GLColor<GLdouble,3> >;

template class ValueCoder<GLColor<GLbyte,4> >;
template class ValueCoder<GLColor<GLubyte,4> >;
template class ValueCoder<GLColor<GLint,4> >;
template class ValueCoder<GLColor<GLuint,4> >;
template class ValueCoder<GLColor<GLfloat,4> >;
template class ValueCoder<GLColor<GLdouble,4> >;

}
