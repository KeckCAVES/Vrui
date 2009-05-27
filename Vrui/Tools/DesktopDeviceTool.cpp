/***********************************************************************
DesktopDeviceTool - Class to represent a desktop input device (joystick,
spaceball, etc.) as a virtual input device.
Copyright (c) 2009 Oliver Kreylos

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

#include <Vrui/Tools/DesktopDeviceTool.h>

namespace Misc {

/**************************************************************
Helper class to read axis descriptors from configuration files:
**************************************************************/

template <>
class ValueCoder<Vrui::DesktopDeviceToolFactory::AxisDescriptor>
	{
	/* Methods: */
	public:
	static std::string encode(const Vrui::DesktopDeviceToolFactory::AxisDescriptor& value)
		{
		std::string result="";
		result+="(";
		result+=ValueCoder<int>::encode(value.index);
		result+=", ";
		result+=ValueCoder<Vrui::Vector>::encode(value.axis);
		result+=")";
		return result;
		}
	static Vrui::DesktopDeviceToolFactory::AxisDescriptor decode(const char* start,const char* end,const char** decodeEnd =0)
		{
		try
			{
			Vrui::DesktopDeviceToolFactory::AxisDescriptor result;
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

/*****************************************
Methods of class DesktopDeviceToolFactory:
*****************************************/

DesktopDeviceToolFactory::DesktopDeviceToolFactory(ToolManager& toolManager)
	:ToolFactory("DesktopDeviceTool",toolManager),
	 numButtons(0),buttonToggleFlags(0),buttonAxisShiftMasks(0),
	 numRotationAxes(0),rotationAxes(0),
	 rotateFactor(Scalar(1)),
	 numTranslationAxes(0),translationAxes(0),
	 translateFactor(getInchFactor()),
	 homeButtonIndex(-1)
	{
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("TransformTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	
	/* Load class settings: */
	typedef std::vector<int> IndexList;
	typedef std::vector<AxisDescriptor> AxisDescriptorList;
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	
	/* Read number of buttons on raw device: */
	numButtons=cfs.retrieveValue<int>("./numButtons");
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
			Misc::throwStdErr("DesktopDeviceTool: Toggle button index out of valid range");
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
			Misc::throwStdErr("DesktopDeviceTool: Axis shift button index out of valid range");
		}
	
	/* Read number of valuators on raw device: */
	numValuators=cfs.retrieveValue<int>("./numValuators");
	
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
	
	/* Get home button index: */
	homeButtonIndex=cfs.retrieveValue<int>("./homeButtonIndex",homeButtonIndex);
	
	/* Configure device glyph for virtual DesktopDevice device: */
	deviceGlyph.configure(cfs,"./deviceGlyphType","./deviceGlyphMaterial");
	
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,numButtons);
	layout.setNumValuators(0,numValuators);
	
	/* Set tool class' factory pointer: */
	DesktopDeviceTool::factory=this;
	}

DesktopDeviceToolFactory::~DesktopDeviceToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	DesktopDeviceTool::factory=0;
	
	delete[] buttonToggleFlags;
	delete[] buttonAxisShiftMasks;
	delete[] rotationAxes;
	delete[] translationAxes;
	}

const char* DesktopDeviceToolFactory::getName(void) const
	{
	return "Desktop Device";
	}

Tool* DesktopDeviceToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new DesktopDeviceTool(this,inputAssignment);
	}

void DesktopDeviceToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveDesktopDeviceToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createDesktopDeviceToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	DesktopDeviceToolFactory* desktopDeviceToolFactory=new DesktopDeviceToolFactory(*toolManager);
	
	/* Return factory object: */
	return desktopDeviceToolFactory;
	}

extern "C" void destroyDesktopDeviceToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/******************************************
Static elements of class DesktopDeviceTool:
******************************************/

DesktopDeviceToolFactory* DesktopDeviceTool::factory=0;

/**********************************
Methods of class DesktopDeviceTool:
**********************************/

bool DesktopDeviceTool::setButtonState(int buttonIndex,bool newButtonState)
	{
	bool result=false;
	
	if(factory->buttonToggleFlags[buttonIndex])
		{
		if(!newButtonState)
			{
			result=true;
			buttonStates[buttonIndex]=!buttonStates[buttonIndex];
			}
		}
	else
		{
		result=buttonStates[buttonIndex]!=newButtonState;
		buttonStates[buttonIndex]=newButtonState;
		}
	
	return result;
	}

DesktopDeviceTool::DesktopDeviceTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:TransformTool(sFactory,inputAssignment),
	 homePosition(TrackerState::identity),
	 axisIndexBase(0)
	{
	/* Re-initialize toggle button states: */
	delete[] buttonStates;
	buttonStates=new bool[factory->numButtons];
	for(int i=0;i<factory->numButtons;++i)
		buttonStates[i]=false;
	}

DesktopDeviceTool::~DesktopDeviceTool(void)
	{
	}

void DesktopDeviceTool::initialize(void)
	{
	/* Create a virtual input device: */
	transformedDevice=addVirtualInputDevice("TransformedDevice",factory->numButtons,factory->numValuators);
	
	/* Set the virtual device's glyph to the configured glyph: */
	getInputGraphManager()->getInputDeviceGlyph(transformedDevice)=factory->deviceGlyph;
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(transformedDevice,this);
	
	/* Store the device's home position: */
	homePosition=transformedDevice->getTransformation();
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(transformedDevice,this);
	
	}

const ToolFactory* DesktopDeviceTool::getFactory(void) const
	{
	return factory;
	}

void DesktopDeviceTool::buttonCallback(int,int deviceButtonIndex,InputDevice::ButtonCallbackData* cbData)
	{
	/* Set the new button state and react if it changed: */
	if(setButtonState(deviceButtonIndex,cbData->newButtonState))
		{
		if(deviceButtonIndex==factory->homeButtonIndex)
			{
			if(buttonStates[deviceButtonIndex])
				{
				/* Reset the virtual input device to its home position: */
				transformedDevice->setTransformation(homePosition);
				}
			}
		else if(factory->buttonAxisShiftMasks[deviceButtonIndex]!=0x0)
			{
			/* Update the current axis shift mask: */
			if(buttonStates[deviceButtonIndex])
				axisIndexBase|=factory->buttonAxisShiftMasks[deviceButtonIndex];
			else
				axisIndexBase&=~factory->buttonAxisShiftMasks[deviceButtonIndex];
			}
		else
			{
			/* Pass the button event through to the virtual input device: */
			transformedDevice->setButtonState(deviceButtonIndex,buttonStates[deviceButtonIndex]);
			}
		}
	}

void DesktopDeviceTool::valuatorCallback(int,int deviceValuatorIndex,InputDevice::ValuatorCallbackData* cbData)
	{
	/* Don't do anything */
	}

void DesktopDeviceTool::frame(void)
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
	
	/* Calculate an incremental transformation for the virtual input device: */
	ONTransform deltaT=ONTransform::translate(translation);
	Point pos=transformedDevice->getPosition();
	deltaT*=ONTransform::translateFromOriginTo(pos);
	deltaT*=ONTransform::rotate(ONTransform::Rotation::rotateScaledAxis(scaledRotationAxis));
	deltaT*=ONTransform::translateToOriginFrom(pos);
	
	/* Update the virtual input device's transformation: */
	deltaT*=transformedDevice->getTransformation();
	deltaT.renormalize();
	transformedDevice->setTransformation(deltaT);
		
	requestUpdate();
	}

}
