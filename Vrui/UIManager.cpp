/***********************************************************************
UIManager - Base class for managers arranging user interface components,
mapping user interface devices and tools, and create user-aligned
displays in physical space.
Copyright (c) 2015-2016 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Vrui/UIManager.h>

#include <Vrui/Vrui.h>
#include <Vrui/GUIInteractor.h>

namespace Vrui {

/**************************
Methods of class UIManager:
**************************/

UIManager::UIManager(const Misc::ConfigurationFileSection& configFileSection)
	:activeGuiInteractor(0),
	 mostRecentGuiInteractor(0),
	 mostRecentHotSpot(getDisplayCenter()),mostRecentDirection(getForwardDirection())
	{
	}

bool UIManager::activateGuiInteractor(GUIInteractor* guiInteractor)
	{
	/* Check if the GUI interactor can be activated, or is already active: */
	if(activeGuiInteractor==0||activeGuiInteractor==guiInteractor)
		{
		/* Activate the GUI interactor: */
		activeGuiInteractor=guiInteractor;
		
		/* Remember it as the most recently active GUI interactor: */
		mostRecentGuiInteractor=guiInteractor;
		
		return true;
		}
	else
		return false;
	}

void UIManager::deactivateGuiInteractor(GUIInteractor* guiInteractor)
	{
	/* Check if the GUI interactor is active: */
	if(activeGuiInteractor==guiInteractor)
		{
		/* Reset the active GUI interactor: */
		activeGuiInteractor=0;
		}
	}

void UIManager::destroyGuiInteractor(GUIInteractor* guiInteractor)
	{
	/* If this GUI interactor is the current most recent one, reset it and remember its hotspot: */
	if(mostRecentGuiInteractor==guiInteractor)
		{
		mostRecentGuiInteractor=0;
		mostRecentHotSpot=guiInteractor->calcHotSpot();
		mostRecentDirection=guiInteractor->getRay().getDirection();
		}
	}

Point UIManager::getHotSpot(void) const
	{
	/* Check if there is a most-recently used GUI interactor: */
	if(mostRecentGuiInteractor!=0)
		{
		/* Return the GUI interactor's current hotspot: */
		return mostRecentGuiInteractor->calcHotSpot();
		}
	else
		{
		/* Return the most recent hotspot: */
		return mostRecentHotSpot;
		}
	}

Vector UIManager::getDirection(void) const
	{
	/* Check if there is a most-recently used GUI interactor: */
	if(mostRecentGuiInteractor!=0)
		{
		/* Return the GUI interactor's current interaction direction: */
		return mostRecentGuiInteractor->getRay().getDirection();
		}
	else
		{
		/* Return a zero vector to disable direction matching: */
		return Vector::zero;
		}
	}

}
