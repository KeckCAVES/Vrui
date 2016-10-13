/***********************************************************************
UIManagerFree - UI manager class that allows arbitrary positions and
orientations for UI components.
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

#include <Vrui/Internal/UIManagerFree.h>

#include <stdexcept>
#include <Geometry/OrthogonalTransformation.h>
#include <GLMotif/Widget.h>
#include <Vrui/Vrui.h>
#include <Vrui/Viewer.h>

namespace Vrui {

/******************************
Methods of class UIManagerFree:
******************************/

UIManagerFree::UIManagerFree(const Misc::ConfigurationFileSection& configFileSection)
	:UIManager(configFileSection)
	{
	}

GLMotif::WidgetArranger::Transformation UIManagerFree::calcTopLevelTransform(GLMotif::Widget* topLevelWidget)
	{
	/* Get a position for the new widget's hot spot: */
	Point hotSpot=getHotSpot();
	
	/* Align the widget with the viewing direction: */
	Vector viewDirection=hotSpot-getMainViewer()->getHeadPosition();
	Vector x=viewDirection^getUpDirection();
	Vector y=x^viewDirection;
	
	/* Calculate the widget transformation: */
	Transformation result=Transformation::translateFromOriginTo(hotSpot);
	result*=Transformation::rotate(Transformation::Rotation::fromBaseVectors(x,y));
	
	/* Align the widget's hot spot with the given hot spot: */
	GLMotif::Vector widgetHotSpot=topLevelWidget->calcHotSpot();
	result*=Transformation::translate(-Transformation::Vector(widgetHotSpot.getXyzw()));
	
	result.renormalize();
	return result;
	}

GLMotif::WidgetArranger::Transformation UIManagerFree::calcTopLevelTransform(GLMotif::Widget* topLevelWidget,const GLMotif::Point& hotSpot)
	{
	/* Align the widget with the viewing direction: */
	Vector viewDirection=hotSpot-getMainViewer()->getHeadPosition();
	Vector x=viewDirection^getUpDirection();
	Vector y=x^viewDirection;
	
	/* Calculate the widget transformation: */
	Transformation result=Transformation::translateFromOriginTo(hotSpot);
	result*=Transformation::rotate(Transformation::Rotation::fromBaseVectors(x,y));
	
	/* Align the widget's hot spot with the given hot spot: */
	GLMotif::Vector widgetHotSpot=topLevelWidget->calcHotSpot();
	result*=Transformation::translate(-Transformation::Vector(widgetHotSpot.getXyzw()));
	
	result.renormalize();
	return result;
	}

GLMotif::WidgetArranger::Transformation UIManagerFree::calcTopLevelTransform(GLMotif::Widget* topLevelWidget,const GLMotif::WidgetArranger::Transformation& widgetToWorld)
	{
	/* Return the transformation unchanged: */
	return widgetToWorld;
	}

Point UIManagerFree::projectRay(const Ray& ray) const
	{
	/* Return the ray's origin: */
	return ray.getOrigin();
	}

void UIManagerFree::projectDevice(InputDevice* device) const
	{
	/* Do nothing... */
	}

ONTransform UIManagerFree::calcUITransform(const Point& point) const
	{
	ONTransform result;
	
	/* Align the transformation with the viewing direction: */
	Vector viewDirection=point-getMainViewer()->getHeadPosition();
	Vector x=viewDirection^getUpDirection();
	Vector y=x^viewDirection;
	result=ONTransform::translateFromOriginTo(point);
	result*=ONTransform::rotate(Rotation::fromBaseVectors(x,y));
	result.renormalize();
	
	return result;
	}

ONTransform UIManagerFree::calcUITransform(const Ray& ray) const
	{
	throw std::runtime_error("UIManagerFree::calcUITransform: Not implemented");
	}

ONTransform UIManagerFree::calcUITransform(const InputDevice* device) const
	{
	throw std::runtime_error("UIManagerFree::calcUITransform: Not implemented");
	}

}
