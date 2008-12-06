/***********************************************************************
FlyNavigationTool - Class encapsulating the behaviour of the old
infamous Vrui single-handed flying navigation tool.
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

#ifndef VRUI_FLYNAVIGATIONTOOL_INCLUDED
#define VRUI_FLYNAVIGATIONTOOL_INCLUDED

#include <Vrui/Tools/NavigationTool.h>

/* Forward declarations: */
namespace Vrui {
class Viewer;
}

namespace Vrui {

class FlyNavigationTool;

class FlyNavigationToolFactory:public ToolFactory
	{
	friend class FlyNavigationTool;
	
	/* Elements: */
	private:
	Vector flyDirection; // Flying direction of tool in device coordinates
	Scalar flyFactor; // Velocity multiplication factor
	
	/* Constructors and destructors: */
	public:
	FlyNavigationToolFactory(ToolManager& toolManager);
	virtual ~FlyNavigationToolFactory(void);
	
	/* Methods: */
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class FlyNavigationTool:public NavigationTool
	{
	friend class FlyNavigationToolFactory;
	
	/* Elements: */
	private:
	static FlyNavigationToolFactory* factory; // Pointer to the factory object for this class
	const Viewer* viewer; // Viewer associated with the navigation tool
	
	/* Constructors and destructors: */
	public:
	FlyNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods: */
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int deviceIndex,int buttonIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	};

}

#endif
