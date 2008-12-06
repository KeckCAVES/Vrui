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

#include <string.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLTexCoordTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLTexEnvTemplates.h>
#include <GL/GLContextData.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/Container.h>

#include <GLMotif/Label.h>

namespace GLMotif {

/**********************
Methods of class Label:
**********************/

void Label::positionLabel(void)
	{
	/* Position the label box according to the alignment parameters: */
	labelBox.origin=getInterior().origin;
	labelBox.doOffset(Vector(marginWidth+leftInset,marginWidth,0.0f));
	switch(hAlignment)
		{
		case GLFont::Left:
			break;
		
		case GLFont::Center:
			labelBox.origin[0]+=0.5f*(getInterior().size[0]-2.0f*marginWidth-leftInset-rightInset-labelBox.size[0]);
			break;
		
		case GLFont::Right:
			labelBox.origin[0]+=getInterior().size[0]-2.0f*marginWidth-leftInset-rightInset-labelBox.size[0];
			break;
		}
	switch(vAlignment)
		{
		case GLFont::Bottom:
			break;
		
		case GLFont::Baseline:
			/* Don't know what to do about it. */
			break;
		
		case GLFont::VCenter:
			labelBox.origin[1]+=0.5f*(getInterior().size[1]-2.0f*marginWidth-labelBox.size[1]);
			break;
		
		case GLFont::Top:
			labelBox.origin[1]+=getInterior().size[1]-2.0f*marginWidth-labelBox.size[1];
			break;
		}
	}

void Label::drawLabel(GLContextData& contextData) const
	{
	/* Retrieve the context data: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Draw the label: */
	glPushAttrib(GL_TEXTURE_BIT);
	GLint lightModelColorControl;
	glGetIntegerv(GL_LIGHT_MODEL_COLOR_CONTROL,&lightModelColorControl);
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,dataItem->textureObjectId);
	if(dataItem->stringVersion!=labelVersion) // Has the string been changed?
		{
		/* Upload the label string texture again: */
		font->uploadStringTexture(label,backgroundColor,foregroundColor);
		dataItem->stringVersion=labelVersion;
		}
	glTexEnvMode(GLTexEnvEnums::TEXTURE_ENV,GLTexEnvEnums::MODULATE);
	glColor4f(1.0f,1.0f,1.0f,backgroundColor[3]);
	glBegin(GL_QUADS);
	glNormal3f(0.0f,0.0f,1.0f);
	glTexCoord(labelTexCoords.getCorner(0));
	glVertex(labelBox.getCorner(0));
	glTexCoord(labelTexCoords.getCorner(1));
	glVertex(labelBox.getCorner(1));
	glTexCoord(labelTexCoords.getCorner(3));
	glVertex(labelBox.getCorner(3));
	glTexCoord(labelTexCoords.getCorner(2));
	glVertex(labelBox.getCorner(2));
	glEnd();
	glBindTexture(GL_TEXTURE_2D,0);
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,lightModelColorControl);
	glPopAttrib();
	}

void Label::setInsets(GLfloat newLeftInset,GLfloat newRightInset)
	{
	/* Set the insets: */
	leftInset=newLeftInset;
	rightInset=newRightInset;
	
	if(isManaged)
		{
		/* Try adjusting the widget size to accomodate the new insets: */
		parent->requestResize(this,calcNaturalSize());
		}
	else
		resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
	}

Label::Label(const char* sName,Container* sParent,const char* sLabel,const GLFont* sFont,bool sManageChild)
	:Widget(sName,sParent,false),
	 marginWidth(0.0f),
	 leftInset(0.0f),rightInset(0.0f),
	 font(sFont),
	 labelVersion(0),label(0),hAlignment(GLFont::Left),vAlignment(GLFont::VCenter)
	{
	/* Get the style sheet: */
	const StyleSheet* ss=getStyleSheet();
	
	/* Label defaults to no border: */
	setBorderWidth(0.0f);
	
	/* Get the margin width: */
	marginWidth=ss->labelMarginWidth;
	
	/* Set the label string: */
	setLabel(sLabel);
	
	/* Manage me: */
	if(sManageChild)
		manageChild();
	}

Label::Label(const char* sName,Container* sParent,const char* sLabel,bool sManageChild)
	:Widget(sName,sParent,false),
	 marginWidth(0.0f),
	 leftInset(0.0f),rightInset(0.0f),
	 font(0),
	 labelVersion(0),label(0),hAlignment(GLFont::Left),vAlignment(GLFont::VCenter)
	{
	/* Get the style sheet: */
	const StyleSheet* ss=getStyleSheet();
	
	/* Label defaults to no border: */
	setBorderWidth(0.0f);
	
	/* Get the margin width: */
	marginWidth=ss->labelMarginWidth;
	
	/* Get the font: */
	font=ss->font;
	
	/* Set the label string: */
	setLabel(sLabel);
	
	/* Manage me: */
	if(sManageChild)
		manageChild();
	}

Label::~Label(void)
	{
	delete[] label;
	}

Vector Label::calcNaturalSize(void) const
	{
	/* Return size of text plus margin: */
	Vector result=labelBox.size;
	result[0]+=2.0f*marginWidth+leftInset+rightInset;
	result[1]+=2.0f*marginWidth;
	
	return calcExteriorSize(result);
	}

void Label::resize(const Box& newExterior)
	{
	/* Resize the parent class widget: */
	Widget::resize(newExterior);
	
	/* Adjust the label position: */
	positionLabel();
	}

void Label::setBackgroundColor(const Color& newBackgroundColor)
	{
	Widget::setBackgroundColor(newBackgroundColor);
	++labelVersion;
	}

void Label::setForegroundColor(const Color& newForegroundColor)
	{
	Widget::setForegroundColor(newForegroundColor);
	++labelVersion;
	}

void Label::draw(GLContextData& contextData) const
	{
	/* Draw parent class decorations: */
	Widget::draw(contextData);
	
	/* Draw the label margin: */
	glColor(backgroundColor);
	glBegin(GL_QUAD_STRIP);
	glNormal3f(0.0f,0.0f,1.0f);
	glVertex(labelBox.getCorner(0));
	glVertex(getInterior().getCorner(0));
	glVertex(labelBox.getCorner(1));
	glVertex(getInterior().getCorner(1));
	glVertex(labelBox.getCorner(3));
	glVertex(getInterior().getCorner(3));
	glVertex(labelBox.getCorner(2));
	glVertex(getInterior().getCorner(2));
	glVertex(labelBox.getCorner(0));
	glVertex(getInterior().getCorner(0));
	glEnd();
	
	/* Draw the label texture: */
	drawLabel(contextData);
	}

void Label::initContext(GLContextData& contextData) const
	{
	/* Create a context data item: */
	DataItem* dataItem=new DataItem(labelVersion);
	contextData.addDataItem(this,dataItem);
	
	/* Upload the string texture: */
	glBindTexture(GL_TEXTURE_2D,dataItem->textureObjectId);
	font->uploadStringTexture(label,backgroundColor,foregroundColor);
	glBindTexture(GL_TEXTURE_2D,0);
	}

void Label::setMarginWidth(GLfloat newMarginWidth)
	{
	/* Set the new margin width: */
	marginWidth=newMarginWidth;
	
	if(isManaged)
		{
		/* Try adjusting the widget size to accomodate the new margin width: */
		parent->requestResize(this,calcNaturalSize());
		}
	else
		resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
	}

void Label::setHAlignment(GLFont::HAlignment newHAlignment)
	{
	/* Set the horizontal alignment: */
	hAlignment=newHAlignment;
	
	/* Update the label position: */
	positionLabel();
	}

void Label::setVAlignment(GLFont::VAlignment newVAlignment)
	{
	/* Set the vertical alignment: */
	vAlignment=newVAlignment;
	
	/* Update the label position: */
	positionLabel();
	}

void Label::setLabel(const char* newLabel)
	{
	/* Copy the new label string: */
	delete[] label;
	size_t len=strlen(newLabel);
	label=new char[len+1];
	memcpy(label,newLabel,len+1);
	++labelVersion;
	
	/* Calculate the label's bounding box size and texture coordinates: */
	labelBox=font->calcStringBox(label);
	labelTexCoords=font->calcStringTexCoords(label);
	
	if(isManaged)
		{
		/* Try adjusting the widget size to accomodate the new label: */
		parent->requestResize(this,calcNaturalSize());
		}
	else
		resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
	}

}
