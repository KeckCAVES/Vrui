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

#include <Vrui/Tools/ScriptExecutorTool.h>

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/MessageLogger.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>
#include <Vrui/OpenFile.h>

namespace Vrui {

/*********************************************************
Methods of class ScriptExecutorToolFactory::Configuration:
*********************************************************/

ScriptExecutorToolFactory::Configuration::Configuration(void)
	{
	}

void ScriptExecutorToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	executablePathName=cfs.retrieveString("./executablePathName",executablePathName);
	arguments=cfs.retrieveValue<std::vector<std::string> >("./arguments",arguments);
	}

void ScriptExecutorToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeString("./executablePathName",executablePathName);
	cfs.storeValue<std::vector<std::string> >("./arguments",arguments);
	}

/******************************************
Methods of class ScriptExecutorToolFactory:
******************************************/

ScriptExecutorToolFactory::ScriptExecutorToolFactory(ToolManager& toolManager)
	:ToolFactory("ScriptExecutorTool",toolManager),
	 scriptSelectionHelper(Vrui::getWidgetManager(),"",".sh",Vrui::openDirectory("."))
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("UtilityTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	ScriptExecutorTool::factory=this;
	}

ScriptExecutorToolFactory::~ScriptExecutorToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	ScriptExecutorTool::factory=0;
	}

const char* ScriptExecutorToolFactory::getName(void) const
	{
	return "Script Executor";
	}

const char* ScriptExecutorToolFactory::getButtonFunction(int) const
	{
	return "Execute Script";
	}

Tool* ScriptExecutorToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new ScriptExecutorTool(this,inputAssignment);
	}

void ScriptExecutorToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveScriptExecutorToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("UtilityTool");
	}

extern "C" ToolFactory* createScriptExecutorToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	ScriptExecutorToolFactory* scriptExecutorToolFactory=new ScriptExecutorToolFactory(*toolManager);
	
	/* Return factory object: */
	return scriptExecutorToolFactory;
	}

extern "C" void destroyScriptExecutorToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/*******************************************
Static elements of class ScriptExecutorTool:
*******************************************/

ScriptExecutorToolFactory* ScriptExecutorTool::factory=0;

/***********************************
Methods of class ScriptExecutorTool:
***********************************/

void ScriptExecutorTool::selectScriptCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	/* Copy the fully-qualified script path name: */
	configuration.executablePathName=cbData->getSelectedPath();
	}

ScriptExecutorTool::ScriptExecutorTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:UtilityTool(factory,inputAssignment),
	 configuration(ScriptExecutorTool::factory->configuration),
	 childProcessId(0)
	{
	}

void ScriptExecutorTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override private configuration data from given configuration file section: */
	configuration.read(configFileSection);
	}

void ScriptExecutorTool::initialize(void)
	{
	/* Bring up a file selection dialog if there is no pre-configured script: */
	if(configuration.executablePathName.empty())
		{
		/* Select a script: */
		factory->scriptSelectionHelper.loadFile("Select Script...",this,&ScriptExecutorTool::selectScriptCallback);
		}
	}

void ScriptExecutorTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write private configuration data to given configuration file section: */
	configuration.write(configFileSection);
	}

const ToolFactory* ScriptExecutorTool::getFactory(void) const
	{
	return factory;
	}

void ScriptExecutorTool::frame(void)
	{
	if(childProcessId!=0)
		{
		/* Check if the current child terminated: */
		int status=0;
		pid_t waitResult=waitpid(childProcessId,&status,WNOHANG);
		if(waitResult>0)
			{
			/* Check the child's exit status: */
			if(WIFEXITED(status)) // Child exited normally with an exit code
				{
				/* Check the exit code: */
				int exitCode=WEXITSTATUS(status);
				if(exitCode!=0)
					Misc::formattedUserWarning("ScriptExecutorTool: Script %s returned with exit code %d.",configuration.executablePathName.c_str(),exitCode);
				}
			else if(WIFSIGNALED(status)) // Child terminated with a signal
				{
				bool childDumpedCore=false;
				#ifdef WCOREDUMP
				childDumpedCore=WCOREDUMP(status);
				#endif
				if(childDumpedCore)
					Misc::formattedUserError("ScriptExecutorTool: Script %s terminated due to signal %d and dumped core.",configuration.executablePathName.c_str(),WTERMSIG(status));
				else
					Misc::formattedUserError("ScriptExecutorTool: Script %s terminated due to signal %d.",configuration.executablePathName.c_str(),WTERMSIG(status));
				}
			
			childProcessId=0;
			}
		}
	}

void ScriptExecutorTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState)
		{
		if(childProcessId==0)
			{
			/* Fork to execute the configured script: */
			childProcessId=fork();
			if(childProcessId<0) // Error during fork
				{
				/* Show an error message: */
				int error=errno;
				Misc::formattedUserError("ScriptExecutorTool: Error %d (%s) during fork.",error,strerror(error));
				childProcessId=0;
				}
			else if(childProcessId==0) // This is the child process
				{
				/* Create script command line: */
				char* scriptArgv[42]; // Command name, up to 40 arguments, NULL pointer
				int scriptArgc=0;
				scriptArgv[scriptArgc++]=const_cast<char*>(configuration.executablePathName.c_str());
				for(std::vector<std::string>::const_iterator aIt=configuration.arguments.begin();aIt!=configuration.arguments.end()&&scriptArgc<=40;++aIt)
					scriptArgv[scriptArgc++]=const_cast<char*>(aIt->c_str());
				scriptArgv[scriptArgc]=0;
				
				/* Execute the script: */
				execvp(scriptArgv[0],scriptArgv);
				}
			}
		else
			{
			/* Show a warning: */
			Misc::formattedUserWarning("ScriptExecutorTool: Script %s is still running. Please try again later.",configuration.executablePathName.c_str());
			}
		}
	}

}
