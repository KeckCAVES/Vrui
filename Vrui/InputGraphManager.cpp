/***********************************************************************
InputGraphManager - Class to maintain the bipartite input device / tool
graph formed by tools being assigned to input devices, and input devices
in turn being grabbed by tools.
Copyright (c) 2004-2005 Oliver Kreylos

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

#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/Ray.h>
#include <Vrui/InputDevice.h>
#include <Vrui/VirtualInputDevice.h>
#include <Vrui/ToolInputAssignment.h>
#include <Vrui/Tools/Tool.h>
#include <Vrui/Vrui.h>

#include <Vrui/InputGraphManager.h>

namespace Vrui {

/**********************************
Methods of class InputGraphManager:
**********************************/

void InputGraphManager::linkInputDevice(InputGraphManager::GraphInputDevice* gid)
	{
	/* Link the graph input device to its graph level: */
	gid->levelPred=0;
	gid->levelSucc=deviceLevels[gid->level];
	if(deviceLevels[gid->level]!=0)
		deviceLevels[gid->level]->levelPred=gid;
	deviceLevels[gid->level]=gid;	
	}

void InputGraphManager::unlinkInputDevice(InputGraphManager::GraphInputDevice* gid)
	{
	/* Unlink the graph input device from its graph level: */
	if(gid->levelPred!=0)
		gid->levelPred->levelSucc=gid->levelSucc;
	else
		deviceLevels[gid->level]=gid->levelSucc;
	if(gid->levelSucc!=0)
		gid->levelSucc->levelPred=gid->levelPred;
	}

void InputGraphManager::linkTool(InputGraphManager::GraphTool* gt)
	{
	/* Link the graph tool to its graph level: */
	gt->levelPred=0;
	gt->levelSucc=toolLevels[gt->level];
	if(toolLevels[gt->level]!=0)
		toolLevels[gt->level]->levelPred=gt;
	toolLevels[gt->level]=gt;	
	}

void InputGraphManager::unlinkTool(InputGraphManager::GraphTool* gt)
	{
	/* Unlink the graph tool from its graph level: */
	if(gt->levelPred!=0)
		gt->levelPred->levelSucc=gt->levelSucc;
	else
		toolLevels[gt->level]=gt->levelSucc;
	if(gt->levelSucc!=0)
		gt->levelSucc->levelPred=gt->levelPred;
	}

void InputGraphManager::growInputGraph(int level)
	{
	/* Check whether the max graph level needs to be adjusted: */
	if(maxGraphLevel<level)
		{
		/* Set the new max graph level: */
		maxGraphLevel=level;
		
		/* Initialize the new levels in the input graph: */
		while(int(deviceLevels.size())<=maxGraphLevel)
			deviceLevels.push_back(0);
		while(int(toolLevels.size())<=maxGraphLevel)
			toolLevels.push_back(0);
		}
	}

void InputGraphManager::shrinkInputGraph(void)
	{
	/* Check whether there are empty levels at the end of the graph: */
	while(deviceLevels[maxGraphLevel]==0&&toolLevels[maxGraphLevel]==0)
		--maxGraphLevel;
	}

void InputGraphManager::updateInputGraph(void)
	{
	/* Iterate through all graph levels and move all input devices and tools to their correct positions: */
	for(int level=0;level<=maxGraphLevel;++level)
		{
		/* Check all input devices: */
		GraphInputDevice* gid=deviceLevels[level];
		while(gid!=0)
			{
			GraphInputDevice* succ=gid->levelSucc;
			
			/* Ensure that ungrabbed devices are in level 0, and grabbed devices are exactly one level above their grabbers: */
			if(gid->grabber==0&&gid->level!=0)
				{
				/* Move the input device to level 0: */
				unlinkInputDevice(gid);
				gid->level=0;
				linkInputDevice(gid);
				}
			else if(gid->grabber!=0&&gid->level!=gid->grabber->level+1)
				{
				/* Move the input device to the level after its grabbing tool: */
				unlinkInputDevice(gid);
				gid->level=gid->grabber->level+1;
				growInputGraph(gid->level);
				linkInputDevice(gid);
				}
			
			gid=succ;
			}
		
		/* Check all tools: */
		GraphTool* gt=toolLevels[level];
		while(gt!=0)
			{
			GraphTool* succ=gt->levelSucc;
			
			/* Get tool's input device assignment: */
			const ToolInputLayout& til=gt->tool->getLayout();
			const ToolInputAssignment& tia=gt->tool->getInputAssignment();
			
			/* Determine the maximal graph level of all input devices the tool is assigned to: */
			int maxDeviceLevel=0;
			for(int i=0;i<til.getNumDevices();++i)
				{
				/* Get pointer to graph input device: */
				GraphInputDevice* gid=deviceMap.getEntry(tia.getDevice(i)).getDest();
				
				if(maxDeviceLevel<gid->level)
					maxDeviceLevel=gid->level;
				}
			
			/* Ensure that a tool is on the same level as its highest input device: */
			if(gt->level!=maxDeviceLevel)
				{
				/* Move the tool to the same level as its highest input device: */
				unlinkTool(gt);
				gt->level=maxDeviceLevel;
				linkTool(gt);
				}
			
			gt=succ;
			}
		}
	
	/* Shrink the input graph: */
	shrinkInputGraph();
	}

InputGraphManager::InputGraphManager(GlyphRenderer* sGlyphRenderer,VirtualInputDevice* sVirtualInputDevice)
	:glyphRenderer(sGlyphRenderer),virtualInputDevice(sVirtualInputDevice),
	 deviceMap(101),toolMap(101),
	 maxGraphLevel(-1)
	{
	/* Initialize the input device manager fake graph tool: */
	inputDeviceManager.tool=0;
	inputDeviceManager.level=-1;
	inputDeviceManager.levelPred=0;
	inputDeviceManager.levelSucc=0;
	}

InputGraphManager::~InputGraphManager(void)
	{
	/* Delete all graph input devices and tools: */
	for(int i=0;i<=maxGraphLevel;++i)
		{
		/* Delete all graph input devices: */
		GraphInputDevice* gid=deviceLevels[i];
		while(gid!=0)
			{
			GraphInputDevice* succ=gid->levelSucc;
			delete gid;
			gid=succ;
			}
		
		/* Delete all graph tools: */
		GraphTool* gt=toolLevels[i];
		while(gt!=0)
			{
			GraphTool* succ=gt->levelSucc;
			delete gt;
			gt=succ;
			}
		}
	}

void InputGraphManager::addInputDevice(InputDevice* newDevice)
	{
	/* Disable all callbacks for the device: */
	newDevice->disableCallbacks();
	
	/* Add the new device to level 0 of the input graph: */
	GraphInputDevice* newGid=new GraphInputDevice;
	newGid->device=newDevice;
	newGid->level=0;
	newGid->navigational=false;
	growInputGraph(0);
	linkInputDevice(newGid);
	newGid->grabber=0; // Mark the device as ungrabbed
	
	/* Add the new graph device to the device map: */
	deviceMap.setEntry(DeviceMap::Entry(newDevice,newGid));
	}

bool InputGraphManager::isNavigational(InputDevice* device) const
	{
	/* Get pointer to the graph input device: */
	const GraphInputDevice* gid=deviceMap.getEntry(device).getDest();
	
	/* Return the device's navigation flag: */
	return gid->navigational;
	}

void InputGraphManager::setNavigational(InputDevice* device,bool newNavigational)
	{
	/* Get pointer to the graph input device: */
	GraphInputDevice* gid=deviceMap.getEntry(device).getDest();
	
	if(newNavigational)
		{
		/* Calculate the transformation from navigation coordinates to the device's current coordinates: */
		gid->fromNavTransform=device->getTransformation();
		gid->fromNavTransform.leftMultiply(getInverseNavigationTransformation());
		}
	
	/* Set the device's navigation flag: */
	gid->navigational=newNavigational;
	}

Glyph& InputGraphManager::getInputDeviceGlyph(InputDevice* device)
	{
	/* Get pointer to the graph input device: */
	GraphInputDevice* gid=deviceMap.getEntry(device).getDest();
	
	/* Return the device's glyph: */
	return gid->deviceGlyph;
	}

InputDevice* InputGraphManager::getFirstInputDevice(void)
	{
	/* Search for the first ungrabbed device in graph level 0: */
	InputDevice* result=0;
	for(GraphInputDevice* gid=deviceLevels[0];gid!=0;gid=gid->levelSucc)
		if(gid->grabber==0)
			{
			result=gid->device;
			break;
			}
	return result;
	}

InputDevice* InputGraphManager::getNextInputDevice(InputDevice* device)
	{
	/* Bail out if the device pointer is invalid: */
	if(device==0)
		return 0;
	
	/* Get the graph input device corresponding to the given device: */
	GraphInputDevice* gid=deviceMap.getEntry(device).getDest();
	
	/* Search for the next ungrabbed device: */
	InputDevice* result=0;
	for(gid=gid->levelSucc;gid!=0;gid=gid->levelSucc)
		if(gid->grabber==0)
			{
			result=gid->device;
			break;
			}
	return result;
	}

InputDevice* InputGraphManager::findInputDevice(const Point& position,bool ungrabbedOnly)
	{
	InputDevice* result=0;
	Scalar gs=Scalar(0.575)*glyphRenderer->getGlyphSize();
	int maxSearchLevel=ungrabbedOnly?0:maxGraphLevel;
	
	/* Check all input devices in all relevant graph levels: */
	for(int level=0;level<=maxSearchLevel;++level)
		for(GraphInputDevice* gid=deviceLevels[level];gid!=0;gid=gid->levelSucc)
			if(gid->grabber==0)
				{
				if(virtualInputDevice->pick(gid->device,position))
					{
					/* Remember the device and stop searching: */
					result=gid->device;
					goto foundIt;
					}
				}
			else if(!ungrabbedOnly)
				{
				/* Check if the given position is inside the input device's glyph: */
				Point dp=gid->device->getTransformation().inverseTransform(position);
				bool inside=true;
				for(int i=0;i<3&&inside;++i)
					inside=Math::abs(dp[i])<=gs;
				
				if(inside)
					{
					/* Remember the device and stop searching: */
					result=gid->device;
					goto foundIt;
					}
				}
	foundIt:
	
	return result;
	}

InputDevice* InputGraphManager::findInputDevice(const Ray& ray,bool ungrabbedOnly)
	{
	InputDevice* result=0;
	Scalar gs=Scalar(0.575)*glyphRenderer->getGlyphSize();
	int maxSearchLevel=ungrabbedOnly?0:maxGraphLevel;
	Scalar lambdaMin=Math::Constants<Scalar>::max;
	
	/* Check all input devices in all relevant graph levels: */
	for(int level=0;level<=maxSearchLevel;++level)
		for(GraphInputDevice* gid=deviceLevels[level];gid!=0;gid=gid->levelSucc)
			if(gid->grabber==0)
				{
				Scalar lambda=virtualInputDevice->pick(gid->device,ray);
				if(lambdaMin>lambda)
					{
					result=gid->device;
					lambdaMin=lambda;
					}
				}
			else if(!ungrabbedOnly)
				{
				Ray r=ray;
				r.inverseTransform(gid->device->getTransformation());
				
				Scalar lMin=Scalar(0);
				Scalar lMax=Math::Constants<Scalar>::max;
				for(int i=0;i<3;++i)
					{
					Scalar l1,l2;
					if(r.getDirection()[i]<Scalar(0))
						{
						l1=(gs-r.getOrigin()[i])/r.getDirection()[i];
						l2=(-gs-r.getOrigin()[i])/r.getDirection()[i];
						}
					else if(r.getDirection()[i]>Scalar(0))
						{
						l1=(-gs-r.getOrigin()[i])/r.getDirection()[i];
						l2=(gs-r.getOrigin()[i])/r.getDirection()[i];
						}
					else if(-gs<=r.getOrigin()[i]&&r.getOrigin()[i]<gs)
						{
						l1=Scalar(0);
						l2=Math::Constants<Scalar>::max;
						}
					else
						l1=l2=Scalar(-1);
					if(lMin<l1)
						lMin=l1;
					if(lMax>l2)
						lMax=l2;
					}
				
				if(lMin<lMax&&lMin<lambdaMin)
					{
					result=gid->device;
					lambdaMin=lMin;
					}
				}
	
	return result;
	}

bool InputGraphManager::grabInputDevice(InputDevice* device,Tool* grabber)
	{
	/* Get pointer to the graph input device: */
	GraphInputDevice* gid=deviceMap.getEntry(device).getDest();
	
	/* Bail out if the device is already grabbed: */
	if(gid->grabber!=0)
		return false;
	
	/* Get pointer to the graph tool: */
	GraphTool* gt=grabber!=0?toolMap.getEntry(grabber).getDest():&inputDeviceManager;
	
	/* Mark the input device as grabbed: */
	gid->grabber=gt;
	
	/* Update the input graph: */
	updateInputGraph();
	
	return true;
	}

void InputGraphManager::releaseInputDevice(InputDevice* device,Tool* grabber)
	{
	/* Get pointer to the graph input device: */
	GraphInputDevice* gid=deviceMap.getEntry(device).getDest();
	
	/* Get pointer to the graph tool: */
	GraphTool* gt=grabber!=0?toolMap.getEntry(grabber).getDest():&inputDeviceManager;
	
	/* Bail out if the device is not grabbed by the self-proclaimed grabber: */
	if(gid->grabber!=gt)
		return;
	
	/* Check if the device is in navigational mode: */
	if(gid->navigational)
		{
		/* Update the transformation from navigation coordinates to the device's current coordinates: */
		gid->fromNavTransform=device->getTransformation();
		gid->fromNavTransform.leftMultiply(getInverseNavigationTransformation());
		}
	
	/* Release the device grab: */
	gid->grabber=0;
	
	/* Update the input graph: */
	updateInputGraph();
	}

void InputGraphManager::removeInputDevice(InputDevice* device)
	{
	/* Find the device's entry in the device map: */
	DeviceMap::Iterator dIt=deviceMap.findEntry(device);
	
	/* Get pointer to the graph input device: */
	GraphInputDevice* gid=dIt->getDest();
	
	/* Remove the graph input device from its graph level: */
	unlinkInputDevice(gid);
	
	/* Remove the input device from the device map: */
	deviceMap.removeEntry(dIt);
	
	/* Delete the graph input device: */
	delete gid;
	
	/* Shrink the input graph: */
	shrinkInputGraph();
	}

void InputGraphManager::addTool(Tool* newTool)
	{
	/* Get tool's input device assignment: */
	const ToolInputLayout& til=newTool->getLayout();
	const ToolInputAssignment& tia=newTool->getInputAssignment();
	
	/* Determine the maximal graph level of all input devices the tool is assigned to: */
	int maxDeviceLevel=0;
	for(int i=0;i<til.getNumDevices();++i)
		{
		/* Get pointer to graph input device: */
		GraphInputDevice* gid=deviceMap.getEntry(tia.getDevice(i)).getDest();
		
		if(maxDeviceLevel<gid->level)
			maxDeviceLevel=gid->level;
		}
	
	/* Add the new tool to level maxDeviceLevel of the input graph: */
	GraphTool* newGt=new GraphTool;
	newGt->tool=newTool;
	newGt->level=maxDeviceLevel;
	linkTool(newGt);
	
	/* Add the new graph tool to the tool map: */
	toolMap.setEntry(ToolMap::Entry(newTool,newGt));
	}

void InputGraphManager::removeTool(Tool* tool)
	{
	/* Find the tool's entry in the tool map: */
	ToolMap::Iterator tIt=toolMap.findEntry(tool);
	
	/* Get pointer to the graph tool: */
	GraphTool* gt=tIt->getDest();
	
	/* Remove the graph tool from its graph level: */
	unlinkTool(gt);
	
	/* Remove the tool from the tool map: */
	toolMap.removeEntry(tIt);
	
	/* Delete the graph tool: */
	delete gt;
	}

void InputGraphManager::update(void)
	{
	/* Set the transformations of ungrabbed navigational devices in the first graph level: */
	for(GraphInputDevice* gid=deviceLevels[0];gid!=0;gid=gid->levelSucc)
		if(gid->navigational&&gid->grabber==0)
			{
			/* Set the device's transformation: */
			NavTrackerState transform=getNavigationTransformation();
			transform*=gid->fromNavTransform;
			transform.renormalize();
			gid->device->setTransformation(TrackerState(transform.getTranslation(),transform.getRotation()));
			}
	
	/* Go through all graph levels: */
	for(int i=0;i<=maxGraphLevel;++i)
		{
		/* Trigger callbacks on all input devices in the level: */
		for(GraphInputDevice* gid=deviceLevels[i];gid!=0;gid=gid->levelSucc)
			{
			gid->device->enableCallbacks();
			gid->device->disableCallbacks();
			}
		
		/* Call frame method on all tools in the level: */
		for(GraphTool* gt=toolLevels[i];gt!=0;gt=gt->levelSucc)
			gt->tool->frame();
		}
	}

void InputGraphManager::glRenderAction(GLContextData& contextData) const
	{
	/* Get the glyph renderer's context data item: */
	const GlyphRenderer::DataItem* glyphRendererContextDataItem=glyphRenderer->getContextDataItem(contextData);
	
	/* Render all input devices in the first input graph level: */
	for(const GraphInputDevice* gid=deviceLevels[0];gid!=0;gid=gid->levelSucc)
		{
		/* Check if the device is an ungrabbed virtual input device: */
		if(gid->grabber==0)
			virtualInputDevice->renderDevice(gid->device,gid->navigational,glyphRendererContextDataItem,contextData);
		else
			glyphRenderer->renderGlyph(gid->deviceGlyph,OGTransform(gid->device->getTransformation()),glyphRendererContextDataItem);
		}

	/* Render all tools in the first input graph level: */
	for(const GraphTool* gt=toolLevels[0];gt!=0;gt=gt->levelSucc)
		gt->tool->display(contextData);
	
	/* Iterate through all higher input graph levels: */
	for(int level=1;level<=maxGraphLevel;++level)
		{
		/* Render all input devices in this level: */
		for(const GraphInputDevice* gid=deviceLevels[level];gid!=0;gid=gid->levelSucc)
			glyphRenderer->renderGlyph(gid->deviceGlyph,OGTransform(gid->device->getTransformation()),glyphRendererContextDataItem);
		
		/* Render all tools in this level: */
		for(const GraphTool* gt=toolLevels[level];gt!=0;gt=gt->levelSucc)
			gt->tool->display(contextData);
		}
	}

}
