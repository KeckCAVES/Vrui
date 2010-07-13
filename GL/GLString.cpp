/***********************************************************************
GLString - Class to represent strings with the additional data required
to render said strings using a texture-based font.
Copyright (c) 2010 Oliver Kreylos

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

#include <GL/GLString.h>

#include <string.h>
#include <GL/GLFont.h>

/*************************
Methods of class GLString:
*************************/

GLString::GLString(const char* sString,const GLFont& font)
	:string(new char[strlen(sString)+1])
	{
	/* Copy the source string: */
	strcpy(string,sString);
	
	/* Update the string's font-related data: */
	font.updateString(*this);
	}

GLString::GLString(const GLString& source)
	:string(new char[strlen(source.string)+1]),
	 texelWidth(source.texelWidth),textureWidth(source.textureWidth),
	 textureBox(source.textureBox)
	{
	/* Copy the source string: */
	strcpy(string,source.string);
	}

GLString& GLString::operator=(const GLString& source)
	{
	if(this!=&source)
		{
		/* Copy the source string: */
		delete[] string;
		string=new char[strlen(source.string)+1];
		strcpy(string,source.string);
		
		/* Copy the source's texture-related data: */
		texelWidth=source.texelWidth;
		textureWidth=source.textureWidth;
		textureBox=source.textureBox;
		}
	
	return *this;
	}

GLString::~GLString(void)
	{
	delete[] string;
	}

void GLString::setString(const char* newString,const GLFont& font)
	{
	/* Copy the new string: */
	delete[] string;
	string=new char[strlen(newString)+1];
	strcpy(string,newString);
	
	/* Update the string's font-related data: */
	font.updateString(*this);
	}

void GLString::adoptString(char* newString,const GLFont& font)
	{
	/* Adopt the new string: */
	delete[] string;
	string=newString;
	
	/* Update the string's font-related data: */
	font.updateString(*this);
	}

void GLString::setFont(const GLFont& font)
	{
	/* Update the string's font-related data: */
	font.updateString(*this);
	}
