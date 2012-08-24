/***********************************************************************
SixAxisSurfaceNavigationTool - Class to convert an input device with six
valuators into a surface-aligned navigation tool.
Copyright (c) 2011-2012 Oliver Kreylos

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

#include <Vrui/Tools/SixAxisSurfaceNavigationTool.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLValueCoders.h>
#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/Viewer.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/****************************************************
Methods of class SixAxisSurfaceNavigationToolFactory:
****************************************************/

SixAxisSurfaceNavigationToolFactory::SixAxisSurfaceNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("SixAxisSurfaceNavigationTool",toolManager),
	 activationToggle(true),
	 canRoll(true),
	 bankTurns(false),levelSpeed(5),
	 canFly(true),
	 probeSize(getDisplaySize()),
	 maxClimb(getDisplaySize()),
	 fixAzimuth(false),
	 drawHud(true),hudColor(0.0f,1.0f,0.0f),
	 hudDist(Geometry::dist(getDisplayCenter(),getMainViewer()->getHeadPosition())),
	 hudRadius(getDisplaySize()),
	 hudFontSize(float(getUiSize())*1.5f)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	layout.setNumValuators(6);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	activationToggle=cfs.retrieveValue<bool>("./activationToggle",activationToggle);
	Vector trans;
	for(int i=0;i<3;++i)
		trans[i]=getDisplaySize();
	trans=cfs.retrieveValue<Vector>("./translateFactors",trans);
	for(int i=0;i<3;++i)
		translateFactors[i]=trans[i];
	Vector rot=cfs.retrieveValue<Vector>("./rotateFactors",Vector(180,180,180));
	for(int i=0;i<3;++i)
		rotateFactors[i]=Math::rad(rot[i]);
	canRoll=cfs.retrieveValue<bool>("./canRoll",canRoll);
	bankTurns=cfs.retrieveValue<bool>("./bankTurns",bankTurns);
	bankFactor=Math::rad(cfs.retrieveValue<Scalar>("./bankFactor",Scalar(60)));
	levelSpeed=cfs.retrieveValue<Scalar>("./levelSpeed",levelSpeed);
	if(levelSpeed<Scalar(0))
		levelSpeed=Scalar(0);
	canFly=cfs.retrieveValue<bool>("./canFly",canFly);
	probeSize=cfs.retrieveValue<Scalar>("./probeSize",probeSize);
	maxClimb=cfs.retrieveValue<Scalar>("./maxClimb",maxClimb);
	fixAzimuth=cfs.retrieveValue<bool>("./fixAzimuth",fixAzimuth);
	drawHud=cfs.retrieveValue<bool>("./drawHud",drawHud);
	hudColor=cfs.retrieveValue<Color>("./hudColor",hudColor);
	hudDist=cfs.retrieveValue<float>("./hudDist",hudDist);
	hudRadius=cfs.retrieveValue<float>("./hudRadius",hudRadius);
	hudFontSize=cfs.retrieveValue<float>("./hudFontSize",hudFontSize);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("SurfaceNavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Set tool class' factory pointer: */
	SixAxisSurfaceNavigationTool::factory=this;
	}

SixAxisSurfaceNavigationToolFactory::~SixAxisSurfaceNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	SixAxisSurfaceNavigationTool::factory=0;
	}

const char* SixAxisSurfaceNavigationToolFactory::getName(void) const
	{
	return "Six-Axis";
	}

const char* SixAxisSurfaceNavigationToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	return "Start / Stop";
	}

const char* SixAxisSurfaceNavigationToolFactory::getValuatorFunction(int valuatorSlotIndex) const
	{
	switch(valuatorSlotIndex)
		{
		case 0:
			return "Translate X";
		
		case 1:
			return "Translate Y";
		
		case 2:
			return "Translate Z";
		
		case 3:
			return "Rotate Z (Yaw)";
		
		case 4:
			return "Rotate X (Pitch)";
		
		case 5:
			return "Rotate Y (Roll)";
		}
	
	/* Never reached; just to make compiler happy: */
	return 0;
	}

Tool* SixAxisSurfaceNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new SixAxisSurfaceNavigationTool(this,inputAssignment);
	}

void SixAxisSurfaceNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveSixAxisSurfaceNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("SurfaceNavigationTool");
	}

extern "C" ToolFactory* createSixAxisSurfaceNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	SixAxisSurfaceNavigationToolFactory* sixAxisSurfaceNavigationToolFactory=new SixAxisSurfaceNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return sixAxisSurfaceNavigationToolFactory;
	}

extern "C" void destroySixAxisSurfaceNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/*****************************************************
Static elements of class SixAxisSurfaceNavigationTool:
*****************************************************/

SixAxisSurfaceNavigationToolFactory* SixAxisSurfaceNavigationTool::factory=0;

/*********************************************
Methods of class SixAxisSurfaceNavigationTool:
*********************************************/

void SixAxisSurfaceNavigationTool::applyNavState(void)
	{
	/* Compose and apply the navigation transformation: */
	NavTransform nav=physicalFrame;
	if(factory->canRoll||factory->bankTurns)
		nav*=NavTransform::rotate(Rotation::rotateY(angles[2])); // Roll
	nav*=NavTransform::rotate(Rotation::rotateX(angles[1])); // Pitch
	nav*=NavTransform::rotate(Rotation::rotateZ(angles[0])); // Yaw
	nav*=Geometry::invert(surfaceFrame);
	setNavigationTransformation(nav);
	}

void SixAxisSurfaceNavigationTool::initNavState(void)
	{
	/* Set up a physical navigation frame around the main viewer's current head position: */
	headPos=getMainViewer()->getHeadPosition();
	calcPhysicalFrame(headPos);
	
	/* Calculate the initial environment-aligned surface frame in navigation coordinates: */
	surfaceFrame=getInverseNavigationTransformation()*physicalFrame;
	NavTransform newSurfaceFrame=surfaceFrame;
	
	/* Align the initial frame with the application's surface: */
	AlignmentData ad(surfaceFrame,newSurfaceFrame,factory->probeSize,factory->maxClimb);
	align(ad,angles[0],angles[1],angles[2]);
	
	/* If flying is allowed and the initial surface frame was above the surface, lift it back up: */
	Scalar z=newSurfaceFrame.inverseTransform(surfaceFrame.getOrigin())[2];
	if(!factory->canFly||z<factory->probeSize)
		z=factory->probeSize;
	newSurfaceFrame*=NavTransform::translate(Vector(Scalar(0),Scalar(0),z));
	
	/* Apply the initial navigation state: */
	surfaceFrame=newSurfaceFrame;
	applyNavState();
	}

SixAxisSurfaceNavigationTool::SixAxisSurfaceNavigationTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:SurfaceNavigationTool(sFactory,inputAssignment),
	 numberRenderer(factory->hudFontSize,true)
	{
	}

const ToolFactory* SixAxisSurfaceNavigationTool::getFactory(void) const
	{
	return factory;
	}

void SixAxisSurfaceNavigationTool::buttonCallback(int,InputDevice::ButtonCallbackData* cbData)
	{
	bool newActive;
	if(factory->activationToggle)
		{
		newActive=isActive();
		if(cbData->newButtonState)
			newActive=!newActive;
		}
	else
		newActive=cbData->newButtonState;
	
	if(isActive())
		{
		if(!newActive)
			{
			/* Deactivate this tool: */
			deactivate();
			}
		}
	else
		{
		/* Try activating this tool: */
		if(newActive&&activate())
			{
			/* Initialize the navigation: */
			initNavState();
			}
		}
	}

void SixAxisSurfaceNavigationTool::frame(void)
	{
	/* Act depending on this tool's current state: */
	if(isActive())
		{
		/* Use the average frame time as simulation time: */
		Scalar dt=getCurrentFrameTime();
		
		/* Update rotation angles based on current rotation valuator states: */
		for(int i=0;i<3;++i)
			angles[i]=wrapAngle(angles[i]+getValuatorState(i+3)*factory->rotateFactors[i]*dt);
		if(angles[1]<Math::rad(Scalar(-90)))
			angles[1]=Math::rad(Scalar(-90));
		else if(angles[1]>Math::rad(Scalar(90)))
			angles[1]=Math::rad(Scalar(90));
		if(!factory->canRoll||factory->bankTurns)
			{
			Scalar targetRoll=factory->bankTurns?getValuatorState(3)*factory->bankFactor:Scalar(0);
			Scalar t=Math::exp(-factory->levelSpeed*dt);
			angles[2]=angles[2]*t+targetRoll*(Scalar(1)-t);
			if(Math::abs(angles[2]-targetRoll)<Scalar(1.0e-3))
				angles[2]=targetRoll;
			}
		
		/* Calculate the new head position: */
		Point newHeadPos=getMainViewer()->getHeadPosition();
		
		/* Create a physical navigation frame around the new foot position: */
		calcPhysicalFrame(newHeadPos);
		
		/* Calculate movement from head position change: */
		Vector move=newHeadPos-headPos;
		headPos=newHeadPos;
		
		/* Add movement velocity based on the current translation valuator states: */
		for(int i=0;i<3;++i)
			move[i]+=getValuatorState(i)*factory->translateFactors[i]*dt;
		
		/* Transform the movement vector from physical space to the physical navigation frame: */
		move=physicalFrame.inverseTransform(move);
		
		/* Rotate by the current azimuth and elevation angles: */
		move=Rotation::rotateX(-angles[1]).transform(move);
		move=Rotation::rotateZ(-angles[0]).transform(move);
		
		/* Move the surface frame: */
		NavTransform newSurfaceFrame=surfaceFrame;
		newSurfaceFrame*=NavTransform::translate(move);
		
		/* Re-align the surface frame with the surface: */
		Point initialOrigin=newSurfaceFrame.getOrigin();
		Rotation initialOrientation=newSurfaceFrame.getRotation();
		AlignmentData ad(surfaceFrame,newSurfaceFrame,factory->probeSize,factory->maxClimb);
		align(ad);
		
		if(!factory->fixAzimuth)
			{
			/* Have the azimuth angle track changes in the surface frame's rotation: */
			Rotation rot=Geometry::invert(initialOrientation)*newSurfaceFrame.getRotation();
			rot.leftMultiply(Rotation::rotateFromTo(rot.getDirection(2),Vector(0,0,1)));
			Vector x=rot.getDirection(0);
			angles[0]=wrapAngle(angles[0]+Math::atan2(x[1],x[0]));
			}
		
		/* If flying is allowed and the initial surface frame was above the surface, lift it back up: */
		Scalar z=newSurfaceFrame.inverseTransform(initialOrigin)[2];
		if(!factory->canFly||z<factory->probeSize)
			z=factory->probeSize;
		newSurfaceFrame*=NavTransform::translate(Vector(Scalar(0),Scalar(0),z));
		
		/* Apply the newly aligned surface frame: */
		surfaceFrame=newSurfaceFrame;
		applyNavState();
		
		/* Request another frame: */
		scheduleUpdate(getApplicationTime()+1.0/125.0);
		}
	}

void SixAxisSurfaceNavigationTool::display(GLContextData& contextData) const
	{
	if(isActive()&&factory->drawHud)
		{
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(1.0f);
		glColor(factory->hudColor);
		
		/* Get the HUD layout parameters: */
		float y=factory->hudDist;
		float r=factory->hudRadius;
		float s=factory->hudFontSize;
		
		/* Go to the physical frame: */
		glPushMatrix();
		glMultMatrix(physicalFrame);
		
		/* Go to the HUD frame: */
		glTranslatef(0.0f,y,0.0f);
		glRotatef(90.0f,1.0f,0.0f,0.0f);
		
		/* Draw the boresight crosshairs: */
		glBegin(GL_LINES);
		glVertex2f(-r*0.05f,   0.00f);
		glVertex2f(-r*0.02f,   0.00f);
		glVertex2f( r*0.02f,   0.00f);
		glVertex2f( r*0.05f,   0.00f);
		glVertex2f(   0.00f,-r*0.05f);
		glVertex2f(   0.00f,-r*0.02f);
		glVertex2f(   0.00f, r*0.02f);
		glVertex2f(   0.00f, r*0.05f);
		glEnd();
		
		/* Get the tool's orientation Euler angles in degrees: */
		float azimuth=Math::deg(angles[0]);
		float elevation=Math::deg(angles[1]);
		float roll=Math::deg(angles[2]);
		
		/* Draw the compass ribbon: */
		glBegin(GL_LINES);
		glVertex2f(-r,r);
		glVertex2f(r,r);
		glEnd();
		glBegin(GL_LINE_STRIP);
		glVertex2f(-s*0.5f,r+s);
		glVertex2f(0.0f,r);
		glVertex2f(s*0.5f,r+s);
		glEnd();
		
		/* Draw the azimuth tick marks: */
		glBegin(GL_LINES);
		for(int az=0;az<360;az+=10)
			{
			float dist=float(az)-azimuth;
			if(dist<-180.0f)
				dist+=360.0f;
			if(dist>180.0f)
				dist-=360.0f;
			if(Math::abs(dist)<=60.0f)
				{
				float x=dist*r/60.0f;
				glVertex2f(x,r);
				glVertex2f(x,r-(az%30==0?s*1.5f:s));
				}
			}
		glEnd();
		
		/* Draw the azimuth labels: */
		GLNumberRenderer::Vector pos;
		pos[1]=r-s*2.0f;
		pos[2]=0.0f;
		for(int az=0;az<360;az+=30)
			{
			float dist=float(az)-azimuth;
			if(dist<-180.0f)
				dist+=360.0f;
			if(dist>180.0f)
				dist-=360.0f;
			if(Math::abs(dist)<=60.0f)
				{
				pos[0]=dist*r/60.0f;
				numberRenderer.drawNumber(pos,az,contextData,0,1);
				}
			}
		
		glRotatef(-roll,0.0f,0.0f,1.0f);
		
		/* Draw the artificial horizon ladder: */
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(10,0xaaaaU);
		glBegin(GL_LINES);
		for(int el=-175;el<0;el+=5)
			{
			float dist=elevation+float(el);
			if(dist<-180.0f)
				dist+=360.0f;
			if(dist>180.0f)
				dist-=360.0f;
			if(Math::abs(dist)<90.0f)
				{
				float z=Math::tan(Math::rad(dist))*y;
				if(Math::abs(z)<=r)
					{
					float x=el%10==0?r*0.2f:r*0.1f;
					glVertex2f(-x,z);
					glVertex2f(x,z);
					}
				}
			}
		glEnd();
		glDisable(GL_LINE_STIPPLE);
		
		glBegin(GL_LINES);
		for(int el=0;el<=180;el+=5)
			{
			float dist=elevation+float(el);
			if(dist<-180.0f)
				dist+=360.0f;
			if(dist>180.0f)
				dist-=360.0f;
			if(Math::abs(dist)<90.0f)
				{
				float z=Math::tan(Math::rad(dist))*y;
				if(Math::abs(z)<=r)
					{
					float x=el%10==0?r*0.2f:r*0.1f;
					glVertex2f(-x,z);
					glVertex2f(x,z);
					}
				}
			}
		glEnd();
		
		/* Draw the artificial horizon labels: */
		pos[0]=r*0.2f+s;
		pos[2]=0.0f;
		for(int el=-170;el<=180;el+=10)
			{
			float dist=elevation+float(el);
			if(dist<-180.0f)
				dist+=360.0f;
			if(dist>180.0f)
				dist-=360.0f;
			if(Math::abs(dist)<90.0f)
				{
				float z=Math::tan(Math::rad(dist))*y;
				if(Math::abs(z)<=r)
					{
					pos[1]=z;
					int drawEl=el;
					if(drawEl>90)
						drawEl=180-el;
					else if(drawEl<-90)
						drawEl=-180-el;
					numberRenderer.drawNumber(pos,drawEl,contextData,-1,0);
					}
				}
			}
		
		glPopMatrix();
		glPopAttrib();
		}
	}

}
