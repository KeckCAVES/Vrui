/***********************************************************************
Label - Class for (text) labels.
Copyright (c) 2001-2005 Oliver Kreylos

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
#include <GL/GLObject.h>
#include <GL/GLFont.h>
#include <GLMotif/Widget.h>

namespace GLMotif {

class Label:public Widget,public GLObject
	{
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint stringVersion;
		GLuint textureObjectId;

		/* Constructors and destructors: */
		DataItem(GLuint sStringVersion)
			:stringVersion(sStringVersion)
			{
			glGenTextures(1,&textureObjectId);
			}
		~DataItem(void)
			{
			glDeleteTextures(1,&textureObjectId);
			}
		};
	
	/* Elements: */
	protected:
	GLfloat marginWidth; // Width of margin around label string
	GLfloat leftInset,rightInset; // Additional inset spacing to the left and the right of the label
	const GLFont* font; // Pointer to the font used to render labels
	GLuint labelVersion; // Version of label string (to compare against texture object version)
	char* label; // Label string
	Box labelBox; // Position and natural size of label string
	GLFont::TBox labelTexCoords; // Texture coordinates of label texture
	GLFont::HAlignment hAlignment; // Horizontal alignment of label string in widget
	GLFont::VAlignment vAlignment; // Vertical alignment of label string in widget
	
	/* Protected methods: */
	void positionLabel(void); // Positions the label inside the widget
	void drawLabel(GLContextData& contextData) const; // Draws the label texture
	void setInsets(GLfloat newLeftInset,GLfloat newRightInset); // Sets the inset values
	
	/* Constructors and destructors: */
	public:
	Label(const char* sName,Container* sParent,const char* sLabel,const GLFont* sFont,bool manageChild =true); // Deprecated
	Label(const char* sName,Container* sParent,const char* sLabel,bool manageChild =true);
	virtual ~Label(void);
	
	/* Methods inherited from Widget: */
	virtual Vector calcNaturalSize(void) const;
	virtual void resize(const Box& newExterior);
	virtual void setBackgroundColor(const Color& newBackgroundColor);
	virtual void setForegroundColor(const Color& newForegroundColor);
	virtual void draw(GLContextData& contextData) const;
	
	/* Methods inherited from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New methods: */
	GLfloat getMarginWidth(void) const // Returns the label's margin width
		{
		return marginWidth;
		}
	void setMarginWidth(GLfloat newMarginWidth); // Changes the margin width
	void setHAlignment(GLFont::HAlignment newHAlignment); // Changes the horizontal alignment
	void setVAlignment(GLFont::VAlignment newVAlignment); // Changes the vertical alignment
	const char* getLabel(void) const // Returns the current label text
		{
		return label;
		}
	virtual void setLabel(const char* newLabel); // Changes the label string
	};

}

#endif
