/***********************************************************************
ViewpointFileNavigationTool - Class for tools to play back previously
saved viewpoint data files.
Copyright (c) 2007-2008 Oliver Kreylos

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

#ifndef VRUI_VIEWPOINTFILENAVIGATIONTOOL_INCLUDED
#define VRUI_VIEWPOINTFILENAVIGATIONTOOL_INCLUDED

#include <string>
#include <vector>
#include <Misc/File.h>
#include <Vrui/Tools/NavigationTool.h>

/* Forward declarations: */
namespace Vrui {
class DenseMatrix;
}

namespace Vrui {

class ViewpointFileNavigationTool;

class ViewpointFileNavigationToolFactory:public ToolFactory
	{
	friend class ViewpointFileNavigationTool;
	
	/* Embedded classes: */
	private:
	enum FileType // Enumerated type for viewpoint file types
		{
		KEYFRAMES,BEZIERCURVESEGMENTS
		};
	
	/* Elements: */
	private:
	FileType fileType; // File type of viewpoint data file
	std::string viewpointFileName; // Name of file from which viewpoint data is loaded
	bool showKeyframes; // Flag whether to render the current target keyframe during animation
	std::string pauseFileName; // Name of file from which scheduled pauses are loaded
	bool autostart; // Flag if new viewpoint file navigation tools start animation immediately
	
	/* Constructors and destructors: */
	public:
	ViewpointFileNavigationToolFactory(ToolManager& toolManager);
	virtual ~ViewpointFileNavigationToolFactory(void);
	
	/* Methods: */
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class ViewpointFileNavigationTool:public NavigationTool
	{
	friend class ViewpointFileNavigationToolFactory;
	
	/* Embedded classes: */
	private:
	struct ControlPoint // Structure defining a viewpoint in environment-independent format
		{
		/* Elements: */
		public:
		Point center;
		Scalar size;
		Vector forward,up;
		};
	
	struct SplineSegment // Structure defining a spline segment interpolating between two keyframe viewpoints
		{
		/* Elements: */
		public:
		Scalar t[2]; // Time marks for the endpoints of the spline segment
		ControlPoint p[4]; // Array of spline segment control points (Bezier polygon)
		};
	
	/* Elements: */
	private:
	static ViewpointFileNavigationToolFactory* factory; // Pointer to the factory object for this class
	std::vector<Scalar> times; // List of viewpoint times read from the viewpoint file
	std::vector<ControlPoint> viewpoints; // List of viewpoints read from the viewpoint file
	std::vector<SplineSegment> splines; // List of interpolating splines
	std::vector<Scalar> pauses; // List of scheduled pauses along the viewpoint curve
	unsigned int nextViewpointIndex; // Index of next viewpoint to be set
	Scalar startTime; // Time at which the navigation tool became active
	bool paused; // Flag if the viewpoint animation is currently paused
	Scalar pauseTime; // Time at which the animation was paused
	Scalar lastParameter; // Curve parameter in last frame
	
	/* Private methods: */
	void writeControlPoint(const ControlPoint& cp,DenseMatrix& b,int rowIndex); // Writes a control point to the spline calculation matrix
	void interpolate(const ControlPoint& p0,const ControlPoint& p1,Scalar t,ControlPoint& result); // Interpolates between two control points
	
	/* Constructors and destructors: */
	public:
	ViewpointFileNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods: */
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int buttonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

}

#endif
