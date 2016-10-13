/***********************************************************************
GLPrintError - Helper function to print a plain-text OpenGL error
message.
Copyright (c) 2010-2016 Oliver Kreylos

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

#include <GL/GLPrintError.h>

#include <string>
#include <Misc/PrintInteger.h>
#include <Misc/MessageLogger.h>
#include <GL/gl.h>
#include <GL/Extensions/GLEXTFramebufferObject.h>

namespace {

/****************
Helper functions:
****************/

void glAppendErrorMsg(std::string& message,GLenum error)
	{
	switch(error)
		{
		case 0:
			message.append("Internal error in glGetError()");
			break;
		
		case GL_INVALID_ENUM:
			message.append("Invalid enum");
			break;
		
		case GL_INVALID_VALUE:
			message.append("Invalid value");
			break;
		
		case GL_INVALID_OPERATION:
			message.append("Invalid operation");
			break;
		
		case GL_STACK_OVERFLOW:
			message.append("Stack overflow");
			break;
		
		case GL_STACK_UNDERFLOW:
			message.append("Stack underflow");
			break;
		
		case GL_OUT_OF_MEMORY:
			message.append("Out of memory");
			break;
		
		case GL_TABLE_TOO_LARGE:
			message.append("Table too large");
			break;
		
		case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
			message.append("Invalid framebuffer operation");
			break;
		
		default:
			{
			message.append("Unknown error ");
			char buffer[10];
			message.append(Misc::print(error,buffer+sizeof(buffer)-1));
			}
		}
	}

}

void glPrintError(void)
	{
	GLenum error;
	while((error=glGetError())!=GL_NO_ERROR)
		{
		std::string message="glPrintError: ";
		glAppendErrorMsg(message,error);
		Misc::logError(message.c_str());
		}
	}

void glPrintError(const char* messageTag)
	{
	GLenum error;
	while((error=glGetError())!=GL_NO_ERROR)
		{
		std::string message="glPrintError: ";
		message.append(messageTag);
		message.push_back(' ');
		glAppendErrorMsg(message,error);
		Misc::logError(message.c_str());
		}
	}
