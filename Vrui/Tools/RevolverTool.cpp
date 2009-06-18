/***********************************************************************
RevolverTool - Class to control multiple buttons (and tools) from a
single button using a revolver metaphor. Generalized from the rotator
tool initially developed by Braden Pellett and Jordan van Aalsburg.
Copyright (c) 2008-2009 Oliver Kreylos

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

#include <stdio.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/RevolverTool.h>

namespace Vrui {

/************************************
Methods of class RevolverToolFactory:
************************************/

RevolverToolFactory::RevolverToolFactory(ToolManager& toolManager)
	:ToolFactory("RevolverTool",toolManager),
	 numButtons(6)
	{
	/* Insert class into class hierarchy: */
	TransformToolFactory* transformToolFactory=dynamic_cast<TransformToolFactory*>(toolManager.loadClass("TransformTool"));
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	numButtons=cfs.retrieveValue<int>("./numButtons",numButtons);
	
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,2);
	layout.setNumValuators(0,0);
	
	/* Set tool class' factory pointer: */
	RevolverTool::factory=this;
	}

RevolverToolFactory::~RevolverToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	RevolverTool::factory=0;
	}

const char* RevolverToolFactory::getName(void) const
	{
	return "Revolver Multi-Button";
	}

Tool* RevolverToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new RevolverTool(this,inputAssignment);
	}

void RevolverToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveRevolverToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createRevolverToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	RevolverToolFactory* revolverToolFactory=new RevolverToolFactory(*toolManager);
	
	/* Return factory object: */
	return revolverToolFactory;
	}

extern "C" void destroyRevolverToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/***************************************
Methods of class RevolverTool::DataItem:
***************************************/

RevolverTool::DataItem::DataItem(GLfloat sDigitHeight)
	:digitHeight(sDigitHeight),
	 spacing(digitHeight*0.25f),
	 digitListBase(glGenLists(11))
	{
	/* Create the digit drawing display lists: */
	GLfloat s=digitHeight*0.5f;
	
	glNewList(digitListBase+0,GL_COMPILE);
	glBegin(GL_LINE_LOOP);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(0.0f*s,0.0f,0.0f*s);
	glVertex3f(1.0f*s,0.0f,0.0f*s);
	glVertex3f(1.0f*s,0.0f,2.0f*s);
	glEnd();
	digitWidths[0]=s;
	glEndList();
	
	glNewList(digitListBase+1,GL_COMPILE);
	glBegin(GL_LINES);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(0.0f*s,0.0f,0.0f*s);
	glEnd();
	digitWidths[1]=0.0f;
	glEndList();
	
	glNewList(digitListBase+2,GL_COMPILE);
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(1.0f*s,0.0f,2.0f*s);
	glVertex3f(1.0f*s,0.0f,1.0f*s);
	glVertex3f(0.0f*s,0.0f,1.0f*s);
	glVertex3f(0.0f*s,0.0f,0.0f*s);
	glVertex3f(1.0f*s,0.0f,0.0f*s);
	glEnd();
	digitWidths[2]=s;
	glEndList();
	
	glNewList(digitListBase+3,GL_COMPILE);
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(1.0f*s,0.0f,2.0f*s);
	glVertex3f(1.0f*s,0.0f,0.0f*s);
	glVertex3f(0.0f*s,0.0f,0.0f*s);
	glEnd();
	glBegin(GL_LINES);
	glVertex3f(0.0f*s,0.0f,1.0f*s);
	glVertex3f(1.0f*s,0.0f,1.0f*s);
	glEnd();
	digitWidths[3]=s;
	glEndList();
	
	glNewList(digitListBase+4,GL_COMPILE);
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(0.0f*s,0.0f,1.0f*s);
	glVertex3f(1.0f*s,0.0f,1.0f*s);
	glEnd();
	glBegin(GL_LINES);
	glVertex3f(1.0f*s,0.0f,2.0f*s);
	glVertex3f(1.0f*s,0.0f,0.0f*s);
	glEnd();
	digitWidths[4]=s;
	glEndList();
	
	glNewList(digitListBase+5,GL_COMPILE);
	glBegin(GL_LINE_STRIP);
	glVertex3f(1.0f*s,0.0f,2.0f*s);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(0.0f*s,0.0f,1.0f*s);
	glVertex3f(1.0f*s,0.0f,1.0f*s);
	glVertex3f(1.0f*s,0.0f,0.0f*s);
	glVertex3f(0.0f*s,0.0f,0.0f*s);
	glEnd();
	digitWidths[5]=s;
	glEndList();
	
	glNewList(digitListBase+6,GL_COMPILE);
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(0.0f*s,0.0f,0.0f*s);
	glVertex3f(1.0f*s,0.0f,0.0f*s);
	glVertex3f(1.0f*s,0.0f,1.0f*s);
	glVertex3f(0.0f*s,0.0f,1.0f*s);
	glEnd();
	digitWidths[6]=s;
	glEndList();
	
	glNewList(digitListBase+7,GL_COMPILE);
	glBegin(GL_LINE_STRIP);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(1.0f*s,0.0f,2.0f*s);
	glVertex3f(1.0f*s,0.0f,0.0f*s);
	glEnd();
	digitWidths[7]=s;
	glEndList();
	
	glNewList(digitListBase+8,GL_COMPILE);
	glBegin(GL_LINE_LOOP);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(0.0f*s,0.0f,0.0f*s);
	glVertex3f(1.0f*s,0.0f,0.0f*s);
	glVertex3f(1.0f*s,0.0f,2.0f*s);
	glEnd();
	glBegin(GL_LINES);
	glVertex3f(0.0f*s,0.0f,1.0f*s);
	glVertex3f(1.0f*s,0.0f,1.0f*s);
	glEnd();
	digitWidths[8]=s;
	glEndList();
	
	glNewList(digitListBase+9,GL_COMPILE);
	glBegin(GL_LINE_STRIP);
	glVertex3f(1.0f*s,0.0f,1.0f*s);
	glVertex3f(0.0f*s,0.0f,1.0f*s);
	glVertex3f(0.0f*s,0.0f,2.0f*s);
	glVertex3f(1.0f*s,0.0f,2.0f*s);
	glVertex3f(1.0f*s,0.0f,0.0f*s);
	glEnd();
	digitWidths[9]=s;
	glEndList();
	
	glNewList(digitListBase+10,GL_COMPILE);
	glBegin(GL_LINES);
	glVertex3f(0.0f*s,0.0f,1.0f*s);
	glVertex3f(1.0f*s,0.0f,1.0f*s);
	glEnd();
	digitWidths[10]=s;
	glEndList();
	}

RevolverTool::DataItem::~DataItem(void)
	{
	glDeleteLists(digitListBase,11);
	}

void RevolverTool::DataItem::writeNumber(const Point& position,int number)
	{
	/* Convert the number to a string: */
	char buffer[20];
	snprintf(buffer,sizeof(buffer),"%d",number);
	
	/* Calculate the number's width: */
	GLfloat width=-spacing;
	for(const char* bPtr=buffer;*bPtr!='\0';++bPtr)
		{
		int index=*bPtr=='-'?10:int(*bPtr)-int('0');
		width+=digitWidths[index];
		width+=spacing;
		}
	
	/* Write the number: */
	glPushMatrix();
	glTranslated(position[0]-width*0.5f,position[1],position[2]-digitHeight*0.5f);
	
	for(const char* bPtr=buffer;*bPtr!='\0';++bPtr)
		{
		GLuint index=*bPtr=='-'?GLuint(10):GLuint(*bPtr)-GLuint('0');
		glCallList(digitListBase+index);
		glTranslatef(digitWidths[index]+spacing,0.0f,0.0f);
		}
	
	glPopMatrix();
	}

/*************************************
Static elements of class RevolverTool:
*************************************/

RevolverToolFactory* RevolverTool::factory=0;

/*****************************
Methods of class RevolverTool:
*****************************/

RevolverTool::RevolverTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:TransformTool(factory,inputAssignment),
	 mappedButtonIndex(0),
	 showNumbersTime(0.0)
	{
	}

RevolverTool::~RevolverTool(void)
	{
	}

void RevolverTool::initialize(void)
	{
	/* Create a virtual input device to shadow the source input device: */
	transformedDevice=addVirtualInputDevice("TransformedDevice",factory->numButtons,0);
	
	/* Disable the virtual input device's glyph: */
	getInputGraphManager()->getInputDeviceGlyph(transformedDevice).disable();
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(transformedDevice,this);
	
	/* Initialize the virtual input device's position: */
	transformedDevice->setTransformation(getDeviceTransformation(0));
	}

const ToolFactory* RevolverTool::getFactory(void) const
	{
	return factory;
	}

void RevolverTool::buttonCallback(int,int deviceButtonIndex,InputDevice::ButtonCallbackData* cbData)
	{
	if(deviceButtonIndex==0)
		{
		/* Pass the button event through to the virtual input device: */
		transformedDevice->setButtonState(mappedButtonIndex,cbData->newButtonState);
		}
	else if(cbData->newButtonState)
		{
		/* Change the currently mapped button: */
		mappedButtonIndex=(mappedButtonIndex+1)%factory->numButtons;
		
		/* Set the newly mapped button's state to the input device's button's state: */
		transformedDevice->setButtonState(mappedButtonIndex,getDeviceButtonState(0,0));
		
		/* Show the current button assignment: */
		showNumbersTime=getApplicationTime()+1.0;
		}
	}

void RevolverTool::frame(void)
	{
	/* Call the base class method: */
	TransformTool::frame();
	
	/* Request a rendering update while the animation is going: */
	if(getApplicationTime()<showNumbersTime)
		requestUpdate();
	}

void RevolverTool::display(GLContextData& contextData) const
	{
	if(getApplicationTime()<showNumbersTime)
		{
		/* Get the context data item: */
		DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
		
		/* Set up OpenGL state: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(1.0f);
		glColor3f(0.0f,1.0f,0.0f);
		glPushMatrix();
		
		/* Draw the "revolver chambers:" */
		glMultMatrix(getDeviceTransformation(0));
		
		Scalar chamberAngle=Scalar(2)*Math::Constants<Scalar>::pi/Scalar(factory->numButtons);
		Scalar angleOffset=Scalar(0);
		double animTime=(getApplicationTime()-(showNumbersTime-1.0))*2.0;
		if(animTime<1.0)
			angleOffset=chamberAngle*Scalar(1.0-animTime);
		
		for(int i=0;i<factory->numButtons;++i)
			{
			Scalar angle=chamberAngle*Scalar(i)+angleOffset;
			Point position(Math::sin(angle)*Scalar(getUiSize()*4.0f),Scalar(0),Math::cos(angle)*Scalar(getUiSize()*4.0f));
			dataItem->writeNumber(position,(mappedButtonIndex+i)%factory->numButtons+1);
			}
		
		glPopMatrix();
		glPopAttrib();
		}
	}

void RevolverTool::initContext(GLContextData& contextData) const
	{
	/* Create and register a data item: */
	DataItem* dataItem=new DataItem(getUiSize()*2.0f);
	contextData.addDataItem(this,dataItem);
	}

}
