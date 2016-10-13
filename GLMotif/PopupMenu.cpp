/***********************************************************************
PopupMenu - Class for top-level GLMotif UI components that act as menus
and only require a single down-motion-up sequence to select an entry.
Copyright (c) 2001-2016 Oliver Kreylos

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

#include <GLMotif/PopupMenu.h>

#include <stdio.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/Event.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/NewButton.h>
#include <GLMotif/CascadeButton.h>

namespace GLMotif {

/**************************
Methods of class PopupMenu:
**************************/

void PopupMenu::addMenuEntry(Widget* newEntry)
	{
	/* Re-parent the new entry to the root menu container: */
	newEntry->reparent(menu,false);
	
	/* Get the style sheet: */
	const StyleSheet* ss=getStyleSheet();
	
	/* Set the new entry's border: */
	newEntry->setBorderWidth(ss->menuButtonBorderWidth);
	
	/* If the new entry is derived from label: Left-align its text */
	Label* newLabel=dynamic_cast<Label*>(newEntry);
	if(newLabel!=0)
		newLabel->setHAlignment(GLFont::Left);
	
	/* Manage the new entry: */
	newEntry->manageChild();
	}

PopupMenu::PopupMenu(const char* sName,WidgetManager* sManager)
	:Popup(sName,sManager),
	 menu(0),
	 foundButton(0),armedButton(0),armedIsCascade(false)
	{
	}

Vector PopupMenu::calcHotSpot(void) const
	{
	Vector result=Widget::calcHotSpot();
	result[0]=getExterior().origin[0]+getExterior().size[0]-marginWidth*0.5f;
	return result;
	}

bool PopupMenu::findRecipient(Event& event)
	{
	foundButton=armedButton;
	
	/* Check if the currently armed button wants the event: */
	if(armedButton!=0&&armedButton->findRecipient(event))
		return true;
	
	/* Check if any of the other widgets inside the popup want the event: */
	if(child->findRecipient(event))
		{
		foundButton=dynamic_cast<Button*>(event.getTargetWidget());
		return true;
		}
	
	/* If no button was found, check if we ourselves want to ignore future events: */
	foundButton=0;
	Event::WidgetPoint wp=event.calcWidgetPoint(this);
	if(isInside(wp.getPoint()))
		return event.setTargetWidget(this,wp);
	
	return false;
	}

void PopupMenu::pointerButtonDown(Event& event)
	{
	/* Arm the event's target widget, if it's a button, by forwarding the button down event: */
	armedButton=foundButton;
	if(armedButton!=0)
		{
		armedIsCascade=dynamic_cast<CascadeButton*>(armedButton)!=0;
		armedButton->pointerButtonDown(event);
		}
	}

void PopupMenu::pointerButtonUp(Event& event)
	{
	/* Disarm the armed button by forwarding the button up event: */
	if(armedButton!=0)
		armedButton->pointerButtonUp(event);
	armedButton=0;
	foundButton=0; // Is this necessary?
	}

void PopupMenu::pointerMotion(Event& event)
	{
	/* Check if this motion event changes the currently armed button: */
	if(armedButton!=foundButton)
		{
		/* Don't disarm a cascade button if there is no new button to arm: */
		if(foundButton!=0||!armedIsCascade)
			{
			/* Disarm the current armed button by sending a fake button up event: */
			if(armedButton!=0)
				armedButton->pointerButtonUp(event);
			
			/* Remember the new button: */
			armedButton=foundButton;
			
			/* Arm the new armed button by sending a fake button down event: */
			if(armedButton!=0)
				{
				armedIsCascade=dynamic_cast<CascadeButton*>(armedButton)!=0;
				armedButton->pointerButtonDown(event);
				}
			}
		}
	else if(armedButton!=0)
		armedButton->pointerMotion(event);
	}

void PopupMenu::addChild(Widget* newChild)
	{
	/* Ignore the redundant addChild call coming from the title bar: */
	if(newChild==title)
		return;
	
	/* Check if a root menu container was already created or managed: */
	if(menu!=0)
		{
		/* Check if this is the root menu container managing itself after creation: */
		if(newChild!=menu)
			{
			/* Re-parent the new child to the root menu container: */
			addMenuEntry(newChild);
			}
		else
			{
			/* Manage the root menu container: */
			Popup::addChild(menu);
			}
		}
	else
		{
		/* Check if the new child is derived from RowColumn: */
		menu=dynamic_cast<RowColumn*>(newChild);
		if(menu==0)
			{
			/* It isn't. Create an unmanaged root menu shell: */
			createMenu();
			
			/* Re-parent the new child to the new root menu container: */
			addMenuEntry(newChild);
			}
		else
			{
			/* Set default layout for menus: */
			menu->setBorderWidth(0.0f);
			menu->setMarginWidth(0.0f);
			// menu->setSpacing(0.0f);
			
			/* Manage the root menu container: */
			Popup::addChild(menu);
			}
		}
	}

RowColumn* PopupMenu::createMenu(void)
	{
	/* Check if no menu container was created yet: */
	if(menu==0)
		{
		/* Create an empty menu container: */
		menu=new RowColumn("_Menu",this,false);
		
		/* Set default layout for menus: */
		menu->setBorderWidth(0.0f);
		menu->setMarginWidth(0.0f);
		// menu->setSpacing(0.0f);
		}
	
	return menu;
	}

void PopupMenu::manageMenu(void)
	{
	/* Create an empty menu container if none was created yet: */
	if(menu==0)
		createMenu();
	
	/* Let the regular mechanism handle it: */
	menu->manageChild();
	}

namespace {

/****************
Helper functions:
****************/

int countButtons(Container* container) // Recursively counts number of button-derived widgets inside a container
	{
	int result=0;
	
	/* Iterate through the container's children: */
	for(Widget* child=container->getFirstChild();child!=0;child=container->getNextChild(child))
		{
		if(dynamic_cast<Button*>(child)!=0||dynamic_cast<NewButton*>(child)!=0) // Time to replace Button with NewButton for good!
			++result;
		else
			{
			/* Check if the child is a sub-container: */
			Container* subContainer=dynamic_cast<Container*>(child);
			if(subContainer!=0)
				{
				/* Add the number of children inside the sub-container: */
				result+=countButtons(subContainer);
				}
			}
		}
	
	return result;
	}

int findButtonIndex(Container* container,Widget* button,int& index) // Recursively search for the given button inside the given container
	{
	int result=-1;
	
	/* Iterate through the container's children: */
	for(Widget* child=container->getFirstChild();child!=0&&result<0;child=container->getNextChild(child))
		{
		if(dynamic_cast<Button*>(child)!=0||dynamic_cast<NewButton*>(child)!=0) // Time to replace Button with NewButton for good!
			{
			if(child==button)
				result=index; // Found it!
			
			/* Only count button-derived widgets: */
			++index;
			}
		else
			{
			/* Check if the child is a sub-container: */
			Container* subContainer=dynamic_cast<Container*>(child);
			if(subContainer!=0)
				{
				/* Search inside the sub-container: */
				result=findButtonIndex(subContainer,button,index);
				}
			}
		}
	
	return result;
	}

bool removeButton(Container* container,Widget* button) // Recursively search for the given button inside the given container and remove it
	{
	/* Iterate through the container's children: */
	for(Widget* child=container->getFirstChild();child!=0;child=container->getNextChild(child))
		{
		if(child==button)
			{
			/* Remove the current child widget from its container: */
			container->removeChild(child);
			return true;
			}
		else
			{
			/* Check if the child is a sub-container: */
			Container* subContainer=dynamic_cast<Container*>(child);
			if(subContainer!=0)
				{
				/* Search inside the sub-container: */
				if(removeButton(subContainer,button))
					return true;
				}
			}
		}
	
	return false;
	}

void removeButton(Container* container,int buttonIndex,int& index) // Recursively search for the button of the given index inside the given container and remove it
	{
	/* Iterate through the container's children: */
	for(Widget* child=container->getFirstChild();child!=0&&index<=buttonIndex;child=container->getNextChild(child))
		{
		if(dynamic_cast<Button*>(child)!=0||dynamic_cast<NewButton*>(child)!=0) // Time to replace Button with NewButton for good!
			{
			if(index==buttonIndex)
				{
				/* Remove the current child widget from its container: */
				container->removeChild(child);
				}
			
			/* Only count button-derived widgets: */
			++index;
			}
		else
			{
			/* Check if the child is a sub-container: */
			Container* subContainer=dynamic_cast<Container*>(child);
			if(subContainer!=0)
				{
				/* Search inside the sub-container: */
				removeButton(subContainer,buttonIndex,index);
				}
			}
		}
	}

}

int PopupMenu::getNumEntries(void)
	{
	if(menu!=0)
		{
		/* Count button-derived widgets in all sub-containers recursively: */
		return countButtons(menu);
		}
	else
		return 0;
	}

Button* PopupMenu::addEntry(const char* newEntryLabel)
	{
	/* Create a new button of the given name: */
	char newButtonName[40];
	snprintf(newButtonName,sizeof(newButtonName),"_MenuButton%d",getNumEntries());
	return new Button(newButtonName,this,newEntryLabel);
	}

int PopupMenu::getEntryIndex(Widget* button)
	{
	if(menu!=0)
		{
		/* Find index of entry in all sub-containers recursively: */
		int index=0;
		return findButtonIndex(menu,button,index);
		}
	else
		return -1;
	}

void PopupMenu::removeEntry(Widget* entry)
	{
	if(menu!=0)
		{
		/* Find the entry of the given index in all sub-containers recursively and remove it: */
		removeButton(menu,entry);
		}
	}

void PopupMenu::removeEntry(int entryIndex)
	{
	if(menu!=0)
		{
		/* Find the entry of the given index in all sub-containers recursively and remove it: */
		int index=0;
		return removeButton(menu,entryIndex,index);
		}
	}

}
