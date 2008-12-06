/***********************************************************************
Application - Base class for Vrui application objects.
Copyright (c) 2004-2005 Oliver Kreylos

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

#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Geometry.h>
#include <Vrui/Vrui.h>

#include <Vrui/Application.h>

namespace Vrui {

/****************************
Methods of class Application:
****************************/

void Application::initSoundWrapper(ALContextData& contextData,void* userData)
	{
	static_cast<Application*>(userData)->initSound(contextData);
	}

void Application::frameWrapper(void* userData)
	{
	static_cast<Application*>(userData)->frame();
	}

void Application::displayWrapper(GLContextData& contextData,void* userData)
	{
	static_cast<Application*>(userData)->display(contextData);
	}

void Application::soundWrapper(ALContextData& contextData,void* userData)
	{
	static_cast<Application*>(userData)->sound(contextData);
	}

void Application::enableSound(void)
	{
	#ifdef VRUI_USE_OPENAL
	useSound=true;
	#endif
	}

Application::Application(int& argc,char**& argv,char**& appDefaults)
	:useSound(false)
	{
	/* Initialize Vrui: */
	init(argc,argv,appDefaults);
	
	/* Enable navigation per default: */
	setNavigationTransformation(NavTransform::identity);
	}

Application::~Application(void)
	{
	/* Deinitialize Vrui: */
	deinit();
	}

void Application::run(void)
	{
	/* Install callbacks with the tool manager: */
	ToolManager* toolManager=getToolManager();
	toolManager->getToolCreationCallbacks().add(this,&Application::toolCreationCallback);
	toolManager->getToolDestructionCallbacks().add(this,&Application::toolDestructionCallback);
	
	/* Install Vrui callbacks: */
	setFrameFunction(frameWrapper,this);
	setDisplayFunction(displayWrapper,this);
	setSoundFunction(soundWrapper,this);
	
	/* Start the display: */
	startDisplay(0,0);
	
	/* Start the sound renderer if requested: */
	if(useSound)
		startSound(initSoundWrapper,this);
	
	/* Run the Vrui main loop: */
	mainLoop();
	
	/* Uninstall tool manager callbacks: */
	toolManager->getToolCreationCallbacks().remove(this,&Application::toolCreationCallback);
	toolManager->getToolDestructionCallbacks().remove(this,&Application::toolDestructionCallback);
	}

void Application::toolCreationCallback(ToolManager::ToolCreationCallbackData*)
	{
	}

void Application::toolDestructionCallback(ToolManager::ToolDestructionCallbackData*)
	{
	}

void Application::initSound(ALContextData&) const
	{
	}

void Application::frame(void)
	{
	}

void Application::display(GLContextData&) const
	{
	}

void Application::sound(ALContextData&) const
	{
	}

}
