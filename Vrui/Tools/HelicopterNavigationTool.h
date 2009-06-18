/***********************************************************************
HelicopterNavigationTool - Class for navigation tools using a simplified
helicopter flight model, a la Enemy Territory: Quake Wars' Anansi. Yeah,
I like that -- wanna fight about it?
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

#ifndef VRUI_HELICOPTERNAVIGATIONTOOL_INCLUDED
#define VRUI_HELICOPTERNAVIGATIONTOOL_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <Vrui/Tools/NavigationTool.h>

namespace Vrui {

class HelicopterNavigationTool;

class HelicopterNavigationToolFactory:public ToolFactory
	{
	friend class HelicopterNavigationTool;
	
	/* Elements: */
	private:
	Scalar rotateFactors[3]; // Array of rotation speeds around the (pitch, roll, yaw) axes in radians/s
	Scalar g; // Acceleration of gravity in physical coordinate units/s^2
	Scalar collectiveMin,collectiveMax; // Min and max amounts of collective acceleration in physical coordinate units/s^2
	Scalar thrust; // Thrust acceleration in physical coordinate units/s^2
	Scalar brake; // Reverse thrust acceleration in physical coordinate units/s^2
	Scalar dragCoefficients[3]; // Drag coefficients in local x, y, z directions
	Scalar viewAngleFactors[2]; // View offset angle factors for hat switch valuators in radians
	
	/* Constructors and destructors: */
	public:
	HelicopterNavigationToolFactory(ToolManager& toolManager);
	virtual ~HelicopterNavigationToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class HelicopterNavigationTool:public NavigationTool,public GLObject
	{
	friend class HelicopterNavigationToolFactory;
	
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		GLuint displayListBase; // Base index of display list to render wireframe digits
		
		/* Constructors and destructors: */
		public:
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	static HelicopterNavigationToolFactory* factory; // Pointer to the factory object for this class
	
	/* Transient navigation state: */
	bool buttons[2]; // Current status of the two control buttons (forward thrust, reverse thrust)
	Scalar valuators[6]; // Current value of the four control valuators (pitch, roll, rudder, collective) and two view valuators
	NavTransform pre,post; // Transformations recreating the navigation transformation at the time the tool was activated
	Point currentPosition; // Current position of virtual helicopter relative to initial transformation
	Rotation currentOrientation; // Current orientation of virtual helicopter relative to initial transformation
	Vector currentVelocity; // Current linear velocity of the virtual helicopter
	double lastFrameTime; // Application time of last frame
	
	/* Constructors and destructors: */
	public:
	HelicopterNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods from Tool: */
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int buttonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void valuatorCallback(int deviceIndex,int valuatorIndex,InputDevice::ValuatorCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

}

#endif
