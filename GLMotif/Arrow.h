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

#ifndef GLMOTIF_ARROW_INCLUDED
#define GLMOTIF_ARROW_INCLUDED

#include <GL/gl.h>
#include <GLMotif/Types.h>

/* Forward declarations: */
class GLContextData;

namespace GLMotif {

class Arrow
	{
	/* Embedded classes: */
	public:
	enum Direction // Enumerated type for arrow directions
		{
		LEFT,UP,RIGHT,DOWN
		};
	enum Depth // Enumerated type for arrow engraving or embossing
		{
		IN,OUT
		};
	enum Style // Enumerated type for arrow styles
		{
		SIMPLE,FANCY
		};
	
	/* Elements: */
	private:
	Direction direction; // Arrow direction
	Style style; // Arrow style
	Depth depth; // Arrow depth
	GLfloat arrowSize; // Size of the arrow glyph
	GLfloat arrowBevelSize; // Size of bevel around arrow
	Box arrowBox; // Box around the arrow glyph; also defines glyph's base plane
	Color arrowColor; // Color for the arrow glyph; margin color is inherited from current state
	Vector* glyphNormals; // Array of normal vectors for the arrow glyph
	Vector* glyphVertices; // Array of vertices for the arrow glyph
	
	/* Private methods: */
	void createArrowGlyph(void); // Creates the arrow glyph
	
	/* Constructors and destructors: */
	public:
	Arrow(void); // Creates a default arrow
	Arrow(Direction sDirection,Style sStyle,Depth sDepth); // Creates an arrow of the given direction, style, and depth
	~Arrow(void); // Destroys the arrow
	
	/* Methods: */
	GLfloat getArrowSize(void) const // Returns the arrow's size
		{
		return arrowSize;
		}
	GLfloat getArrowBevelSize(void) const // Returns the arrow's bevel size
		{
		return arrowBevelSize;
		}
	const Box& getArrowBox(void) const // Returns the arrow box
		{
		return arrowBox;
		}
	const Color& getArrowColor(void) const // Returns the arrow glyph's color
		{
		return arrowColor;
		}
	GLfloat getPreferredBoxSize(void) const; // Returns the arrow's preferred box size
	ZRange calcZRange(void) const; // Returns the range of z values of the arrow
	void setDirection(Direction newDirection); // Sets the arrow's direction
	void setStyle(Style newStyle); // Sets the arrow's style
	void setDepth(Depth newDepth); // Sets the arrow's depth
	void setArrowSize(GLfloat newArrowSize); // Sets the arrow's size
	void setArrowBevelSize(GLfloat newArrowBevelSize); // Sets the arrow's bevel size
	void setArrowBox(const Box& newArrowBox); // Repositions the arrow
	void setArrowColor(const Color& newArrowColor); // Sets the arrow glyph's color
	void draw(GLContextData& contextData) const; // Draws the arrow glyph
	};

}

#endif
