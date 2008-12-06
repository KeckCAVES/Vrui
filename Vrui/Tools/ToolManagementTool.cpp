/***********************************************************************
ToolManagementTool - A class to assign arbitrary tools to arbitrary
combinations of input devices / buttons / valuators using a modal dialog
and overriding callbacks.
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
#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLGeometryWrappers.h>
#include <GLMotif/Event.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/Button.h>
#include <GLMotif/WidgetAlgorithms.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/Viewer.h>
#include <Vrui/MutexMenu.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/ToolManagementTool.h>

namespace Vrui {

/******************************************
Methods of class ToolManagementToolFactory:
******************************************/

ToolManagementToolFactory::ToolManagementToolFactory(ToolManager& toolManager)
	:ToolFactory("ToolManagementTool",toolManager),
	 initialMenuOffset(getInchFactor()*6)
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* menuToolFactory=toolManager.loadClass("MenuTool");
	menuToolFactory->addChildClass(this);
	addParentClass(menuToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	initialMenuOffset=cfs.retrieveValue<Scalar>("./initialMenuOffset",initialMenuOffset);
	
	/* Set tool class' factory pointer: */
	ToolManagementTool::factory=this;
	}

ToolManagementToolFactory::~ToolManagementToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	ToolManagementTool::factory=0;
	}

Tool* ToolManagementToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new ToolManagementTool(this,inputAssignment);
	}

void ToolManagementToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveToolManagementToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("MenuTool");
	}

extern "C" ToolFactory* createToolManagementToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	ToolManagementToolFactory* toolManagementToolFactory=new ToolManagementToolFactory(*toolManager);
	
	/* Return factory object: */
	return toolManagementToolFactory;
	}

extern "C" void destroyToolManagementToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/********************************************************
Methods of class ToolManagementTool::SetCallbacksFunctor:
********************************************************/

ToolManagementTool::SetCallbacksFunctor::SetCallbacksFunctor(ToolManagementTool* sTool,bool sRemove)
	:tool(sTool),remove(sRemove)
	{
	}

void ToolManagementTool::SetCallbacksFunctor::operator()(GLMotif::Widget* widget) const
	{
	/* Check if the widget is a menu button: */
	GLMotif::Button* button=dynamic_cast<GLMotif::Button*>(widget);
	if(button!=0)
		{
		if(remove)
			{
			/* Remove the tool's callback from the button: */
			button->getSelectCallbacks().remove(ToolManagementTool::toolMenuSelectionCallbackWrapper,tool);
			}
		else
			{
			/* Add the tool's callback to the button: */
			button->getSelectCallbacks().add(ToolManagementTool::toolMenuSelectionCallbackWrapper,tool);
			}
		}
	}

/*******************************************
Static elements of class ToolManagementTool:
*******************************************/

ToolManagementToolFactory* ToolManagementTool::factory=0;

/***********************************
Methods of class ToolManagementTool:
***********************************/

void ToolManagementTool::toolMenuSelectionCallbackWrapper(Misc::CallbackData* cbData,void* userData)
	{
	ToolManagementTool* thisPtr=static_cast<ToolManagementTool*>(userData);
	GLMotif::Button::SelectCallbackData* cbData2=static_cast<GLMotif::Button::SelectCallbackData*>(cbData);
	thisPtr->toolMenuSelectionCallback(cbData2->button);
	}

void ToolManagementTool::inputDeviceButtonCallbackWrapper(Misc::CallbackData* cbData,void* userData)
	{
	ToolManagementTool* thisPtr=static_cast<ToolManagementTool*>(userData);
	InputDevice::ButtonCallbackData* cbData2=static_cast<InputDevice::ButtonCallbackData*>(cbData);
	thisPtr->inputDeviceButtonCallback(cbData2->buttonIndex,cbData2->newButtonState);
	
	/* Cancel further processing of this callback: */
	cbData->callbackList->requestInterrupt();
	}

Ray ToolManagementTool::calcSelectionRay(void) const
	{
	/* Get pointer to input device: */
	InputDevice* device=input.getDevice(0);
	
	/* Calculate ray equation: */
	Point start=device->getPosition();
	Vector direction=device->getRayDirection();
	return Ray(start,direction);
	}

void ToolManagementTool::finishCreatingTool(void)
	{
	/* Create and assign the tool: */
	// Tool* newTool=getToolManager()->createTool(createToolFactory,*tia);
	printf("Creating tool\n");
	
	/* Clean up: */
	createToolFactory=0;
	delete tia;
	tia=0;
	}

void ToolManagementTool::toolMenuSelectionCallback(GLMotif::Button* button)
	{
	/* Get a tool factory for the tool class selected by the user: */
	createToolFactory=getToolManager()->loadClass(button->getName());
	
	if(createToolFactory!=0)
		{
		printf("Creating tool class %s\n",button->getName());
		printf("%d devices\n",createToolFactory->getLayout().getNumDevices());
		for(int i=0;i<createToolFactory->getLayout().getNumDevices();++i)
			printf("Device %d: %d buttons, %d valuators\n",i,createToolFactory->getLayout().getNumButtons(i),createToolFactory->getLayout().getNumValuators(i));
		
		/* Create the tool input assignment: */
		tia=new ToolInputAssignment(createToolFactory->getLayout());
		
		/* Start probing for devices: */
		probingForDevice=true;
		currentDeviceIndex=0;
		}
	}

void ToolManagementTool::inputDeviceButtonCallback(int buttonIndex,bool newState)
	{
	if(newState) // The button has just been pressed
		{
		if(firstPressedButtonIndex<0)
			firstPressedButtonIndex=buttonIndex;
		}
	else // The button has just been released
		{
		if(buttonIndex==firstPressedButtonIndex)
			{
			/* Save this button index in the tool input assignment: */
			tia->setButtonIndex(currentDeviceIndex,currentButtonIndex,buttonIndex);
			printf("Found button index %d\n",buttonIndex);
			
			/* Go to the next button: */
			++currentButtonIndex;
			firstPressedButtonIndex=-1;
			if(currentButtonIndex==createToolFactory->getLayout().getNumButtons(currentDeviceIndex))
				{
				/* Release all buttons on the probed device: */
				InputDevice* device=tia->getDevice(currentDeviceIndex);
				for(int i=0;i<device->getNumButtons();++i)
					device->getButtonCallbacks(i).remove(inputDeviceButtonCallbackWrapper,this);
				
				/* Stop probing for buttons on this device: */
				probingForButtons=false;
				printf("Finished probing for buttons\n");
				
				/* Probe for the next device: */
				++currentDeviceIndex;
				if(currentDeviceIndex<createToolFactory->getLayout().getNumDevices())
					probingForDevice=true;
				else
					finishCreatingTool();
				}
			}
		}
	}

ToolManagementTool::ToolManagementTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:MenuTool(factory,inputAssignment),
	 viewer(0),
	 displayRay(false),
	 createToolFactory(0),
	 tia(0),
	 probingForDevice(false),
	 currentDeviceIndex(-1),
	 probingForButtons(false),
	 currentButtonIndex(-1),
	 firstPressedButtonIndex(-1)
	{
	/* Retrieve the viewer associated with this menu tool: */
	#if 0
	int viewerIndex=configFile.retrieveValue<int>("./viewerIndex");
	viewer=getViewer(viewerIndex);
	#else
	viewer=getMainViewer();
	#endif
	}

ToolManagementTool::~ToolManagementTool(void)
	{
	/* Remove callbacks from the tool selection menu: */
	/* ... */
	}

const ToolFactory* ToolManagementTool::getFactory(void) const
	{
	return factory;
	}

void ToolManagementTool::buttonCallback(int deviceIndex,int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		if(createToolFactory==0)
			{
			/* Activate the tool selection menu: */
			if(activate())
				{
				typedef GLMotif::WidgetManager::Transformation WTransform;
				typedef WTransform::Point WPoint;
				typedef WTransform::Vector WVector;
				
				/* Calculate the menu transformation: */
				WPoint globalHotSpot=calcSelectionRay()(factory->initialMenuOffset);
				
				/* Align the widget with the viewing direction: */
				WVector viewDirection=globalHotSpot-viewer->getHeadPosition();
				WVector x=Geometry::cross(viewDirection,getUpDirection());
				WVector y=Geometry::cross(x,viewDirection);
				WTransform menuTransformation=WTransform::translateFromOriginTo(globalHotSpot);
				WTransform::Rotation rot=WTransform::Rotation::fromBaseVectors(x,y);
				menuTransformation*=WTransform::rotate(rot);
				menuTransformation*=WTransform::scale(getInchFactor());
				GLMotif::Vector menuHotSpot=menu->getPopup()->calcHotSpot();
				menuTransformation*=WTransform::translate(-WVector(menuHotSpot.getXyzw()));
				
				/* Pop up the menu: */
				getWidgetManager()->popupPrimaryWidget(menu->getPopup(),menuTransformation);
				
				/* Deliver the event: */
				GLMotif::Event event(false);
				event.setWorldLocation(calcSelectionRay());
				getWidgetManager()->pointerButtonDown(event);
				
				displayRay=true;
				}
			}
		else if(probingForDevice)
			{
			/* Find the device currently pointed at: */
			displayRay=true;
			}
		}
	else // Button has just been released
		{
		if(createToolFactory==0)
			{
			if(isActive())
				{
				/* Deliver the event: */
				GLMotif::Event event(true);
				event.setWorldLocation(calcSelectionRay());
				getWidgetManager()->pointerButtonUp(event);
				
				/* Pop down the menu: */
				getWidgetManager()->popdownWidget(menu->getPopup());
				
				/* Deactivate the tool: */
				deactivate();
				
				displayRay=false;
				}
			}
		else if(probingForDevice)
			{
			/* Find the device current pointed at: */
			InputDevice* device=getInputGraphManager()->findInputDevice(calcSelectionRay(),false);
			printf("Found device %s\n",device->getDeviceName());
			
			/* Store the device in the tool input assignment: */
			tia->setDevice(currentDeviceIndex,device);
			probingForDevice=false;
			
			if(createToolFactory->getLayout().getNumButtons(currentDeviceIndex)>0)
				{
				/* Start probing for buttons on the device: */
				probingForButtons=true;
				currentButtonIndex=0;
				firstPressedButtonIndex=-1;
				printf("Starting to probe for buttons\n");
				
				/* Hijack all buttons on the device: */
				for(int i=0;i<device->getNumButtons();++i)
					device->getButtonCallbacks(i).addToFront(inputDeviceButtonCallbackWrapper,this);
				}
			else
				{
				/* Probe for the next device: */
				++deviceIndex;
				if(deviceIndex<createToolFactory->getLayout().getNumDevices())
					probingForDevice=true;
				else
					finishCreatingTool();
				}
			
			displayRay=false;
			}
		}
	}

void ToolManagementTool::frame(void)
	{
	if(displayRay)
		{
		/* Update the selection ray: */
		selectionRay=calcSelectionRay();
		
		/* Deliver the event: */
		GLMotif::Event event(true);
		event.setWorldLocation(selectionRay);
		getWidgetManager()->pointerMotion(event);
		}
	}

void ToolManagementTool::display(GLContextData&) const
	{
	if(displayRay)
		{
		/* Draw the selection ray: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glColor3f(1.0f,0.0f,0.0f);
		glLineWidth(3.0f);
		glBegin(GL_LINES);
		glVertex(selectionRay.getOrigin());
		glVertex(selectionRay(factory->initialMenuOffset*10));
		glEnd();
		glPopAttrib();
		}
	}

void ToolManagementTool::setMenu(MutexMenu*)
	{
	/* Ignore the menu given to us, and get the tool manager's tool selection menu instead: */
	MutexMenu* toolSelectionMenu=getToolManager()->getToolMenu();
	
	/* Call the base class method: */
	MenuTool::setMenu(toolSelectionMenu);
	
	/* Associate callbacks with the tool selection menu: */
	traverseWidgetTree(toolSelectionMenu->getPopup(),SetCallbacksFunctor(this,false));
	}

}
