/***********************************************************************
WiggleScreenshotTool - Class for tools to save save a sequence of
screenshots from different viewpoints to generate a "wigglegif."
Copyright (c) 2016 Oliver Kreylos

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

#include <Vrui/Tools/WiggleScreenshotTool.h>

#include <Misc/PrintInteger.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Images/Config.h>
#include <Vrui/Vrui.h>
#include <Vrui/VRWindow.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/***********************************************************
Methods of class WiggleScreenshotToolFactory::Configuration:
***********************************************************/

WiggleScreenshotToolFactory::Configuration::Configuration(void)
	 #if IMAGES_CONFIG_HAVE_PNG
	 :screenshotFileName("WiggleScreenshotTool.png"),
	 #else
	 :screenshotFileName("WiggleScreenshotTool.ppm"),
	 #endif
	 windowIndex(0),
	 numFrames(10),angleIncrement(Math::rad(Scalar(2.5)))
	{
	}

void WiggleScreenshotToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	screenshotFileName=cfs.retrieveString("./screenshotFileName",screenshotFileName);
	windowIndex=cfs.retrieveValue<int>("./windowIndex",windowIndex);
	numFrames=cfs.retrieveValue<unsigned int>("./numFrames",numFrames);
	angleIncrement=Math::rad(cfs.retrieveValue<Scalar>("./angleIncrement",Math::deg(angleIncrement)));
	}

void WiggleScreenshotToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeString("./screenshotFileName",screenshotFileName);
	cfs.storeValue<int>("./windowIndex",windowIndex);
	cfs.storeValue<unsigned int>("./numFrames",numFrames);
	cfs.storeValue<Scalar>("./angleIncrement",Math::deg(angleIncrement));
	}

/********************************************
Methods of class WiggleScreenshotToolFactory:
********************************************/

WiggleScreenshotToolFactory::WiggleScreenshotToolFactory(ToolManager& toolManager)
	:ToolFactory("WiggleScreenshotTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* utilityToolFactory=toolManager.loadClass("UtilityTool");
	utilityToolFactory->addChildClass(this);
	addParentClass(utilityToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	WiggleScreenshotTool::factory=this;
	}

WiggleScreenshotToolFactory::~WiggleScreenshotToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	WiggleScreenshotTool::factory=0;
	}

const char* WiggleScreenshotToolFactory::getName(void) const
	{
	return "Wiggle Screenshot";
	}

const char* WiggleScreenshotToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	return "Take Wiggle Screenshot";
	}

Tool* WiggleScreenshotToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new WiggleScreenshotTool(this,inputAssignment);
	}

void WiggleScreenshotToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveWiggleScreenshotToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("UtilityTool");
	}

extern "C" ToolFactory* createWiggleScreenshotToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	WiggleScreenshotToolFactory* wiggleScreenshotToolFactory=new WiggleScreenshotToolFactory(*toolManager);
	
	/* Return factory object: */
	return wiggleScreenshotToolFactory;
	}

extern "C" void destroyWiggleScreenshotToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/*********************************************
Static elements of class WiggleScreenshotTool:
*********************************************/

WiggleScreenshotToolFactory* WiggleScreenshotTool::factory=0;

/*************************************
Methods of class WiggleScreenshotTool:
*************************************/

WiggleScreenshotTool::WiggleScreenshotTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:UtilityTool(factory,inputAssignment),
	 configuration(WiggleScreenshotTool::factory->configuration),
	 window(0),
	 frameIndex(0)
	{
	}

void WiggleScreenshotTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override private configuration data from given configuration file section: */
	configuration.read(configFileSection);
	}

void WiggleScreenshotTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write private configuration data to given configuration file section: */
	configuration.write(configFileSection);
	}

void WiggleScreenshotTool::initialize(void)
	{
	if(isMaster())
		{
		/* Get the screenshot window: */
		window=getWindow(configuration.windowIndex);
		}
	}

const ToolFactory* WiggleScreenshotTool::getFactory(void) const
	{
	return factory;
	}

void WiggleScreenshotTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Screenshot button has just been pressed
		{
		/* Try activating this tool as a navigation tool if it is currently idle: */
		if(frameIndex==0&&activateNavigationTool(this))
			{
			/* Store the current navigation transformation: */
			initialNavTransform=getNavigationTransformation();
			
			/* Start the wiggle screenshot: */
			frameIndex=1;
			}
		}
	}

void WiggleScreenshotTool::frame(void)
	{
	/* Check if the tool is active: */
	if(frameIndex!=0)
		{
		if(frameIndex<=configuration.numFrames*2)
			{
			/* Calculate the navigation transformation for this frame: */
			Scalar angle;
			if(frameIndex<=configuration.numFrames)
				{
				/* Wiggle from left to right: */
				angle=(Scalar(frameIndex-1)-Math::div2(Scalar(configuration.numFrames-1)))*configuration.angleIncrement;
				}
			else
				{
				/* Wiggle from right to left: */
				angle=(Math::div2(Scalar(configuration.numFrames-1))-Scalar(frameIndex-configuration.numFrames-1))*configuration.angleIncrement;
				}
			NavTransform nav=NavTransform::rotateAround(getDisplayCenter(),Rotation::rotateAxis(getUpDirection(),angle));
			nav*=initialNavTransform;
			setNavigationTransformation(nav);
			
			if(window!=0)
				{
				/* Request a screenshot: */
				std::string::iterator extIt=configuration.screenshotFileName.end();
				for(std::string::iterator sfnIt=configuration.screenshotFileName.begin();sfnIt!=configuration.screenshotFileName.end();++sfnIt)
					if(*sfnIt=='.')
						extIt=sfnIt;
				std::string screenshotFileName(configuration.screenshotFileName.begin(),extIt);
				char index[5];
				char* iPtr=Misc::print(frameIndex-1,index+sizeof(index)-1);
				while(iPtr!=index)
					*(--iPtr)='0';
				screenshotFileName.append(index);
				screenshotFileName.append(extIt,configuration.screenshotFileName.end());
				window->requestScreenshot(screenshotFileName.c_str());
				}
			
			/* Go to the next frame: */
			++frameIndex;
			requestUpdate();
			}
		else
			{
			/* Stop the animation: */
			setNavigationTransformation(initialNavTransform);
			deactivateNavigationTool(this);
			frameIndex=0;
			}
		}
	}

}
