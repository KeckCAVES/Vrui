/***********************************************************************
WandNavigationTool - Class encapsulating the navigation behaviour of a
classical CAVE wand.
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
#include <Math/Math.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/WandNavigationTool.h>

namespace Vrui {

/******************************************
Methods of class WandNavigationToolFactory:
******************************************/

WandNavigationToolFactory::WandNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("WandNavigationTool",toolManager),
	 scaleFactor(getInchFactor()*Scalar(12))
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,2);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("NavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	scaleFactor=cfs.retrieveValue<Scalar>("./scaleFactor",scaleFactor);
	
	/* Set tool class' factory pointer: */
	WandNavigationTool::factory=this;
	}

WandNavigationToolFactory::~WandNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	WandNavigationTool::factory=0;
	}

Tool* WandNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new WandNavigationTool(this,inputAssignment);
	}

void WandNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveWandNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("NavigationTool");
	}

extern "C" ToolFactory* createWandNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	WandNavigationToolFactory* wandNavigationToolFactory=new WandNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return wandNavigationToolFactory;
	}

extern "C" void destroyWandNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/*******************************************
Static elements of class WandNavigationTool:
*******************************************/

WandNavigationToolFactory* WandNavigationTool::factory=0;

/***********************************
Methods of class WandNavigationTool:
***********************************/

WandNavigationTool::WandNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(factory,inputAssignment),
	 navigationMode(IDLE)
	{
	}

const ToolFactory* WandNavigationTool::getFactory(void) const
	{
	return factory;
	}

void WandNavigationTool::buttonCallback(int,int buttonIndex,InputDevice::ButtonCallbackData* cbData)
	{
	/* Get pointer to input device: */
	InputDevice* device=input.getDevice(0);
	
	/* Process based on which button was pressed: */
	switch(buttonIndex)
		{
		case 0:
			if(cbData->newButtonState) // Button has just been pressed
				{
				/* Act depending on this tool's current state: */
				switch(navigationMode)
					{
					case IDLE:
						/* Try activating this tool: */
						if(activate())
							{
							/* Initialize the navigation transformations: */
							preScale=Geometry::invert(device->getTransformation());
							preScale*=getNavigationTransformation();
							
							/* Go from IDLE to MOVING mode: */
							navigationMode=MOVING;
							}
						break;
					
					case SCALING_PAUSED:
						/* Determine the scaling center and direction: */
						scalingCenter=device->getPosition();
						scalingDirection=device->getRayDirection();
						initialScale=scalingCenter*scalingDirection;
						
						/* Initialize the transformation parts: */
						preScale=NavTrackerState::translateFromOriginTo(scalingCenter);
						postScale=NavTrackerState::translateToOriginFrom(scalingCenter);
						postScale*=getNavigationTransformation();
						
						/* Go from SCALING_PAUSED to SCALING mode: */
						navigationMode=SCALING;
						break;
					
					default:
						/* This shouldn't happen; just ignore the event */
						break;
					}
				}
			else // Button has just been released
				{
				/* Act depending on this tool's current state: */
				switch(navigationMode)
					{
					case SCALING:
						/* Pause scaling until button is pressed again: */
						
						/* Go from SCALING to SCALING_PAUSED mode: */
						navigationMode=SCALING_PAUSED;
						break;
					
					case MOVING:
						/* Deactivate this tool: */
						deactivate();
						
						/* Go from MOVING to IDLE mode: */
						navigationMode=IDLE;
						break;
					
					default:
						/* This shouldn't happen; just ignore the event */
						break;
					}
				}
			break;
		
		case 1:
			if(cbData->newButtonState) // Button has just been pressed
				{
				/* Act depending on this tool's current state: */
				switch(navigationMode)
					{
					case IDLE:
						/* Ignore the event and pass it through to the tool listening at the output: */
						break;
					
					case MOVING:
						/* Determine the scaling center and direction: */
						scalingCenter=device->getPosition();
						scalingDirection=device->getRayDirection();
						initialScale=scalingCenter*scalingDirection;
						
						/* Initialize the transformation parts: */
						preScale=NavTrackerState::translateFromOriginTo(scalingCenter);
						postScale=NavTrackerState::translateToOriginFrom(scalingCenter);
						postScale*=getNavigationTransformation();
						
						/* Go from MOVING to SCALING mode: */
						navigationMode=SCALING;
						break;
					
					default:
						/* This shouldn't happen; just ignore the event */
						break;
					}
				}
			else // Button has just been released
				{
				/* Act depending on this tool's current state: */
				switch(navigationMode)
					{
					case SCALING:
						/* Initialize the transformation parts: */
						preScale=Geometry::invert(device->getTransformation());
						preScale*=getNavigationTransformation();
						
						/* Go from SCALING to MOVING mode: */
						navigationMode=MOVING;
						break;
					
					case SCALING_PAUSED:
						/* Deactivate this tool: */
						deactivate();
						
						/* Go from SCALING_PAUSED to IDLE mode: */
						navigationMode=IDLE;
						break;
					
					default:
						/* This shouldn't happen; just ignore the event */
						break;
					}
				}
			break;
		}
	}

void WandNavigationTool::frame(void)
	{
	/* Get pointer to input device: */
	InputDevice* device=input.getDevice(0);
	
	/* Act depending on this tool's current state: */
	switch(navigationMode)
		{
		case IDLE:
			/* Do nothing */
			break;
		
		case MOVING:
			{
			/* Compose the new navigation transformation: */
			NavTrackerState navigation=device->getTransformation();
			navigation*=preScale;
			
			/* Update Vrui's navigation transformation: */
			setNavigationTransformation(navigation);
			break;
			}
		
		case SCALING:
			{
			/* Compose the new navigation transformation: */
			NavTrackerState navigation=preScale;
			Scalar currentScale=device->getPosition()*scalingDirection-initialScale;
			navigation*=NavTrackerState::scale(Math::exp(currentScale/factory->scaleFactor));
			navigation*=postScale;
			
			/* Update Vrui's navigation transformation: */
			setNavigationTransformation(navigation);
			break;
			}
		
		case SCALING_PAUSED:
			/* Do nothing */
			break;
		}
	}

}
