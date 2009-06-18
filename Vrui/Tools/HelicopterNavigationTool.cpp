/***********************************************************************
HelicopterNavigationTool - Class for navigation tools using a simplified
helicopter flight model, a la Enemy Territory: Quake Wars' Anansi. Yeah,
I like that -- wanna fight about it?
Copyright (c) 2007-2009 Oliver Kreylos

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

#include <Misc/Utility.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Viewer.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/HelicopterNavigationTool.h>

namespace Vrui {

/************************************************
Methods of class HelicopterNavigationToolFactory:
************************************************/

HelicopterNavigationToolFactory::HelicopterNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("HelicopterNavigationTool",toolManager),
	 g(getMeterFactor()*Scalar(9.81)),
	 collectiveMin(Scalar(0)),collectiveMax(g*Scalar(1.5)),
	 thrust(g*Scalar(5)),
	 brake(g*Scalar(0.75))
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,3);
	layout.setNumValuators(0,6);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	Vector rot=cfs.retrieveValue<Vector>("./rotateFactors",Vector(-60,-60,60));
	for(int i=0;i<3;++i)
		rotateFactors[i]=Math::rad(rot[i]);
	g=cfs.retrieveValue<Scalar>("./g",g);
	collectiveMin=cfs.retrieveValue<Scalar>("./collectiveMin",collectiveMin);
	collectiveMax=cfs.retrieveValue<Scalar>("./collectiveMax",collectiveMax);
	thrust=cfs.retrieveValue<Scalar>("./thrust",thrust);
	brake=cfs.retrieveValue<Scalar>("./brake",brake);
	Vector drag=cfs.retrieveValue<Vector>("./dragCoefficients",Vector(0.3,0.1,0.3));
	for(int i=0;i<3;++i)
		dragCoefficients[i]=-Math::abs(drag[i]);
	Geometry::Vector<Scalar,2> view=cfs.retrieveValue<Geometry::Vector<Scalar,2> >("./viewAngleFactors",Geometry::Vector<Scalar,2>(35,-25));
	for(int i=0;i<2;++i)
		viewAngleFactors[i]=Math::rad(view[i]);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("NavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Set tool class' factory pointer: */
	HelicopterNavigationTool::factory=this;
	}

HelicopterNavigationToolFactory::~HelicopterNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	HelicopterNavigationTool::factory=0;
	}

const char* HelicopterNavigationToolFactory::getName(void) const
	{
	return "Helicopter Flight";
	}

Tool* HelicopterNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new HelicopterNavigationTool(this,inputAssignment);
	}

void HelicopterNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveHelicopterNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("NavigationTool");
	}

extern "C" ToolFactory* createHelicopterNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	HelicopterNavigationToolFactory* helicopterNavigationToolFactory=new HelicopterNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return helicopterNavigationToolFactory;
	}

extern "C" void destroyHelicopterNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/***************************************************
Methods of class HelicopterNavigationTool::DataItem:
***************************************************/

HelicopterNavigationTool::DataItem::DataItem(void)
	:displayListBase(glGenLists(11))
	{
	}

HelicopterNavigationTool::DataItem::~DataItem(void)
	{
	glDeleteLists(displayListBase,11);
	}

/*************************************************
Static elements of class HelicopterNavigationTool:
*************************************************/

HelicopterNavigationToolFactory* HelicopterNavigationTool::factory=0;

/*****************************************
Methods of class HelicopterNavigationTool:
*****************************************/

HelicopterNavigationTool::HelicopterNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(factory,inputAssignment)
	{
	/* Initialize button and valuator states: */
	for(int i=0;i<6;++i)
		valuators[i]=Scalar(0);
	for(int i=0;i<2;++i)
		buttons[i]=false;
	}

const ToolFactory* HelicopterNavigationTool::getFactory(void) const
	{
	return factory;
	}

void HelicopterNavigationTool::buttonCallback(int,int buttonIndex,InputDevice::ButtonCallbackData* cbData)
	{
	/* Process based on which button was pressed: */
	if(buttonIndex==0)
		{
		if(cbData->newButtonState)
			{
			/* Act depending on this tool's current state: */
			if(isActive())
				{
				/* Go back to original transformation: */
				NavTransform nav=pre;
				nav*=NavTransform::translateToOriginFrom(currentPosition);
				nav*=post;
				setNavigationTransformation(nav);
				
				/* Deactivate this tool: */
				deactivate();
				}
			else
				{
				/* Try activating this tool: */
				if(activate())
					{
					/* Initialize the navigation: */
					Vector x=Geometry::cross(getForwardDirection(),getUpDirection());
					Vector y=Geometry::cross(getUpDirection(),x);
					pre=NavTransform::translateFromOriginTo(getMainViewer()->getHeadPosition());
					pre*=NavTransform::rotate(Rotation::fromBaseVectors(x,y));
					
					post=Geometry::invert(pre);
					post*=getNavigationTransformation();
					
					currentPosition=Point::origin;
					currentOrientation=Rotation::identity;
					currentVelocity=Vector::zero;
					lastFrameTime=getApplicationTime();
					}
				}
			}
		}
	else
		{
		/* Store the new state of the button: */
		buttons[buttonIndex-1]=cbData->newButtonState;
		}
	}

void HelicopterNavigationTool::valuatorCallback(int,int valuatorIndex,InputDevice::ValuatorCallbackData* cbData)
	{
	/* Store the new valuator state: */
	valuators[valuatorIndex]=cbData->newValuatorValue;
	}

void HelicopterNavigationTool::frame(void)
	{
	/* Act depending on this tool's current state: */
	if(isActive())
		{
		double newFrameTime=getApplicationTime();
		Scalar dt=Scalar(newFrameTime-lastFrameTime);
		
		/* Update the current orientation based on the pitch, roll, and yaw controls: */
		Vector rot=Vector(valuators[0]*factory->rotateFactors[0],valuators[1]*factory->rotateFactors[1],valuators[2]*factory->rotateFactors[2]);
		currentOrientation.leftMultiply(Rotation::rotateScaledAxis(rot*dt));
		currentOrientation.renormalize();
		
		/* Update the current velocity based on collective, throttle and brake: */
		Vector accel=Vector(0,0,-factory->g);
		accel+=currentOrientation.inverseTransform(Vector(0,0,(factory->collectiveMax-factory->collectiveMin)*(Scalar(1)-valuators[3])*Scalar(0.5)+factory->collectiveMin));
		if(buttons[0])
			accel+=currentOrientation.inverseTransform(Vector(0,factory->thrust,0));
		if(buttons[1])
			accel+=currentOrientation.inverseTransform(Vector(0,-factory->brake,0));
		
		/* Calculate drag: */
		Vector localVel=currentOrientation.transform(currentVelocity);
		Vector drag=Vector(localVel[0]*factory->dragCoefficients[0],localVel[1]*factory->dragCoefficients[1],localVel[2]*factory->dragCoefficients[2]);
		accel+=currentOrientation.inverseTransform(drag);
		
		currentVelocity+=accel*dt;
		
		/* Update the current position based on current velocity: */
		currentPosition+=currentVelocity*dt;
		
		/* Set the new navigation transformation: */
		NavTransform nav=pre;
		nav*=NavTransform::rotate(Rotation::rotateZ(valuators[4]*factory->viewAngleFactors[0]));
		nav*=NavTransform::rotate(Rotation::rotateX(valuators[5]*factory->viewAngleFactors[1]));
		nav*=NavTransform::rotate(currentOrientation);
		nav*=NavTransform::translateToOriginFrom(currentPosition);
		nav*=post;
		setNavigationTransformation(nav);
		
		/* Prepare for the next frame: */
		lastFrameTime=newFrameTime;
		Vrui::requestUpdate();
		}
	}

void HelicopterNavigationTool::display(GLContextData& contextData) const
	{
	if(isActive())
		{
		/* Get the data item: */
		DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
		
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(1.0f);
		glColor3f(0.0f,1.0f,0.0f);
		
		float y=float(getFrontplaneDist())*1.25f;
		
		glPushMatrix();
		glMultMatrix(pre);
		glRotate(valuators[4]*Math::deg(factory->viewAngleFactors[0]),Vector(0,0,1));
		glRotate(valuators[5]*Math::deg(factory->viewAngleFactors[1]),Vector(1,0,0));
		
		glBegin(GL_LINES);
		glVertex3f(-y*0.02f,y,   0.00f);
		glVertex3f(-y*0.01f,y,   0.00f);
		glVertex3f( y*0.01f,y,   0.00f);
		glVertex3f( y*0.02f,y,   0.00f);
		glVertex3f(   0.00f,y,-y*0.02f);
		glVertex3f(   0.00f,y,-y*0.01f);
		glVertex3f(   0.00f,y, y*0.01f);
		glVertex3f(   0.00f,y, y*0.02f);
		glEnd();
		
		/* Draw the flight path marker: */
		Vector vel=currentOrientation.transform(currentVelocity);
		if(vel[1]>Scalar(0))
			{
			vel*=y/vel[1];
			Scalar maxVel=Misc::max(Math::abs(vel[0]),Math::abs(vel[2]));
			if(maxVel>=Scalar(y*0.5f))
				{
				vel[0]*=Scalar(y*0.5f)/maxVel;
				vel[2]*=Scalar(y*0.5f)/maxVel;
				glColor3f(1.0f,0.0f,0.0f);
				}
			else
				glColor3f(0.0f,1.0f,0.0f);
			
			glBegin(GL_LINE_LOOP);
			glVertex3f(vel[0]-y*0.005f,vel[1],vel[2]+  0.000f);
			glVertex3f(vel[0]+  0.000f,vel[1],vel[2]-y*0.005f);
			glVertex3f(vel[0]+y*0.005f,vel[1],vel[2]+  0.000f);
			glVertex3f(vel[0]+  0.000f,vel[1],vel[2]+y*0.005f);
			glEnd();
			}
		
		/* Draw the artificial horizon ribbon: */
		glRotate(currentOrientation);
		Vector yAxis=currentOrientation.inverseTransform(Vector(0,1,0));
		Scalar yAngle=Math::deg(Math::atan2(yAxis[0],yAxis[1]));
		glRotate(-yAngle,Vector(0,0,1));
		glCallList(dataItem->displayListBase+10);
		
		glPopMatrix();
		glPopAttrib();
		}
	}

void HelicopterNavigationTool::initContext(GLContextData& contextData) const
	{
	/* Create and register a data item: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Create the digit display lists: */
	float y=float(getFrontplaneDist())*1.25f;
	float s=y*0.0025f;
	
	glNewList(dataItem->displayListBase+0,GL_COMPILE);
	glBegin(GL_LINE_LOOP);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(0.0f*s,0.0f,0.0f*s);
	glVertex3f(1.0f*s,0.0f,0.0f*s);
	glVertex3f(1.0f*s,0.0f,2.0f*s);
	glEnd();
	glTranslatef(s*1.5f,0.0f,0.0f);
	glEndList();
	
	glNewList(dataItem->displayListBase+1,GL_COMPILE);
	glBegin(GL_LINES);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(0.0f*s,0.0f,0.0f*s);
	glEnd();
	glTranslatef(s*0.5f,0.0f,0.0f);
	glEndList();
	
	glNewList(dataItem->displayListBase+2,GL_COMPILE);
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(1.0f*s,0.0f,2.0f*s);
	glVertex3f(1.0f*s,0.0f,1.0f*s);
	glVertex3f(0.0f*s,0.0f,1.0f*s);
	glVertex3f(0.0f*s,0.0f,0.0f*s);
	glVertex3f(1.0f*s,0.0f,0.0f*s);
	glEnd();
	glTranslatef(s*1.5f,0.0f,0.0f);
	glEndList();
	
	glNewList(dataItem->displayListBase+3,GL_COMPILE);
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(1.0f*s,0.0f,2.0f*s);
	glVertex3f(1.0f*s,0.0f,0.0f*s);
	glVertex3f(0.0f*s,0.0f,0.0f*s);
	glEnd();
	glBegin(GL_LINES);
	glVertex3f(0.0f*s,0.0f,1.0f*s);
	glVertex3f(1.0f*s,0.0f,1.0f*s);
	glEnd();
	glTranslatef(s*1.5f,0.0f,0.0f);
	glEndList();
	
	glNewList(dataItem->displayListBase+4,GL_COMPILE);
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(0.0f*s,0.0f,1.0f*s);
	glVertex3f(1.0f*s,0.0f,1.0f*s);
	glEnd();
	glBegin(GL_LINES);
	glVertex3f(1.0f*s,0.0f,2.0f*s);
	glVertex3f(1.0f*s,0.0f,0.0f*s);
	glEnd();
	glTranslatef(s*1.5f,0.0f,0.0f);
	glEndList();
	
	glNewList(dataItem->displayListBase+5,GL_COMPILE);
	glBegin(GL_LINE_STRIP);
	glVertex3f(1.0f*s,0.0f,2.0f*s);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(0.0f*s,0.0f,1.0f*s);
	glVertex3f(1.0f*s,0.0f,1.0f*s);
	glVertex3f(1.0f*s,0.0f,0.0f*s);
	glVertex3f(0.0f*s,0.0f,0.0f*s);
	glEnd();
	glTranslatef(s*1.5f,0.0f,0.0f);
	glEndList();
	
	glNewList(dataItem->displayListBase+6,GL_COMPILE);
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(0.0f*s,0.0f,0.0f*s);
	glVertex3f(1.0f*s,0.0f,0.0f*s);
	glVertex3f(1.0f*s,0.0f,1.0f*s);
	glVertex3f(0.0f*s,0.0f,1.0f*s);
	glEnd();
	glTranslatef(s*1.5f,0.0f,0.0f);
	glEndList();
	
	glNewList(dataItem->displayListBase+7,GL_COMPILE);
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(1.0f*s,0.0f,2.0f*s);
	glVertex3f(1.0f*s,0.0f,0.0f*s);
	glEnd();
	glTranslatef(s*1.5f,0.0f,0.0f);
	glEndList();
	
	glNewList(dataItem->displayListBase+8,GL_COMPILE);
	glBegin(GL_LINE_LOOP);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(0.0f*s,0.0f,0.0f*s);
	glVertex3f(1.0f*s,0.0f,0.0f*s);
	glVertex3f(1.0f*s,0.0f,2.0f*s);
	glEnd();
	glBegin(GL_LINES);
	glVertex3f(0.0f*s,0.0f,1.0f*s);
	glVertex3f(1.0f*s,0.0f,1.0f*s);
	glEnd();
	glTranslatef(s*1.5f,0.0f,0.0f);
	glEndList();
	
	glNewList(dataItem->displayListBase+9,GL_COMPILE);
	glBegin(GL_LINE_STRIP);
	glVertex3f(1.0f*s,0.0f,1.0f*s);
	glVertex3f(0.0f*s,0.0f,1.0f*s);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(1.0f*s,0.0f,2.0f*s);
	glVertex3f(1.0f*s,0.0f,0.0f*s);
	glEnd();
	glTranslatef(s*1.5f,0.0f,0.0f);
	glEndList();
	
	/* Draw the entire artificial horizon ribbon: */
	glNewList(dataItem->displayListBase+10,GL_COMPILE);
	glColor3f(0.0f,1.0f,0.0f);
	
	/* Draw the climb ladder: */
	glBegin(GL_LINES);
	glVertex3f(-y*0.1f,y,0.0f);
	glVertex3f( y*0.1f,y,0.0f);
	glEnd();

	glEnable(GL_LINE_STIPPLE);
	glLineStipple(10,0xaaaaU);
	glBegin(GL_LINES);
	for(int i=0;i<9;++i)
		{
		float angle=Math::rad(float(i+1)*10.0f);
		float ac=Math::cos(angle)*y;
		float as=Math::sin(angle)*y;
		glVertex3f(-y*0.1f,ac, as);
		glVertex3f( y*0.1f,ac, as);
		glVertex3f(-y*0.1f,ac,-as);
		glVertex3f( y*0.1f,ac,-as);
		}
	for(int i=0;i<9;++i)
		{
		float angle=Math::rad(float(i*2+1)*5.0f);
		float ac=Math::cos(angle)*y;
		float as=Math::sin(angle)*y;
		glVertex3f(-y*0.075f,ac, as);
		glVertex3f( y*0.075f,ac, as);
		glVertex3f(-y*0.075f,ac,-as);
		glVertex3f( y*0.075f,ac,-as);
		}
	glEnd();
	glDisable(GL_LINE_STIPPLE);
	
	/* Draw the climb labels: */
	char buffer[10];
	for(int i=-9;i<=9;++i)
		{
		glPushMatrix();
		float angle=float(i)*10.0f;
		glRotatef(angle,1.0f,0.0f,0.0f);
		
		glTranslatef(y*0.105f,y,-s);
		snprintf(buffer,sizeof(buffer),"%d",Math::abs(i*10));
		for(char* bufPtr=buffer;*bufPtr!='\0';++bufPtr)
			glCallList(dataItem->displayListBase+(GLuint(*bufPtr)-GLuint('0')));
		glPopMatrix();
		}
	
	glEndList();
	}

}
