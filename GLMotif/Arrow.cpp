/***********************************************************************
Arrow - Helper class to render assorted arrow glyphs as part of other
widgets.
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

#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLNormalTemplates.h>
#include <GL/GLVertexTemplates.h>

#include <GLMotif/Arrow.h>

namespace GLMotif {

/**********************
Methods of class Arrow:
**********************/

void Arrow::createArrowGlyph(void)
	{
	/* Create the arrow box vertices: */
	switch(direction)
		{
		case LEFT:
			glyphVertices[0]=arrowBox.getCorner(2);
			glyphVertices[1]=arrowBox.getCorner(0);
			glyphVertices[2]=arrowBox.getCorner(1);
			glyphVertices[3]=arrowBox.getCorner(3);
			break;
		
		case UP:
			glyphVertices[0]=arrowBox.getCorner(3);
			glyphVertices[1]=arrowBox.getCorner(2);
			glyphVertices[2]=arrowBox.getCorner(0);
			glyphVertices[3]=arrowBox.getCorner(1);
			break;
		
		case RIGHT:
			glyphVertices[0]=arrowBox.getCorner(1);
			glyphVertices[1]=arrowBox.getCorner(3);
			glyphVertices[2]=arrowBox.getCorner(2);
			glyphVertices[3]=arrowBox.getCorner(0);
			break;
		
		case DOWN:
			glyphVertices[0]=arrowBox.getCorner(0);
			glyphVertices[1]=arrowBox.getCorner(1);
			glyphVertices[2]=arrowBox.getCorner(3);
			glyphVertices[3]=arrowBox.getCorner(2);
			break;
		}
	
	/* Create the arrow vertices: */
	Vector center;
	for(int i=0;i<2;++i)
		center[i]=arrowBox.origin[i]+arrowBox.size[i]*0.5f;
	center[2]=arrowBox.origin[2];
	switch(style)
		{
		case SIMPLE:
			for(int i=4;i<10;++i)
				glyphVertices[i]=center;
			
			switch(direction)
				{
				case LEFT:
					glyphNormals[0]=Vector(-0.408f,-0.816f, 0.408f);
					glyphNormals[1]=Vector( 0.707f, 0.000f, 0.707f);
					glyphNormals[2]=Vector(-0.408f, 0.816f, 0.408f);
					glyphVertices[4][0]-=arrowSize+arrowBevelSize;
					glyphVertices[5][0]+=arrowSize+arrowBevelSize;
					glyphVertices[5][1]-=arrowSize+arrowBevelSize;
					glyphVertices[6][0]+=arrowSize+arrowBevelSize;
					glyphVertices[6][1]+=arrowSize+arrowBevelSize;
					glyphVertices[7][0]-=arrowSize;
					glyphVertices[8][0]+=arrowSize;
					glyphVertices[8][1]-=arrowSize;
					glyphVertices[9][0]+=arrowSize;
					glyphVertices[9][1]+=arrowSize;
					break;
				
				case UP:
					glyphNormals[0]=Vector(-0.816f, 0.408f, 0.408f);
					glyphNormals[1]=Vector( 0.000f,-0.707f, 0.707f);
					glyphNormals[2]=Vector( 0.816f, 0.408f, 0.408f);
					glyphVertices[4][1]+=arrowSize+arrowBevelSize;
					glyphVertices[5][0]-=arrowSize+arrowBevelSize;
					glyphVertices[5][1]-=arrowSize+arrowBevelSize;
					glyphVertices[6][0]+=arrowSize+arrowBevelSize;
					glyphVertices[6][1]-=arrowSize+arrowBevelSize;
					glyphVertices[7][1]+=arrowSize;
					glyphVertices[8][0]-=arrowSize;
					glyphVertices[8][1]-=arrowSize;
					glyphVertices[9][0]+=arrowSize;
					glyphVertices[9][1]-=arrowSize;
					break;
				
				case RIGHT:
					glyphNormals[0]=Vector( 0.408f, 0.816f, 0.408f);
					glyphNormals[1]=Vector(-0.707f, 0.000f, 0.707f);
					glyphNormals[2]=Vector( 0.408f,-0.816f, 0.408f);
					glyphVertices[4][0]+=arrowSize+arrowBevelSize;
					glyphVertices[5][0]-=arrowSize+arrowBevelSize;
					glyphVertices[5][1]+=arrowSize+arrowBevelSize;
					glyphVertices[6][0]-=arrowSize+arrowBevelSize;
					glyphVertices[6][1]-=arrowSize+arrowBevelSize;
					glyphVertices[7][0]+=arrowSize;
					glyphVertices[8][0]-=arrowSize;
					glyphVertices[8][1]+=arrowSize;
					glyphVertices[9][0]-=arrowSize;
					glyphVertices[9][1]-=arrowSize;
					break;
				
				case DOWN:
					glyphNormals[0]=Vector( 0.816f,-0.408f, 0.408f);
					glyphNormals[1]=Vector( 0.000f, 0.707f, 0.707f);
					glyphNormals[2]=Vector(-0.816f,-0.408f, 0.408f);
					glyphVertices[4][1]-=arrowSize+arrowBevelSize;
					glyphVertices[5][0]+=arrowSize+arrowBevelSize;
					glyphVertices[5][1]+=arrowSize+arrowBevelSize;
					glyphVertices[6][0]-=arrowSize+arrowBevelSize;
					glyphVertices[6][1]+=arrowSize+arrowBevelSize;
					glyphVertices[7][1]-=arrowSize;
					glyphVertices[8][0]+=arrowSize;
					glyphVertices[8][1]+=arrowSize;
					glyphVertices[9][0]-=arrowSize;
					glyphVertices[9][1]+=arrowSize;
					break;
				}
			
			switch(depth)
				{
				case IN:
					for(int i=0;i<3;++i)
						{
						glyphNormals[i][0]=-glyphNormals[i][0];
						glyphNormals[i][1]=-glyphNormals[i][1];
						}
					for(int i=7;i<10;++i)
						glyphVertices[i][2]-=arrowBevelSize;
					break;
				
				case OUT:
					for(int i=7;i<10;++i)
						glyphVertices[i][2]+=arrowBevelSize;
					break;
				}
			break;
		
		case FANCY:
			for(int i=4;i<18;++i)
				glyphVertices[i]=center;
			
			switch(direction)
				{
				case LEFT:
					glyphNormals[0]=Vector(-0.577f,-0.577f, 0.577f);
					glyphNormals[1]=Vector( 0.707f, 0.000f, 0.707f);
					glyphNormals[2]=Vector( 0.000f,-0.707f, 0.707f);
					glyphNormals[3]=Vector( 0.707f, 0.000f, 0.707f);
					glyphNormals[4]=Vector( 0.000f, 0.707f, 0.707f);
					glyphNormals[5]=Vector( 0.707f, 0.000f, 0.707f);
					glyphNormals[6]=Vector(-0.577f, 0.577f, 0.577f);
					glyphVertices[ 4][0]-=arrowSize*2.0f+arrowBevelSize;
					glyphVertices[ 5][0]+=arrowBevelSize;
					glyphVertices[ 5][1]-=arrowSize*2.0f+arrowBevelSize*2.0f;
					glyphVertices[ 6][0]+=arrowBevelSize;
					glyphVertices[ 6][1]-=arrowSize+arrowBevelSize;
					glyphVertices[ 7][0]+=arrowSize*2.0f+arrowBevelSize;
					glyphVertices[ 7][1]-=arrowSize+arrowBevelSize;
					glyphVertices[ 8][0]+=arrowSize*2.0f+arrowBevelSize;
					glyphVertices[ 8][1]+=arrowSize+arrowBevelSize;
					glyphVertices[ 9][0]+=arrowBevelSize;
					glyphVertices[ 9][1]+=arrowSize+arrowBevelSize;
					glyphVertices[10][0]+=arrowBevelSize;
					glyphVertices[10][1]+=arrowSize*2.0f+arrowBevelSize*2.0f;
					glyphVertices[11][0]-=arrowSize*2.0f;
					glyphVertices[12][1]-=arrowSize*2.0f;
					glyphVertices[13][1]-=arrowSize;
					glyphVertices[14][0]+=arrowSize*2.0f;
					glyphVertices[14][1]-=arrowSize;
					glyphVertices[15][0]+=arrowSize*2.0f;
					glyphVertices[15][1]+=arrowSize;
					glyphVertices[16][1]+=arrowSize;
					glyphVertices[17][1]+=arrowSize*2.0f;
					break;
				
				case UP:
					glyphNormals[0]=Vector(-0.577f, 0.577f, 0.577f);
					glyphNormals[1]=Vector( 0.000f,-0.707f, 0.707f);
					glyphNormals[2]=Vector(-0.707f, 0.000f, 0.707f);
					glyphNormals[3]=Vector( 0.000f,-0.707f, 0.707f);
					glyphNormals[4]=Vector( 0.707f, 0.000f, 0.707f);
					glyphNormals[5]=Vector( 0.000f,-0.707f, 0.707f);
					glyphNormals[6]=Vector( 0.577f, 0.577f, 0.577f);
					glyphVertices[ 4][1]+=arrowSize*2.0f+arrowBevelSize;
					glyphVertices[ 5][0]-=arrowSize*2.0f+arrowBevelSize*2.0f;
					glyphVertices[ 5][1]-=arrowBevelSize;
					glyphVertices[ 6][0]-=arrowSize+arrowBevelSize;
					glyphVertices[ 6][1]-=arrowBevelSize;
					glyphVertices[ 7][0]-=arrowSize+arrowBevelSize;
					glyphVertices[ 7][1]-=arrowSize*2.0f+arrowBevelSize;
					glyphVertices[ 8][0]+=arrowSize+arrowBevelSize;
					glyphVertices[ 8][1]-=arrowSize*2.0f+arrowBevelSize;
					glyphVertices[ 9][0]+=arrowSize+arrowBevelSize;
					glyphVertices[ 9][1]-=arrowBevelSize;
					glyphVertices[10][0]+=arrowSize*2.0f+arrowBevelSize*2.0f;
					glyphVertices[10][1]-=arrowBevelSize;
					glyphVertices[11][1]+=arrowSize*2.0f;
					glyphVertices[12][0]-=arrowSize*2.0f;
					glyphVertices[13][0]-=arrowSize;
					glyphVertices[14][0]-=arrowSize;
					glyphVertices[14][1]-=arrowSize*2.0f;
					glyphVertices[15][0]+=arrowSize;
					glyphVertices[15][1]-=arrowSize*2.0f;
					glyphVertices[16][0]+=arrowSize;
					glyphVertices[17][0]+=arrowSize*2.0f;
					break;
				
				case RIGHT:
					glyphNormals[0]=Vector( 0.577f, 0.577f, 0.577f);
					glyphNormals[1]=Vector(-0.707f, 0.000f, 0.707f);
					glyphNormals[2]=Vector( 0.000f, 0.707f, 0.707f);
					glyphNormals[3]=Vector(-0.707f, 0.000f, 0.707f);
					glyphNormals[4]=Vector( 0.000f,-0.707f, 0.707f);
					glyphNormals[5]=Vector(-0.707f, 0.000f, 0.707f);
					glyphNormals[6]=Vector( 0.577f,-0.577f, 0.577f);
					glyphVertices[ 4][0]+=arrowSize*2.0f+arrowBevelSize;
					glyphVertices[ 5][0]-=arrowBevelSize;
					glyphVertices[ 5][1]+=arrowSize*2.0f+arrowBevelSize*2.0f;
					glyphVertices[ 6][0]-=arrowBevelSize;
					glyphVertices[ 6][1]+=arrowSize+arrowBevelSize;
					glyphVertices[ 7][0]-=arrowSize*2.0f+arrowBevelSize;
					glyphVertices[ 7][1]+=arrowSize+arrowBevelSize;
					glyphVertices[ 8][0]-=arrowSize*2.0f+arrowBevelSize;
					glyphVertices[ 8][1]-=arrowSize+arrowBevelSize;
					glyphVertices[ 9][0]-=arrowBevelSize;
					glyphVertices[ 9][1]-=arrowSize+arrowBevelSize;
					glyphVertices[10][0]-=arrowBevelSize;
					glyphVertices[10][1]-=arrowSize*2.0f+arrowBevelSize*2.0f;
					glyphVertices[11][0]+=arrowSize*2.0f;
					glyphVertices[12][1]+=arrowSize*2.0f;
					glyphVertices[13][1]+=arrowSize;
					glyphVertices[14][0]-=arrowSize*2.0f;
					glyphVertices[14][1]+=arrowSize;
					glyphVertices[15][0]-=arrowSize*2.0f;
					glyphVertices[15][1]-=arrowSize;
					glyphVertices[16][1]-=arrowSize;
					glyphVertices[17][1]-=arrowSize*2.0f;
					break;
				
				case DOWN:
					glyphNormals[0]=Vector( 0.577f,-0.577f, 0.577f);
					glyphNormals[1]=Vector( 0.000f, 0.707f, 0.707f);
					glyphNormals[2]=Vector( 0.707f, 0.000f, 0.707f);
					glyphNormals[3]=Vector( 0.000f, 0.707f, 0.707f);
					glyphNormals[4]=Vector(-0.707f, 0.000f, 0.707f);
					glyphNormals[5]=Vector( 0.000f, 0.707f, 0.707f);
					glyphNormals[6]=Vector(-0.577f,-0.577f, 0.577f);
					glyphVertices[ 4][1]-=arrowSize*2.0f+arrowBevelSize;
					glyphVertices[ 5][0]+=arrowSize*2.0f+arrowBevelSize*2.0f;
					glyphVertices[ 5][1]+=arrowBevelSize;
					glyphVertices[ 6][0]+=arrowSize+arrowBevelSize;
					glyphVertices[ 6][1]+=arrowBevelSize;
					glyphVertices[ 7][0]+=arrowSize+arrowBevelSize;
					glyphVertices[ 7][1]+=arrowSize*2.0f+arrowBevelSize;
					glyphVertices[ 8][0]-=arrowSize+arrowBevelSize;
					glyphVertices[ 8][1]+=arrowSize*2.0f+arrowBevelSize;
					glyphVertices[ 9][0]-=arrowSize+arrowBevelSize;
					glyphVertices[ 9][1]+=arrowBevelSize;
					glyphVertices[10][0]-=arrowSize*2.0f+arrowBevelSize*2.0f;
					glyphVertices[10][1]+=arrowBevelSize;
					glyphVertices[11][1]-=arrowSize*2.0f;
					glyphVertices[12][0]+=arrowSize*2.0f;
					glyphVertices[13][0]+=arrowSize;
					glyphVertices[14][0]+=arrowSize;
					glyphVertices[14][1]+=arrowSize*2.0f;
					glyphVertices[15][0]-=arrowSize;
					glyphVertices[15][1]+=arrowSize*2.0f;
					glyphVertices[16][0]-=arrowSize;
					glyphVertices[17][0]-=arrowSize*2.0f;
					break;
				}
			
			switch(depth)
				{
				case IN:
					for(int i=0;i<7;++i)
						{
						glyphNormals[i][0]=-glyphNormals[i][0];
						glyphNormals[i][1]=-glyphNormals[i][1];
						}
					for(int i=11;i<18;++i)
						glyphVertices[i][2]-=arrowBevelSize;
					break;
				
				case OUT:
					for(int i=11;i<18;++i)
						glyphVertices[i][2]+=arrowBevelSize;
					break;
				}
			break;
		}
	}

Arrow::Arrow(void)
	:direction(RIGHT),style(SIMPLE),depth(IN),
	 arrowSize(0.0f),arrowBevelSize(0.0f),
	 arrowBox(Box(Vector(0.0f,0.0f,0.0f),Vector(0.0f,0.0f,0.0f))),
	 glyphNormals(0),glyphVertices(0)
	{
	/* Allocate the glyph arrays: */
	glyphNormals=new Vector[3];
	glyphVertices=new Vector[10];
	}

Arrow::Arrow(Arrow::Direction sDirection,Arrow::Style sStyle,Arrow::Depth sDepth)
	:direction(sDirection),style(sStyle),depth(sDepth),
	 arrowSize(0.0f),arrowBevelSize(0.0f),
	 arrowBox(Box(Vector(0.0f,0.0f,0.0f),Vector(0.0f,0.0f,0.0f))),
	 glyphNormals(0),glyphVertices(0)
	{
	/* Allocate the glyph arrays: */
	switch(style)
		{
		case SIMPLE:
			glyphNormals=new Vector[3];
			glyphVertices=new Vector[10];
			break;
		
		case FANCY:
			glyphNormals=new Vector[7];
			glyphVertices=new Vector[18];
			break;
		}
	}

Arrow::~Arrow(void)
	{
	delete[] glyphNormals;
	delete[] glyphVertices;
	}

GLfloat Arrow::getPreferredBoxSize(void) const
	{
	switch(style)
		{
		case SIMPLE:
			return (arrowSize+arrowBevelSize)*2.0f;
			break;
		
		case FANCY:
			return (arrowSize+arrowBevelSize)*4.0f;
			break;
		}
	
	return 0.0f; // Just to make compiler happy
	}

ZRange Arrow::calcZRange(void) const
	{
	ZRange result=ZRange(arrowBox.origin[2],arrowBox.origin[2]);
	if(depth==IN)
		result.first-=arrowBevelSize;
	else
		result.second+=arrowBevelSize;
	return result;
	}

void Arrow::setDirection(Arrow::Direction newDirection)
	{
	if(direction==newDirection)
		return;
	
	/* Set the new arrow direction: */
	direction=newDirection;
	
	/* Update the arrow glyph: */
	createArrowGlyph();
	}

void Arrow::setStyle(Arrow::Style newStyle)
	{
	if(style==newStyle)
		return;
	
	/* Set the new style: */
	style=newStyle;
	
	/* Allocate the new glyph arrays: */
	delete[] glyphNormals;
	delete[] glyphVertices;
	switch(style)
		{
		case SIMPLE:
			glyphNormals=new Vector[3];
			glyphVertices=new Vector[10];
			break;
		
		case FANCY:
			glyphNormals=new Vector[7];
			glyphVertices=new Vector[18];
			break;
		}
	
	/* Update the arrow glyph: */
	createArrowGlyph();
	}

void Arrow::setDepth(Arrow::Depth newDepth)
	{
	if(depth==newDepth)
		return;
	
	/* Set the new depth: */
	depth=newDepth;
	
	/* Update the arrow glyph: */
	createArrowGlyph();
	}

void Arrow::setArrowSize(GLfloat newArrowSize)
	{
	/* Set the new arrow size: */
	arrowSize=newArrowSize;
	
	/* Update the arrow glyph: */
	createArrowGlyph();
	}

void Arrow::setArrowBevelSize(GLfloat newArrowBevelSize)
	{
	/* Set the new arrow bevel size: */
	arrowBevelSize=newArrowBevelSize;
	
	/* Update the arrow glyph: */
	createArrowGlyph();
	}

void Arrow::setArrowBox(const Box& newArrowBox)
	{
	/* Set the new arrow box: */
	arrowBox=newArrowBox;
	
	/* Update the arrow glyph: */
	createArrowGlyph();
	}

void Arrow::setArrowColor(const Color& newArrowColor)
	{
	/* Set the new arrow color: */
	arrowColor=newArrowColor;
	}

void Arrow::draw(GLContextData&) const
	{
	switch(style)
		{
		case SIMPLE:
			/* Draw the margin around the arrow: */
			glBegin(GL_TRIANGLE_FAN);
			glNormal3f(0.0f,0.0f,1.0f);
			glVertex(glyphVertices[4]);
			glVertex(glyphVertices[6]);
			glVertex(glyphVertices[3]);
			glVertex(glyphVertices[0]);
			glVertex(glyphVertices[1]);
			glVertex(glyphVertices[2]);
			glVertex(glyphVertices[5]);
			glEnd();
			glBegin(GL_QUADS);
			glVertex(glyphVertices[6]);
			glVertex(glyphVertices[5]);
			glVertex(glyphVertices[2]);
			glVertex(glyphVertices[3]);
			glEnd();
			
			/* Draw the arrow bevel: */
			glBegin(GL_QUADS);
			glColor(arrowColor);
			glNormal(glyphNormals[0]);
			glVertex(glyphVertices[4]);
			glVertex(glyphVertices[5]);
			glVertex(glyphVertices[8]);
			glVertex(glyphVertices[7]);
			glNormal(glyphNormals[1]);
			glVertex(glyphVertices[5]);
			glVertex(glyphVertices[6]);
			glVertex(glyphVertices[9]);
			glVertex(glyphVertices[8]);
			glNormal(glyphNormals[2]);
			glVertex(glyphVertices[6]);
			glVertex(glyphVertices[4]);
			glVertex(glyphVertices[7]);
			glVertex(glyphVertices[9]);
			glEnd();
			
			/* Draw the arrow face: */
			glBegin(GL_TRIANGLES);
			glNormal3f(0.0f,0.0f,1.0f);
			glVertex(glyphVertices[7]);
			glVertex(glyphVertices[8]);
			glVertex(glyphVertices[9]);
			glEnd();
			break;
		
		case FANCY:
			{
			/* Draw the margin around the arrow: */
			glBegin(GL_TRIANGLE_FAN);
			glNormal3f(0.0f,0.0f,1.0f);
			glVertex(glyphVertices[ 4]);
			glVertex(glyphVertices[10]);
			glVertex(glyphVertices[ 0]);
			glVertex(glyphVertices[ 1]);
			glVertex(glyphVertices[ 5]);
			glEnd();
			glBegin(GL_TRIANGLE_FAN);
			glVertex(glyphVertices[ 2]);
			glVertex(glyphVertices[ 8]);
			glVertex(glyphVertices[ 7]);
			glVertex(glyphVertices[ 6]);
			glVertex(glyphVertices[ 5]);
			glVertex(glyphVertices[ 1]);
			glEnd();
			glBegin(GL_TRIANGLE_FAN);
			glVertex(glyphVertices[ 3]);
			glVertex(glyphVertices[ 0]);
			glVertex(glyphVertices[10]);
			glVertex(glyphVertices[ 9]);
			glVertex(glyphVertices[ 8]);
			glVertex(glyphVertices[ 2]);
			glEnd();
			
			/* Draw the arrow bevel: */
			glBegin(GL_QUADS);
			glColor(arrowColor);
			int i1=10;
			for(int i2=4;i2<11;++i2)
				{
				glNormal(glyphNormals[i1-4]);
				glVertex(glyphVertices[i1]);
				glVertex(glyphVertices[i2]);
				glVertex(glyphVertices[i2+7]);
				glVertex(glyphVertices[i1+7]);
				i1=i2;
				}
			glEnd();
			
			/* Draw the arrow face: */
			glBegin(GL_TRIANGLE_FAN);
			glNormal3f(0.0f,0.0f,1.0f);
			for(int i=11;i<18;++i)
				glVertex(glyphVertices[i]);
			glEnd();
			break;
			}
		}
	}

}
