/***********************************************************************
MultitouchFirstPersonNavigationTool - Tool class for surface-aligned
first-person navigation using a multitouch screen.
Copyright (c) 2015 Oliver Kreylos

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

#include <Vrui/Tools/MultitouchFirstPersonNavigationTool.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/Ray.h>
#include <Geometry/Plane.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLValueCoders.h>
#include <GL/GLContextData.h>
#include <GL/GLNumberRenderer.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/Viewer.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/**************************************************************************
Methods of class MultitouchFirstPersonNavigationToolFactory::Configuration:
**************************************************************************/

MultitouchFirstPersonNavigationToolFactory::Configuration::Configuration(void)
	:activationToggle(true),
	 rotateFactors(getDisplaySize()/Scalar(2)),
	 dollyFactor(getDisplaySize()/getUiSize()),
	 panFactors(getDisplaySize()/getUiSize()),
	 fallAcceleration(getMeterFactor()*Scalar(9.81)),
	 probeSize(getInchFactor()*Scalar(12)),
	 maxClimb(getInchFactor()*Scalar(12)),
	 fixAzimuth(false),
	 levelOnExit(false),
	 drawHud(true),hudColor(0.0f,1.0f,0.0f),
	 hudDist(Geometry::dist(getDisplayCenter(),getMainViewer()->getHeadPosition())),
	 hudRadius(getDisplaySize()*0.5f),
	 hudFontSize(float(getUiSize())*1.5f)
	{
	}

void MultitouchFirstPersonNavigationToolFactory::Configuration::load(const Misc::ConfigurationFileSection& cfs)
	{
	activationToggle=cfs.retrieveValue<bool>("./activationToggle",activationToggle);
	rotateFactors=cfs.retrieveValue<Misc::FixedArray<Scalar,2> >("./rotateFactors",rotateFactors);
	dollyFactor=cfs.retrieveValue<Scalar>("./dollyFactor",dollyFactor);
	panFactors=cfs.retrieveValue<Misc::FixedArray<Scalar,2> >("./panFactors",panFactors);
	fallAcceleration=cfs.retrieveValue<Scalar>("./fallAcceleration",fallAcceleration);
	probeSize=cfs.retrieveValue<Scalar>("./probeSize",probeSize);
	maxClimb=cfs.retrieveValue<Scalar>("./maxClimb",maxClimb);
	fixAzimuth=cfs.retrieveValue<bool>("./fixAzimuth",fixAzimuth);
	levelOnExit=cfs.retrieveValue<bool>("./levelOnExit",levelOnExit);
	drawHud=cfs.retrieveValue<bool>("./drawHud",drawHud);
	hudColor=cfs.retrieveValue<Color>("./hudColor",hudColor);
	hudDist=cfs.retrieveValue<float>("./hudDist",hudDist);
	hudRadius=cfs.retrieveValue<float>("./hudRadius",hudRadius);
	hudFontSize=cfs.retrieveValue<float>("./hudFontSize",hudFontSize);
	}

void MultitouchFirstPersonNavigationToolFactory::Configuration::save(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue<bool>("./activationToggle",activationToggle);
	cfs.storeValue<Misc::FixedArray<Scalar,2> >("./rotateFactors",rotateFactors);
	cfs.storeValue<Scalar>("./dollyFactor",dollyFactor);
	cfs.storeValue<Misc::FixedArray<Scalar,2> >("./panFactors",panFactors);
	cfs.storeValue<Scalar>("./fallAcceleration",fallAcceleration);
	cfs.storeValue<Scalar>("./probeSize",probeSize);
	cfs.storeValue<Scalar>("./maxClimb",maxClimb);
	cfs.storeValue<bool>("./fixAzimuth",fixAzimuth);
	cfs.storeValue<bool>("./levelOnExit",levelOnExit);
	cfs.storeValue<bool>("./drawHud",drawHud);
	cfs.storeValue<Color>("./hudColor",hudColor);
	cfs.storeValue<float>("./hudDist",hudDist);
	cfs.storeValue<float>("./hudRadius",hudRadius);
	cfs.storeValue<float>("./hudFontSize",hudFontSize);
	}

/***********************************************************
Methods of class MultitouchFirstPersonNavigationToolFactory:
***********************************************************/

MultitouchFirstPersonNavigationToolFactory::MultitouchFirstPersonNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("MultitouchFirstPersonNavigationTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(4);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("SurfaceNavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	config.load(toolManager.getToolClassSection(getClassName()));
	
	/* Set tool class' factory pointer: */
	MultitouchFirstPersonNavigationTool::factory=this;
	}

MultitouchFirstPersonNavigationToolFactory::~MultitouchFirstPersonNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	MultitouchFirstPersonNavigationTool::factory=0;
	}

const char* MultitouchFirstPersonNavigationToolFactory::getName(void) const
	{
	return "Multitouch First-Person Navigation";
	}

const char* MultitouchFirstPersonNavigationToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	switch(buttonSlotIndex)
		{
		case 0:
			return "Activate";
		
		case 1:
			return "Rotate/Dolly";
		
		case 2:
			return "Rotate/Dolly";
		
		case 3:
			return "Pan";
		}
	
	/* Never reached; just to make compiler happy: */
	return 0;
	}

Tool* MultitouchFirstPersonNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new MultitouchFirstPersonNavigationTool(this,inputAssignment);
	}

void MultitouchFirstPersonNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveMultitouchFirstPersonNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("SurfaceNavigationTool");
	}

extern "C" ToolFactory* createMultitouchFirstPersonNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	MultitouchFirstPersonNavigationToolFactory* multitouchFirstPersonNavigationToolFactory=new MultitouchFirstPersonNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return multitouchFirstPersonNavigationToolFactory;
	}

extern "C" void destroyMultitouchFirstPersonNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/************************************************************
Static elements of class MultitouchFirstPersonNavigationTool:
************************************************************/

MultitouchFirstPersonNavigationToolFactory* MultitouchFirstPersonNavigationTool::factory=0;

/****************************************************
Methods of class MultitouchFirstPersonNavigationTool:
****************************************************/

void MultitouchFirstPersonNavigationTool::applyNavState(void)
	{
	/* Compose and apply the navigation transformation: */
	NavTransform nav=physicalFrame;
	nav*=NavTransform::rotateAround(Point(0,0,headHeight),Rotation::rotateX(elevation));
	
	nav*=NavTransform::rotate(Rotation::rotateZ(azimuth));
	nav*=Geometry::invert(surfaceFrame);
	setNavigationTransformation(nav);
	}

void MultitouchFirstPersonNavigationTool::initNavState(void)
	{
	/* Calculate the main viewer's current head and foot positions: */
	Point headPos=getMainViewer()->getHeadPosition();
	footPos=projectToFloor(headPos);
	headHeight=Geometry::dist(headPos,footPos);
	
	/* Set up a physical navigation frame around the main viewer's current head position: */
	calcPhysicalFrame(headPos);
	
	/* Calculate the initial environment-aligned surface frame in navigation coordinates: */
	surfaceFrame=getInverseNavigationTransformation()*physicalFrame;
	NavTransform newSurfaceFrame=surfaceFrame;
	
	/* Align the initial frame with the application's surface and calculate Euler angles: */
	AlignmentData ad(surfaceFrame,newSurfaceFrame,config.probeSize,config.maxClimb);
	Scalar roll;
	align(ad,azimuth,elevation,roll);
	
	/* Reset the movement velocity: */
	controlVelocity=Vector::zero;
	fallVelocity=Scalar(0);
	
	/* If the initial surface frame was above the surface, lift it back up and start falling: */
	footHeight=newSurfaceFrame.inverseTransform(surfaceFrame.getOrigin())[2];
	if(footHeight>Scalar(0))
		{
		newSurfaceFrame*=NavTransform::translate(Vector(Scalar(0),Scalar(0),footHeight));
		if(lockToGround)
			fallVelocity-=config.fallAcceleration*getCurrentFrameTime();
		}
	
	/* Initialize the navigation mode to idle while no other buttons are pressed: */
	navigationMode=IDLE;
	
	/* Move the physical frame to the foot position, and adjust the surface frame accordingly: */
	newSurfaceFrame*=Geometry::invert(physicalFrame)*NavTransform::translate(footPos-headPos)*physicalFrame;
	physicalFrame.leftMultiply(NavTransform::translate(footPos-headPos));
	
	/* Apply the initial navigation state: */
	surfaceFrame=newSurfaceFrame;
	applyNavState();
	}

void MultitouchFirstPersonNavigationTool::stopNavState(void)
	{
	if(config.levelOnExit)
		{
		/* Calculate the main viewer's current head and foot positions: */
		Point headPos=getMainViewer()->getHeadPosition();
		footPos=projectToFloor(headPos);
		headHeight=Geometry::dist(headPos,footPos);
		
		/* Reset the elevation angle: */
		elevation=Scalar(0);
		
		/* Apply the final navigation state: */
		applyNavState();
		}
	}

MultitouchFirstPersonNavigationTool::MultitouchFirstPersonNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:SurfaceNavigationTool(factory,inputAssignment),
	 config(MultitouchFirstPersonNavigationTool::factory->config),
	 numberRenderer(0),
	 lockToGround(false)
	{
	}

MultitouchFirstPersonNavigationTool::~MultitouchFirstPersonNavigationTool(void)
	{
	}

void MultitouchFirstPersonNavigationTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override the current configuration from the given configuration file section: */
	config.load(configFileSection);
	}

void MultitouchFirstPersonNavigationTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Save the current configuration to the given configuration file section: */
	config.save(configFileSection);
	}

void MultitouchFirstPersonNavigationTool::initialize(void)
	{
	/* Create the number renderer: */
	numberRenderer=new GLNumberRenderer(config.hudFontSize,true);
	}

void MultitouchFirstPersonNavigationTool::deinitialize(void)
	{
	/* Destroy the number renderer: */
	delete numberRenderer;
	numberRenderer=0;
	}

const ToolFactory* MultitouchFirstPersonNavigationTool::getFactory(void) const
	{
	return factory;
	}

void MultitouchFirstPersonNavigationTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	if(buttonSlotIndex==0)
		{
		/* Determine the new activation state of the tool: */
		bool newActive;
		if(config.activationToggle)
			{
			newActive=isActive();
			if(cbData->newButtonState)
				newActive=!newActive;
			}
		else
			newActive=cbData->newButtonState;
		
		/* Activate or deactivate the tool: */
		if(isActive())
			{
			if(!newActive)
				{
				/* Deactivate this tool: */
				stopNavState();
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
	else if(isActive())
		{
		if(cbData->newButtonState)
			{
			switch(navigationMode)
				{
				case IDLE:
					if(buttonSlotIndex==1||buttonSlotIndex==2) // User wants to rotate
						{
						if(Geometry::dist(getButtonDevicePosition(buttonSlotIndex),Point(0,0,-2.8))<Scalar(3)*getUiSize())
							{
							lockToGround=!lockToGround;
							if(lockToGround&&footHeight>Scalar(0))
								fallVelocity-=config.fallAcceleration*getCurrentFrameTime();
							}
						else
							{
							navigationMode=ROTATING;
							rotatingButtonSlotIndex=buttonSlotIndex;
							lastRotationPos=getButtonDevicePosition(rotatingButtonSlotIndex);
							}
						}
					else if(buttonSlotIndex==3) // User wants to pan
						{
						navigationMode=PANNING;
						lastPanningPos=getButtonDevicePosition(3);
						}
					break;
				
				case ROTATING:
					if(buttonSlotIndex==1||buttonSlotIndex==2) // User wants to dolly
						{
						navigationMode=DOLLYING;
						lastDollyingDist=Geometry::dist(getButtonDevicePosition(1),getButtonDevicePosition(2));
						}
					break;
				
				default:
					/* Do nothing: */
					;
				}
			}
		else
			{
			switch(navigationMode)
				{
				case ROTATING:
					if(buttonSlotIndex==rotatingButtonSlotIndex) // User wants to stop rotating
						{
						navigationMode=IDLE;
						controlVelocity=Vector::zero;
						}
					break;
				
				case DOLLYING:
					if(buttonSlotIndex==1||buttonSlotIndex==2) // User wants to stop dollying and go back to rotating
						{
						navigationMode=ROTATING;
						rotatingButtonSlotIndex=3-buttonSlotIndex; // Use the other button, which is still press, to continue rotating
						lastRotationPos=getButtonDevicePosition(rotatingButtonSlotIndex);
						controlVelocity*=Scalar(0.5);
						}
					break;
				
				case PANNING:
					if(buttonSlotIndex==3) // User wants to stop panning
						{
						navigationMode=IDLE;
						controlVelocity=Vector::zero;
						}
					break;
				
				default:
					/* Do nothing: */
					;
				}
			}
		}
	}

void MultitouchFirstPersonNavigationTool::frame(void)
	{
	/* Act depending on this tool's current state: */
	if(isActive())
		{
		bool update=false;
		
		switch(navigationMode)
			{
			case IDLE:
				break;
			
			case ROTATING:
				{
				/* Calculate the rotation device's displacement: */
				Point newRotationPos=getButtonDevicePosition(rotatingButtonSlotIndex);
				Vector rotation=newRotationPos-lastRotationPos;
				
				/* Calculate the device's left/right and up/down displacement components: */
				Vector right=getForwardDirection()^getUpDirection();
				right.normalize();
				Scalar x=right*rotation;
				Scalar y=getUpDirection()*rotation;
				if(x!=Scalar(0)||y!=Scalar(0))
					{
					/* Update the azimuth angle: */
					if(config.rotateFactors[0]!=Scalar(0))
						azimuth=wrapAngle(azimuth+x/config.rotateFactors[0]);
					
					/* Update the elevation angle: */
					if(config.rotateFactors[1]!=Scalar(0))
						{
						Scalar zenith=Math::rad(Scalar(90));
						elevation=Math::clamp(elevation+y/config.rotateFactors[1],-zenith,zenith);
						}
					
					update=true;
					}
				
				lastRotationPos=newRotationPos;
				break;
				}
			
			case DOLLYING:
				{
				/* Calculate the change in pinch gesture scale: */
				Scalar newDollyingDist=Geometry::dist(getButtonDevicePosition(1),getButtonDevicePosition(2));
				
				/* Convert the scale to a linear displacement along the viewing direction: */
				Scalar dolly=(newDollyingDist-lastDollyingDist)*config.dollyFactor;
				controlVelocity[0]=Scalar(0);
				if(lockToGround)
					{
					controlVelocity[1]=dolly;
					controlVelocity[2]=Scalar(0);
					}
				else
					{
					controlVelocity[1]=Math::cos(elevation)*dolly;
					controlVelocity[2]=-Math::sin(elevation)*dolly;
					}
				
				lastDollyingDist=newDollyingDist;
				break;
				}
			
			case PANNING:
				{
				/* Calculate the panning device's displacement: */
				Point newPanningPos=getButtonDevicePosition(3);
				Vector panning=newPanningPos-lastPanningPos;
				
				/* Calculate the device's left/right and up/down displacement components: */
				Vector right=getForwardDirection()^getUpDirection();
				right.normalize();
				Scalar x=right*panning;
				Scalar y=getUpDirection()*panning;
				if(x!=Scalar(0)||y!=Scalar(0))
					{
					controlVelocity[0]=x*config.panFactors[0];
					if(lockToGround)
						{
						controlVelocity[1]=Scalar(0);
						controlVelocity[2]=Scalar(0);
						}
					else
						{
						controlVelocity[1]=Math::sin(elevation)*config.panFactors[1];
						controlVelocity[2]=Math::cos(elevation)*config.panFactors[1];
						}
					}
				
				lastPanningPos=newPanningPos;
				break;
				}
			}
		
		/* Calculate the new head and foot positions: */
		Point newHeadPos=getMainViewer()->getHeadPosition();
		Point newFootPos=projectToFloor(newHeadPos);
		headHeight=Geometry::dist(newHeadPos,newFootPos);
		
		/* Check for movement: */
		update=update||controlVelocity!=Vector::zero||fallVelocity!=Scalar(0)||newFootPos!=footPos;
		
		if(update)
			{
			/* Create a physical navigation frame around the new foot position: */
			calcPhysicalFrame(newFootPos);
			
			/* Calculate the movement from walking: */
			Vector move=newFootPos-footPos;
			footPos=newFootPos;
			
			/* Calculate induced movement velocity based on controls and falling velocity: */
			Vector moveVelocity=controlVelocity;
			moveVelocity[2]+=fallVelocity;
			
			/* Add movement velocity: */
			move+=moveVelocity*getCurrentFrameTime();
			
			/* Transform the movement vector from physical space to the physical navigation frame: */
			move=physicalFrame.inverseTransform(move);
			
			/* Rotate by the current azimuth angle: */
			move=Rotation::rotateZ(-azimuth).transform(move);
			
			/* Move the surface frame: */
			NavTransform newSurfaceFrame=surfaceFrame;
			newSurfaceFrame*=NavTransform::translate(move);
			
			/* Re-align the surface frame with the surface: */
			Point initialOrigin=newSurfaceFrame.getOrigin();
			Rotation initialOrientation=newSurfaceFrame.getRotation();
			AlignmentData ad(surfaceFrame,newSurfaceFrame,config.probeSize,config.maxClimb);
			align(ad);
			
			if(!config.fixAzimuth)
				{
				/* Have the azimuth angle track changes in the surface frame's rotation: */
				Rotation rot=Geometry::invert(initialOrientation)*newSurfaceFrame.getRotation();
				rot.leftMultiply(Rotation::rotateFromTo(rot.getDirection(2),Vector(0,0,1)));
				Vector x=rot.getDirection(0);
				azimuth=wrapAngle(azimuth+Math::atan2(x[1],x[0]));
				}
			
			/* Check if the initial surface frame is above the surface: */
			footHeight=newSurfaceFrame.inverseTransform(initialOrigin)[2];
			if(footHeight>Scalar(0))
				{
				/* Lift the aligned frame back up to the original altitude and fall: */
				newSurfaceFrame*=NavTransform::translate(Vector(Scalar(0),Scalar(0),footHeight));
				if(lockToGround)
					fallVelocity-=config.fallAcceleration*getCurrentFrameTime();
				}
			else
				{
				/* Stop falling: */
				fallVelocity=Scalar(0);
				}
			
			/* Apply the newly aligned surface frame: */
			surfaceFrame=newSurfaceFrame;
			applyNavState();
			
			if(moveVelocity[0]!=Scalar(0)||moveVelocity[1]!=Scalar(0)||(lockToGround&&footHeight>Scalar(0)))
				{
				/* Request another frame: */
				scheduleUpdate(getNextAnimationTime());
				}
			}
		}
	}

void MultitouchFirstPersonNavigationTool::display(GLContextData& contextData) const
	{
	if(isActive()&&config.drawHud)
		{
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glDepthRange(0.0,0.0);
		glLineWidth(1.0f);
		glColor(config.hudColor);
		
		/* Get the HUD layout parameters: */
		float y=config.hudDist;
		float r=config.hudRadius;
		float s=config.hudFontSize;
		
		/* Go to the physical frame: */
		glPushMatrix();
		glMultMatrix(physicalFrame);
		
		/* Go to the HUD frame: */
		glTranslatef(0.0f,y,float(headHeight));
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
		
		/* Draw the ground locking icon: */
		glBegin(GL_LINES);
		if(lockToGround)
			{
			glVertex2f(   0.00f,-r*0.92f);
			glVertex2f(   0.00f,-r*1.00f);
			glVertex2f(-r*0.03f,-r*0.97f);
			glVertex2f(   0.00f,-r*1.00f);
			glVertex2f(   0.00f,-r*1.00f);
			glVertex2f( r*0.03f,-r*0.97f);
			glVertex2f(-r*0.03f,-r*1.00f);
			glVertex2f( r*0.03f,-r*1.00f);
			}
		else
			{
			glVertex2f(   0.00f,-r*0.89f);
			glVertex2f(   0.00f,-r*0.97f);
			glVertex2f(-r*0.03f,-r*0.94f);
			glVertex2f(   0.00f,-r*0.97f);
			glVertex2f(   0.00f,-r*0.97f);
			glVertex2f( r*0.03f,-r*0.94f);
			glVertex2f(-r*0.03f,-r*1.00f);
			glVertex2f( r*0.03f,-r*1.00f);
			}
		glEnd();
		
		/* Get the tool's orientation Euler angles in degrees: */
		float azimuthDeg=Math::deg(azimuth);
		
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
			float dist=float(az)-azimuthDeg;
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
			float dist=float(az)-azimuthDeg;
			if(dist<-180.0f)
				dist+=360.0f;
			if(dist>180.0f)
				dist-=360.0f;
			if(Math::abs(dist)<=60.0f)
				{
				pos[0]=dist*r/60.0f;
				numberRenderer->drawNumber(pos,az,contextData,0,1);
				}
			}
		
		glPopMatrix();
		glDepthRange(0.0,1.0);
		glPopAttrib();
		}
	}

}
