/***********************************************************************
FPSNavigationTool - Class encapsulating the navigation behaviour of a
typical first-person shooter (FPS) game.
Copyright (c) 2005-2009 Oliver Kreylos

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

#include <Vrui/Tools/FPSNavigationTool.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/Ray.h>
#include <Geometry/Plane.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/InputDeviceAdapterMouse.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRScreen.h>
#include <Vrui/VRWindow.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

namespace Vrui {

/*****************************************
Methods of class FPSNavigationToolFactory:
*****************************************/

FPSNavigationToolFactory::FPSNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("FPSNavigationTool",toolManager),
	 rotateFactor(getInchFactor()*Scalar(12)),
	 moveSpeed(getInchFactor()*Scalar(120))
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,5);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("SurfaceNavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	rotateFactor=cfs.retrieveValue<Scalar>("./rotateFactor",rotateFactor);
	moveSpeed=cfs.retrieveValue<Scalar>("./moveSpeed",moveSpeed);
	
	/* Set tool class' factory pointer: */
	FPSNavigationTool::factory=this;
	}

FPSNavigationToolFactory::~FPSNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	FPSNavigationTool::factory=0;
	}

const char* FPSNavigationToolFactory::getName(void) const
	{
	return "FPS (Mouse Look + Buttons)";
	}

Tool* FPSNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new FPSNavigationTool(this,inputAssignment);
	}

void FPSNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveFPSNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("SurfaceNavigationTool");
	}

extern "C" ToolFactory* createFPSNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	FPSNavigationToolFactory* fpsNavigationToolFactory=new FPSNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return fpsNavigationToolFactory;
	}

extern "C" void destroyFPSNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/******************************************
Static elements of class FPSNavigationTool:
******************************************/

FPSNavigationToolFactory* FPSNavigationTool::factory=0;

/**********************************
Methods of class FPSNavigationTool:
**********************************/

Point FPSNavigationTool::calcMousePosition(void) const
	{
	if(mouseAdapter!=0)
		{
		/* Return the mouse position directly: */
		return Point(mouseAdapter->getMousePosition()[0],mouseAdapter->getMousePosition()[1],Scalar(0));
		}
	else
		{
		/* Calculate the ray equation: */
		Ray ray=getDeviceRay(0);
		
		/* Intersect the ray with the main screen: */
		ONTransform screenT=getMainScreen()->getScreenTransformation();
		Vector screenNormal=screenT.getDirection(2);
		Scalar screenOffset=screenT.getOrigin()*screenNormal;
		Scalar lambda=(screenOffset-ray.getOrigin()*screenNormal)/(ray.getDirection()*screenNormal);
		
		/* Return the intersection point in screen coordinates: */
		return screenT.inverseTransform(ray(lambda));
		}
	}

void FPSNavigationTool::startNavigating(void)
	{
	/* Initialize the navigation state: */
	if(mouseAdapter!=0)
		{
		/* Get the current cursor position in the controlling window: */
		for(int i=0;i<2;++i)
			oldMousePos[i]=mouseAdapter->getMousePosition()[i];
		
		/* Enable mouse warping on the controlling window: */
		mouseAdapter->getWindow()->hideCursor();
		mouseAdapter->getWindow()->getWindowCenterPos(lastMousePos.getComponents());
		lastMousePos[2]=Scalar(0);
		mouseAdapter->getWindow()->setCursorPosWithAdjust(lastMousePos.getComponents());
		mouseAdapter->setMousePosition(mouseAdapter->getWindow(),lastMousePos.getComponents());
		}
	else
		lastMousePos=calcMousePosition();
	
	/* Calculate the initial environment-aligned surface frame in navigation coordinates: */
	headPos=getMainViewer()->getHeadPosition();
	footPos=getFloorPlane().project(headPos);
	NavTransform initialSurfaceFrame=NavTransform::translateFromOriginTo(footPos);
	// initialSurfaceFrame*=NavTransform::rotate(navFrame);
	initialSurfaceFrame.leftMultiply(getInverseNavigationTransformation());
	
	/* Align the initial frame with the application's surface: */
	surfaceFrame=initialSurfaceFrame;
	align(surfaceFrame);
	
	/* Calculate rotation of initial frame relative to aligned frame: */
	Rotation rot=Geometry::invert(initialSurfaceFrame.getRotation())*surfaceFrame.getRotation();
	
	/* Align initial Z axis with aligned Y-Z plane: */
	Vector z=rot.getDirection(2);
	if(z[0]!=Scalar(0))
		{
		if(z[1]!=Scalar(0)||z[2]!=Scalar(0))
			{
			Vector rollAxis(Scalar(0),z[2],-z[1]);
			Scalar roll=Math::asin(z[0]);
			rot.leftMultiply(Rotation::rotateAxis(rollAxis,-roll));
			}
		else
			rot.leftMultiply(Rotation::rotateY(Math::rad(Scalar(z[0]>Scalar(0)?-90:90))));
		z=rot.getDirection(2);
		}
	
	/* Calculate the elevation angle: */
	angles[0]=Math::atan2(-z[1],z[2]);
	if(angles[0]<Math::rad(Scalar(-90)))
		angles[0]=Math::rad(Scalar(-90));
	else if(angles[0]>Math::rad(Scalar(90)))
		angles[0]=Math::rad(Scalar(90));
	rot.leftMultiply(Rotation::rotateX(-angles[0]));
	
	/* Calculate the azimuth angle: */
	Vector x=rot.getDirection(0);
	angles[1]=Math::atan2(x[1],x[0]);
	rot.leftMultiply(Rotation::rotateZ(-angles[1]));
	
	moveVelocity=Vector::zero;
	
	/* Apply the new navigation transformation: */
	applyNavigation();
	}

void FPSNavigationTool::applyNavigation(void)
	{
	/* Compose and apply the navigation transformation: */
	NavTransform nav=NavTransform::identity;
	nav*=NavTransform::translateFromOriginTo(headPos);
	// nav*=NavTransform::rotate(navFrame);
	nav*=NavTransform::rotate(Rotation::rotateX(angles[0]));
	nav*=NavTransform::rotate(Rotation::rotateZ(angles[1]));
	nav*=NavTransform::translate(footPos-headPos);
	nav*=Geometry::invert(surfaceFrame);
	setNavigationTransformation(nav);
	}

void FPSNavigationTool::stopNavigating(void)
	{
	if(mouseAdapter!=0)
		{
		/* Disable mouse warping on the controlling window: */
		mouseAdapter->setMousePosition(mouseAdapter->getWindow(),oldMousePos);
		mouseAdapter->getWindow()->setCursorPos(oldMousePos);
		mouseAdapter->getWindow()->showCursor();
		}
	}

FPSNavigationTool::FPSNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:SurfaceNavigationTool(factory,inputAssignment),
	 mouseAdapter(0)
	{
	}

void FPSNavigationTool::initialize(void)
	{
	/* Get a pointer to the input device's controlling adapter: */
	mouseAdapter=dynamic_cast<InputDeviceAdapterMouse*>(getInputDeviceManager()->findInputDeviceAdapter(getDevice(0)));
	
	/* Initialize the navigation frame: */
	Vector right=Geometry::cross(getForwardDirection(),getUpDirection());
	navFrame=Rotation::fromBaseVectors(right,getForwardDirection());
	}

const ToolFactory* FPSNavigationTool::getFactory(void) const
	{
	return factory;
	}

void FPSNavigationTool::buttonCallback(int,int buttonIndex,InputDevice::ButtonCallbackData* cbData)
	{
	/* Process based on which button was pressed: */
	switch(buttonIndex)
		{
		case 0:
			if(cbData->newButtonState) // Button has just been pressed
				{
				/* Act depending on this tool's current state: */
				if(isActive())
					{
					/* Deactivate this tool: */
					deactivate();
					stopNavigating();
					}
				else
					{
					/* Try activating this tool: */
					if(activate())
						startNavigating();
					}
				}
			break;
		
		case 1:
			if(cbData->newButtonState) // Button has just been pressed
				moveVelocity[0]-=factory->moveSpeed;
			else // Button has just been released
				moveVelocity[0]+=factory->moveSpeed;
			break;
		
		case 2:
			if(cbData->newButtonState) // Button has just been pressed
				moveVelocity[0]+=factory->moveSpeed;
			else // Button has just been released
				moveVelocity[0]-=factory->moveSpeed;
			break;
		
		case 3:
			if(cbData->newButtonState) // Button has just been pressed
				moveVelocity[1]-=factory->moveSpeed;
			else // Button has just been released
				moveVelocity[1]+=factory->moveSpeed;
			break;
		
		case 4:
			if(cbData->newButtonState) // Button has just been pressed
				moveVelocity[1]+=factory->moveSpeed;
			else // Button has just been released
				moveVelocity[1]-=factory->moveSpeed;
			break;
		}
	}

void FPSNavigationTool::frame(void)
	{
	/* Act depending on this tool's current state: */
	if(isActive())
		{
		/* Calculate the change in mouse position: */
		Point mousePos=calcMousePosition();
		if(mousePos[0]!=lastMousePos[0]||mousePos[1]!=lastMousePos[1]||moveVelocity[0]!=Scalar(0)||moveVelocity[1]!=Scalar(0))
			{
			angles[1]+=(mousePos[0]-lastMousePos[0])/factory->rotateFactor;
			angles[1]=Math::wrapRad(angles[1]);
			angles[0]+=(mousePos[1]-lastMousePos[1])/factory->rotateFactor;
			if(angles[0]<Math::rad(Scalar(-90)))
				angles[0]=Math::rad(Scalar(-90));
			else if(angles[0]>Math::rad(Scalar(90)))
				angles[0]=Math::rad(Scalar(90));
			
			/* Calculate the new head and foot positions: */
			headPos=getMainViewer()->getHeadPosition();
			Point newFootPos=getFloorPlane().project(headPos);
			
			/* Move the surface frame to the new foot position: */
			Vector move=newFootPos-footPos;
			footPos=newFootPos;
			
			/* Move by the current velocity: */
			Rotation yawT=Rotation::rotateZ(angles[1]);
			move+=yawT.inverseTransform(moveVelocity*getCurrentFrameTime());
			surfaceFrame.leftMultiply(NavTransform::translate(move));
			
			/* Re-align the surface frame */
			align(surfaceFrame);
			
			/* Set the new navigation transformation: */
			applyNavigation();
			
			if(mousePos[0]!=lastMousePos[0]||mousePos[1]!=lastMousePos[1])
				{
				if(mouseAdapter!=0)
					{
					/* Warp the cursor back to the center of the window: */
					mouseAdapter->getWindow()->setCursorPos(lastMousePos.getComponents());
					}
				else
					lastMousePos=mousePos;
				}
			}
		}
	}

}
