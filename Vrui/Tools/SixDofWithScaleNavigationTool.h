/***********************************************************************
SixDofWithScaleNavigationTool - Class for simple 6-DOF dragging using a
single input device, with an additional input device used as a slider
for zooming.
Copyright (c) 2004-2008 Oliver Kreylos

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

#ifndef VRUI_SIXDOFWITHSCALENAVIGATIONTOOL_INCLUDED
#define VRUI_SIXDOFWITHSCALENAVIGATIONTOOL_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <Vrui/Tools/NavigationTool.h>

namespace Vrui {

class SixDofWithScaleNavigationTool;

class SixDofWithScaleNavigationToolFactory:public ToolFactory,public GLObject
	{
	friend class SixDofWithScaleNavigationTool;
	
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint modelListId; // Display list ID to render tools' models
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	Scalar scaleDeviceDistance; // Maximum distance between the two input devices for scaling mode
	Scalar scaleDeviceDistance2; // Square of above
	Vector deviceScaleDirection; // Scale direction vector in zoom device's coordinate system
	Scalar scaleFactor; // Distance the device has to be moved along the scaling line to scale by factor of e
	
	/* Constructors and destructors: */
	public:
	SixDofWithScaleNavigationToolFactory(ToolManager& toolManager);
	virtual ~SixDofWithScaleNavigationToolFactory(void);
	
	/* Methods: */
	virtual void initContext(GLContextData& contextData) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class SixDofWithScaleNavigationTool:public NavigationTool
	{
	friend class SixDofWithScaleNavigationToolFactory;
	
	enum NavigationMode // Enumerated type for states the tool can be in
		{
		IDLE,MOVING,SCALING
		};
	
	/* Elements: */
	private:
	static SixDofWithScaleNavigationToolFactory* factory; // Pointer to the factory object for this class
	
	/* Transient navigation state: */
	NavigationMode navigationMode; // The tool's current navigation mode
	NavTrackerState preScale; // Transformation to be applied to the navigation transformation before scaling
	Point scalingCenter; // Center position of scaling operation
	Scalar initialScale; // Initial distance between input devices
	NavTrackerState postScale; // Transformation to be applied to the navigation transformation after scaling
	
	/* Constructors and destructors: */
	public:
	SixDofWithScaleNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods: */
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int buttonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

}

#endif
