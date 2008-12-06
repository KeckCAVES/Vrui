/***********************************************************************
ALContextData - Class to store per-AL-context data for application
objects.
Copyright (c) 2006-2008 Oliver Kreylos

This file is part of the OpenAL Support Library (ALSupport).

The OpenAL Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenAL Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenAL Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <AL/ALThingManager.h>

#include <AL/ALContextData.h>

/**************************************
Static elements of class ALContextData:
**************************************/

Misc::CallbackList ALContextData::currentContextDataChangedCallbacks;
ALContextData* ALContextData::currentContextData=0;

/******************************
Methods of class ALContextData:
******************************/

ALContextData::ALContextData(int sTableSize,float sWaterMark,float sGrowRate)
	:context(sTableSize,sWaterMark,sGrowRate)
	{
	}

ALContextData::~ALContextData(void)
	{
	/* Delete all data items in this context: */
	for(ItemHash::Iterator it=context.begin();!it.isFinished();++it)
		delete it->getDest();
	}

void ALContextData::initThing(const ALObject* thing)
	{
	ALThingManager::theThingManager.initThing(thing);
	}

void ALContextData::destroyThing(const ALObject* thing)
	{
	ALThingManager::theThingManager.destroyThing(thing);
	}

void ALContextData::resetThingManager(void)
	{
	ALThingManager::theThingManager.processActions();
	}

void ALContextData::updateThings(void)
	{
	ALThingManager::theThingManager.updateThings(*this);
	}

void ALContextData::makeCurrent(ALContextData* newCurrentContextData)
	{
	if(newCurrentContextData!=currentContextData)
		{
		/* Create the callback data object: */
		CurrentContextDataChangedCallbackData cbData(currentContextData,newCurrentContextData);
		
		/* Set the current context data object: */
		currentContextData=newCurrentContextData;
		
		/* Call all callbacks: */
		currentContextDataChangedCallbacks.call(&cbData);
		}
	}
