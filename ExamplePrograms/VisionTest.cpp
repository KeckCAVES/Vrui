/***********************************************************************
VisionTest - Utility to draw a vision test chart to test the visual
acuity provided by a VR display.
Copyright (c) 2015 Oliver Kreylos

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
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Math/Random.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/LinearUnit.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLMaterialTemplates.h>
#include <GL/GLObject.h>
#include <GL/GLContextData.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>
#include <Vrui/Viewer.h>

class VisionTest:public Vrui::Application,public GLObject
	{
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint displayListId; // ID of display list to render the vision chart
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	private:
	Vrui::Scalar distance; // Distance at which to render the vision chart in Vrui physical units
	Vrui::Scalar baseSize; // Size of optotypes in 20/20 row in Vrui physical units
	Vrui::OGTransform chartTransform; // Transformation from normalized chart space (20/20 row) to Vrui physical space
	double angles[14*5]; // Random angles at which to draw the Langolt "C" optotypes, in 45 degree steps
	
	/* Private methods: */
	void renderLandoltC(double centerX,double centerY,double diameter,double angle) const; // Renders a rotated Landolt "C" optotype at the given position and size
	
	/* Constructors and destructors: */
	public:
	VisionTest(int& argc,char**& argv);
	
	/* Methods from Vrui::Application: */
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

/*************************************
Methods of class VisionTest::DataItem:
*************************************/

VisionTest::DataItem::DataItem(void)
	:displayListId(glGenLists(1))
	{
	}

VisionTest::DataItem::~DataItem(void)
	{
	glDeleteLists(displayListId,1);
	}

/***************************
Methods of class VisionTest:
***************************/

void VisionTest::renderLandoltC(double centerX,double centerY,double diameter,double angle) const
	{
	double r=diameter*0.5;
	double w=diameter*0.2;
	double innerAlpha=Math::asin(0.5*w/(r-w));
	double outerAlpha=Math::asin(0.5*w/r);
	
	glBegin(GL_QUAD_STRIP);
	glNormal3d(0.0,0.0,1.0);
	int numSegments=128;
	for(int i=0;i<=numSegments;++i)
		{
		double ia=(innerAlpha*double(numSegments-i)+(2.0*Math::Constants<double>::pi-innerAlpha)*double(i))/double(numSegments);
		double oa=(outerAlpha*double(numSegments-i)+(2.0*Math::Constants<double>::pi-outerAlpha)*double(i))/double(numSegments);
		glVertex2d(centerX+Math::cos(ia+angle)*(r-w),centerY+Math::sin(ia+angle)*(r-w));
		glVertex2d(centerX+Math::cos(oa+angle)*r,centerY+Math::sin(oa+angle)*r);
		}
	glEnd();
	}

VisionTest::VisionTest(int& argc,char**& argv)
	:Vrui::Application(argc,argv)
	{
	/* Parse the command line: */
	Geometry::LinearUnit distanceUnit(Geometry::LinearUnit::FOOT,Geometry::LinearUnit::Scalar(1)); // Default unit is foot
	distance=Vrui::Scalar(20); // Default distance is 20'
	if(argc>1)
		distance=Vrui::Scalar(atof(argv[1]));
	if(argc>2)
		distanceUnit=Geometry::LinearUnit(argv[2],Geometry::LinearUnit::Scalar(1));
	
	/* Convert the distance to Vrui physical units through meters: */
	distance*=Vrui::getMeterFactor()/distanceUnit.getMeterFactor();
	
	/* Calculate the optotype size of the 20/20 row at the selected distance: */
	baseSize=Math::mul2(Math::tan(Math::div2(Math::rad(Vrui::Scalar(5.0/60.0)))))*distance;
	
	std::cout<<distance<<", "<<baseSize<<std::endl;
	
	/* Initialize the optotype rendering angles: */
	for(int row=0;row<14;++row)
		for(int column=0;column<5;++column)
			angles[row*5+column]=double(Math::randUniformCO(0,8))*2.0*Math::Constants<double>::pi/8.0;
	}

void VisionTest::display(GLContextData& contextData) const
	{
	/* Retrieve the context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(1.0f);
	
	/* Render vision chart selected distance in front of display center: */
	glPushMatrix();
	glMultMatrix(chartTransform);
	
	/* Render the vision chart: */
	glCallList(dataItem->displayListId);
	
	glPopMatrix();
	glPopAttrib();
	}

void VisionTest::resetNavigation(void)
	{
	/* Calculate the chart transformation by placing the test chart distance ahead of the current viewer position: */
	Vrui::Point center=Vrui::getMainViewer()->getHeadPosition();
	center+=Vrui::getForwardDirection()*distance;
	chartTransform=Vrui::OGTransform::translateFromOriginTo(center);
	
	/* Place the chart vertically: */
	Vrui::Vector y=Vrui::getUpDirection();
	Vrui::Vector x=Vrui::getForwardDirection()^y;
	chartTransform*=Vrui::OGTransform::rotate(Vrui::Rotation::fromBaseVectors(x,y));
	
	/* Scale the chart so that the 20/20 row optotypes subtend exactly 5 minutes of arc: */
	chartTransform*=Vrui::OGTransform::scale(baseSize);
	
	chartTransform.renormalize();
	
	/* Render everything in physical coordinates: */
	Vrui::setNavigationTransformation(Vrui::NavTransform::identity);
	}

void VisionTest::initContext(GLContextData& contextData) const
	{
	/* Create a context data item and associate it with the OpenGL context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Initialize the chart display list: */
	glNewList(dataItem->displayListId,GL_COMPILE);
	
	glColor(Vrui::getForegroundColor());
	
	/* Draw the rows upwards from the central 20/20 row: */
	double spacing=1.5;
	
	double diameter=1.0;
	double y=0.0;
	for(int row=0;row<10;++row)
		{
		for(int i=0;i<5;++i)
			renderLandoltC(0.0-diameter*spacing*double(i-2),y,diameter,angles[(row+4)*5+i]);
		
		y+=diameter*spacing;
		diameter*=1.25;
		}
	
	/* Draw the rows downwards from the central 20/20 row: */
	diameter=1.0;
	y=0.0;
	
	/* Draw the central row indicators: */
	glBegin(GL_LINES);
	glVertex2d(-diameter*spacing*-10.0,0.0);
	glVertex2d(-diameter*spacing*-3.0,0.0);
	
	glVertex2d(-diameter*spacing*3.0,0.0);
	glVertex2d(-diameter*spacing*10.0,0.0);
	glEnd();
	
	for(int row=1;row<5;++row)
		{
		diameter/=1.25;
		y-=diameter*spacing;
		
		for(int i=0;i<5;++i)
			renderLandoltC(0.0-diameter*spacing*double(i-2),y,diameter,angles[(4-row)*5+i]);
		}
	
	glEndList();
	}

/* Create and execute an application object: */
VRUI_APPLICATION_RUN(VisionTest)
