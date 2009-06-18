/***********************************************************************
ScalebarNavigationTool - Class to scale navigational coordinates using a
scale bar glyph with an associated settings dialog.
Copyright (c) 2007-2009 Oliver Kreylos

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

#ifndef VRUI_SCALEBARNAVIGATIONTOOL_INCLUDED
#define VRUI_SCALEBARNAVIGATIONTOOL_INCLUDED

#include <Geometry/OrthonormalTransformation.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <Vrui/Tools/NavigationTool.h>

namespace Vrui {

class ScalebarNavigationTool;

class ScalebarNavigationToolFactory:public ToolFactory,public GLObject
	{
	friend class ScalebarNavigationTool;
	
	/* Embedded classes: */
	private:
	enum NavUnit // Enumerated type for navigational coordinate units
		{
		MM,CM,M,KM,INCH,MILE
		};
	
	enum ScaleMode // Enumerated type for scalebar length increments
		{
		BINARY,NATURAL,DECADIC
		};
	
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint scalebarEndListId; // Display list ID for scalebar ends
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	Scalar scalebarWidth; // Width of scalebar in physical coordinates
	Scalar scalebarMaxHeight; // Maximum height of scalebar in physical coordinates
	Scalar scalebarEndWidth; // Width of scalebar ends in physical coordinates
	Scalar scalebarEndHeight; // Height of scalebar ends in physical coordinates
	NavUnit defaultNavUnit; // Default navigational coordinate unit for new scalebar navigation tools
	ScaleMode defaultScaleMode; // Default scalebar length increment mode for new scalebar navigation tools
	
	/* Constructors and destructors: */
	public:
	ScalebarNavigationToolFactory(ToolManager& toolManager);
	virtual ~ScalebarNavigationToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

class ScalebarNavigationTool:public NavigationTool
	{
	friend class ScalebarNavigationToolFactory;
	
	/* Elements: */
	private:
	static ScalebarNavigationToolFactory* factory; // Pointer to the factory object for this class
	
	/* Scalebar glyph state: */
	ScalebarNavigationToolFactory::NavUnit navUnit; // Unit of navigational coordinates
	Scalar unitFactor; // Length of one navigational unit in inches
	ScalebarNavigationToolFactory::ScaleMode scaleMode; // Scalebar length increment mode
	ONTransform scalebarTransform; // Position and orientation of scale bar in physical coordinates
	Scalar scaleFactor; // Current (raw) scale factor from navigational to physical coordinates
	Scalar scalebarLengthNav; // Current scale bar length in navigational units
	Scalar scalebarLength; // Current length of scale bar in physical coordinates
	
	/* Transient navigation state: */
	
	/* Constructors and destructors: */
	public:
	ScalebarNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods from Tool: */
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int buttonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

}

#endif
