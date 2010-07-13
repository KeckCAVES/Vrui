/***********************************************************************
DesktopDeviceNavigationTool - Class to represent a desktop input device
(joystick, spaceball, etc.) as a navigation tool combined with a virtual
input device.
Copyright (c) 2006-2009 Oliver Kreylos

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

#include <Vrui/Tools/DesktopDeviceNavigationTool.h>

namespace Misc {

/**************************************************************
Helper class to read axis descriptors from configuration files:
**************************************************************/

template <>
class ValueCoder<Vrui::DesktopDeviceNavigationToolFactory::AxisDescriptor>
	{
	/* Methods: */
	public:
	static std::string encode(const Vrui::DesktopDeviceNavigationToolFactory::AxisDescriptor& value)
		{
		std::string result="";
		result+="(";
		result+=ValueCoder<int>::encode(value.index);
		result+=", ";
		result+=ValueCoder<Vrui::Vector>::encode(value.axis);
		result+=")";
		return result;
		}
	static Vrui::DesktopDeviceNavigationToolFactory::AxisDescriptor decode(const char* start,const char* end,const char** decodeEnd =0)
		{
		try
			{
			Vrui::DesktopDeviceNavigationToolFactory::AxisDescriptor result;
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
			throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to DesktopDevice axis descriptor"));
			}
		}
	};

}

namespace Vrui {

/***************************************************
Methods of class DesktopDeviceNavigationToolFactory:
***************************************************/

DesktopDeviceNavigationToolFactory::DesktopDeviceNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("DesktopDeviceNavigationTool",toolManager),
	 numButtons(0),buttonToggleFlags(0),buttonAxisShiftMasks(0),
	 numRotationAxes(0),rotationAxes(0),
	 rotateFactor(Scalar(1)),
	 numTranslationAxes(0),translationAxes(0),
	 translateFactor(getInchFactor()),
	 invertNavigation(false),
	 numZoomAxes(0),zoomAxes(0),
	 zoomFactor(Scalar(1)),
	 homeButtonIndex(-1),
	 showScreenCenter(false)
	{
	/* Load class settings: */
	typedef std::vector<int> IndexList;
	typedef std::vector<AxisDescriptor> AxisDescriptorList;
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	
	/* Read number of buttons on raw device: */
	numButtons=cfs.retrieveValue<int>("./numButtons",0);
	buttonToggleFlags=new bool[numButtons];
	buttonAxisShiftMasks=new int[numButtons];
	for(int i=0;i<numButtons;++i)
		{
		buttonToggleFlags[i]=false;
		buttonAxisShiftMasks[i]=0x0;
		}
	
	/* Read list of toggle button indices: */
	IndexList toggleButtonIndices=cfs.retrieveValue<IndexList>("./toggleButtonIndices",IndexList());
	for(IndexList::const_iterator tbiIt=toggleButtonIndices.begin();tbiIt!=toggleButtonIndices.end();++tbiIt)
		{
		if(*tbiIt>=0&&*tbiIt<numButtons)
			buttonToggleFlags[*tbiIt]=true;
		else
			Misc::throwStdErr("DesktopDeviceNavigationTool: Toggle button index out of valid range");
		}
	
	/* Read list of axis shift button indices: */
	IndexList axisShiftButtonIndices=cfs.retrieveValue<IndexList>("./axisShiftButtonIndices",IndexList());
	int nextButtonMask=0x1;
	for(IndexList::const_iterator asbiIt=axisShiftButtonIndices.begin();asbiIt!=axisShiftButtonIndices.end();++asbiIt)
		{
		if(*asbiIt>=0&&*asbiIt<numButtons)
			{
			buttonAxisShiftMasks[*asbiIt]=nextButtonMask;
			nextButtonMask<<=1;
			}
		else
			Misc::throwStdErr("DesktopDeviceNavigationTool: Axis shift button index out of valid range");
		}
	
	/* Read number of valuators on raw device: */
	numValuators=cfs.retrieveValue<int>("./numValuators",0);
	
	/* Read list of rotational axis descriptors: */
	AxisDescriptorList rotationalAxisDescriptors=cfs.retrieveValue<AxisDescriptorList>("./rotationalAxes",AxisDescriptorList());
	numRotationAxes=rotationalAxisDescriptors.size();
	rotationAxes=new AxisDescriptor[numRotationAxes];
	for(int i=0;i<numRotationAxes;++i)
		rotationAxes[i]=rotationalAxisDescriptors[i];
	rotateFactor=cfs.retrieveValue<Scalar>("./rotateFactor",rotateFactor);
	
	/* Read list of translational axis descriptors: */
	AxisDescriptorList translationalAxisDescriptors=cfs.retrieveValue<AxisDescriptorList>("./translationalAxes",AxisDescriptorList());
	numTranslationAxes=translationalAxisDescriptors.size();
	translationAxes=new AxisDescriptor[numTranslationAxes];
	for(int i=0;i<numTranslationAxes;++i)
		translationAxes[i]=translationalAxisDescriptors[i];
	translateFactor=cfs.retrieveValue<Scalar>("./translateFactor",translateFactor);
	
	/* Get navigation button index: */
	navigationButtonIndex=cfs.retrieveValue<int>("./navigationButtonIndex",numButtons-1);
	
	/* Read navigation mode flag: */
	invertNavigation=cfs.retrieveValue<bool>("./invertNavigation",invertNavigation);
	
	/* Read list of zoom axis descriptors: */
	AxisDescriptorList zoomAxisDescriptors=cfs.retrieveValue<AxisDescriptorList>("./zoomAxes",AxisDescriptorList());
	numZoomAxes=zoomAxisDescriptors.size();
	zoomAxes=new AxisDescriptor[numZoomAxes];
	for(int i=0;i<numZoomAxes;++i)
		zoomAxes[i]=zoomAxisDescriptors[i];
	zoomFactor=cfs.retrieveValue<Scalar>("./zoomFactor",zoomFactor);
	
	/* Read the navigation center: */
	navigationCenter=getMainScreen()->getScreenTransformation().transform(Point(getMainScreen()->getWidth()*Scalar(0.5),getMainScreen()->getHeight()*Scalar(0.5),Scalar(0)));
	navigationCenter=cfs.retrieveValue<Point>("./navigationCenter",navigationCenter);
	
	/* Get home button index: */
	homeButtonIndex=cfs.retrieveValue<int>("./homeButtonIndex",homeButtonIndex);
	
	/* Configure device glyph for virtual DesktopDevice device: */
	deviceGlyph.configure(cfs,"./deviceGlyphType","./deviceGlyphMaterial");
	
	showScreenCenter=cfs.retrieveValue<bool>("./showScreenCenter",showScreenCenter);
	
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,numButtons);
	layout.setNumValuators(0,numValuators);
	
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("NavigationTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	
	/* Set tool class' factory pointer: */
	DesktopDeviceNavigationTool::factory=this;
	}

DesktopDeviceNavigationToolFactory::~DesktopDeviceNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	DesktopDeviceNavigationTool::factory=0;
	
	delete[] buttonToggleFlags;
	delete[] buttonAxisShiftMasks;
	delete[] rotationAxes;
	delete[] translationAxes;
	delete[] zoomAxes;
	}

const char* DesktopDeviceNavigationToolFactory::getName(void) const
	{
	return "Desktop Device";
	}

Tool* DesktopDeviceNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new DesktopDeviceNavigationTool(this,inputAssignment);
	}

void DesktopDeviceNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveDesktopDeviceNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("NavigationTool");
	}

extern "C" ToolFactory* createDesktopDeviceNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	DesktopDeviceNavigationToolFactory* spaceBallNavigationToolFactory=new DesktopDeviceNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return spaceBallNavigationToolFactory;
	}

extern "C" void destroyDesktopDeviceNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/****************************************************
Static elements of class DesktopDeviceNavigationTool:
****************************************************/

DesktopDeviceNavigationToolFactory* DesktopDeviceNavigationTool::factory=0;

/********************************************
Methods of class DesktopDeviceNavigationTool:
********************************************/

void DesktopDeviceNavigationTool::setButtonState(int buttonIndex,bool newButtonState)
	{
	if(buttonIndex==factory->homeButtonIndex)
		{
		if(newButtonState)
			{
			/* Reset the virtual input device to its home position: */
			virtualDevice->setTransformation(homePosition);
			}
		}
	else if(buttonIndex==factory->navigationButtonIndex)
		{
		/* Set navigation state: */
		if(newButtonState)
			{
			/* Try activating this tool: */
			if(activate())
				{
				/* Initialize the navigation transformation: */
				postScale=getNavigationTransformation();
				}
			}
		else
			{
			/* Deactivate this tool: */
			deactivate();
			}
		}
	else if(factory->buttonAxisShiftMasks[buttonIndex]!=0x0)
		{
		/* Update the current axis shift mask: */
		if(newButtonState)
			axisIndexBase|=factory->buttonAxisShiftMasks[buttonIndex];
		else
			axisIndexBase&=~factory->buttonAxisShiftMasks[buttonIndex];
		}
	else
		{
		/* Pass the button event through to the virtual input device: */
		virtualDevice->setButtonState(buttonIndex,newButtonState);
		}
	}

DesktopDeviceNavigationTool::DesktopDeviceNavigationTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(sFactory,inputAssignment),
	 virtualDevice(0),homePosition(TrackerState::identity),
	 toggleButtonStates(new bool[factory->numButtons]),
	 axisIndexBase(0)
	{
	/* Initialize toggle button states: */
	for(int i=0;i<factory->numButtons;++i)
		toggleButtonStates[i]=false;
	}

DesktopDeviceNavigationTool::~DesktopDeviceNavigationTool(void)
	{
	delete[] toggleButtonStates;
	}

void DesktopDeviceNavigationTool::initialize(void)
	{
	/* Create a virtual input device to shadow the raw DesktopDevice device: */
	virtualDevice=addVirtualInputDevice("VirtualDesktopDevice",factory->numButtons,0);
	getInputGraphManager()->getInputDeviceGlyph(virtualDevice)=factory->deviceGlyph;
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(virtualDevice,this);
	
	/* Store the device's home position: */
	homePosition=virtualDevice->getTransformation();
	}

void DesktopDeviceNavigationTool::deinitialize(void)
	{
	/* Release the virtual input device: */
	getInputGraphManager()->releaseInputDevice(virtualDevice,this);
	
	/* Destroy the virtual input device: */
	getInputDeviceManager()->destroyInputDevice(virtualDevice);
	}

const ToolFactory* DesktopDeviceNavigationTool::getFactory(void) const
	{
	return factory;
	}

void DesktopDeviceNavigationTool::buttonCallback(int,int buttonIndex,InputDevice::ButtonCallbackData* cbData)
	{
	if(factory->buttonToggleFlags[buttonIndex])
		{
		/* Check if the toggle state has to be changed: */
		if(!cbData->newButtonState)
			{
			/* Change the button state: */
			toggleButtonStates[buttonIndex]=!toggleButtonStates[buttonIndex];
			setButtonState(buttonIndex,toggleButtonStates[buttonIndex]);
			}
		}
	else
		setButtonState(buttonIndex,cbData->newButtonState);
	}

void DesktopDeviceNavigationTool::frame(void)
	{
	int aib=axisIndexBase*factory->numValuators;
	
	/* Convert translational axes into translation vector: */
	Vector translation=Vector::zero;
	for(int i=0;i<factory->numTranslationAxes;++i)
		if(factory->translationAxes[i].index>=aib&&factory->translationAxes[i].index<aib+factory->numValuators)
			translation+=factory->translationAxes[i].axis*getDeviceValuator(0,factory->translationAxes[i].index-aib);
	translation*=factory->translateFactor*getFrameTime();
	
	/* Convert rotational axes into rotation axis vector and rotation angle: */
	Vector scaledRotationAxis=Vector::zero;
	for(int i=0;i<factory->numRotationAxes;++i)
		if(factory->rotationAxes[i].index>=aib&&factory->rotationAxes[i].index<aib+factory->numValuators)
			scaledRotationAxis+=factory->rotationAxes[i].axis*getDeviceValuator(0,factory->rotationAxes[i].index-aib);
	scaledRotationAxis*=factory->rotateFactor*getFrameTime();
	
	/* Act depending on the tool's activation state: */
	if(isActive())
		{
		/* Convert zoom axes into zoom factor: */
		Vector deltaZoom=Vector::zero;
		for(int i=0;i<factory->numZoomAxes;++i)
			if(factory->zoomAxes[i].index>=aib&&factory->zoomAxes[i].index<aib+factory->numValuators)
				deltaZoom+=factory->zoomAxes[i].axis*getDeviceValuator(0,factory->zoomAxes[i].index-aib);
		deltaZoom*=factory->zoomFactor*getFrameTime();
		
		if(translation!=Vector::zero||scaledRotationAxis!=Vector::zero||deltaZoom[2]!=Scalar(0))
			{
			/* Apply proper navigation mode: */
			if(factory->invertNavigation)
				{
				translation=-translation;
				scaledRotationAxis=-scaledRotationAxis;
				}
			
			/* Calculate an incremental transformation based on the translation and rotation: */
			NavTrackerState deltaT=NavTrackerState::translateFromOriginTo(factory->navigationCenter);
			deltaT*=NavTrackerState::translate(translation);
			deltaT*=NavTrackerState::rotate(NavTrackerState::Rotation::rotateScaledAxis(scaledRotationAxis));
			deltaT*=NavTrackerState::scale(Math::exp(-deltaZoom[2]));
			deltaT*=NavTrackerState::translateToOriginFrom(factory->navigationCenter);
			
			/* Update the accumulated transformation: */
			postScale.leftMultiply(deltaT);
			
			/* Update the navigation transformation: */
			setNavigationTransformation(postScale);
			
			requestUpdate();
			}
		}
	else if(translation!=Vector::zero||scaledRotationAxis!=Vector::zero)
		{
		/* Calculate an incremental transformation for the virtual input device: */
		ONTransform deltaT=ONTransform::translate(translation);
		Point pos=virtualDevice->getPosition();
		deltaT*=ONTransform::translateFromOriginTo(pos);
		deltaT*=ONTransform::rotate(ONTransform::Rotation::rotateScaledAxis(scaledRotationAxis));
		deltaT*=ONTransform::translateToOriginFrom(pos);
		
		/* Update the virtual input device's transformation: */
		deltaT*=virtualDevice->getTransformation();
		deltaT.renormalize();
		virtualDevice->setTransformation(deltaT);
		
		requestUpdate();
		}
	}

void DesktopDeviceNavigationTool::display(GLContextData& contextData) const
	{
	if(factory->showScreenCenter&&isActive())
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
