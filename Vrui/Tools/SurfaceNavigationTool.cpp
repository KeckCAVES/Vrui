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

#include <Vrui/Tools/SurfaceNavigationTool.h>

#include <Misc/FunctionCalls.h>
#include <Geometry/Vector.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/*********************************************
Methods of class SurfaceNavigationToolFactory:
*********************************************/

SurfaceNavigationToolFactory::SurfaceNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("SurfaceNavigationTool",toolManager)
	{
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("NavigationTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	}

const char* SurfaceNavigationToolFactory::getName(void) const
	{
	return "Surface-Aligned Navigation";
	}

extern "C" ToolFactory* createSurfaceNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	SurfaceNavigationToolFactory* surfaceNavigationToolFactory=new SurfaceNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return surfaceNavigationToolFactory;
	}

extern "C" void destroySurfaceNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/**************************************
Methods of class SurfaceNavigationTool:
**************************************/

void SurfaceNavigationTool::align(NavTransform& surfaceFrame)
	{
	if(alignFunction!=0)
		{
		/* Call the alignment function: */
		(*alignFunction)(surfaceFrame);
		}
	else
		{
		/* Default behavior: Snap frame to z=0 plane and align it with identity transformation: */
		Vector translation=surfaceFrame.getTranslation();
		translation[2]=Scalar(0);
		Scalar scaling=surfaceFrame.getScaling();
		surfaceFrame=NavTransform(translation,Rotation::identity,scaling);
		}
	}

SurfaceNavigationTool::SurfaceNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(factory,inputAssignment),
	 alignFunction(0)
	{
	}

SurfaceNavigationTool::~SurfaceNavigationTool(void)
	{
	/* Delete the alignment function call object: */
	delete alignFunction;
	}

void SurfaceNavigationTool::setAlignFunction(Misc::FunctionCall<NavTransform&>* newAlignFunction)
	{
	/* Delete the current alignment function call object: */
	delete alignFunction;
	
	/* Install the new alignment function call object: */
	alignFunction=newAlignFunction;
	}

}
