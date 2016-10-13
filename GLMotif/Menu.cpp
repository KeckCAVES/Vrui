/***********************************************************************
Menu - Subclass of RowColumn that only contains buttons and acts as a
pop-up menu.
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

#include <GLMotif/Menu.h>

#include <stdio.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/Event.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/CascadeButton.h>

namespace GLMotif {

/*********************
Methods of class Menu:
*********************/

void Menu::childrenSelectCallbackWrapper(Misc::CallbackData* callbackData,void* userData)
	{
	/* Extract the widget pointers: */
	Button::SelectCallbackData* cbStruct=static_cast<Button::SelectCallbackData*>(callbackData);
	Menu* thisPtr=static_cast<Menu*>(userData);
	
	/* Call the entry select callbacks: */
	EntrySelectCallbackData cbData(thisPtr,cbStruct->button,cbStruct);
	thisPtr->entrySelectCallbacks.call(&cbData);
	}

Menu::Menu(const char* sName,Container* sParent,bool sManageChild)
	:RowColumn(sName,sParent,false),
	 armedWidget(0),newArmedWidget(0)
	{
	/* Set default layout for menus: */
	setMarginWidth(0.0f);
	
	/* Manage me: */
	if(sManageChild)
		manageChild();
	}

#if 0

bool Menu::findRecipient(Event& event)
	{
	newArmedWidget=armedWidget;
	
	/* Check if the currently armed widget wants the event: */
	if(armedWidget!=0&&armedWidget->findRecipient(event))
		return true;
	
	/* Check if any of the other child widgets want the event: */
	for(WidgetList::iterator chIt=children.begin();chIt!=children.end();++chIt)
		if(*chIt!=armedWidget&&(*chIt)->findRecipient(event))
			{
			newArmedWidget=dynamic_cast<Button*>(event.getTargetWidget());
			return true;
			}
	
	/* If no child was found, check if we ourselves want to ignore future events: */
	newArmedWidget=0;
	Event::WidgetPoint wp=event.calcWidgetPoint(this);
	if(isInside(wp.getPoint()))
		return event.setTargetWidget(this,wp);
	
	return false;
	}

void Menu::pointerButtonDown(Event& event)
	{
	/* Arm the event's target widget, if it's a button, by forwarding the button down event: */
	armedWidget=newArmedWidget;
	if(armedWidget!=0)
		armedWidget->pointerButtonDown(event);
	}

void Menu::pointerButtonUp(Event& event)
	{
	/* Disarm the armed widget by forwarding the button up event: */
	if(armedWidget!=0)
		armedWidget->pointerButtonUp(event);
	armedWidget=0;
	newArmedWidget=0;
	}

void Menu::pointerMotion(Event& event)
	{
	/* Check if this motion event changes the currently armed widget: */
	if(armedWidget!=newArmedWidget)
		{
		/* Don't disarm a cascade button if there is no new widget to arm: */
		if(newArmedWidget!=0||dynamic_cast<CascadeButton*>(armedWidget)==0)
			{
			/* Disarm the current armed widget by sending a fake button up event: */
			if(armedWidget!=0)
				armedWidget->pointerButtonUp(event);
			
			/* Arm the new widget: */
			armedWidget=newArmedWidget;
			
			/* Arm the new armed widget by sending a fake button down event: */
			if(armedWidget!=0)
				armedWidget->pointerButtonDown(event);
			}
		}
	else if(armedWidget!=0)
		armedWidget->pointerMotion(event);
	}

#endif

void Menu::addChild(Widget* newChild)
	{
	/* Get the style sheet: */
	const StyleSheet* ss=getStyleSheet();
	
	/* Set the child's border: */
	newChild->setBorderWidth(ss->menuButtonBorderWidth);
	
	/* If the child is a label: Left-align its text */
	Label* newLabel=dynamic_cast<Label*>(newChild);
	if(newLabel!=0)
		newLabel->setHAlignment(GLFont::Left);
	
	/* Call the parent class widget's routine: */
	RowColumn::addChild(newChild);
	
	/* If the child is a button: Set the child's select callback to point to us: */
	Button* newButton=dynamic_cast<Button*>(newChild);
	if(newButton!=0)
		newButton->getSelectCallbacks().add(childrenSelectCallbackWrapper,this);
	}

void Menu::addEntry(const char* newEntryLabel)
	{
	/* Create a new button: */
	char newButtonName[40];
	snprintf(newButtonName,sizeof(newButtonName),"_MenuButton%d",int(children.size()));
	new Button(newButtonName,this,newEntryLabel);
	}

int Menu::getEntryIndex(const Button* entry) const
	{
	int result=-1;
	int index=0;
	for(WidgetList::const_iterator chIt=children.begin();chIt!=children.end();++chIt,++index)
		if(*chIt==entry)
			{
			result=index;
			break;
			}
	
	return result;
	}

}
