/***********************************************************************
MouseNavigationTool - Class encapsulating the navigation behaviour of a
mouse in the OpenInventor SoXtExaminerViewer.
Copyright (c) 2004-2010 Oliver Kreylos

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
#include <Vrui/Internal/InputDeviceAdapterMouse.h>
#include <Vrui/ToolManager.h>

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
	 spinThreshold(getUiSize()*Scalar(2)),
	 showScreenCenter(true),
	 interactWithWidgets(true)
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

const char* MouseNavigationToolFactory::getValuatorFunction(int) const
	{
	return "Quick Zoom/Dolly";
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
	/* Get the transformation of the screen currently containing the input device: */
	Scalar viewport[4];
	ONTransform screenT=getMouseScreenTransform(mouseAdapter,viewport);
	
	/* Calculate the screen's center: */
	Point center;
	center[0]=Math::mid(viewport[0],viewport[1]);
	center[1]=Math::mid(viewport[2],viewport[3]);
	center[2]=Scalar(0);
	
	/* Transform the center position to physical coordinates: */
	return screenT.transform(center);
	}

Point MouseNavigationTool::calcScreenPos(void) const
	{
	/* Calculate the ray equation: */
	Ray ray=getButtonDeviceRay(0);
	
	/* Get the transformation of the screen currently containing the input device: */
	Scalar viewport[4];
	ONTransform screenT=getMouseScreenTransform(mouseAdapter,viewport);
	
	/* Intersect the device ray with the screen: */
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
	
	/* Get the transformation of the screen currently containing the input device: */
	Scalar viewport[4];
	ONTransform screenT=getMouseScreenTransform(mouseAdapter,viewport);
	
	/* Calculate the rotation offset vector: */
	rotateOffset=screenT.transform(Vector(0,0,factory->rotatePlaneOffset));
	
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
	if(mouseAdapter!=0&&mouseAdapter->getWindow()!=0)
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
	 GUIInteractor(false,Scalar(0),getButtonDevice(0)),
	 mouseAdapter(0),
	 currentValue(0),
	 dolly(MouseNavigationTool::factory->invertDolly),navigationMode(IDLE)
	{
	/* Find the mouse input device adapter controlling the input device: */
	InputDevice* rootDevice=getInputGraphManager()->getRootDevice(getButtonDevice(0));
	mouseAdapter=dynamic_cast<InputDeviceAdapterMouse*>(getInputDeviceManager()->findInputDeviceAdapter(rootDevice));
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
						if(factory->interactWithWidgets)
							{
							/* Check if the GUI interactor accepts the event: */
							GUIInteractor::updateRay();
							if(GUIInteractor::buttonDown(false))
								{
								/* Deactivate this tool if it is spinning: */
								if(navigationMode==SPINNING)
									deactivate();
								
								/* Go to widget interaction mode: */
								navigationMode=WIDGETING;
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
						if(GUIInteractor::isActive())
							{
							/* Deliver the event: */
							GUIInteractor::buttonUp();
							}
						
						/* Deactivate this tool: */
						navigationMode=IDLE;
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
	if(factory->interactWithWidgets)
		{
		/* Update the GUI interactor: */
		GUIInteractor::updateRay();
		GUIInteractor::move();
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
			Scalar viewport[4];
			ONTransform screenT=getMouseScreenTransform(mouseAdapter,viewport);
			Vector dollyingDirection=screenT.transform(factory->screenDollyingDirection);
			
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
			Scalar viewport[4];
			ONTransform screenT=getMouseScreenTransform(mouseAdapter,viewport);
			Vector scalingDirection=screenT.transform(factory->screenScalingDirection);
			
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
		
		default:
			;
		}
	}

void MouseNavigationTool::display(GLContextData& contextData) const
	{
	if(factory->interactWithWidgets)
		{
		/* Draw the GUI interactor's state: */
		GUIInteractor::glRenderAction(contextData);
		}
	
	if(factory->showScreenCenter&&navigationMode!=IDLE&&navigationMode!=WIDGETING)
		{
		/* Save and set up OpenGL state: */
		glPushAttrib(GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glDepthFunc(GL_LEQUAL);
		
		/* Go to screen coordinates: */
		glPushMatrix();
		Scalar viewport[4];
		glMultMatrix(getMouseScreenTransform(mouseAdapter,viewport));
		
		/* Determine the crosshair colors: */
		Color bgColor=getBackgroundColor();
		Color fgColor;
		for(int i=0;i<3;++i)
			fgColor[i]=1.0f-bgColor[i];
		fgColor[3]=bgColor[3];
		
		/* Calculate the window's or screen's center: */
		Scalar centerPos[2];
		for(int i=0;i<2;++i)
			centerPos[i]=Math::mid(viewport[2*i+0],viewport[2*i+1]);
		
		/* Calculate the endpoints of the screen's crosshair lines in screen coordinates: */
		Point l=Point(viewport[0],centerPos[1],Scalar(0));
		Point r=Point(viewport[1],centerPos[1],Scalar(0));
		Point b=Point(centerPos[0],viewport[2],Scalar(0));
		Point t=Point(centerPos[0],viewport[3],Scalar(0));
		
		/* Draw the screen crosshairs: */
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
