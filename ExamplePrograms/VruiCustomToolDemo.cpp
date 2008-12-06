/***********************************************************************
VruiCustomToolDemo - VR application showing how to create application-
specific tools and register them with the Vrui tool manager, and how
custom tools can interact with the VR application.
Copyright (c) 2006 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <iostream>
#include <Vrui/Tools/Tool.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>

class VruiCustomToolDemo:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	class MyTool; // Forward declaration
	
	class MyToolFactory:public Vrui::ToolFactory // Class for factories that create/destroy objects of the custom tool class
		{
		friend class MyTool;
		
		/* Elements: */
		private:
		VruiCustomToolDemo* application; // Pointer to the application object that owns the custom tool factory
		
		/* Constructors and destructors: */
		public:
		MyToolFactory(Vrui::ToolManager& toolManager,VruiCustomToolDemo* sApplication);
		virtual ~MyToolFactory(void);
		
		/* Methods: */
		virtual Vrui::Tool* createTool(const Vrui::ToolInputAssignment& inputAssignment) const;
		virtual void destroyTool(Vrui::Tool* tool) const;
		};
	
	class MyTool:public Vrui::Tool // The custom tool class
		{
		friend class MyToolFactory;
		
		/* Elements: */
		private:
		static MyToolFactory* factory; // Pointer to the factory object for this class
		
		/* Constructors and destructors: */
		public:
		MyTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
		
		/* Methods: */
		virtual const Vrui::ToolFactory* getFactory(void) const;
		virtual void buttonCallback(int deviceIndex,int buttonIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
		};
	
	/* Elements: */
	MyToolFactory myFactory; // Factory object for the custom tool class
	
	/* Constructors and destructors: */
	public:
	VruiCustomToolDemo(int& argc,char**& argv,char**& appDefaults);
	
	/* Methods: */
	void selectApplicationObject(void); // Dummy method to show how custom tools can interact with the application
	};

/**************************************************
Methods of class VruiCustomToolDemo::MyToolFactory:
**************************************************/

VruiCustomToolDemo::MyToolFactory::MyToolFactory(Vrui::ToolManager& toolManager,VruiCustomToolDemo* sApplication)
	:Vrui::ToolFactory("MyTool",toolManager),
	 application(sApplication)
	{
	/* Initialize the input device layout of the custom tool class: */
	layout.setNumDevices(1); // Custom tools require one input device
	layout.setNumButtons(0,2); // Custom tools require two buttons on the input device
	
	#if 0 // Tool classes directly derived from Vrui::Tool must not do this
	/* Insert the custom tool class into the tool hierarchy: */
	Vrui::ToolFactory* toolFactory=toolManager.loadClass("Tool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	#endif
	
	/* Set the custom tool class' factory pointer: */
	MyTool::factory=this;
	}

VruiCustomToolDemo::MyToolFactory::~MyToolFactory(void)
	{
	/* Reset the custom tool class' factory pointer: */
	MyTool::factory=0;
	}

Vrui::Tool* VruiCustomToolDemo::MyToolFactory::createTool(const Vrui::ToolInputAssignment& inputAssignment) const
	{
	/* Create a new object of the custom tool class: */
	MyTool* newTool=new MyTool(this,inputAssignment);
	
	return newTool;
	}

void VruiCustomToolDemo::MyToolFactory::destroyTool(Vrui::Tool* tool) const
	{
	/* Cast the tool pointer to the custom tool class (not really necessary): */
	MyTool* myTool=dynamic_cast<MyTool*>(tool);
	
	/* Destroy the tool: */
	delete myTool;
	}

/***************************************************
Static elements of class VruiCustomToolDemo::MyTool:
***************************************************/

VruiCustomToolDemo::MyToolFactory* VruiCustomToolDemo::MyTool::factory=0;

/*******************************************
Methods of class VruiCustomToolDemo::MyTool:
*******************************************/

VruiCustomToolDemo::MyTool::MyTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::Tool(factory,inputAssignment)
	{
	}

const Vrui::ToolFactory* VruiCustomToolDemo::MyTool::getFactory(void) const
	{
	return factory;
	}

void VruiCustomToolDemo::MyTool::buttonCallback(int deviceIndex,int buttonIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		std::cout<<"MyTool: Button "<<buttonIndex<<" has just been pressed"<<std::endl;
		
		/* Call an application method if the second button was pressed: */
		if(buttonIndex==1)
			factory->application->selectApplicationObject();
		}
	else // Button has just been released
		{
		std::cout<<"MyTool: Button "<<buttonIndex<<" has just been released"<<std::endl;
		}
	}

/***********************************
Methods of class VruiCustomToolDemo:
***********************************/

VruiCustomToolDemo::VruiCustomToolDemo(int& argc,char**& argv,char**& appDefaults)
	:Vrui::Application(argc,argv,appDefaults),
	 myFactory(*Vrui::getToolManager(),this)
	{
	/* Register the custom tool class with the Vrui tool manager: */
	Vrui::getToolManager()->addClass(&myFactory,0);
	}

void VruiCustomToolDemo::selectApplicationObject(void)
	{
	std::cout<<"VruiCustomToolDemo: selectApplicationObject has just been called"<<std::endl;
	}

/*************
Main function:
*************/

int main(int argc,char* argv[])
	{
	/* Create an application object: */
	char** appDefaults=0; // This is an additional parameter no one ever uses
	VruiCustomToolDemo app(argc,argv,appDefaults);
	
	/* Run the Vrui main loop: */
	app.run();
	
	/* Exit to OS: */
	return 0;
	}
