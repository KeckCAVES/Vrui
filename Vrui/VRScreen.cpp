/***********************************************************************
VRScreen - Class for display screens (fixed and head-mounted) in VR
environments.
Copyright (c) 2004-2008 Oliver Kreylos

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

#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Ray.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLMatrixTemplates.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/InputDevice.h>
#include <Vrui/Vrui.h>

#include <Vrui/VRScreen.h>

namespace Vrui {

/*************************
Methods of class VRScreen:
*************************/

VRScreen::VRScreen(void)
	:screenName(0),
	 deviceMounted(false),device(0),
	 transform(ONTransform::identity),inverseTransform(ONTransform::identity)
	{
	screenSize[0]=screenSize[1]=Scalar(0);
	}

VRScreen::~VRScreen(void)
	{
	delete[] screenName;
	}

void VRScreen::initialize(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Read the screen's name: */
	std::string name=configFileSection.retrieveString("./name");
	screenName=new char[name.size()+1];
	strcpy(screenName,name.c_str());
	
	/* Determine whether screen is device-mounted: */
	deviceMounted=configFileSection.retrieveValue<bool>("./deviceMounted",false);
	if(deviceMounted)
		{
		/* Retrieve the input device this screen is attached to: */
		std::string deviceName=configFileSection.retrieveString("./deviceName");
		device=findInputDevice(deviceName.c_str());
		if(device==0)
			Misc::throwStdErr("VRScreen: Mounting device \"%s\" not found",deviceName.c_str());
		}
	
	/* Retrieve screen position/orientation in physical or device coordinates: */
	Point origin=configFileSection.retrieveValue<Point>("./origin");
	Vector horizontalAxis=configFileSection.retrieveValue<Vector>("./horizontalAxis");
	screenSize[0]=configFileSection.retrieveValue<Scalar>("./width");
	Vector verticalAxis=configFileSection.retrieveValue<Vector>("./verticalAxis");
	screenSize[1]=configFileSection.retrieveValue<Scalar>("./height");
	ONTransform::Rotation rot=ONTransform::Rotation::fromBaseVectors(horizontalAxis,verticalAxis);
	transform=ONTransform(origin-Point::origin,rot);
	Point rotateCenter=configFileSection.retrieveValue<Point>("./rotateCenter",Point::origin);
	Vector rotateAxis=configFileSection.retrieveValue<Vector>("./rotateAxis",Vector(1,0,0));
	Scalar rotateAngle=configFileSection.retrieveValue<Scalar>("./rotateAngle",Scalar(0));
	if(rotateAngle!=Scalar(0))
		{
		ONTransform screenRotation=ONTransform::translateFromOriginTo(rotateCenter);
		screenRotation*=ONTransform::rotate(ONTransform::Rotation::rotateAxis(rotateAxis,Math::rad(rotateAngle)));
		screenRotation*=ONTransform::translateToOriginFrom(rotateCenter);
		transform.leftMultiply(screenRotation);
		}
	inverseTransform=Geometry::invert(transform);
	}

void VRScreen::attachToDevice(const InputDevice* newDevice)
	{
	/* Set the device to which the screen is mounted, and update the mounted flag: */
	deviceMounted=newDevice!=0;
	device=newDevice;
	}

void VRScreen::setSize(Scalar newWidth,Scalar newHeight)
	{
	/* Adjust the screen's origin in its own coordinate system: */
	transform*=ONTransform::translate(Vector(Math::div2(screenSize[0]-newWidth),Math::div2(screenSize[1]-newHeight),0));
	inverseTransform=Geometry::invert(transform);
	
	/* Adjust the screen's sizes: */
	screenSize[0]=newWidth;
	screenSize[1]=newHeight;
	}

void VRScreen::setTransform(const ONTransform& newTransform)
	{
	/* Update the screen-to-physical/device transformation and its inverse: */
	transform=newTransform;
	inverseTransform=Geometry::invert(transform);
	}

ONTransform VRScreen::getScreenTransformation(void) const
	{
	ONTransform result=transform;
	if(deviceMounted)
		result.leftMultiply(device->getTransformation());
	return result;
	}

void VRScreen::setScreenTransform(void) const
	{
	/* Save the current matrix mode: */
	glPushAttrib(GL_TRANSFORM_BIT);
	
	/* Save the modelview matrix: */
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	/* Modify the modelview matrix: */
	if(deviceMounted)
		glMultMatrix(device->getTransformation());
	glMultMatrix(transform);
	
	/* Restore the current matrix mode: */
	glPopAttrib();
	}

void VRScreen::resetScreenTransform(void) const
	{
	/* Save the current matrix mode: */
	glPushAttrib(GL_TRANSFORM_BIT);
	
	/* Restore the modelview matrix: */
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	
	/* Restore the current matrix mode: */
	glPopAttrib();
	}

}
