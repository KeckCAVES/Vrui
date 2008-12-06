/***********************************************************************
TextField - Class for labels displaying values as text.
Copyright (c) 2006 Oliver Kreylos

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

#ifndef GLMOTIF_TEXTFIELD_INCLUDED
#define GLMOTIF_TEXTFIELD_INCLUDED

#include <GLMotif/Label.h>

namespace GLMotif {

class TextField:public Label
	{
	/* Embedded classes: */
	public:
	enum FloatFormat // Enumerated type for floating-point number formatting modes
		{
		FIXED,SCIENTIFIC,SMART
		};
	
	/* Elements: */
	protected:
	GLint charWidth; // Fixed width of the text field's interior in average character widths
	GLint fieldWidth,precision; // Field width and precision for numerical values (negative values disable that formatting feature)
	FloatFormat floatFormat; // Formatting mode for floating-point numbers (default is smart)
	
	/* Protected methods: */
	char* createFormatString(char* buffer); // Creates a printf format string for floating-point numbers in the given buffer
	
	/* Constructors and destructors: */
	public:
	TextField(const char* sName,Container* sParent,const GLFont* sFont,GLint sCharWidth,bool manageChild =true); // Deprecated
	TextField(const char* sName,Container* sParent,GLint sCharWidth,bool manageChild =true);
	
	/* Methods inherited from Label: */
	virtual Vector calcNaturalSize(void) const;
	virtual void setLabel(const char* newLabel);
	
	/* New methods: */
	GLint getCharWidth(void) const // Returns current text field size in characters
		{
		return charWidth;
		}
	void setCharWidth(GLint newCharWidth); // Sets the text field's new width
	GLint getFieldWidth(void) const // Returns field width for numerical values
		{
		return fieldWidth;
		}
	void setFieldWidth(GLint newFieldWidth); // Sets new field width for numerical values
	GLint getPrecision(void) const // Returns precision for numerical values
		{
		return precision;
		}
	void setPrecision(GLint newPrecision); // Sets precision for numerical values
	FloatFormat getFloatFormat(void) const // Returns floating-point formatting mode
		{
		return floatFormat;
		}
	void setFloatFormat(FloatFormat newFloatFormat); // Sets new floating-point formatting mode
	template <class ValueParam>
	void setValue(const ValueParam& value); // Sets the text field to the given value; works for int, unsigned int, float, and double
	};

}

#endif
