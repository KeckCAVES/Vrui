/***********************************************************************
InputDeviceAdapterJoystick - Class to directly connect a joystick or
other device supported by the Linux joystick layer to a Vrui input
device.
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

#include <Vrui/InputDeviceAdapterJoystick.h>

#ifdef __LINUX__
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>
#endif
#ifdef __DARWIN__
#include <IOKit/hid/IOHIDKeys.h>
#endif
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#ifdef __LINUX__
#include <Comm/FdSet.h>
#endif
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/Vrui.h>

namespace Misc {

/**************
Helper classes:
**************/

template <class ScalarParam>
class ValueCoder<Math::BrokenLine<ScalarParam> >
	{
	/* Methods: */
	static std::string encode(const Math::BrokenLine<ScalarParam>& v)
		{
		/* Encode the broken line as a vector with four elements: */
		std::vector<ScalarParam> values;
		values.push_back(v.min);
		values.push_back(v.deadMin);
		values.push_back(v.deadMax);
		values.push_back(v.max);
		return Misc::ValueCoder<std::vector<ScalarParam> >::encode(values);
		}
	static Math::BrokenLine<ScalarParam> decode(const char* start,const char* end,const char** decodeEnd =0)
		{
		/* Decode a vector with four elements: */
		std::vector<ScalarParam> values=Misc::ValueCoder<std::vector<ScalarParam> >::decode(start,end,decodeEnd);
		if(values.size()!=4)
			throw Misc::DecodingError(std::string("Wrong number of elements in ")+std::string(start,end));
		return Math::BrokenLine<ScalarParam>(values[0],values[1],values[2],values[3]);
		}
	};

}

namespace Vrui {

/*******************************************
Methods of class InputDeviceAdapterJoystick:
*******************************************/

#ifdef __LINUX__

void InputDeviceAdapterJoystick::createInputDevice(int deviceIndex,const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Read input device name: */
	std::string name=configFileSection.retrieveString("./name");
	
	/* Read joystick's device node: */
	std::string deviceNode=configFileSection.retrieveString("./deviceNode");
	
	/* Create a new device structure: */
	Device newDevice;
	
	/* Open the joystick device: */
	if((newDevice.deviceFd=open(deviceNode.c_str(),O_RDONLY))<0)
		Misc::throwStdErr("InputDeviceAdapterJoystick::createDevice: Could not open device node %s",deviceNode.c_str());
	
	/* Query the joystick's number of axes and buttons: */
	bool error=false;
	unsigned char nb,nv;
	error=error||ioctl(newDevice.deviceFd,JSIOCGBUTTONS,&nb)<0;
	error=error||ioctl(newDevice.deviceFd,JSIOCGAXES,&nv)<0;
	
	/* Query the joystick's name: */
	char joystickName[256];
	error=error||ioctl(newDevice.deviceFd,JSIOCGNAME(sizeof(joystickName)),joystickName)<0;
	
	if(error)
		{
		close(newDevice.deviceFd);
		Misc::throwStdErr("InputDeviceAdapterJoystick::createDevice: Could not query layout of device node %s",deviceNode.c_str());
		}
	
	/* Print a message: */
	newDevice.numButtons=nb;
	newDevice.numValuators=nv;
	std::cout<<"InputDeviceAdapterJoystick: Adding joystick "<<joystickName<<" with "<<newDevice.numButtons<<" buttons and "<<newDevice.numValuators<<" axes as device "<<name<<std::endl;
	
	/* Create new input device as a physical device: */
	newDevice.device=inputDeviceManager->createInputDevice(name.c_str(),InputDevice::TRACK_NONE,newDevice.numButtons,newDevice.numValuators,true);
	
	/* Store the new device structure: */
	devices.push_back(newDevice);
	
	/* Save the new input device: */
	inputDevices[deviceIndex]=newDevice.device;
	}

#endif

#ifdef __DARWIN__

void InputDeviceAdapterJoystick::createInputDevice(int deviceIndex,const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Create a new device structure: */
	Device newDevice;
	
	/* Read input device name: */
	newDevice.name=configFileSection.retrieveString("./name");
	
	/* Read HID device's vendor / product IDs: */
	std::string deviceVendorProductId=configFileSection.retrieveString("./deviceVendorProductId");
	
	/* Split ID string into vendor ID / product ID: */
	char* colonPtr;
	newDevice.vendorId=strtol(deviceVendorProductId.c_str(),&colonPtr,16);
	char* endPtr;
	newDevice.productId=strtol(colonPtr+1,&endPtr,16);
	if(*colonPtr!=':'||*endPtr!='\0'||newDevice.vendorId<0||newDevice.productId<0)
		Misc::throwStdErr("InputDeviceAdapterJoystick::InputDeviceAdapterJoystick: Malformed vendorId:productId string \"%s\" for device %s",deviceVendorProductId.c_str(),newDevice.name.c_str());
	
	/* Get the device index: */
	newDevice.deviceIndex=configFileSection.retrieveValue<int>("./deviceIndex",0);
	
	// DEBUGGING
	std::cout<<"Searching for device "<<std::hex<<newDevice.vendorId<<":"<<newDevice.productId<<", index "<<std::dec<<newDevice.deviceIndex<<std::endl;
	
	/* Set state indices to bogus values: */
	newDevice.firstButtonIndex=-1;
	newDevice.numButtons=0;
	newDevice.firstValuatorIndex=-1;
	newDevice.numValuators=0;
	
	/* Don't create Vrui input device: */
	newDevice.device=0;
	
	/* Store the new device structure: */
	devices.push_back(newDevice);
	}

#endif

#ifdef __DARWIN__

void InputDeviceAdapterJoystick::hidDeviceValueChangedCallback(IOReturn result,void* device,IOHIDValueRef newValue)
	{
	if(result!=kIOReturnSuccess)
		return;
	
	/* Get the element's descriptor: */
	ElementMap::Iterator elementIt=elementMap.findEntry(ElementKey(device,IOHIDElementGetCookie(IOHIDValueGetElement(newValue))));
	if(elementIt.isFinished())
		return;
	ElementDescriptor& ed=elementIt->getDest();
	
	/* Lock the device state: */
	{
	Threads::Mutex::Lock deviceStateLock(deviceStateMutex);
	
	/* Update the state arrays: */
	switch(ed.elementType)
		{
		case ElementDescriptor::BUTTON:
			/* Update a button state: */
			buttonStates[ed.index]=IOHIDValueGetIntegerValue(newValue)!=0;
			break;
		
		case ElementDescriptor::VALUATOR:
			/* Update a valuator state: */
			valuatorStates[ed.index]=ed.axisMapper.map(double(IOHIDValueGetIntegerValue(newValue)));
			break;
		
		case ElementDescriptor::HATSWITCH:
			{
			/* Update two valuators based on the hat switch's angle: */
			int value=IOHIDValueGetIntegerValue(newValue);
			if(value>=ed.hsMin&&value<=ed.hsMax)
				{
				/* Convert value into angle: */
				double angle=2.0*Math::Constants<double>::pi*double(value-ed.hsMin)/double(ed.hsMax+1-ed.hsMin);
				
				/* Update axis valuator states: */
				valuatorStates[ed.index+0]=Math::sin(angle);
				valuatorStates[ed.index+1]=Math::cos(angle);
				}
			else
				{
				/* Handle Null value: */
				valuatorStates[ed.index+0]=0.0;
				valuatorStates[ed.index+1]=0.0;
				}
			break;
			}
		}
	}
	
	/* Request a Vrui update: */
	requestUpdate();
	}

#endif

void* InputDeviceAdapterJoystick::devicePollingThreadMethod(void)
	{
	/* Enable immediate cancellation: */
	Threads::Thread::setCancelState(Threads::Thread::CANCEL_ENABLE);
	Threads::Thread::setCancelType(Threads::Thread::CANCEL_ASYNCHRONOUS);
	
	#ifdef __LINUX__
	/* Read device events until interrupted: */
	while(true)
		{
		/* Poll the device files of all devices: */
		Comm::FdSet deviceFds;
		for(std::vector<Device>::iterator dIt=devices.begin();dIt!=devices.end();++dIt)
			deviceFds.add(dIt->deviceFd);
		if(Comm::select(&deviceFds,0,0)>0)
			{
			/* Read events from all device files: */
			{
			Threads::Mutex::Lock deviceStateLock(deviceStateMutex);
			for(std::vector<Device>::iterator dIt=devices.begin();dIt!=devices.end();++dIt)
				if(deviceFds.isSet(dIt->deviceFd))
					{
					/* Attempt to read multiple events at once: */
					js_event events[32];
					ssize_t numEvents=read(dIt->deviceFd,events,sizeof(events)*sizeof(js_event));
					if(numEvents>0)
						{
						/* Process all read events in order: */
						numEvents/=sizeof(js_event);
						for(ssize_t i=0;i<numEvents;++i)
							{
							switch(events[i].type&~JS_EVENT_INIT)
								{
								case JS_EVENT_BUTTON:
									buttonStates[dIt->firstButtonIndex+int(events[i].number)]=events[i].value!=0;
									break;
								
								case JS_EVENT_AXIS:
									valuatorStates[dIt->firstValuatorIndex+int(events[i].number)]=double(events[i].value)/32767.0;
									break;
								}
							}
						}
					}
			}
			
			/* Request a Vrui update: */
			requestUpdate();
			}
		}
	#endif
	
	#ifdef __DARWIN__
	/* Associate the HID manager with the current run loop: */
	IOHIDManagerScheduleWithRunLoop(hidManager,CFRunLoopGetCurrent(),kCFRunLoopDefaultMode);
	
	/* Execute the run loop: */
	CFRunLoopRun();
	#endif
	
	return 0;
	}

#ifdef __DARWIN__

namespace {

/****************
Helper functions:
****************/

bool setDictionaryValue(CFMutableDictionaryRef dictionary,const char* key,long value)
	{
	/* Create a Core Foundation string for the key: */
	CFAutoRelease<CFStringRef> keyString=CFStringCreateWithCString(kCFAllocatorDefault,key,kCFStringEncodingUTF8);
	if(keyString==0)
		return false;
	
	/* Create a Core Foundation number for the value: */
	CFAutoRelease<CFNumberRef> valueNumber=CFNumberCreate(kCFAllocatorDefault,kCFNumberLongType,&value);
	if(valueNumber==0)
		return false;
	CFDictionaryAddValue(dictionary,keyString,valueNumber);
	
	return true;
	}

bool hidDeviceMatches(IOHIDDeviceRef device,long vendorId,long productId)
	{
	CFTypeRef vendorIdRef=IOHIDDeviceGetProperty(device,CFSTR(kIOHIDVendorIDKey));
	if(vendorIdRef==0||CFGetTypeID(vendorIdRef)!=CFNumberGetTypeID())
		return false;
	long dVendorId;
	if(!CFNumberGetValue(static_cast<CFNumberRef>(vendorIdRef),kCFNumberLongType,&dVendorId)||dVendorId!=vendorId)
		{
		std::cout<<"Vendor ID: "<<dVendorId<<" vs "<<vendorId<<std::endl;
		return false;
		}
	CFTypeRef productIdRef=IOHIDDeviceGetProperty(device,CFSTR(kIOHIDProductIDKey));
	if(productIdRef==0||CFGetTypeID(productIdRef)!=CFNumberGetTypeID())
		return false;
	long dProductId;
	if(!CFNumberGetValue(static_cast<CFNumberRef>(productIdRef),kCFNumberLongType,&dProductId)||dProductId!=productId)
		{
		std::cout<<"Product ID: "<<dProductId<<" vs "<<productId<<std::endl;
		return false;
		}
	
	return true;
	}

std::ostream& operator<<(std::ostream& os,CFStringRef string)
	{
	/* Try direct conversion first: */
	if(CFStringGetCStringPtr(string,kCFStringEncodingUTF8)!=0)
		os<<string;
	else
		{
		/* Extract string the hard way: */
		CFIndex bufferSize=CFStringGetLength(string)+1;
		char* buffer=new char[bufferSize];
		CFStringGetCString(string,buffer,bufferSize,kCFStringEncodingUTF8);
		os<<buffer;
		delete[] buffer;
		}
	return os;
	}

}

#endif

InputDeviceAdapterJoystick::InputDeviceAdapterJoystick(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection)
	:InputDeviceAdapter(sInputDeviceManager),
	 buttonStates(0),valuatorStates(0)
	 #ifdef __DARWIN__
	 ,hidManager(0),elementMap(31)
	 #endif
	{
	#ifdef __DARWIN__
	/* Get a reference to the HID manager: */
	hidManager=IOHIDManagerCreate(kCFAllocatorDefault,kIOHIDOptionsTypeNone);
	if(hidManager==0)
		Misc::throwStdErr("InputDeviceAdapterJoystick::InputDeviceAdapterJoystick: Could not access HID manager");
	#endif
	
	/* Initialize input device adapter: */
	InputDeviceAdapter::initializeAdapter(configFileSection);
	
	#ifdef __DARWIN__
	{
	/* Create a set of dictionaries to match the vendor / product IDs of all configured devices: */
	CFAutoRelease<CFMutableArrayRef> dictionarySet=CFArrayCreateMutable(kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);
	if(dictionarySet==0)
		Misc::throwStdErr("InputDeviceAdapterJoystick::InputDeviceAdapterJoystick: Could not create dictionary set");
	for(std::vector<Device>::iterator dIt=devices.begin();dIt!=devices.end();++dIt)
		{
		/* Create a dictionary to match the device's vendor / product IDs: */
		CFAutoRelease<CFMutableDictionaryRef> dictionary=CFDictionaryCreateMutable(kCFAllocatorDefault,2,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
		if(dictionary==0)
			Misc::throwStdErr("InputDeviceAdapterJoystick::InputDeviceAdapterJoystick: Could not create dictionary");
		
		/* Add keys for vendor and product IDs: */
		if(!setDictionaryValue(dictionary,kIOHIDVendorIDKey,dIt->vendorId)||!setDictionaryValue(dictionary,kIOHIDProductIDKey,dIt->productId))
			Misc::throwStdErr("InputDeviceAdapterJoystick::InputDeviceAdapterJoystick: Could not set product / vendor IDs in dictionary");
		
		/* Add the dictionary to the set: */
		CFArrayAppendValue(dictionarySet,dictionary);
		}
	
	/* Install the dictionary set with the HID manager: */
	IOHIDManagerSetDeviceMatchingMultiple(hidManager,dictionarySet);
	}
	
	/* Open the HID manager: */
	if(IOHIDManagerOpen(hidManager,kIOHIDOptionsTypeNone)!=kIOReturnSuccess)
		Misc::throwStdErr("InputDeviceAdapterJoystick::InputDeviceAdapterJoystick: Could not open HID manager");
	
	int totalNumButtons=0;
	int totalNumValuators=0;
	
	{
	/* Get the set of matching devices: */
	CFAutoRelease<CFSetRef> deviceSet=IOHIDManagerCopyDevices(hidManager);
	if(deviceSet==0||CFSetGetCount(deviceSet)==0)
		Misc::throwStdErr("InputDeviceAdapterJoystick::InputDeviceAdapterJoystick: No HID devices found");
	
	/* Access the found device handles: */
	CFIndex numHidDevices=CFSetGetCount(deviceSet);
	IOHIDDeviceRef* hidDevices=new IOHIDDeviceRef[numHidDevices];
	CFSetGetValues(deviceSet,const_cast<const void**>(reinterpret_cast<void**>(hidDevices)));
	
	/* Process all configured devices: */
	int deviceIndex=0;
	for(std::vector<Device>::iterator dIt=devices.begin();dIt!=devices.end();++dIt)
		{
		dIt->firstButtonIndex=totalNumButtons;
		dIt->firstValuatorIndex=totalNumValuators;
		
		/* Find the device in the list of HID devices: */
		IOHIDDeviceRef device=0;
		int matched=0;
		for(CFIndex i=0;i<numHidDevices;++i)
			{
			/* Check if the HID device matches the configured device's vendor / product ID: */
			if(hidDeviceMatches(hidDevices[i],dIt->vendorId,dIt->productId))
				{
				/* Check if it's the configured index in a set of identical devices: */
				if(matched==dIt->deviceIndex)
					{
					/* Found it! */
					device=hidDevices[i];
					break;
					}
				
				/* Keep looking: */
				++matched;
				}
			}
		if(device!=0)
			{
			/* Print a message: */
			CFTypeRef deviceNameObject=IOHIDDeviceGetProperty(device,CFSTR(kIOHIDProductKey));
			if(deviceNameObject!=0&&CFGetTypeID(deviceNameObject)==CFStringGetTypeID())
				std::cout<<"InputDeviceAdapterJoystick::InputDeviceAdapterJoystick: Adding device "<<static_cast<CFStringRef>(deviceNameObject)<<std::endl;
			
			/* Get all elements on the device: */
			CFAutoRelease<CFArrayRef> elements=IOHIDDeviceCopyMatchingElements(device,0,kIOHIDOptionsTypeNone);
			if(elements!=0)
				{
				/* Iterate through the element list: */
				CFIndex numElements=CFArrayGetCount(elements);
				for(CFIndex i=0;i<numElements;++i)
					{
					CFTypeRef elementObject=CFArrayGetValueAtIndex(elements,i);
					if(elementObject!=0&&CFGetTypeID(elementObject)==IOHIDElementGetTypeID())
						{
						IOHIDElementRef element=static_cast<IOHIDElementRef>(const_cast<void*>(elementObject));
						switch(IOHIDElementGetType(element))
							{
							case kIOHIDElementTypeInput_Button:
								{
								/* Add a button element to the device: */
								ElementKey ek(device,IOHIDElementGetCookie(element));
								ElementDescriptor ed;
								ed.elementType=ElementDescriptor::BUTTON;
								ed.index=dIt->firstButtonIndex+dIt->numButtons;
								elementMap.setEntry(ElementMap::Entry(ek,ed));
								++dIt->numButtons;
								break;
								}
							
							case kIOHIDElementTypeInput_Misc:
							case kIOHIDElementTypeInput_Axis:
								{
								/* Check the element's usage for a hat switch control: */
								if(IOHIDElementGetUsagePage(element)==0x01&&IOHIDElementGetUsage(element)==0x39)
									{
									/* Add a hat switch to the device: */
									ElementKey ek(device,IOHIDElementGetCookie(element));
									ElementDescriptor ed;
									ed.elementType=ElementDescriptor::HATSWITCH;
									ed.index=dIt->firstValuatorIndex+dIt->numValuators;
									ed.hsMin=int(IOHIDElementGetLogicalMin(element));
									ed.hsMax=int(IOHIDElementGetLogicalMax(element));
									elementMap.setEntry(ElementMap::Entry(ek,ed));
									dIt->numValuators+=2;
									}
								else
									{
									/* Add a valuator to the device: */
									ElementKey ek(device,IOHIDElementGetCookie(element));
									ElementDescriptor ed;
									ed.elementType=ElementDescriptor::VALUATOR;
									ed.index=dIt->firstValuatorIndex+dIt->numValuators;
									ed.axisMapper=ElementDescriptor::AxisMapper(double(IOHIDElementGetLogicalMin(element)),double(IOHIDElementGetLogicalMax(element)));
									elementMap.setEntry(ElementMap::Entry(ek,ed));
									++dIt->numValuators;
									}
								break;
								}
							
							default:
								/* Just to make compiler happy... */
								;
							}
						}
					}
				
				/* Create new input device as a physical device: */
				dIt->device=inputDeviceManager->createInputDevice(dIt->name.c_str(),InputDevice::TRACK_NONE,dIt->numButtons,dIt->numValuators,true);
				
				/* Register a value change callback with the HID device: */
				IOHIDDeviceRegisterInputValueCallback(device,hidDeviceValueChangedCallbackWrapper,this);
				}
			else
				std::cout<<"Ignoring device "<<dIt->name<<" since the its elements could not be enumerated"<<std::endl;
			}
		else
			std::cout<<"Ignoring device "<<dIt->name<<" since no matching HID device was found"<<std::endl;
		
		/* Save the new input device: */
		inputDevices[deviceIndex]=dIt->device;
		
		totalNumButtons+=dIt->numButtons;
		totalNumValuators+=dIt->numValuators;
		}
	
	delete[] hidDevices;
	}
	#else
	/* Count the total number of buttons and valuators: */
	int totalNumButtons=0;
	int totalNumValuators=0;
	for(std::vector<Device>::iterator dIt=devices.begin();dIt!=devices.end();++dIt)
		{
		dIt->firstButtonIndex=totalNumButtons;
		totalNumButtons+=dIt->numButtons;
		dIt->firstValuatorIndex=totalNumValuators;
		totalNumValuators+=dIt->numValuators;
		}
	#endif
	
	/* Create the device state arrays: */
	buttonStates=new bool[totalNumButtons];
	for(int i=0;i<totalNumButtons;++i)
		buttonStates[i]=false;
	valuatorStates=new double[totalNumValuators];
	for(int i=0;i<totalNumValuators;++i)
		valuatorStates[i]=0.0;
	
	/* Start the device polling thread: */
	devicePollingThread.start(this,&InputDeviceAdapterJoystick::devicePollingThreadMethod);
	}

InputDeviceAdapterJoystick::~InputDeviceAdapterJoystick(void)
	{
	/* Shut down the device polling thread: */
	{
	Threads::Mutex::Lock deviceStateLock(deviceStateMutex);
	devicePollingThread.cancel();
	devicePollingThread.join();
	}
	
	/* Delete the state arrays: */
	delete[] buttonStates;
	delete[] valuatorStates;
	
	#ifdef __LINUX__
	/* Close all device files: */
	for(std::vector<Device>::iterator dIt=devices.begin();dIt!=devices.end();++dIt)
		close(dIt->deviceFd);
	#endif
	}

void InputDeviceAdapterJoystick::updateInputDevices(void)
	{
	/* Copy the current device state array into the input devices: */
	Threads::Mutex::Lock deviceStateLock(deviceStateMutex);
	
	for(std::vector<Device>::iterator dIt=devices.begin();dIt!=devices.end();++dIt)
		{
		/* Set the device's button and valuator states: */
		for(int i=0;i<dIt->numButtons;++i)
			dIt->device->setButtonState(i,buttonStates[dIt->firstButtonIndex+i]);
		for(int i=0;i<dIt->numValuators;++i)
			dIt->device->setValuator(i,valuatorStates[dIt->firstValuatorIndex+i]);
		}
	}

}
