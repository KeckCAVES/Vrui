/***********************************************************************
TransferPool - Class to manage a pool of USB transfer buffers for
asynchronous bulk or isochronous transmission.
Copyright (c) 2014-2018 Oliver Kreylos

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

#include <USB/TransferPool.h>

#include <unistd.h>
#include <stdexcept>
#include <Misc/ThrowStdErr.h>
#include <Misc/MessageLogger.h>
#include <Misc/FunctionCalls.h>
#include <USB/Config.h>
#include <USB/Device.h>

namespace USB {

/*****************************
Methods of class TransferPool:
*****************************/

bool TransferPool::allocateTransfers(void)
	{
	/* Allocate the transfer backing buffer: */
	buffer=new unsigned char[size_t(numTransfers)*transferSize];
	
	/* Allocate all USB transfer objects: */
	transfers=new Transfer[numTransfers];
	bool ok=true;
	for(unsigned int i=0;i<numTransfers&&ok;++i)
		ok=(transfers[i].transfer=libusb_alloc_transfer(numPackets))!=0;
	
	if(ok)
		{
		/* Initialize the list of unused transfer objects: */
		for(unsigned int i=0;i<numTransfers;++i)
			unused.push_back(&transfers[i]);
		}
	else
		{
		/* Clean up after failed transfer object allocation: */
		for(unsigned int i=0;i<numTransfers;++i)
			libusb_free_transfer(transfers[i].transfer);
		delete[] transfers;
		delete[] buffer;
		}
	
	return ok;
	}

void TransferPool::transferCallback(libusb_transfer* transfer)
	{
	/* Get the object pointer: */
	TransferPool* thisPtr=static_cast<TransferPool*>(transfer->user_data);
	
	/* Remove the transfer from the active list and submit a new unused one: */
	Transfer* tli;
	{
	Threads::Spinlock::Lock listLock(thisPtr->listMutex);
	
	tli=thisPtr->active.findAndRemove(transfer);
	
	++thisPtr->activeDeficit;
	if(!thisPtr->cancelling)
		{
		/* Submit currently unused transfer blocks until the active pool is back at its intended size: */
		while(thisPtr->activeDeficit>0U&&!thisPtr->unused.empty())
			{
			int submitResult=libusb_submit_transfer(thisPtr->unused.front()->transfer);
			if(submitResult==0)
				{
				thisPtr->active.push_back(thisPtr->unused.pop_front());
				--thisPtr->activeDeficit;
				}
			else
				break;
			}
		
		if(thisPtr->active.empty())
			Misc::consoleError("USB::TransferPool: Buffer underrun, transmission stalled");
		}
	}
	
	if(transfer->status==LIBUSB_TRANSFER_COMPLETED&&!thisPtr->cancelling)
		{
		/* Call the end-user callback: */
		(*thisPtr->userTransferCallback)(tli);
		}
	}

TransferPool::TransferPool(unsigned int sNumTransfers,size_t sTransferSize)
	:numTransfers(sNumTransfers),
	 numPackets(0),packetSize(0),
	 transferSize(sTransferSize),
	 buffer(0),transfers(0),
	 activeDeficit(0),
	 cancelling(false),
	 userTransferCallback(0)
	{
	if(!allocateTransfers())
		throw std::runtime_error("USB::TransferPool: Error while allocating USB transfer objects");
	}

TransferPool::TransferPool(unsigned int sNumTransfers,unsigned int sNumPackets,size_t sPacketSize)
	:numTransfers(sNumTransfers),
	 numPackets(sNumPackets),packetSize(sPacketSize),
	 transferSize(size_t(numPackets)*packetSize),
	 buffer(0),transfers(0),
	 activeDeficit(0),
	 cancelling(false),
	 userTransferCallback(0)
	{
	if(!allocateTransfers())
		throw std::runtime_error("USB::TransferPool: Error while allocating USB transfer objects");
	}

TransferPool::~TransferPool(void)
	{
	/* Cancel all still-active transfers: */
	cancel();
	
	/* Destroy all transfer objects: */
	for(unsigned int i=0;i<numTransfers;++i)
		libusb_free_transfer(transfers[i].transfer);
	delete[] transfers;
	
	/* Free the backing buffer: */
	delete[] buffer;
	}

void TransferPool::submit(Device& device,unsigned int endpoint,unsigned int numActiveTransfers,TransferPool::UserTransferCallback* newUserTransferCallback)
	{
	/* Prepare all transfer objects: */
	unsigned char* bufferPtr=buffer;
	for(unsigned int i=0;i<numTransfers;++i,bufferPtr+=transferSize)
		{
		if(numPackets==0||packetSize==0)
			{
			/* Prepare a bulk transfer: */
			libusb_fill_bulk_transfer(transfers[i].transfer,device.getDeviceHandle(),endpoint,bufferPtr,transferSize,transferCallback,this,0);
			}
		else
			{
			/* Prepare an isochronous transfer: */
			libusb_fill_iso_transfer(transfers[i].transfer,device.getDeviceHandle(),endpoint,bufferPtr,transferSize,numPackets,transferCallback,this,0);
			libusb_set_iso_packet_lengths(transfers[i].transfer,packetSize);
			}
		}
	
	/* Store the user transfer callback: */
	userTransferCallback=newUserTransferCallback;
	
	/* Submit the given number of transfer objects: */
	unsigned int numSubmittedTransfers=0;
	{
	Threads::Spinlock::Lock listLock(listMutex);
	for(unsigned int i=0;i<numActiveTransfers;++i)
		{
		/* Submit the first unused transfer object: */
		int submitResult=libusb_submit_transfer(unused.front()->transfer);
		if(submitResult==0)
			{
			/* Move the just submitted transfer from the unused to the active list: */
			active.push_back(unused.pop_front());
			++numSubmittedTransfers;
			}
		else
			{
			#if USB_CONFIG_HAVE_STRERROR
			Misc::formattedConsoleError("USB::TransferPool: Unable to submit transfer due to error %s",libusb_strerror(libusb_error(submitResult)));
			#else
			Misc::formattedConsoleError("USB::TransferPool: Unable to submit transfer due to error %s",libusb_error_name(submitResult));
			#endif
			}
		}
	}
	if(numSubmittedTransfers<numActiveTransfers)
		Misc::formattedConsoleError("USB::TransferPool: Failed submitting %u out of %u requested transfers",numActiveTransfers-numSubmittedTransfers,numActiveTransfers);
	
	/* Keep track of the number of transfers that need to be submitted at the next opportunity: */
	activeDeficit=numActiveTransfers-numSubmittedTransfers;
	}

void TransferPool::cancel(void)
	{
	/* Cancel all active transfers: */
	{
	Threads::Spinlock::Lock listLock(listMutex);
	cancelling=true;
	for(Transfer* tli=active.front();tli!=0;tli=tli->succ)
		libusb_cancel_transfer(tli->transfer);
	}
	
	/* Wait for all cancellations to complete: */
	while(true)
		{
		{
		Threads::Spinlock::Lock listLock(listMutex);
		if(active.empty())
			{
			cancelling=false;
			break;
			}
		}
		
		/* Wait for a bit: */
		usleep(1000);
		}
	
	activeDeficit=0;
	
	/* Delete the user transfer callback: */
	delete userTransferCallback;
	userTransferCallback=0;
	}

void TransferPool::release(TransferPool::Transfer* transfer)
	{
	/* Put the given transfer list item at the end of the unused list: */
	Threads::Spinlock::Lock listLock(listMutex);
	unused.push_back(transfer);
	}

}
