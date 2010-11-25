/***********************************************************************
SurfaceNavigationTool - Base class for navigation tools that are limited
to navigate along an application-defined surface.
Copyright (c) 2009-2010 Oliver Kreylos

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

#include <Vrui/SurfaceNavigationTool.h>

#include <Misc/FunctionCalls.h>
#include <Geometry/Vector.h>
#include <Geometry/Rotation.h>
#include <Geometry/Plane.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/*********************************************
Methods of class SurfaceNavigationToolFactory:
*********************************************/

SurfaceNavigationToolFactory::SurfaceNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("SurfaceNavigationTool",toolManager)
	{
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("NavigationTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	}

const char* SurfaceNavigationToolFactory::getName(void) const
	{
	return "Surface-Aligned Navigation";
	}

/**************************************
Methods of class SurfaceNavigationTool:
**************************************/

Point SurfaceNavigationTool::projectToFloor(const Point& p)
	{
	/* Project the given point onto the floor plane along the up direction: */
	const Vector& normal=getFloorPlane().getNormal();
	Scalar lambda=(getFloorPlane().getOffset()-p*normal)/(getUpDirection()*normal);
	return p+getUpDirection()*lambda;
	}

NavTransform& SurfaceNavigationTool::calcPhysicalFrame(const Point& basePoint)
	{
	/* Center the physical frame at the base point: */
	physicalFrame=NavTransform::translateFromOriginTo(basePoint);
	
	/* Align the physical frame with the forward and up directions: */
	Vector x=Geometry::cross(getForwardDirection(),getUpDirection());
	Vector y=Geometry::cross(getUpDirection(),x);
	physicalFrame*=NavTransform::rotate(Rotation::fromBaseVectors(x,y));
	
	return physicalFrame;
	}

void SurfaceNavigationTool::align(const SurfaceNavigationTool::AlignmentData& alignmentData)
	{
	/* Align the initial surface frame: */
	if(alignFunction!=0)
		{
		/* Call the alignment function: */
		(*alignFunction)(alignmentData);
		}
	else
		{
		/* Default behavior: Snap frame to z=0 plane and align it with identity transformation: */
		Vector translation=alignmentData.surfaceFrame.getTranslation();
		translation[2]=Scalar(0);
		Scalar scaling=alignmentData.surfaceFrame.getScaling();
		alignmentData.surfaceFrame=NavTransform(translation,Rotation::identity,scaling);
		}
	}

void SurfaceNavigationTool::align(const SurfaceNavigationTool::AlignmentData& alignmentData,Scalar& azimuth,Scalar& elevation,Scalar& roll)
	{
	/* Copy the initial surface frame: */
	NavTransform initialSurfaceFrame=alignmentData.surfaceFrame;
	
	/* Align the initial surface frame: */
	if(alignFunction!=0)
		{
		/* Call the alignment function: */
		(*alignFunction)(alignmentData);
		}
	else
		{
		/* Default behavior: Snap frame to z=0 plane and align it with identity transformation: */
		Vector translation=alignmentData.surfaceFrame.getTranslation();
		translation[2]=Scalar(0);
		Scalar scaling=alignmentData.surfaceFrame.getScaling();
		alignmentData.surfaceFrame=NavTransform(translation,Rotation::identity,scaling);
		}
	
	/* Calculate rotation of initial frame relative to aligned frame: */
	Rotation rot=Geometry::invert(initialSurfaceFrame.getRotation())*alignmentData.surfaceFrame.getRotation();
	
	/* Align initial Z axis with aligned Y-Z plane to calculate the roll angle: */
	Vector z=rot.getDirection(2);
	if(z[0]!=Scalar(0))
		{
		/* Check if the roll angle is well-defined: */
		if(z[1]!=Scalar(0)||z[2]!=Scalar(0))
			{
			/* Calculate the roll axis and angle: */
			Vector rollAxis(Scalar(0),z[2],-z[1]);
			roll=Math::asin(z[0]);
			
			/* Remove the roll component: */
			rot.leftMultiply(Rotation::rotateAxis(rollAxis,-roll));
			}
		else
			{
			/* Calculate the roll angle around the y axis: */
			roll=Math::rad(Scalar(z[0]>Scalar(0)?-90:90));
			
			/* Remove the roll component: */
			rot.leftMultiply(Rotation::rotateY(roll));
			}
		
		/* Get the roll-adjusted z axis: */
		z=rot.getDirection(2);
		}
	
	/* Calculate the elevation angle: */
	elevation=Math::atan2(-z[1],z[2]);
	
	/* Remove the elevation component: */
	rot.leftMultiply(Rotation::rotateX(-elevation));
	
	/* Calculate the azimuth angle: */
	Vector x=rot.getDirection(0);
	azimuth=Math::atan2(x[1],x[0]);
	}

SurfaceNavigationTool::SurfaceNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(factory,inputAssignment),
	 alignFunction(0)
	{
	}

SurfaceNavigationTool::~SurfaceNavigationTool(void)
	{
	/* Delete the alignment function call object: */
	delete alignFunction;
	}

void SurfaceNavigationTool::setAlignFunction(SurfaceNavigationTool::AlignFunction* newAlignFunction)
	{
	/* Delete the current alignment function call object: */
	delete alignFunction;
	
	/* Install the new alignment function call object: */
	alignFunction=newAlignFunction;
	}

}
