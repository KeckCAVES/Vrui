/***********************************************************************
GenericToolFactory - Class for factories for generic user interaction
tools.
Copyright (c) 2005-2009 Oliver Kreylos

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

#ifndef VRUI_GENERICTOOLFACTORY_INCLUDED
#define VRUI_GENERICTOOLFACTORY_INCLUDED

#include <string>
#include <Vrui/Tools/Tool.h>

namespace Vrui {

template <class CreatedToolParam>
class GenericToolFactory:public ToolFactory
	{
	/* Embedded classes: */
	public:
	typedef CreatedToolParam CreatedTool; // Class of tools created by this factory
	
	/* Elements: */
	private:
	std::string displayName; // Display name for tools of this class
	
	/* Constructors and destructors: */
	public:
	GenericToolFactory(const char* sClassName,const char* sDisplayName,ToolFactory* parentClass,ToolManager& toolManager)
		:ToolFactory(sClassName,toolManager),
		 displayName(sDisplayName)
		{
		/* Add the tool factory to the class hierarchy: */
		if(parentClass!=0)
			{
			parentClass->addChildClass(this);
			addParentClass(parentClass);
			}
		
		/* Set the tool class' factory pointer: */
		CreatedTool::factory=this;
		}
	~GenericToolFactory(void)
		{
		/* Reset the tool class' factory pointer: */
		CreatedTool::factory=0;
		}
	
	/* Methods: */
	virtual const char* getName(void) const
		{
		return displayName.c_str();
		}
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const
		{
		return new CreatedTool(this,inputAssignment);
		}
	virtual void destroyTool(Tool* tool) const
		{
		delete tool;
		}
	void setNumDevices(int newNumDevices)
		{
		layout.setNumDevices(newNumDevices);
		}
	void setNumButtons(int deviceIndex,int newNumButtons)
		{
		layout.setNumButtons(deviceIndex,newNumButtons);
		}
	void setNumValuators(int deviceIndex,int newNumValuators)
		{
		layout.setNumValuators(deviceIndex,newNumValuators);
		}
	};

}

#endif
