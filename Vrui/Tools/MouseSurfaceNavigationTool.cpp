/***********************************************************************
MouseSurfaceNavigationTool - Class for navigation tools that use the
mouse to move along an application-defined surface.
Copyright (c) 2009-2016 Oliver Kreylos

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

#include <Vrui/Tools/MouseSurfaceNavigationTool.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/Event.h>
#include <GLMotif/TitleBar.h>
#include <GLMotif/WidgetManager.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/UIManager.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/*****************************************************************
Methods of class MouseSurfaceNavigationToolFactory::Configuration:
*****************************************************************/

MouseSurfaceNavigationToolFactory::Configuration::Configuration(void)
	:rotateFactor(getDisplaySize()/Scalar(4)),
	 scalingDirection(-getUpDirection()),
	 scaleFactor(getDisplaySize()/Scalar(4)),
	 wheelScaleFactor(Scalar(0.5)),
	 throwThreshold(getUiSize()*Scalar(2)),
	 probeSize(getUiSize()),
	 maxClimb(getDisplaySize()),
	 fixAzimuth(false),
	 showCompass(true),
	 compassPos(getDisplaySize()*Scalar(0.5),getDisplaySize()*Scalar(0.5),Scalar(0)),
	 compassSize(getUiSize()*Scalar(5)),compassThickness(getUiSize()*Scalar(0.5)),
	 showScreenCenter(true)
	{
	}

void MouseSurfaceNavigationToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	rotateFactor=cfs.retrieveValue<Scalar>("./rotateFactor",rotateFactor);
	scalingDirection=cfs.retrieveValue<Vector>("./scalingDirection",scalingDirection);
	scaleFactor=cfs.retrieveValue<Scalar>("./scaleFactor",scaleFactor);
	wheelScaleFactor=cfs.retrieveValue<Scalar>("./wheelScaleFactor",wheelScaleFactor);
	throwThreshold=cfs.retrieveValue<Scalar>("./throwThreshold",throwThreshold);
	probeSize=cfs.retrieveValue<Scalar>("./probeSize",probeSize);
	maxClimb=cfs.retrieveValue<Scalar>("./maxClimb",maxClimb);
	fixAzimuth=cfs.retrieveValue<bool>("./fixAzimuth",fixAzimuth);
	showCompass=cfs.retrieveValue<bool>("./showCompass",showCompass);
	compassPos=cfs.retrieveValue<Point>("./compassPos",compassPos);
	compassSize=cfs.retrieveValue<Scalar>("./compassSize",compassSize);
	compassThickness=cfs.retrieveValue<Scalar>("./compassThickness",compassThickness);
	showScreenCenter=cfs.retrieveValue<bool>("./showScreenCenter",showScreenCenter);
	}

void MouseSurfaceNavigationToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue<Scalar>("./rotateFactor",rotateFactor);
	cfs.storeValue<Vector>("./scalingDirection",scalingDirection);
	cfs.storeValue<Scalar>("./scaleFactor",scaleFactor);
	cfs.storeValue<Scalar>("./wheelScaleFactor",wheelScaleFactor);
	cfs.storeValue<Scalar>("./throwThreshold",throwThreshold);
	cfs.storeValue<Scalar>("./probeSize",probeSize);
	cfs.storeValue<Scalar>("./maxClimb",maxClimb);
	cfs.storeValue<bool>("./fixAzimuth",fixAzimuth);
	cfs.storeValue<bool>("./showCompass",showCompass);
	cfs.storeValue<Point>("./compassPos",compassPos);
	cfs.storeValue<Scalar>("./compassSize",compassSize);
	cfs.storeValue<Scalar>("./compassThickness",compassThickness);
	cfs.storeValue<bool>("./showScreenCenter",showScreenCenter);
	}

/**************************************************
Methods of class MouseSurfaceNavigationToolFactory:
**************************************************/

MouseSurfaceNavigationToolFactory::MouseSurfaceNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("MouseSurfaceNavigationTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(2);
	layout.setNumValuators(1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("SurfaceNavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	MouseSurfaceNavigationTool::factory=this;
	}

MouseSurfaceNavigationToolFactory::~MouseSurfaceNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	MouseSurfaceNavigationTool::factory=0;
	}

const char* MouseSurfaceNavigationToolFactory::getName(void) const
	{
	return "Mouse (Multiple Buttons)";
	}

const char* MouseSurfaceNavigationToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	switch(buttonSlotIndex)
		{
		case 0:
			return "Rotate";
		
		case 1:
			return "Pan";
		}
	
	/* Never reached; just to make compiler happy: */
	return 0;
	}

const char* MouseSurfaceNavigationToolFactory::getValuatorFunction(int) const
	{
	return "Quick Zoom";
	}

Tool* MouseSurfaceNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new MouseSurfaceNavigationTool(this,inputAssignment);
	}

void MouseSurfaceNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveMouseSurfaceNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("SurfaceNavigationTool");
	}

extern "C" ToolFactory* createMouseSurfaceNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	MouseSurfaceNavigationToolFactory* mouseSurfaceNavigationToolFactory=new MouseSurfaceNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return mouseSurfaceNavigationToolFactory;
	}

extern "C" void destroyMouseSurfaceNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/*****************************************************
Methods of class MouseSurfaceNavigationTool::DataItem:
*****************************************************/

MouseSurfaceNavigationTool::DataItem::DataItem(void)
	:compassDisplayList(glGenLists(1))
	{
	}

MouseSurfaceNavigationTool::DataItem::~DataItem(void)
	{
	glDeleteLists(compassDisplayList,1);
	}

/***************************************************
Static elements of class MouseSurfaceNavigationTool:
***************************************************/

MouseSurfaceNavigationToolFactory* MouseSurfaceNavigationTool::factory=0;

/*******************************************
Methods of class MouseSurfaceNavigationTool:
*******************************************/

Point MouseSurfaceNavigationTool::calcInteractionPos(void) const
	{
	/* Intersect the device's pointing ray with the widget plane: */
	Point deviceRayStart=getButtonDevicePosition(0);
	Vector deviceRayDir=getButtonDeviceRayDirection(0);
	
	Point planeCenter=interactionPlane.getOrigin();
	Vector planeNormal=interactionPlane.getDirection(2);
	Scalar lambda=((planeCenter-deviceRayStart)*planeNormal)/(deviceRayDir*planeNormal);
	return deviceRayStart+deviceRayDir*lambda;
	}

void MouseSurfaceNavigationTool::applyNavState(void) const
	{
	/* Compose and apply the navigation transformation: */
	NavTransform nav=physicalFrame;
	nav*=NavTransform::rotate(Rotation::rotateX(elevation));
	nav*=NavTransform::rotate(Rotation::rotateZ(azimuth));
	nav*=Geometry::invert(surfaceFrame);
	setNavigationTransformation(nav);
	}

void MouseSurfaceNavigationTool::initNavState(void)
	{
	/* Query the rotation center: */
	screenCenter=getDisplayCenter();
	
	/* Set up the interaction plane: */
	interactionPlane=getUiManager()->calcUITransform(screenCenter);
	
	/* Project the rotation center into the interaction plane: */
	screenCenter=interactionPlane.getOrigin();
	
	/* Set up a physical navigation frame around the rotation center: */
	calcPhysicalFrame(screenCenter);
	
	/* Calculate the initial environment-aligned surface frame in navigation coordinates: */
	surfaceFrame=getInverseNavigationTransformation()*physicalFrame;
	NavTransform newSurfaceFrame=surfaceFrame;
	
	/* Align the initial frame with the application's surface and calculate Euler angles: */
	AlignmentData ad(surfaceFrame,newSurfaceFrame,configuration.probeSize,configuration.maxClimb);
	Scalar roll;
	align(ad,azimuth,elevation,roll);
	
	/* Limit elevation angle to down direction: */
	if(elevation<Scalar(0))
		elevation=Scalar(0);
	
	if(configuration.showCompass)
		{
		/* Start showing the virtual compass: */
		showCompass=true;
		}
	
	/* Apply the newly aligned surface frame: */
	surfaceFrame=newSurfaceFrame;
	applyNavState();
	}

void MouseSurfaceNavigationTool::realignSurfaceFrame(NavTransform& newSurfaceFrame)
	{
	/* Re-align the surface frame with the surface: */
	Rotation initialOrientation=newSurfaceFrame.getRotation();
	AlignmentData ad(surfaceFrame,newSurfaceFrame,configuration.probeSize,configuration.maxClimb);
	align(ad);
	
	if(!configuration.fixAzimuth)
		{
		/* Have the azimuth angle track changes in the surface frame's rotation: */
		Rotation rot=Geometry::invert(initialOrientation)*newSurfaceFrame.getRotation();
		rot.leftMultiply(Rotation::rotateFromTo(rot.getDirection(2),Vector(0,0,1)));
		Vector x=rot.getDirection(0);
		azimuth=wrapAngle(azimuth+Math::atan2(x[1],x[0]));
		}
	
	/* Store and apply the newly aligned surface frame: */
	surfaceFrame=newSurfaceFrame;
	applyNavState();
	}

void MouseSurfaceNavigationTool::navigationTransformationChangedCallback(Misc::CallbackData* cbData)
	{
	/* Stop showing the virtual compass if this tool is no longer active: */
	if(!SurfaceNavigationTool::isActive())
		showCompass=false;
	}

MouseSurfaceNavigationTool::MouseSurfaceNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:SurfaceNavigationTool(factory,inputAssignment),
	 configuration(MouseSurfaceNavigationTool::factory->configuration),
	 currentPos(Point::origin),currentValue(0),
	 navigationMode(IDLE),
	 showCompass(false)
	{
	/* Register a callback when the navigation transformation changes: */
	getNavigationTransformationChangedCallbacks().add(this,&MouseSurfaceNavigationTool::navigationTransformationChangedCallback);
	}

MouseSurfaceNavigationTool::~MouseSurfaceNavigationTool(void)
	{
	/* Remove then navigation transformation change callback: */
	getNavigationTransformationChangedCallbacks().remove(this,&MouseSurfaceNavigationTool::navigationTransformationChangedCallback);
	}

void MouseSurfaceNavigationTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override private configuration data from given configuration file section: */
	configuration.read(configFileSection);
	}

void MouseSurfaceNavigationTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write private configuration data to given configuration file section: */
	configuration.write(configFileSection);
	}

void MouseSurfaceNavigationTool::initialize(void)
	{
	/* Query the rotation center: */
	screenCenter=getDisplayCenter();
	
	/* Set up the interaction plane: */
	interactionPlane=getUiManager()->calcUITransform(screenCenter);
	
	/* Project the rotation center into the interaction plane: */
	screenCenter=interactionPlane.getOrigin();
	
	/* Calculate a UI transformation to the top-right of the rotation center: */
	compassTransform=getUiManager()->calcUITransform(interactionPlane.transform(configuration.compassPos));
	}

const ToolFactory* MouseSurfaceNavigationTool::getFactory(void) const
	{
	return factory;
	}

void MouseSurfaceNavigationTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
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
					case THROWING:
						/* Try activating this tool: */
						if(navigationMode==THROWING||activate())
							{
							initNavState();
							currentPos=calcInteractionPos();
							navigationMode=ROTATING;
							}
						break;
					
					case PANNING:
						currentPos=calcInteractionPos();
						navigationMode=SCALING;
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
						/* Deactivate this tool: */
						deactivate();
						
						/* Go to idle mode: */
						navigationMode=IDLE;
						break;
					
					case SCALING:
						currentPos=calcInteractionPos();
						navigationMode=PANNING;
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
					case THROWING:
						/* Try activating this tool: */
						if(navigationMode==THROWING||activate())
							{
							initNavState();
							currentPos=calcInteractionPos();
							navigationMode=PANNING;
							}
						break;
					
					case ROTATING:
						currentPos=calcInteractionPos();
						navigationMode=SCALING;
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
						{
						/* Check if the input device is still moving: */
						Point newCurrentPos=calcInteractionPos();
						Vector delta=interactionPlane.inverseTransform(newCurrentPos-currentPos);
						if(Geometry::mag(delta)>configuration.throwThreshold)
							{
							/* Calculate throwing velocity: */
							throwVelocity=delta/(getApplicationTime()-lastMoveTime);
							
							/* Go to throwing mode: */
							navigationMode=THROWING;
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
					
					case SCALING:
						currentPos=calcInteractionPos();
						navigationMode=ROTATING;
						break;
					
					default:
						/* This shouldn't happen; just ignore the event */
						break;
					}
				}
			break;
		}
	}

void MouseSurfaceNavigationTool::valuatorCallback(int,InputDevice::ValuatorCallbackData* cbData)
	{
	currentValue=Scalar(cbData->newValuatorValue);
	if(currentValue!=Scalar(0))
		{
		/* Act depending on this tool's current state: */
		switch(navigationMode)
			{
			case IDLE:
			case THROWING:
				/* Try activating this tool: */
				if(navigationMode==THROWING||activate())
					{
					/* Go to wheel scaling mode: */
					initNavState();
					navigationMode=SCALING_WHEEL;
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

void MouseSurfaceNavigationTool::frame(void)
	{
	/* Calculate the new mouse position: */
	Point newCurrentPos=calcInteractionPos();
	
	/* Act depending on this tool's current state: */
	switch(navigationMode)
		{
		case ROTATING:
			{
			/* Calculate the rotation vector: */
			Vector delta=interactionPlane.inverseTransform(newCurrentPos-currentPos);
			
			/* Adjust the azimuth angle: */
			azimuth=wrapAngle(azimuth+delta[0]/configuration.rotateFactor);
			
			/* Adjust the elevation angle: */
			elevation=Math::clamp(elevation-delta[1]/configuration.rotateFactor,Scalar(0),Math::rad(Scalar(90)));
			
			/* Apply the new transformation: */
			applyNavState();
			break;
			}
		
		case PANNING:
			{
			NavTransform newSurfaceFrame=surfaceFrame;
			
			/* Calculate the translation vector: */
			Vector delta=interactionPlane.inverseTransform(newCurrentPos-currentPos);
			delta=Rotation::rotateZ(-azimuth).transform(delta);
			
			/* Translate the surface frame: */
			newSurfaceFrame*=NavTransform::translate(-delta);
			
			/* Re-align the surface frame with the surface: */
			realignSurfaceFrame(newSurfaceFrame);
			
			break;
			}
		
		case THROWING:
			{
			NavTransform newSurfaceFrame=surfaceFrame;
			
			/* Calculate the throw translation vector: */
			Vector delta=throwVelocity*getFrameTime();
			delta=Rotation::rotateZ(-azimuth).transform(delta);
			
			/* Translate the surface frame: */
			newSurfaceFrame*=NavTransform::translate(-delta);
			
			/* Re-align the surface frame with the surface: */
			realignSurfaceFrame(newSurfaceFrame);
			
			/* Schedule another frame: */
			scheduleUpdate(getNextAnimationTime());
			
			break;
			}
		
		case SCALING:
			{
			NavTransform newSurfaceFrame=surfaceFrame;
			/* Scale the surface frame: */
			Scalar scale=((newCurrentPos-currentPos)*configuration.scalingDirection)/configuration.scaleFactor;
			newSurfaceFrame*=NavTrackerState::scale(Math::exp(-scale));
			
			/* Re-align the surface frame with the surface: */
			realignSurfaceFrame(newSurfaceFrame);
			
			break;
			}
		
		case SCALING_WHEEL:
			{
			NavTransform newSurfaceFrame=surfaceFrame;
			
			/* Scale the surface frame: */
			newSurfaceFrame*=NavTrackerState::scale(Math::pow(configuration.wheelScaleFactor,-currentValue));
			
			/* Re-align the surface frame with the surface: */
			realignSurfaceFrame(newSurfaceFrame);
			
			break;
			}
		
		default:
			;
		}
	
	/* Update the current mouse position: */
	if(currentPos!=newCurrentPos)
		{
		currentPos=calcInteractionPos();
		lastMoveTime=getApplicationTime();
		}
	}

void MouseSurfaceNavigationTool::display(GLContextData& contextData) const
	{
	if(showCompass||(configuration.showScreenCenter&&navigationMode!=IDLE))
		{
		/* Save and set up OpenGL state: */
		glPushAttrib(GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glDepthFunc(GL_LEQUAL);
		
		if(configuration.showScreenCenter&&navigationMode!=IDLE)
			{
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
			}
		
		if(showCompass)
			{
			/* Get the data item: */
			DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
			
			/* Go to compass rose coordinates: */
			glPushMatrix();
			glMultMatrix(compassTransform);
			glRotate(Math::deg(azimuth),0,0,1);
			
			/* Draw the compass rose's background: */
			glLineWidth(3.0f);
			glColor(getBackgroundColor());
			glCallList(dataItem->compassDisplayList);
			
			/* Draw the compass rose's foreground: */
			glLineWidth(1.0f);
			glColor(getForegroundColor());
			glCallList(dataItem->compassDisplayList);
			
			/* Go back to physical coordinates: */
			glPopMatrix();
			}
		
		/* Restore OpenGL state: */
		glPopAttrib();
		}
	}

void MouseSurfaceNavigationTool::initContext(GLContextData& contextData) const
	{
	/* Create a data item: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Create the compass rose display list: */
	glNewList(dataItem->compassDisplayList,GL_COMPILE);
	
	/* Draw the compass ring: */
	glBegin(GL_LINE_LOOP);
	for(int i=0;i<30;++i)
		{
		Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*(Scalar(i)+Scalar(0.5))/Scalar(30);
		glVertex(Math::sin(angle)*(configuration.compassSize+configuration.compassThickness),Math::cos(angle)*(configuration.compassSize+configuration.compassThickness));
		}
	for(int i=0;i<30;++i)
		{
		Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*(Scalar(i)+Scalar(0.5))/Scalar(30);
		glVertex(Math::sin(angle)*(configuration.compassSize-configuration.compassThickness),Math::cos(angle)*(configuration.compassSize-configuration.compassThickness));
		}
	glEnd();
	
	/* Draw the compass arrow: */
	glBegin(GL_LINE_LOOP);
	glVertex(configuration.compassThickness,configuration.compassSize*Scalar(-1.25));
	glVertex(configuration.compassThickness,configuration.compassSize*Scalar(1.25));
	glVertex(configuration.compassThickness*Scalar(2.5),configuration.compassSize*Scalar(1.25));
	glVertex(Scalar(0),configuration.compassSize*Scalar(1.75));
	glVertex(-configuration.compassThickness*Scalar(2.5),configuration.compassSize*Scalar(1.25));
	glVertex(-configuration.compassThickness,configuration.compassSize*Scalar(1.25));
	glVertex(-configuration.compassThickness,configuration.compassSize*Scalar(-1.25));
	glEnd();
	glBegin(GL_LINES);
	glVertex(-configuration.compassSize*Scalar(1.25),Scalar(0));
	glVertex(configuration.compassSize*Scalar(1.25),Scalar(0));
	glEnd();
	
	glEndList();
	}

}
