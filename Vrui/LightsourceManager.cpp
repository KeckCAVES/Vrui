/***********************************************************************
LightsourceManager - Class to manage light sources in virtual
environments. Maps created Lightsource objects to OpenGL light sources.
Copyright (c) 2005-2011 Oliver Kreylos

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

#include <Vrui/LightsourceManager.h>

#include <GL/gl.h>
#include <GL/GLLightTemplates.h>
#include <GL/GLLight.h>
#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/DisplayState.h>

namespace Vrui {

/*********************************************
Methods of class LightsourceManager::DataItem:
*********************************************/

LightsourceManager::DataItem::DataItem(void)
	:lastNumLightsources(0)
	{
	}

LightsourceManager::DataItem::~DataItem(void)
	{
	}

/***********************************
Methods of class LightsourceManager:
***********************************/

LightsourceManager::LightsourceManager(void)
	:firstLightsource(0),lastLightsource(0)
	{
	}

LightsourceManager::~LightsourceManager(void)
	{
	/* Destroy all light sources: */
	while(firstLightsource!=0)
		{
		LightsourceListItem* succ=firstLightsource->succ;
		delete firstLightsource;
		firstLightsource=succ;
		}
	}

void LightsourceManager::initContext(GLContextData& contextData) const
	{
	/* Create a new context data item: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

Lightsource* LightsourceManager::createLightsource(bool physical)
	{
	/* Create a new light source: */
	LightsourceListItem* newLightsource=new LightsourceListItem(physical);
	
	/* Add the new light source to the list: */
	if(lastLightsource!=0)
		lastLightsource->succ=newLightsource;
	else
		firstLightsource=newLightsource;
	lastLightsource=newLightsource;
	
	/* Return the new light source: */
	return newLightsource;
	}

Lightsource* LightsourceManager::createLightsource(bool physical,const GLLight& sLight)
	{
	/* Create a new light source: */
	LightsourceListItem* newLightsource=new LightsourceListItem(physical,sLight);
	
	/* Add the new light source to the list: */
	if(lastLightsource!=0)
		lastLightsource->succ=newLightsource;
	else
		firstLightsource=newLightsource;
	lastLightsource=newLightsource;
	
	/* Return the new light source: */
	return newLightsource;
	}

void LightsourceManager::destroyLightsource(Lightsource* lightsource)
	{
	/* Find the light source in the list: */
	LightsourceListItem* lsPtr1;
	LightsourceListItem* lsPtr2;
	for(lsPtr1=0,lsPtr2=firstLightsource;lsPtr2!=0&&lsPtr2!=lightsource;lsPtr1=lsPtr2,lsPtr2=lsPtr2->succ)
		;
	
	/* Remove the light source if it was found (ignore otherwise): */
	if(lsPtr2!=0)
		{
		if(lsPtr1!=0)
			lsPtr1->succ=lsPtr2->succ;
		else
			firstLightsource=lsPtr2->succ;
		if(lastLightsource==lsPtr2)
			lastLightsource=lsPtr1;
		delete lsPtr2;
		}
	}

void LightsourceManager::setLightsources(GLContextData& contextData) const
	{
	/* Retrieve the data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Process all light sources: */
	int lightIndex=0;
	for(const LightsourceListItem* lsPtr=firstLightsource;lsPtr!=0&&lightIndex<dataItem->lightTracker.getMaxNumLights();lsPtr=lsPtr->succ)
		{
		if(lsPtr->isEnabled())
			{
			glEnableLight(lightIndex);
			glLight(lightIndex,lsPtr->getLight());
			
			/* Increment the light index: */
			++lightIndex;
			}
		}
	
	/* Disable all unused light sources still enabled from the last pass: */
	for(GLsizei i=lightIndex;i<dataItem->lastNumLightsources;++i)
		glDisableLight(i);
	dataItem->lastNumLightsources=lightIndex;
	
	/* Update the light source tracker (could be optimized by not going through OpenGL): */
	dataItem->lightTracker.update();
	}

void LightsourceManager::setLightsources(DisplayState* displayState,GLContextData& contextData) const
	{
	/* Retrieve the data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Process all physical light sources first: */
	GLsizei lightIndex=0;
	bool haveNavigationalLightsources=false;
	for(const LightsourceListItem* lsPtr=firstLightsource;lsPtr!=0&&lightIndex<dataItem->lightTracker.getMaxNumLights();lsPtr=lsPtr->succ)
		{
		if(lsPtr->isEnabled())
			{
			/* Only set light source now if it is physical: */
			if(lsPtr->physical)
				{
				glEnableLight(lightIndex);
				glLight(lightIndex,lsPtr->getLight());
				
				/* Increment the light index: */
				++lightIndex;
				}
			else
				haveNavigationalLightsources=true;
			}
		}
	
	if(haveNavigationalLightsources)
		{
		/* Temporarily go to navigational coordinates: */
		glPushMatrix();
		glLoadIdentity();
		glMultMatrix(displayState->modelviewNavigational);
		
		/* Process all navigational light sources: */
		for(const LightsourceListItem* lsPtr=firstLightsource;lsPtr!=0&&lightIndex<dataItem->lightTracker.getMaxNumLights();lsPtr=lsPtr->succ)
			{
			if(lsPtr->isEnabled())
				{
				/* Only set light source now if it is navigational: */
				if(!lsPtr->physical)
					{
					glEnableLight(lightIndex);
					glLight(lightIndex,lsPtr->getLight());
					
					/* Increment the light index: */
					++lightIndex;
					}
				}
			}
		
		/* Return to physical coordinates: */
		glPopMatrix();
		}
	
	/* Disable all unused light sources still enabled from the last pass: */
	for(GLsizei i=lightIndex;i<dataItem->lastNumLightsources;++i)
		glDisableLight(i);
	dataItem->lastNumLightsources=lightIndex;
	
	/* Update the light source tracker (could be optimized by not going through OpenGL): */
	dataItem->lightTracker.update();
	}

const GLLightTracker& LightsourceManager::getLightTracker(GLContextData& contextData) const
	{
	/* Retrieve the data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Return the light tracker: */
	return dataItem->lightTracker;
	}

}
