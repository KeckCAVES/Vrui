/***********************************************************************
MouseDialogNavigationTool - Class providing a newbie-friendly interface
to the standard MouseDialogNavigationTool using a dialog box of navigation
options.
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
#include <Vrui/InputDeviceManager.h>
#include <Vrui/InputDeviceAdapterMouse.h>
#include <Vrui/VRScreen.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRWindow.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/MouseDialogNavigationTool.h>

namespace Vrui {

/*************************************************
Methods of class MouseDialogNavigationToolFactory:
*************************************************/

MouseDialogNavigationToolFactory::MouseDialogNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("MouseDialogNavigationTool",toolManager),
	 rotatePlaneOffset(getInchFactor()*Scalar(3)),
	 rotateFactor(getInchFactor()*Scalar(3)),
	 screenDollyingDirection(0,-1,0),
	 screenScalingDirection(0,-1,0),
	 dollyFactor(Scalar(1)),
	 scaleFactor(getInchFactor()*Scalar(3)),
	 spinThreshold(getInchFactor()*Scalar(0.25))
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("NavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	rotatePlaneOffset=cfs.retrieveValue<Scalar>("./rotatePlaneOffset",rotatePlaneOffset);
	rotateFactor=cfs.retrieveValue<Scalar>("./rotateFactor",rotateFactor);
	screenDollyingDirection=cfs.retrieveValue<Vector>("./screenDollyingDirection",screenDollyingDirection);
	screenScalingDirection=cfs.retrieveValue<Vector>("./screenScalingDirection",screenScalingDirection);
	dollyFactor=cfs.retrieveValue<Scalar>("./dollyFactor",dollyFactor);
	scaleFactor=cfs.retrieveValue<Scalar>("./scaleFactor",scaleFactor);
	spinThreshold=cfs.retrieveValue<Scalar>("./spinThreshold",spinThreshold);
	
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

Point MouseDialogNavigationTool::calcScreenCenter(void)
	{
	/* Determine the screen containing the input device and the screen's center: */
	const VRScreen* screen;
	Point centerPos;
	if(mouseAdapter!=0&&mouseAdapter->getWindow()!=0)
		{
		screen=mouseAdapter->getWindow()->getVRScreen();
		mouseAdapter->getWindow()->getWindowCenterPos(centerPos.getComponents());
		}
	else
		{
		screen=getMainScreen();
		centerPos[0]=getMainScreen()->getWidth()*Scalar(0.5);
		centerPos[1]=getMainScreen()->getHeight()*Scalar(0.5);
		}
	centerPos[2]=Scalar(0);
	
	/* Calculate the center position in physical coordinates: */
	return screen->getScreenTransformation().transform(centerPos);
	}

Point MouseDialogNavigationTool::calcScreenPos(void)
	{
	/* Calculate the ray equation: */
	Ray ray=getDeviceRay(0);
	
	/* Find the screen currently containing the input device: */
	const VRScreen* screen;
	if(mouseAdapter!=0&&mouseAdapter->getWindow()!=0)
		screen=mouseAdapter->getWindow()->getVRScreen();
	else
		screen=getMainScreen();
	
	/* Intersect ray with the screen: */
	ONTransform screenT=screen->getScreenTransformation();
	Vector normal=screenT.getDirection(2);
	Scalar d=normal*screenT.getOrigin();
	Scalar divisor=normal*ray.getDirection();
	if(divisor==Scalar(0))
		return Point::origin;
	
	Scalar lambda=(d-ray.getOrigin()*normal)/divisor;
	if(lambda<Scalar(0))
		return Point::origin;
	
	return ray(lambda);
	}

void MouseDialogNavigationTool::startRotating(void)
	{
	/* Calculate the rotation center: */
	screenCenter=calcScreenCenter();
	
	/* Calculate initial rotation position: */
	lastRotationPos=calcScreenPos();
	
	/* Calculate the rotation offset vector: */
	rotateOffset=getMainScreen()->getScreenTransformation().transform(Vector(0,0,factory->rotatePlaneOffset));
	
	preScale=NavTrackerState::translateFromOriginTo(screenCenter);
	rotation=NavTrackerState::identity;
	postScale=NavTrackerState::translateToOriginFrom(screenCenter);
	postScale*=getNavigationTransformation();
	}

void MouseDialogNavigationTool::startPanning(void)
	{
	/* Calculate initial motion position: */
	motionStart=calcScreenPos();
	
	preScale=getNavigationTransformation();
	}

void MouseDialogNavigationTool::startDollying(void)
	{
	/* Calculate the dollying direction: */
	if(mouseAdapter!=0)
		dollyDirection=mouseAdapter->getWindow()->getViewer()->getHeadPosition()-calcScreenCenter();
	else
		dollyDirection=getMainViewer()->getHeadPosition()-calcScreenCenter();
	dollyDirection.normalize();
	
	/* Calculate initial motion position: */
	motionStart=calcScreenPos();
	
	preScale=getNavigationTransformation();
	}

void MouseDialogNavigationTool::startScaling(void)
	{
	/* Calculate the scaling center: */
	screenCenter=calcScreenCenter();
	
	/* Calculate initial motion position: */
	motionStart=calcScreenPos();
	
	preScale=NavTrackerState::translateFromOriginTo(screenCenter);
	postScale=NavTrackerState::translateToOriginFrom(screenCenter);
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
	 mouseAdapter(0),
	 navigationDialogPopup(0),
	 navigationMode(ROTATING),
	 spinning(false),
	 showScreenCenter(false)
	{
	/* Find the mouse input device adapter controlling the input device: */
	mouseAdapter=dynamic_cast<InputDeviceAdapterMouse*>(getInputDeviceManager()->findInputDeviceAdapter(getDevice(0)));
	
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
	popupPrimaryWidget(navigationDialogPopup,getNavigationTransformation().transform(getDisplayCenter()));
	}

MouseDialogNavigationTool::~MouseDialogNavigationTool(void)
	{
	/* Delete the navigation dialog: */
	delete navigationDialogPopup;
	}

const ToolFactory* MouseDialogNavigationTool::getFactory(void) const
	{
	return factory;
	}

void MouseDialogNavigationTool::buttonCallback(int,int buttonIndex,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Deactivate spinning: */
		spinning=false;
		
		/* Start navigating according to the current navigation mode: */
		switch(navigationMode)
			{
			case ROTATING:
				if(activate())
					startRotating();
				break;
			
			case PANNING:
				if(activate())
					startPanning();
				break;
			
			case DOLLYING:
				if(activate())
					startDollying();
				break;
			
			case SCALING:
				if(activate())
					startScaling();
				break;
			}
		}
	else // Button has just been released
		{
		/* Check for spinning if currently in rotating mode: */
		if(navigationMode==ROTATING)
			{
			/* Check if the input device is still moving: */
			Point currentPos=calcScreenPos();
			Vector delta=currentPos-lastRotationPos;
			if(Geometry::mag(delta)>factory->spinThreshold)
				{
				/* Calculate spinning angular velocity: */
				Vector offset=(lastRotationPos-screenCenter)+rotateOffset;
				Vector axis=Geometry::cross(offset,delta);
				Scalar angularVelocity=Geometry::mag(delta)/(factory->rotateFactor*getFrameTime());
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
	currentPos=calcScreenPos();
	
	/* Act depending on this tool's current state: */
	if(isActive())
		{
		if(spinning)
			{
			/* Calculate incremental rotation: */
			rotation.leftMultiply(NavTrackerState::rotate(NavTrackerState::Rotation::rotateScaledAxis(spinAngularVelocity*getFrameTime())));
			
			NavTrackerState t=preScale;
			t*=rotation;
			t*=postScale;
			setNavigationTransformation(t);
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
					Vector axis=Geometry::cross(offset,delta);
					Scalar angle=Geometry::mag(delta)/factory->rotateFactor;
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
					/* Calculate the current dollying direction: */
					Vector dollyingDirection;
					if(mouseAdapter!=0)
						dollyingDirection=mouseAdapter->getWindow()->getVRScreen()->getScreenTransformation().transform(factory->screenDollyingDirection);
					else
						dollyingDirection=getMainScreen()->getScreenTransformation().transform(factory->screenDollyingDirection);
					
					/* Update the navigation transformation: */
					Scalar dollyDist=((currentPos-motionStart)*dollyingDirection)/factory->dollyFactor;
					NavTrackerState t=NavTrackerState::translate(dollyDirection*dollyDist);
					t*=preScale;
					setNavigationTransformation(t);
					break;
					}
				
				case SCALING:
					{
					/* Calculate the current scaling direction: */
					Vector scalingDirection;
					if(mouseAdapter!=0)
						scalingDirection=mouseAdapter->getWindow()->getVRScreen()->getScreenTransformation().transform(factory->screenScalingDirection);
					else
						scalingDirection=getMainScreen()->getScreenTransformation().transform(factory->screenScalingDirection);
					
					/* Update the navigation transformation: */
					Scalar scale=((currentPos-motionStart)*scalingDirection)/factory->scaleFactor;
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
		glPushAttrib(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_LINE_BIT|GL_TEXTURE_BIT);
		glDisable(GL_LIGHTING);
		
		/* Get a pointer to the screen the mouse is on: */
		const VRScreen* screen;
		if(mouseAdapter!=0&&mouseAdapter->getWindow()!=0)
			screen=mouseAdapter->getWindow()->getVRScreen();
		else
			screen=getMainScreen();
		ONTransform screenT=screen->getScreenTransformation();
		
		/* Go to screen coordinates: */
		glPushMatrix();
		glMultMatrix(screenT);
		
		/* Determine the screen containing the input device and find its center: */
		Scalar centerPos[2];
		if(mouseAdapter!=0)
			mouseAdapter->getWindow()->getWindowCenterPos(centerPos);
		else
			{
			centerPos[0]=getMainScreen()->getWidth()*Scalar(0.5);
			centerPos[1]=getMainScreen()->getHeight()*Scalar(0.5);
			}
		
		/* Calculate the endpoints of the screen's crosshair lines in screen coordinates: */
		Point l=Point(Scalar(0),centerPos[1],Scalar(0));
		Point r=Point(screen->getWidth(),centerPos[1],Scalar(0));
		Point b=Point(centerPos[0],Scalar(0),Scalar(0));
		Point t=Point(centerPos[0],screen->getHeight(),Scalar(0));
		
		/* Determine the crosshair colors: */
		Color bgColor=getBackgroundColor();
		Color fgColor;
		for(int i=0;i<3;++i)
			fgColor[i]=1.0f-bgColor[i];
		fgColor[3]=bgColor[3];
		
		/* Draw the screen crosshairs: */
		glDepthFunc(GL_LEQUAL);
		glLineWidth(3.0f);
		glColor(bgColor);
		glBegin(GL_LINES);
		glVertex(l);
		glVertex(r);
		glVertex(b);
		glVertex(t);
		glEnd();
		glLineWidth(1.0f);
		glColor(fgColor);
		glBegin(GL_LINES);
		glVertex(l);
		glVertex(r);
		glVertex(b);
		glVertex(t);
		glEnd();
		
		/* Go back to physical coordinates: */
		glPopMatrix();
		
		/* Restore OpenGL state: */
		glPopAttrib();
		}
	}

}
