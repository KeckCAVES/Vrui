/***********************************************************************
Application - Base class for Vrui application objects.
Copyright (c) 2004-2012 Oliver Kreylos

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
#include <Vrui/Tool.h>

/* Forward declarations: */
namespace Misc {
class CallbackData;
}
class GLContextData;
class ALContextData;

namespace Vrui {

class Application
	{
	/* Embedded classes: */
	public:
	class ToolBase // Base mix-in class for application tools
		{
		/* Constructors and destructors: */
		public:
		ToolBase(void)
			{
			}
		virtual ~ToolBase(void)
			{
			}
		
		/* Methods: */
		virtual void setApplication(Application* sApplication) =0; // Sets the tool's application pointer
		};
	
	template <class DerivedApplicationParam>
	class Tool:public ToolBase // Base class for tools that need to link back to the application object owning them
		{
		/* Embedded classes: */
		public:
		typedef DerivedApplicationParam DerivedApplication; // The derived application class
		
		/* Elements: */
		protected:
		DerivedApplication* application; // Pointer to the application object owning this tool
		
		/* Constructors and destructors: */
		public:
		Tool(void)
			:application(0)
			{
			}
		
		/* Methods from ToolBase: */
		virtual void setApplication(Application* sApplication)
			{
			application=dynamic_cast<DerivedApplication*>(sApplication);
			}
		};
	
	/* Private methods: */
	private:
	static void frameWrapper(void* userData);
	static void displayWrapper(GLContextData& contextData,void* userData);
	static void soundWrapper(ALContextData& contextData,void* userData);
	
	/* Constructors and destructors: */
	public:
	Application(int& argc,char**& argv,char**& appDefaults); // Initializes Vrui environment
	Application(int& argc,char**& argv); // Ditto, using a NULL appDefaults pointer
	virtual ~Application(void);
	
	/* Methods: */
	void run(void); // Runs Vrui main loop
	
	/* Methods to be overridden by derived classes to insert functionality into Vrui's main loop: */
	virtual void toolCreationCallback(ToolManager::ToolCreationCallbackData* cbData); // Called when the tool manager creates a new tool
	virtual void toolDestructionCallback(ToolManager::ToolDestructionCallbackData* cbData); // Called when the tool manager destroys a tool
	virtual void frame(void); // Synchronization method called exactly once per frame
	virtual void display(GLContextData& contextData) const; // Rendering method called at least once per window per frame, potentially concurrently from background thread(s)
	virtual void sound(ALContextData& contextData) const; // Sound rendering method called at least once per sound context per frame, potentially concurrently from background thread(s)
	};

}

#endif
