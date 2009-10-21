/***********************************************************************
ClipPlaneManager - Class to manage clipping planes in virtual
environments. Maps created ClipPlane objects to OpenGL clipping planes.
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

#include <Vrui/ClipPlaneManager.h>

#include <Geometry/Plane.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/DisplayState.h>

namespace Vrui {

/*******************************************
Methods of class ClipPlaneManager::DataItem:
*******************************************/

ClipPlaneManager::DataItem::DataItem(void)
	:lastNumClipPlanes(0)
	{
	/* Query the maximum number of clipping planes from OpenGL: */
	glGetIntegerv(GL_MAX_CLIP_PLANES,&numClipPlanes);
	}

ClipPlaneManager::DataItem::~DataItem(void)
	{
	}

/*********************************
Methods of class ClipPlaneManager:
*********************************/

ClipPlaneManager::ClipPlaneManager(void)
	:firstClipPlane(0),lastClipPlane(0)
	{
	}

ClipPlaneManager::~ClipPlaneManager(void)
	{
	/* Destroy all clipping planes: */
	while(firstClipPlane!=0)
		{
		ClipPlaneListItem* succ=firstClipPlane->succ;
		delete firstClipPlane;
		firstClipPlane=succ;
		}
	}

void ClipPlaneManager::initContext(GLContextData& contextData) const
	{
	/* Create a new context data item: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

ClipPlane* ClipPlaneManager::createClipPlane(bool physical)
	{
	/* Create a new clipping plane: */
	ClipPlaneListItem* newClipPlane=new ClipPlaneListItem(physical);
	
	/* Add the new clipping plane to the list: */
	if(lastClipPlane!=0)
		lastClipPlane->succ=newClipPlane;
	else
		firstClipPlane=newClipPlane;
	lastClipPlane=newClipPlane;
	
	/* Return the new clipping plane: */
	return newClipPlane;
	}

ClipPlane* ClipPlaneManager::createClipPlane(bool physical,const Plane& sPlane)
	{
	/* Create a new clipping plane: */
	ClipPlaneListItem* newClipPlane=new ClipPlaneListItem(physical,sPlane);
	
	/* Add the new clipping plane to the list: */
	if(lastClipPlane!=0)
		lastClipPlane->succ=newClipPlane;
	else
		firstClipPlane=newClipPlane;
	lastClipPlane=newClipPlane;
	
	/* Return the new clipping plane: */
	return newClipPlane;
	}

void ClipPlaneManager::destroyClipPlane(ClipPlane* clipPlane)
	{
	/* Find the clipping plane in the list: */
	ClipPlaneListItem* lsPtr1;
	ClipPlaneListItem* lsPtr2;
	for(lsPtr1=0,lsPtr2=firstClipPlane;lsPtr2!=0&&lsPtr2!=clipPlane;lsPtr1=lsPtr2,lsPtr2=lsPtr2->succ)
		;
	
	/* Remove the clipping plane if it was found (ignore otherwise): */
	if(lsPtr2!=0)
		{
		if(lsPtr1!=0)
			lsPtr1->succ=lsPtr2->succ;
		else
			firstClipPlane=lsPtr2->succ;
		if(lastClipPlane==lsPtr2)
			lastClipPlane=lsPtr1;
		delete lsPtr2;
		}
	}

void ClipPlaneManager::setClipPlanes(GLContextData& contextData) const
	{
	/* Retrieve the data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Process all clipping planes: */
	GLsizei clipPlaneIndex=0;
	for(const ClipPlaneListItem* cpPtr=firstClipPlane;cpPtr!=0&&clipPlaneIndex<dataItem->numClipPlanes;cpPtr=cpPtr->succ)
		{
		if(cpPtr->isEnabled())
			{
			/* Enable the OpenGL clipping plane: */
			if(clipPlaneIndex>=dataItem->lastNumClipPlanes)
				glEnable(GL_CLIP_PLANE0+clipPlaneIndex);
			
			/* Set the OpenGL clipping plane's plane equation: */
			GLdouble plane[4];
			for(int i=0;i<3;++i)
				plane[i]=cpPtr->getPlane().getNormal()[i];
			plane[3]=-cpPtr->getPlane().getOffset();
			glClipPlane(GL_CLIP_PLANE0+clipPlaneIndex,plane);
			
			/* Increment the clipping plane index: */
			++clipPlaneIndex;
			}
		}
	
	/* Disable all unused clipping planes still enabled from the last pass: */
	for(GLsizei i=clipPlaneIndex;i<dataItem->lastNumClipPlanes;++i)
		glDisable(GL_CLIP_PLANE0+i);
	dataItem->lastNumClipPlanes=clipPlaneIndex;
	}

void ClipPlaneManager::setClipPlanes(DisplayState* displayState,GLContextData& contextData) const
	{
	/* Retrieve the data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Process all physical clipping planes first: */
	GLsizei clipPlaneIndex=0;
	bool haveNavigationalClipPlanes=false;
	for(const ClipPlaneListItem* cpPtr=firstClipPlane;cpPtr!=0&&clipPlaneIndex<dataItem->numClipPlanes;cpPtr=cpPtr->succ)
		{
		if(cpPtr->isEnabled())
			{
			/* Only set clipping plane now if it is physical: */
			if(cpPtr->physical)
				{
				/* Enable the OpenGL clipping plane: */
				if(clipPlaneIndex>=dataItem->lastNumClipPlanes)
					glEnable(GL_CLIP_PLANE0+clipPlaneIndex);
				
				/* Set the OpenGL clipping plane's plane equation: */
				GLdouble plane[4];
				for(int i=0;i<3;++i)
					plane[i]=cpPtr->getPlane().getNormal()[i];
				plane[3]=-cpPtr->getPlane().getOffset();
				glClipPlane(GL_CLIP_PLANE0+clipPlaneIndex,plane);
				
				/* Increment the clipping plane index: */
				++clipPlaneIndex;
				}
			else
				haveNavigationalClipPlanes=true;
			}
		}
	
	if(haveNavigationalClipPlanes&&clipPlaneIndex<dataItem->numClipPlanes)
		{
		/* Temporarily go to navigational coordinates: */
		glPushMatrix();
		glLoadIdentity();
		glMultMatrix(displayState->modelviewNavigational);
		
		/* Process all navigational clipping planes: */
		for(const ClipPlaneListItem* cpPtr=firstClipPlane;cpPtr!=0&&clipPlaneIndex<dataItem->numClipPlanes;cpPtr=cpPtr->succ)
			{
			if(cpPtr->isEnabled()&&cpPtr->physical)
				{
				/* Enable the OpenGL clipping plane: */
				if(clipPlaneIndex>=dataItem->lastNumClipPlanes)
					glEnable(GL_CLIP_PLANE0+clipPlaneIndex);
				
				/* Set the OpenGL clipping plane's plane equation: */
				GLdouble plane[4];
				for(int i=0;i<3;++i)
					plane[i]=cpPtr->getPlane().getNormal()[i];
				plane[3]=-cpPtr->getPlane().getOffset();
				glClipPlane(GL_CLIP_PLANE0+clipPlaneIndex,plane);
				
				/* Increment the clipping plane index: */
				++clipPlaneIndex;
				}
			}
		
		/* Return to physical coordinates: */
		glPopMatrix();
		}
	
	/* Disable all unused clipping planes still enabled from the last pass: */
	for(GLsizei i=clipPlaneIndex;i<dataItem->lastNumClipPlanes;++i)
		glDisable(GL_CLIP_PLANE0+i);
	dataItem->lastNumClipPlanes=clipPlaneIndex;
	}

void ClipPlaneManager::disableClipPlanes(GLContextData& contextData)
	{
	/* Retrieve the data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Disable all previously enabled clipping planes: */
	for(GLsizei clipPlaneIndex=0;clipPlaneIndex<dataItem->lastNumClipPlanes;++clipPlaneIndex)
		glDisable(GL_CLIP_PLANE0+clipPlaneIndex);
	
	dataItem->lastNumClipPlanes=0;
	}

}
