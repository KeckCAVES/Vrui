/***********************************************************************
MouseNavigationTool - Class encapsulating the navigation behaviour of a
mouse in the OpenInventor SoXtExaminerViewer.
Copyright (c) 2004-2015 Oliver Kreylos

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

#include <Vrui/Tools/MouseNavigationTool.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRWindow.h>
#include <Vrui/UIManager.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/**********************************************************
Methods of class MouseNavigationToolFactory::Configuration:
**********************************************************/

MouseNavigationToolFactory::Configuration::Configuration(void)
	:rotatePlaneOffset(getDisplaySize()/Scalar(4)),
	 rotateFactor(getDisplaySize()/Scalar(4)),
	 invertDolly(false),
	 dollyCenter(true),scaleCenter(true),
	 dollyingDirection(-getUpDirection()),
	 scalingDirection(-getUpDirection()),
	 dollyFactor(Scalar(1)),
	 scaleFactor(getDisplaySize()/Scalar(4)),
	 wheelDollyFactor(-getDisplaySize()),
	 wheelScaleFactor(Scalar(0.5)),
	 spinThreshold(getUiSize()*Scalar(1)),
	 showScreenCenter(true)
	{
	}

void MouseNavigationToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	rotatePlaneOffset=cfs.retrieveValue<Scalar>("./rotatePlaneOffset",rotatePlaneOffset);
	rotateFactor=cfs.retrieveValue<Scalar>("./rotateFactor",rotateFactor);
	invertDolly=cfs.retrieveValue<bool>("./invertDolly",invertDolly);
	dollyCenter=cfs.retrieveValue<bool>("./dollyCenter",dollyCenter);
	scaleCenter=cfs.retrieveValue<bool>("./scaleCenter",scaleCenter);
	dollyingDirection=cfs.retrieveValue<Vector>("./dollyingDirection",dollyingDirection);
	scalingDirection=cfs.retrieveValue<Vector>("./scalingDirection",scalingDirection);
	dollyFactor=cfs.retrieveValue<Scalar>("./dollyFactor",dollyFactor);
	scaleFactor=cfs.retrieveValue<Scalar>("./scaleFactor",scaleFactor);
	wheelDollyFactor=cfs.retrieveValue<Scalar>("./wheelDollyFactor",wheelDollyFactor);
	wheelScaleFactor=cfs.retrieveValue<Scalar>("./wheelScaleFactor",wheelScaleFactor);
	spinThreshold=cfs.retrieveValue<Scalar>("./spinThreshold",spinThreshold);
	showScreenCenter=cfs.retrieveValue<bool>("./showScreenCenter",showScreenCenter);
	}

void MouseNavigationToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue<Scalar>("./rotatePlaneOffset",rotatePlaneOffset);
	cfs.storeValue<Scalar>("./rotateFactor",rotateFactor);
	cfs.storeValue<bool>("./invertDolly",invertDolly);
	cfs.storeValue<bool>("./dollyCenter",dollyCenter);
	cfs.storeValue<bool>("./scaleCenter",scaleCenter);
	cfs.storeValue<Vector>("./dollyingDirection",dollyingDirection);
	cfs.storeValue<Vector>("./scalingDirection",scalingDirection);
	cfs.storeValue<Scalar>("./dollyFactor",dollyFactor);
	cfs.storeValue<Scalar>("./scaleFactor",scaleFactor);
	cfs.storeValue<Scalar>("./wheelDollyFactor",wheelDollyFactor);
	cfs.storeValue<Scalar>("./wheelScaleFactor",wheelScaleFactor);
	cfs.storeValue<Scalar>("./spinThreshold",spinThreshold);
	cfs.storeValue<bool>("./showScreenCenter",showScreenCenter);
	}

/*******************************************
Methods of class MouseNavigationToolFactory:
*******************************************/

MouseNavigationToolFactory::MouseNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("MouseNavigationTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(3);
	layout.setNumValuators(1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("NavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	MouseNavigationTool::factory=this;
	}

MouseNavigationToolFactory::~MouseNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	MouseNavigationTool::factory=0;
	}

const char* MouseNavigationToolFactory::getName(void) const
	{
	return "Mouse (Multiple Buttons)";
	}

const char* MouseNavigationToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	switch(buttonSlotIndex)
		{
		case 0:
			return "Rotate";
		
		case 1:
			return "Pan";
		
		case 2:
			return "Zoom/Dolly Switch";
		}
	
	/* Never reached; just to make compiler happy: */
	return 0;
	}

const char* MouseNavigationToolFactory::getValuatorFunction(int valuatorSlotIndex) const
	{
	switch(valuatorSlotIndex)
		{
		case 0:
			return "Quick Zoom/Dolly";
		}
	
	/* Never reached; just to make compiler happy: */
	return 0;
	}

Tool* MouseNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new MouseNavigationTool(this,inputAssignment);
	}

void MouseNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveMouseNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("NavigationTool");
	}

extern "C" ToolFactory* createMouseNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	MouseNavigationToolFactory* mouseNavigationToolFactory=new MouseNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return mouseNavigationToolFactory;
	}

extern "C" void destroyMouseNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/********************************************
Static elements of class MouseNavigationTool:
********************************************/

MouseNavigationToolFactory* MouseNavigationTool::factory=0;

/************************************
Methods of class MouseNavigationTool:
************************************/

void MouseNavigationTool::startNavigating(void)
	{
	/* Calculate the rotation center: */
	screenCenter=getDisplayCenter();
	
	/* Set up the interaction plane: */
	interactionPlane=getUiManager()->calcUITransform(screenCenter);
	
	/* Project the rotation center into the interaction plane: */
	screenCenter=interactionPlane.getOrigin();
	}

Point MouseNavigationTool::calcInteractionPos(void) const
	{
	/* Intersect the device's pointing ray with the widget plane: */
	Point deviceRayStart=getButtonDevicePosition(0);
	Vector deviceRayDir=getButtonDeviceRayDirection(0);
	
	Point planeCenter=interactionPlane.getOrigin();
	Vector planeNormal=interactionPlane.getDirection(2);
	Scalar lambda=((planeCenter-deviceRayStart)*planeNormal)/(deviceRayDir*planeNormal);
	return deviceRayStart+deviceRayDir*lambda;
	}

void MouseNavigationTool::startRotating(void)
	{
	startNavigating();
	
	/* Calculate initial rotation position: */
	lastRotationPos=calcInteractionPos();
	
	/* Calculate the rotation offset vector: */
	rotateOffset=interactionPlane.transform(Vector(0,0,configuration.rotatePlaneOffset));
	
	preScale=NavTrackerState::translateFromOriginTo(screenCenter);
	rotation=NavTrackerState::identity;
	postScale=NavTrackerState::translateToOriginFrom(screenCenter);
	postScale*=getNavigationTransformation();
	
	/* Go to rotating mode: */
	navigationMode=ROTATING;
	}

void MouseNavigationTool::startPanning(void)
	{
	startNavigating();
	
	/* Calculate initial motion position: */
	motionStart=calcInteractionPos();
	
	preScale=getNavigationTransformation();
	
	/* Go to panning mode: */
	navigationMode=PANNING;
	}

void MouseNavigationTool::startDollying(void)
	{
	startNavigating();
	
	/* Calculate the dollying direction: */
	if(configuration.dollyCenter)
		dollyDirection=-getForwardDirection();
	else
		dollyDirection=-getButtonDeviceRayDirection(0);
	
	/* Calculate initial motion position: */
	motionStart=calcInteractionPos();
	
	preScale=getNavigationTransformation();
	
	/* Go to dollying mode: */
	navigationMode=DOLLYING;
	}

void MouseNavigationTool::startScaling(void)
	{
	startNavigating();
	
	/* Calculate initial motion position: */
	motionStart=calcInteractionPos();
	
	if(configuration.scaleCenter)
		{
		preScale=NavTrackerState::translateFromOriginTo(screenCenter);
		postScale=NavTrackerState::translateToOriginFrom(screenCenter);
		}
	else
		{
		preScale=NavTrackerState::translateFromOriginTo(motionStart);
		postScale=NavTrackerState::translateToOriginFrom(motionStart);
		}
	postScale*=getNavigationTransformation();
	
	/* Go to scaling mode: */
	navigationMode=SCALING;
	}

MouseNavigationTool::MouseNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(factory,inputAssignment),
	 configuration(MouseNavigationTool::factory->configuration),
	 currentPos(Point::origin),currentValue(0),
	 dolly(configuration.invertDolly),navigationMode(IDLE)
	{
	}

void MouseNavigationTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override private configuration data from given configuration file section: */
	configuration.read(configFileSection);
	}

void MouseNavigationTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write private configuration data to given configuration file section: */
	configuration.write(configFileSection);
	}

const ToolFactory* MouseNavigationTool::getFactory(void) const
	{
	return factory;
	}

void MouseNavigationTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	/* Process based on which button was pressed: */
	switch(buttonSlotIndex)
		{
		case 0:
			if(cbData->newButtonState) // Button has just been pressed
				{
				/* Act depending on this tool's current state: */
				switch(navigationMode)
					{
					case IDLE:
					case SPINNING:
						/* Try activating this tool: */
						if(navigationMode==SPINNING||activate())
							startRotating();
						break;
					
					case PANNING:
						if(dolly)
							startDollying();
						else
							startScaling();
						break;
					
					default:
						/* This shouldn't happen; just ignore the event */
						break;
					}
				}
			else // Button has just been released
				{
				/* Act depending on this tool's current state: */
				switch(navigationMode)
					{
					case ROTATING:
						{
						/* Check if the input device is still moving: */
						Point currentPos=calcInteractionPos();
						Vector delta=currentPos-lastRotationPos;
						if(Geometry::mag(delta)>configuration.spinThreshold)
							{
							/* Calculate spinning angular velocity: */
							Vector offset=(lastRotationPos-screenCenter)+rotateOffset;
							Vector axis=offset^delta;
							Scalar angularVelocity=Geometry::mag(delta)/(configuration.rotateFactor*(getApplicationTime()-lastMoveTime));
							spinAngularVelocity=axis*(Scalar(0.5)*angularVelocity/axis.mag());
							
							/* Go to spinning mode: */
							navigationMode=SPINNING;
							}
						else
							{
							/* Deactivate this tool: */
							deactivate();
							
							/* Go to idle mode: */
							navigationMode=IDLE;
							}
						break;
						}
					
					case DOLLYING:
					case SCALING:
						startPanning();
						break;
					
					default:
						/* This shouldn't happen; just ignore the event */
						break;
					}
				}
			break;
		
		case 1:
			if(cbData->newButtonState) // Button has just been pressed
				{
				/* Act depending on this tool's current state: */
				switch(navigationMode)
					{
					case IDLE:
					case SPINNING:
						/* Try activating this tool: */
						if(navigationMode==SPINNING||activate())
							startPanning();
						break;
					
					case ROTATING:
						if(dolly)
							startDollying();
						else
							startScaling();
						break;
					
					default:
						/* This shouldn't happen; just ignore the event */
						break;
					}
				}
			else // Button has just been released
				{
				/* Act depending on this tool's current state: */
				switch(navigationMode)
					{
					case PANNING:
						/* Deactivate this tool: */
						deactivate();
						
						/* Go to idle mode: */
						navigationMode=IDLE;
						break;
					
					case DOLLYING:
					case SCALING:
						startRotating();
						break;
					
					default:
						/* This shouldn't happen; just ignore the event */
						break;
					}
				}
			break;
		
		case 2:
			/* Set the dolly flag: */
			dolly=cbData->newButtonState;
			if(configuration.invertDolly)
				dolly=!dolly;
			if(dolly) // Dollying has just been enabled
				{
				/* Act depending on this tool's current state: */
				switch(navigationMode)
					{
					case SCALING:
						startDollying();
						break;
					
					default:
						/* Nothing to do */
						break;
					}
				}
			else
				{
				/* Act depending on this tool's current state: */
				switch(navigationMode)
					{
					case DOLLYING:
						startScaling();
						break;
					
					default:
						/* Nothing to do */
						break;
					}
				}
			break;
		}
	}

void MouseNavigationTool::valuatorCallback(int,InputDevice::ValuatorCallbackData* cbData)
	{
	currentValue=Scalar(cbData->newValuatorValue);
	if(currentValue!=Scalar(0))
		{
		/* Act depending on this tool's current state: */
		switch(navigationMode)
			{
			case IDLE:
			case SPINNING:
				/* Try activating this tool: */
				if(navigationMode==SPINNING||activate())
					{
					if(dolly)
						{
						/* Start normal dollying: */
						startDollying();
						
						/* Change to wheel dollying mode: */
						currentWheelScale=Scalar(1);
						navigationMode=DOLLYING_WHEEL;
						}
					else
						{
						/* Start normal scaling: */
						startScaling();
						
						/* Change to wheel scaling mode: */
						currentWheelScale=Scalar(1);
						navigationMode=SCALING_WHEEL;
						}
					}
				break;
			
			default:
				/* This can definitely happen; just ignore the event */
				break;
			}
		}
	else
		{
		/* Act depending on this tool's current state: */
		switch(navigationMode)
			{
			case DOLLYING_WHEEL:
			case SCALING_WHEEL:
				/* Deactivate this tool: */
				deactivate();
				
				/* Go to idle mode: */
				navigationMode=IDLE;
				break;
			
			default:
				/* This can definitely happen; just ignore the event */
				break;
			}
		}
	}

void MouseNavigationTool::frame(void)
	{
	/* Update the current mouse position: */
	Point newCurrentPos=calcInteractionPos();
	if(currentPos!=newCurrentPos)
		{
		currentPos=newCurrentPos;
		lastMoveTime=getApplicationTime();
		}
	
	/* Act depending on this tool's current state: */
	switch(navigationMode)
		{
		case ROTATING:
			{
			/* Calculate the rotation position: */
			Vector offset=(lastRotationPos-screenCenter)+rotateOffset;
			
			/* Calculate mouse displacement vector: */
			Point rotationPos=currentPos;
			Vector delta=rotationPos-lastRotationPos;
			lastRotationPos=rotationPos;
			
			/* Calculate incremental rotation: */
			Vector axis=offset^delta;
			Scalar angle=Geometry::mag(delta)/configuration.rotateFactor;
			if(angle!=Scalar(0))
				rotation.leftMultiply(NavTrackerState::rotate(NavTrackerState::Rotation::rotateAxis(axis,angle)));
			
			NavTrackerState t=preScale;
			t*=rotation;
			t*=postScale;
			setNavigationTransformation(t);
			break;
			}
		
		case SPINNING:
			{
			/* Calculate incremental rotation: */
			rotation.leftMultiply(NavTrackerState::rotate(NavTrackerState::Rotation::rotateScaledAxis(spinAngularVelocity*getFrameTime())));
			
			NavTrackerState t=preScale;
			t*=rotation;
			t*=postScale;
			setNavigationTransformation(t);
			
			/* Request another frame: */
			scheduleUpdate(getNextAnimationTime());
			
			break;
			}
		
		case PANNING:
			{
			/* Update the navigation transformation: */
			NavTrackerState t=NavTrackerState::translate(currentPos-motionStart);
			t*=preScale;
			setNavigationTransformation(t);
			break;
			}
		
		case DOLLYING:
			{
			/* Update the navigation transformation: */
			Scalar dollyDist=((currentPos-motionStart)*configuration.dollyingDirection)/configuration.dollyFactor;
			NavTrackerState t=NavTrackerState::translate(dollyDirection*dollyDist);
			t*=preScale;
			setNavigationTransformation(t);
			break;
			}
		
		case SCALING:
			{
			/* Update the navigation transformation: */
			Scalar scale=((currentPos-motionStart)*configuration.scalingDirection)/configuration.scaleFactor;
			NavTrackerState t=preScale;
			t*=NavTrackerState::scale(Math::exp(scale));
			t*=postScale;
			setNavigationTransformation(t);
			break;
			}
		
		case DOLLYING_WHEEL:
			{
			/* Update the navigation transformation: */
			Scalar scale=currentValue;
			currentWheelScale+=configuration.wheelDollyFactor*scale;
			NavTrackerState t=NavTrackerState::translate(dollyDirection*currentWheelScale);
			t*=preScale;
			setNavigationTransformation(t);
			break;
			}
		
		case SCALING_WHEEL:
			{
			/* Update the navigation transformation: */
			Scalar scale=currentValue;
			currentWheelScale*=Math::pow(configuration.wheelScaleFactor,scale);
			NavTrackerState t=preScale;
			t*=NavTrackerState::scale(currentWheelScale);
			t*=postScale;
			setNavigationTransformation(t);
			break;
			}
		
		default:
			;
		}
	}

void MouseNavigationTool::display(GLContextData& contextData) const
	{
	if(configuration.showScreenCenter&&navigationMode!=IDLE)
		{
		/* Save and set up OpenGL state: */
		glPushAttrib(GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glDepthFunc(GL_LEQUAL);
		
		/* Draw the screen center crosshairs: */
		Vector x=interactionPlane.getDirection(0)*getDisplaySize();
		Vector y=interactionPlane.getDirection(1)*getDisplaySize();
		glLineWidth(3.0f);
		glColor(getBackgroundColor());
		glBegin(GL_LINES);
		glVertex(screenCenter-x);
		glVertex(screenCenter+x);
		glVertex(screenCenter-y);
		glVertex(screenCenter+y);
		glEnd();
		glLineWidth(1.0f);
		glColor(getForegroundColor());
		glBegin(GL_LINES);
		glVertex(screenCenter-x);
		glVertex(screenCenter+x);
		glVertex(screenCenter-y);
		glVertex(screenCenter+y);
		glEnd();
		
		/* Restore OpenGL state: */
		glPopAttrib();
		}
	}

}
