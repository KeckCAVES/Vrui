/***********************************************************************
SixAxisTransformTool - Class to convert an input device with six
valuators into a virtual 6-DOF input device.
Copyright (c) 2010-2011 Oliver Kreylos

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

#include <Vrui/Tools/SixAxisTransformTool.h>

#include <vector>
#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/GeometryValueCoders.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/********************************************
Methods of class SixAxisTransformToolFactory:
********************************************/

SixAxisTransformToolFactory::SixAxisTransformToolFactory(ToolManager& toolManager)
	:ToolFactory("SixAxisTransformTool",toolManager),
	 followDisplayCenter(false),homePosition(getDisplayCenter())
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1,true);
	layout.setNumValuators(6,true);
	
	/* Insert class into class hierarchy: */
	ToolFactory* transformToolFactory=toolManager.loadClass("TransformTool");
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	if(cfs.hasTag("./homePosition"))
		{
		/* Read the home position: */
		homePosition=cfs.retrieveValue<Point>("./homePosition");
		}
	else
		{
		/* If no home position configured, use the current display center: */
		followDisplayCenter=true;
		}
	
	typedef std::vector<Vector> VectorList;
	
	/* Initialize translation vectors: */
	Scalar translateFactor=cfs.retrieveValue<Scalar>("./translateFactor",getDisplaySize()/Scalar(3));
	VectorList translationVectors;
	for(int i=0;i<3;++i)
		{
		Vector t=Vector::zero;
		t[i]=Scalar(1);
		translationVectors.push_back(t);
		}
	translationVectors=cfs.retrieveValue<VectorList>("./translationVectors",translationVectors);
	if(translationVectors.size()!=3)
		Misc::throwStdErr("SixAxisTransformToolFactory: wrong number of translation vectors; got %u, needed 3",(unsigned int)translationVectors.size());
	
	for(int i=0;i<3;++i)
		translations[i]=translationVectors[i]*translateFactor;
	
	/* Initialize rotation axes: */
	Scalar rotateFactor=Math::rad(cfs.retrieveValue<Scalar>("./rotateFactor",Scalar(180)));
	VectorList scaledRotationAxes;
	for(int i=0;i<3;++i)
		{
		Vector r=Vector::zero;
		r[i]=Scalar(1);
		scaledRotationAxes.push_back(r);
		}
	scaledRotationAxes=cfs.retrieveValue<VectorList>("./scaledRotationAxes",scaledRotationAxes);
	if(scaledRotationAxes.size()!=3)
		Misc::throwStdErr("SixAxisTransformToolFactory: wrong number of rotation axes; got %u, needed 3",(unsigned int)scaledRotationAxes.size());
	
	for(int i=0;i<3;++i)
		rotations[i]=scaledRotationAxes[i]*rotateFactor;
	
	/* Configure the device glyph: */
	deviceGlyph.configure(cfs,"./deviceGlyphType","./deviceGlyphMaterial");
	if(!deviceGlyph.isEnabled())
		{
		deviceGlyph.setGlyphType(Glyph::CONE);
		deviceGlyph.enable();
		}
	
	/* Set tool class' factory pointer: */
	SixAxisTransformTool::factory=this;
	}

SixAxisTransformToolFactory::~SixAxisTransformToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	SixAxisTransformTool::factory=0;
	}

const char* SixAxisTransformToolFactory::getName(void) const
	{
	return "Six-Axis Device";
	}

const char* SixAxisTransformToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	if(buttonSlotIndex==0)
		return "Home";
	else
		return "Forwarded Button";
	}

const char* SixAxisTransformToolFactory::getValuatorFunction(int valuatorSlotIndex) const
	{
	switch(valuatorSlotIndex)
		{
		case 0:
			return "Translate Vector 0";
		
		case 1:
			return "Translate Vector 1";
		
		case 2:
			return "Translate Vector 2";
		
		case 3:
			return "Rotate Axis 0";
		
		case 4:
			return "Rotate Axis 1";
		
		case 5:
			return "Rotate Axis 2";
		
		default:
			return "Forwarded Valuator";
		}
	}

Tool* SixAxisTransformToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new SixAxisTransformTool(this,inputAssignment);
	}

void SixAxisTransformToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveSixAxisTransformToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createSixAxisTransformToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	SixAxisTransformToolFactory* sixAxisTransformToolFactory=new SixAxisTransformToolFactory(*toolManager);
	
	/* Return factory object: */
	return sixAxisTransformToolFactory;
	}

extern "C" void destroySixAxisTransformToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/*********************************************
Static elements of class SixAxisTransformTool:
*********************************************/

SixAxisTransformToolFactory* SixAxisTransformTool::factory=0;

/*************************************
Methods of class SixAxisTransformTool:
*************************************/

SixAxisTransformTool::SixAxisTransformTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:TransformTool(sFactory,inputAssignment)
	{
	/* Set the number of private buttons: */
	numPrivateButtons=1;
	}

void SixAxisTransformTool::initialize(void)
	{
	/* Let the base class do its thing: */
	TransformTool::initialize();
	
	/* Set the virtual input device's glyph: */
	getInputGraphManager()->getInputDeviceGlyph(transformedDevice)=factory->deviceGlyph;
	
	/* Initialize the virtual input device's position: */
	transformedDevice->setTransformation(TrackerState::translateFromOriginTo(factory->followDisplayCenter?getDisplayCenter():factory->homePosition));
	transformedDevice->setDeviceRayDirection(getForwardDirection());
	}

const ToolFactory* SixAxisTransformTool::getFactory(void) const
	{
	return factory;
	}

void SixAxisTransformTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	if(buttonSlotIndex==0)
		{
		if(cbData->newButtonState) // Button has just been pressed
			{
			/* Reset the transformed device to the home position: */
			transformedDevice->setTransformation(TrackerState::translateFromOriginTo(factory->followDisplayCenter?getDisplayCenter():factory->homePosition));
			}
		}
	else
		{
		/* Let transform tool handle it: */
		TransformTool::buttonCallback(buttonSlotIndex,cbData);
		}
	}

void SixAxisTransformTool::frame(void)
	{
	/* Assemble translation from translation vectors and current valuator values: */
	Vector translation=Vector::zero;
	for(int i=0;i<3;++i)
		translation+=factory->translations[i]*Scalar(getValuatorState(i));
	translation*=getCurrentFrameTime();
	
	/* Assemble rotation from scaled rotation axes and current valuator values: */
	Vector rotation=Vector::zero;
	for(int i=0;i<3;++i)
		rotation+=factory->rotations[i]*Scalar(getValuatorState(3+i));
	rotation*=getCurrentFrameTime();
	
	/* Calculate an incremental transformation for the virtual input device: */
	ONTransform deltaT=ONTransform::translate(translation);
	Point pos=transformedDevice->getPosition();
	deltaT*=ONTransform::translateFromOriginTo(pos);
	deltaT*=ONTransform::rotate(ONTransform::Rotation::rotateScaledAxis(rotation));
	deltaT*=ONTransform::translateToOriginFrom(pos);
	
	/* Update the virtual input device's transformation: */
	deltaT*=transformedDevice->getTransformation();
	deltaT.renormalize();
	transformedDevice->setTransformation(deltaT);
	
	/* Request another frame if the input device has moved: */
	if(translation!=Vector::zero||rotation!=Vector::zero)
		{
		/* Request another frame: */
		scheduleUpdate(getApplicationTime()+1.0/125.0);
		}
	}

}
