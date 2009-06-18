/***********************************************************************
SurfaceNavigationTool - Base class for navigation tools that are limited
to navigate along an application-defined surface.
Copyright (c) 2009 Oliver Kreylos

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

#ifndef VRUI_SURFACENAVIGATIONTOOL_INCLUDED
#define VRUI_SURFACENAVIGATIONTOOL_INCLUDED

#include <Vrui/Tools/NavigationTool.h>

/* Forward declarations: */
namespace Misc {
template <class ParameterParam>
class FunctionCall;
}

namespace Vrui {
class ToolManager;
}

namespace Vrui {

class SurfaceNavigationToolFactory:public ToolFactory
	{
	/* Constructors and destructors: */
	public:
	SurfaceNavigationToolFactory(ToolManager& toolManager);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	};

class SurfaceNavigationTool:public NavigationTool
	{
	/* Elements: */
	private:
	Misc::FunctionCall<NavTransform&>* alignFunction; // Function call that aligns the passed local navigation frame to the application-defined surface
	
	/* Protected methods: */
	protected:
	void align(NavTransform& surfaceFrame); // Aligns the given navigation frame with an application-defined surface
	
	/* Constructors and destructors: */
	public:
	SurfaceNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~SurfaceNavigationTool(void);
	
	/* New methods: */
	void setAlignFunction(Misc::FunctionCall<NavTransform&>* newAlignFunction); // Sets a new align function; inherits function call object
	};

}

#endif
