/***********************************************************************
Environment-dependent part of Vrui virtual reality development toolkit.
Copyright (c) 2000-2011 Oliver Kreylos

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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <Misc/ThrowStdErr.h>
#include <Misc/FdSet.h>
#include <Misc/File.h>
#include <Misc/Timer.h>
#include <Misc/StringMarshaller.h>
#include <Misc/GetCurrentDirectory.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/TimerEventScheduler.h>
#include <Threads/Thread.h>
#include <Threads/Mutex.h>
#include <Threads/Barrier.h>
#include <Cluster/Multiplexer.h>
#include <Cluster/MulticastPipe.h>
#include <Math/Constants.h>
#include <Geometry/Point.h>
#include <Geometry/Plane.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/AffineTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/GLValueCoders.h>
#include <GL/GLContextData.h>
#include <X11/keysym.h>
#include <GLMotif/Event.h>
#include <GLMotif/Popup.h>
#include <AL/Config.h>
#include <AL/ALContextData.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/Internal/InputDeviceAdapterMouse.h>
#include <Vrui/CoordinateManager.h>
#include <Vrui/VRWindow.h>
#include <Vrui/SoundContext.h>
#include <Vrui/ToolManager.h>
#include <Vrui/VisletManager.h>
#include <Vrui/ViewSpecification.h>

#include <Vrui/Internal/Vrui.h>

namespace Vrui {

namespace {

/***********************************
Workbench-specific global variables:
***********************************/

int vruiEventPipe[2]={-1,-1};
Threads::Mutex vruiEventPipeMutex;
volatile unsigned int vruiNumSignaledEvents=0;
Misc::ConfigurationFile* vruiConfigFile=0;
char* vruiApplicationName=0;
int vruiNumWindows=0;
VRWindow** vruiWindows=0;
bool vruiWindowsMultithreaded=false;
Threads::Thread* vruiRenderingThreads=0;
Threads::Barrier vruiRenderingBarrier;
int vruiNumSoundContexts=0;
SoundContext** vruiSoundContexts=0;
Cluster::Multiplexer* vruiMultiplexer=0;
Cluster::MulticastPipe* vruiPipe=0;
int vruiNumSlaves=0;
pid_t* vruiSlavePids=0;
int vruiSlaveArgc=0;
char** vruiSlaveArgv=0;
volatile bool vruiAsynchronousShutdown=false;

/*****************************************
Workbench-specific private Vrui functions:
*****************************************/

/* Signal handler to shut down Vrui if something goes wrong: */
void vruiTerminate(int)
	{
	/* Request an asynchronous shutdown: */
	vruiAsynchronousShutdown=true;
	}

/* Generic cleanup function called in case of an error: */
void vruiErrorShutdown(bool signalError)
	{
	if(signalError)
		{
		if(vruiMultiplexer!=0)
			{
			/* Signal a fatal error to all nodes and let them die: */
			//vruiMultiplexer->fatalError();
			}
		
		/* Return with an error condition: */
		exit(1);
		}
	
	/* Clean up: */
	vruiState->finishMainLoop();
	GLContextData::shutdownThingManager();
	if(vruiRenderingThreads!=0)
		{
		/* Cancel all rendering threads: */
		for(int i=0;i<vruiNumWindows;++i)
			{
			vruiRenderingThreads[i].cancel();
			vruiRenderingThreads[i].join();
			}
		delete[] vruiRenderingThreads;
		}
	if(vruiWindows!=0)
		{
		/* Delete all windows: */
		for(int i=0;i<vruiNumWindows;++i)
			delete vruiWindows[i];
		delete[] vruiWindows;
		}
	ALContextData::shutdownThingManager();
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	if(vruiSoundContexts!=0)
		{
		/* Destroy all sound contexts: */
		for(int i=0;i<vruiNumSoundContexts;++i)
			delete vruiSoundContexts[i];
		delete[] vruiSoundContexts;
		}
	#endif
	delete[] vruiApplicationName;
	delete vruiState;
	
	if(vruiMultiplexer!=0)
		{
		bool master=vruiMultiplexer->isMaster();
		
		/* Destroy the multiplexer: */
		delete vruiPipe;
		delete vruiMultiplexer;
		
		if(master&&vruiSlavePids!=0)
			{
			/* Wait for all slaves to terminate: */
			for(int i=0;i<vruiNumSlaves;++i)
				waitpid(vruiSlavePids[i],0,0);
			delete[] vruiSlavePids;
			}
		if(!master&&vruiSlaveArgv!=0)
			{
			/* Delete the slave command line: */
			for(int i=0;i<vruiSlaveArgc;++i)
				delete[] vruiSlaveArgv[i];
			delete[] vruiSlaveArgv;
			}
		}
	
	/* Close the configuration file: */
	delete vruiConfigFile;
	
	if(vruiEventPipe[0]>=0)
		{
		/* Close the event pipe: */
		close(vruiEventPipe[0]);
		close(vruiEventPipe[1]);
		}
	}

void vruiOpenConfigurationFile(const char* userConfigurationFileName)
	{
	try
		{
		/* Open the system-wide configuration file: */
		vruiConfigFile=new Misc::ConfigurationFile(SYSVRUICONFIGFILE);
		}
	catch(std::runtime_error error)
		{
		/* Bail out: */
		std::cerr<<"Caught exception "<<error.what()<<" while reading system-wide configuration file"<<std::endl;
		vruiErrorShutdown(true);
		}
	
	try
		{
		/* Merge in the user configuration file: */
		vruiConfigFile->merge(userConfigurationFileName);
		}
	catch(Misc::File::OpenError err)
		{
		/* Ignore the error and continue */
		}
	catch(std::runtime_error error)
		{
		/* Bail out on errors in user configuration file: */
		std::cerr<<"Caught exception "<<error.what()<<" while reading user configuration file"<<std::endl;
		vruiErrorShutdown(true);
		}
	}

void vruiGoToRootSection(const char*& rootSectionName)
	{
	try
		{
		/* Fall back to simulator mode if the root section does not exist: */
		bool rootSectionFound=false;
		if(rootSectionName!=0)
			{
			Misc::ConfigurationFile::SectionIterator rootIt=vruiConfigFile->getRootSection().getSection("/Vrui");
			for(Misc::ConfigurationFile::SectionIterator sIt=rootIt.beginSubsections();sIt!=rootIt.endSubsections();++sIt)
				if(sIt.getName()==rootSectionName)
					{
					rootSectionFound=true;
					break;
					}
			}
		if(!rootSectionFound)
			rootSectionName=VRUIDEFAULTROOTSECTIONNAME;
		}
	catch(...)
		{
		/* Bail out if configuration file does not contain the Vrui section: */
		std::cerr<<"Configuration file does not contain /Vrui section"<<std::endl;
		vruiErrorShutdown(true);
		}
	
	/* Go to the given root section: */
	vruiConfigFile->setCurrentSection("/Vrui");
	vruiConfigFile->setCurrentSection(rootSectionName);
	}

struct VruiRenderingThreadArg
	{
	/* Elements: */
	public:
	int windowIndex; // Index of the window in the Vrui window array
	Misc::ConfigurationFileSection windowConfigFileSection; // Configuration file section for the window
	InputDeviceAdapterMouse* mouseAdapter; // Pointer to the mouse input device adapter
	};

void* vruiRenderingThreadFunction(VruiRenderingThreadArg threadArg)
	{
	Threads::Thread::setCancelState(Threads::Thread::CANCEL_ENABLE);
	// Threads::Thread::setCancelType(Threads::Thread::CANCEL_ASYNCHRONOUS);
	
	/* Get this thread's parameters: */
	int windowIndex=threadArg.windowIndex;
	
	/* Create this thread's rendering window: */
	try
		{
		char windowName[256];
		if(vruiNumWindows>1)
			snprintf(windowName,sizeof(windowName),"%s%d",vruiApplicationName,windowIndex);
		else
			snprintf(windowName,sizeof(windowName),"%s",vruiApplicationName);
		vruiWindows[windowIndex]=new VRWindow(windowName,threadArg.windowConfigFileSection,vruiState,threadArg.mouseAdapter);
		vruiWindows[windowIndex]->getCloseCallbacks().add(vruiState,&VruiState::quitCallback);
		}
	catch(std::runtime_error error)
		{
		std::cerr<<"Caught exception "<<error.what()<<" while initializing rendering window "<<windowIndex<<std::endl;
		delete vruiWindows[windowIndex];
		vruiWindows[windowIndex]=0;
		}
	VRWindow* window=vruiWindows[windowIndex];
	
	/* Initialize all GLObjects for this window's context data: */
	window->makeCurrent();
	window->getContextData().updateThings();
	
	/* Synchronize with the other rendering threads: */
	vruiRenderingBarrier.synchronize();
	
	/* Terminate early if there was a problem creating the rendering window: */
	if(window==0)
		return 0;
	
	/* Enter the rendering loop and redraw the window until interrupted: */
	while(true)
		{
		/* Wait for the start of the rendering cycle: */
		vruiRenderingBarrier.synchronize();
		
		/* Draw the window's contents: */
		window->draw();
		
		/* Wait until all threads are done rendering: */
		glFinish();
		vruiRenderingBarrier.synchronize();
		
		if(vruiState->multiplexer)
			{
			/* Wait until all other nodes are done rendering: */
			vruiRenderingBarrier.synchronize();
			}
		
		/* Swap buffers: */
		window->swapBuffers();
		}
	
	return 0;
	}

}

/**********************************
Call-in functions for user program:
**********************************/

void init(int& argc,char**& argv,char**&)
	{
	/* Determine whether this node is the master or a slave: */
	if(argc==8&&strcmp(argv[1],"-vruiMultipipeSlave")==0)
		{
		/********************
		This is a slave node:
		********************/
		
		/* Read multipipe settings from the command line: */
		int numSlaves=atoi(argv[2]);
		int nodeIndex=atoi(argv[3]);
		char* master=argv[4];
		int masterPort=atoi(argv[5]);
		char* multicastGroup=argv[6];
		int multicastPort=atoi(argv[7]);
		
		/* Connect back to the master: */
		try
			{
			/* Create the multicast multiplexer: */
			vruiMultiplexer=new Cluster::Multiplexer(numSlaves,nodeIndex,master,masterPort,multicastGroup,multicastPort);
			
			/* Wait until the entire cluster is connected: */
			vruiMultiplexer->waitForConnection();
			
			/* Open a multicast pipe: */
			vruiPipe=new Cluster::MulticastPipe(vruiMultiplexer);
			
			/* Read the entire configuration file and the root section name: */
			vruiConfigFile=new Misc::ConfigurationFile(*vruiPipe);
			char* rootSectionName=Misc::readCString(*vruiPipe);
			
			/* Go to the given root section: */
			vruiConfigFile->setCurrentSection("/Vrui");
			vruiConfigFile->setCurrentSection(rootSectionName);
			delete[] rootSectionName;
			
			/* Read the application's command line: */
			vruiSlaveArgc=vruiPipe->read<int>();
			vruiSlaveArgv=new char*[vruiSlaveArgc+1];
			for(int i=0;i<=vruiSlaveArgc;++i)
				vruiSlaveArgv[i]=0;
			for(int i=0;i<vruiSlaveArgc;++i)
				vruiSlaveArgv[i]=Misc::readCString(*vruiPipe);
			
			/* Override the actual command line provided by the caller: */
			argc=vruiSlaveArgc;
			argv=vruiSlaveArgv;
			}
		catch(std::runtime_error error)
			{
			std::cerr<<"Node "<<nodeIndex<<": Caught exception "<<error.what()<<" while initializing cluster communication"<<std::endl;
			vruiErrorShutdown(true);
			}
		}
	else
		{
		/***********************
		This is the master node:
		***********************/
		
		/* Open the Vrui event pipe: */
		if(pipe(vruiEventPipe)!=0)
			{
			/* This is bad; need to shut down: */
			std::cerr<<"Error while opening event pipe"<<std::endl;
			vruiErrorShutdown(true);
			}
		
		/* Set both ends of the pipe to non-blocking I/O: */
		for(int i=0;i<2;++i)
			{
			long flags=fcntl(vruiEventPipe[i],F_GETFL);
			fcntl(vruiEventPipe[i],F_SETFL,flags|O_NONBLOCK);
			}
		
		/* Get the user configuration file's name: */
		const char* userConfigFileName=getenv("VRUI_CONFIGFILE");
		if(userConfigFileName==0)
			userConfigFileName="./Vrui.cfg";
		
		/* Open the global and user configuration files: */
		vruiOpenConfigurationFile(userConfigFileName);
		
		/* Get the root section name: */
		const char* rootSectionName=getenv("VRUI_ROOTSECTION");
		if(rootSectionName==0)
			rootSectionName=getenv("HOSTNAME");
		if(rootSectionName==0)
			rootSectionName=getenv("HOST");
		
		/* Apply configuration-related arguments from the command line: */
		for(int i=1;i<argc;++i)
			if(argv[i][0]=='-')
				{
				if(strcasecmp(argv[i]+1,"mergeConfig")==0)
					{
					/* Next parameter is name of another configuration file to merge: */
					if(i+1<argc)
						{
						try
							{
							/* Merge in the user configuration file: */
							vruiConfigFile->merge(argv[i+1]);
							}
						catch(std::runtime_error err)
							{
							/* Print a warning and carry on: */
							std::cerr<<"Vrui::init: Ignoring -mergeConfig argument due to "<<err.what()<<std::endl;
							}
						
						/* Remove parameters from argument list: */
						argc-=2;
						for(int j=i;j<argc;++j)
							argv[j]=argv[j+2];
						--i;
						}
					else
						{
						/* Ignore the mergeConfig parameter: */
						std::cerr<<"Vrui::init: No configuration file name given after -mergeConfig option"<<std::endl;
						--argc;
						}
					}
				else if(strcasecmp(argv[i]+1,"rootSection")==0)
					{
					/* Next parameter is name of root section to use: */
					if(i+1<argc)
						{
						/* Save root section name: */
						rootSectionName=argv[i+1];
						
						/* Remove parameters from argument list: */
						argc-=2;
						for(int j=i;j<argc;++j)
							argv[j]=argv[j+2];
						--i;
						}
					else
						{
						/* Ignore the rootSection parameter: */
						std::cerr<<"Vrui::init: No root section name given after -rootSection option"<<std::endl;
						--argc;
						}
					}
				}
		
		/* Go to the configuration's root section: */
		vruiGoToRootSection(rootSectionName);
		
		/* Check if this is a multipipe environment: */
		if(vruiConfigFile->retrieveValue<bool>("./enableMultipipe",false))
			{
			typedef std::vector<std::string> StringList;
			
			try
				{
				/* Read multipipe settings from configuration file: */
				std::string master=vruiConfigFile->retrieveString("./multipipeMaster");
				int masterPort=vruiConfigFile->retrieveValue<int>("./multipipeMasterPort",0);
				StringList slaves=vruiConfigFile->retrieveValue<StringList>("./multipipeSlaves");
				vruiNumSlaves=slaves.size();
				std::string multicastGroup=vruiConfigFile->retrieveString("./multipipeMulticastGroup");
				int multicastPort=vruiConfigFile->retrieveValue<int>("./multipipeMulticastPort");
				unsigned int multicastSendBufferSize=vruiConfigFile->retrieveValue<unsigned int>("./multipipeSendBufferSize",16);
				
				/* Create the multicast multiplexer: */
				vruiMultiplexer=new Cluster::Multiplexer(vruiNumSlaves,0,master.c_str(),masterPort,multicastGroup.c_str(),multicastPort);
				vruiMultiplexer->setSendBufferSize(multicastSendBufferSize);
				
				/* Start the multipipe slaves on all slave nodes: */
				std::string multipipeRemoteCommand=vruiConfigFile->retrieveString("./multipipeRemoteCommand","ssh");
				masterPort=vruiMultiplexer->getLocalPortNumber();
				vruiSlavePids=new pid_t[vruiNumSlaves];
				std::string cwd=Misc::getCurrentDirectory();
				size_t rcLen=cwd.length()+strlen(argv[0])+master.length()+multicastGroup.length()+512;
				char* rc=new char[rcLen];
				for(int i=0;i<vruiNumSlaves;++i)
					{
					pid_t childPid=fork();
					if(childPid==0)
						{
						/* Create a command line to run the program from the current working directory: */
						int ai=0;
						ai+=snprintf(rc+ai,rcLen-ai,"cd '%s' ;",cwd.c_str());
						ai+=snprintf(rc+ai,rcLen-ai," %s",argv[0]);
						ai+=snprintf(rc+ai,rcLen-ai," -vruiMultipipeSlave");
						ai+=snprintf(rc+ai,rcLen-ai," %d %d",vruiNumSlaves,i+1);
						ai+=snprintf(rc+ai,rcLen-ai," %s %d",master.c_str(),masterPort);
						ai+=snprintf(rc+ai,rcLen-ai," %s %d",multicastGroup.c_str(),multicastPort);
						
						/* Create command line for the ssh (or other remote login) program: */
						char* sshArgv[20];
						int sshArgc=0;
						sshArgv[sshArgc++]=const_cast<char*>(multipipeRemoteCommand.c_str());
						sshArgv[sshArgc++]=const_cast<char*>(slaves[i].c_str());
						sshArgv[sshArgc++]=rc;
						sshArgv[sshArgc]=0;
						
						/* Run the remote login program: */
						execvp(sshArgv[0],sshArgv);
						}
					else
						{
						/* Store PID of ssh process for later: */
						vruiSlavePids[i]=childPid;
						}
					}
				
				/* Clean up: */
				delete[] rc;
				
				/* Wait until the entire cluster is connected: */
				vruiMultiplexer->waitForConnection();
				
				/* Open a multicast pipe: */
				vruiPipe=new Cluster::MulticastPipe(vruiMultiplexer);
				
				/* Send the entire Vrui configuration file and the root section name across the pipe: */
				vruiConfigFile->writeToPipe(*vruiPipe);
				Misc::writeCString(rootSectionName,*vruiPipe);
				
				/* Write the application's command line: */
				vruiPipe->write<int>(argc);
				for(int i=0;i<argc;++i)
					Misc::writeCString(argv[i],*vruiPipe);
				
				/* Flush the pipe: */
				vruiPipe->flush();
				}
			catch(std::runtime_error error)
				{
				std::cerr<<"Master node: Caught exception "<<error.what()<<" while initializing cluster communication"<<std::endl;
				vruiErrorShutdown(true);
				}
			}
		}
	
	/* Initialize Vrui state object: */
	try
		{
		vruiState=new VruiState(vruiMultiplexer,vruiPipe);
		vruiState->initialize(vruiConfigFile->getCurrentSection());
		}
	catch(std::runtime_error error)
		{
		std::cerr<<"Caught exception "<<error.what()<<" while initializing Vrui state object"<<std::endl;
		vruiErrorShutdown(true);
		}
	
	/* Process additional command line arguments: */
	for(int i=1;i<argc;++i)
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"addToolClass")==0)
				{
				/* Next parameter is name of tool class to load: */
				if(i+1<argc)
					{
					/* Load the tool class: */
					vruiState->toolManager->loadClass(argv[i+1]);
					
					/* Remove parameters from argument list: */
					argc-=2;
					for(int j=i;j<argc;++j)
						argv[j]=argv[j+2];
					--i;
					break;
					}
				else
					{
					/* Ignore the addToolClass parameter: */
					std::cerr<<"Vrui::init: No tool class name given after -addToolClass option"<<std::endl;
					--argc;
					}
				}
			else if(strcasecmp(argv[i]+1,"addTool")==0)
				{
				/* Next parameter is name of tool binding configuration file section: */
				if(i+1<argc)
					{
					try
						{
						/* Load the tool: */
						vruiState->toolManager->loadToolBinding(argv[i+1]);
						}
					catch(std::runtime_error err)
						{
						/* Print a warning and carry on: */
						std::cerr<<"Vrui::init: Ignoring tool binding "<<argv[i+1]<<" due to exception "<<err.what()<<std::endl;
						}
					
					/* Remove parameters from argument list: */
					argc-=2;
					for(int j=i;j<argc;++j)
						argv[j]=argv[j+2];
					--i;
					break;
					}
				else
					{
					/* Ignore the addTool parameter: */
					std::cerr<<"Vrui::init: No tool binding section name given after -addTool option"<<std::endl;
					--argc;
					}
				}
			else if(strcasecmp(argv[i]+1,"vislet")==0)
				{
				if(i+1<argc)
					{
					/* First parameter is name of vislet class: */
					const char* className=argv[i+1];
					
					/* Find semicolon terminating vislet parameter list: */
					int argEnd;
					for(argEnd=i+2;argEnd<argc&&(argv[argEnd][0]!=';'||argv[argEnd][1]!='\0');++argEnd)
						;
					
					if(vruiState->visletManager!=0)
						{
						try
							{
							/* Initialize the vislet: */
							VisletFactory* factory=vruiState->visletManager->loadClass(className);
							vruiState->visletManager->createVislet(factory,argEnd-(i+2),argv+(i+2));
							}
						catch(std::runtime_error err)
							{
							/* Print a warning and carry on: */
							std::cerr<<"Vrui::init: Ignoring vislet of type "<<className<<" due to exception "<<err.what()<<std::endl;
							}
						}
					
					/* Remove all vislet parameters from the command line: */
					if(argEnd<argc)
						++argEnd;
					int numArgs=argEnd-i;
					argc-=numArgs;
					for(int j=i;j<argc;++j)
						argv[j]=argv[j+numArgs];
					--i;
					}
				else
					{
					/* Ignore the vislet parameter: */
					std::cerr<<"Vrui::init: No vislet class name given after -vislet option"<<std::endl;
					argc=i;
					}
				}
			else if(strcasecmp(argv[i]+1,"loadView")==0)
				{
				/* Next parameter is name of viewpoint file to load: */
				if(i+1<argc)
					{
					/* Save viewpoint file name: */
					vruiState->viewpointFileName=argv[i+1];
					
					/* Remove parameters from argument list: */
					argc-=2;
					for(int j=i;j<argc;++j)
						argv[j]=argv[j+2];
					--i;
					break;
					}
				else
					{
					/* Ignore the loadView parameter: */
					std::cerr<<"Vrui::init: No viewpoint file name given after -loadView option"<<std::endl;
					--argc;
					}
				}
			else if(strcasecmp(argv[i]+1,"setLinearUnit")==0)
				{
				/* Next two parameters are unit name and scale factor: */
				if(i+2<argc)
					{
					/* Set the linear unit: */
					getCoordinateManager()->setUnit(Geometry::LinearUnit(argv[i+1],atof(argv[i+2])));
					
					/* Remove parameters from argument list: */
					argc-=3;
					for(int j=i;j<argc;++j)
						argv[j]=argv[j+3];
					--i;
					break;
					}
				else
					{
					/* Ignore the setLinearUnit parameter: */
					std::cerr<<"Vrui::init: No unit name and scale factor given after -setLinearUnit option"<<std::endl;
					--argc;
					}
				}
			}
	
	/* Extract the application name: */
	const char* appNameStart=argv[0];
	const char* cPtr;
	for(cPtr=appNameStart;*cPtr!='\0';++cPtr)
		if(*cPtr=='/')
			appNameStart=cPtr+1;
	vruiApplicationName=new char[cPtr-appNameStart+1];
	memcpy(vruiApplicationName,appNameStart,cPtr-appNameStart);
	vruiApplicationName[cPtr-appNameStart]='\0';
	}

void startDisplay(void)
	{
	/* Wait for all nodes in the multicast group to reach this point: */
	if(vruiState->multiplexer!=0)
		vruiState->pipe->barrier();
	
	/* Find the mouse adapter listed in the input device manager (if there is one): */
	InputDeviceAdapterMouse* mouseAdapter=0;
	for(int i=0;i<vruiState->inputDeviceManager->getNumInputDeviceAdapters()&&mouseAdapter==0;++i)
		mouseAdapter=dynamic_cast<InputDeviceAdapterMouse*>(vruiState->inputDeviceManager->getInputDeviceAdapter(i));
	
	try
		{
		/* Retrieve the list of VR windows and determine whether to use one rendering thread per window: */
		typedef std::vector<std::string> StringList;
		
		StringList windowNames;
		if(vruiState->multiplexer!=0)
			{
			char windowNamesTag[40];
			snprintf(windowNamesTag,sizeof(windowNamesTag),"./node%dWindowNames",vruiState->multiplexer->getNodeIndex());
			windowNames=vruiConfigFile->retrieveValue<StringList>(windowNamesTag);
			
			char windowsMultithreadedTag[40];
			snprintf(windowsMultithreadedTag,sizeof(windowsMultithreadedTag),"./node%dWindowsMultithreaded",vruiState->multiplexer->getNodeIndex());
			vruiWindowsMultithreaded=vruiConfigFile->retrieveValue<bool>(windowsMultithreadedTag,false);
			}
		else
			{
			windowNames=vruiConfigFile->retrieveValue<StringList>("./windowNames");
			vruiWindowsMultithreaded=vruiConfigFile->retrieveValue<bool>("./windowsMultithreaded",false);
			}
		vruiNumWindows=windowNames.size();
		
		/* Ready the GLObject manager to initialize its objects per-window: */
		GLContextData::resetThingManager();
		
		/* Create all rendering windows: */
		vruiWindows=new VRWindow*[vruiNumWindows];
		for(int i=0;i<vruiNumWindows;++i)
			vruiWindows[i]=0;
		
		if(vruiWindowsMultithreaded)
			{
			/* Initialize the rendering barrier: */
			vruiRenderingBarrier.setNumSynchronizingThreads(vruiNumWindows+1);
			
			/* Create one rendering thread for each window (which will in turn create the windows themselves): */
			vruiRenderingThreads=new Threads::Thread[vruiNumWindows];
			for(int i=0;i<vruiNumWindows;++i)
				{
				VruiRenderingThreadArg ta;
				ta.windowIndex=i;
				ta.windowConfigFileSection=vruiConfigFile->getSection(windowNames[i].c_str());
				ta.mouseAdapter=mouseAdapter;
				vruiRenderingThreads[i].start(vruiRenderingThreadFunction,ta);
				}
			
			/* Wait until all threads have created their windows: */
			vruiRenderingBarrier.synchronize();
			
			/* Check if all windows have been properly created: */
			bool windowsOk=true;
			for(int i=0;i<vruiNumWindows;++i)
				if(vruiWindows[i]==0)
					windowsOk=false;
			if(!windowsOk)
				Misc::throwStdErr("Vrui::startDisplay: Could not create all rendering windows");
			}
		else
			{
			for(int i=0;i<vruiNumWindows;++i)
				{
				char windowName[256];
				if(vruiNumWindows>1)
					snprintf(windowName,sizeof(windowName),"%s%d",vruiApplicationName,i);
				else
					snprintf(windowName,sizeof(windowName),"%s",vruiApplicationName);
				vruiWindows[i]=new VRWindow(windowName,vruiConfigFile->getSection(windowNames[i].c_str()),vruiState,mouseAdapter);
				vruiWindows[i]->getCloseCallbacks().add(vruiState,&VruiState::quitCallback);
				
				/* Initialize all GLObjects for this window's context data: */
				vruiWindows[i]->makeCurrent();
				vruiWindows[i]->getContextData().updateThings();
				}
			}
		}
	catch(std::runtime_error error)
		{
		std::cerr<<"Caught exception "<<error.what()<<" while initializing rendering windows"<<std::endl;
		vruiErrorShutdown(true);
		}
	catch(...)
		{
		std::cerr<<"Caught spurious exception while initializing rendering windows"<<std::endl;
		vruiErrorShutdown(true);
		}
	}

void startSound(void)
	{
	/* Wait for all nodes in the multicast group to reach this point: */
	if(vruiState->multiplexer!=0)
		vruiState->pipe->barrier();
	
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	/* Retrieve the name of the sound context: */
	std::string soundContextName;
	if(vruiState->multiplexer!=0)
		{
		char soundContextNameTag[40];
		snprintf(soundContextNameTag,sizeof(soundContextNameTag),"./node%dSoundContextName",vruiState->multiplexer->getNodeIndex());
		soundContextName=vruiConfigFile->retrieveValue<std::string>(soundContextNameTag,"");
		}
	else
		soundContextName=vruiConfigFile->retrieveValue<std::string>("./soundContextName","");
	if(soundContextName=="")
		return;
	
	/* Ready the ALObject manager to initialize its objects per-context: */
	ALContextData::resetThingManager();
	
	try
		{
		/* Create a new sound context: */
		SoundContext* sc=new SoundContext(vruiConfigFile->getSection(soundContextName.c_str()),vruiState);
		
		/* Install the Vrui sound context: */
		vruiNumSoundContexts=1;
		vruiSoundContexts=new SoundContext*[1];
		vruiSoundContexts[0]=sc;
		
		/* Initialize all ALObjects for this sound context's context data: */
		vruiSoundContexts[0]->makeCurrent();
		vruiSoundContexts[0]->getContextData().updateThings();
		}
	catch(std::runtime_error err)
		{
		std::cerr<<"Disabling OpenAL sound due to exception "<<err.what()<<std::endl;
		if(vruiSoundContexts[0]!=0)
			{
			delete vruiSoundContexts[0];
			vruiSoundContexts[0]=0;
			}
		}
	catch(...)
		{
		std::cerr<<"Disabling OpenAL sound due to spurious exception"<<std::endl;
		if(vruiSoundContexts[0]!=0)
			{
			delete vruiSoundContexts[0];
			vruiSoundContexts[0]=0;
			}
		}
	#endif
	}

bool vruiHandleAllEvents(bool allowBlocking,bool checkStdin)
	{
	bool handledEvents=false;
	
	/* Check if there are pending events on the event pipe or any windows' X event queues: */
	Misc::FdSet readFds;
	bool mustBlock=allowBlocking;
	#ifdef DEBUG
	{
	Threads::Mutex::Lock vruiEventPipeLock(vruiEventPipeMutex);
	#endif
	if(vruiNumSignaledEvents>0&&vruiEventPipe[0]>=0)
		{
		readFds.add(vruiEventPipe[0]);
		mustBlock=false;
		}
	#ifdef DEBUG
	}
	#endif
	for(int i=0;i<vruiNumWindows;++i)
		if(XPending(vruiWindows[i]->getDisplay()))
			{
			readFds.add(ConnectionNumber(vruiWindows[i]->getDisplay()));
			mustBlock=false;
			}
	
	/* If there are no pending events, and blocking is allowed, block until something happens: */
	if(mustBlock)
		{
		/* Fill the file descriptor set to wait for events: */
		if(checkStdin)
			readFds.add(fileno(stdin)); // Return on input on stdin, as well
		if(vruiEventPipe[0]>=0)
			readFds.add(vruiEventPipe[0]);
		for(int i=0;i<vruiNumWindows;++i)
			readFds.add(ConnectionNumber(vruiWindows[i]->getDisplay()));
		
		/* Block until any events arrive: */
		if(vruiState->nextFrameTime!=0.0||vruiState->timerEventScheduler->hasPendingEvents())
			{
			/* Calculate the time interval until the next scheduled event: */
			double nextFrameTime=Math::Constants<double>::max;
			if(vruiState->timerEventScheduler->hasPendingEvents())
				nextFrameTime=vruiState->timerEventScheduler->getNextEventTime();
			if(vruiState->nextFrameTime!=0.0&&nextFrameTime>vruiState->nextFrameTime)
				nextFrameTime=vruiState->nextFrameTime;
			double dtimeout=nextFrameTime-vruiState->appTime.peekTime();
			struct timeval timeout;
			if(dtimeout>0.0)
				{
				timeout.tv_sec=long(Math::floor(dtimeout));
				timeout.tv_usec=long(Math::floor((dtimeout-double(timeout.tv_sec))*1000000.0+0.5));
				}
			else
				{
				timeout.tv_sec=0;
				timeout.tv_usec=0;
				}
			
			/* Block until the next scheduled timer event comes due: */
			if(Misc::select(&readFds,0,0,&timeout)==0)
				handledEvents=true; // Must stop waiting if a timer event is due
			}
		else
			{
			/* Block until kingdom come: */
			Misc::select(&readFds,0,0);
			}
		}
	
	/* Process any pending X events: */
	for(int i=0;i<vruiNumWindows;++i)
		if(readFds.isSet(ConnectionNumber(vruiWindows[i]->getDisplay())))
			{
			/* Process all pending events for this display connection: */
			bool isKeyRepeat=false; // Flag if the next event is a key repeat event
			do
				{
				/* Get the next event: */
				XEvent event;
				XNextEvent(vruiWindows[i]->getDisplay(),&event);
				
				/* Check for key repeat events (a KeyRelease immediately followed by a KeyPress with the same time stamp): */
				if(event.type==KeyRelease&&XPending(vruiWindows[i]->getDisplay()))
					{
					/* Check if the next event is a KeyPress with the same time stamp: */
					XEvent nextEvent;
					XPeekEvent(vruiWindows[i]->getDisplay(),&nextEvent);
					if(nextEvent.type==KeyPress&&nextEvent.xkey.time==event.xkey.time&&nextEvent.xkey.keycode==event.xkey.keycode)
						{
						/* Mark the next event as a key repeat: */
						isKeyRepeat=true;
						continue;
						}
					}
				
				/* Pass it to all windows interested in it: */
				bool finishProcessing=false;
				for(int j=0;j<vruiNumWindows;++j)
					if(vruiWindows[j]->isEventForWindow(event))
						finishProcessing=vruiWindows[j]->processEvent(event)||finishProcessing;
				handledEvents=!isKeyRepeat||finishProcessing;
				isKeyRepeat=false;
				
				/* Stop processing events if something significant happened: */
				if(finishProcessing)
					goto doneWithEvents;
				}
			while(XPending(vruiWindows[i]->getDisplay()));
			}
	doneWithEvents:
	
	/* Read pending data from stdin and exit if escape key is pressed: */
	if(checkStdin)
		{
		if(!mustBlock)
			{
			/* Check for pending key presses real quick: */
			struct timeval timeout;
			timeout.tv_sec=0;
			timeout.tv_usec=0;
			readFds.add(fileno(stdin));
			Misc::select(&readFds,0,0,&timeout);
			if(readFds.isSet(fileno(stdin))&&fgetc(stdin)==27)
				{
				/* Call the quit callback: */
				Misc::CallbackData cbData;
				vruiState->quitCallback(&cbData);
				handledEvents=true;
				}
			}
		}
	
	if(vruiEventPipe[0]>=0)
		{
		/* Flush the event pipe no matter what: */
		Threads::Mutex::Lock eventPipeLock(vruiEventPipeMutex);
		
		/* Read accumulated bytes from the event pipe (it's nonblocking): */
		char readBuffer[16]; // More than enough; there should only be one byte in the pipe
		if(read(vruiEventPipe[0],readBuffer,sizeof(readBuffer))>0)
			handledEvents=true;
		
		/* Reset the number of accumulated events: */
		vruiNumSignaledEvents=0;
		}
	
	return handledEvents;
	}

void vruiInnerLoopMultiWindow(void)
	{
	bool keepRunning=true;
	while(keepRunning)
		{
		/* Handle all events, blocking if there are none unless in continuous mode: */
		if(vruiState->updateContinuously)
			{
			/* Check for and handle events without blocking: */
			vruiHandleAllEvents(false,vruiNumWindows==0&&vruiState->master);
			}
		else
			{
			/* Wait for and process events until something actually happens: */
			while(!vruiHandleAllEvents(true,vruiNumWindows==0&&vruiState->master))
				;
			}
		
		/* Check for asynchronous shutdown: */
		keepRunning=keepRunning&&!vruiAsynchronousShutdown;
		
		/* Run a single Vrui frame: */
		if(vruiState->multiplexer!=0)
			vruiState->pipe->broadcast(keepRunning);
		if(!keepRunning)
			{
			if(vruiState->multiplexer!=0&&vruiState->master)
				vruiState->pipe->flush();
			
			/* Bail out of the inner loop: */
			break;
			}
		
		/* Update the Vrui state: */
		vruiState->update();
		
		/* Reset the AL thing manager: */
		ALContextData::resetThingManager();
		
		#if ALSUPPORT_CONFIG_HAVE_OPENAL
		/* Update all sound contexts: */
		for(int i=0;i<vruiNumSoundContexts;++i)
			vruiSoundContexts[i]->draw();
		#endif
		
		/* Reset the GL thing manager: */
		GLContextData::resetThingManager();
		
		if(vruiWindowsMultithreaded)
			{
			/* Start the rendering cycle by synchronizing with the render threads: */
			vruiRenderingBarrier.synchronize();
			
			/* Wait until all threads are done rendering: */
			vruiRenderingBarrier.synchronize();
			
			if(vruiState->multiplexer!=0)
				{
				/* Synchronize with other nodes: */
				vruiState->pipe->barrier();
				
				/* Notify the render threads to swap buffers: */
				vruiRenderingBarrier.synchronize();
				}
			}
		else
			{
			/* Update rendering: */
			for(int i=0;i<vruiNumWindows;++i)
				vruiWindows[i]->draw();
			
			if(vruiState->multiplexer!=0)
				{
				/* Synchronize with other nodes: */
				for(int i=0;i<vruiNumWindows;++i)
					{
					vruiWindows[i]->makeCurrent();
					glFinish();
					}
				vruiState->pipe->barrier();
				}
			
			/* Swap all buffers at once: */
			for(int i=0;i<vruiNumWindows;++i)
				{
				vruiWindows[i]->makeCurrent();
				vruiWindows[i]->swapBuffers();
				}
			}
		
		/* Print current frame rate on head node's console for window-less Vrui processes: */
		if(vruiNumWindows==0&&vruiState->master)
			{
			printf("Current frame rate: %8.3f fps\r",1.0/vruiState->currentFrameTime);
			fflush(stdout);
			}
		}
	}

void vruiInnerLoopSingleWindow(void)
	{
	bool keepRunning=true;
	while(true)
		{
		/* Handle all events, blocking if there are none unless in continuous mode: */
		if(vruiState->updateContinuously)
			{
			/* Check for and handle events without blocking: */
			vruiHandleAllEvents(false,false);
			}
		else
			{
			/* Wait for and process events until something actually happens: */
			while(!vruiHandleAllEvents(true,false))
				;
			}
		
		/* Check for asynchronous shutdown: */
		keepRunning=keepRunning&&!vruiAsynchronousShutdown;
		
		/* Run a single Vrui frame: */
		if(vruiState->multiplexer!=0)
			vruiState->pipe->broadcast(keepRunning);
		if(!keepRunning)
			{
			if(vruiState->multiplexer!=0&&vruiState->master)
				vruiState->pipe->flush();
			
			/* Bail out of the inner loop: */
			break;
			}
		
		/* Update the Vrui state: */
		vruiState->update();
		
		/* Reset the AL thing manager: */
		ALContextData::resetThingManager();
		
		#if ALSUPPORT_CONFIG_HAVE_OPENAL
		/* Update all sound contexts: */
		for(int i=0;i<vruiNumSoundContexts;++i)
			vruiSoundContexts[i]->draw();
		#endif
		
		/* Reset the GL thing manager: */
		GLContextData::resetThingManager();
		
		/* Update rendering: */
		vruiWindows[0]->draw();
		
		if(vruiState->multiplexer!=0)
			{
			/* Synchronize with other nodes: */
			glFinish();
			vruiState->pipe->barrier();
			}
		
		/* Swap buffer: */
		vruiWindows[0]->swapBuffers();
		}
	}

void mainLoop(void)
	{
	/* Start the display subsystem: */
	startDisplay();
	
	if(vruiState->useSound)
		{
		/* Start the sound subsystem: */
		startSound();
		}
	
	/* Wait for all nodes in the multicast group to reach this point: */
	if(vruiState->multiplexer!=0)
		vruiState->pipe->barrier();
	
	/* Prepare Vrui state for main loop: */
	vruiState->prepareMainLoop();
	
	#if 0
	/* Turn off the screen saver: */
	int screenSaverTimeout,screenSaverInterval;
	int screenSaverPreferBlanking,screenSaverAllowExposures;
	XGetScreenSaver(vruiWindow->getDisplay(),&screenSaverTimeout,&screenSaverInterval,&screenSaverPreferBlanking,&screenSaverAllowExposures);
	XSetScreenSaver(vruiWindow->getDisplay(),0,0,DefaultBlanking,DefaultExposures);
	XResetScreenSaver(vruiWindow->getDisplay());
	#endif
	
	if(vruiState->master&&vruiNumWindows==0)
		{
		/* Disable line buffering on stdin to detect key presses in the inner loop: */
		termios term;
		tcgetattr(fileno(stdin),&term);
		term.c_lflag&=~ICANON;
		tcsetattr(fileno(stdin),TCSANOW,&term);
		setbuf(stdin,0);
		
		printf("Press Esc to exit...\n");
		}
	
	/* Perform the main loop until the ESC key is hit: */
	if(vruiNumWindows!=1)
		vruiInnerLoopMultiWindow();
	else
		vruiInnerLoopSingleWindow();
	
	/* Perform first clean-up steps: */
	vruiState->finishMainLoop();
	
	/* Shut down the rendering system: */
	GLContextData::shutdownThingManager();
	if(vruiRenderingThreads!=0)
		{
		/* Cancel all rendering threads: */
		for(int i=0;i<vruiNumWindows;++i)
			{
			vruiRenderingThreads[i].cancel();
			vruiRenderingThreads[i].join();
			}
		delete[] vruiRenderingThreads;
		}
	if(vruiWindows!=0)
		{
		/* Delete all windows: */
		for(int i=0;i<vruiNumWindows;++i)
			delete vruiWindows[i];
		delete[] vruiWindows;
		}
	
	/* Shut down the sound system: */
	ALContextData::shutdownThingManager();
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	if(vruiSoundContexts!=0)
		{
		/* Destroy all sound contexts: */
		for(int i=0;i<vruiNumSoundContexts;++i)
			delete vruiSoundContexts[i];
		delete[] vruiSoundContexts;
		}
	#endif
	
	#if 0
	/* Turn the screen saver back on: */
	XSetScreenSaver(vruiWindow->getDisplay(),screenSaverTimeout,screenSaverInterval,screenSaverPreferBlanking,screenSaverAllowExposures);
	#endif
	}

void deinit(void)
	{
	/* Clean up: */
	delete[] vruiApplicationName;
	delete vruiState;
	
	if(vruiMultiplexer!=0)
		{
		bool master=vruiMultiplexer->isMaster();
		
		/* Destroy the multiplexer: */
		delete vruiPipe;
		delete vruiMultiplexer;
		
		if(master&&vruiSlavePids!=0)
			{
			/* Wait for all slaves to terminate: */
			for(int i=0;i<vruiNumSlaves;++i)
				waitpid(vruiSlavePids[i],0,0);
			delete[] vruiSlavePids;
			}
		if(!master&&vruiSlaveArgv!=0)
			{
			/* Delete the slave command line: */
			for(int i=0;i<vruiSlaveArgc;++i)
				delete[] vruiSlaveArgv[i];
			delete[] vruiSlaveArgv;
			}
		}
	
	/* Close the configuration file: */
	delete vruiConfigFile;
	
	if(vruiEventPipe[0]>=0)
		{
		/* Close the vrui event pipe: */
		close(vruiEventPipe[0]);
		close(vruiEventPipe[1]);
		}
	}

void shutdown(void)
	{
	/* Signal asynchronous shutdown if this node is the master node: */
	if(vruiState->master)
		{
		vruiAsynchronousShutdown=true;
		requestUpdate();
		}
	}

int getNumWindows(void)
	{
	return vruiNumWindows;
	}

VRWindow* getWindow(int index)
	{
	return vruiWindows[index];
	}

int getNumSoundContexts(void)
	{
	return vruiNumSoundContexts;
	}

SoundContext* getSoundContext(int index)
	{
	return vruiSoundContexts[index];
	}

ViewSpecification calcViewSpec(int windowIndex,int eyeIndex)
	{
	/* Get the view specification in physical coordinates: */
	ViewSpecification viewSpec=vruiWindows[windowIndex]->calcViewSpec(eyeIndex);
	
	if(vruiState->navigationTransformationEnabled)
		{
		/* Transform the view specification to navigation coordinates: */
		ATransform invNav=vruiState->inverseNavigationTransformation;
		Scalar invNavScale=vruiState->inverseNavigationTransformation.getScaling();
		Plane newScreenPlane=viewSpec.getScreenPlane();
		newScreenPlane.transform(invNav);
		newScreenPlane.normalize();
		viewSpec.setScreenPlane(newScreenPlane);
		Scalar newScreenSize[2];
		for(int i=0;i<2;++i)
			{
			newScreenSize[i]=viewSpec.getScreenSize(i);
			newScreenSize[i]*=invNavScale;
			}
		viewSpec.setScreenSize(newScreenSize);
		viewSpec.setEye(invNav.transform(viewSpec.getEye()));
		viewSpec.setEyeScreenDistance(viewSpec.getEyeScreenDistance()*invNavScale);
		for(int i=0;i<8;++i)
			viewSpec.setFrustumVertex(i,invNav.transform(viewSpec.getFrustumVertex(i)));
		for(int i=0;i<6;++i)
			{
			Plane newFrustumPlane=viewSpec.getFrustumPlane(i);
			newFrustumPlane.transform(invNav);
			newFrustumPlane.normalize();
			viewSpec.setFrustumPlane(i,newFrustumPlane);
			}
		}
	
	return viewSpec;
	}

void requestUpdate(void)
	{
	if(vruiState->master)
		{
		Threads::Mutex::Lock eventPipeLock(vruiEventPipeMutex);
		
		/* Send a byte to the event pipe if nothing has been written yet: */
		if(vruiNumSignaledEvents==0)
			{
			char byte=1;
			if(write(vruiEventPipe[1],&byte,sizeof(char))<0)
				{
				/* There's nothing to do! Stupid gcc! */
				}
			}
		
		/* Count the number of pending events: */
		++vruiNumSignaledEvents;
		}
	}

}
