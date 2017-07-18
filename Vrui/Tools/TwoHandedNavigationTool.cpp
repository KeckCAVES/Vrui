/***********************************************************************
TwoHandedNavigationTool - Class encapsulating the behaviour of the old
famous Vrui two-handed navigation tool.
Copyright (c) 2004-2017 Oliver Kreylos

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

#include <Vrui/Tools/TwoHandedNavigationTool.h>

#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/***********************************************
Methods of class TwoHandedNavigationToolFactory:
***********************************************/

TwoHandedNavigationToolFactory::TwoHandedNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("TwoHandedNavigationTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(2);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("NavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Set tool class' factory pointer: */
	TwoHandedNavigationTool::factory=this;
	}

TwoHandedNavigationToolFactory::~TwoHandedNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	TwoHandedNavigationTool::factory=0;
	}

const char* TwoHandedNavigationToolFactory::getName(void) const
	{
	return "Ambidextrous 6-DOF + Scaling";
	}

const char* TwoHandedNavigationToolFactory::getButtonFunction(int) const
	{
	return "Grab Space / Zoom";
	}

Tool* TwoHandedNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new TwoHandedNavigationTool(this,inputAssignment);
	}

void TwoHandedNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveTwoHandedNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("NavigationTool");
	}

extern "C" ToolFactory* createTwoHandedNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	TwoHandedNavigationToolFactory* twoHandedNavigationToolFactory=new TwoHandedNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return twoHandedNavigationToolFactory;
	}

extern "C" void destroyTwoHandedNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/************************************************
Static elements of class TwoHandedNavigationTool:
************************************************/

TwoHandedNavigationToolFactory* TwoHandedNavigationTool::factory=0;

/****************************************
Methods of class TwoHandedNavigationTool:
****************************************/

TwoHandedNavigationTool::TwoHandedNavigationTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(factory,inputAssignment),
	 navigationMode(IDLE)
	{
	}

const ToolFactory* TwoHandedNavigationTool::getFactory(void) const
	{
	return factory;
	}

void TwoHandedNavigationTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Act depending on this tool's current state: */
		switch(navigationMode)
			{
			case IDLE:
				/* Try activating this tool: */
				if(activate())
					{
					/* Initialize moving state: */
					movingButtonSlotIndex=buttonSlotIndex;
					movingTransform=Geometry::invert(getButtonDeviceTransformation(movingButtonSlotIndex));
					movingTransform*=getNavigationTransformation();
					
					/* Go from IDLE to MOVING mode: */
					navigationMode=MOVING;
					}
				break;
			
			case MOVING:
				/* Check if the correct button has been pressed: */
				if(buttonSlotIndex!=movingButtonSlotIndex)
					{
					/* Get the positions of the two button devices in navigational space: */
					Point navPoss[2];
					for(int i=0;i<2;++i)
						navPoss[i]=getInverseNavigationTransformation().transform(getButtonDevicePosition(i));
					
					/* Check if the two points are distinct from each other: */
					Scalar navDist=Geometry::dist(navPoss[0],navPoss[1]);
					if(navDist!=Scalar(0))
						{
						/* Calculate the navigation-space center, axis direction, and a normal vector: */
						Point navCenter=Geometry::mid(navPoss[0],navPoss[1]);
						Vector navAxis=navPoss[1]-navPoss[0];
						Vector navNormal=Geometry::normal(navAxis);
						
						/* Calculate a normalizing post-scale transformation that maps the device positions to (-0.5, 0, 0) and (0.5, 0, 0), and the normal to (0, 1, 0): */
						postScaleTransform=NavTrackerState::rotate(Geometry::invert(Rotation::fromBaseVectors(navAxis,navNormal)));
						postScaleTransform*=NavTrackerState::scale(Scalar(1)/navDist);
						postScaleTransform*=NavTrackerState::translateToOriginFrom(navCenter);
						
						/* Transform the normal vector to physical space: */
						physNormal=getNavigationTransformation().transform(navNormal);
						
						/* Get the current device orientations: */
						for(int i=0;i<2;++i)
							prevDevOrientations[i]=getButtonDeviceTransformation(i).getRotation();
						
						/* Go from MOVING to SCALING mode: */
						navigationMode=SCALING;
						}
					}
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
				/* Initialize moving state: */
				movingButtonSlotIndex=1-buttonSlotIndex;
				movingTransform=Geometry::invert(getButtonDeviceTransformation(movingButtonSlotIndex));
				movingTransform*=getNavigationTransformation();
				
				/* Go from SCALING to MOVING mode: */
				navigationMode=MOVING;
				break;
			
			case MOVING:
				/* Check if the correct button has been released: */
				if(buttonSlotIndex==movingButtonSlotIndex)
					{
					/* Deactivate this tool: */
					deactivate();
					
					/* Go from MOVING to IDLE mode: */
					navigationMode=IDLE;
					}
				break;
			
			default:
				/* This shouldn't happen; just ignore the event */
				break;
			}
		}
	}

void TwoHandedNavigationTool::frame(void)
	{
	/* Act depending on this tool's current state: */
	switch(navigationMode)
		{
		case IDLE:
			/* Do nothing */
			break;
		
		case MOVING:
			{
			/* Compose the new navigation transformation: */
			NavTrackerState navigation=getButtonDeviceTransformation(movingButtonSlotIndex);
			navigation*=movingTransform;
			
			/* Update Vrui's navigation transformation: */
			setNavigationTransformation(navigation);
			break;
			}
		
		case SCALING:
			{
			/* Get the devices' positions and average incremental rotation in physical space: */
			Point physPoss[2];
			Vector physRotAxis=Vector::zero;
			for(int i=0;i<2;++i)
				{
				physPoss[i]=getButtonDevicePosition(i);
				const Rotation& devOrientation=getButtonDeviceTransformation(i).getRotation();
				physRotAxis+=(devOrientation*Geometry::invert(prevDevOrientations[i])).getScaledAxis();
				prevDevOrientations[i]=devOrientation;
				}
			physRotAxis*=Scalar(0.5);
			Vector physAxis=physPoss[1]-physPoss[0];
			Scalar physLen2=physAxis.sqr();
			
			/* Align the incremental rotation with the physical axis: */
			physRotAxis=physAxis*((physRotAxis*physAxis)/physLen2);
			
			/* Rotate the physical-space normal vector with the along-axis incremental rotation and orthogonalize it with the axis: */
			physNormal=Rotation::rotateScaledAxis(physRotAxis).transform(physNormal);
			physNormal.orthogonalize(physAxis);
			
			/* Construct the navigation transformation to align the normalized navigation frame with the physical axis and normal: */
			NavTrackerState navigation=NavTrackerState::translateFromOriginTo(Geometry::mid(physPoss[0],physPoss[1]));
			navigation*=NavTrackerState::rotate(Rotation::fromBaseVectors(physAxis,physNormal));
			navigation*=NavTrackerState::scale(Math::sqrt(physLen2));
			navigation*=postScaleTransform;
			
			/* Update Vrui's navigation transformation: */
			setNavigationTransformation(navigation);
			break;
			}
		}
	}

}
