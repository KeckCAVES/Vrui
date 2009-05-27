/***********************************************************************
ListBox - Class for widgets containing lists of text strings.
Copyright (c) 2008 Oliver Kreylos

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

#include <math.h>
#include <string.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLTexCoordTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLTexEnvTemplates.h>
#include <GL/GLContextData.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/Event.h>
#include <GLMotif/Container.h>

#include <GLMotif/ListBox.h>

namespace GLMotif {

/**********************************
Methods of class ListBox::DataItem:
**********************************/

ListBox::DataItem::DataItem(void)
	:version(0),pageSize(0),itemTextureIds(0)
	{
	}

ListBox::DataItem::~DataItem(void)
	{
	if(itemTextureIds!=0)
		{
		glDeleteTextures(pageSize,itemTextureIds);
		delete[] itemTextureIds;
		}
	}

/************************
Methods of class ListBox:
************************/

void ListBox::calcMaxVisibleItemWidth(void)
	{
	maxVisibleItemWidth=0.0f;
	for(int i=position;i<position+pageSize&&i<int(items.size());++i)
		if(maxVisibleItemWidth<items[i].width)
			maxVisibleItemWidth=items[i].width;
	}

void ListBox::updatePageSlots(void)
	{
	/* Update the page slot states: */
	for(int i=0;i<pageSize;++i)
		{
		pageSlots[i].slotBox=itemsBox;
		pageSlots[i].slotBox.origin[1]+=itemsBox.size[1]-font->getTextHeight()-GLfloat(i)*(font->getTextHeight()+itemSep);
		pageSlots[i].slotBox.size[1]=font->getTextHeight();
		if(position+i<int(items.size()))
			{
			pageSlots[i].item=items[position+i].item;
			pageSlots[i].textWidth=items[position+i].width;
			pageSlots[i].textTexCoords=font->calcStringTexCoords(pageSlots[i].item);
			if(horizontalOffset>0.0f)
				{
				/* Take the horizontal offset into account: */
				if(pageSlots[i].textWidth>horizontalOffset)
					{
					/* Adjust the text width and texture coordinates to clip strings on the left: */
					GLfloat newTextureWidth=(pageSlots[i].textWidth-horizontalOffset)*pageSlots[i].textTexCoords.size[0]/pageSlots[i].textWidth;
					pageSlots[i].textTexCoords.origin[0]+=pageSlots[i].textTexCoords.size[0]-newTextureWidth;
					pageSlots[i].textTexCoords.size[0]=newTextureWidth;
					pageSlots[i].textWidth-=horizontalOffset;
					}
				else
					pageSlots[i].textWidth=0.0f;
				}
			if(pageSlots[i].textWidth>pageSlots[i].slotBox.size[0])
				{
				/* Adjust the text width and texture coordinates to clip strings on the right: */
				pageSlots[i].textTexCoords.size[0]=pageSlots[i].slotBox.size[0]*pageSlots[i].textTexCoords.size[0]/pageSlots[i].textWidth;
				pageSlots[i].textWidth=pageSlots[i].slotBox.size[0];
				}
			}
		else
			{
			pageSlots[i].item=0;
			pageSlots[i].textWidth=0.0f;
			}
		pageSlots[i].textEnd[0]=pageSlots[i].slotBox.getCorner(0);
		pageSlots[i].textEnd[0][0]+=pageSlots[i].textWidth;
		pageSlots[i].textEnd[1]=pageSlots[i].slotBox.getCorner(2);
		pageSlots[i].textEnd[1][0]+=pageSlots[i].textWidth;
		}
	
	/* Update the page slots version number: */
	++version;
	}

ListBox::ListBox(const char* sName,Container* sParent,int sPreferredWidth,int sPreferredPageSize,bool sManageChild)
	:Widget(sName,sParent,false),
	 marginWidth(0.0f),itemSep(0.0f),
	 font(0),
	 preferredWidth(sPreferredWidth),preferredPageSize(sPreferredPageSize),
	 selectionMode(ALWAYS_ONE),
	 autoResize(false),
	 itemsBox(Vector(0.0f,0.0f,0.0f),Vector(0.0f,0.0f,0.0f)),
	 maxItemWidth(0.0f),
	 pageSize(0),pageSlots(0),
	 position(0),
	 maxVisibleItemWidth(0.0f),
	 horizontalOffset(0.0f),
	 selectedItem(-1),
	 version(0),lastClickTime(0.0)
	{
	/* Get the style sheet: */
	const StyleSheet* ss=getStyleSheet();
	
	/* Get the font: */
	font=ss->font;
	
	setBorderWidth(ss->textfieldBorderWidth);
	setBorderType(Widget::LOWERED);
	setBackgroundColor(ss->textfieldBgColor);
	setForegroundColor(ss->textfieldFgColor);
	marginWidth=ss->textfieldMarginWidth;
	
	/* Manage me: */
	if(sManageChild)
		manageChild();
	}

ListBox::~ListBox(void)
	{
	/* Delete the page slots: */
	delete[] pageSlots;
	
	/* Delete the list items: */
	for(std::vector<Item>::iterator iIt=items.begin();iIt!=items.end();++iIt)
		delete[] iIt->item;
	}

Vector ListBox::calcNaturalSize(void) const
	{
	/* Calculate the list box's preferred size: */
	Vector result;
	result[0]=GLfloat(preferredWidth)*font->getCharacterWidth();
	if(autoResize&&result[0]<maxItemWidth)
		result[0]=maxItemWidth;
	result[0]+=2.0f*marginWidth;
	result[1]=GLfloat(preferredPageSize)*(font->getTextHeight()+itemSep)-itemSep+2.0f*marginWidth;
	result[2]=0.0f;
	
	return calcExteriorSize(result);
	}

void ListBox::resize(const Box& newExterior)
	{
	/* Resize the parent class widget: */
	Widget::resize(newExterior);
	
	/* Keep track of changing page parameters: */
	int changeMask=0x0;
	
	/* Resize the item box: */
	GLfloat oldWidth=itemsBox.size[0];
	itemsBox=getInterior();
	itemsBox.doInset(Vector(marginWidth,marginWidth,0.0f));
	
	/* Calculate and adapt to the new page size: */
	int newPageSize=int(floor((itemsBox.size[1]+itemSep)/(font->getTextHeight()+itemSep)));
	if(newPageSize<0)
		newPageSize=0;
	GLfloat oldMaxVisibleItemWidth=maxVisibleItemWidth;
	if(newPageSize!=pageSize)
		{
		/* Update the current page size and list position: */
		delete[] pageSlots;
		pageSize=newPageSize;
		pageSlots=new ListBoxSlot[pageSize];
		changeMask|=PageChangedCallbackData::PAGESIZE_CHANGED;
		
		/* Limit the position to the valid range: */
		if(position>0&&position>int(items.size())-pageSize)
			{
			position=int(items.size())-pageSize;
			changeMask|=PageChangedCallbackData::POSITION_CHANGED;
			}
		if(position<0)
			position=0;
		
		/* Calculate the maximum item width: */
		calcMaxVisibleItemWidth();
		}
	
	/* Adapt to the new list width: */
	if(maxVisibleItemWidth!=oldMaxVisibleItemWidth)
		changeMask|=PageChangedCallbackData::MAXITEMWIDTH_CHANGED;
	if(oldWidth!=itemsBox.size[0])
		{
		changeMask|=PageChangedCallbackData::LISTWIDTH_CHANGED;
		
		/* Limit the horizontal offset to the valid range: */
		if(horizontalOffset>0.0f&&horizontalOffset>maxVisibleItemWidth-itemsBox.size[0])
			{
			horizontalOffset=maxVisibleItemWidth-itemsBox.size[0];
			changeMask|=PageChangedCallbackData::HORIZONTALOFFSET_CHANGED;
			}
		if(horizontalOffset<0.0f)
			horizontalOffset=0.0f;
		}
	
	/* Update the currently visible items: */
	updatePageSlots();
	
	if(changeMask!=0x0)
		{
		/* Send a page change callback: */
		PageChangedCallbackData cbData(this,changeMask,position,int(items.size()),pageSize,horizontalOffset,maxItemWidth,itemsBox.size[0]);
		pageChangedCallbacks.call(&cbData);
		}
	}

void ListBox::draw(GLContextData& contextData) const
	{
	/* Draw the parent class widget: */
	Widget::draw(contextData);
	
	/* Draw the margin around the list items: */
	glColor(backgroundColor);
	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(0.0f,0.0f,1.0f);
	glVertex(getInterior().getCorner(1));
	glVertex(getInterior().getCorner(3));
	for(int i=0;i<pageSize;++i)
		{
		glVertex(pageSlots[i].slotBox.getCorner(3));
		glVertex(pageSlots[i].slotBox.getCorner(1));
		}
	glVertex(itemsBox.getCorner(1));
	glVertex(itemsBox.getCorner(0));
	glVertex(getInterior().getCorner(0));
	glEnd();
	glBegin(GL_TRIANGLE_FAN);
	glVertex(getInterior().getCorner(2));
	glVertex(getInterior().getCorner(0));
	glVertex(itemsBox.getCorner(0));
	for(int i=pageSize-1;i>=0;--i)
		{
		glVertex(pageSlots[i].slotBox.getCorner(0));
		glVertex(pageSlots[i].slotBox.getCorner(2));
		}
	glVertex(pageSlots[0].textEnd[1]);
	glVertex(pageSlots[0].slotBox.getCorner(3));
	glVertex(getInterior().getCorner(3));
	glEnd();
	
	/* Draw the list item separators: */
	glBegin(GL_QUAD_STRIP);
	for(int i=0;i<pageSize;++i)
		{
		glVertex(pageSlots[i].slotBox.getCorner(3));
		glVertex(pageSlots[i].textEnd[1]);
		if(position+i==selectedItem)
			{
			glColor3f(0.5f,0.5f,0.5f);
			glVertex(pageSlots[i].slotBox.getCorner(3));
			glVertex(pageSlots[i].textEnd[1]);
			glVertex(pageSlots[i].slotBox.getCorner(1));
			glVertex(pageSlots[i].textEnd[0]);
			glColor(backgroundColor);
			}
		glVertex(pageSlots[i].slotBox.getCorner(1));
		glVertex(pageSlots[i].textEnd[0]);
		}
	glVertex(itemsBox.getCorner(1));
	glVertex(itemsBox.getCorner(0));
	glEnd();
	glBegin(GL_QUADS);
	for(int i=0;i<pageSize-1;++i)
		{
		glVertex(pageSlots[i].textEnd[0]);
		glVertex(pageSlots[i].slotBox.getCorner(0));
		glVertex(pageSlots[i+1].slotBox.getCorner(2));
		glVertex(pageSlots[i+1].textEnd[1]);
		}
	glEnd();
	glBegin(GL_TRIANGLES);
	glVertex(pageSlots[pageSize-1].textEnd[0]);
	glVertex(pageSlots[pageSize-1].slotBox.getCorner(0));
	glVertex(itemsBox.getCorner(0));
	glEnd();
	
	/* Retrieve the data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	if(dataItem->version!=version)
		{
		/* Update the data item: */
		if(dataItem->pageSize!=pageSize)
			{
			if(dataItem->itemTextureIds!=0)
				{
				glDeleteTextures(dataItem->pageSize,dataItem->itemTextureIds);
				delete[] dataItem->itemTextureIds;
				}
			dataItem->pageSize=pageSize;
			dataItem->itemTextureIds=new GLuint[dataItem->pageSize];
			glGenTextures(dataItem->pageSize,dataItem->itemTextureIds);
			}
		}
	
	/* Draw the list items: */
	glPushAttrib(GL_TEXTURE_BIT);
	GLint lightModelColorControl;
	glGetIntegerv(GL_LIGHT_MODEL_COLOR_CONTROL,&lightModelColorControl);
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
	glEnable(GL_TEXTURE_2D);
	glTexEnvMode(GLTexEnvEnums::TEXTURE_ENV,GLTexEnvEnums::MODULATE);
	glColor4f(1.0f,1.0f,1.0f,backgroundColor[3]);
	for(int i=0;i<pageSize&&position+i<int(items.size());++i)
		{
		glBindTexture(GL_TEXTURE_2D,dataItem->itemTextureIds[i]);
		if(dataItem->version!=version)
			{
			/* Upload the item string texture again: */
			if(position+i==selectedItem)
				font->uploadStringTexture(pageSlots[i].item,Color(0.5f,0.5f,0.5f),Color(1.0f,1.0f,1.0f));
			else
				font->uploadStringTexture(pageSlots[i].item,backgroundColor,foregroundColor);
			}
		glBegin(GL_QUADS);
		glTexCoord(pageSlots[i].textTexCoords.getCorner(0));
		glVertex(pageSlots[i].slotBox.getCorner(0));
		glTexCoord(pageSlots[i].textTexCoords.getCorner(1));
		glVertex(pageSlots[i].textEnd[0]);
		glTexCoord(pageSlots[i].textTexCoords.getCorner(3));
		glVertex(pageSlots[i].textEnd[1]);
		glTexCoord(pageSlots[i].textTexCoords.getCorner(2));
		glVertex(pageSlots[i].slotBox.getCorner(2));
		glEnd();
		}
	glBindTexture(GL_TEXTURE_2D,0);
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,lightModelColorControl);
	glPopAttrib();
	dataItem->version=version;
	}

void ListBox::pointerButtonDown(Event& event)
	{
	/* Determine which page slot was clicked on: */
	Point p=event.getWidgetPoint().getPoint();
	for(int i=0;i<pageSize&&position+i<int(items.size());++i)
		{
		const Box& b=pageSlots[i].slotBox;
		if(p[0]>=b.origin[0]&&p[0]<b.origin[0]+b.size[0]&&p[1]>=b.origin[1]&&p[1]<b.origin[1]+b.size[1])
			{
			int oldSelectedItem=selectedItem;
			
			/* Select or deselect the clicked list item: */
			if(selectedItem!=position+i)
				{
				/* Select the list item: */
				selectItem(position+i);
				
				/* Call the value changed callbacks: */
				ValueChangedCallbackData cbData(this,oldSelectedItem,selectedItem);
				valueChangedCallbacks.call(&cbData);
				
				/* Reset the click counter: */
				numClicks=1;
				}
			else
				{
				/* Check for a double click: */
				if(getManager()->getTime()-lastClickTime<0.25)
					{
					/* Increase the click counter: */
					++numClicks;
					}
				else if(selectionMode==ATMOST_ONE)
					{
					/* Deselect the list item: */
					selectItem(-1);
					
					/* Call the value changed callbacks: */
					ValueChangedCallbackData cbData(this,oldSelectedItem,selectedItem);
					valueChangedCallbacks.call(&cbData);
					}
				}
			
			/* Remember the click time: */
			lastClickTime=getManager()->getTime();
			break;
			}
		}
	}

void ListBox::pointerButtonUp(Event& event)
	{
	if(numClicks>=2)
		{
		/* Call the item selection callbacks: */
		ItemSelectedCallbackData cbData(this,selectedItem);
		itemSelectedCallbacks.call(&cbData);
		}
	}

void ListBox::pointerMotion(Event& event)
	{
	}

void ListBox::initContext(GLContextData& contextData) const
	{
	/* Create a data item and store it with the OpenGL context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

void ListBox::setMarginWidth(GLfloat newMarginWidth)
	{
	/* Set the margin width: */
	marginWidth=newMarginWidth;
	
	if(isManaged)
		{
		/* Try to resize the widget to accomodate the new setting: */
		parent->requestResize(this,calcNaturalSize());
		}
	}

void ListBox::setItemSeparation(GLfloat newItemSep)
	{
	/* Set the item separation: */
	itemSep=newItemSep;
	
	if(isManaged)
		{
		/* Try to resize the widget to accomodate the new setting: */
		parent->requestResize(this,calcNaturalSize());
		}
	}

void ListBox::setSelectionMode(ListBox::SelectionMode newSelectionMode)
	{
	/* Set the new selection mode: */
	selectionMode=newSelectionMode;
	}

void ListBox::setAutoResize(bool newAutoResize)
	{
	/* Set the autoresize flag: */
	autoResize=newAutoResize;
	
	if(autoResize&&maxItemWidth>itemsBox.size[0])
		{
		/* Resize the list box to accomodate the largest item: */
		if(isManaged)
			parent->requestResize(this,calcNaturalSize());
		else
			resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
		}
	}

void ListBox::setPosition(int newPosition)
	{
	/* Limit the new position to the valid range: */
	if(newPosition>int(items.size())-pageSize)
		newPosition=int(items.size())-pageSize;
	if(newPosition<0)
		newPosition=0;
	
	if(newPosition!=position)
		{
		/* Set the list position: */
		position=newPosition;
		int reasonMask=PageChangedCallbackData::POSITION_CHANGED;
		
		/* Update the visible list items: */
		GLfloat oldMaxVisibleItemWidth=maxVisibleItemWidth;
		calcMaxVisibleItemWidth();
		
		/* Limit the horizontal offset to the valid range: */
		if(horizontalOffset>0.0f&&horizontalOffset>maxVisibleItemWidth-itemsBox.size[0])
			{
			horizontalOffset=maxVisibleItemWidth-itemsBox.size[0];
			reasonMask|=PageChangedCallbackData::HORIZONTALOFFSET_CHANGED;
			}
		if(horizontalOffset<0.0f)
			horizontalOffset=0.0f;
		
		/* Update the visible page slots: */
		updatePageSlots();
		
		/* Call the page change callbacks: */
		if(oldMaxVisibleItemWidth!=maxVisibleItemWidth)
			reasonMask|=PageChangedCallbackData::MAXITEMWIDTH_CHANGED;
		PageChangedCallbackData cbData(this,reasonMask,position,int(items.size()),pageSize,horizontalOffset,maxVisibleItemWidth,itemsBox.size[0]);
		pageChangedCallbacks.call(&cbData);
		}
	}

void ListBox::setHorizontalOffset(GLfloat newHorizontalOffset)
	{
	/* Set the horizontal offset: */
	if(newHorizontalOffset>maxVisibleItemWidth-itemsBox.size[0])
		newHorizontalOffset=maxVisibleItemWidth-itemsBox.size[0];
	if(newHorizontalOffset<0.0f)
		newHorizontalOffset=0.0f;
	
	if(newHorizontalOffset!=horizontalOffset)
		{
		horizontalOffset=newHorizontalOffset;
		
		/* Update the visible list items: */
		updatePageSlots();
		
		/* Call the page change callbacks: */
		PageChangedCallbackData cbData(this,PageChangedCallbackData::HORIZONTALOFFSET_CHANGED,position,int(items.size()),pageSize,horizontalOffset,maxVisibleItemWidth,itemsBox.size[0]);
		pageChangedCallbacks.call(&cbData);
		}
	}

void ListBox::clear(void)
	{
	/* Do nothing if the list is already empty: */
	if(items.empty())
		return;
	
	/* Clear the list: */
	int reasonMask=PageChangedCallbackData::NUMITEMS_CHANGED;
	for(std::vector<Item>::iterator iIt=items.begin();iIt!=items.end();++iIt)
		delete[] iIt->item;
	items.clear();
	
	/* Reset all ancillary data: */
	GLfloat oldItemWidth=maxItemWidth;
	maxItemWidth=0.0f;
	if(position!=0)
		reasonMask|=PageChangedCallbackData::POSITION_CHANGED;
	position=0;
	if(maxVisibleItemWidth!=0.0f)
		reasonMask|=PageChangedCallbackData::MAXITEMWIDTH_CHANGED;
	maxVisibleItemWidth=0.0f;
	if(horizontalOffset!=0.0f)
		reasonMask|=PageChangedCallbackData::HORIZONTALOFFSET_CHANGED;
	horizontalOffset=0.0f;
	selectedItem=-1;
	
	/* Update the displayed page: */
	updatePageSlots();
	
	{
	/* Call the list changed callbacks: */
	ListChangedCallbackData cbData(this,ListChangedCallbackData::LIST_CLEARED,0);
	listChangedCallbacks.call(&cbData);
	}
	
	{
	/* Call the page change callbacks: */
	PageChangedCallbackData cbData(this,reasonMask,position,int(items.size()),pageSize,horizontalOffset,maxVisibleItemWidth,itemsBox.size[0]);
	pageChangedCallbacks.call(&cbData);
	}
	
	if(autoResize&&itemsBox.size[0]==oldItemWidth)
		{
		if(isManaged)
			parent->requestResize(this,calcNaturalSize());
		else
			resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
		}
	}

void ListBox::addItem(const char* newItem)
	{
	/* Add the new item to the list: */
	Item it;
	it.item=new char[strlen(newItem)+1];
	strcpy(it.item,newItem);
	it.width=font->calcStringBox(newItem).size[0];
	items.push_back(it);
	int reasonMask=PageChangedCallbackData::NUMITEMS_CHANGED;
	
	if(position+pageSize>=int(items.size()))
		{
		/* Update the visible list items: */
		GLfloat oldMaxVisibleItemWidth=maxVisibleItemWidth;
		calcMaxVisibleItemWidth();
		
		/* Adjust the horizontal offset: */
		if(horizontalOffset>0.0f&&horizontalOffset>maxVisibleItemWidth-itemsBox.size[0])
			{
			horizontalOffset=maxVisibleItemWidth-itemsBox.size[0];
			reasonMask|=PageChangedCallbackData::HORIZONTALOFFSET_CHANGED;
			}
		if(horizontalOffset<0.0f)
			horizontalOffset=0.0f;
		
		updatePageSlots();
		
		if(oldMaxVisibleItemWidth!=maxVisibleItemWidth)
			reasonMask|=PageChangedCallbackData::MAXITEMWIDTH_CHANGED;
		}
	
	{
	/* Call the list changed callbacks: */
	ListChangedCallbackData cbData(this,ListChangedCallbackData::ITEM_INSERTED,items.size()-1);
	listChangedCallbacks.call(&cbData);
	}
	
	{
	/* Call the page change callbacks: */
	PageChangedCallbackData cbData(this,reasonMask,position,int(items.size()),pageSize,horizontalOffset,maxVisibleItemWidth,itemsBox.size[0]);
	pageChangedCallbacks.call(&cbData);
	}
	
	if(maxItemWidth<it.width)
		{
		maxItemWidth=it.width;
		if(autoResize&&maxItemWidth>itemsBox.size[0])
			{
			if(isManaged)
				parent->requestResize(this,calcNaturalSize());
			else
				resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
			}
		}
	}

void ListBox::insertItem(int index,const char* newItem)
	{
	/* Add the new item to the list: */
	Item it;
	it.item=new char[strlen(newItem)+1];
	strcpy(it.item,newItem);
	it.width=font->calcStringBox(newItem).size[0];
	items.insert(items.begin()+index,it);
	int reasonMask=PageChangedCallbackData::NUMITEMS_CHANGED;
	
	if(index<position)
		{
		/* Adjust the position by one so that the displayed items don't change: */
		++position;
		reasonMask|=PageChangedCallbackData::POSITION_CHANGED;
		}
	else if(index<position+pageSize)
		{
		/* Update the visible list items: */
		GLfloat oldMaxVisibleItemWidth=maxVisibleItemWidth;
		calcMaxVisibleItemWidth();
		
		/* Adjust the horizontal offset: */
		if(horizontalOffset>0.0f&&horizontalOffset>maxVisibleItemWidth-itemsBox.size[0])
			{
			horizontalOffset=maxVisibleItemWidth-itemsBox.size[0];
			reasonMask|=PageChangedCallbackData::HORIZONTALOFFSET_CHANGED;
			}
		if(horizontalOffset<0.0f)
			horizontalOffset=0.0f;
		
		updatePageSlots();
		
		if(oldMaxVisibleItemWidth!=maxVisibleItemWidth)
			reasonMask|=PageChangedCallbackData::MAXITEMWIDTH_CHANGED;
		}
	
	/* Update the selected item if it is affected: */
	if(selectedItem>=index)
		{
		/* Adjust the selected item's index: */
		selectItem(selectedItem+1);
		}
	
	{
	/* Call the list changed callbacks: */
	ListChangedCallbackData cbData(this,ListChangedCallbackData::ITEM_INSERTED,index);
	listChangedCallbacks.call(&cbData);
	}
	
	{
	/* Call the page change callbacks: */
	PageChangedCallbackData cbData(this,reasonMask,position,int(items.size()),pageSize,horizontalOffset,maxVisibleItemWidth,itemsBox.size[0]);
	pageChangedCallbacks.call(&cbData);
	}
	
	if(maxItemWidth<it.width)
		{
		maxItemWidth=it.width;
		if(autoResize&&maxItemWidth>itemsBox.size[0])
			{
			if(isManaged)
				parent->requestResize(this,calcNaturalSize());
			else
				resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
			}
		}
	}

void ListBox::setItem(int index,const char* newItem)
	{
	/* Replace the list item: */
	GLfloat oldItemWidth=items[index].width;
	delete[] items[index].item;
	items[index].item=new char[strlen(newItem)+1];
	strcpy(items[index].item,newItem);
	items[index].width=font->calcStringBox(newItem).size[0];
	int reasonMask=0x0;
	
	if(position<=index&&index<position+pageSize)
		{
		/* Update the visible list items: */
		GLfloat oldMaxVisibleItemWidth=maxVisibleItemWidth;
		calcMaxVisibleItemWidth();
		
		if(oldMaxVisibleItemWidth!=maxVisibleItemWidth)
			{
			reasonMask|=PageChangedCallbackData::MAXITEMWIDTH_CHANGED;
			
			/* Adjust the horizontal offset: */
			if(horizontalOffset>0.0f&&horizontalOffset>maxVisibleItemWidth-itemsBox.size[0])
				{
				horizontalOffset=maxVisibleItemWidth-itemsBox.size[0];
				reasonMask|=PageChangedCallbackData::HORIZONTALOFFSET_CHANGED;
				}
			if(horizontalOffset<0.0f)
				horizontalOffset=0.0f;
			}
		updatePageSlots();
		}
	
	/* Call the list changed callbacks: */
	ListChangedCallbackData cbData(this,ListChangedCallbackData::ITEM_CHANGED,index);
	listChangedCallbacks.call(&cbData);
	
	if(reasonMask!=0x0)
		{
		/* Call the page change callbacks: */
		PageChangedCallbackData cbData(this,reasonMask,position,int(items.size()),pageSize,horizontalOffset,maxVisibleItemWidth,itemsBox.size[0]);
		pageChangedCallbacks.call(&cbData);
		}
	
	if(maxItemWidth<items[index].width)
		{
		maxItemWidth=items[index].width;
		if(autoResize&&maxItemWidth>itemsBox.size[0])
			{
			if(isManaged)
				parent->requestResize(this,calcNaturalSize());
			else
				resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
			}
		}
	else if(maxItemWidth==oldItemWidth)
		{
		/* Find the new widest item:*/
		maxItemWidth=0.0f;
		for(std::vector<Item>::const_iterator iIt=items.begin();iIt!=items.end();++iIt)
			if(maxItemWidth<iIt->width)
				maxItemWidth=iIt->width;
		
		if(autoResize&&maxItemWidth<oldItemWidth&&itemsBox.size[0]==oldItemWidth)
			{
			if(isManaged)
				parent->requestResize(this,calcNaturalSize());
			else
				resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
			}
		}
	}

void ListBox::removeItem(int index)
	{
	/* Remove the list item: */
	GLfloat oldItemWidth=items[index].width;
	delete[] items[index].item;
	items.erase(items.begin()+index);
	int reasonMask=PageChangedCallbackData::NUMITEMS_CHANGED;
	
	if(index<position)
		{
		/* Adjust the position so that the list of visible items does not change: */
		--position;
		reasonMask|=PageChangedCallbackData::POSITION_CHANGED;
		}
	else if(index>=position&&index<position+pageSize)
		{
		/* Adjust the position if the page overruns the shorter list: */
		if(position>0&&position>int(items.size())-pageSize)
			{
			position=int(items.size())-pageSize;
			reasonMask|=PageChangedCallbackData::POSITION_CHANGED;
			}
		if(position<0)
			position=0;
		
		/* Update the visible list items: */
		GLfloat oldMaxVisibleItemWidth=maxVisibleItemWidth;
		calcMaxVisibleItemWidth();
		
		if(oldMaxVisibleItemWidth!=maxVisibleItemWidth)
			{
			reasonMask|=PageChangedCallbackData::MAXITEMWIDTH_CHANGED;
			
			/* Adjust the horizontal offset: */
			if(horizontalOffset>0.0f&&horizontalOffset>maxVisibleItemWidth-itemsBox.size[0])
				{
				horizontalOffset=maxVisibleItemWidth-itemsBox.size[0];
				reasonMask|=PageChangedCallbackData::HORIZONTALOFFSET_CHANGED;
				}
			if(horizontalOffset<0.0f)
				horizontalOffset=0.0f;
			}
		
		updatePageSlots();
		}
	
	/* Update the selected item if it is affected: */
	if(selectedItem==index)
		{
		/* Select the next item in the list: */
		int newSelectedItem=selectedItem;
		if(newSelectedItem>int(items.size())-1)
			newSelectedItem=int(items.size())-1;
		selectItem(newSelectedItem);
		}
	else if(selectedItem>index)
		{
		/* Adjust the selected item's index: */
		selectItem(selectedItem-1);
		}
	
	{
	/* Call the list changed callbacks: */
	ListChangedCallbackData cbData(this,ListChangedCallbackData::ITEM_REMOVED,index);
	listChangedCallbacks.call(&cbData);
	}
	
	{
	/* Call the page change callbacks: */
	PageChangedCallbackData cbData(this,reasonMask,position,int(items.size()),pageSize,horizontalOffset,maxVisibleItemWidth,itemsBox.size[0]);
	pageChangedCallbacks.call(&cbData);
	}
	
	if(maxItemWidth==oldItemWidth)
		{
		/* Find the new widest item:*/
		maxItemWidth=0.0f;
		for(std::vector<Item>::const_iterator iIt=items.begin();iIt!=items.end();++iIt)
			if(maxItemWidth<iIt->width)
				maxItemWidth=iIt->width;
		
		if(autoResize&&maxItemWidth<oldItemWidth&&itemsBox.size[0]==oldItemWidth)
			{
			if(isManaged)
				parent->requestResize(this,calcNaturalSize());
			else
				resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
			}
		}
	}

void ListBox::selectItem(int newSelectedItem,bool moveToPage)
	{
	if(selectedItem==newSelectedItem)
		return;
	
	if(moveToPage&&newSelectedItem>=0)
		{
		/* Move the selected item to the page if it is not visible: */
		if(position>newSelectedItem)
			setPosition(newSelectedItem);
		else if(position<newSelectedItem-pageSize+1)
			setPosition(newSelectedItem-pageSize+1);
		}
	
	/* Invalidate the page slot cache if the old or new selected items are visible: */
	if(selectedItem>=position&&selectedItem<position+pageSize)
		++version;
	selectedItem=newSelectedItem;
	if(selectedItem>=position&&selectedItem<position+pageSize)
		++version;
	}

}
