/***********************************************************************
SpaceBallTool - Class to abstract a raw SpaceBall relative 6-DOF device
into an absolute 6-DOF virtual input device.
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

#include <vector>
#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/SpaceBallTool.h>

namespace Vrui {

/*************************************
Methods of class SpaceBallToolFactory:
*************************************/

SpaceBallToolFactory::SpaceBallToolFactory(ToolManager& toolManager)
	:ToolFactory("SpaceBallTool",toolManager),
	 translateFactor(getInchFactor()),
	 rotateFactor(Scalar(1))
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,12);
	layout.setNumValuators(0,6);
	
	#if 0
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("Tool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	#endif
	
	/* Initialize toggle button emulation: */
	for(int i=0;i<12;++i)
		buttonToggleFlags[i]=false;
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	translateFactor=cfs.retrieveValue<Scalar>("./translateFactor",translateFactor);
	rotateFactor=cfs.retrieveValue<Scalar>("./rotateFactor",rotateFactor);
	typedef std::vector<int> IndexList;
	IndexList toggleButtonIndices=cfs.retrieveValue<IndexList>("./toggleButtonIndices",IndexList());
	for(IndexList::const_iterator tbiIt=toggleButtonIndices.begin();tbiIt!=toggleButtonIndices.end();++tbiIt)
		{
		if(*tbiIt>=0&&*tbiIt<12)
			buttonToggleFlags[*tbiIt]=true;
		else
			Misc::throwStdErr("SpaceBallTool: Button index out of valid range");
		}
	deviceGlyph.configure(cfs,"./deviceGlyphType","./deviceGlyphMaterial");
	
	/* Set tool class' factory pointer: */
	SpaceBallTool::factory=this;
	}

SpaceBallToolFactory::~SpaceBallToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	SpaceBallTool::factory=0;
	}

Tool* SpaceBallToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new SpaceBallTool(this,inputAssignment);
	}

void SpaceBallToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveSpaceBallToolDependencies(Plugins::FactoryManager<ToolFactory>&)
	{
	#if 0
	/* Load base classes: */
	manager.loadClass("Tool");
	#endif
	}

extern "C" ToolFactory* createSpaceBallToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	SpaceBallToolFactory* spaceBallToolFactory=new SpaceBallToolFactory(*toolManager);
	
	/* Return factory object: */
	return spaceBallToolFactory;
	}

extern "C" void destroySpaceBallToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/**************************************
Static elements of class SpaceBallTool:
**************************************/

SpaceBallToolFactory* SpaceBallTool::factory=0;

/******************************
Methods of class SpaceBallTool:
******************************/

SpaceBallTool::SpaceBallTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:Tool(factory,inputAssignment),
	 spaceBall(0)
	{
	/* Initialize toggle button states: */
	for(int i=0;i<12;++i)
		toggleButtonStates[i]=false;
	}

SpaceBallTool::~SpaceBallTool(void)
	{
	}

void SpaceBallTool::initialize(void)
	{
	/* Create a virtual input device to shadow the raw SpaceBall device: */
	spaceBall=addVirtualInputDevice("VirtualSpaceBall",12,0);
	getInputGraphManager()->getInputDeviceGlyph(spaceBall)=factory->deviceGlyph;
	
	/* Permanently grab the virtual input device: */
	getInputGraphManager()->grabInputDevice(spaceBall,this);
	}

void SpaceBallTool::deinitialize(void)
	{
	/* Release the virtual input device: */
	getInputGraphManager()->releaseInputDevice(spaceBall,this);
	
	/* Destroy the virtual input device: */
	getInputDeviceManager()->destroyInputDevice(spaceBall);
	}

const ToolFactory* SpaceBallTool::getFactory(void) const
	{
	return factory;
	}

void SpaceBallTool::buttonCallback(int,int buttonIndex,InputDevice::ButtonCallbackData* cbData)
	{
	if(factory->buttonToggleFlags[buttonIndex])
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

void SpaceBallTool::frame(void)
	{
	/* Convert linear SpaceBall axes into translation vector: */
	Vector translation;
	translation[0]=input.getDevice(0)->getValuator(input.getValuatorIndex(0,0));
	translation[1]=input.getDevice(0)->getValuator(input.getValuatorIndex(0,2));
	translation[2]=input.getDevice(0)->getValuator(input.getValuatorIndex(0,1));
	translation*=factory->translateFactor*getCurrentFrameTime();
	
	/* Convert rotational SpaceBall axes into rotation axis vector and rotation angle: */
	Vector scaledRotationAxis;
	scaledRotationAxis[0]=input.getDevice(0)->getValuator(input.getValuatorIndex(0,3));
	scaledRotationAxis[1]=input.getDevice(0)->getValuator(input.getValuatorIndex(0,5));
	scaledRotationAxis[2]=input.getDevice(0)->getValuator(input.getValuatorIndex(0,4));
	scaledRotationAxis*=factory->rotateFactor*getCurrentFrameTime();
	
	/* Calculate an incremental transformation based on the translation and rotation: */
	ONTransform deltaT=ONTransform::translate(translation);
	Point pos=spaceBall->getPosition();
	deltaT*=ONTransform::translateFromOriginTo(pos);
	deltaT*=ONTransform::rotate(ONTransform::Rotation::rotateScaledAxis(scaledRotationAxis));
	deltaT*=ONTransform::translateToOriginFrom(pos);
	
	/* Update the virtual input device's transformation: */
	spaceBall->setTransformation(deltaT*spaceBall->getTransformation());
	}

}
