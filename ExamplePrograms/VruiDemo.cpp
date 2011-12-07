/***********************************************************************
VruiDemo - "Empty" VR application that displays a simple OpenGL scene in
a virtual reality environment, showing off some Vrui toolkit
functionality.
Copyright (c) 2003-2006 Oliver Kreylos

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
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLObject.h>
#include <GL/GLContextData.h>
#include <GL/GLGeometryWrappers.h>
#include <GLMotif/Button.h>
#include <GLMotif/Menu.h>
#include <GLMotif/PopupMenu.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>

class VruiDemo:public Vrui::Application,public GLObject
	{
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem // Data structure storing OpenGL-dependent application data
		{
		/* Elements: */
		public:
		GLuint textureObjectId; // Texture object ID of some texture
		GLuint displayListId; // Display list ID of some display list
		
		/* Constructors and destructors: */
		DataItem(void)
			{
			/* Create a texture object to hold a texture: */
			glGenTextures(1,&textureObjectId);
			
			/* Create a display list: */
			displayListId=glGenLists(1);
			};
		virtual ~DataItem(void)
			{
			/* Destroy the texture object: */
			glDeleteTextures(1,&textureObjectId);
			
			/* Destroy the display list: */
			glDeleteLists(displayListId,1);
			};
		};
	
	/* Elements: */
	private:
	Vrui::Scalar modelAngles[3]; // Euler angles to animate the model in degrees
	Vrui::Scalar rotationSpeeds[3]; // Rotation speeds around the Euler axes in degrees/s
	
	/* Vrui parameters: */
	GLMotif::PopupMenu* mainMenu; // The program's main menu
	
	/* Private methods: */
	GLMotif::PopupMenu* createMainMenu(void); // Creates the program's main menu
	
	/* Constructors and destructors: */
	public:
	VruiDemo(int& argc,char**& argv,char**& appDefaults); // Initializes the Vrui toolkit and the application
	virtual ~VruiDemo(void); // Shuts down the Vrui toolkit
	
	/* Methods: */
	virtual void initContext(GLContextData& contextData) const; // Called once upon creation of each OpenGL context
	virtual void display(GLContextData& contextData) const; // Called for every eye and every window on every frame
	virtual void frame(void); // Called exactly once per frame
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
	/* Initialize the animation parameters: */
	for(int i=0;i<3;++i)
		modelAngles[i]=Vrui::Scalar(0);
	rotationSpeeds[0]=Vrui::Scalar(9);
	rotationSpeeds[1]=Vrui::Scalar(-31);
	rotationSpeeds[2]=Vrui::Scalar(19);
	
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

void VruiDemo::initContext(GLContextData& contextData) const
	{
	/*********************************************************************
	This method is called once for every OpenGL context created for the
	program. It must not change application state, but only create
	OpenGL-dependent application data and store them in the GLContextData
	object for retrieval in the display method.
	*********************************************************************/
	
	/* Create context data item and store it in the GLContextData object: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Now is the time to upload all needed texture data: */
	glBindTexture(GL_TEXTURE_2D,dataItem->textureObjectId);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	GLfloat texImage[4][4]=
		{
		{0.0f, 0.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 0.0f, 0.0f},
		{1.0f, 1.0f, 0.0f, 0.0f}
		};
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,4,4,0,GL_LUMINANCE,GL_FLOAT,texImage);
	glBindTexture(GL_TEXTURE_2D,0);
	
	/* Now is also the time to upload all display lists' contents: */
	glNewList(dataItem->displayListId,GL_COMPILE);
	
	/* Draw a wireframe cube 10" wide (unless a user scaled the navigation transformation): */
	glPushAttrib(GL_LIGHTING_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(3.0f);
	glColor(1.0f,1.0f,1.0f);
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
	glPopAttrib();
	
	/* Finish the display list: */
	glEndList();
	}

void VruiDemo::display(GLContextData& contextData) const
	{
	/*********************************************************************
	This method is called once for every eye in every window on every
	frame. It must not change application state, as it is called an
	unspecified number of times. It also must not clear the screen or
	initialize the OpenGL transformation matrices. When this method is
	called, Vrui will already have rendered its own state (menus etc.) and
	have set up the transformation matrices so that all rendering in this
	method happens in navigation (i.e., model) coordinates.
	*********************************************************************/
	
	/* Get the OpenGL-dependent application data from the GLContextData object: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Insert generic OpenGL code here... */
	// ...
	
	/* Use the texture uploaded in the initDisplay() method: */
	glBindTexture(GL_TEXTURE_2D,dataItem->textureObjectId);
	// ...
	glBindTexture(GL_TEXTURE_2D,0);
	
	/* Set up the animation transformation: */
	glPushMatrix();
	
	/* Rotate around X, then Y, then Z: */
	glRotate(modelAngles[0],Vrui::Vector(1,0,0));
	glRotate(modelAngles[1],Vrui::Vector(0,1,0));
	glRotate(modelAngles[2],Vrui::Vector(0,0,1));
	
	/* Call the display list created in the initDisplay() method: */
	glCallList(dataItem->displayListId);
	
	/* Reset the animation transformation: */
	glPopMatrix();
	}

void VruiDemo::frame(void)
	{
	/*********************************************************************
	This function is called exactly once per frame, no matter how many
	eyes or windows exist. It is the perfect place to change application
	state (run simulations, animate models, etc.).
	*********************************************************************/
	
	/* Get the time since the last frame: */
	double frameTime=Vrui::getCurrentFrameTime();
	
	/* Change the model angles: */
	for(int i=0;i<3;++i)
		{
		modelAngles[i]+=rotationSpeeds[i]*frameTime;
		modelAngles[i]=Math::mod(modelAngles[i],Vrui::Scalar(360));
		}
	
	/* Request another rendering cycle to show the animation: */
	Vrui::scheduleUpdate(Vrui::getApplicationTime()+1.0/125.0); // Aim for 125 FPS
	}

void VruiDemo::resetNavigationCallback(Misc::CallbackData* cbData)
	{
	/* Reset the Vrui navigation transformation: */
	Vrui::NavTransform t=Vrui::NavTransform::identity;
	t*=Vrui::NavTransform::translateFromOriginTo(Vrui::getDisplayCenter());
	t*=Vrui::NavTransform::scale(Vrui::getInchFactor());
	Vrui::setNavigationTransformation(t);
	
	/*********************************************************************
	Now the coordinate system's origin is in the middle of the
	environment, e.g., in the middle of the workbench, and one coordinate
	unit is one inch long.
	*********************************************************************/
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
