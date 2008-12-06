/***********************************************************************
GLExtensionManager - Class to manage OpenGL extensions.
Copyright (c) 2005-2006 Oliver Kreylos
Mac OS X adaptation copyright (c) 2006 Braden Pellett

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

#include <string.h>
#include <GL/gl.h>
#ifndef GLX_GLXEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES 1
#endif
#ifndef HAVE_GLXGETPROCADDRESS
#include <dlfcn.h>
#endif
#include <GL/glx.h>
#include <GL/Extensions/GLExtension.h>

#include <GL/GLExtensionManager.h>

#if 0
#ifdef HAVE_GLXGETPROCADDRESS
#ifndef GLX_ARB_get_proc_address
#define GLX_ARB_get_proc_address 1
typedef void (*__GLXextFuncPtr)(void);
extern __GLXextFuncPtr glXGetProcAddressARB(const GLubyte*);
#endif
#endif
#endif

/*******************************************
Static elements of class GLExtensionManager:
*******************************************/

GL_THREAD_LOCAL(GLExtensionManager*) GLExtensionManager::currentExtensionManager=0;

/***********************************
Methods of class GLExtensionManager:
***********************************/

GLExtensionManager::FunctionPointer GLExtensionManager::getFunctionPtr(const char* functionName)
	{
	#ifdef HAVE_GLXGETPROCADDRESS
	return glXGetProcAddressARB(reinterpret_cast<const GLubyte*>(functionName));
	#else
	/* Mac OS X's GLX does not support glXGetProcAddress, strangely enough: */
	FunctionPointer result;
	*reinterpret_cast<void**>(&result)=dlsym(RTLD_DEFAULT,functionName);
	return result;
	#endif
	}

GLExtensionManager::GLExtensionManager(void)
	:head(0),tail(0)
	{
	}

GLExtensionManager::~GLExtensionManager(void)
	{
	/* Destroy all registered extensions: */
	while(head!=0)
		{
		GLExtension* succ=head->succ;
		delete head;
		head=succ;
		}
	}

void GLExtensionManager::makeCurrent(GLExtensionManager* newCurrentExtensionManager)
	{
	if(newCurrentExtensionManager!=currentExtensionManager)
		{
		if(currentExtensionManager!=0)
			{
			/* Deactivate all extensions in the current extension manager: */
			for(GLExtension* extPtr=currentExtensionManager->head;extPtr!=0;extPtr=extPtr->succ)
				extPtr->deactivate();
			}
		
		/* Set the new current extension manager: */
		currentExtensionManager=newCurrentExtensionManager;
		
		if(currentExtensionManager!=0)
			{
			/* Activate all extensions in the current extension manager: */
			for(GLExtension* extPtr=currentExtensionManager->head;extPtr!=0;extPtr=extPtr->succ)
				extPtr->activate();
			}
		}
	}

bool GLExtensionManager::isExtensionSupported(const char* queryExtensionName)
	{
	ssize_t queryLen=strlen(queryExtensionName);
	
	/* Get the space-separated string of extension names: */
	const char* extensionNames=(const char*)glGetString(GL_EXTENSIONS);
	
	/* Compare each extension name against the query name: */
	const char* extBegin=extensionNames;
	while(*extBegin!='\0')
		{
		/* Find the next space or end-of-string character: */
		const char* extEnd;
		for(extEnd=extBegin;*extEnd!='\0'&&*extEnd!=' ';++extEnd)
			;
		
		/* Compare extension name against query name: */
		if(extEnd-extBegin==queryLen&&strncmp(extBegin,queryExtensionName,queryLen)==0)
			return true;
		
		/* Go to the next extension: */
		extBegin=extEnd;
		while(*extBegin==' ')
			++extBegin;
		}
	
	return false;
	}

bool GLExtensionManager::isExtensionRegistered(const char* extensionName)
	{
	/* Search the extension name in the list of registered extensions: */
	const GLExtension* extPtr;
	for(extPtr=currentExtensionManager->head;extPtr!=0;extPtr=extPtr->succ)
		if(strcmp(extPtr->getExtensionName(),extensionName)==0)
			break;
	
	/* Return true if the extension was found: */
	return extPtr!=0;
	}

void GLExtensionManager::registerExtension(GLExtension* newExtension)
	{
	/* Add the new extension to the end of the extension list: */
	if(currentExtensionManager->tail!=0)
		currentExtensionManager->tail->succ=newExtension;
	else
		currentExtensionManager->head=newExtension;
	currentExtensionManager->tail=newExtension;
	
	/* Activate the new extension: */
	newExtension->activate();
	}
