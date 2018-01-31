/***********************************************************************
InputDeviceAdapterMultitouch - Class to convert a direct-mode
multitouch-capable screen into a set of Vrui input devices.
Copyright (c) 2015-2017 Oliver Kreylos

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

#include <Vrui/Internal/InputDeviceAdapterMultitouch.h>

// DEBUGGING
#include <iostream>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/InputGraphManager.h>

#include <stdio.h>
#include <Misc/PrintInteger.h>
#include <Misc/StandardHashFunction.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLColor.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/VRWindow.h>
#include <Vrui/Internal/Vrui.h>

namespace Vrui {

/*********************************************
Methods of class InputDeviceAdapterMultitouch:
*********************************************/

InputDeviceAdapterMultitouch::InputDeviceAdapterMultitouch(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection)
	:InputDeviceAdapter(sInputDeviceManager),
	 maxNumDevices(10),
	 numModifierButtons(5),
	 numDeviceButtons(3),
	 maxContactArea(20),
	 activationInterval(0.01),
	 multicontactRadius(100.0),
	 deviceMappers(0),
	 modifierPlane(0),
	 touchIdMapper(17),
	 modifierTouchId(-1),previousModifierPlane(-1),modifierPanelTimeout(0.0),
	 mostRecentTouchWindow(0)
	{
	/* Retrieve the maximum number of parallel touch contacts: */
	maxNumDevices=configFileSection.retrieveValue<int>("./maxNumDevices",maxNumDevices);
	
	/* Retrieve the number of modifier buttons: */
	numModifierButtons=configFileSection.retrieveValue<int>("./numModifierButtons",numModifierButtons);
	
	/* Retrieve the number of buttons per touch contact per modifier plane: */
	numDeviceButtons=configFileSection.retrieveValue<int>("./numDeviceButtons",numDeviceButtons);
	
	/* Retrieve the maximum touch contact area for palm rejection: */
	maxContactArea=configFileSection.retrieveValue<double>("./maxContactArea",maxContactArea);
	
	/* Retrieve the multi-contact activation interval: */
	activationInterval=configFileSection.retrieveValue<double>("./activationInterval",activationInterval);
	
	/* Retrieve the multi-contact activation radius: */
	multicontactRadius=configFileSection.retrieveValue<double>("./multicontactRadius",multicontactRadius);
	
	/* Allocate new adapter state arrays: */
	numInputDevices=maxNumDevices+1;
	inputDevices=new InputDevice*[numInputDevices];
	for(int i=0;i<numInputDevices;++i)
		inputDevices[i]=0;
	deviceMappers=new DeviceMapper[maxNumDevices+1]; // The extra device mapper at the end is for modifier touch contacts
	
	/* Create all input devices: */
	for(int i=0;i<maxNumDevices;++i)
		{
		/* Create a new one-button directional input device: */
		char deviceName[32];
		snprintf(deviceName,sizeof(deviceName),"Multitouch%02d",i);
		InputDevice* newDevice=inputDeviceManager->createInputDevice(deviceName,InputDevice::TRACK_POS|InputDevice::TRACK_DIR,numModifierButtons*numDeviceButtons,0,true);
		
		// DEBUGGING
		// Glyph& deviceGlyph=inputDeviceManager->getInputGraphManager()->getInputDeviceGlyph(newDevice);
		// deviceGlyph.enable();
		// deviceGlyph.setGlyphType(Glyph::SPHERE);
		
		/* Store the input device: */
		inputDevices[i]=newDevice;
		deviceMappers[i].device=newDevice;
		}
	
	/* Create an additional input device to represent the modifier planes as buttons: */
	InputDevice* newDevice=inputDeviceManager->createInputDevice("MultitouchModifierDevice",InputDevice::TRACK_POS|InputDevice::TRACK_DIR,numModifierButtons,0,true);
	inputDevices[maxNumDevices]=newDevice;
	}

InputDeviceAdapterMultitouch::~InputDeviceAdapterMultitouch(void)
	{
	/* Delete the device mapper array: */
	delete[] deviceMappers;
	}

std::string InputDeviceAdapterMultitouch::getFeatureName(const InputDeviceFeature& feature) const
	{
	if(feature.getDevice()==inputDevices[maxNumDevices])
		{
		std::string result="Plane";
		int planeIndex=feature.getFeatureIndex();
		char index[5];
		result.append(Misc::print(planeIndex,index+4));
		
		return result;
		}
	else
		{
		std::string result="Plane";
		int planeIndex=feature.getFeatureIndex()/numDeviceButtons;
		char index[5];
		result.append(Misc::print(planeIndex,index+4));
		result.append("Button");
		int buttonIndex=feature.getFeatureIndex()%numDeviceButtons;
		result.append(Misc::print(buttonIndex,index+4));
		
		return result;
		}
	}

int InputDeviceAdapterMultitouch::getFeatureIndex(InputDevice* device,const char* featureName) const
	{
	/* Check if the feature is on one of our devices: */
	int deviceIndex;
	for(deviceIndex=0;deviceIndex<numInputDevices&&inputDevices[deviceIndex]!=device;++deviceIndex)
		;
	if(deviceIndex>=numInputDevices)
		return -1;
	
	/* Parse the feature name: */
	if(deviceIndex==maxNumDevices)
		{
		int planeIndex;
		if(sscanf(featureName,"Plane%d",&planeIndex)==1&&
		   planeIndex>=0&&planeIndex<numModifierButtons)
			return planeIndex;
		else
			return -1;
		}
	else
		{
		int planeIndex,buttonIndex;
		if(sscanf(featureName,"Plane%dButton%d",&planeIndex,&buttonIndex)==2&&
		   planeIndex>=0&&planeIndex<numModifierButtons&&buttonIndex>=0&&buttonIndex<numDeviceButtons)
			return planeIndex*numDeviceButtons+buttonIndex;
		else
			return -1;
		}
	}

void InputDeviceAdapterMultitouch::updateInputDevices(void)
	{
	/* Update all currently active input devices: */
	double nextTimeout=Math::Constants<double>::max;
	for(int i=0;i<maxNumDevices;++i)
		if(deviceMappers[i].state>=DeviceMapper::Activating)
			{
			// DEBUGGING
			// std::cout<<"Updating mapper "<<i<<std::endl;
			
			DeviceMapper& dm=deviceMappers[i];
			
			/* Check if the device mapping is done activating: */
			if(dm.state==DeviceMapper::Activating&&dm.activationTimeout<=getApplicationTime())
				{
				/* Activate this device mapping: */
				dm.state=DeviceMapper::Active;
				
				/* Press the selected device button if this is a primary: */
				if(dm.pred==0)
					{
					/* Limit the button index to the valid range: */
					if(dm.buttonIndex>numDeviceButtons-1)
						dm.buttonIndex=numDeviceButtons-1;
					
					/* Map the button index into the currently active modifier plane: */
					dm.buttonIndex+=modifierPlane*numDeviceButtons;
					
					/* Press the button: */
					dm.device->setButtonState(dm.buttonIndex,true);
					
					// DEBUGGING
					// std::cout<<"Marking primary mapper "<<i<<" as active"<<std::endl;
					}
				else
					{
					// DEBUGGING
					// std::cout<<"Marking secondary mapper "<<i<<" as active"<<std::endl;
					}
				}
			
			/* Check if a dead primary device mapping can be deactivated: */
			if(dm.dead&&dm.succ==0)
				{
				/* Deactivate this device mapping: */
				dm.state=DeviceMapper::Inactive;
				
				/* Release the selected device button: */
				dm.device->setButtonState(dm.buttonIndex,false);
				
				// DEBUGGING
				// std::cout<<"Deactivating dead primary mapper "<<i<<std::endl;
				}
			
			/* Update the mapped Vrui input device if this mapping is a primary: */
			if(dm.pred==0)
				{
				/* Average the window positions of all secondaries: */
				Scalar avgPos[2]={0.0,0.0};
				int numDevices=0;
				
				/* Use the mapping's own position if it is still alive: */
				if(!dm.dead)
					{
					for(int i=0;i<2;++i)
						avgPos[i]+=dm.windowPos[i];
					++numDevices;
					}
				
				/* Add the positions of all secondaries that are still mapped to the same window: */
				for(DeviceMapper* sPtr=dm.succ;sPtr!=0;sPtr=sPtr->succ)
					if(sPtr->window==dm.window)
						{
						for(int i=0;i<2;++i)
							avgPos[i]+=sPtr->windowPos[i];
						++numDevices;
						}
				
				/* Update the input device position if there are still alive contacts: */
				if(numDevices>0)
					{
					for(int i=0;i<2;++i)
						avgPos[i]=avgPos[i]/Scalar(numDevices)+dm.offset[i];
					
					// DEBUGGING
					// std::cout<<"Device i: "<<avgPos[0]<<", "<<avgPos[1]<<std::endl;
					
					/* Set multitouch device's transformation and device ray: */
					Point lastDevicePos=dm.device->getPosition();
					dm.window->updateScreenDevice(avgPos,dm.device);
					
					/* Calculate the multitouch device's linear velocity: */
					dm.device->setLinearVelocity((dm.device->getPosition()-lastDevicePos)/Vrui::getFrameTime());
					}
				}
			}
	
	/* Update the state of the modifier plane device: */
	for(int i=0;i<numModifierButtons;++i)
		inputDevices[maxNumDevices]->setButtonState(i,i==modifierPlane);
	
	/* Schedule another frame if there are pending activation events: */
	if(nextTimeout<Math::Constants<double>::max)
		scheduleUpdate(nextTimeout);
	
	/* Schedule another frame if the modifier button panel is still being shown: */
	if(getApplicationTime()<modifierPanelTimeout)
		scheduleUpdate(modifierPanelTimeout);
	}

void InputDeviceAdapterMultitouch::glRenderAction(GLContextData& contextData) const
	{
	/* Check if the currently rendered window is the most recent touch event source: */
	const DisplayState& ds=getDisplayState(contextData);
	if(ds.window==mostRecentTouchWindow&&(modifierTouchId>=0||getApplicationTime()<modifierPanelTimeout))
		{
		/* Draw the left-swipe modifier button panel: */
		glPushAttrib(GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glDepthFunc(GL_LEQUAL);
		glLineWidth(1.0f);
		
		glPushMatrix();
		glLoadIdentity();
		
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0.0,double(ds.window->getWindowWidth()),0.0,double(ds.window->getWindowHeight()),0.0,1.0);
		
		double buttonSize=double(ds.window->getWindowHeight())/double(numModifierButtons);
		double tabSize=buttonSize*0.8;
		double cornerSize=tabSize*0.25;
		
		/* Draw the inactive modifier button tabs: */
		glLineWidth(3.0f);
		glColor(getBackgroundColor());
		
		for(int pass=0;pass<2;++pass)
			{
			for(int i=0;i<numModifierButtons;++i)
				{
				double y0=double(i)*buttonSize+buttonSize*0.1;
				
				glBegin(GL_LINE_STRIP);
				glVertex2d(0.0,y0);
				glVertex2d(tabSize-cornerSize,y0);
				glVertex2d(tabSize,y0+cornerSize);
				glVertex2d(tabSize,y0+tabSize-cornerSize);
				glVertex2d(tabSize-cornerSize,y0+tabSize);
				glVertex2d(0.0,y0+tabSize);
				glEnd();
				}
			
			glLineWidth(1.0f);
			glColor(getForegroundColor());
			}
		
		/* Draw the active modifier button tab: */
		double y0=double(modifierPlane)*buttonSize+buttonSize*0.1;
		
		glBegin(GL_POLYGON);
		glVertex2d(0.0,y0);
		glVertex2d(tabSize-cornerSize,y0);
		glVertex2d(tabSize,y0+cornerSize);
		glVertex2d(tabSize,y0+tabSize-cornerSize);
		glVertex2d(tabSize-cornerSize,y0+tabSize);
		glVertex2d(0.0,y0+tabSize);
		glEnd();
		
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		
		glPopAttrib();
		}
	}

void InputDeviceAdapterMultitouch::touchBegin(VRWindow* newWindow,const InputDeviceAdapterMultitouch::TouchEvent& event)
	{
	/* Reject the touch event as a palm contact if the touch ellipse is too large: */
	if(event.majorAxis*event.minorAxis>maxContactArea)
		return;
	
	/* Check if the touch ID is already assigned, due to a spurious TouchBegin event: */
	if(!touchIdMapper.isEntry(event.id))
		{
		/* Check if this is a left-swipe modifier button panel event with no active modifier touch: */
		if(event.x<=Scalar(0)&&modifierTouchId==-1)
			{
			/* Use the dedicated modifier device mapper: */
			DeviceMapper* newDm=deviceMappers+maxNumDevices;
			
			/* Start a modifier button mapping: */
			newDm->state=DeviceMapper::Modifier;
			newDm->window=newWindow;
			newDm->set(event);
			newDm->dead=false;
			
			/* Map the new touch ID to the new device mapper: */
			touchIdMapper[event.id]=newDm;
			
			/* Start a new modifier touch contact: */
			modifierTouchId=event.id;
			previousModifierPlane=modifierPlane;
			
			/* Calculate the new modifier plane: */
			modifierPlane=Math::clamp(int(Math::floor((Scalar(1)-event.y/Scalar(newWindow->getWindowHeight()))*Scalar(numModifierButtons))),0,numModifierButtons-1);
			}
		else
			{
			/* Find an unused device mapper and check if the new contact is close to any currently activating contact: */
			DeviceMapper* newDm=0;
			DeviceMapper* primary=0;
			DeviceMapper* dmPtr=deviceMappers;
			for(int i=0;i<maxNumDevices;++i,++dmPtr)
				{
				/* Check if this is the first unused device mapper: */
				if(dmPtr->state==DeviceMapper::Inactive&&newDm==0)
					newDm=dmPtr;
				
				/* Check if this is a close-by activating device mapper: */
				if(dmPtr->state==DeviceMapper::Activating&&
				   dmPtr->window==newWindow&&
				   Math::sqr(dmPtr->windowPos[0]-event.x)+Math::sqr(dmPtr->windowPos[1]-event.y)<Math::sqr(multicontactRadius))
					{
					/* Add the new contact as a secondary to the primary contact: */
					primary=dmPtr;
					}
				}
			
			if(newDm!=0)
				{
				/* Start activating a new device mapping: */
				newDm->state=DeviceMapper::Activating;
				newDm->window=newWindow;
				newDm->set(event);
				newDm->dead=false;
				
				// DEBUGGING
				// std::cout<<"Starting touch at "<<touchX<<", "<<touchY<<std::endl;
				
				/* Check whether this will be a primary or secondary contact: */
				if(primary!=0)
					{
					/* Find the actual primary contact by traversing the list of linked contacts to the left: */
					while(primary->pred!=0)
						primary=primary->pred;
					
					/* Copy the primary's activation timeout: */
					newDm->activationTimeout=primary->activationTimeout;
					
					/* Calculate the primary's reported position immediately before and after this event to update the offset vector: */
					Scalar posSum[2];
					posSum[1]=posSum[0]=Scalar(0);
					unsigned int numContacts=0;
					for(DeviceMapper* cPtr=primary;cPtr!=0;cPtr=cPtr->succ)
						{
						posSum[0]+=cPtr->windowPos[0];
						posSum[1]+=cPtr->windowPos[1];
						++numContacts;
						}
					
					/* Calculate a new position offset to make the transition smooth: */
					for(int i=0;i<2;++i)
						primary->offset[i]=(posSum[i]/Scalar(numContacts)+primary->offset[i])-(posSum[i]+newDm->windowPos[i])/Scalar(numContacts+1);
					
					/* Increase the primary contact's button index and insert the new contact as the first secondary: */
					++primary->buttonIndex;
					newDm->succ=primary->succ;
					if(primary->succ!=0)
						primary->succ->pred=newDm;
					newDm->pred=primary;
					primary->succ=newDm;
					
					// DEBUGGING
					// std::cout<<"Activating mapper "<<(newDm-deviceMappers)<<" as secondary of mapper "<<(primary-deviceMappers)<<std::endl;
					}
				else
					{
					/* Start a new primary contact: */
					newDm->activationTimeout=peekApplicationTime()+activationInterval;
					newDm->pred=0;
					newDm->succ=0;
					newDm->buttonIndex=0;
					newDm->offset[1]=newDm->offset[0]=Scalar(0);
					
					// DEBUGGING
					// std::cout<<"Activating mapper "<<(newDm-deviceMappers)<<" as primary"<<std::endl;
					}
				}
			
			/* Map the new touch ID to the new device mapper: */
			touchIdMapper[event.id]=newDm;
			}
		}
	
	/* Remember the event source window: */
	mostRecentTouchWindow=newWindow;
	}

void InputDeviceAdapterMultitouch::touchUpdate(VRWindow* newWindow,const InputDeviceAdapterMultitouch::TouchEvent& event)
	{
	/* Find a device mapper for the touch ID: */
	Misc::HashTable<int,DeviceMapper*>::Iterator tmIt=touchIdMapper.findEntry(event.id);
	if(!tmIt.isFinished())
		{
		/* Update the device mapping: */
		DeviceMapper& dm=*tmIt->getDest();
		dm.window=newWindow;
		dm.set(event);
		
		/* Kill the touch contact if the touch ellipse is too large: */
		if(dm.majorAxis*dm.minorAxis>maxContactArea)
			dm.dead=true;
		
		/* Check if this is a modifier touch: */
		if(!dm.dead&&dm.state==DeviceMapper::Modifier)
			{
			/* Calculate the new modifier plane: */
			modifierPlane=Math::clamp(int(Math::floor((Scalar(1)-event.y/Scalar(newWindow->getWindowHeight()))*Scalar(numModifierButtons))),0,numModifierButtons-1);
			}
		
		// DEBUGGING
		// std::cout<<"Updating mapper "<<(&dm-deviceMappers)<<std::endl;
		}
	
	/* Remember the event source window: */
	mostRecentTouchWindow=newWindow;
	}

void InputDeviceAdapterMultitouch::touchEnd(VRWindow* newWindow,const InputDeviceAdapterMultitouch::TouchEvent& event)
	{
	/* Find a device mapper for the touch ID: */
	Misc::HashTable<int,DeviceMapper*>::Iterator tmIt=touchIdMapper.findEntry(event.id);
	if(!tmIt.isFinished())
		{
		/* Update the device mapping: */
		DeviceMapper& dm=*tmIt->getDest();
		dm.window=newWindow;
		dm.set(event);
		
		/* Kill the touch contact if the touch ellipse is too large: */
		if(dm.majorAxis*dm.minorAxis>maxContactArea)
			dm.dead=true;
		else
			{
			/* Act based on the type of device mapping: */
			if(dm.state==DeviceMapper::Modifier) // A modifier touch event
				{
				/* Check if the touch contact left back through the left edge: */
				if(event.x<=Scalar(0))
					{
					/* Revert back to the modifier plane that was active when the modifier contact started: */
					modifierPlane=previousModifierPlane;
					}
				
				/* Finish the current modifier touch: */
				modifierTouchId=-1;
				modifierPanelTimeout=peekApplicationTime()+1.0;
				
				/* Deactivate the device mapping: */
				dm.state=DeviceMapper::Inactive;
				}
			else if(dm.pred!=0) // A secondary touch contact
				{
				/* Find the primary contact: */
				DeviceMapper* primary=dm.pred;
				while(primary->pred!=0)
					primary=primary->pred;
				
				/* Remove the device mapping from the multi-contact list: */
				dm.pred->succ=dm.succ;
				if(dm.succ!=0)
					dm.succ->pred=dm.pred;
				
				/* Calculate the primary's reported position immediately before and after this event to update the offset vector: */
				Scalar posSum[2];
				posSum[1]=posSum[0]=Scalar(0);
				unsigned int numContacts=0;
				for(DeviceMapper* cPtr=primary;cPtr!=0;cPtr=cPtr->succ)
					{
					posSum[0]+=cPtr->windowPos[0];
					posSum[1]+=cPtr->windowPos[1];
					++numContacts;
					}
				
				/* Calculate a new position offset to make the transition smooth: */
				for(int i=0;i<2;++i)
					primary->offset[i]=((posSum[i]+dm.windowPos[i])/Scalar(numContacts+1)+primary->offset[i])-posSum[i]/Scalar(numContacts);
				
				/* Deactivate the device mapping: */
				dm.state=DeviceMapper::Inactive;
				
				// DEBUGGING
				// std::cout<<"Deactivating secondary mapper "<<(&dm-deviceMappers)<<std::endl;
				}
			else // A primary touch contact
				{
				/* Mark the device mapping as dead: */
				dm.dead=true;
				
				/* Calculate the primary's reported position immediately before and after this event to update the offset vector: */
				Scalar posSum[2];
				posSum[1]=posSum[0]=Scalar(0);
				unsigned int numContacts=0;
				for(DeviceMapper* cPtr=dm.succ;cPtr!=0;cPtr=cPtr->succ)
					{
					posSum[0]+=cPtr->windowPos[0];
					posSum[1]+=cPtr->windowPos[1];
					++numContacts;
					}
				
				/* Calculate a new position offset to make the transition smooth: */
				for(int i=0;i<2;++i)
					dm.offset[i]=((posSum[i]+dm.windowPos[i])/Scalar(numContacts+1)+dm.offset[i])-posSum[i]/Scalar(numContacts);
				
				// DEBUGGING
				// std::cout<<"Marking primary mapper "<<(&dm-deviceMappers)<<" as dead"<<std::endl;
				}
			}
		/* Remove the touch ID from the mapper: */
		touchIdMapper.removeEntry(tmIt);
		}
	
	/* Remember the event source window: */
	mostRecentTouchWindow=newWindow;
	}

}
