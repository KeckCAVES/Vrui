/***********************************************************************
UIManagerPlanar - UI manager class that aligns user interface components
on a fixed plane.
Copyright (c) 2015 Oliver Kreylos

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

#include <Vrui/Internal/UIManagerPlanar.h>

#include <stdexcept>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <GLMotif/Widget.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>

namespace Vrui {

/********************************
Methods of class UIManagerPlanar:
********************************/

UIManagerPlanar::UIManagerPlanar(const Misc::ConfigurationFileSection& configFileSection)
	:UIManager(configFileSection),
	 constrainMovement(configFileSection.retrieveValue<bool>("./constrainMovement",true))
	{
	/* Construct the default UI plane: */
	Vector z=getUpDirection();
	Vector x=getForwardDirection()^z;
	Vector n=x^z;
	plane=Plane(n,getDisplayCenter());
	
	/* Override the UI plane from the configuration file: */
	plane=configFileSection.retrieveValue<Plane>("./plane",plane);
	
	/* Override the "up" vector: */
	z=configFileSection.retrieveValue<Vector>("./up",z);
	
	/* Calculate the plane orientation: */
	x=z^plane.getNormal();
	z=plane.getNormal()^x;
	orientation=Rotation::fromBaseVectors(x,z);
	}

GLMotif::WidgetArranger::Transformation UIManagerPlanar::calcTopLevelTransform(GLMotif::Widget* topLevelWidget)
	{
	/* Project the default hot spot onto the plane: */
	Point planeHotSpot=plane.project(getHotSpot());
	
	/* Calculate the widget transformation: */
	Transformation result(planeHotSpot-Point::origin,orientation,Scalar(1));
	
	/* Align the widget's hot spot with the given hot spot: */
	GLMotif::Vector widgetHotSpot=topLevelWidget->calcHotSpot();
	result*=Transformation::translate(-Transformation::Vector(widgetHotSpot.getXyzw()));
	
	result.renormalize();
	return result;
	}

GLMotif::WidgetArranger::Transformation UIManagerPlanar::calcTopLevelTransform(GLMotif::Widget* topLevelWidget,const GLMotif::Point& hotSpot)
	{
	/* Project the given hot spot onto the plane: */
	Point planeHotSpot=plane.project(hotSpot);
	
	/* Calculate the widget transformation: */
	Transformation result(planeHotSpot-Point::origin,orientation,Scalar(1));
	
	/* Align the widget's hot spot with the given hot spot: */
	GLMotif::Vector widgetHotSpot=topLevelWidget->calcHotSpot();
	result*=Transformation::translate(-Transformation::Vector(widgetHotSpot.getXyzw()));
	
	result.renormalize();
	return result;
	}

GLMotif::WidgetArranger::Transformation UIManagerPlanar::calcTopLevelTransform(GLMotif::Widget* topLevelWidget,const GLMotif::WidgetArranger::Transformation& widgetToWorld)
	{
	if(constrainMovement)
		{
		/* Project the widget's hot spot onto the plane: */
		GLMotif::Vector widgetHotSpot=topLevelWidget->calcHotSpot();
		Point planeHotSpot=plane.project(widgetToWorld.transform(Point(widgetHotSpot.getXyzw())));
		
		/* Calculate the widget transformation: */
		Transformation result(planeHotSpot-Point::origin,orientation,Scalar(1));
		
		/* Align the widget's hot spot with the given hot spot: */
		result*=Transformation::translate(-Transformation::Vector(widgetHotSpot.getXyzw()));
		
		result.renormalize();
		return result;
		}
	else
		{
		/* Return the given transformation unchanged: */
		return widgetToWorld;
		}
	}

Point UIManagerPlanar::projectRay(const Ray& ray) const
	{
	/* Check if the ray intersects the UI plane: */
	Scalar divisor=plane.getNormal()*ray.getDirection();
	if(divisor!=Scalar(0))
		{
		/* Intersect the device ray with the UI plane: */
		Scalar lambda=(plane.getOffset()-plane.getNormal()*ray.getOrigin())/divisor;
		return ray(lambda);
		}
	else
		{
		/* Return the projection of the ray's origin onto the plane: */
		return plane.project(ray.getOrigin());
		}
	}

void UIManagerPlanar::projectDevice(InputDevice* device) const
	{
	/* Get the device's ray: */
	Ray ray=device->getRay();
	
	/* Check if the device ray intersects the UI plane: */
	Scalar divisor=plane.getNormal()*ray.getDirection();
	Scalar lambda(0);
	Point devicePos;
	if(divisor!=Scalar(0))
		{
		/* Intersect the device ray with the UI plane: */
		lambda=(plane.getOffset()-plane.getNormal()*ray.getOrigin())/divisor;
		devicePos=ray(lambda);
		}
	else
		{
		/* Project the device's position onto the plane: */
		devicePos=plane.project(device->getPosition());
		}
	
	/* Move the device to the intersection point (rotate by 90 degrees to have y axis point into screen): */
	Rotation newOrientation=orientation;
	newOrientation*=Rotation::rotateX(Math::rad(Scalar(-90)));
	newOrientation.renormalize();
	device->setTransformation(TrackerState(devicePos-Point::origin,newOrientation));
	
	/* Update the device's ray: */
	device->setDeviceRay(newOrientation.inverseTransform(ray.getDirection()),-lambda);
	}

ONTransform UIManagerPlanar::calcUITransform(const Point& point) const
	{
	/* Project the given point into the plane: */
	Point planePoint=plane.project(point);
	
	/* Calculate and return the UI transformation: */
	return ONTransform(planePoint-Point::origin,orientation);
	}

ONTransform UIManagerPlanar::calcUITransform(const Ray& ray) const
	{
	throw std::runtime_error("UIManagerPlanar::calcUITransform: Not implemented");
	}

ONTransform UIManagerPlanar::calcUITransform(const InputDevice* device) const
	{
	throw std::runtime_error("UIManagerPlanar::calcUITransform: Not implemented");
	}

}
