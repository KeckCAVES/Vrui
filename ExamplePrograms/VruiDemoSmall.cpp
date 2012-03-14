/***********************************************************************
VruiDemoSmall - Extremely simple Vrui application to demonstrate the
small amount of code overhead introduced by the Vrui toolkit.
Copyright (c) 2006-2012 Oliver Kreylos

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
