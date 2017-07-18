/***********************************************************************
VruiAppTemplate - Template to write a very simple Vrui application
displaying an OpenGL scene in immediate mode, with a basic menu system
to control the application and set rendering parameters.
Copyright (c) 2011-2015 Oliver Kreylos

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

#include <GL/gl.h>
#include <GL/GLMaterial.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/RadioBox.h>
#include <GLMotif/Button.h>
#include <GLMotif/CascadeButton.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>

class VruiAppTemplate:public Vrui::Application
	{
	/* Elements: */
	private:
	int renderingMode; // Current OpenGL rendering mode (0: points, 1: lines, 2: polygons)
	GLMaterial material; // OpenGL material properties
	GLMotif::PopupMenu* mainMenu; // The program's main menu
	
	/* Private methods: */
	GLMotif::PopupMenu* createRenderingModesMenu(void); // Creates the rendering modes submenu
	GLMotif::PopupMenu* createMainMenu(void); // Creates the program's main menu
	void renderingModesMenuCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData); // Method called when the user makes a selection from the rendering modes submenu
	
	/* Constructors and destructors: */
	public:
	VruiAppTemplate(int& argc,char**& argv); // Initializes the Vrui toolkit and the application
	virtual ~VruiAppTemplate(void); // Shuts down the Vrui toolkit
	
	/* Methods: */
	virtual void frame(void); // Called exactly once per frame
	virtual void display(GLContextData& contextData) const; // Called for every eye and every window on every frame
	virtual void resetNavigation(void);
	};

/********************************
Methods of class VruiAppTemplate:
********************************/

GLMotif::PopupMenu* VruiAppTemplate::createRenderingModesMenu(void)
	{
	/* Create the submenu's top-level shell: */
	GLMotif::PopupMenu* renderingModesMenu=new GLMotif::PopupMenu("RenderingModesMenu",Vrui::getWidgetManager());
	
	/* Create a radio box of rendering modes: */
	GLMotif::RadioBox* renderingModes=new GLMotif::RadioBox("RenderingModes",renderingModesMenu,false);
	renderingModes->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
	
	/*********************************************************************
	Add rendering mode buttons:
	*********************************************************************/
	
	renderingModes->addToggle("Points");
	renderingModes->addToggle("Lines");
	renderingModes->addToggle("Polygons");
	
	/* Select the toggle corresponding to the current rendering mode: */
	renderingModes->setSelectedToggle(renderingMode);
	
	/* Install a selection callback: */
	renderingModes->getValueChangedCallbacks().add(this,&VruiAppTemplate::renderingModesMenuCallback);
	
	/* Finish building the radio box: */
	renderingModes->manageChild();
	
	/* Finish building the rendering modes menu: */
	renderingModesMenu->manageMenu();
	return renderingModesMenu;
	}

GLMotif::PopupMenu* VruiAppTemplate::createMainMenu(void)
	{
	/* Create the main menu shell: */
	GLMotif::PopupMenu* mainMenu=new GLMotif::PopupMenu("MainMenu",Vrui::getWidgetManager());
	mainMenu->setTitle("Vrui App Template");
	
	/*********************************************************************
	Enable the following code to create a submenu to change rendering
	modes:
	*********************************************************************/
	
	#if 1
	
	/* Create a cascade button to show the "Rendering Modes" submenu: */
	GLMotif::CascadeButton* renderingModesCascade=new GLMotif::CascadeButton("RenderingModesCascade",mainMenu,"Rendering Modes");
	renderingModesCascade->setPopup(createRenderingModesMenu());
	
	#endif
	
	/* Finish building the main menu: */
	mainMenu->manageMenu();
	return mainMenu;
	}

void VruiAppTemplate::renderingModesMenuCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData)
	{
	/* Set the application's rendering mode to the newly selected toggle: */
	renderingMode=cbData->radioBox->getToggleIndex(cbData->newSelectedToggle);
	}

VruiAppTemplate::VruiAppTemplate(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 renderingMode(2), // Render polygons initially
	 material(GLMaterial::Color(0.0f,0.5f,1.0f),GLMaterial::Color(1.0f,1.0f,1.0f),25.0f), // Use a bluish specular material
	 mainMenu(0)
	{
	/* Create the user interface: */
	mainMenu=createMainMenu();
	
	/* Install the main menu: */
	Vrui::setMainMenu(mainMenu);
	}

VruiAppTemplate::~VruiAppTemplate(void)
	{
	delete mainMenu;
	}

void VruiAppTemplate::frame(void)
	{
	/*********************************************************************
	This function is called exactly once per frame, no matter how many
	eyes or windows exist. It is the appropriate place to change
	application or Vrui state (run simulations, animate models,
	synchronize with background threads, change the navigation
	transformation, etc.).
	*********************************************************************/
	
	/*********************************************************************
	Insert application state update code here.
	*********************************************************************/
	}

void VruiAppTemplate::display(GLContextData& contextData) const
	{
	/*********************************************************************
	This method is called once for every eye in every window on every
	frame. It must not change application or Vrui state, as it is called
	an unspecified number of times, and might be called from parallel
	background threads. It also must not clear the screen or initialize
	the OpenGL transformation matrices. When this method is called, Vrui
	will already have rendered its own state (menus etc.) and have set up
	the transformation matrices so that all rendering in this method
	happens in navigation (i.e., model) coordinates.
	*********************************************************************/
	
	/* Save OpenGL state changed by the following rendering code: */
	glPushAttrib(GL_ENABLE_BIT|GL_POLYGON_BIT);
	
	/* Set the rendering mode: */
	switch(renderingMode)
		{
		case 0: // Points
			glPolygonMode(GL_FRONT_AND_BACK,GL_POINT);
			glDisable(GL_LIGHTING);
			break;
		
		case 1: // Lines
			glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
			glDisable(GL_LIGHTING);
			break;
		
		case 2: // Polygons
			glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
			glEnable(GL_LIGHTING);
			break;
		
		default: // Not used
			;
		}
	
	/*********************************************************************
	Insert OpenGL code here.
	*********************************************************************/
	
	/* Draw a two-sided square: */
	glMaterial(GLMaterialEnums::FRONT_AND_BACK,material);
	
	glBegin(GL_QUADS);
	glNormal3f(0.0f,0.0f,1.0f);
	glVertex3f(-1.0f,-1.0f,0.0f);
	glVertex3f( 1.0f,-1.0f,0.0f);
	glVertex3f( 1.0f, 1.0f,0.0f);
	glVertex3f(-1.0f, 1.0f,0.0f);
	
	glNormal3f(0.0f,0.0f,-1.0f);
	glVertex3f(-1.0f,-1.0f,0.0f);
	glVertex3f(-1.0f, 1.0f,0.0f);
	glVertex3f( 1.0f, 1.0f,0.0f);
	glVertex3f( 1.0f,-1.0f,0.0f);
	glEnd();
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

void VruiAppTemplate::resetNavigation(void)
	{
	/*********************************************************************
	The following code specifies the center of the OpenGL scene, the
	overall size of the OpenGL scene, and the "up" direction of the OpenGL
	scene, all in the same model coordinate system used in the display()
	method.
	*********************************************************************/
	
	/* Center point is the origin: */
	Vrui::Point center(0,0,0);
	
	/* Scene size is one model coordinate unit: */
	Vrui::Scalar size=2;
	
	/* Y axis points up: */
	Vrui::Vector up(0,1,0);
	
	/* Reset the Vrui navigation transformation: */
	Vrui::setNavigationTransformation(center,size,up);
	}

/* Create and execute an application object: */
VRUI_APPLICATION_RUN(VruiAppTemplate)
