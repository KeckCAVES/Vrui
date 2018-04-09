/***********************************************************************
Viewer - Class for viewers/observers in VR environments.
Copyright (c) 2004-2018 Oliver Kreylos

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

#include <Vrui/Viewer.h>

#include <string.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLLightTemplates.h>
#include <GL/GLLight.h>
#include <GL/GLValueCoders.h>
#include <Vrui/Vrui.h>
#include <Vrui/Lightsource.h>
#include <Vrui/LightsourceManager.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/Internal/InputDeviceAdapter.h>

namespace Vrui {

/***********************
Methods of class Viewer:
***********************/

void Viewer::inputDeviceStateChangeCallback(InputGraphManager::InputDeviceStateChangeCallbackData* cbData)
	{
	/* Set viewer state if this is our head tracking device: */
	if(headTracked&&cbData->inputDevice==headDevice)
		enabled=cbData->newEnabled;
	}

Viewer::Viewer(void)
	:viewerName(0),
	 headTracked(false),headDevice(0),headDeviceAdapter(0),headDeviceIndex(-1),
	 headDeviceTransformation(TrackerState::identity),
	 deviceViewDirection(0,1,0),deviceUpDirection(0,0,1),
	 deviceMonoEyePosition(Point::origin),
	 deviceLeftEyePosition(Point::origin),
	 deviceRightEyePosition(Point::origin),
	 lightsource(0),
	 headLightDevicePosition(Point::origin),
	 headLightDeviceDirection(0,1,0),
	 enabled(true)
	{
	/* Create the viewer's light source: */
	lightsource=getLightsourceManager()->createLightsource(true);
	
	/* Disable the light source by default: */
	lightsource->disable();
	
	/* Register callbacks with the input graph manager: */
	getInputGraphManager()->getInputDeviceStateChangeCallbacks().add(this,&Viewer::inputDeviceStateChangeCallback);
	}

Viewer::~Viewer(void)
	{
	delete[] viewerName;
	if(lightsource!=0)
		getLightsourceManager()->destroyLightsource(lightsource);
	
	/* Unregister callbacks with the input graph manager: */
	getInputGraphManager()->getInputDeviceStateChangeCallbacks().remove(this,&Viewer::inputDeviceStateChangeCallback);
	}

void Viewer::initialize(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Read the viewer's name: */
	std::string name=configFileSection.retrieveString("./name",configFileSection.getName());
	viewerName=new char[name.size()+1];
	strcpy(viewerName,name.c_str());
	
	/* Determine whether the viewer is head-tracked: */
	headTracked=configFileSection.retrieveValue<bool>("./headTracked",headTracked);
	if(headTracked)
		{
		/* Retrieve head tracking device pointer: */
		headDevice=findInputDevice(configFileSection.retrieveString("./headDevice").c_str());
		if(headDevice==0)
			Misc::throwStdErr("Viewer: Head device \"%s\" not found",configFileSection.retrieveString("./headDevice").c_str());
		attachToDevice(headDevice);
		}
	else
		{
		/* Retrieve fixed head position/orientation: */
		headDeviceTransformation=configFileSection.retrieveValue<TrackerState>("./headDeviceTransformation");
		}
	
	/* Get view direction and eye positions in head device coordinates: */
	deviceViewDirection=configFileSection.retrieveValue<Vector>("./viewDirection",deviceViewDirection);
	deviceUpDirection=configFileSection.retrieveValue<Vector>("./upDirection",deviceUpDirection);
	deviceMonoEyePosition=configFileSection.retrieveValue<Point>("./monoEyePosition",deviceMonoEyePosition);
	deviceLeftEyePosition=configFileSection.retrieveValue<Point>("./leftEyePosition",deviceLeftEyePosition);
	deviceRightEyePosition=configFileSection.retrieveValue<Point>("./rightEyePosition",deviceRightEyePosition);
	
	/* Get head light enable flag: */
	if(configFileSection.retrieveValue<bool>("./headLightEnabled",true))
		lightsource->enable();
	
	/* Get head light position and direction in head device coordinates: */
	headLightDevicePosition=configFileSection.retrieveValue<Point>("./headLightPosition",deviceMonoEyePosition);
	headLightDeviceDirection=configFileSection.retrieveValue<Vector>("./headLightDirection",deviceViewDirection);
	
	/* Retrieve head light settings: */
	GLLight::Color headLightColor=configFileSection.retrieveValue<GLLight::Color>("./headLightColor",GLLight::Color(1.0f,1.0f,1.0f));
	lightsource->getLight().diffuse=headLightColor;
	lightsource->getLight().specular=headLightColor;
	lightsource->getLight().spotCutoff=configFileSection.retrieveValue<GLfloat>("./headLightSpotCutoff",180.0f);
	lightsource->getLight().spotExponent=configFileSection.retrieveValue<GLfloat>("./headLightSpotExponent",0.0f);
	
	/* Initialize head light source if head tracking is disabled: */
	if(!headTracked)
		{
		Point hlp=headDeviceTransformation.transform(headLightDevicePosition);
		lightsource->getLight().position=GLLight::Position(GLfloat(hlp[0]),GLfloat(hlp[1]),GLfloat(hlp[2]),1.0f);
		Vector hld=headDeviceTransformation.transform(headLightDeviceDirection);
		hld.normalize();
		lightsource->getLight().spotDirection=GLLight::SpotDirection(GLfloat(hld[0]),GLfloat(hld[1]),GLfloat(hld[2]));
		}
	}

void Viewer::attachToDevice(InputDevice* newHeadDevice)
	{
	/* Do nothing if new head device is invalid: */
	if(newHeadDevice!=0)
		{
		/* Set the new head device and update the head tracked flag: */
		headTracked=true;
		headDevice=newHeadDevice;
		
		/* Get the head device's adapter and device index: */
		headDeviceAdapter=getInputDeviceManager()->findInputDeviceAdapter(headDevice);
		headDeviceIndex=headDeviceAdapter->findInputDevice(headDevice);
		
		/* Check if the head device is currently enabled: */
		enabled=getInputGraphManager()->isEnabled(headDevice);
		}
	}

void Viewer::detachFromDevice(const TrackerState& newHeadDeviceTransformation)
	{
	/* Disable head tracking and set the static head device transformation: */
	headTracked=false;
	headDeviceTransformation=newHeadDeviceTransformation;
	headDeviceAdapter=0;
	headDeviceIndex=-1;
	
	/* Update head light source state: */
	Point hlp=headDeviceTransformation.transform(headLightDevicePosition);
	lightsource->getLight().position=GLLight::Position(GLfloat(hlp[0]),GLfloat(hlp[1]),GLfloat(hlp[2]),1.0f);
	Vector hld=headDeviceTransformation.transform(headLightDeviceDirection);
	hld.normalize();
	lightsource->getLight().spotDirection=GLLight::SpotDirection(GLfloat(hld[0]),GLfloat(hld[1]),GLfloat(hld[2]));
	
	/* Enable the viewer: */
	enabled=true;
	}

void Viewer::setIPD(Scalar newIPD)
	{
	/* Calculate the new eye displacement vector: */
	Vector newEyeOffset=deviceRightEyePosition-deviceLeftEyePosition;
	newEyeOffset=newEyeOffset*(newIPD*Scalar(0.5)/newEyeOffset.mag());
	
	/* Set the left and right eye positions: */
	deviceLeftEyePosition=deviceMonoEyePosition-newEyeOffset;
	deviceRightEyePosition=deviceMonoEyePosition+newEyeOffset;
	}

void Viewer::setEyes(const Vector& newViewDirection,const Point& newMonoEyePosition,const Vector& newEyeOffset)
	{
	/* Set the view direction: */
	deviceViewDirection=newViewDirection;
	
	/* Set the mono eye position: */
	deviceMonoEyePosition=newMonoEyePosition;
	
	/* Set the left and right eye positions: */
	deviceLeftEyePosition=deviceMonoEyePosition-newEyeOffset;
	deviceRightEyePosition=deviceMonoEyePosition+newEyeOffset;
	}

void Viewer::setHeadlightState(bool newHeadlightState)
	{
	if(newHeadlightState)
		lightsource->enable();
	else
		lightsource->disable();
	}

void Viewer::update(void)
	{
	/* Update head light source state if head tracking is enabled: */
	if(headTracked)
		{
		/* Update head light source state: */
		const TrackerState& headTransform=headTracked?headDevice->getTransformation():headDeviceTransformation;
		Point hlp=headTransform.transform(headLightDevicePosition);
		lightsource->getLight().position=GLLight::Position(GLfloat(hlp[0]),GLfloat(hlp[1]),GLfloat(hlp[2]),1.0f);
		Vector hld=headTransform.transform(headLightDeviceDirection);
		hld.normalize();
		lightsource->getLight().spotDirection=GLLight::SpotDirection(GLfloat(hld[0]),GLfloat(hld[1]),GLfloat(hld[2]));
		}
	}

TrackerState Viewer::peekHeadTransformation(void)
	{
	if(headTracked)
		{
		/* Return up-to-date tracking data from the input device adapter: */
		return headDeviceAdapter->peekTrackerState(headDeviceIndex);
		}
	else
		{
		/* Return fixed head transformation: */
		return headDeviceTransformation;
		}
	}

}
