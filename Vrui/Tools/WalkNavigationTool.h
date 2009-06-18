/***********************************************************************
WalkNavigationTool - Class to navigate in a VR environment by walking
around a fixed center position.
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

#ifndef VRUI_WALKNAVIGATIONTOOL_INCLUDED
#define VRUI_WALKNAVIGATIONTOOL_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/Plane.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <Vrui/Vrui.h>
#include <Vrui/Tools/NavigationTool.h>

/* Forward declarations: */
class GLContextData;

namespace Vrui {

class WalkNavigationTool;

class WalkNavigationToolFactory:public ToolFactory,public GLObject
	{
	friend class WalkNavigationTool;
	
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint modelListId; // Display list ID to render movement circles
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	private:
	Plane floorPlane; // Plane equation of the VR environment's floor (normal vector points "up")
	bool centerOnActivation; // Flag if to center navigation on the head position when the tool is activated
	Point centerPoint; // Center point of movement circles on floor
	Scalar moveSpeed; // Maximum movement speed
	Scalar innerRadius; // Radius of circle of no motion around center point
	Scalar outerRadius; // Radius where maximum movement speed is reached
	Vector centerViewDirection; // Central view direction
	Scalar rotateSpeed; // Maximum rotation speed in radians per second
	Scalar innerAngle; // Angle of no rotation around central view direction
	Scalar outerAngle; // Angle where maximum rotation speed is reached
	bool drawMovementCircles; // Flag whether to draw the movement circles
	Vrui::Color movementCircleColor; // Color for drawing movement circles
	
	/* Constructors and destructors: */
	public:
	WalkNavigationToolFactory(ToolManager& toolManager);
	virtual ~WalkNavigationToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

class WalkNavigationTool:public NavigationTool
	{
	friend class WalkNavigationToolFactory;
	
	/* Elements: */
	private:
	static WalkNavigationToolFactory* factory; // Pointer to the factory object for this class
	
	/* Transient navigation state: */
	Point centerPoint; // Center point of movement circle while the navigation tool is active
	NavTransform preScale; // Previous navigation transformation
	Vector translation; // Total accumulated translation
	Scalar rotation; // Total accumulated rotation around up axis
	
	/* Constructors and destructors: */
	public:
	WalkNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods from Tool: */
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int buttonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

}

#endif
