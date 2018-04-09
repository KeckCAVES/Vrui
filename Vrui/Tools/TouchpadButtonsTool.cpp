/***********************************************************************
TouchpadButtonsTool - Transform a clickable touchpad or analog stick to
multiple buttons arranged around a circle.
Copyright (c) 2016-2018 Oliver Kreylos

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

#include <Vrui/Tools/TouchpadButtonsTool.h>

#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLValueCoders.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/**********************************************************
Methods of class TouchpadButtonsToolFactory::Configuration:
**********************************************************/

TouchpadButtonsToolFactory::Configuration::Configuration(void)
	:numButtons(4),centerRadius(0.5),useCenterButton(false),
	 drawOnTouch(false),touchpadRadius(1),touchpadColor(0.0f,1.0f,0.0f,1.0f)
	{
	}

void TouchpadButtonsToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	numButtons=cfs.retrieveValue<int>("./numButtons",numButtons);
	centerRadius=cfs.retrieveValue<double>("./centerRadius",centerRadius);
	useCenterButton=cfs.retrieveValue<bool>("./useCenterButton",useCenterButton);
	drawOnTouch=cfs.retrieveValue<bool>("./drawOnTouch",drawOnTouch);
	if(drawOnTouch)
		{
		touchpadTransform=cfs.retrieveValue<ONTransform>("./touchpadTransform",touchpadTransform);
		touchpadRadius=cfs.retrieveValue<Scalar>("./touchpadRadius",touchpadRadius);
		touchpadColor=cfs.retrieveValue<GLColor<GLfloat,4> >("./touchpadColor",touchpadColor);
		}
	}

void TouchpadButtonsToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue<int>("./numButtons",numButtons);
	cfs.storeValue<double>("./centerRadius",centerRadius);
	cfs.storeValue<bool>("./useCenterButton",useCenterButton);
	cfs.storeValue<bool>("./drawOnTouch",drawOnTouch);
	if(drawOnTouch)
		{
		cfs.storeValue<ONTransform>("./touchpadTransform",touchpadTransform);
		cfs.storeValue<Scalar>("./touchpadRadius",touchpadRadius);
		cfs.storeValue<GLColor<GLfloat,4> >("./touchpadColor",touchpadColor);
		}
	}

/*******************************************
Methods of class TouchpadButtonsToolFactory:
*******************************************/

TouchpadButtonsToolFactory::TouchpadButtonsToolFactory(ToolManager& toolManager)
	:ToolFactory("TouchpadButtonsTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1,true);
	layout.setNumValuators(2);
	
	/* Insert class into class hierarchy: */
	TransformToolFactory* transformToolFactory=dynamic_cast<TransformToolFactory*>(toolManager.loadClass("TransformTool"));
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	TouchpadButtonsTool::factory=this;
	}

TouchpadButtonsToolFactory::~TouchpadButtonsToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	TouchpadButtonsTool::factory=0;
	}

const char* TouchpadButtonsToolFactory::getName(void) const
	{
	return "Touchpad -> Buttons";
	}

const char* TouchpadButtonsToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	if(buttonSlotIndex==0)
		return "Press Button";
	else
		return "Draw Touchpad";
	}

const char* TouchpadButtonsToolFactory::getValuatorFunction(int valuatorSlotIndex) const
	{
	static const char* valuatorNames[2]=
		{
		"Touchpad X Axis","Touchpad Y Axis"
		};
	return valuatorNames[valuatorSlotIndex];
	}

Tool* TouchpadButtonsToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new TouchpadButtonsTool(this,inputAssignment);
	}

void TouchpadButtonsToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveTouchpadButtonsToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createTouchpadButtonsToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	TouchpadButtonsToolFactory* touchpadButtonsToolFactory=new TouchpadButtonsToolFactory(*toolManager);
	
	/* Return factory object: */
	return touchpadButtonsToolFactory;
	}

extern "C" void destroyTouchpadButtonsToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/**********************************************
Methods of class TouchpadButtonsTool::DataItem:
**********************************************/

TouchpadButtonsTool::DataItem::DataItem(GLuint sNumButtons)
	:numButtons(sNumButtons),
	 displayListBase(0)
	{
	/* Create the touchpad part display lists: */
	displayListBase=glGenLists(2+numButtons);
	}

TouchpadButtonsTool::DataItem::~DataItem(void)
	{
	/* Delete the display lists: */
	glDeleteLists(displayListBase,2+numButtons);
	}

/********************************************
Static elements of class TouchpadButtonsTool:
********************************************/

TouchpadButtonsToolFactory* TouchpadButtonsTool::factory=0;

/************************************
Methods of class TouchpadButtonsTool:
************************************/

int TouchpadButtonsTool::calcButtonIndex(void) const
	{
	int result=-1;
	
	/* Determine which button to press: */
	double x=getValuatorState(0);
	double y=getValuatorState(1);
	double r2=x*x+y*y;
	if(r2>=Math::sqr(configuration.centerRadius))
		{
		/* Calculate the angle of the touchpad contact: */
		double angle=Math::atan2(-x,y);
		if(angle<0.0)
			angle+=2.0*Math::Constants<double>::pi;
		double anglePerButton=2.0*Math::Constants<double>::pi/double(configuration.numButtons);
		
		/* Find the touched perimeter button: */
		result=int(Math::floor(angle/anglePerButton+0.5))%configuration.numButtons;
		}
	else if(configuration.useCenterButton)
		{
		/* Return the center button: */
		result=configuration.numButtons;
		}
	
	return result;
	}

TouchpadButtonsTool::TouchpadButtonsTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:TransformTool(factory,inputAssignment),
	 configuration(TouchpadButtonsTool::factory->configuration),
	 drawTouchpad(false),pressedButton(-1)
	{
	}

TouchpadButtonsTool::~TouchpadButtonsTool(void)
	{
	}

void TouchpadButtonsTool::initialize(void)
	{
	/* Create a virtual input device to shadow the source input device: */
	int numButtons=configuration.numButtons;
	if(configuration.useCenterButton)
		++numButtons;
	transformedDevice=addVirtualInputDevice("TouchpadButtonsToolTransformedDevice",numButtons,0);
	
	/* Copy the source device's tracking type: */
	transformedDevice->setTrackType(sourceDevice->getTrackType());
	
	/* Disable the virtual input device's glyph: */
	getInputGraphManager()->getInputDeviceGlyph(transformedDevice).disable();
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(transformedDevice,this);
	
	/* Initialize the virtual input device's position: */
	resetDevice();
	}

const ToolFactory* TouchpadButtonsTool::getFactory(void) const
	{
	return factory;
	}

void TouchpadButtonsTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	/* Check which button was pressed or released: */
	if(buttonSlotIndex==0)
		{
		if(cbData->newButtonState)
			{
			/* Find the selected button: */
			pressedButton=calcButtonIndex();
			if(pressedButton>=0)
				transformedDevice->setButtonState(pressedButton,true);
			}
		else if(pressedButton>=0)
			{
			/* Release the currently pressed button: */
			transformedDevice->setButtonState(pressedButton,false);
			pressedButton=-1;
			}
		}
	else if(configuration.drawOnTouch)
		{
		/* Remember if the touchpad is supposed to be drawn: */
		drawTouchpad=cbData->newButtonState;
		}
	}

void TouchpadButtonsTool::display(GLContextData& contextData) const
	{
	/* Bail out if the touchpad is not supposed to be drawn: */
	if(!drawTouchpad)
		return;
	
	/* Get the data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(1.0f);
	
	/* Go to the touchpad's coordinate system: */
	glPushMatrix();
	glMultMatrix(getButtonDeviceTransformation(0)*configuration.touchpadTransform);
	
	/* Draw the touchpad: */
	glCallList(dataItem->displayListBase+0);
	
	/* Check if a button is touched: */
	int touchedButton=calcButtonIndex();
	if(touchedButton>=0)
		{
		/* Highlight the touched button: */
		glCallList(dataItem->displayListBase+2+touchedButton);
		}
	
	/* Draw the current finger position: */
	glTranslate(Scalar(getValuatorState(0))*configuration.touchpadRadius,Scalar(getValuatorState(1))*configuration.touchpadRadius,Scalar(0));
	glCallList(dataItem->displayListBase+1);
	
	/* Return to physical coordinates: */
	glPopMatrix();
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

InputDeviceFeatureSet TouchpadButtonsTool::getSourceFeatures(const InputDeviceFeature& forwardedFeature)
	{
	/* Paranoia: Check if the forwarded feature is on the transformed device: */
	if(forwardedFeature.getDevice()!=transformedDevice)
		Misc::throwStdErr("TouchpadButtonsTool::getSourceFeatures: Forwarded feature is not on transformed device");
	
	/* Return all input feature slots: */
	InputDeviceFeatureSet result;
	result.push_back(input.getButtonSlotFeature(0));
	for(int i=0;i<2;++i)
		result.push_back(input.getValuatorSlotFeature(i));
	
	return result;
	}

InputDeviceFeatureSet TouchpadButtonsTool::getForwardedFeatures(const InputDeviceFeature& sourceFeature)
	{
	/* Find the input assignment slot for the given feature: */
	int slotIndex=input.findFeature(sourceFeature);
	
	/* Check if the source feature belongs to this tool: */
	if(slotIndex<0)
		Misc::throwStdErr("TouchpadButtonsTool::getForwardedFeatures: Source feature is not part of tool's input assignment");
	
	/* Return the currently touched button: */
	InputDeviceFeatureSet result;
	int touchedButtonIndex=calcButtonIndex();
	if(touchedButtonIndex>=0)
		result.push_back(InputDeviceFeature(transformedDevice,InputDevice::BUTTON,touchedButtonIndex));
	
	return result;
	}

void TouchpadButtonsTool::initContext(GLContextData& contextData) const
	{
	/* Create the OpenGL context data item: */
	GLuint numButtons=configuration.numButtons;
	if(configuration.useCenterButton)
		++numButtons;
	DataItem* dataItem=new DataItem(numButtons);
	contextData.addDataItem(this,dataItem);
	
	/* Create the touchpad display list: */
	glNewList(dataItem->displayListBase+0,GL_COMPILE);
	
	/* Ensure that number of vertices around the circle is a multiple of number of buttons: */
	int numVertices=32;
	numVertices=numVertices+configuration.numButtons-numVertices%configuration.numButtons;
	Scalar a0=-Math::Constants<Scalar>::pi/Scalar(configuration.numButtons);
	Scalar r0=configuration.touchpadRadius;
	Scalar r1=r0*configuration.centerRadius;
	
	/* Draw the touchpad perimeter: */
	glColor(configuration.touchpadColor);
	glBegin(GL_LINE_LOOP);
	for(int i=0;i<numVertices;++i)
		{
		Scalar angle=Scalar(i)*Scalar(2)*Math::Constants<Scalar>::pi/Scalar(numVertices)+a0;
		glVertex(-Math::sin(angle)*r0,Math::cos(angle)*r0,Scalar(0));
		}
	glEnd();
	
	/* Draw the center button perimeter: */
	glBegin(GL_LINE_LOOP);
	for(int i=0;i<numVertices;++i)
		{
		Scalar angle=Scalar(i)*Scalar(2)*Math::Constants<Scalar>::pi/Scalar(numVertices)+a0;
		glVertex(-Math::sin(angle)*r1,Math::cos(angle)*r1,Scalar(0));
		}
	glEnd();
	
	/* Draw the button separators: */
	glBegin(GL_LINES);
	for(int i=0;i<configuration.numButtons;++i)
		{
		Scalar angle=Scalar(i)*Scalar(2)*Math::Constants<Scalar>::pi/Scalar(configuration.numButtons)+a0;
		glVertex(-Math::sin(angle)*r1,Math::cos(angle)*r1,Scalar(0));
		glVertex(-Math::sin(angle)*r0,Math::cos(angle)*r0,Scalar(0));
		}
	glEnd();
	
	glEndList();
	
	/* Create the finger indicator display list: */
	glNewList(dataItem->displayListBase+1,GL_COMPILE);
	
	Scalar fr=configuration.touchpadRadius*Scalar(0.1);
	glColor(configuration.touchpadColor);
	glBegin(GL_LINE_LOOP);
	for(int i=0;i<12;++i)
		{
		Scalar angle=Scalar(i)*Scalar(2)*Math::Constants<Scalar>::pi/Scalar(12);
		glVertex(-Math::sin(angle)*fr,Math::cos(angle)*fr,fr*Scalar(0.5));
		}
	glEnd();
	
	glEndList();
	
	/* Create display lists for the perimeter buttons: */
	GLColor<GLfloat,4> buttonColor;
	GLfloat avg=(configuration.touchpadColor[0]+configuration.touchpadColor[1]+configuration.touchpadColor[2])/3.0f;
	for(int i=0;i<3;++i)
		buttonColor[i]=(configuration.touchpadColor[i]+avg*2.0f)/3.0f;
	buttonColor[3]=1.0f;
	for(int buttonIndex=0;buttonIndex<configuration.numButtons;++buttonIndex)
		{
		glNewList(dataItem->displayListBase+2+buttonIndex,GL_COMPILE);
		
		glColor(buttonColor);
		glBegin(GL_QUAD_STRIP);
		int i0=(buttonIndex*numVertices)/configuration.numButtons;
		int i1=((buttonIndex+1)*numVertices)/configuration.numButtons;
		for(int i=i0;i<=i1;++i)
			{
			Scalar angle=Scalar(i)*Scalar(2)*Math::Constants<Scalar>::pi/Scalar(numVertices)+a0;
			glVertex(-Math::sin(angle)*r1,Math::cos(angle)*r1,Scalar(0));
			glVertex(-Math::sin(angle)*r0,Math::cos(angle)*r0,Scalar(0));
			}
		glEnd();
		
		glEndList();
		}
	
	if(configuration.useCenterButton)
		{
		/* Create a display list for the center button: */
		glNewList(dataItem->displayListBase+2+configuration.numButtons,GL_COMPILE);
		
		glColor(buttonColor);
		glBegin(GL_POLYGON);
		for(int i=0;i<numVertices;++i)
			{
			Scalar angle=Scalar(i)*Scalar(2)*Math::Constants<Scalar>::pi/Scalar(numVertices)+a0;
			glVertex(-Math::sin(angle)*r1,Math::cos(angle)*r1,Scalar(0));
			}
		glEnd();
		
		glEndList();
		}
	}

}
