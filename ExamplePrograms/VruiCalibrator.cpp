/***********************************************************************
VruiCalibrator - Simple program to check the calibration of a VR
environment.
Copyright (c) 2005 Oliver Kreylos

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

#include <Math/Math.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/Button.h>
#include <GLMotif/Menu.h>
#include <GLMotif/PopupMenu.h>
#include <Vrui/InputDevice.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>

class VruiDemo:public Vrui::Application
	{
	/* Elements: */
	private:
	
	/* Vrui parameters: */
	GLColor<GLfloat,4> modelColor; // Color to draw the model
	GLMotif::PopupMenu* mainMenu; // The program's main menu
	
	/* Private methods: */
	GLMotif::PopupMenu* createMainMenu(void); // Creates the program's main menu
	
	/* Constructors and destructors: */
	public:
	VruiDemo(int& argc,char**& argv,char**& appDefaults); // Initializes the Vrui toolkit and the application
	virtual ~VruiDemo(void); // Shuts down the Vrui toolkit
	
	/* Methods: */
	virtual void display(GLContextData& contextData) const; // Called for every eye and every window on every frame
	void resetNavigationCallback(Misc::CallbackData* cbData); // Method to reset the Vrui navigation transformation to its default
	};

/*************************
Methods of class VruiDemo:
*************************/

GLMotif::PopupMenu* VruiDemo::createMainMenu(void)
	{
	/* Create a popup shell to hold the main menu: */
	GLMotif::PopupMenu* mainMenuPopup=new GLMotif::PopupMenu("MainMenuPopup",Vrui::getWidgetManager());
	mainMenuPopup->setTitle("Vrui Demonstration");
	
	/* Create the main menu itself: */
	GLMotif::Menu* mainMenu=new GLMotif::Menu("MainMenu",mainMenuPopup,false);
	
	/* Create a button: */
	GLMotif::Button* resetNavigationButton=new GLMotif::Button("ResetNavigationButton",mainMenu,"Reset Navigation");
	
	/* Add a callback to the button: */
	resetNavigationButton->getSelectCallbacks().add(this,&VruiDemo::resetNavigationCallback);
	
	/* Finish building the main menu: */
	mainMenu->manageChild();
	
	return mainMenuPopup;
	}

VruiDemo::VruiDemo(int& argc,char**& argv,char**& appDefaults)
	:Vrui::Application(argc,argv,appDefaults),
	 mainMenu(0)
	{
	/* Calculate the model color: */
	for(int i=0;i<3;++i)
		modelColor[i]=1.0f-Vrui::getBackgroundColor()[i];
	modelColor[3]=1.0f;
	
	/* Create the user interface: */
	mainMenu=createMainMenu();
	
	/* Install the main menu: */
	Vrui::setMainMenu(mainMenu);
	
	/* Set the navigation transformation: */
	resetNavigationCallback(0);
	}

VruiDemo::~VruiDemo(void)
	{
	delete mainMenu;
	}

void VruiDemo::display(GLContextData& contextData) const
	{
	Vrui::Point displayCenter=Vrui::getDisplayCenter();
	Vrui::Scalar inchScale=Vrui::getInchFactor();
	
	/* Set up OpenGL state: */
	GLboolean lightingEnabled=glIsEnabled(GL_LIGHTING);
	if(lightingEnabled)
		glDisable(GL_LIGHTING);
	GLfloat lineWidth;
	glGetFloatv(GL_LINE_WIDTH,&lineWidth);
	glLineWidth(1.0f);
	
	/* Draw a 10" wireframe cube in the middle of the environment: */
	glPushMatrix();
	glTranslate(displayCenter-Vrui::Point::origin);
	glScale(inchScale,inchScale,inchScale);
	
	glColor(modelColor);
	glBegin(GL_LINES);
	glVertex(-5.0f,-5.0f,-5.0f);
	glVertex( 5.0f,-5.0f,-5.0f);
	glVertex(-5.0f, 5.0f,-5.0f);
	glVertex( 5.0f, 5.0f,-5.0f);
	glVertex(-5.0f, 5.0f, 5.0f);
	glVertex( 5.0f, 5.0f, 5.0f);
	glVertex(-5.0f,-5.0f, 5.0f);
	glVertex( 5.0f,-5.0f, 5.0f);
	
	glVertex(-5.0f,-5.0f,-5.0f);
	glVertex(-5.0f, 5.0f,-5.0f);
	glVertex( 5.0f,-5.0f,-5.0f);
	glVertex( 5.0f, 5.0f,-5.0f);
	glVertex( 5.0f,-5.0f, 5.0f);
	glVertex( 5.0f, 5.0f, 5.0f);
	glVertex(-5.0f,-5.0f, 5.0f);
	glVertex(-5.0f, 5.0f, 5.0f);
	
	glVertex(-5.0f,-5.0f,-5.0f);
	glVertex(-5.0f,-5.0f, 5.0f);
	glVertex( 5.0f,-5.0f,-5.0f);
	glVertex( 5.0f,-5.0f, 5.0f);
	glVertex( 5.0f, 5.0f,-5.0f);
	glVertex( 5.0f, 5.0f, 5.0f);
	glVertex(-5.0f, 5.0f,-5.0f);
	glVertex(-5.0f, 5.0f, 5.0f);
	glEnd();
	glPopMatrix();
	
	/* Draw coordinate axes for each input device: */
	int numDevices=Vrui::getNumInputDevices();
	for(int i=0;i<numDevices;++i)
		{
		Vrui::InputDevice* id=Vrui::getInputDevice(i);
		if(id->is6DOFDevice())
			{
			glPushMatrix();
			glMultMatrix(id->getTransformation());
			glScale(inchScale,inchScale,inchScale);
			glBegin(GL_LINES);
			glColor3f(1.0f,0.0f,0.0f);
			glVertex3f(-5.0f,0.0f,0.0f);
			glVertex3f( 5.0f,0.0f,0.0f);
			glColor3f(0.0f,1.0f,0.0f);
			glVertex3f(0.0f,-5.0f,0.0f);
			glVertex3f(0.0f, 5.0f,0.0f);
			glColor3f(0.0f,0.0f,1.0f);
			glVertex3f(0.0f,0.0f,-5.0f);
			glVertex3f(0.0f,0.0f, 5.0f);
			glEnd();
			glPopMatrix();
			}
		}
	
	/* This only works in the CAVE - draw a grid on the "front wall" to calibrate external cameras: */
	glBegin(GL_LINES);
	glColor3f(1.0f,1.0f,0.0f);
	for(int y=0;y<=8;++y)
		{
		glVertex(-60.0f,-36.0f,float(y)*12.0f);
		glVertex(60.0f,-36.0f,float(y)*12.0f);
		}
	for(int x=0;x<=10;++x)
		{
		glVertex(float(x)*12.0f-60.0f,-36.0f,0.0f);
		glVertex(float(x)*12.0f-60.0f,-36.0f,96.0f);
		}
	glEnd();
	
	/* Restore OpenGL state: */
	glLineWidth(lineWidth);
	if(lightingEnabled)
		glEnable(GL_LIGHTING);
	}

void VruiDemo::resetNavigationCallback(Misc::CallbackData* cbData)
	{
	/* Reset the Vrui navigation transformation: */
	Vrui::NavTransform t=Vrui::NavTransform::identity;
	t*=Vrui::NavTransform::translateFromOriginTo(Vrui::getDisplayCenter());
	t*=Vrui::NavTransform::scale(Vrui::getInchFactor());
	t*=Vrui::NavTransform::translateToOriginFrom(Vrui::getDisplayCenter());
	Vrui::setNavigationTransformation(t);
	}

int main(int argc,char* argv[])
	{
	/* Create an application object: */
	char** appDefaults=0; // This is an additional parameter no one ever uses
	VruiDemo app(argc,argv,appDefaults);
	
	/* Run the Vrui main loop: */
	app.run();
	
	/* Exit to OS: */
	return 0;
	}
