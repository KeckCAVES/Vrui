/***********************************************************************
Label - Class for (text) labels.
Copyright (c) 2001-2010 Oliver Kreylos

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

#ifndef GLMOTIF_LABEL_INCLUDED
#define GLMOTIF_LABEL_INCLUDED

#include <GL/gl.h>
#include <GL/GLFont.h>
#include <GL/GLLabel.h>
#include <GLMotif/Widget.h>

namespace GLMotif {

class Label:public Widget
	{
	/* Elements: */
	protected:
	GLfloat marginWidth; // Width of margin around label string
	GLfloat leftInset,rightInset; // Additional inset spacing to the left and the right of the label
	GLLabel label; // The label string
	GLFont::HAlignment hAlignment; // Horizontal alignment of label string in widget
	GLFont::VAlignment vAlignment; // Vertical alignment of label string in widget
	
	/* Protected methods: */
	void positionLabel(void); // Positions the label inside the widget
	void setInsets(GLfloat newLeftInset,GLfloat newRightInset); // Sets the inset values
	
	/* Constructors and destructors: */
	public:
	Label(const char* sName,Container* sParent,const char* sLabel,const GLFont* sFont,bool manageChild =true); // Deprecated
	Label(const char* sName,Container* sParent,const char* sLabel,bool manageChild =true);
	Label(const char* sName,Container* sParent,const char* sLabelBegin,const char* sLabelEnd,bool manageChild =true);
	
	/* Methods inherited from Widget: */
	virtual Vector calcNaturalSize(void) const;
	virtual void resize(const Box& newExterior);
	virtual void setBackgroundColor(const Color& newBackgroundColor);
	virtual void setForegroundColor(const Color& newForegroundColor);
	virtual void draw(GLContextData& contextData) const;
	
	/* New methods: */
	GLfloat getMarginWidth(void) const // Returns the label's margin width
		{
		return marginWidth;
		}
	void setMarginWidth(GLfloat newMarginWidth); // Changes the margin width
	void setHAlignment(GLFont::HAlignment newHAlignment); // Changes the horizontal alignment
	void setVAlignment(GLFont::VAlignment newVAlignment); // Changes the vertical alignment
	const GLLabel& getLabel(void) const // Returns the label object
		{
		return label;
		}
	int getLabelLength(void) const // Returns the length of the current label text
		{
		return label.getLength();
		}
	const char* getString(void) const // Returns the current label text
		{
		return label.getString();
		}
	virtual void setString(const char* newLabelBegin,const char* newLabelEnd); // Changes the label text
	void setString(const char* newLabel); // Convenience version of above for C-style strings
	};

}

#endif
