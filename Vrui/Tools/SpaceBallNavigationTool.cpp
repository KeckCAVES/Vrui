/***********************************************************************
SpaceBallNavigationTool - Class to represent a raw 6-DOF SpaceBall
device as a navigation tool combined with a virtual input device.
Copyright (c) 2006-2008 Oliver Kreylos

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

#include <ctype.h>
#include <vector>
#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/VRScreen.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/SpaceBallNavigationTool.h>

namespace Misc {

/**************************************************************
Helper class to read axis descriptors from configuration files:
**************************************************************/

template <>
class ValueCoder<Vrui::SpaceBallNavigationToolFactory::AxisDescriptor>
	{
	/* Methods: */
	public:
	static std::string encode(const Vrui::SpaceBallNavigationToolFactory::AxisDescriptor& value)
		{
		std::string result="";
		result+="(";
		result+=ValueCoder<int>::encode(value.index);
		result+=", ";
		result+=ValueCoder<Vrui::Vector>::encode(value.axis);
		result+=")";
		return result;
		}
	static Vrui::SpaceBallNavigationToolFactory::AxisDescriptor decode(const char* start,const char* end,const char** decodeEnd =0)
		{
		try
			{
			Vrui::SpaceBallNavigationToolFactory::AxisDescriptor result;
			const char* cPtr=start;
			
			/* Check for opening parenthesis: */
			if(cPtr==end||*cPtr!='(')
				throw 42;
			++cPtr;
			
			/* Skip whitespace: */
			while(cPtr!=end&&isspace(*cPtr))
				++cPtr;
			
			/* Decode axis index: */
			result.index=ValueCoder<int>::decode(cPtr,end,&cPtr);
			
			/* Skip whitespace: */
			while(cPtr!=end&&isspace(*cPtr))
				++cPtr;
			
			/* Check for comma separator: */
			if(cPtr==end||*cPtr!=',')
				throw 42;
			++cPtr;
			
			/* Skip whitespace: */
			while(cPtr!=end&&isspace(*cPtr))
				++cPtr;
			
			/* Decode axis: */
			result.axis=ValueCoder<Vrui::Vector>::decode(cPtr,end,&cPtr);
			
			/* Skip whitespace: */
			while(cPtr!=end&&isspace(*cPtr))
				++cPtr;
			
			/* Check for closing parenthesis: */
			if(cPtr==end||*cPtr!=')')
				throw 42;
			++cPtr;
			
			if(decodeEnd!=0)
				*decodeEnd=cPtr;
			return result;
			}
		catch(...)
			{
			throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to SpaceBall axis descriptor"));
			}
		}
	}

}

namespace Vrui {

/***********************************************
Methods of class SpaceBallNavigationToolFactory:
***********************************************/

SpaceBallNavigationToolFactory::SpaceBallNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("SpaceBallNavigationTool",toolManager),
	 numButtons(0),buttonToggleFlags(0),
	 numRotationAxes(0),rotationAxes(0),
	 rotateFactor(Scalar(1)),
	 numTranslationAxes(0),translationAxes(0),
	 translateFactor(getInchFactor()),
	 showScreenCenter(false)
	{
	/* Load class settings: */
	typedef std::vector<int> IndexList;
	typedef std::vector<AxisDescriptor> AxisDescriptorList;
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	
	/* Read number of buttons on raw SpaceBall device: */
	numButtons=cfs.retrieveValue<int>("./numButtons");
	buttonToggleFlags=new bool[numButtons];
	for(int i=0;i<numButtons;++i)
		buttonToggleFlags[i]=false;
	
	/* Read list of toggle button indices: */
	IndexList toggleButtonIndices=cfs.retrieveValue<IndexList>("./toggleButtonIndices",IndexList());
	for(IndexList::const_iterator tbiIt=toggleButtonIndices.begin();tbiIt!=toggleButtonIndices.end();++tbiIt)
		{
		if(*tbiIt>=0&&*tbiIt<numButtons)
			buttonToggleFlags[*tbiIt]=true;
		else
			Misc::throwStdErr("SpaceBallNavigationTool: Button index out of valid range");
		}
	
	/* Read list of rotational axis descriptors: */
	AxisDescriptorList rotationalAxisDescriptors=cfs.retrieveValue<AxisDescriptorList>("./rotationalAxes");
	numRotationAxes=rotationalAxisDescriptors.size();
	rotationAxes=new AxisDescriptor[numRotationAxes];
	for(int i=0;i<numRotationAxes;++i)
		rotationAxes[i]=rotationalAxisDescriptors[i];
	rotateFactor=cfs.retrieveValue<Scalar>("./rotateFactor",rotateFactor);
	
	/* Read list of translational axis descriptors: */
	AxisDescriptorList translationalAxisDescriptors=cfs.retrieveValue<AxisDescriptorList>("./translationalAxes");
	numTranslationAxes=translationalAxisDescriptors.size();
	translationAxes=new AxisDescriptor[numTranslationAxes];
	for(int i=0;i<numTranslationAxes;++i)
		translationAxes[i]=translationalAxisDescriptors[i];
	translateFactor=cfs.retrieveValue<Scalar>("./translateFactor",translateFactor);
	
	/* Get navigation toggle buttons: */
	navigationToggleButtonIndex=cfs.retrieveValue<int>("./navigationToggleButtonIndex",numButtons-1);
	zoomToggleButtonIndex=cfs.retrieveValue<int>("./zoomToggleButtonIndex",numButtons-2);
	
	/* Configure device glyph for virtual SpaceBall device: */
	deviceGlyph.configure(cfs,"./deviceGlyphType","./deviceGlyphMaterial");
	
	showScreenCenter=cfs.retrieveValue<bool>("./showScreenCenter",showScreenCenter);
	
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,numButtons);
	layout.setNumValuators(0,numRotationAxes+numTranslationAxes);
	
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("NavigationTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	
	/* Set tool class' factory pointer: */
	SpaceBallNavigationTool::factory=this;
	}

SpaceBallNavigationToolFactory::~SpaceBallNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	SpaceBallNavigationTool::factory=0;
	
	delete[] buttonToggleFlags;
	delete[] rotationAxes;
	delete[] translationAxes;
	}

Tool* SpaceBallNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new SpaceBallNavigationTool(this,inputAssignment);
	}

void SpaceBallNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveSpaceBallNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("NavigationTool");
	}

extern "C" ToolFactory* createSpaceBallNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	SpaceBallNavigationToolFactory* spaceBallNavigationToolFactory=new SpaceBallNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return spaceBallNavigationToolFactory;
	}

extern "C" void destroySpaceBallNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/***********************************************
Static elements of class SpaceBallNavigationTool:
***********************************************/

SpaceBallNavigationToolFactory* SpaceBallNavigationTool::factory=0;

/****************************************
Methods of class SpaceBallNavigationTool:
****************************************/

SpaceBallNavigationTool::SpaceBallNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(factory,inputAssignment),
	 spaceBall(0),
	 toggleButtonStates(new bool[static_cast<const SpaceBallNavigationToolFactory*>(factory)->numButtons]),
	 navigationMode(IDLE)
	{
	/* Initialize toggle button states: */
	for(int i=0;i<static_cast<const SpaceBallNavigationToolFactory*>(factory)->numButtons;++i)
		toggleButtonStates[i]=false;
	}

SpaceBallNavigationTool::~SpaceBallNavigationTool(void)
	{
	delete[] toggleButtonStates;
	}

void SpaceBallNavigationTool::initialize(void)
	{
	/* Create a virtual input device to shadow the raw SpaceBall device: */
	spaceBall=addVirtualInputDevice("VirtualSpaceBall",factory->numButtons,0);
	getInputGraphManager()->getInputDeviceGlyph(spaceBall)=factory->deviceGlyph;
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(spaceBall,this);
	}

void SpaceBallNavigationTool::deinitialize(void)
	{
	/* Release the virtual input device: */
	getInputGraphManager()->releaseInputDevice(spaceBall,this);
	
	/* Destroy the virtual input device: */
	getInputDeviceManager()->destroyInputDevice(spaceBall);
	}

const ToolFactory* SpaceBallNavigationTool::getFactory(void) const
	{
	return factory;
	}

void SpaceBallNavigationTool::buttonCallback(int,int buttonIndex,InputDevice::ButtonCallbackData* cbData)
	{
	if(buttonIndex==factory->navigationToggleButtonIndex)
		{
		if(cbData->newButtonState)
			{
			/* Toggle navigation state: */
			switch(navigationMode)
				{
				case IDLE:
					/* Try activating this tool: */
					if(activate)
						{
						/* Calculate the rotation center: */
						screenCenter=getMainScreen()->getScreenTransformation().transform(Point(getMainScreen()->getWidth()*Scalar(0.5),getMainScreen()->getHeight()*Scalar(0.5),Scalar(0)));
						
						preScale=NavTrackerState::translateFromOriginTo(screenCenter);
						rotation=NavTrackerState::identity;
						postScale=NavTrackerState::translateToOriginFrom(screenCenter);
						postScale*=getNavigationTransformation();
						
						/* Go to moving mode: */
						navigationMode=MOVING;
						}
					break;
				
				case MOVING:
				case ZOOMING:
					/* Deactivate this tool: */
					deactivate();
					
					/* Go to idle mode: */
					navigationMode=IDLE;
					break;
				}
			}
		}
	else if(buttonIndex==factory->zoomToggleButtonIndex&&navigationMode!=IDLE)
		{
		switch(navigationMode)
			{
			case MOVING:
				/* Calculate the scaling center: */
				screenCenter=getMainScreen()->getScreenTransformation().transform(Point(getMainScreen()->getWidth()*Scalar(0.5),getMainScreen()->getHeight()*Scalar(0.5),Scalar(0)));
				
				preScale=NavTrackerState::translateFromOriginTo(screenCenter);
				zoom=Scalar(1);
				postScale=NavTrackerState::translateToOriginFrom(screenCenter);
				postScale*=getNavigationTransformation();
				
				/* Go to zooming mode: */
				navigationMode=ZOOMING;
				break;
			
			case ZOOMING:
				/* Calculate the rotation center: */
				screenCenter=getMainScreen()->getScreenTransformation().transform(Point(getMainScreen()->getWidth()*Scalar(0.5),getMainScreen()->getHeight()*Scalar(0.5),Scalar(0)));
				
				preScale=NavTrackerState::translateFromOriginTo(screenCenter);
				rotation=NavTrackerState::identity;
				postScale=NavTrackerState::translateToOriginFrom(screenCenter);
				postScale*=getNavigationTransformation();
				
				/* Go to moving mode: */
				navigationMode=MOVING;
				break;
			
			default:
				;
			}
		}
	else if(factory->buttonToggleFlags[buttonIndex])
		{
		/* Check if the toggle state has to be changed: */
		if(!cbData->newButtonState)
			toggleButtonStates[buttonIndex]=!toggleButtonStates[buttonIndex];
		
		/* Pass the possibly changed toggle button state through to the virtual input device: */
		spaceBall->setButtonState(buttonIndex,toggleButtonStates[buttonIndex]);
		}
	else
		{
		/* Pass the button event through to the virtual input device: */
		spaceBall->setButtonState(buttonIndex,cbData->newButtonState);
		}
	}

void SpaceBallNavigationTool::frame(void)
	{
	/* Convert linear SpaceBall axes into translation vector: */
	Vector translation=Vector::zero;
	for(int i=0;i<factory->numTranslationAxes;++i)
		translation+=factory->translationAxes[i].axis*input.getDevice(0)->getValuator(factory->translationAxes[i].index);
	translation*=factory->translateFactor*getCurrentFrameTime();
	
	/* Convert rotational SpaceBall axes into rotation axis vector and rotation angle: */
	Vector scaledRotationAxis=Vector::zero;
	for(int i=0;i<factory->numRotationAxes;++i)
		scaledRotationAxis+=factory->rotationAxes[i].axis*input.getDevice(0)->getValuator(factory->rotationAxes[i].index);
	scaledRotationAxis*=factory->rotateFactor*getCurrentFrameTime();
	
	switch(navigationMode)
		{
		case IDLE:
			{
			/* Calculate an incremental transformation for the virtual input device: */
			ONTransform deltaT=ONTransform::translate(translation);
			Point pos=spaceBall->getPosition();
			deltaT*=ONTransform::translateFromOriginTo(pos);
			deltaT*=ONTransform::rotate(ONTransform::Rotation::rotateScaledAxis(scaledRotationAxis));
			deltaT*=ONTransform::translateToOriginFrom(pos);
			
			/* Update the virtual input device's transformation: */
			deltaT*=spaceBall->getTransformation();
			spaceBall->setTransformation(deltaT);
			break;
			}
		
		case MOVING:
			{
			/* Calculate an incremental transformation based on the translation and rotation: */
			NavTrackerState deltaT=NavTrackerState::translate(translation);
			deltaT*=NavTrackerState::rotate(NavTrackerState::Rotation::rotateScaledAxis(scaledRotationAxis));
			
			/* Update the accumulated transformation: */
			rotation.leftMultiply(deltaT);
			
			/* Update the navigation transformation: */
			NavTrackerState t=preScale;
			t*=rotation;
			t*=postScale;
			setNavigationTransformation(t);
			break;
			}
		
		case ZOOMING:
			{
			/* Update the accumulated zooming factor: */
			zoom*=Math::exp(-translation[2]);
			
			/* Update the navigation transformation: */
			NavTrackerState t=preScale;
			t*=NavTrackerState::scale(zoom);
			t*=postScale;
			setNavigationTransformation(t);
			break;
			}
		}
	}

void SpaceBallNavigationTool::display(GLContextData& contextData) const
	{
	if(factory->showScreenCenter&&navigationMode!=IDLE)
		{
		Color bgColor=getBackgroundColor();
		Color fgColor;
		for(int i=0;i<3;++i)
			fgColor[i]=1.0f-bgColor[i];
		fgColor[3]=bgColor[3];
		ONTransform screenT=getMainScreen()->getScreenTransformation();
		Point screenCenter=screenT.transform(Point(getMainScreen()->getWidth()*Scalar(0.5),getMainScreen()->getHeight()*Scalar(0.5),Scalar(0)));
		Vector x=screenT.transform(Vector(getMainScreen()->getWidth()*Scalar(0.5),Scalar(0),Scalar(0)));
		Vector y=screenT.transform(Vector(Scalar(0),getMainScreen()->getHeight()*Scalar(0.5),Scalar(0)));
		glPushAttrib(GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glDepthFunc(GL_LEQUAL);
		glLineWidth(3.0f);
		glColor(bgColor);
		glBegin(GL_LINES);
		glVertex(screenCenter-x);
		glVertex(screenCenter+x);
		glVertex(screenCenter-y);
		glVertex(screenCenter+y);
		glEnd();
		glLineWidth(1.0f);
		glColor(fgColor);
		glBegin(GL_LINES);
		glVertex(screenCenter-x);
		glVertex(screenCenter+x);
		glVertex(screenCenter-y);
		glVertex(screenCenter+y);
		glEnd();
		glPopAttrib();
		}
	}

}
