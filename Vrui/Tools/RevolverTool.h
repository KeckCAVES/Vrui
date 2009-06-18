/***********************************************************************
RevolverTool - Class to control multiple buttons (and tools) from a
single button using a revolver metaphor. Generalized from the rotator
tool initially developed by Braden Pellett and Jordan van Aalsburg.
Copyright (c) 2008-2009 Oliver Kreylos

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

#ifndef VRUI_REVOLVERTOOL_INCLUDED
#define VRUI_REVOLVERTOOL_INCLUDED

#include <GL/gl.h>
#include <GL/GLObject.h>

#include <Vrui/Tools/TransformTool.h>

namespace Vrui {

class RevolverTool;

class RevolverToolFactory:public ToolFactory
	{
	friend class RevolverTool;
	
	/* Elements: */
	private:
	int numButtons; // Number of buttons on the revolver tool's virtual input device
	
	/* Constructors and destructors: */
	public:
	RevolverToolFactory(ToolManager& toolManager);
	virtual ~RevolverToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class RevolverTool:public TransformTool,public GLObject
	{
	friend class RevolverToolFactory;
	
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLfloat digitHeight; // Height of digits
		GLfloat digitWidths[11]; // Widths of digits
		GLfloat spacing; // Spacing between digits
		GLuint digitListBase; // Base index of display lists to draw digits
		
		/* Constructors and destructors: */
		public:
		DataItem(GLfloat sDigitHeight);
		virtual ~DataItem(void);
		
		/* Methods: */
		void writeNumber(const Point& position,int number); // Writes a number
		};
	
	/* Elements: */
	static RevolverToolFactory* factory; // Pointer to the factory object for this class
	int mappedButtonIndex; // Index of the currently mapped button on the virtual input device
	double showNumbersTime; // Application time until which to show the virtual button numbers
	
	/* Constructors and destructors: */
	public:
	RevolverTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~RevolverTool(void);
	
	/* Methods from Tool: */
	virtual void initialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int deviceButtonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	virtual void initContext(GLContextData& contextData) const;
	};

}

#endif
