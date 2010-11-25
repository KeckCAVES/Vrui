/***********************************************************************
QuikWriteTool - Class for tools to enter text using the stroke-based
QuikWrite user interface, developed by Ken Perlin.
Copyright (c) 2010 Oliver Kreylos

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

#ifndef VRUI_QUIKWRITETOOL_INCLUDED
#define VRUI_QUIKWRITETOOL_INCLUDED

#include <Geometry/Plane.h>
#include <Geometry/OrthonormalTransformation.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <GL/GLLabel.h>
#include <Vrui/Vrui.h>
#include <Vrui/UserInterfaceTool.h>

namespace Vrui {

class QuikWriteTool;

class QuikWriteToolFactory:public ToolFactory
	{
	friend class QuikWriteTool;
	
	/* Elements: */
	private:
	Scalar squareSize; // Size of QuikWrite square
	Scalar initialSquareDist; // Distance from 6DOF input device at which to display the QuikWrite square
	Color backgroundColor; // Background color for QuikWrite square
	Color foregroundColor; // Foreground color for QuikWrite square
	
	/* Constructors and destructors: */
	public:
	QuikWriteToolFactory(ToolManager& toolManager);
	virtual ~QuikWriteToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class QuikWriteTool:public UserInterfaceTool,public GLObject
	{
	friend class QuikWriteToolFactory;
	
	/* Embedded classes: */
	private:
	enum Alphabet // Enumerated type for QuikWrite alphabets
		{
		LOWERCASE,UPPERCASE,PUNCTUATION,NUMERIC
		};
	
	enum StrokeState // Enumerated type for states of a single stroke
		{
		REST,MINOR
		};
	
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint squareListId; // ID of display list to render QuikWrite square and special symbols
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	private:
	static QuikWriteToolFactory* factory; // Pointer to the factory object for this class
	static const char characters[4][9][9]; // QuikWrite alphabet character tables
	Point petalPos[32]; // Positions of the petal labels
	GLLabel petals[32]; // Characters currently associated with the zones of the QuikWrite square
	
	/* Transient state: */
	bool active; // Flag whether the tool is currently active
	ONTransform squareTransform; // Position and orientation of QuikWrite square in physical space while active
	Plane squarePlane; // Plane containing the QuikWrite square in physical space
	bool haveLeftRest; // Flag if the device ever left the QuikWrite square's rest zone
	Alphabet alphabet; // The currently displayed alphabet
	bool alphabetLocked; // Flag whether the current alphabet resets to lowercase after the next character
	StrokeState strokeState; // State of current stroke
	int strokeMajor,strokeMinor; // Major and minor stroke zones
	bool unconfirmed; // Flag if there are unconfirmed characters
	
	/* Private methods: */
	int getZone(bool inZone5) const; // Returns the index of the QuikWrite zone currently being pointed at
	void setAlphabet(Alphabet newAlphabet); // Selects the given alphabet
	void switchAlphabet(Alphabet newAlphabet); // Switches to the given alphabet
	void drawSquare(void) const; // Draws the QuikWrite square and special symbols
	
	/* Constructors and destructors: */
	public:
	QuikWriteTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~QuikWriteTool(void);
	
	/* Methods from Tool: */
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

}

#endif
