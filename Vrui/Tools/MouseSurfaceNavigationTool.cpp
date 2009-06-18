/***********************************************************************
MouseSurfaceNavigationTool - Class for navigation tools that use the
mouse to move along an application-defined surface.
Copyright (c) 2009 Oliver Kreylos

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

// DEBUGGING
//#include <iostream>
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

/**************************************************
Methods of class MouseSurfaceNavigationToolFactory:
**************************************************/

MouseSurfaceNavigationToolFactory::MouseSurfaceNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("MouseSurfaceNavigationTool",toolManager),
	 rotateFactor(getInchFactor()*Scalar(3)),
	 screenScalingDirection(0,-1,0),
	 scaleFactor(getInchFactor()*Scalar(3)),
	 wheelScaleFactor(Scalar(0.5)),
	 fixAzimuth(false),
	 showCompass(true),
	 showScreenCenter(false),
	 interactWithWidgets(true)
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,2);
	layout.setNumValuators(0,1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("SurfaceNavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	rotateFactor=cfs.retrieveValue<Scalar>("./rotateFactor",rotateFactor);
	screenScalingDirection=cfs.retrieveValue<Vector>("./screenScalingDirection",screenScalingDirection);
	scaleFactor=cfs.retrieveValue<Scalar>("./scaleFactor",scaleFactor);
	wheelScaleFactor=cfs.retrieveValue<Scalar>("./wheelScaleFactor",wheelScaleFactor);
	fixAzimuth=cfs.retrieveValue<bool>("./fixAzimuth",fixAzimuth);
	showCompass=cfs.retrieveValue<bool>("./showCompass",showCompass);
	showScreenCenter=cfs.retrieveValue<bool>("./showScreenCenter",showScreenCenter);
	interactWithWidgets=cfs.retrieveValue<bool>("./interactWithWidgets",interactWithWidgets);
	
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

/***************************************************
Static elements of class MouseSurfaceNavigationTool:
***************************************************/

MouseSurfaceNavigationToolFactory* MouseSurfaceNavigationTool::factory=0;

/*******************************************
Methods of class MouseSurfaceNavigationTool:
*******************************************/

void MouseSurfaceNavigationTool::initNavState(void)
	{
	/* Calculate the initial environment-aligned surface frame in navigation coordinates: */
	physicalFrame=NavTransform::translateFromOriginTo(getDisplayCenter());
	Vector right=Geometry::cross(getForwardDirection(),getUpDirection());
	physicalFrame*=NavTransform::rotate(Rotation::fromBaseVectors(right,getForwardDirection()));
	NavTransform initialSurfaceFrame=getInverseNavigationTransformation()*physicalFrame;
	// DEBUGGING
	//std::cout<<"Initial nav transform: "<<getNavigationTransformation()<<std::endl;
	// DEBUGGING
	//std::cout<<"initialSurfaceFrame = "<<initialSurfaceFrame<<std::endl;
	
	/* Align the initial frame with the application's surface: */
	surfaceFrame=initialSurfaceFrame;
	align(surfaceFrame);
	// DEBUGGING
	//std::cout<<"surfaceFrame = "<<surfaceFrame<<std::endl;
	
	/* Calculate rotation of initial frame relative to aligned frame: */
	Rotation rot=Geometry::invert(initialSurfaceFrame.getRotation())*surfaceFrame.getRotation();
	
	/* Align initial Z axis with aligned Y-Z plane: */
	Vector z=rot.getDirection(2);
	// DEBUGGING
	//std::cout<<"Z = "<<z[0]<<", "<<z[1]<<", "<<z[2]<<std::endl;
	if(z[0]!=Scalar(0))
		{
		#if 1
		if(z[1]!=Scalar(0)||z[2]!=Scalar(0))
			{
			Vector rollAxis(Scalar(0),z[2],-z[1]);
			Scalar roll=Math::asin(z[0]);
			// DEBUGGING
			//std::cout<<"Roll = "<<Math::deg(roll)<<std::endl;
			rot.leftMultiply(Rotation::rotateAxis(rollAxis,-roll));
			}
		else
			{
			rot.leftMultiply(Rotation::rotateY(Math::rad(Scalar(z[0]>Scalar(0)?-90:90))));
			// DEBUGGING
			//std::cout<<"Roll = "<<Scalar(z[0]>Scalar(0)?-90:90)<<std::endl;
			}
		#else
		Scalar roll=Math::atan2(-z[0],z[2]);
		// DEBUGGING
		//std::cout<<"Roll = "<<Math::deg(roll)<<std::endl;
		rot.leftMultiply(Rotation::rotateY(roll));
		#endif
		z=rot.getDirection(2);
		// DEBUGGING
		//std::cout<<"Z' = "<<z[0]<<", "<<z[1]<<", "<<z[2]<<std::endl;
		}
	
	/* Calculate the elevation angle: */
	elevation=Math::atan2(-z[1],z[2]);
	if(elevation<Scalar(0))
		elevation=Scalar(0);
	else if(elevation>Math::rad(Scalar(90)))
		elevation=Math::rad(Scalar(90));
	rot.leftMultiply(Rotation::rotateX(-elevation));
	
	/* Calculate the azimuth angle: */
	Vector x=rot.getDirection(0);
	// DEBUGGING
	//std::cout<<"X = "<<x[0]<<", "<<x[1]<<", "<<x[2]<<std::endl;
	azimuth=Math::atan2(x[1],x[0]);
	rot.leftMultiply(Rotation::rotateZ(-azimuth));
	
	// DEBUGGING
	//std::cout<<"Nav state: "<<Math::deg(azimuth)<<", "<<Math::deg(elevation)<<", "<<rot<<std::endl;
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

Point MouseSurfaceNavigationTool::calcScreenPos(void) const
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

MouseSurfaceNavigationTool::MouseSurfaceNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:SurfaceNavigationTool(factory,inputAssignment),
	 mouseAdapter(0),
	 currentValue(0),
	 navigationMode(IDLE),
	 draggedWidget(0)
	{
	/* Find the mouse input device adapter controlling the input device: */
	mouseAdapter=dynamic_cast<InputDeviceAdapterMouse*>(getInputDeviceManager()->findInputDeviceAdapter(getDevice(0)));
	}

const ToolFactory* MouseSurfaceNavigationTool::getFactory(void) const
	{
	return factory;
	}

void MouseSurfaceNavigationTool::buttonCallback(int,int buttonIndex,InputDevice::ButtonCallbackData* cbData)
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
						if(factory->interactWithWidgets)
							{
							/* Check if the mouse pointer is over a GLMotif widget: */
							GLMotif::Event event(false);
							event.setWorldLocation(getDeviceRay(0));
							if(getWidgetManager()->pointerButtonDown(event))
								{
								/* Go to widget interaction mode: */
								navigationMode=WIDGETING;
								
								/* Drag the entire root widget if the event's target widget is a title bar: */
								if(dynamic_cast<GLMotif::TitleBar*>(event.getTargetWidget())!=0)
									{
									/* Start dragging: */
									draggedWidget=event.getTargetWidget();
									
									/* Calculate the dragging transformation: */
									NavTrackerState initialTracker=NavTrackerState::translateFromOriginTo(calcScreenPos());
									initialWidget=Geometry::invert(initialTracker);
									initialWidget*=getWidgetManager()->calcWidgetTransformation(draggedWidget);
									}
								else
									draggedWidget=0;
								}
							else
								{
								/* Try activating this tool: */
								if(activate())
									{
									initNavState();
									lastPos=calcScreenPos();
									navigationMode=ROTATING;
									}
								}
							}
						else
							{
							/* Try activating this tool: */
							if(activate())
								{
								initNavState();
								lastPos=calcScreenPos();
								navigationMode=ROTATING;
								}
							}
						break;
					
					case PANNING:
						lastPos=calcScreenPos();
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
						/* Deactivate this tool: */
						deactivate();
						
						/* Go to idle mode: */
						navigationMode=IDLE;
						break;
					
					case SCALING:
						lastPos=calcScreenPos();
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
						/* Try activating this tool: */
						if(activate())
							{
							initNavState();
							lastPos=calcScreenPos();
							navigationMode=PANNING;
							}
						break;
					
					case ROTATING:
						lastPos=calcScreenPos();
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
						/* Deactivate this tool: */
						deactivate();
						
						/* Go to idle mode: */
						navigationMode=IDLE;
						break;
					
					case SCALING:
						lastPos=calcScreenPos();
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

void MouseSurfaceNavigationTool::valuatorCallback(int,int,InputDevice::ValuatorCallbackData* cbData)
	{
	currentValue=Scalar(cbData->newValuatorValue);
	if(currentValue!=Scalar(0))
		{
		/* Act depending on this tool's current state: */
		switch(navigationMode)
			{
			case IDLE:
				/* Try activating this tool: */
				if(activate())
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
				current*=initialWidget;
				getWidgetManager()->setPrimaryWidgetTransformation(draggedWidget,GLMotif::WidgetManager::Transformation(current));
				}
			break;
			}
		
		case ROTATING:
			{
			/* Calculate the rotation vector: */
			Vector delta=currentPos-lastPos;
			lastPos=currentPos;
			
			/* Adjust the azimuth angle: */
			azimuth+=delta[0]/factory->rotateFactor;
			if(azimuth<-Math::Constants<Scalar>::pi)
				azimuth+=Scalar(2)*Math::Constants<Scalar>::pi;
			else if(azimuth>Math::Constants<Scalar>::pi)
				azimuth-=Scalar(2)*Math::Constants<Scalar>::pi;
			
			/* Adjust the elevation angle: */
			elevation-=delta[2]/factory->rotateFactor;
			if(elevation<Scalar(0))
				elevation=Scalar(0);
			else if(elevation>Math::rad(Scalar(90)))
				elevation=Math::rad(Scalar(90));
			
			/* Apply the new transformation: */
			applyNavState();
			break;
			}
		
		case PANNING:
			{
			/* Calculate the translation vector: */
			Vector delta=currentPos-lastPos;
			lastPos=currentPos;
			delta=Rotation::rotateX(Math::rad(Scalar(-90))).transform(delta);
			delta=Rotation::rotateZ(-azimuth).transform(delta);
			
			/* Translate the surface frame: */
			surfaceFrame*=NavTransform::translate(-delta);
			
			/* Re-align the surface frame with the surface: */
			NavTransform initialSurfaceFrame=surfaceFrame;
			align(surfaceFrame);
			
			if(!factory->fixAzimuth)
				{
				/* Have the azimuth angle track changes in the surface frame's rotation: */
				Rotation rot=Geometry::invert(initialSurfaceFrame.getRotation())*surfaceFrame.getRotation();
				rot.leftMultiply(Rotation::rotateFromTo(rot.getDirection(2),Vector(0,0,1)));
				Vector x=rot.getDirection(0);
				Scalar azimuthDelta=Math::atan2(x[1],x[0]);
				azimuth+=azimuthDelta;
				if(azimuth<-Math::Constants<Scalar>::pi)
					azimuth+=Scalar(2)*Math::Constants<Scalar>::pi;
				else if(azimuth>Math::Constants<Scalar>::pi)
					azimuth-=Scalar(2)*Math::Constants<Scalar>::pi;
				}
			
			/* Apply the new transformation: */
			applyNavState();
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
			
			/* Scale the surface frame: */
			Scalar scale=((currentPos-lastPos)*scalingDirection)/factory->scaleFactor;
			lastPos=currentPos;
			surfaceFrame*=NavTrackerState::scale(Math::exp(scale));
			
			/* Apply the new transformation: */
			applyNavState();
			break;
			}
		
		case SCALING_WHEEL:
			{
			/* Scale the surface frame: */
			surfaceFrame*=NavTrackerState::scale(Math::exp(factory->wheelScaleFactor*currentValue));
			
			/* Apply the new transformation: */
			applyNavState();
			break;
			}
		}
	}

void MouseSurfaceNavigationTool::display(GLContextData& contextData) const
	{
	if(factory->showCompass||(factory->showScreenCenter&&navigationMode!=IDLE&&navigationMode!=WIDGETING))
		{
		/* Save and set up OpenGL state: */
		glPushAttrib(GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		
		/* Get a pointer to the screen the mouse is on: */
		const VRWindow* window=0;
		const VRScreen* screen;
		if(mouseAdapter!=0&&mouseAdapter->getWindow()!=0)
			{
			window=mouseAdapter->getWindow();
			screen=window->getVRScreen();
			}
		else
			screen=getMainScreen();
		ONTransform screenT=screen->getScreenTransformation();
		
		/* Go to screen coordinates: */
		glPushMatrix();
		glMultMatrix(screenT);
		
		/* Determine the crosshair colors: */
		Color bgColor=getBackgroundColor();
		Color fgColor;
		for(int i=0;i<3;++i)
			fgColor[i]=1.0f-bgColor[i];
		fgColor[3]=bgColor[3];
		
		if(factory->showCompass)
			{
			Scalar dialSize=getUiSize()*Scalar(5);
			Scalar thickness=getUiSize()*Scalar(0.5);
			
			glPushMatrix();
			if(window!=0)
				glTranslate(window->getScreenViewport()[1]-dialSize*Scalar(3),window->getScreenViewport()[3]-dialSize*Scalar(3),0);
			else
				glTranslate(screen->getWidth()-dialSize*Scalar(3),screen->getHeight()-dialSize*Scalar(3),0);
			glRotate(Math::deg(azimuth),0,0,1);
			
			glLineWidth(3.0f);
			glColor(bgColor);
			glBegin(GL_LINE_LOOP);
			for(int i=0;i<30;++i)
				{
				Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*(Scalar(i)+Scalar(0.5))/Scalar(30);
				glVertex(Math::sin(angle)*(dialSize+thickness),Math::cos(angle)*(dialSize+thickness));
				}
			for(int i=0;i<30;++i)
				{
				Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*(Scalar(i)+Scalar(0.5))/Scalar(30);
				glVertex(Math::sin(angle)*(dialSize-thickness),Math::cos(angle)*(dialSize-thickness));
				}
			glEnd();
			glBegin(GL_LINE_LOOP);
			glVertex(thickness,dialSize*Scalar(-1.25));
			glVertex(thickness,dialSize*Scalar(1.25));
			glVertex(thickness*Scalar(2.5),dialSize*Scalar(1.25));
			glVertex(Scalar(0),dialSize*Scalar(1.75));
			glVertex(-thickness*Scalar(2.5),dialSize*Scalar(1.25));
			glVertex(-thickness,dialSize*Scalar(1.25));
			glVertex(-thickness,dialSize*Scalar(-1.25));
			glEnd();
			glBegin(GL_LINES);
			glVertex(-dialSize*Scalar(1.25),Scalar(0));
			glVertex(dialSize*Scalar(1.25),Scalar(0));
			glEnd();
			
			glLineWidth(1.0f);
			glColor(fgColor);
			glBegin(GL_LINE_LOOP);
			for(int i=0;i<30;++i)
				{
				Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*(Scalar(i)+Scalar(0.5))/Scalar(30);
				glVertex(Math::sin(angle)*(dialSize+thickness),Math::cos(angle)*(dialSize+thickness));
				}
			for(int i=0;i<30;++i)
				{
				Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*(Scalar(i)+Scalar(0.5))/Scalar(30);
				glVertex(Math::sin(angle)*(dialSize-thickness),Math::cos(angle)*(dialSize-thickness));
				}
			glEnd();
			glBegin(GL_LINE_LOOP);
			glVertex(thickness,dialSize*Scalar(-1.25));
			glVertex(thickness,dialSize*Scalar(1.25));
			glVertex(thickness*Scalar(2.5),dialSize*Scalar(1.25));
			glVertex(Scalar(0),dialSize*Scalar(1.75));
			glVertex(-thickness*Scalar(2.5),dialSize*Scalar(1.25));
			glVertex(-thickness,dialSize*Scalar(1.25));
			glVertex(-thickness,dialSize*Scalar(-1.25));
			glEnd();
			glBegin(GL_LINES);
			glVertex(-dialSize*Scalar(1.25),Scalar(0));
			glVertex(dialSize*Scalar(1.25),Scalar(0));
			glEnd();
			
			glPopMatrix();
			}
		
		if(factory->showScreenCenter&&navigationMode!=IDLE&&navigationMode!=WIDGETING)
			{
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
			}
		
		/* Go back to physical coordinates: */
		glPopMatrix();
		
		/* Restore OpenGL state: */
		glPopAttrib();
		}
	}

}
