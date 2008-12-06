/***********************************************************************
GLThingManager - Class manage initialization and destruction of OpenGL-
related state in cooperation with GLContextData objects.
Copyright (c) 2006 Oliver Kreylos

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

#include <GL/GLObject.h>
#include <GL/GLContextData.h>

#include <GL/GLThingManager.h>

/***************************************
Static elements of class GLThingManager:
***************************************/

GLThingManager GLThingManager::theThingManager;

/*******************************
Methods of class GLThingManager:
*******************************/

GLThingManager::GLThingManager(void)
	:active(true),
	 firstNewAction(0),lastNewAction(0),
	 firstProcessAction(0)
	{
	}

GLThingManager::~GLThingManager(void)
	{
	/* Delete all actions: */
	while(firstProcessAction!=0)
		{
		ThingAction* succ=firstProcessAction->succ;
		delete firstProcessAction;
		firstProcessAction=succ;
		}
	{
	Threads::Mutex::Lock newActionLock(newActionMutex);
	theThingManager.active=false;
	while(firstNewAction!=0)
		{
		ThingAction* succ=firstNewAction->succ;
		delete firstNewAction;
		firstNewAction=succ;
		}
	}
	}

void GLThingManager::shutdown(void)
	{
	/* Delete all pending actions: */
	while(theThingManager.firstProcessAction!=0)
		{
		ThingAction* succ=theThingManager.firstProcessAction->succ;
		delete theThingManager.firstProcessAction;
		theThingManager.firstProcessAction=succ;
		}
	
	/* Mark the thing manager as inactive: */
	{
	Threads::Mutex::Lock newActionLock(theThingManager.newActionMutex);
	theThingManager.active=false;
	while(theThingManager.firstNewAction!=0)
		{
		ThingAction* succ=theThingManager.firstNewAction->succ;
		delete theThingManager.firstNewAction;
		theThingManager.firstNewAction=succ;
		}
	}
	}

void GLThingManager::initThing(const GLObject* thing)
	{
	{
	Threads::Mutex::Lock newActionLock(newActionMutex);
	if(active)
		{
		/* Append the new thing action to the new action list: */
		ThingAction* newAction=new ThingAction;
		newAction->thing=thing;
		newAction->action=ThingAction::INIT;
		newAction->succ=0;
		if(lastNewAction!=0)
			lastNewAction->succ=newAction;
		else
			firstNewAction=newAction;
		lastNewAction=newAction;
		}
	}
	}

void GLThingManager::destroyThing(const GLObject* thing)
	{
	Threads::Mutex::Lock newActionLock(newActionMutex);
	if(active)
		{
		/* Search if the thing has a pending initialization action: */
		ThingAction* taPtr1=0;
		ThingAction* taPtr2;
		for(taPtr2=firstNewAction;taPtr2!=0;taPtr1=taPtr2,taPtr2=taPtr2->succ)
			if(taPtr2->thing==thing&&taPtr2->action==ThingAction::INIT)
				break;
		
		if(taPtr2!=0)
			{
			/* Thing has pending initialization; remove it: */
			if(taPtr1!=0)
				taPtr1->succ=taPtr2->succ;
			else
				firstNewAction=taPtr2->succ;
			if(taPtr2->succ==0)
				lastNewAction=taPtr1;
			delete taPtr2;
			}
		else
			{
			/* Append a destruction action to the list: */
			ThingAction* newAction=new ThingAction;
			newAction->thing=thing;
			newAction->action=ThingAction::DESTROY;
			newAction->succ=0;
			if(lastNewAction!=0)
				lastNewAction->succ=newAction;
			else
				firstNewAction=newAction;
			lastNewAction=newAction;
			}
		}
	}

void GLThingManager::processActions(void)
	{
	/* Delete the old process list: */
	while(firstProcessAction!=0)
		{
		ThingAction* succ=firstProcessAction->succ;
		delete firstProcessAction;
		firstProcessAction=succ;
		}
	
	/* Move the new action list to the process list: */
	{
	Threads::Mutex::Lock newActionLock(newActionMutex);
	firstProcessAction=firstNewAction;
	firstNewAction=0;
	lastNewAction=0;
	}
	}

void GLThingManager::updateThings(GLContextData& contextData) const
	{
	/* Perform all actions on the process list in order: */
	for(const ThingAction* taPtr=firstProcessAction;taPtr!=0;taPtr=taPtr->succ)
		{
		if(taPtr->action==ThingAction::INIT)
			{
			/* Call the thing's context initialization routine: */
			taPtr->thing->initContext(contextData);
			}
		else
			{
			/* Delete the context data item associated with the thing: */
			contextData.removeDataItem(taPtr->thing);
			}
		}
	}
