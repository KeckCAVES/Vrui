/***********************************************************************
WalkSurfaceNavigationTool - Version of the WalkNavigationTool that lets
a user navigate along an application-defined surface.
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

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLValueCoders.h>
#include <GL/GLGeometryWrappers.h>
#include <Vrui/Viewer.h>
#include <Vrui/ToolManager.h>

#include <Vrui/Tools/WalkSurfaceNavigationTool.h>

namespace Vrui {

/***********************************************************
Methods of class WalkSurfaceNavigationToolFactory::DataItem:
***********************************************************/

WalkSurfaceNavigationToolFactory::DataItem::DataItem(void)
	{
	/* Create tools' model display list: */
	modelListId=glGenLists(1);
	}

WalkSurfaceNavigationToolFactory::DataItem::~DataItem(void)
	{
	/* Destroy tools' model display list: */
	glDeleteLists(modelListId,1);
	}

/*************************************************
Methods of class WalkSurfaceNavigationToolFactory:
*************************************************/

WalkSurfaceNavigationToolFactory::WalkSurfaceNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("WalkSurfaceNavigationTool",toolManager),
	 centerOnActivation(false),
	 centerPoint(getDisplayCenter()),
	 moveSpeed(getDisplaySize()),
	 innerRadius(getDisplaySize()*Scalar(0.5)),outerRadius(getDisplaySize()*Scalar(0.75)),
	 centerViewDirection(getForwardDirection()),
	 rotateSpeed(Math::rad(Scalar(120))),
	 innerAngle(Math::rad(Scalar(30))),outerAngle(Math::rad(Scalar(120))),
	 fixAzimuth(false),
	 drawMovementCircles(true),
	 movementCircleColor(0.0f,1.0f,0.0f)
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("SurfaceNavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	centerOnActivation=cfs.retrieveValue<bool>("./centerOnActivation",centerOnActivation);
	centerPoint=cfs.retrieveValue<Point>("./centerPoint",centerPoint);
	centerPoint=getFloorPlane().project(centerPoint);
	moveSpeed=cfs.retrieveValue<Scalar>("./moveSpeed",moveSpeed);
	innerRadius=cfs.retrieveValue<Scalar>("./innerRadius",innerRadius);
	outerRadius=cfs.retrieveValue<Scalar>("./outerRadius",outerRadius);
	centerViewDirection=cfs.retrieveValue<Vector>("./centerViewDirection",centerViewDirection);
	centerViewDirection-=getUpDirection()*((centerViewDirection*getUpDirection())/Geometry::sqr(getUpDirection()));
	centerViewDirection.normalize();
	rotateSpeed=Math::rad(cfs.retrieveValue<Scalar>("./rotateSpeed",Math::deg(rotateSpeed)));
	innerAngle=Math::rad(cfs.retrieveValue<Scalar>("./innerAngle",Math::deg(innerAngle)));
	outerAngle=Math::rad(cfs.retrieveValue<Scalar>("./outerAngle",Math::deg(outerAngle)));
	fixAzimuth=cfs.retrieveValue<bool>("./fixAzimuth",fixAzimuth);
	drawMovementCircles=cfs.retrieveValue<bool>("./drawMovementCircles",drawMovementCircles);
	movementCircleColor=cfs.retrieveValue<Color>("./movementCircleColor",movementCircleColor);
	
	/* Set tool class' factory pointer: */
	WalkSurfaceNavigationTool::factory=this;
	}

WalkSurfaceNavigationToolFactory::~WalkSurfaceNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	WalkSurfaceNavigationTool::factory=0;
	}

const char* WalkSurfaceNavigationToolFactory::getName(void) const
	{
	return "Walk On Surface";
	}

Tool* WalkSurfaceNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new WalkSurfaceNavigationTool(this,inputAssignment);
	}

void WalkSurfaceNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

void WalkSurfaceNavigationToolFactory::initContext(GLContextData& contextData) const
	{
	if(drawMovementCircles)
		{
		/* Create a new data item: */
		DataItem* dataItem=new DataItem;
		contextData.addDataItem(this,dataItem);
		
		/* Create the tool model display list: */
		glNewList(dataItem->modelListId,GL_COMPILE);
		
		/* Set up OpenGL state: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(1.0f);
		glColor(movementCircleColor);
		
		/* Create a coordinate system for the floor plane: */
		Vector y=centerViewDirection;
		Vector x=Geometry::cross(y,getFloorPlane().getNormal());
		x.normalize();
		
		/* Draw the inner circle: */
		glBegin(GL_LINE_LOOP);
		for(int i=0;i<64;++i)
			{
			Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(i)/Scalar(64);
			glVertex(Point::origin-x*(Math::sin(angle)*innerRadius)+y*(Math::cos(angle)*innerRadius));
			}
		glEnd();
		
		/* Draw the outer circle: */
		glBegin(GL_LINE_LOOP);
		for(int i=0;i<64;++i)
			{
			Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(i)/Scalar(64);
			glVertex(Point::origin-x*(Math::sin(angle)*outerRadius)+y*(Math::cos(angle)*outerRadius));
			}
		glEnd();
		
		/* Draw the inner angle: */
		glBegin(GL_LINE_STRIP);
		glVertex(Point::origin-x*(Math::sin(innerAngle)*innerRadius)+y*(Math::cos(innerAngle)*innerRadius));
		glVertex(Point::origin);
		glVertex(Point::origin-x*(Math::sin(-innerAngle)*innerRadius)+y*(Math::cos(-innerAngle)*innerRadius));
		glEnd();
		
		/* Draw the outer angle: */
		glBegin(GL_LINE_STRIP);
		glVertex(Point::origin-x*(Math::sin(outerAngle)*outerRadius)+y*(Math::cos(outerAngle)*outerRadius));
		glVertex(Point::origin);
		glVertex(Point::origin-x*(Math::sin(-outerAngle)*outerRadius)+y*(Math::cos(-outerAngle)*outerRadius));
		glEnd();
		
		/* Reset OpenGL state: */
		glPopAttrib();
		
		glEndList();
		}
	}

extern "C" void resolveWalkSurfaceNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("SurfaceNavigationTool");
	}

extern "C" ToolFactory* createWalkSurfaceNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	WalkSurfaceNavigationToolFactory* walkSurfaceNavigationToolFactory=new WalkSurfaceNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return walkSurfaceNavigationToolFactory;
	}

extern "C" void destroyWalkSurfaceNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/**************************************************
Static elements of class WalkSurfaceNavigationTool:
**************************************************/

WalkSurfaceNavigationToolFactory* WalkSurfaceNavigationTool::factory=0;

/******************************************
Methods of class WalkSurfaceNavigationTool:
******************************************/

Point WalkSurfaceNavigationTool::getFootPoint(void) const
	{
	/* Get the viewer's current head position: */
	Point head=getMainViewer()->getHeadPosition();
	
	/* Project it onto the floor plane along the up direction: */
	const Vector& normal=getFloorPlane().getNormal();
	Scalar lambda=(getFloorPlane().getOffset()-head*normal)/(getUpDirection()*normal);
	return head+getUpDirection()*lambda;
	}

void WalkSurfaceNavigationTool::initNavState(void)
	{
	/* Create a navigation frame at the center point: */
	physicalFrame=NavTransform::translateFromOriginTo(centerPoint);
	Vector right=Geometry::cross(getForwardDirection(),getUpDirection());
	physicalFrame*=NavTransform::rotate(Rotation::fromBaseVectors(right,getForwardDirection()));
	
	/* Calculate the initial navigation frame in model coordinates: */
	NavTransform initialSurfaceFrame=getInverseNavigationTransformation();
	initialSurfaceFrame*=physicalFrame;
	
	/* Align the initial frame with the application's surface: */
	surfaceFrame=initialSurfaceFrame;
	align(surfaceFrame);
	
	/* Align the aligned frame's xy plane with the physical frame's xy plane: */
	Rotation rot=Geometry::invert(initialSurfaceFrame.getRotation())*surfaceFrame.getRotation();
	rot.leftMultiply(Rotation::rotateFromTo(rot.getDirection(2),Vector(0,0,1)));
	
	/* Calculate the initial azimuth angle: */
	Vector x=rot.getDirection(0);
	azimuth=Math::atan2(x[1],x[0]);
	
	/* Initialize the elevation angle: */
	elevation=Scalar(0);
	}

void WalkSurfaceNavigationTool::applyNavState(void) const
	{
	/* Compose and apply the navigation transformation: */
	NavTransform nav=physicalFrame;
	nav*=NavTransform::rotate(Rotation::rotateX(elevation));
	nav*=NavTransform::rotate(Rotation::rotateZ(azimuth));
	nav*=Geometry::invert(surfaceFrame);
	setNavigationTransformation(nav);
	}

WalkSurfaceNavigationTool::WalkSurfaceNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:SurfaceNavigationTool(factory,inputAssignment),
	 centerPoint(static_cast<const WalkSurfaceNavigationToolFactory*>(factory)->centerPoint)
	{
	}

const ToolFactory* WalkSurfaceNavigationTool::getFactory(void) const
	{
	return factory;
	}

void WalkSurfaceNavigationTool::buttonCallback(int,int buttonIndex,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Act depending on this tool's current state: */
		if(isActive())
			{
			/* Deactivate this tool: */
			deactivate();
			}
		else
			{
			/* Try activating this tool: */
			if(activate())
				{
				/* Store the center point for this navigation sequence: */
				if(factory->centerOnActivation)
					centerPoint=getFootPoint();
				
				/* Initialize the navigation state: */
				initNavState();
				}
			}
		}
	}

void WalkSurfaceNavigationTool::frame(void)
	{
	/* Act depending on this tool's current state: */
	if(isActive())
		{
		/* Get the current foot position: */
		Point foot=getFootPoint();
		
		/* Calculate the movement direction and speed: */
		Vector moveDir=centerPoint-foot;
		Scalar moveDirLen=moveDir.mag();
		Scalar speed=Scalar(0);
		if(moveDirLen>=factory->outerRadius)
			speed=factory->moveSpeed;
		else if(moveDirLen>factory->innerRadius)
			speed=factory->moveSpeed*(moveDirLen-factory->innerRadius)/(factory->outerRadius-factory->innerRadius);
		moveDir*=(speed*getFrameTime())/moveDirLen;
		
		/* Translate the surface frame: */
		moveDir=Rotation::rotateZ(-azimuth).transform(moveDir);
		surfaceFrame*=NavTransform::translate(moveDir);
		
		/* Re-align the surface frame with the surface: */
		NavTransform initialSurfaceFrame=surfaceFrame;
		align(surfaceFrame);
		
		Scalar azimuthDelta=Scalar(0);
		if(!factory->fixAzimuth)
			{
			/* Have the azimuth angle track changes in the surface frame's rotation: */
			Rotation rot=Geometry::invert(initialSurfaceFrame.getRotation())*surfaceFrame.getRotation();
			rot.leftMultiply(Rotation::rotateFromTo(rot.getDirection(2),Vector(0,0,1)));
			Vector x=rot.getDirection(0);
			azimuthDelta=Math::atan2(x[1],x[0]);
			}
		
		/* Project the viewing direction into the horizontal plane: */
		Vector viewDir=getMainViewer()->getViewDirection();
		viewDir-=getUpDirection()*((viewDir*getUpDirection())/Geometry::sqr(getUpDirection()));
		Scalar viewDir2=Geometry::sqr(viewDir);
		if(viewDir2!=Scalar(0))
			{
			/* Calculate the rotation speed: */
			Scalar viewAngleCos=(viewDir*factory->centerViewDirection)/Math::sqrt(viewDir2);
			Scalar viewAngle;
			if(viewAngleCos>Scalar(1)-Math::Constants<Scalar>::epsilon)
				viewAngle=Scalar(0);
			else if(viewAngleCos<Scalar(-1)+Math::Constants<Scalar>::epsilon)
				viewAngle=Math::Constants<Scalar>::pi;
			else
				viewAngle=Math::acos(viewAngleCos);
			Scalar rotateSpeed=Scalar(0);
			if(viewAngle>=factory->outerAngle)
				rotateSpeed=factory->rotateSpeed;
			else if(viewAngle>factory->innerAngle)
				rotateSpeed=factory->rotateSpeed*(viewAngle-factory->innerAngle)/(factory->outerAngle-factory->innerAngle);
			Vector x=Geometry::cross(factory->centerViewDirection,getUpDirection());
			if(viewDir*x<Scalar(0))
				rotateSpeed=-rotateSpeed;
			azimuthDelta+=rotateSpeed*getFrameTime();
			}
		
		/* Change the current azimuth angle: */
		azimuth+=azimuthDelta;
		if(azimuth<-Math::Constants<Scalar>::pi)
			azimuth+=Scalar(2)*Math::Constants<Scalar>::pi;
		else if(azimuth>=Math::Constants<Scalar>::pi)
			azimuth-=Scalar(2)*Math::Constants<Scalar>::pi;
		
		/* Apply the new transformation: */
		applyNavState();
		}
	}

void WalkSurfaceNavigationTool::display(GLContextData& contextData) const
	{
	if(factory->drawMovementCircles)
		{
		/* Get a pointer to the context entry: */
		WalkSurfaceNavigationToolFactory::DataItem* dataItem=contextData.retrieveDataItem<WalkSurfaceNavigationToolFactory::DataItem>(factory);
		
		/* Translate to the center point: */
		glPushMatrix();
		glTranslate(centerPoint-Point::origin);
		
		/* Execute the tool model display list: */
		glCallList(dataItem->modelListId);
		
		glPopMatrix();
		}
	}

}
