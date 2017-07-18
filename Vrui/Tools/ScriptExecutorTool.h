/***********************************************************************
ScriptExecutorTool - Class for tools to execute an external program or
shell script when a button is pressed.
Copyright (c) 2016 Oliver Kreylos

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

#ifndef VRUI_SCRIPTEXECUTORTOOL_INCLUDED
#define VRUI_SCRIPTEXECUTORTOOL_INCLUDED

#include <unistd.h>
#include <string>
#include <vector>
#include <GLMotif/FileSelectionHelper.h>
#include <Vrui/UtilityTool.h>

namespace Vrui {

class ScriptExecutorTool;

class ScriptExecutorToolFactory:public ToolFactory
	{
	friend class ScriptExecutorTool;
	
	/* Embedded classes: */
	private:
	struct Configuration // Structure containing tool settings
		{
		/* Elements: */
		public:
		std::string executablePathName; // Full path and name of executable or shell script to run when button is pressed
		std::vector<std::string> arguments; // List of command line arguments to pass to the executable or shell script
		
		/* Constructors and destructors: */
		Configuration(void); // Creates default configuration
		
		/* Methods: */
		void read(const Misc::ConfigurationFileSection& cfs); // Overrides configuration from configuration file section
		void write(Misc::ConfigurationFileSection& cfs) const; // Writes configuration to configuration file section
		};
	
	/* Elements: */
	Configuration configuration; // Default configuration for all tools
	GLMotif::FileSelectionHelper scriptSelectionHelper; // Helper object to select script path names
	
	/* Constructors and destructors: */
	public:
	ScriptExecutorToolFactory(ToolManager& toolManager);
	virtual ~ScriptExecutorToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class ScriptExecutorTool:public UtilityTool
	{
	friend class ScriptExecutorToolFactory;
	
	/* Elements: */
	private:
	static ScriptExecutorToolFactory* factory; // Pointer to the factory object for this class
	ScriptExecutorToolFactory::Configuration configuration; // Private configuration of this tool
	pid_t childProcessId; // Process ID of currently-running script child process
	
	/* Private methods: */
	void selectScriptCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData); // Called when the user selects a script to execute
	
	/* Constructors and destructors: */
	public:
	ScriptExecutorTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods from Tool: */
	virtual void configure(const Misc::ConfigurationFileSection& configFileSection);
	virtual void storeState(Misc::ConfigurationFileSection& configFileSection) const;
	virtual void initialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void frame(void);
	virtual void buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData);
	};

}

#endif
