/***********************************************************************
GlyphRenderer - Class to quickly render several kinds of common glyphs.
Copyright (c) 2004-2005 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <GL/GLValueCoders.h>
#include <GL/GLTransformationWrappers.h>
#include <GL/GLModels.h>

#include <Vrui/GlyphRenderer.h>

namespace Vrui {

/**********************
Methods of class Glyph:
**********************/

void Glyph::render(int glyphType,GLfloat glyphSize)
	{
	switch(glyphType)
		{
		case CONE:
			glRotatef(-90.0f,1.0f,0.0f,0.0f);
			glTranslatef(0.0f,0.0f,-0.75f*glyphSize);
			glDrawCone(0.25f*glyphSize,glyphSize,16);
			break;
		
		case CUBE:
			glDrawCube(glyphSize);
			break;
		
		case SPHERE:
			glDrawSphereIcosahedron(0.5f*glyphSize,8);
			break;
		
		case CROSSBALL:
			glDrawSphereIcosahedron(0.4f*glyphSize,8);
			glDrawCylinder(0.125f*glyphSize,1.1f*glyphSize,16);
			glRotatef(90.0f,1.0f,0.0f,0.0f);
			glDrawCylinder(0.125f*glyphSize,1.1f*glyphSize,16);
			glRotatef(90.0f,0.0f,1.0f,0.0f);
			glDrawCylinder(0.125f*glyphSize,1.1f*glyphSize,16);
			break;
		
		case BOX:
			{
			#if 1
			glDrawWireframeCube(glyphSize,glyphSize*0.075f,glyphSize*0.15f);
			#else
			GLfloat gs=glyphSize*0.5f;
			glPushAttrib(GL_ENABLE_BIT|GL_POINT_BIT|GL_LINE_BIT);
			glDisable(GL_LIGHTING);
			glPointSize(5.0f);
			glBegin(GL_POINTS);
			glVertex3f(-gs,-gs,-gs);
			glVertex3f( gs,-gs,-gs);
			glVertex3f(-gs, gs,-gs);
			glVertex3f( gs, gs,-gs);
			glVertex3f(-gs,-gs, gs);
			glVertex3f( gs,-gs, gs);
			glVertex3f(-gs, gs, gs);
			glVertex3f( gs, gs, gs);
			glEnd();
			glLineWidth(3.0f);
			glBegin(GL_LINE_STRIP);
			glVertex3f(-gs,-gs,-gs);
			glVertex3f( gs,-gs,-gs);
			glVertex3f( gs, gs,-gs);
			glVertex3f(-gs, gs,-gs);
			glVertex3f(-gs,-gs,-gs);
			glVertex3f(-gs,-gs, gs);
			glVertex3f( gs,-gs, gs);
			glVertex3f( gs, gs, gs);
			glVertex3f(-gs, gs, gs);
			glVertex3f(-gs,-gs, gs);
			glEnd();
			glBegin(GL_LINES);
			glVertex3f( gs,-gs,-gs);
			glVertex3f( gs,-gs, gs);
			glVertex3f( gs, gs,-gs);
			glVertex3f( gs, gs, gs);
			glVertex3f(-gs, gs,-gs);
			glVertex3f(-gs, gs, gs);
			glEnd();
			glPopAttrib();
			#endif
			break;
			}
		}
	}

Glyph::Glyph(void)
	:enabled(false),
	 glyphType(CROSSBALL),
	 glyphMaterial(GLMaterial::Color(0.5f,0.5f,0.5f),GLMaterial::Color(1.0f,1.0f,1.0f),25.0f)
	{
	}

void Glyph::enable(void)
	{
	enabled=true;
	}

void Glyph::enable(Glyph::GlyphType newGlyphType,const GLMaterial& newGlyphMaterial)
	{
	enabled=true;
	glyphType=newGlyphType;
	glyphMaterial=newGlyphMaterial;
	}

void Glyph::disable(void)
	{
	enabled=false;
	}

void Glyph::setGlyphType(Glyph::GlyphType newGlyphType)
	{
	glyphType=newGlyphType;
	}

void Glyph::setGlyphMaterial(const GLMaterial& newGlyphMaterial)
	{
	glyphMaterial=newGlyphMaterial;
	}

void Glyph::configure(const Misc::ConfigurationFileSection& configFileSection,const char* glyphTypeTagName,const char* glyphMaterialTagName)
	{
	/* Retrieve glyph type as string: */
	std::string glyphTypeName=configFileSection.retrieveString(glyphTypeTagName,"None");
	if(glyphTypeName!="None")
		{
		if(glyphTypeName=="Cone")
			glyphType=CONE;
		else if(glyphTypeName=="Cube")
			glyphType=CUBE;
		else if(glyphTypeName=="Sphere")
			glyphType=SPHERE;
		else if(glyphTypeName=="Crossball")
			glyphType=CROSSBALL;
		else if(glyphTypeName=="Box")
			glyphType=BOX;
		else
			Misc::throwStdErr("GlyphRenderer::Glyph: Invalid glyph type %s",glyphTypeName.c_str());
		enabled=true;
		glyphMaterial=configFileSection.retrieveValue<GLMaterial>(glyphMaterialTagName,glyphMaterial);
		}
	else
		enabled=false;
	}

/****************************************
Methods of class GlyphRenderer::DataItem:
****************************************/

GlyphRenderer::DataItem::DataItem(void)
	:glyphDisplayLists(glGenLists(Glyph::GLYPHS_END))
	{
	}

GlyphRenderer::DataItem::~DataItem(void)
	{
	glDeleteLists(glyphDisplayLists,Glyph::GLYPHS_END);
	}

/******************************
Methods of class GlyphRenderer:
******************************/

GlyphRenderer::GlyphRenderer(GLfloat sGlyphSize)
	:glyphSize(sGlyphSize)
	{
	}

void GlyphRenderer::initContext(GLContextData& contextData) const
	{
	/* Create a new context data item: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Render all glyph types: */
	for(int glyphType=Glyph::CONE;glyphType<Glyph::GLYPHS_END;++glyphType)
		{
		glNewList(dataItem->glyphDisplayLists+glyphType,GL_COMPILE);
		Glyph::render(glyphType,glyphSize);
		glEndList();
		}
	}

void GlyphRenderer::renderGlyph(const Glyph& glyph,const OGTransform& transformation,const GlyphRenderer::DataItem* contextDataItem) const
	{
	/* Check if the glyph is enabled: */
	if(glyph.enabled)
		{
		/* Render glyph: */
		glPushMatrix();
		glMultMatrix(transformation);
		glMaterial(GLMaterialEnums::FRONT,glyph.glyphMaterial);
		glCallList(contextDataItem->glyphDisplayLists+glyph.glyphType);
		glPopMatrix();
		}
	}

}
