/***********************************************************************
HelicopterNavigationTool - Class for navigation tools using a simplified
helicopter flight model, a la Enemy Territory: Quake Wars' Anansi. Yeah,
I like that -- wanna fight about it?
Copyright (c) 2007-2010 Oliver Kreylos

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

#include <Vrui/Tools/HelicopterNavigationTool.h>

#include <Misc/Utility.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/Viewer.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/************************************************
Methods of class HelicopterNavigationToolFactory:
************************************************/

HelicopterNavigationToolFactory::HelicopterNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("HelicopterNavigationTool",toolManager),
	 g(getMeterFactor()*Scalar(9.81)),
	 collectiveMin(Scalar(0)),collectiveMax(g*Scalar(1.5)),
	 thrust(g*Scalar(1)),
	 brake(g*Scalar(0.5)),
	 probeSize(getMeterFactor()*Scalar(1.5)),
	 maxClimb(getMeterFactor()*Scalar(1.5))
	{
	/* Initialize tool layout: */
	layout.setNumButtons(3);
	layout.setNumValuators(6);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	Vector rot=cfs.retrieveValue<Vector>("./rotateFactors",Vector(-60,-60,45));
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
	probeSize=cfs.retrieveValue<Scalar>("./probeSize",probeSize);
	maxClimb=cfs.retrieveValue<Scalar>("./maxClimb",maxClimb);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("SurfaceNavigationTool");
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

const char* HelicopterNavigationToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	switch(buttonSlotIndex)
		{
		case 0:
			return "Start / Stop";
		
		case 1:
			return "Thrusters";
		
		case 2:
			return "Brake";
		}
	
	/* Never reached; just to make compiler happy: */
	return 0;
	}

const char* HelicopterNavigationToolFactory::getValuatorFunction(int valuatorSlotIndex) const
	{
	switch(valuatorSlotIndex)
		{
		case 0:
			return "Cyclic Pitch";
		
		case 1:
			return "Cyclic Roll";
		
		case 2:
			return "Rudder Yaw";
		
		case 3:
			return "Collective";
		
		case 4:
			return "Look Left/Right";
		
		case 5:
			return "Look Up/Down";
		}
	
	/* Never reached; just to make compiler happy: */
	return 0;
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
	:ladderDisplayListId(glGenLists(1))
	{
	}

HelicopterNavigationTool::DataItem::~DataItem(void)
	{
	glDeleteLists(ladderDisplayListId,1);
	}

/*************************************************
Static elements of class HelicopterNavigationTool:
*************************************************/

HelicopterNavigationToolFactory* HelicopterNavigationTool::factory=0;

/*****************************************
Methods of class HelicopterNavigationTool:
*****************************************/

void HelicopterNavigationTool::applyNavState(void)
	{
	/* Compose and apply the navigation transformation: */
	NavTransform nav=physicalFrame;
	nav*=NavTransform::rotate(Rotation::rotateZ(getValuatorState(4)*factory->viewAngleFactors[0]));
	nav*=NavTransform::rotate(Rotation::rotateX(getValuatorState(5)*factory->viewAngleFactors[1]));
	nav*=NavTransform::rotate(orientation);
	nav*=Geometry::invert(surfaceFrame);
	setNavigationTransformation(nav);
	}

void HelicopterNavigationTool::initNavState(void)
	{
	/* Set up a physical navigation frame around the main viewer's current head position: */
	calcPhysicalFrame(getMainViewer()->getHeadPosition());
	
	/* Calculate the initial environment-aligned surface frame in navigation coordinates: */
	surfaceFrame=getInverseNavigationTransformation()*physicalFrame;
	NavTransform newSurfaceFrame=surfaceFrame;
	
	/* Align the initial frame with the application's surface: */
	AlignmentData ad(surfaceFrame,newSurfaceFrame,factory->probeSize,factory->maxClimb);
	align(ad);
	
	/* Calculate the orientation of the current navigation transformation in the aligned surface frame: */
	orientation=Geometry::invert(surfaceFrame.getRotation())*newSurfaceFrame.getRotation();
	
	/* Reset the movement velocity: */
	velocity=Vector::zero;
	
	/* If the initial surface frame was above the surface, lift it back up: */
	elevation=newSurfaceFrame.inverseTransform(surfaceFrame.getOrigin())[2];
	if(elevation>Scalar(-1.0e-4))
		newSurfaceFrame*=NavTransform::translate(Vector(Scalar(0),Scalar(0),elevation));
	else
		elevation=Scalar(0);
	
	/* Apply the initial navigation state: */
	surfaceFrame=newSurfaceFrame;
	applyNavState();
	}

HelicopterNavigationTool::HelicopterNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:SurfaceNavigationTool(factory,inputAssignment),
	 numberRenderer(float(getUiSize())*1.5f,true)
	{
	/* This object's GL state depends on the number renderer's GL state: */
	dependsOn(&numberRenderer);
	}

const ToolFactory* HelicopterNavigationTool::getFactory(void) const
	{
	return factory;
	}

void HelicopterNavigationTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	/* Process based on which button was pressed: */
	if(buttonSlotIndex==0)
		{
		if(cbData->newButtonState)
			{
			/* Act depending on this tool's current state: */
			if(isActive())
				{
				/* Deactivate this tool: */
				deactivate();
				}
			else
				{
				/* Try activating this tool: */
				if(activate())
					{
					/* Initialize the navigation: */
					initNavState();
					}
				}
			}
		}
	}

void HelicopterNavigationTool::frame(void)
	{
	/* Act depending on this tool's current state: */
	if(isActive())
		{
		/* Use the average frame time as simulation time: */
		Scalar dt=getCurrentFrameTime();
		
		/* Update the current position based on the current velocity: */
		NavTransform newSurfaceFrame=surfaceFrame;
		newSurfaceFrame*=NavTransform::translate(velocity*dt);
		
		/* Re-align the surface frame with the surface: */
		Point initialOrigin=newSurfaceFrame.getOrigin();
		Rotation initialOrientation=newSurfaceFrame.getRotation();
		AlignmentData ad(surfaceFrame,newSurfaceFrame,factory->probeSize,factory->maxClimb);
		align(ad);
		
		/* Update the orientation to reflect rotations in the surface frame: */
		orientation*=Geometry::invert(surfaceFrame.getRotation())*newSurfaceFrame.getRotation();
		
		/* Check if the initial surface frame was above the surface: */
		elevation=newSurfaceFrame.inverseTransform(initialOrigin)[2];
		if(elevation>Scalar(-1.0e-4))
			{
			/* Lift the aligned frame back up to the original altitude: */
			newSurfaceFrame*=NavTransform::translate(Vector(Scalar(0),Scalar(0),elevation));
			}
		else
			{
			/* Collide with the ground: */
			elevation=Scalar(0);
			velocity=Vector::zero;
			}
		
		/* Update the current orientation based on the pitch, roll, and yaw controls: */
		Vector rot;
		for(int i=0;i<3;++i)
			rot[i]=getValuatorState(i)*factory->rotateFactors[i];
		orientation.leftMultiply(Rotation::rotateScaledAxis(rot*dt));
		orientation.renormalize();
		
		/* Calculate the current acceleration based on gravity, collective, thrust, and brake: */
		Vector accel=Vector(0,0,-factory->g);
		Scalar collective=Scalar(0.5)*(Scalar(1)-getValuatorState(3))*(factory->collectiveMax-factory->collectiveMin)+factory->collectiveMin;
		accel+=orientation.inverseTransform(Vector(0,0,collective));
		if(getButtonState(1))
			accel+=orientation.inverseTransform(Vector(0,factory->thrust,0));
		if(getButtonState(2))
			accel+=orientation.inverseTransform(Vector(0,-factory->brake,0));
		
		/* Calculate drag: */
		Vector localVelocity=orientation.transform(velocity);
		Vector drag;
		for(int i=0;i<3;++i)
			drag[i]=localVelocity[i]*factory->dragCoefficients[i];
		accel+=orientation.inverseTransform(drag);
		
		/* Update the current velocity: */
		velocity+=accel*dt;
		
		/* Apply the newly aligned surface frame: */
		surfaceFrame=newSurfaceFrame;
		applyNavState();
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
		glMultMatrix(physicalFrame);
		glRotate(getValuatorState(4)*Math::deg(factory->viewAngleFactors[0]),Vector(0,0,1));
		glRotate(getValuatorState(5)*Math::deg(factory->viewAngleFactors[1]),Vector(1,0,0));
		
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
		Vector vel=orientation.transform(velocity);
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
		glRotate(orientation);
		Vector yAxis=orientation.inverseTransform(Vector(0,1,0));
		Scalar yAngle=Math::deg(Math::atan2(yAxis[0],yAxis[1]));
		glRotate(-yAngle,Vector(0,0,1));
		glCallList(dataItem->ladderDisplayListId);
		
		glPopMatrix();
		glPopAttrib();
		}
	}

void HelicopterNavigationTool::initContext(GLContextData& contextData) const
	{
	/* Create and register a data item: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Draw the entire artificial horizon ribbon: */
	glNewList(dataItem->ladderDisplayListId,GL_COMPILE);
	glColor3f(0.0f,1.0f,0.0f);
	
	float y=float(getFrontplaneDist())*1.25f;
	float s=y*0.0025f;
	
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
	for(int i=-9;i<=9;++i)
		{
		glPushMatrix();
		float angle=float(i)*10.0f;
		glRotatef(angle,1.0f,0.0f,0.0f);
		
		glTranslatef(y*0.105f,y,-s);
		glRotatef(90.0f,1.0f,0.0f,0.0f);
		numberRenderer.drawNumber(i*10,contextData);
		glPopMatrix();
		}
	
	glEndList();
	}

}
