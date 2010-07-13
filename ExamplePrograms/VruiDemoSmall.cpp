#include <GL/gl.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>

class VruiDemo:public Vrui::Application
	{
	/* Constructors and destructors: */
	public:
	VruiDemo(int& argc,char**& argv,char**& appDefaults);
	
	/* Methods: */
	virtual void display(GLContextData& contextData) const;
	};

/*************************
Methods of class VruiDemo:
*************************/

VruiDemo::VruiDemo(int& argc,char**& argv,char**& appDefaults)
	:Vrui::Application(argc,argv,appDefaults)
	{
	/* Set the navigation transformation: */
	Vrui::setNavigationTransformation(Vrui::Point::origin,Vrui::Scalar(9));
	}

void VruiDemo::display(GLContextData& contextData) const
	{
	/* Draw a wireframe cube: */
	glPushAttrib(GL_LIGHTING_BIT|GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(3.0f);
	glColor3f(1.0f,1.0f,1.0f);
	glBegin(GL_LINES);
	glVertex3f(-5.0f,-5.0f,-5.0f);
	glVertex3f( 5.0f,-5.0f,-5.0f);
	glVertex3f(-5.0f, 5.0f,-5.0f);
	glVertex3f( 5.0f, 5.0f,-5.0f);
	glVertex3f(-5.0f, 5.0f, 5.0f);
	glVertex3f( 5.0f, 5.0f, 5.0f);
	glVertex3f(-5.0f,-5.0f, 5.0f);
	glVertex3f( 5.0f,-5.0f, 5.0f);
	
	glVertex3f(-5.0f,-5.0f,-5.0f);
	glVertex3f(-5.0f, 5.0f,-5.0f);
	glVertex3f( 5.0f,-5.0f,-5.0f);
	glVertex3f( 5.0f, 5.0f,-5.0f);
	glVertex3f( 5.0f,-5.0f, 5.0f);
	glVertex3f( 5.0f, 5.0f, 5.0f);
	glVertex3f(-5.0f,-5.0f, 5.0f);
	glVertex3f(-5.0f, 5.0f, 5.0f);
	
	glVertex3f(-5.0f,-5.0f,-5.0f);
	glVertex3f(-5.0f,-5.0f, 5.0f);
	glVertex3f( 5.0f,-5.0f,-5.0f);
	glVertex3f( 5.0f,-5.0f, 5.0f);
	glVertex3f( 5.0f, 5.0f,-5.0f);
	glVertex3f( 5.0f, 5.0f, 5.0f);
	glVertex3f(-5.0f, 5.0f,-5.0f);
	glVertex3f(-5.0f, 5.0f, 5.0f);
	glEnd();
	glPopAttrib();
	}

int main(int argc,char* argv[])
	{
	/* Create an application object: */
	char** appDefaults=0;
	VruiDemo app(argc,argv,appDefaults);
	
	/* Run the Vrui main loop: */
	app.run();
	
	/* Exit to OS: */
	return 0;
	}
