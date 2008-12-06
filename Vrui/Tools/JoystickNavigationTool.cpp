/***********************************************************************
JoystickNavigationTool - Class to represent a raw joystick device as a
navigation tool combined with a virtual input device.
Copyright (c) 2005-2008 Oliver Kreylos

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
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/JoystickNavigationTool.h>

namespace Misc {

/**************************************************************
Helper class to read axis descriptors from configuration files:
**************************************************************/

template <>
class ValueCoder<Vrui::JoystickNavigationToolFactory::AxisDescriptor>
	{
	/* Methods: */
	public:
	static std::string encode(const Vrui::JoystickNavigationToolFactory::AxisDescriptor& value)
		{
		std::string result="";
		result+="(";
		result+=ValueCoder<int>::encode(value.index);
		result+=", ";
		result+=ValueCoder<Vrui::Vector>::encode(value.axis);
		result+=")";
		return result;
		}
	static Vrui::JoystickNavigationToolFactory::AxisDescriptor decode(const char* start,const char* end,const char** decodeEnd =0)
		{
		try
			{
			Vrui::JoystickNavigationToolFactory::AxisDescriptor result;
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
			throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to joystick axis descriptor"));
			}
		}
	}

}

namespace Vrui {

/**********************************************
Methods of class JoystickNavigationToolFactory:
**********************************************/

JoystickNavigationToolFactory::JoystickNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("JoystickNavigationTool",toolManager),
	 numButtons(0),
	 buttonToggleFlags(0),
	 numRotationAxes(0),
	 rotationAxes(0),
	 rotateFactor(Scalar(1)),
	 numTranslationAxes(0),
	 translationAxes(0),
	 translateFactor(getInchFactor())
	{
	/* Load class settings: */
	typedef std::vector<int> IndexList;
	typedef std::vector<AxisDescriptor> AxisDescriptorList;
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	
	/* Read number of buttons on raw joystick device: */
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
			Misc::throwStdErr("JoystickNavigationTool: Button index out of valid range");
		}
	
	/* Read list of rotational axis descriptors: */
	rotateFactor=cfs.retrieveValue<Scalar>("./rotateFactor",rotateFactor);
	AxisDescriptorList rotationalAxisDescriptors=cfs.retrieveValue<AxisDescriptorList>("./rotationalAxes");
	numRotationAxes=rotationalAxisDescriptors.size();
	rotationAxes=new AxisDescriptor[numRotationAxes];
	for(int i=0;i<numRotationAxes;++i)
		rotationAxes[i]=rotationalAxisDescriptors[i];
	
	/* Read list of translational axis descriptors: */
	translateFactor=cfs.retrieveValue<Scalar>("./translateFactor",translateFactor);
	AxisDescriptorList translationalAxisDescriptors=cfs.retrieveValue<AxisDescriptorList>("./translationalAxes");
	numTranslationAxes=translationalAxisDescriptors.size();
	translationAxes=new AxisDescriptor[numTranslationAxes];
	for(int i=0;i<numTranslationAxes;++i)
		translationAxes[i]=translationalAxisDescriptors[i];
	
	/* Get navigation toggle button: */
	navigationToggleButtonIndex=cfs.retrieveValue<int>("./navigationToggleButtonIndex",numButtons-1);
	
	/* Configure device glyph for virtual joystick device: */
	deviceGlyph.configure(cfs,"./deviceGlyphType","./deviceGlyphMaterial");
	
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,numButtons);
	layout.setNumValuators(0,numRotationAxes+numTranslationAxes);
	
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("NavigationTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	
	/* Set tool class' factory pointer: */
	JoystickNavigationTool::factory=this;
	}

JoystickNavigationToolFactory::~JoystickNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	JoystickNavigationTool::factory=0;
	
	delete[] buttonToggleFlags;
	delete[] rotationAxes;
	delete[] translationAxes;
	}

Tool* JoystickNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new JoystickNavigationTool(this,inputAssignment);
	}

void JoystickNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveJoystickNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("NavigationTool");
	}

extern "C" ToolFactory* createJoystickNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	JoystickNavigationToolFactory* joystickNavigationToolFactory=new JoystickNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return joystickNavigationToolFactory;
	}

extern "C" void destroyJoystickNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/***********************************************
Static elements of class JoystickNavigationTool:
***********************************************/

JoystickNavigationToolFactory* JoystickNavigationTool::factory=0;

/***************************************
Methods of class JoystickNavigationTool:
***************************************/

JoystickNavigationTool::JoystickNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(factory,inputAssignment),
	 joystick(0),
	 toggleButtonStates(new bool[static_cast<const JoystickNavigationToolFactory*>(factory)->numButtons])
	{
	/* Initialize toggle button states: */
	for(int i=0;i<static_cast<const JoystickNavigationToolFactory*>(factory)->numButtons;++i)
		toggleButtonStates[i]=false;
	}

JoystickNavigationTool::~JoystickNavigationTool(void)
	{
	delete[] toggleButtonStates;
	}

void JoystickNavigationTool::initialize(void)
	{
	/* Create a virtual input device to shadow the raw joystick device: */
	joystick=addVirtualInputDevice("VirtualJoystick",factory->numButtons,0);
	getInputGraphManager()->getInputDeviceGlyph(joystick)=factory->deviceGlyph;
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(joystick,this);
	}

void JoystickNavigationTool::deinitialize(void)
	{
	/* Release the virtual input device: */
	getInputGraphManager()->releaseInputDevice(joystick,this);
	
	/* Destroy the virtual input device: */
	getInputDeviceManager()->destroyInputDevice(joystick);
	}

const ToolFactory* JoystickNavigationTool::getFactory(void) const
	{
	return factory;
	}

void JoystickNavigationTool::buttonCallback(int,int buttonIndex,InputDevice::ButtonCallbackData* cbData)
	{
	if(buttonIndex==factory->navigationToggleButtonIndex)
		{
		if(cbData->newButtonState)
			{
			/* Toggle the navigation state: */
			if(isActive())
				{
				/* Deactivate this tool: */
				deactivate();
				}
			else
				{
				/* Try activating this tool: */
				activate();
				}
			}
		}
	else if(factory->buttonToggleFlags[buttonIndex])
		{
		/* Check if the toggle state has to be changed: */
		if(!cbData->newButtonState)
			toggleButtonStates[buttonIndex]=!toggleButtonStates[buttonIndex];
		
		/* Pass the possibly changed toggle button state through to the virtual input device: */
		joystick->setButtonState(buttonIndex,toggleButtonStates[buttonIndex]);
		}
	else
		{
		/* Pass the button event through to the virtual input device: */
		joystick->setButtonState(buttonIndex,cbData->newButtonState);
		}
	}

void JoystickNavigationTool::frame(void)
	{
	/* Convert rotational joystick axes into rotation axis vector and rotation angle: */
	Vector rotation=Vector::zero;
	for(int i=0;i<factory->numRotationAxes;++i)
		rotation+=factory->rotationAxes[i].axis*input.getDevice(0)->getValuator(factory->rotationAxes[i].index);
	rotation*=factory->rotateFactor*getCurrentFrameTime();
	
	/* Convert linear joystick axes into translation vector: */
	Vector translation=Vector::zero;
	for(int i=0;i<factory->numTranslationAxes;++i)
		translation+=factory->translationAxes[i].axis*input.getDevice(0)->getValuator(factory->translationAxes[i].index);
	translation*=factory->translateFactor*getCurrentFrameTime();
	
	/* Calculate an incremental transformation based on the translation and rotation: */
	ONTransform deltaT=ONTransform::translate(translation);
	Point pos=joystick->getPosition();
	deltaT*=ONTransform::translateFromOriginTo(pos);
	deltaT*=ONTransform::rotate(ONTransform::Rotation::rotateScaledAxis(rotation));
	deltaT*=ONTransform::translateToOriginFrom(pos);
	
	if(isActive())
		{
		/* Update the navigation transformation: */
		deltaT.doInvert();
		setNavigationTransformation(NavTransform(deltaT)*getNavigationTransformation());
		}
	else
		{
		/* Update the virtual input device's transformation: */
		joystick->setTransformation(deltaT*joystick->getTransformation());
		}
	}

}
