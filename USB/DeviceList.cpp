/***********************************************************************
DeviceList - Class representing lists of USB devices resulting from
device enumeration.
Copyright (c) 2010-2011 Oliver Kreylos

This file is part of the USB Support Library (USB).

The USB Support Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The USB Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the USB Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <USB/DeviceList.h>

#include <libusb-1.0/libusb.h>
#include <Misc/ThrowStdErr.h>
#include <USB/Context.h>

namespace USB {

/***************************
Methods of class DeviceList:
***************************/

DeviceList::DeviceList(const Context& context)
	:numDevices(0),deviceList(0)
	{
	/* Enumerate devices in the given context: */
	ssize_t enumerateResult=libusb_get_device_list(context.getContext(),&deviceList);
	if(enumerateResult>=0)
		numDevices=size_t(enumerateResult);
	else
		{
		deviceList=0;
		Misc::throwStdErr("USB::DeviceList::DeviceList: Error while enumerating USB devices");
		}
	}

DeviceList::~DeviceList(void)
	{
	if(numDevices>0)
		{
		/* Unreference all devices and destroy the list: */
		libusb_free_device_list(deviceList,1);
		}
	}

size_t DeviceList::getNumDevices(unsigned short idVendor,unsigned short idProduct) const
	{
	size_t result=0;
	for(size_t i=0;i<numDevices;++i)
		{
		/* Get the device's descriptor: */
		libusb_device_descriptor descriptor;
		if(libusb_get_device_descriptor(deviceList[i],&descriptor)==0)
			{
			/* Check the vendor/product ID: */
			if(descriptor.idVendor==idVendor&&descriptor.idProduct==idProduct)
				{
				/* Found one! */
				++result;
				}
			}
		}
	return result;
	}

libusb_device* DeviceList::getDevice(unsigned short idVendor,unsigned short idProduct,size_t index) const
	{
	libusb_device* result=0;
	for(size_t i=0;result==0&&i<numDevices;++i)
		{
		/* Get the device's descriptor: */
		libusb_device_descriptor descriptor;
		if(libusb_get_device_descriptor(deviceList[i],&descriptor)==0)
			{
			/* Check the vendor/product ID: */
			if(descriptor.idVendor==idVendor&&descriptor.idProduct==idProduct)
				{
				/* Found a match! */
				if(index>0) // Not the one
					--index;
				else // The one!
					result=deviceList[i];
				}
			}
		}
	return result;
	}

}
