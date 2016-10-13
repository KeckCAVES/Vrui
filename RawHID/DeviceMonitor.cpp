/***********************************************************************
DeviceMonitor - Class monitoring dynamic appearances/disappearances of
raw HID devices.
Copyright (c) 2016 Oliver Kreylos

This file is part of the Raw HID Support Library (RawHID).

The Raw HID Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Raw HID Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Raw HID Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <RawHID/DeviceMonitor.h>

#include <stdlib.h>
#include <string.h>
#include <Misc/FunctionCalls.h>
#include <RawHID/Internal/UdevDevice.h>
#include <RawHID/Internal/UdevMonitor.h>

namespace RawHID {

/******************************
Methods of class DeviceMonitor:
******************************/

void* DeviceMonitor::eventDispatcherThreadMethod(void)
	{
	/* Enable thread cancellation: */
	Threads::Thread::setCancelState(Threads::Thread::CANCEL_ENABLE);
	// Threads::Thread::setCancelType(Threads::Thread::CANCEL_ASYNCHRONOUS);
	
	while(true)
		{
		/* Wait for the next device event: */
		UdevDevice device=monitor->receiveDeviceEvent();
		if(device.isValid())
			{
			/* Check if the event is an add or remove event: */
			const char* action=device.getAction();
			if(action!=0&&strcmp(action,"add")==0)
				{
				/* Get the device's parent in the HID subsystem: */
				UdevDevice hid=device.getParent("hid");
				if(hid.isValid())
					{
					Threads::Mutex::Lock addListenersLock(addListenersMutex);
					
					/* Prepare an event callback structure: */
					AddEvent ev;
					ev.deviceMonitor=this;
					ev.deviceNode=device.getDevnode();
					
					/* Extract the HID device's bus type, vendor and product IDs, and serial number: */
					char* hidId=const_cast<char*>(hid.getPropertyValue("HID_ID")); // Necessary as strtoul doesn't take a const char* as endptr, but it's fine
					ev.busType=int(strtoul(hidId,&hidId,16));
					if(*hidId!='\0')
						ev.vendorId=(unsigned short)(strtoul(hidId+1,&hidId,16));
					else
						ev.vendorId=0U;
					if(*hidId!='\0')
						ev.productId=(unsigned short)(strtoul(hidId+1,&hidId,16));
					else
						ev.productId=0U;
					ev.serialNumber=hid.getPropertyValue("HID_UNIQ");
					
					/* Match the device's parameters against any registered listeners: */
					for(std::vector<AddListener>::iterator lIt=addListeners.begin();lIt!=addListeners.end();++lIt)
						if(((lIt->matchMask&BusTypeMask)==0x0||lIt->busType==ev.busType)&&
						   ((lIt->matchMask&VendorIdMask)==0x0||lIt->vendorId==ev.vendorId)&&
						   ((lIt->matchMask&ProductIdMask)==0x0||lIt->productId==ev.productId)&&
						   ((lIt->matchMask&SerialNumberMask)==0x0||lIt->serialNumber==ev.serialNumber))
							{
							/* Call the callback: */
							ev.listenerKey=lIt->listenerKey;
							(*lIt->callback)(ev);
							}
					}
				}
			else if(action!=0&&strcmp(action,"remove")==0)
				{
				Threads::Mutex::Lock removeListenersLock(removeListenersMutex);
				
				/* Prepare an event callback structure: */
				RemoveEvent ev;
				ev.deviceMonitor=this;
				ev.deviceNode=device.getDevnode();
				
				/* Match the device's node file name against any registered listeners: */
				std::vector<RemoveListener>::iterator end=removeListeners.end();
				for(std::vector<RemoveListener>::iterator lIt=removeListeners.begin();lIt!=end;++lIt)
					if(lIt->deviceNode==ev.deviceNode)
						{
						/* Call the callback: */
						ev.listenerKey=lIt->listenerKey;
						(*lIt->callback)(ev);
						
						/* Remove the listener from the list, as its device node just disappeared: */
						delete lIt->callback;
						--end;
						*lIt=*end;
						--lIt;
						}
				
				/* Now actually remove all removed listeners from the list: */
				removeListeners.erase(end,removeListeners.end());
				}
			}
		}
	
	return 0;
	}

DeviceMonitor::DeviceMonitor(void)
	:monitor(0),
	 nextAddKey(0),nextRemoveKey(0)
	{
	/* Create a new udev monitor with a private udev context: */
	monitor=new UdevMonitor;
	
	/* Configure the monitor to wait for events on the raw HID subsystem: */
	monitor->addSubsystemFilter("hidraw",0);
	
	/* Start listening for udev events: */
	monitor->listen();
	
	/* Start the event dispatcher thread: */
	eventDispatcherThread.start(this,&DeviceMonitor::eventDispatcherThreadMethod);
	}

DeviceMonitor::~DeviceMonitor(void)
	{
	/* Shut down the event dispatcher thread: */
	eventDispatcherThread.cancel();
	eventDispatcherThread.join();
	
	/* Destroy the lists of registered listeners: */
	for(std::vector<AddListener>::iterator lIt=addListeners.begin();lIt!=addListeners.end();++lIt)
		delete lIt->callback;
	for(std::vector<RemoveListener>::iterator lIt=removeListeners.begin();lIt!=removeListeners.end();++lIt)
		delete lIt->callback;
	
	/* Destroy the udev monitor: */
	delete monitor;
	}

DeviceMonitor::ListenerKey DeviceMonitor::registerAddEventListener(int newMatchMask,int newBusType,unsigned short newVendorId,unsigned short newProductId,const char* newSerialNumber,DeviceMonitor::AddEventCallback* newCallback)
	{
	Threads::Mutex::Lock addListenersLock(addListenersMutex);
	
	/* Create a listener structure: */
	AddListener al;
	al.listenerKey=nextAddKey;
	++nextAddKey;
	al.matchMask=newMatchMask;
	al.busType=newBusType;
	al.vendorId=newVendorId;
	al.productId=newProductId;
	if(newSerialNumber!=0)
		al.serialNumber=newSerialNumber;
	else
		al.matchMask&=~SerialNumberMask;
	al.callback=newCallback;
	
	/* Add the listener structure to the list: */
	addListeners.push_back(al);
	
	/* Return the assigned key: */
	return al.listenerKey;
	}

void DeviceMonitor::unregisterAddEventListener(DeviceMonitor::ListenerKey listenerKey)
	{
	Threads::Mutex::Lock addListenersLock(addListenersMutex);
	
	/* Find the listener structure in the list: */
	std::vector<AddListener>::iterator lIt;
	for(lIt=addListeners.begin();lIt!=addListeners.end()&&lIt->listenerKey!=listenerKey;++lIt)
		;
	if(lIt!=addListeners.end())
		{
		/* Remove the found list entry: */
		delete lIt->callback;
		*lIt=addListeners.back();
		addListeners.pop_back();
		}
	}

DeviceMonitor::ListenerKey DeviceMonitor::registerRemoveEventListener(const char* newDeviceNode,DeviceMonitor::RemoveEventCallback* newCallback)
	{
	Threads::Mutex::Lock removeListenersLock(removeListenersMutex);
	
	/* Create a listener structure: */
	RemoveListener rl;
	rl.listenerKey=nextRemoveKey;
	++nextRemoveKey;
	rl.deviceNode=newDeviceNode;
	rl.callback=newCallback;
	
	/* Add the listener structure to the list: */
	removeListeners.push_back(rl);
	
	/* Return the assigned key: */
	return rl.listenerKey;
	}

void DeviceMonitor::unregisterRemoveEventListener(DeviceMonitor::ListenerKey listenerKey)
	{
	Threads::Mutex::Lock removeListenersLock(removeListenersMutex);
	
	/* Find the listener structure in the list: */
	std::vector<RemoveListener>::iterator lIt;
	for(lIt=removeListeners.begin();lIt!=removeListeners.end()&&lIt->listenerKey!=listenerKey;++lIt)
		;
	if(lIt!=removeListeners.end())
		{
		/* Remove the found list entry: */
		delete lIt->callback;
		*lIt=removeListeners.back();
		removeListeners.pop_back();
		}
	}

}
