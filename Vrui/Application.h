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

#ifndef VRUI_APPLICATION_INCLUDED
#define VRUI_APPLICATION_INCLUDED

#include <Vrui/ToolManager.h>

/* Forward declarations: */
namespace Misc {
class CallbackData;
}
class GLContextData;
class ALContextData;

namespace Vrui {

class Application
	{
	/* Elements: */
	private:
	bool useSound; // Flag whether the application wants to use spatial sound
	
	/* Private methods: */
	static void initSoundWrapper(ALContextData& contextData,void* userData);
	static void frameWrapper(void* userData);
	static void displayWrapper(GLContextData& contextData,void* userData);
	static void soundWrapper(ALContextData& contextData,void* userData);
	
	/* Protected methods: */
	protected:
	void enableSound(void); // Enables sound use for the application (off by default)
	
	/* Constructors and destructors: */
	public:
	Application(int& argc,char**& argv,char**& appDefaults); // Initializes Vrui environment
	virtual ~Application(void);
	
	/* Methods: */
	void run(void); // Runs Vrui main loop
	virtual void toolCreationCallback(ToolManager::ToolCreationCallbackData* cbData);
	virtual void toolDestructionCallback(ToolManager::ToolDestructionCallbackData* cbData);
	virtual void initSound(ALContextData& contextData) const; // Initializes application for sound in given OpenAL context
	virtual void frame(void); // Method called exactly once per frame
	virtual void display(GLContextData& contextData) const; // Rendering method called at least once per frame
	virtual void sound(ALContextData& contextData) const; // Sound rendering method called at least once per frame
	};

}

#endif
