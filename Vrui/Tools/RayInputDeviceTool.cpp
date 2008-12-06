/***********************************************************************
RayInputDeviceTool - Class for tools using a ray to interact with
virtual input devices.
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

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/CallbackList.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <GL/gl.h>
#include <GL/GLGeometryWrappers.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/InputDevice.h>
#include <Vrui/Viewer.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/RayInputDeviceTool.h>

namespace Vrui {

/******************************************
Methods of class RayInputDeviceToolFactory:
******************************************/

RayInputDeviceToolFactory::RayInputDeviceToolFactory(ToolManager& toolManager)
	:ToolFactory("RayInputDeviceTool",toolManager),
	 rotateFactor(getInchFactor()*12)
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* inputDeviceToolFactory=toolManager.loadClass("InputDeviceTool");
	inputDeviceToolFactory->addChildClass(this);
	addParentClass(inputDeviceToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	rotateFactor=cfs.retrieveValue<Scalar>("./rotateFactor",rotateFactor);
	
	/* Set tool class' factory pointer: */
	RayInputDeviceTool::factory=this;
	}

RayInputDeviceToolFactory::~RayInputDeviceToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	RayInputDeviceTool::factory=0;
	}

Tool* RayInputDeviceToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new RayInputDeviceTool(this,inputAssignment);
	}

void RayInputDeviceToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveRayInputDeviceToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("InputDeviceTool");
	}

extern "C" ToolFactory* createRayInputDeviceToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	RayInputDeviceToolFactory* rayInputDeviceToolFactory=new RayInputDeviceToolFactory(*toolManager);
	
	/* Return factory object: */
	return rayInputDeviceToolFactory;
	}

extern "C" void destroyRayInputDeviceToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/*******************************************
Static elements of class RayInputDeviceTool:
*******************************************/

RayInputDeviceToolFactory* RayInputDeviceTool::factory=0;

/***********************************
Methods of class RayInputDeviceTool:
***********************************/

Ray RayInputDeviceTool::calcInteractionRay(void) const
	{
	/* Get pointer to input device: */
	InputDevice* device=input.getDevice(0);
	
	/* Calculate ray equation: */
	Point start=device->getPosition();
	Vector direction=device->getRayDirection();
	return Ray(start,direction);
	}

RayInputDeviceTool::RayInputDeviceTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:InputDeviceTool(sFactory,inputAssignment),
	 viewer(0),
	 dragger(getGlyphRenderer()->getGlyphSize(),factory->rotateFactor)
	{
	/* Retrieve the viewer associated with this tool: */
	#if 0
	int viewerIndex=configFile.retrieveValue<int>("./viewerIndex");
	viewer=getViewer(viewerIndex);
	#else
	viewer=getMainViewer();
	#endif
	}

const ToolFactory* RayInputDeviceTool::getFactory(void) const
	{
	return factory;
	}

void RayInputDeviceTool::buttonCallback(int,int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Calculate the current selection ray: */
		Ray ray=calcInteractionRay();
		
		/* Try activating the tool: */
		if(activate(ray))
			{
			/* Pick the input device with the box ray dragger: */
			if(!dragger.pick(getGrabbedDevice()->getTransformation(),ray,-viewer->getViewDirection()))
				{
				/* Deactivate the tool again (it was a close miss): */
				deactivate();
				}
			else
				{
				/* Cancel processing of this callback to preempt cascaded tools: */
				cbData->callbackList->requestInterrupt();
				}
			}
		}
	else // Button has just been released
		{
		if(isActive())
			{
			/* Release the box dragger: */
			dragger.release();
			
			/* Deactivate the tool: */
			deactivate();
			
			/* Cancel processing of this callback to preempt cascaded tools: */
			cbData->callbackList->requestInterrupt();
			}
		}
	}

void RayInputDeviceTool::frame(void)
	{
	if(isActive())
		{
		/* Update the interaction ray: */
		interactionRay=calcInteractionRay();
		
		/* Drag the box dragger: */
		dragger.drag(interactionRay);
		
		/* Set the grabbed device's position and orientation: */
		getGrabbedDevice()->setTransformation(dragger.getCurrentTransformation());
		}
	}

void RayInputDeviceTool::display(GLContextData&) const
	{
	if(isActive())
		{
		/* Draw the interaction ray: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glColor3f(1.0f,0.0f,0.0f);
		glLineWidth(3.0f);
		glBegin(GL_LINES);
		glVertex(interactionRay.getOrigin());
		glVertex(interactionRay(getDisplaySize()));
		glEnd();
		glPopAttrib();
		}
	}

}
