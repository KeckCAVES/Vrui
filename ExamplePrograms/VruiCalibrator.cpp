/***********************************************************************
VruiCalibrator - Simple program to check the calibration of a VR
environment.
Copyright (c) 2005-2016 Oliver Kreylos

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

#include <stdlib.h>
#include <string.h>
#include <Math/Math.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>
#include <Vrui/Viewer.h>
#include <Vrui/Application.h>

class VruiCalibrator:public Vrui::Application
	{
	/* Elements: */
	private:
	
	/* Vrui parameters: */
	GLColor<GLfloat,4> modelColor; // Color to draw the model
	int ignoreDeviceIndex; // Index of device which is not to be drawn (because it is the head device)
	bool viewerAlignedGrid; // Flag whether to align the grid with the main viewer's viewing direction to check distortion correction
	Vrui::Scalar viewerGridDistance; // Distance from main viewer to grid
	Vrui::Scalar viewerGridFov; // Tangent of half of viewer-aligned grid's field-of-view
	int numGridSquares; // Number of squares along each edge of the grid
	bool checkerboard; // Flag to draw a black&white checkerboard instead of a grid
	float gridLineWidth; // Cosmetic line width of grid lines in pixels
	
	/* Constructors and destructors: */
	public:
	VruiCalibrator(int& argc,char**& argv); // Initializes the Vrui toolkit and the application
	virtual ~VruiCalibrator(void); // Shuts down the Vrui toolkit
	
	/* Methods: */
	virtual void display(GLContextData& contextData) const; // Called for every eye and every window on every frame
	virtual void resetNavigation(void);
	};

/*******************************
Methods of class VruiCalibrator:
*******************************/

VruiCalibrator::VruiCalibrator(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 modelColor(Vrui::getForegroundColor()),ignoreDeviceIndex(-1),
	 viewerAlignedGrid(false),viewerGridDistance(Vrui::getInchFactor()*Vrui::Scalar(36)),viewerGridFov(Math::tan(Math::div2(Math::rad(Vrui::Scalar(90.0))))),
	 numGridSquares(10),checkerboard(false),gridLineWidth(1.0f)
	{
	/* Parse the command line: */
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"ignoreDeviceIndex")==0||strcasecmp(argv[i]+1,"idi")==0)
				{
				++i;
				ignoreDeviceIndex=atoi(argv[i]);
				}
			else if(strcasecmp(argv[i]+1,"viewerAlignedGrid")==0||strcasecmp(argv[i]+1,"vag")==0)
				viewerAlignedGrid=true;
			else if(strcasecmp(argv[i]+1,"viewerGridDistance")==0||strcasecmp(argv[i]+1,"vgd")==0)
				{
				++i;
				viewerGridDistance=Vrui::Scalar(atof(argv[i]));
				}
			else if(strcasecmp(argv[i]+1,"viewerGridFov")==0||strcasecmp(argv[i]+1,"vgf")==0)
				{
				++i;
				viewerGridFov=Math::tan(Math::rad(Math::div2(Vrui::Scalar(atof(argv[i])))));
				}
			else if(strcasecmp(argv[i]+1,"numGridSquares")==0||strcasecmp(argv[i]+1,"ngs")==0)
				{
				++i;
				numGridSquares=atoi(argv[i]);
				}
			else if(strcasecmp(argv[i]+1,"checkerboard")==0||strcasecmp(argv[i]+1,"c")==0)
				checkerboard=true;
			else if(strcasecmp(argv[i]+1,"gridLineWidth")==0||strcasecmp(argv[i]+1,"glw")==0)
				{
				++i;
				gridLineWidth=atoi(argv[i]);
				}
			}
		}
	}

VruiCalibrator::~VruiCalibrator(void)
	{
	}

void VruiCalibrator::display(GLContextData& contextData) const
	{
	Vrui::Point displayCenter=Vrui::getDisplayCenter();
	Vrui::Scalar inchScale=Vrui::getInchFactor();
	
	/* Set up OpenGL state: */
	GLboolean lightingEnabled=glIsEnabled(GL_LIGHTING);
	if(lightingEnabled)
		glDisable(GL_LIGHTING);
	GLfloat lineWidth;
	glGetFloatv(GL_LINE_WIDTH,&lineWidth);
	glLineWidth(gridLineWidth);
	
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
	
	/* Draw coordinate axes and linear/angular velocity vectors for each input device: */
	int numDevices=Vrui::getNumInputDevices();
	for(int i=0;i<numDevices;++i)
		{
		Vrui::InputDevice* id=Vrui::getInputDevice(i);
		if(i!=ignoreDeviceIndex&&id->is6DOFDevice())
			{
			Vrui::Point pos=id->getPosition();
			glBegin(GL_LINES);
			
			/* Draw the linear velocity vector: */
			glColor3f(1.0f,1.0f,0.0f);
			glVertex(pos);
			glVertex(pos+id->getLinearVelocity());
			
			/* Draw the angular velocity vector: */
			glColor3f(0.0f,1.0f,1.0f);
			glVertex(pos);
			glVertex(pos+id->getAngularVelocity()*Vrui::Scalar(5));
			glEnd();
			
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
	
	/* Draw a grid to check calibration and distortion correction: */
	Vrui::OGTransform ct;
	if(viewerAlignedGrid)
		{
		/* Create a transformation from a unit square in the (x, y) plane to a plane perpendicular to the main viewer's viewing direction: */
		Vrui::ONTransform viewerTrans=Vrui::getMainViewer()->getHeadTransformation();
		Vrui::Vector viewDir=viewerTrans.inverseTransform(Vrui::getMainViewer()->getViewDirection());
		Vrui::Vector x=Vrui::Vector::zero;
		x[Geometry::findOrthogonalAxis(viewDir)]=Vrui::Scalar(1);
		Vrui::Vector z=x^viewDir;
		x=viewDir^z;
		
		ct=viewerTrans;
		ct*=Vrui::OGTransform::translate(viewDir*viewerGridDistance);
		ct*=Vrui::OGTransform::rotate(Vrui::Rotation::fromBaseVectors(x,z));
		ct*=Vrui::OGTransform::scale(viewerGridDistance*viewerGridFov);
		}
	else
		{
		/* Create a transformation from a unit square in the (x, y) plane to an upright environment-scaled square through the display center: */
		Vrui::OGTransform ct=Vrui::OGTransform::translateFromOriginTo(Vrui::getDisplayCenter());
		Vrui::Vector z=Vrui::getUpDirection();
		Vrui::Vector x=Vrui::getForwardDirection()^z;
		ct*=Vrui::OGTransform::rotate(Vrui::Rotation::fromBaseVectors(x,z));
		ct*=Vrui::OGTransform::scale(Vrui::getDisplaySize());
		}
	
	/* Go to square coordinates: */
	glPushMatrix();
	glMultMatrix(ct);
	
	if(checkerboard)
		{
		/* Draw a black&white checkerboard: */
		float ngs=float(numGridSquares);
		float go=ngs*0.5f;
		glBegin(GL_QUADS);
		for(int y=0;y<numGridSquares;++y)
			for(int x=0;x<numGridSquares;++x)
				{
				GLfloat col=(x+y)%2==0?1.0f:0.0f;
				glColor3f(col,col,col);
				glVertex(2.0f*(float(x)-go)/ngs,2.0f*(float(y)-go)/ngs);
				glVertex(2.0f*(float(x+1)-go)/ngs,2.0f*(float(y)-go)/ngs);
				glVertex(2.0f*(float(x+1)-go)/ngs,2.0f*(float(y+1)-go)/ngs);
				glVertex(2.0f*(float(x)-go)/ngs,2.0f*(float(y+1)-go)/ngs);
				}
		glEnd();
		}
	else
		{
		/* Draw a grid of lines: */
		float ngs=float(numGridSquares);
		float go=ngs*0.5f;
		glBegin(GL_LINES);
		glColor(modelColor);
		for(int y=0;y<=numGridSquares;++y)
			{
			glVertex(-1.0f,2.0f*(float(y)-go)/ngs);
			glVertex(1.0f,2.0f*(float(y)-go)/ngs);
			}
		for(int x=0;x<=numGridSquares;++x)
			{
			glVertex(2.0f*(float(x)-go)/ngs,-1.0f);
			glVertex(2.0f*(float(x)-go)/ngs,1.0f);
			}
		glEnd();
		}
	
	/* Return to navigational coordinates: */
	glPopMatrix();
	
	/* Restore OpenGL state: */
	glLineWidth(lineWidth);
	if(lightingEnabled)
		glEnable(GL_LIGHTING);
	}

void VruiCalibrator::resetNavigation(void)
	{
	/* Reset the Vrui navigation transformation: */
	Vrui::NavTransform t=Vrui::NavTransform::identity;
	t*=Vrui::NavTransform::translateFromOriginTo(Vrui::getDisplayCenter());
	// t*=Vrui::NavTransform::scale(Vrui::getInchFactor());
	t*=Vrui::NavTransform::translateToOriginFrom(Vrui::getDisplayCenter());
	Vrui::setNavigationTransformation(t);
	}

/* Create and execute an application object: */
VRUI_APPLICATION_RUN(VruiCalibrator)
