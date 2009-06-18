/***********************************************************************
PopupWindow - Class for main windows with a draggable title bar and an
optional close button.
Copyright (c) 2001-2009 Oliver Kreylos

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

#ifndef GLMOTIF_POPUPWINDOW_INCLUDED
#define GLMOTIF_POPUPWINDOW_INCLUDED

#include <GLMotif/Container.h>

/* Forward declarations: */
class GLFont;
namespace GLMotif {
class TitleBar;
}

namespace GLMotif {

class PopupWindow:public Container
	{
	/* Elements: */
	protected:
	WidgetManager* manager; // Pointer to the widget manager
	TitleBar* titleBar; // Pointer to the title bar widget
	int resizableMask; // Bit mask whether the window can be resized horizontally (0x1) and/or vertically (0x2)
	GLfloat childBorderWidth; // Width of border around child widget
	Widget* child; // Single child of the popup window
	
	bool isResizing; // Flag if the window is currently being resized by the user
	int resizeBorderMask; // Bit mask of which borders are being dragged 1 - left, 2 - right, 4 - bottom, 8 - top
	GLfloat resizeOffset[2]; // Offset from the initial resizing position to the relevant border
	
	/* Constructors and destructors: */
	public:
	PopupWindow(const char* sName,WidgetManager* sManager,const char* sTitleString,const GLFont* font); // Deprecated
	PopupWindow(const char* sName,WidgetManager* sManager,const char* sTitleString);
	virtual ~PopupWindow(void); // Pops down the popup window and destroys it
	
	/* Methods inherited from Widget: */
	virtual const WidgetManager* getManager(void) const
		{
		return manager;
		}
	virtual WidgetManager* getManager(void)
		{
		return manager;
		}
	virtual Vector calcNaturalSize(void) const;
	virtual ZRange calcZRange(void) const;
	virtual void resize(const Box& newExterior);
	virtual Vector calcHotSpot(void) const;
	virtual void draw(GLContextData& contextData) const;
	virtual bool findRecipient(Event& event);
	virtual void pointerButtonDown(Event& event);
	virtual void pointerButtonUp(Event& event);
	virtual void pointerMotion(Event& event);
	
	/* Methods inherited from Container: */
	virtual void addChild(Widget* newChild);
	virtual void requestResize(Widget* child,const Vector& newExteriorSize);
	virtual Widget* getFirstChild(void);
	virtual Widget* getNextChild(Widget* child);
	
	/* New methods: */
	void setTitleBorderWidth(GLfloat newTitleBorderWidth); // Changes the title border width
	void setTitleBarColor(const Color& newTitleBarColor); // Sets the color of the title bar
	void setTitleBarTextColor(const Color& newTitleBarTextColor); // Sets the text color of the title bar
	void setTitleString(const char* newTitleString); // Changes the title label string
	void setResizableFlags(bool horizontal,bool vertical); // Sets whether the popup window can be resized interactively
	void setChildBorderWidth(GLfloat newChildBorderWidth); // Changes the border width around the child widget
	const char* getTitleString(void) const; // Returns the current title label string
	const Widget* getChild(void) const // Returns the popup window's child
		{
		return child;
		}
	Widget* getChild(void) // Ditto
		{
		return child;
		}
	};

}

#endif
