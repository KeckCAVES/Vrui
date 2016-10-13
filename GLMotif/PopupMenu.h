/***********************************************************************
PopupMenu - Class for top-level GLMotif UI components that act as menus
and only require a single down-motion-up sequence to select an entry.
Copyright (c) 2001-2015 Oliver Kreylos

This file is part of the GLMotif Widget Library (GLMotif).

The GLMotif Widget Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GLMotif Widget Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the GLMotif Widget Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef GLMOTIF_POPUPMENU_INCLUDED
#define GLMOTIF_POPUPMENU_INCLUDED

#include <GLMotif/Popup.h>

/* Forward declarations: */
namespace GLMotif {
class RowColumn;
class Button;
}

namespace GLMotif {

class PopupMenu:public Popup
	{
	/* Elements: */
	protected:
	RowColumn* menu; // Pointer to the RowColumn-derived child widget containing the root menu entries
	Button* foundButton; // Pointer to button found during most recent findRecipient call
	Button* armedButton; // Pointer to currently armed button in menu, or 0
	bool armedIsCascade; // Flag whether the currently armed button is a cascade button
	
	/* Protected methods: */
	void addMenuEntry(Widget* newEntry); // Adds a new entry to the root menu shell; used during addChild processing
	
	/* Constructors and destructors: */
	public:
	PopupMenu(const char* sName,WidgetManager* sManager);
	
	/* Methods inherited from Widget: */
	virtual Vector calcHotSpot(void) const;
	virtual bool findRecipient(Event& event);
	virtual void pointerButtonDown(Event& event);
	virtual void pointerButtonUp(Event& event);
	virtual void pointerMotion(Event& event);
	
	/* Methods inherited from Container: */
	virtual void addChild(Widget* newChild);
	
	/* New methods: */
	RowColumn* getMenu(void) // Returns the root menu container
		{
		return menu;
		}
	RowColumn* createMenu(void); // Creates an unmanaged root menu container if none exists yet
	void manageMenu(void); // Shortcut to manage the root menu shell after it has been fully constructed
	int getNumEntries(void); // Returns the total number of buttons in the menu, including sub-containers
	Button* addEntry(const char* newEntryLabel); // Adds a new simple menu entry and returns a pointer to the created button
	int getEntryIndex(Widget* entry); // Returns the index of the given menu entry, assumed to be a button, counting through sub-containers
	void removeEntry(Widget* entry); // Removes the first instance of the given entry from the menu
	void removeEntry(int entryIndex); // Removes the entry of the given index from the menu
	};

}

#endif
