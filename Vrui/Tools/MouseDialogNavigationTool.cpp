/***********************************************************************
MouseDialogNavigationTool - Class providing a newbie-friendly interface
to the standard MouseNavigationTool using a dialog box of navigation
options.
Copyright (c) 2007-2015 Oliver Kreylos

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

#include <Vrui/Tools/MouseDialogNavigationTool.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/RadioBox.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRWindow.h>
#include <Vrui/UIManager.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/**********************************************************
Methods of class MouseDialogNavigationToolFactory::Configuration:
**********************************************************/

MouseDialogNavigationToolFactory::Configuration::Configuration(void)
	:rotatePlaneOffset(getDisplaySize()/Scalar(4)),
	 rotateFactor(getDisplaySize()/Scalar(4)),
	 dollyCenter(true),scaleCenter(true),
	 dollyingDirection(-getUpDirection()),
	 scalingDirection(-getUpDirection()),
	 dollyFactor(Scalar(1)),
	 scaleFactor(getDisplaySize()/Scalar(4)),
	 spinThreshold(getUiSize()*Scalar(1)),
	 fixedMode(-1)
	{
	}

void MouseDialogNavigationToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	rotatePlaneOffset=cfs.retrieveValue<Scalar>("./rotatePlaneOffset",rotatePlaneOffset);
	rotateFactor=cfs.retrieveValue<Scalar>("./rotateFactor",rotateFactor);
	dollyCenter=cfs.retrieveValue<bool>("./dollyCenter",dollyCenter);
	scaleCenter=cfs.retrieveValue<bool>("./scaleCenter",scaleCenter);
	dollyingDirection=cfs.retrieveValue<Vector>("./dollyingDirection",dollyingDirection);
	scalingDirection=cfs.retrieveValue<Vector>("./scalingDirection",scalingDirection);
	dollyFactor=cfs.retrieveValue<Scalar>("./dollyFactor",dollyFactor);
	scaleFactor=cfs.retrieveValue<Scalar>("./scaleFactor",scaleFactor);
	spinThreshold=cfs.retrieveValue<Scalar>("./spinThreshold",spinThreshold);
	fixedMode=cfs.retrieveValue<int>("./fixedMode",fixedMode);
	}

void MouseDialogNavigationToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue<Scalar>("./rotatePlaneOffset",rotatePlaneOffset);
	cfs.storeValue<Scalar>("./rotateFactor",rotateFactor);
	cfs.storeValue<bool>("./dollyCenter",dollyCenter);
	cfs.storeValue<bool>("./scaleCenter",scaleCenter);
	cfs.storeValue<Vector>("./dollyingDirection",dollyingDirection);
	cfs.storeValue<Vector>("./scalingDirection",scalingDirection);
	cfs.storeValue<Scalar>("./dollyFactor",dollyFactor);
	cfs.storeValue<Scalar>("./scaleFactor",scaleFactor);
	cfs.storeValue<Scalar>("./spinThreshold",spinThreshold);
	cfs.storeValue<int>("./fixedMode",fixedMode);
	}

/*************************************************
Methods of class MouseDialogNavigationToolFactory:
*************************************************/

MouseDialogNavigationToolFactory::MouseDialogNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("MouseDialogNavigationTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("NavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	MouseDialogNavigationTool::factory=this;
	}

MouseDialogNavigationToolFactory::~MouseDialogNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	MouseDialogNavigationTool::factory=0;
	}

const char* MouseDialogNavigationToolFactory::getName(void) const
	{
	return "Mouse (via Dialog Box)";
	}

const char* MouseDialogNavigationToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	return "Navigate";
	}

Tool* MouseDialogNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new MouseDialogNavigationTool(this,inputAssignment);
	}

void MouseDialogNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveMouseDialogNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("NavigationTool");
	}

extern "C" ToolFactory* createMouseDialogNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	MouseDialogNavigationToolFactory* mouseDialogNavigationToolFactory=new MouseDialogNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return mouseDialogNavigationToolFactory;
	}

extern "C" void destroyMouseDialogNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/**************************************************
Static elements of class MouseDialogNavigationTool:
**************************************************/

MouseDialogNavigationToolFactory* MouseDialogNavigationTool::factory=0;

/******************************************
Methods of class MouseDialogNavigationTool:
******************************************/

void MouseDialogNavigationTool::startNavigating(void)
	{
	/* Calculate the rotation center: */
	screenCenter=getDisplayCenter();
	
	/* Set up the interaction plane: */
	interactionPlane=getUiManager()->calcUITransform(screenCenter);
	
	/* Project the rotation center into the interaction plane: */
	screenCenter=interactionPlane.getOrigin();
	}

Point MouseDialogNavigationTool::calcInteractionPos(void) const
	{
	/* Intersect the device's pointing ray with the widget plane: */
	Point deviceRayStart=getButtonDevicePosition(0);
	Vector deviceRayDir=getButtonDeviceRayDirection(0);
	
	Point planeCenter=interactionPlane.getOrigin();
	Vector planeNormal=interactionPlane.getDirection(2);
	Scalar lambda=((planeCenter-deviceRayStart)*planeNormal)/(deviceRayDir*planeNormal);
	return deviceRayStart+deviceRayDir*lambda;
	}

void MouseDialogNavigationTool::startRotating(void)
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
	}

void MouseDialogNavigationTool::startPanning(void)
	{
	startNavigating();
	
	/* Calculate initial motion position: */
	motionStart=calcInteractionPos();
	
	preScale=getNavigationTransformation();
	}

void MouseDialogNavigationTool::startDollying(void)
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
	}

void MouseDialogNavigationTool::startScaling(void)
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
	}

void MouseDialogNavigationTool::navigationModesValueChangedCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData)
	{
	/* Change the navigation mode: */
	switch(cbData->radioBox->getToggleIndex(cbData->newSelectedToggle))
		{
		case 0:
			navigationMode=ROTATING;
			break;
		
		case 1:
			navigationMode=PANNING;
			break;
		
		case 2:
			navigationMode=DOLLYING;
			break;
		
		case 3:
			navigationMode=SCALING;
			break;
		}
	}

void MouseDialogNavigationTool::showScreenCenterToggleValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	showScreenCenter=cbData->set;
	}

MouseDialogNavigationTool::MouseDialogNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(factory,inputAssignment),
	 configuration(MouseDialogNavigationTool::factory->configuration),
	 navigationDialogPopup(0),
	 currentPos(Point::origin),
	 navigationMode(ROTATING),
	 spinning(false),
	 showScreenCenter(false)
	{
	}

MouseDialogNavigationTool::~MouseDialogNavigationTool(void)
	{
	}

void MouseDialogNavigationTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override private configuration data from given configuration file section: */
	configuration.read(configFileSection);
	}

void MouseDialogNavigationTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write private configuration data to given configuration file section: */
	configuration.write(configFileSection);
	}

void MouseDialogNavigationTool::initialize(void)
	{
	if(configuration.fixedMode>=0)
		{
		/* Set the fixed navigation mode: */
		switch(configuration.fixedMode)
			{
			case 0:
				navigationMode=ROTATING;
				break;
			
			case 1:
				navigationMode=PANNING;
				break;
			
			case 2:
				navigationMode=DOLLYING;
				break;
			
			case 3:
				navigationMode=SCALING;
				break;
			}
		
		/* Disable showing the screen center: */
		showScreenCenter=false;
		}
	else
		{
		/* Create the tool's GUI: */
		navigationDialogPopup=new GLMotif::PopupWindow("NavigationDialogPopup",getWidgetManager(),"Mouse Navigation Dialog");
		
		GLMotif::RowColumn* navigationDialog=new GLMotif::RowColumn("NavigationDialog",navigationDialogPopup,false);
		
		GLMotif::RadioBox* navigationModes=new GLMotif::RadioBox("NavigationModes",navigationDialog,false);
		navigationModes->setOrientation(GLMotif::RowColumn::VERTICAL);
		navigationModes->setPacking(GLMotif::RowColumn::PACK_GRID);
		navigationModes->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
		
		navigationModes->addToggle("Rotate");
		navigationModes->addToggle("Pan");
		navigationModes->addToggle("Dolly");
		navigationModes->addToggle("Scale");
		
		switch(navigationMode)
			{
			case ROTATING:
				navigationModes->setSelectedToggle(0);
				break;
			
			case PANNING:
				navigationModes->setSelectedToggle(1);
				break;
			
			case DOLLYING:
				navigationModes->setSelectedToggle(2);
				break;
			
			case SCALING:
				navigationModes->setSelectedToggle(3);
				break;
			}
		navigationModes->getValueChangedCallbacks().add(this,&MouseDialogNavigationTool::navigationModesValueChangedCallback);
		navigationModes->manageChild();
		
		GLMotif::ToggleButton* showScreenCenterToggle=new GLMotif::ToggleButton("ShowScreenCenterToggle",navigationDialog,"Show Screen Center");
		showScreenCenterToggle->setToggle(showScreenCenter);
		showScreenCenterToggle->getValueChangedCallbacks().add(this,&MouseDialogNavigationTool::showScreenCenterToggleValueChangedCallback);
		
		navigationDialog->manageChild();
		
		/* Pop up the navigation dialog: */
		popupPrimaryWidget(navigationDialogPopup);
		}
	}

void MouseDialogNavigationTool::deinitialize(void)
	{
	/* Delete the navigation dialog: */
	delete navigationDialogPopup;
	}

const ToolFactory* MouseDialogNavigationTool::getFactory(void) const
	{
	return factory;
	}

void MouseDialogNavigationTool::buttonCallback(int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Deactivate spinning: */
		spinning=false;
		
		/* Start navigating according to the current navigation mode: */
		if(activate())
			{
			switch(navigationMode)
				{
				case ROTATING:
					startRotating();
					break;
				
				case PANNING:
					startPanning();
					break;
				
				case DOLLYING:
					startDollying();
					break;
				
				case SCALING:
					startScaling();
					break;
				}
			}
		}
	else // Button has just been released
		{
		/* Check for spinning if currently in rotating mode: */
		if(navigationMode==ROTATING)
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
				
				/* Enable spinning: */
				spinning=true;
				}
			else
				{
				/* Deactivate the tool: */
				deactivate();
				}
			}
		else
			{
			/* Deactivate the tool: */
			deactivate();
			}
		}
	}

void MouseDialogNavigationTool::frame(void)
	{
	/* Update the current mouse position: */
	Point newCurrentPos=calcInteractionPos();
	if(currentPos!=newCurrentPos)
		{
		currentPos=newCurrentPos;
		lastMoveTime=getApplicationTime();
		}
	
	/* Act depending on this tool's current state: */
	if(NavigationTool::isActive())
		{
		if(spinning)
			{
			/* Calculate incremental rotation: */
			rotation.leftMultiply(NavTrackerState::rotate(NavTrackerState::Rotation::rotateScaledAxis(spinAngularVelocity*getFrameTime())));
			
			NavTrackerState t=preScale;
			t*=rotation;
			t*=postScale;
			setNavigationTransformation(t);
			
			/* Request another frame: */
			scheduleUpdate(getNextAnimationTime());
			}
		else
			{
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
				}
			}
		}
	}

void MouseDialogNavigationTool::display(GLContextData& contextData) const
	{
	if(showScreenCenter)
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
