/***********************************************************************
CascadeButton - Class for buttons that pop up secondary top-level
GLMotif UI components.
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

#ifndef GLMOTIF_CASCADEBUTTON_INCLUDED
#define GLMOTIF_CASCADEBUTTON_INCLUDED

#include <GL/gl.h>
#include <GLMotif/GlyphGadget.h>
#include <GLMotif/DecoratedButton.h>

/* Forward declarations: */
namespace GLMotif {
class Popup;
}

namespace GLMotif {

class CascadeButton:public DecoratedButton
	{
	/* Elements: */
	protected:
	Popup* popup; // Window to pop up when button is selected
	bool isPopped; // Flag if the popup window is displayed
	Point popupBottom,popupTop; // Bottom-left and top-left corners of popup in button's coordinate system
	Point lastEventPos; // Last event position for which findRecipient was called
	Point foundPos; // Event position in current event processing round
	Widget* foundWidget; // Widget that responded to the last findRecipient call
	GlyphGadget arrow; // The cascade button's arrow glyph
	
	/* Protected methods inherited from Button: */
	virtual void setArmed(bool newArmed);
	
	/* Protected methods inherited from DecoratedButton: */
	virtual void drawDecoration(GLContextData& contextData) const;
	
	/* Constructors and destructors: */
	public:
	CascadeButton(const char* sName,Container* sParent,const char* sLabel,const GLFont* sFont,bool manageChild =true); // Deprecated
	CascadeButton(const char* sName,Container* sParent,const char* sLabel,bool manageChild =true);
	virtual ~CascadeButton(void);
	
	/* Methods inherited from Widget: */
	virtual ZRange calcZRange(void) const;
	virtual void resize(const Box& newExterior);
	virtual void setBackgroundColor(const Color& newBackgroundColor);
	virtual bool findRecipient(Event& event);
	virtual void pointerButtonDown(Event& event);
	virtual void pointerButtonUp(Event& event);
	virtual void pointerMotion(Event& event);
	
	/* New methods: */
	void setPopup(Popup* newPopup);
	void setArrowBorderSize(GLfloat newArrowBorderSize);
	void setArrowSize(GLfloat newArrowSize);
	Popup* getPopup(void)
		{
		return popup;
		}
	};

}

#endif
