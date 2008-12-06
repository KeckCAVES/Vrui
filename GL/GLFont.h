/***********************************************************************
GLFont - Class to represent texture-based fonts and to render 3D text.
Copyright (c) 1999-2005 Oliver Kreylos

This file is part of the OpenGL Support Library (GLSupport).

The OpenGL Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenGL Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenGL Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef GLFONT_INCLUDED
#define GLFONT_INCLUDED

#include <Misc/Endianness.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLVector.h>
#include <GL/GLBox.h>
#include <GL/GLObject.h>

/* Forward declarations: */
namespace Misc {
class File;
}

class GLFont
	{
	/* Embedded classes: */
	public:
	typedef GLColor<GLfloat,4> Color; // Type for color values
	typedef GLVector<GLfloat,3> Vector; // Type for model space vectors and points
	typedef GLBox<GLfloat,3> Box; // Type for model space boxes
	typedef GLBox<GLfloat,2> TBox; // Type for texture space boxes
	
	enum HAlignment
		{
		Left,Center,Right
		};
	
	enum VAlignment
		{
		Top,VCenter,Baseline,Bottom
		};
	
	class String:public GLObject // Class to simplify caching string textures
		{
		/* Embedded classes: */
		private:
		struct DataItem:public GLObject::DataItem
			{
			/* Elements: */
			public:
			GLuint textureObjectId;
			
			/* Constructors and destructors: */
			DataItem(void)
				{
				glGenTextures(1,&textureObjectId);
				}
			~DataItem(void)
				{
				glDeleteTextures(1,&textureObjectId);
				}
			};
		
		/* Elements: */
		private:
		const GLFont* font; // Pointer to the font for this string
		char* string; // The string
		GLsizei stringWidth; // Width of string in texels
		Box box; // String's bounding box
		GLsizei textureWidth; // Width of string's texture map
		TBox texCoord; // String's texture coordinates
		Color backgroundColor; // String's background color
		Color foregroundColor; // String's foreground color
		
		/* Constructors and destructors: */
		public:
		String(const GLFont* sFont,const char* sString);
		~String(void);
		
		/* Methods: */
		void setBackgroundColor(const Color& newBackgroundColor)
			{
			backgroundColor=newBackgroundColor;
			}
		void setForegroundColor(const Color& newForegroundColor)
			{
			foregroundColor=newForegroundColor;
			}
		virtual void initContext(GLContextData& contextData) const;
		void draw(const Vector& origin,GLContextData& contextData) const; // Draws the string
		};
	
	friend class String;
	
	private:
	struct CharInfo
		{
		/* Elements: */
		public:
		/* Character box description: */
		GLshort width; // Width of character box
		GLshort ascent,descent; // Height of character box above and below baseline
		GLshort glyphOffset; // Offset and width of character glyph in box
		GLsizei rasterLineOffset; // Offset of raster line descriptors in main array
		GLsizei spanOffset; // Offset of span descriptors in main array
		
		/* Methods: */
		void read(Misc::File& file); // Reads a CharInfo structure from a font file
		};
	
	/* Elements: */
	GLint firstCharacter; // Index of first character in font
	GLsizei numCharacters; // Number of characters in font
	GLshort maxAscent,maxDescent; // Maximal height and depth of characters
	GLshort maxLeftLap,maxRightLap; // Maximal overlap of a character versus its box
	CharInfo* characters; // Array of CharInfo structures describing all characters
	GLsizei numRasterLines; // Total number of raster lines for all characters
	GLubyte* rasterLines; // Array of number of spans per raster line for all characters
	GLsizei numSpans; // Total number of spans of all raster lines and characters
	GLubyte* spans; // Array of spans
	GLint fontHeight; // Total height of the font (ascent+descent+2 border pixels)
	GLint baseLine; // Position of baseline
	GLsizei textureHeight; // Height of a texture image to hold a single line of text
	GLfloat averageWidth; // Average width of a character box
	
	/* Current font status: */
	GLfloat textHeight; // Scaled height of font
	Color backgroundColor; // Text background color
	Color foregroundColor; // Text foreground color
	HAlignment hAlignment; // Horizontal alignment
	VAlignment vAlignment; // Vertical alignment
	bool antialiasing; // Flag to enable antialiasing
	
	/* Private methods: */
	GLsizei calcStringWidth(const char* string) const; // Calculates the texel width of a string
	Box calcStringBox(GLsizei stringWidth) const; // Returns the bounding box of a string
	TBox calcStringTexCoords(GLsizei stringWidth,GLsizei textureWidth) const; // Calculates the texture coordinates needed to render a string
	void uploadStringTexture(const char* string,GLsizei stringWidth,GLsizei textureWidth) const; // Creates and uploads a texture for a string
	void uploadStringTexture(const char* string,const Color& stringBackgroundColor,const Color& stringForegroundColor,GLsizei stringWidth,GLsizei textureWidth) const; // Creates and uploads a texture for a string using the given colors
	void loadFont(Misc::File& file); // Loads font from given file
	
	/* Constructors and Destructors: */
	public:
	GLFont(const char* fontName); // Creates a GL font from a font file
	~GLFont(void);
	
	/* Methods: */
	bool isValid(void) const // Checks if the font object was created successfully
		{
		return characters!=0;
		}
	GLfloat getTextPixelHeight(void) const // Returns font's unscaled height
		{
		return GLfloat(fontHeight);
		}
	GLfloat getTextHeight(void) const // Returns the font's scaled height
		{
		return textHeight;
		}
	void setTextHeight(GLfloat newTextHeight) // Sets the font's scaled height
		{
		textHeight=newTextHeight;
		}
	GLfloat getCharacterWidth(void) const // Returns the average scaled character width
		{
		return averageWidth*textHeight;
		}
	template <class InputColorType>
	void setBackgroundColor(const InputColorType& newBackgroundColor) // Sets the text background color
		{
		backgroundColor=newBackgroundColor;
		}
	template <class InputColorType>
	void setForegroundColor(const InputColorType& newForegroundColor) // Sets the text foreground color
		{
		foregroundColor=newForegroundColor;
		}
	void setHAlignment(HAlignment newHAlignment) // Sets the font's horizontal alignment
		{
		hAlignment=newHAlignment;
		}
	void setVAlignment(VAlignment newVAlignment) // Sets the font's vertical alignment
		{
		vAlignment=newVAlignment;
		}
	void setAntialiasing(bool newAntialiasing) // Sets the antialiasing flag
		{
		antialiasing=newAntialiasing;
		}
	Box calcStringBox(const char* string) const // Returns the bounding box of a string
		{
		return calcStringBox(calcStringWidth(string));
		}
	TBox calcStringTexCoords(const char* string) const // Calculates the texture coordinates needed to render a string
		{
		GLsizei stringWidth=calcStringWidth(string);
		GLsizei textureWidth;
		for(textureWidth=1;textureWidth<stringWidth;textureWidth<<=1)
			;
		
		return calcStringTexCoords(stringWidth,textureWidth);
		}
	void uploadStringTexture(const char* string) const; // Uploads a string's texture image
	void uploadStringTexture(const char* string,const Color& stringBackgroundColor,const Color& stringForegroundColor) const; // Uploads a string's texture image with the given colors
	void drawString(const Vector& origin,const char* string) const; // Draws a simple, one-line string
	};

#endif
