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

#include <string.h>
#include <stdio.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/Container.h>
#include <GLMotif/WidgetManager.h>

#include <GLMotif/TextField.h>

namespace GLMotif {

/**************************
Methods of class TextField:
**************************/

char* TextField::createFormatString(char* buffer)
	{
	char* bufPtr=buffer;
	*bufPtr++='%';
	if(fieldWidth>=0)
		*bufPtr++='*';
	if(precision>=0)
		{
		*bufPtr++='.';
		*bufPtr++='*';
		}
	switch(floatFormat)
		{
		case FIXED:
			*bufPtr++='f';
			break;
		
		case SCIENTIFIC:
			*bufPtr++='e';
			break;
		
		case SMART:
			*bufPtr++='g';
			break;
		}
	*bufPtr++='\0';
	return buffer;
	}

TextField::TextField(const char* sName,Container* sParent,const GLFont* sFont,GLint sCharWidth,bool sManageChild)
	:Label(sName,sParent,"",sFont,false),
	 charWidth(sCharWidth),
	 fieldWidth(-1),precision(-1),floatFormat(SMART)
	{
	/* Get the style sheet: */
	const StyleSheet* ss=getStyleSheet();
	
	setBorderWidth(ss->textfieldBorderWidth);
	setBorderType(Widget::LOWERED);
	setBackgroundColor(ss->textfieldBgColor);
	setForegroundColor(ss->textfieldFgColor);
	setMarginWidth(ss->textfieldMarginWidth);
	setHAlignment(GLFont::Right);
	
	/* Manage me: */
	if(sManageChild)
		manageChild();
	}

TextField::TextField(const char* sName,Container* sParent,GLint sCharWidth,bool sManageChild)
	:Label(sName,sParent,"",false),
	 charWidth(sCharWidth),
	 fieldWidth(-1),precision(-1),floatFormat(SMART)
	{
	/* Get the style sheet: */
	const StyleSheet* ss=getStyleSheet();
	
	setBorderWidth(ss->textfieldBorderWidth);
	setBorderType(Widget::LOWERED);
	setBackgroundColor(ss->textfieldBgColor);
	setForegroundColor(ss->textfieldFgColor);
	setMarginWidth(ss->textfieldMarginWidth);
	setHAlignment(GLFont::Right);
	
	/* Manage me: */
	if(sManageChild)
		manageChild();
	}

Vector TextField::calcNaturalSize(void) const
	{
	/* Return the size of the text box plus margin: */
	Vector result(GLfloat(charWidth)*font->getCharacterWidth(),font->getTextHeight(),0.0f);
	result[0]+=2.0f*marginWidth+leftInset+rightInset;
	result[1]+=2.0f*marginWidth;
	
	return calcExteriorSize(result);
	}

void TextField::setLabel(const char* newLabel)
	{
	/* Copy the new label string: */
	delete[] label;
	label=new char[strlen(newLabel)+1];
	strcpy(label,newLabel);
	++labelVersion;
	
	/* Calculate the label's bounding box size and texture coordinates: */
	labelBox=font->calcStringBox(label);
	labelTexCoords=font->calcStringTexCoords(label);
	
	/* Adjust the label position: */
	positionLabel();
	}

void TextField::setCharWidth(GLint newCharWidth)
	{
	/* Set the width: */
	charWidth=newCharWidth;
	
	if(isManaged)
		{
		/* Try adjusting the widget size to accomodate the new margin width: */
		parent->requestResize(this,calcNaturalSize());
		}
	else
		resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
	}

void TextField::setFieldWidth(GLint newFieldWidth)
	{
	/* Set the field width: */
	fieldWidth=newFieldWidth;
	}

void TextField::setPrecision(GLint newPrecision)
	{
	/* Set the precision: */
	precision=newPrecision;
	}

void TextField::setFloatFormat(TextField::FloatFormat newFloatFormat)
	{
	/* Set the floating-point formatting mode: */
	floatFormat=newFloatFormat;
	}

template <class ValueParam>
void TextField::setValue(const ValueParam& value)
	{
	}

template <>
void TextField::setValue<int>(const int& value)
	{
	char label[80];
	if(fieldWidth>=0)
		snprintf(label,sizeof(label),"%*d",fieldWidth,value);
	else
		snprintf(label,sizeof(label),"%d",value);
	setLabel(label);
	}

template <>
void TextField::setValue<unsigned int>(const unsigned int& value)
	{
	char label[80];
	if(fieldWidth>=0)
		snprintf(label,sizeof(label),"%*u",fieldWidth,value);
	else
		snprintf(label,sizeof(label),"%u",value);
	setLabel(label);
	}

template <>
void TextField::setValue<float>(const float& value)
	{
	char format[10],label[80];
	if(fieldWidth>=0)
		{
		if(precision>=0)
			snprintf(label,sizeof(label),createFormatString(format),fieldWidth,precision,value);
		else
			snprintf(label,sizeof(label),createFormatString(format),fieldWidth,value);
		}
	else
		{
		if(precision>=0)
			snprintf(label,sizeof(label),createFormatString(format),precision,value);
		else
			snprintf(label,sizeof(label),createFormatString(format),value);
		}
	setLabel(label);
	}

template <>
void TextField::setValue<double>(const double& value)
	{
	char format[10],label[80];
	if(fieldWidth>=0)
		{
		if(precision>=0)
			snprintf(label,sizeof(label),createFormatString(format),fieldWidth,precision,value);
		else
			snprintf(label,sizeof(label),createFormatString(format),fieldWidth,value);
		}
	else
		{
		if(precision>=0)
			snprintf(label,sizeof(label),createFormatString(format),precision,value);
		else
			snprintf(label,sizeof(label),createFormatString(format),value);
		}
	setLabel(label);
	}

}
