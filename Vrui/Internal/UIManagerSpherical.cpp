/***********************************************************************
UIManagerSpherical - UI manager class that aligns user interface
components on a fixed sphere surrounding the viewer.
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

#include <Vrui/Internal/UIManagerSpherical.h>

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

/***********************************
Methods of class UIManagerSpherical:
***********************************/

GLMotif::WidgetArranger::Transformation UIManagerSpherical::calcTopLevelTransformInternal(GLMotif::Widget* topLevelWidget,const Point& hotSpot) const
	{
	/* Project the given hot spot onto the sphere: */
	Vector d=hotSpot-sphere.getCenter();
	Scalar dLen=d.mag();
	if(dLen==Scalar(0))
		{
		d=getForwardDirection();
		dLen=d.mag();
		}
	Scalar r=sphere.getRadius();
	if(alignSecant)
		{
		/* Adjust the projection radius such that the widget is halfway in and out of the sphere: */
		const GLMotif::Box& exterior=topLevelWidget->getExterior();
		Scalar widgetSize=Scalar(Math::sqr(exterior.size[0])+Math::sqr(exterior.size[1]));
		r=Math::sqrt(Math::sqr(r)-widgetSize*Scalar(0.25));
		}
	Point sphereHotSpot=sphere.getCenter()+d*(r/dLen);
	
	/* Calculate the widget transformation: */
	Vector x=d^getUpDirection();
	if(x.mag()==Scalar(0))
		x=getForwardDirection()^getUpDirection();
	Vector y=x^d;
	Transformation result(sphereHotSpot-Point::origin,Rotation::fromBaseVectors(x,y),Scalar(1));
	
	/* Align the widget's hot spot with the given hot spot: */
	GLMotif::Vector widgetHotSpot=topLevelWidget->calcHotSpot();
	result*=Transformation::translate(-Transformation::Vector(widgetHotSpot.getXyzw()));
	
	result.renormalize();
	return result;
	}

UIManagerSpherical::UIManagerSpherical(const Misc::ConfigurationFileSection& configFileSection)
	:UIManager(configFileSection),
	 sphere(Point::origin,Scalar(1)),
	 alignSecant(configFileSection.retrieveValue<bool>("./alignSecant",true)),
	 constrainMovement(configFileSection.retrieveValue<bool>("./constrainMovement",true))
	{
	/* Configure the UI sphere: */
	sphere.setCenter(configFileSection.retrieveValue<Point>("./sphereCenter"));
	sphere.setRadius(configFileSection.retrieveValue<Scalar>("./sphereRadius",Geometry::dist(sphere.getCenter(),getDisplayCenter())));
	}

GLMotif::WidgetArranger::Transformation UIManagerSpherical::calcTopLevelTransform(GLMotif::Widget* topLevelWidget)
	{
	/* Return a top-level transformation for the default hot spot: */
	return calcTopLevelTransformInternal(topLevelWidget,getHotSpot());
	}

GLMotif::WidgetArranger::Transformation UIManagerSpherical::calcTopLevelTransform(GLMotif::Widget* topLevelWidget,const GLMotif::Point& hotSpot)
	{
	/* Return a top-level transformation for the given hot spot: */
	return calcTopLevelTransformInternal(topLevelWidget,hotSpot);
	}

GLMotif::WidgetArranger::Transformation UIManagerSpherical::calcTopLevelTransform(GLMotif::Widget* topLevelWidget,const GLMotif::WidgetArranger::Transformation& widgetToWorld)
	{
	if(constrainMovement)
		{
		/* Return a top-level transformation for the widget's current hot spot: */
		return calcTopLevelTransformInternal(topLevelWidget,widgetToWorld.transform(Point(topLevelWidget->calcHotSpot().getXyzw())));
		}
	else
		{
		/* Return the given transformation unchanged: */
		return widgetToWorld;
		}
	}

Point UIManagerSpherical::projectRay(const Ray& ray) const
	{
	/* Check if the line defined by the ray intersects the sphere: */
	Scalar d2=Geometry::sqr(ray.getDirection());
	Vector oc=ray.getOrigin()-sphere.getCenter();
	Scalar ph=oc*ray.getDirection();
	Scalar det=Math::sqr(ph)-(Geometry::sqr(oc)-Math::sqr(sphere.getRadius()))*d2;
	if(det>=Scalar(0))
		{
		/* Calculate the point where the line exits the sphere: */
		det=Math::sqrt(det);
		return ray((-ph+det)/d2);
		}
	else
		{
		/* Return the projection of the ray's origin onto the sphere: */
		Vector d=ray.getOrigin()-sphere.getCenter();
		Scalar dLen=d.mag();
		if(dLen==Scalar(0))
			{
			d=getForwardDirection();
			dLen=d.mag();
			}
		return sphere.getCenter()+d*(sphere.getRadius()/dLen);
		}
	}

void UIManagerSpherical::projectDevice(InputDevice* device) const
	{
	/* Get the device's ray: */
	Ray ray=device->getRay();
	
	/* Check if the line defined by the device's ray intersects the sphere: */
	Scalar d2=Geometry::sqr(ray.getDirection());
	Vector oc=ray.getOrigin()-sphere.getCenter();
	Scalar ph=(oc*ray.getDirection());
	Scalar det=Math::sqr(ph)-(Geometry::sqr(oc)-Math::sqr(sphere.getRadius()))*d2;
	Scalar lambda(0);
	Point devicePos;
	Vector y;
	if(det>=Scalar(0))
		{
		/* Calculate the point where the line exits the sphere: */
		det=Math::sqrt(det);
		lambda=(-ph+det)/d2; // Second intersection
		devicePos=ray(lambda);
		y=devicePos-sphere.getCenter();
		}
	else
		{
		/* Project the device's position onto the sphere: */
		y=device->getPosition()-sphere.getCenter();
		Scalar yLen=y.mag();
		if(yLen==Scalar(0))
			{
			y=getForwardDirection();
			yLen=y.mag();
			}
		devicePos=sphere.getCenter()+y*(sphere.getRadius()/yLen);
		}
	
	/* Calculate a device orientation such that the y axis is normal to the sphere and points outwards: */
	Vector x=y^getUpDirection();
	if(x.mag()==Scalar(0))
		x=getForwardDirection()^getUpDirection();
	
	/* Set the device transformation: */
	device->setTransformation(TrackerState(devicePos-Point::origin,Rotation::fromBaseVectors(x,y)));
	
	/* Update the device's ray: */
	device->setDeviceRay(device->getTransformation().inverseTransform(ray.getDirection()),-lambda);
	}

ONTransform UIManagerSpherical::calcUITransform(const Point& point) const
	{
	/* Project the given point onto the sphere: */
	Vector d=point-sphere.getCenter();
	Scalar dLen=d.mag();
	if(dLen==Scalar(0))
		{
		d=getForwardDirection();
		dLen=d.mag();
		}
	Point spherePoint=sphere.getCenter()+d*(sphere.getRadius()/dLen);
	
	/* Calculate the UI transformation: */
	Vector x=d^getUpDirection();
	if(x.mag()==Scalar(0))
		x=getForwardDirection()^getUpDirection();
	Vector y=x^d;
	return ONTransform(spherePoint-Point::origin,Rotation::fromBaseVectors(x,y));
	}

ONTransform UIManagerSpherical::calcUITransform(const Ray& ray) const
	{
	throw std::runtime_error("UIManagerSpherical::calcUITransform: Not implemented");
	}

ONTransform UIManagerSpherical::calcUITransform(const InputDevice* device) const
	{
	throw std::runtime_error("UIManagerSpherical::calcUITransform: Not implemented");
	}

}
