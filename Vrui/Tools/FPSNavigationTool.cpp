/***********************************************************************
FPSNavigationTool - Class encapsulating the navigation behaviour of a
typical first-person shooter (FPS) game.
Copyright (c) 2005-2010 Oliver Kreylos

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
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/Internal/InputDeviceAdapterMouse.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRScreen.h>
#include <Vrui/VRWindow.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/*****************************************
Methods of class FPSNavigationToolFactory:
*****************************************/

FPSNavigationToolFactory::FPSNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("FPSNavigationTool",toolManager),
	 rotateFactor(getInchFactor()*Scalar(12)),
	 moveSpeed(getInchFactor()*Scalar(120)),
	 fallAcceleration(getMeterFactor()*Scalar(9.81)),
	 probeSize(getInchFactor()*Scalar(12)),
	 maxClimb(getInchFactor()*Scalar(24)),
	 fixAzimuth(false),
	 showHud(true),
	 levelOnExit(false)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(5);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("SurfaceNavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	rotateFactor=cfs.retrieveValue<Scalar>("./rotateFactor",rotateFactor);
	moveSpeed=cfs.retrieveValue<Scalar>("./moveSpeed",moveSpeed);
	fallAcceleration=cfs.retrieveValue<Scalar>("./fallAcceleration",fallAcceleration);
	probeSize=cfs.retrieveValue<Scalar>("./probeSize",probeSize);
	maxClimb=cfs.retrieveValue<Scalar>("./maxClimb",maxClimb);
	fixAzimuth=cfs.retrieveValue<bool>("./fixAzimuth",fixAzimuth);
	showHud=cfs.retrieveValue<bool>("./showHud",showHud);
	levelOnExit=cfs.retrieveValue<bool>("./levelOnExit",levelOnExit);
	
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

const char* FPSNavigationToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	switch(buttonSlotIndex)
		{
		case 0:
			return "Start / Stop";
		
		case 1:
			return "Strafe Left";
		
		case 2:
			return "Strafe Right";
		
		case 3:
			return "Walk Backwards";
		
		case 4:
			return "Walk Forward";
		}
	
	/* Never reached; just to make compiler happy: */
	return 0;
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
		Ray ray=getButtonDeviceRay(0);
		
		/* Intersect the ray with the main screen: */
		ONTransform screenT=getMainScreen()->getScreenTransformation();
		Vector screenNormal=screenT.getDirection(2);
		Scalar screenOffset=screenT.getOrigin()*screenNormal;
		Scalar lambda=(screenOffset-ray.getOrigin()*screenNormal)/(ray.getDirection()*screenNormal);
		
		/* Return the intersection point in screen coordinates: */
		return screenT.inverseTransform(ray(lambda));
		}
	}

void FPSNavigationTool::applyNavState(void)
	{
	/* Compose and apply the navigation transformation: */
	NavTransform nav=physicalFrame;
	nav*=NavTransform::rotateAround(Point(0,0,headHeight),Rotation::rotateX(elevation));
	nav*=NavTransform::rotate(Rotation::rotateZ(azimuth));
	nav*=Geometry::invert(surfaceFrame);
	setNavigationTransformation(nav);
	}

void FPSNavigationTool::initNavState(void)
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
	AlignmentData ad(surfaceFrame,newSurfaceFrame,factory->probeSize,factory->maxClimb);
	Scalar roll;
	align(ad,azimuth,elevation,roll);
	
	/* Reset the movement velocity: */
	moveVelocity=Vector::zero;
	
	/* If the initial surface frame was above the surface, lift it back up and start falling: */
	Scalar z=newSurfaceFrame.inverseTransform(surfaceFrame.getOrigin())[2];
	if(z>Scalar(0))
		{
		newSurfaceFrame*=NavTransform::translate(Vector(Scalar(0),Scalar(0),z));
		moveVelocity[2]-=factory->fallAcceleration*getCurrentFrameTime();
		}
	
	/* Move the physical frame to the foot position, and adjust the surface frame accordingly: */
	newSurfaceFrame*=Geometry::invert(physicalFrame)*NavTransform::translate(footPos-headPos)*physicalFrame;
	physicalFrame.leftMultiply(NavTransform::translate(footPos-headPos));
	
	/* Apply the initial navigation state: */
	surfaceFrame=newSurfaceFrame;
	applyNavState();
	}

void FPSNavigationTool::stopNavState(void)
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
	 mouseAdapter(0),
	 numberRenderer(float(getUiSize())*1.5f,true)
	{
	}

void FPSNavigationTool::initialize(void)
	{
	/* Get a pointer to the input device's controlling adapter: */
	mouseAdapter=dynamic_cast<InputDeviceAdapterMouse*>(getInputDeviceManager()->findInputDeviceAdapter(getButtonDevice(0)));
	}

const ToolFactory* FPSNavigationTool::getFactory(void) const
	{
	return factory;
	}

void FPSNavigationTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	/* Process based on which button was pressed: */
	switch(buttonSlotIndex)
		{
		case 0:
			if(cbData->newButtonState) // Button has just been pressed
				{
				/* Act depending on this tool's current state: */
				if(isActive())
					{
					if(factory->levelOnExit)
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
					
					/* Deactivate this tool: */
					deactivate();
					stopNavState();
					}
				else
					{
					/* Try activating this tool: */
					if(activate())
						initNavState();
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
		bool update=false;
		
		/* Calculate the change in mouse position: */
		Point mousePos=calcMousePosition();
		if(mousePos[0]!=lastMousePos[0]||mousePos[1]!=lastMousePos[1])
			{
			/* Update the azimuth angle: */
			azimuth+=(mousePos[0]-lastMousePos[0])/factory->rotateFactor;
			if(azimuth<-Math::Constants<Scalar>::pi)
				azimuth+=Scalar(2)*Math::Constants<Scalar>::pi;
			else if(azimuth>Math::Constants<Scalar>::pi)
				azimuth-=Scalar(2)*Math::Constants<Scalar>::pi;
			
			/* Update the elevation angle: */
			elevation+=(mousePos[1]-lastMousePos[1])/factory->rotateFactor;
			if(elevation<Math::rad(Scalar(-90)))
				elevation=Math::rad(Scalar(-90));
			else if(elevation>Math::rad(Scalar(90)))
				elevation=Math::rad(Scalar(90));
			
			update=true;
			}
		
		/* Calculate the new head and foot positions: */
		Point newFootPos=projectToFloor(getMainViewer()->getHeadPosition());
		headHeight=Geometry::dist(getMainViewer()->getHeadPosition(),newFootPos);
		
		/* Check for movement: */
		if(moveVelocity!=Vector::zero||newFootPos!=footPos)
			update=true;
		
		if(update)
			{
			/* Create a physical navigation frame around the new foot position: */
			calcPhysicalFrame(newFootPos);
			
			/* Calculate the movement from walking: */
			Vector move=newFootPos-footPos;
			footPos=newFootPos;
			
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
			AlignmentData ad(surfaceFrame,newSurfaceFrame,factory->probeSize,factory->maxClimb);
			align(ad);
			
			if(!factory->fixAzimuth)
				{
				/* Have the azimuth angle track changes in the surface frame's rotation: */
				Rotation rot=Geometry::invert(initialOrientation)*newSurfaceFrame.getRotation();
				rot.leftMultiply(Rotation::rotateFromTo(rot.getDirection(2),Vector(0,0,1)));
				Vector x=rot.getDirection(0);
				azimuth+=Math::atan2(x[1],x[0]);
				if(azimuth<-Math::Constants<Scalar>::pi)
					azimuth+=Scalar(2)*Math::Constants<Scalar>::pi;
				else if(azimuth>Math::Constants<Scalar>::pi)
					azimuth-=Scalar(2)*Math::Constants<Scalar>::pi;
				}
			
			/* Check if the initial surface frame is above the surface: */
			Scalar z=newSurfaceFrame.inverseTransform(initialOrigin)[2];
			if(z>Scalar(0))
				{
				/* Lift the aligned frame back up to the original altitude and fall: */
				newSurfaceFrame*=NavTransform::translate(Vector(Scalar(0),Scalar(0),z));
				moveVelocity[2]-=factory->fallAcceleration*getCurrentFrameTime();
				}
			else
				{
				/* Stop falling: */
				moveVelocity[2]=Scalar(0);
				}
			
			/* Apply the newly aligned surface frame: */
			surfaceFrame=newSurfaceFrame;
			applyNavState();
			
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

void FPSNavigationTool::display(GLContextData& contextData) const
	{
	if(factory->showHud&&isActive())
		{
		/* Save and set up OpenGL state: */
		glPushAttrib(GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		
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
		
		/* Calculate the HUD layout: */
		Scalar crosshairSize1=getUiSize()*Scalar(0.5);
		Scalar crosshairSize2=getUiSize()*Scalar(1.5);
		Scalar ribbonY=viewport[3]-getUiSize()*Scalar(3);
		Scalar ribbonSize=(viewport[1]-viewport[0])*Scalar(0.25);
		Scalar ribbonTickSize=getUiSize();
		Scalar azimuthDeg=Math::deg(azimuth);
		
		/* Draw the foreground: */
		glLineWidth(1.0f);
		glColor(fgColor);
		glBegin(GL_LINES);
		
		/* Draw the crosshairs: */
		glVertex(centerPos[0]-crosshairSize2,centerPos[1]);
		glVertex(centerPos[0]-crosshairSize1,centerPos[1]);
		glVertex(centerPos[0]+crosshairSize1,centerPos[1]);
		glVertex(centerPos[0]+crosshairSize2,centerPos[1]);
		glVertex(centerPos[0],centerPos[1]-crosshairSize2);
		glVertex(centerPos[0],centerPos[1]-crosshairSize1);
		glVertex(centerPos[0],centerPos[1]+crosshairSize1);
		glVertex(centerPos[0],centerPos[1]+crosshairSize2);
		
		/* Draw the azimuth ribbon: */
		glVertex(centerPos[0]-ribbonTickSize,ribbonY+ribbonTickSize*Scalar(2));
		glVertex(centerPos[0],ribbonY);
		glVertex(centerPos[0]+ribbonTickSize,ribbonY+ribbonTickSize*Scalar(2));
		glVertex(centerPos[0],ribbonY);
		glVertex(centerPos[0]-ribbonSize,ribbonY);
		glVertex(centerPos[0]+ribbonSize,ribbonY);
		
		/* Draw the small azimuth tick marks: */
		for(int az=0;az<360;az+=10)
			{
			Scalar dist=Scalar(az)-azimuthDeg;
			if(dist<Scalar(-180))
				dist+=Scalar(360);
			if(dist>Scalar(180))
				dist-=Scalar(360);
			if(Math::abs(dist)<=Scalar(60))
				{
				glVertex(centerPos[0]+dist*ribbonSize/Scalar(60),ribbonY);
				glVertex(centerPos[0]+dist*ribbonSize/Scalar(60),ribbonY-ribbonTickSize);
				}
			}
		
		/* Draw the big azimuth tick marks: */
		for(int az=0;az<360;az+=30)
			{
			Scalar dist=Scalar(az)-azimuthDeg;
			if(dist<Scalar(-180))
				dist+=Scalar(360);
			if(dist>Scalar(180))
				dist-=Scalar(360);
			if(Math::abs(dist)<=Scalar(60))
				{
				Scalar x=centerPos[0]+dist*ribbonSize/Scalar(60);
				glVertex(x,ribbonY);
				glVertex(x,ribbonY-ribbonTickSize*Scalar(2));
				}
			}
		
		glEnd();
		
		/* Draw the azimuth labels: */
		for(int az=0;az<360;az+=30)
			{
			Scalar dist=Scalar(az)-azimuthDeg;
			if(dist<Scalar(-180))
				dist+=Scalar(360);
			if(dist>Scalar(180))
				dist-=Scalar(360);
			if(Math::abs(dist)<=Scalar(60))
				{
				/* Draw the azimuth number: */
				GLNumberRenderer::Vector pos;
				pos[0]=centerPos[0]+dist*ribbonSize/Scalar(60);
				pos[1]=ribbonY-ribbonTickSize*Scalar(2.5);
				pos[2]=0.0f;
				numberRenderer.drawNumber(pos,az,contextData,0,1);
				}
			}
		
		/* Go back to physical coordinates: */
		glPopMatrix();
		
		/* Restore OpenGL state: */
		glPopAttrib();
		}
	}

}
