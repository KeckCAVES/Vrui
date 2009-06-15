/***********************************************************************
MouseNavigationTool - Class encapsulating the navigation behaviour of a
mouse in the OpenInventor SoXtExaminerViewer.
Copyright (c) 2004-2009 Oliver Kreylos

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
#include <GLMotif/Event.h>
#include <GLMotif/TitleBar.h>
#include <GLMotif/WidgetManager.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/InputDeviceAdapterMouse.h>
#include <Vrui/VRScreen.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRWindow.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

namespace Vrui {

/*******************************************
Methods of class MouseNavigationToolFactory:
*******************************************/

MouseNavigationToolFactory::MouseNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("MouseNavigationTool",toolManager),
	 rotatePlaneOffset(getInchFactor()*Scalar(3)),
	 rotateFactor(getInchFactor()*Scalar(3)),
	 invertDolly(false),
	 screenDollyingDirection(0,-1,0),
	 screenScalingDirection(0,-1,0),
	 dollyFactor(Scalar(1)),
	 scaleFactor(getInchFactor()*Scalar(3)),
	 wheelDollyFactor(getInchFactor()*Scalar(-12)),
	 wheelScaleFactor(Scalar(0.5)),
	 spinThreshold(getInchFactor()*Scalar(0.25)),
	 showScreenCenter(true),
	 interactWithWidgets(true)
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,3);
	layout.setNumValuators(0,1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("NavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	rotatePlaneOffset=cfs.retrieveValue<Scalar>("./rotatePlaneOffset",rotatePlaneOffset);
	rotateFactor=cfs.retrieveValue<Scalar>("./rotateFactor",rotateFactor);
	invertDolly=cfs.retrieveValue<bool>("./invertDolly",invertDolly);
	screenDollyingDirection=cfs.retrieveValue<Vector>("./screenDollyingDirection",screenDollyingDirection);
	screenScalingDirection=cfs.retrieveValue<Vector>("./screenScalingDirection",screenScalingDirection);
	dollyFactor=cfs.retrieveValue<Scalar>("./dollyFactor",dollyFactor);
	scaleFactor=cfs.retrieveValue<Scalar>("./scaleFactor",scaleFactor);
	wheelDollyFactor=cfs.retrieveValue<Scalar>("./wheelDollyFactor",wheelDollyFactor);
	wheelScaleFactor=cfs.retrieveValue<Scalar>("./wheelScaleFactor",wheelScaleFactor);
	spinThreshold=cfs.retrieveValue<Scalar>("./spinThreshold",spinThreshold);
	showScreenCenter=cfs.retrieveValue<bool>("./showScreenCenter",showScreenCenter);
	interactWithWidgets=cfs.retrieveValue<bool>("./interactWithWidgets",interactWithWidgets);
	
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

Point MouseNavigationTool::calcScreenCenter(void) const
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

Point MouseNavigationTool::calcScreenPos(void) const
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

void MouseNavigationTool::startRotating(void)
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
	
	/* Go to rotating mode: */
	navigationMode=ROTATING;
	}

void MouseNavigationTool::startPanning(void)
	{
	/* Calculate initial motion position: */
	motionStart=calcScreenPos();
	
	preScale=getNavigationTransformation();
	
	/* Go to panning mode: */
	navigationMode=PANNING;
	}

void MouseNavigationTool::startDollying(void)
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
	
	/* Go to dollying mode: */
	navigationMode=DOLLYING;
	}

void MouseNavigationTool::startScaling(void)
	{
	/* Calculate the scaling center: */
	screenCenter=calcScreenCenter();
	
	/* Calculate initial motion position: */
	motionStart=calcScreenPos();
	
	preScale=NavTrackerState::translateFromOriginTo(screenCenter);
	postScale=NavTrackerState::translateToOriginFrom(screenCenter);
	postScale*=getNavigationTransformation();
	
	/* Go to scaling mode: */
	navigationMode=SCALING;
	}

MouseNavigationTool::MouseNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(factory,inputAssignment),
	 mouseAdapter(0),
	 currentValue(0),
	 dolly(MouseNavigationTool::factory->invertDolly),navigationMode(IDLE),
	 draggedWidget(0)
	{
	/* Find the mouse input device adapter controlling the input device: */
	mouseAdapter=dynamic_cast<InputDeviceAdapterMouse*>(getInputDeviceManager()->findInputDeviceAdapter(getDevice(0)));
	}

const ToolFactory* MouseNavigationTool::getFactory(void) const
	{
	return factory;
	}

void MouseNavigationTool::buttonCallback(int,int buttonIndex,InputDevice::ButtonCallbackData* cbData)
	{
	/* Process based on which button was pressed: */
	switch(buttonIndex)
		{
		case 0:
			if(cbData->newButtonState) // Button has just been pressed
				{
				/* Act depending on this tool's current state: */
				switch(navigationMode)
					{
					case IDLE:
					case SPINNING:
						if(factory->interactWithWidgets)
							{
							/* Check if the mouse pointer is over a GLMotif widget: */
							GLMotif::Event event(false);
							event.setWorldLocation(getDeviceRay(0));
							if(getWidgetManager()->pointerButtonDown(event))
								{
								if(navigationMode==SPINNING)
									{
									/* Deactivate this tool: */
									deactivate();
									}
								
								/* Go to widget interaction mode: */
								navigationMode=WIDGETING;
								
								/* Drag the entire root widget if the event's target widget is a title bar: */
								if(dynamic_cast<GLMotif::TitleBar*>(event.getTargetWidget())!=0)
									{
									/* Start dragging: */
									draggedWidget=event.getTargetWidget();
									
									/* Calculate the dragging transformation: */
									NavTrackerState initialTracker=NavTrackerState::translateFromOriginTo(calcScreenPos());
									preScale=Geometry::invert(initialTracker);
									GLMotif::WidgetManager::Transformation initialWidget=getWidgetManager()->calcWidgetTransformation(draggedWidget);
									preScale*=NavTrackerState(initialWidget);
									}
								else
									draggedWidget=0;
								}
							else
								{
								/* Try activating this tool: */
								if(navigationMode==SPINNING||activate())
									startRotating();
								}
							}
						else
							{
							/* Try activating this tool: */
							if(navigationMode==SPINNING||activate())
								startRotating();
							}
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
					case WIDGETING:
						{
						/* Deliver the event: */
						GLMotif::Event event(true);
						event.setWorldLocation(getDeviceRay(0));
						getWidgetManager()->pointerButtonUp(event);
						
						/* Deactivate this tool: */
						navigationMode=IDLE;
						draggedWidget=0;
						break;
						}
					
					case ROTATING:
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
			if(factory->invertDolly)
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

void MouseNavigationTool::valuatorCallback(int,int,InputDevice::ValuatorCallbackData* cbData)
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
						/* Calculate the dollying direction: */
						if(mouseAdapter!=0)
							dollyDirection=mouseAdapter->getWindow()->getViewer()->getHeadPosition()-calcScreenCenter();
						else
							dollyDirection=getMainViewer()->getHeadPosition()-calcScreenCenter();
						dollyDirection.normalize();
						
						/* Initialize the wheel dollying factor: */
						currentWheelScale=Scalar(1);
						
						preScale=getNavigationTransformation();
						
						/* Go to wheel dollying mode: */
						navigationMode=DOLLYING_WHEEL;
						}
					else
						{
						/* Calculate the scaling center: */
						screenCenter=calcScreenCenter();
						
						/* Initialize the wheel scaling factor: */
						currentWheelScale=Scalar(1);
						
						preScale=NavTrackerState::translateFromOriginTo(screenCenter);
						postScale=NavTrackerState::translateToOriginFrom(screenCenter);
						postScale*=getNavigationTransformation();
						
						/* Go to wheel scaling mode: */
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
	currentPos=calcScreenPos();
	
	/* Act depending on this tool's current state: */
	switch(navigationMode)
		{
		case IDLE:
			/* Do nothing */
			break;
		
		case WIDGETING:
			{
			/* Deliver the event: */
			GLMotif::Event event(true);
			event.setWorldLocation(getDeviceRay(0));
			getWidgetManager()->pointerMotion(event);
			
			if(draggedWidget!=0)
				{
				/* Update the dragged widget's transformation: */
				NavTrackerState current=NavTrackerState::translateFromOriginTo(currentPos);
				current*=preScale;
				getWidgetManager()->setPrimaryWidgetTransformation(draggedWidget,GLMotif::WidgetManager::Transformation(current));
				}
			break;
			}
		
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
		
		case SPINNING:
			{
			/* Calculate incremental rotation: */
			rotation.leftMultiply(NavTrackerState::rotate(NavTrackerState::Rotation::rotateScaledAxis(spinAngularVelocity*getFrameTime())));
			
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
		
		case DOLLYING_WHEEL:
			{
			/* Update the navigation transformation: */
			Scalar scale=currentValue;
			currentWheelScale+=factory->wheelDollyFactor*scale;
			NavTrackerState t=NavTrackerState::translate(dollyDirection*currentWheelScale);
			t*=preScale;
			setNavigationTransformation(t);
			break;
			}
		
		case SCALING_WHEEL:
			{
			/* Update the navigation transformation: */
			Scalar scale=currentValue;
			currentWheelScale*=Math::pow(factory->wheelScaleFactor,scale);
			NavTrackerState t=preScale;
			t*=NavTrackerState::scale(currentWheelScale);
			t*=postScale;
			setNavigationTransformation(t);
			break;
			}
		}
	}

void MouseNavigationTool::display(GLContextData& contextData) const
	{
	if(factory->showScreenCenter&&navigationMode!=IDLE&&navigationMode!=WIDGETING)
		{
		/* Save and set up OpenGL state: */
		glPushAttrib(GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_LINE_BIT);
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
