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

#ifndef USB_TRANSFERPOOL_INCLUDED
#define USB_TRANSFERPOOL_INCLUDED

#include <stddef.h>
#include <libusb-1.0/libusb.h>
#include <Threads/Spinlock.h>

/* Forward declarations: */
namespace Misc {
template <class ParameterParam>
class FunctionCall;
}
namespace USB {
class Device;
}

namespace USB {

class TransferPool
	{
	/* Embedded classes: */
	public:
	class TransferQueue; // Forward declaration
	
	class Transfer // Structure for items in lists of transfer objects
		{
		friend class TransferPool;
		friend class TransferQueue;
		
		/* Elements: */
		private:
		libusb_transfer* transfer; // Pointer to the USB transfer object
		Transfer* succ; // Pointer to the next item in the transfer object list
		
		/* Constructors and destructors: */
		Transfer(void) // Constructs an invalid transfer list item
			:transfer(0),succ(0)
			{
			}
		
		/* Methods: */
		public:
		const libusb_transfer& getTransfer(void) const // Returns the USB transfer object
			{
			return *transfer;
			}
		bool isIsochronous(void) const // Returns true if the transfer is isochronous
			{
			return transfer->type==LIBUSB_TRANSFER_TYPE_ISOCHRONOUS;
			}
		enum libusb_transfer_status getStatus(void) const // Returns the status of a non-isochronous transfer
			{
			return transfer->status;
			}
		bool isCompleted(void) const // Returns true if a non-isochronous transfer completed successfully
			{
			return transfer->status==LIBUSB_TRANSFER_COMPLETED;
			}
		int getSize(void) const // Returns the buffer size of a non-isochronous transfer
			{
			return transfer->length;
			}
		int getDataSize(void) const // Returns the actual amount of data in a non-isochronous transfer
			{
			return transfer->actual_length;
			}
		const unsigned char* getData(void) const // Returns the data in a non-isochronous transfer
			{
			return transfer->buffer;
			}
		int getNumPackets(void) const // Returns the number of packets in an isochronous transfer
			{
			return transfer->num_iso_packets;
			}
		const libusb_iso_packet_descriptor& getPacketDescriptor(int packetIndex) const // Returns the USB transfer packet descriptor for the given isochronous transfer packet
			{
			return transfer->iso_packet_desc[packetIndex];
			}
		enum libusb_transfer_status getPacketStatus(int packetIndex) const // Returns the status of the given isochronous transfer packet
			{
			return transfer->iso_packet_desc[packetIndex].status;
			}
		bool isPacketCompleted(int packetIndex) const // Returns true if the given isochronous transfer packet completed successfully
			{
			return transfer->iso_packet_desc[packetIndex].status==LIBUSB_TRANSFER_COMPLETED;
			}
		unsigned int getPacketSize(int packetIndex) const // Returns the buffer size of the given isochronous transfer packet
			{
			return transfer->iso_packet_desc[packetIndex].length;
			}
		unsigned int getPacketDataSize(int packetIndex) const // Returns the actual amount of data in the given isochronous transfer packet
			{
			return transfer->iso_packet_desc[packetIndex].actual_length;
			}
		const unsigned char* getPacketData(int packetIndex) const // Returns the data in an isochronous transfer packet
			{
			return libusb_get_iso_packet_buffer_simple(transfer,packetIndex);
			}
		};
	
	class TransferQueue // Class for queues of transfer objects
		{
		/* Elements: */
		private:
		Transfer* head; // Pointer to first element, or 0 for empty queue
		Transfer* tail; // Pointer to last element, or 0 for empty queue
		
		/* Constructors and destructors: */
		public:
		TransferQueue(void) // Creates an empty transfer queue
			:head(0),tail(0)
			{
			}
		
		/* Methods: */
		bool empty(void) const // Returns true if the queue is empty
			{
			return head==0;
			}
		Transfer* front(void) // Returns the first queued transfer; queue must not be empty
			{
			return head;
			}
		Transfer* pop_front(void) // Removes the first queued transfer; queue must not be empty
			{
			Transfer* oldHead=head;
			head=head->succ;
			if(head==0)
				tail=0;
			oldHead->succ=0;
			
			return oldHead;
			}
		void push_back(Transfer* transfer) // Appends the given transfer to the end of the queue
			{
			transfer->succ=0;
			if(tail!=0)
				tail->succ=transfer;
			else
				head=transfer;
			tail=transfer;
			}
		Transfer* findAndRemove(libusb_transfer* usbTransfer) // Removes and returns the transfer that contains the given transfer object
			{
			/* Find the given USB transfer object: */
			Transfer* pred=0;
			Transfer* t;
			for(t=head;t!=0&&t->transfer!=usbTransfer;pred=t,t=t->succ)
				;
			
			if(t!=0)
				{
				/* Remove the transfer from the queue: */
				if(pred!=0)
					pred->succ=t->succ;
				else
					head=t->succ;
				if(t->succ==0)
					tail=pred;
				t->succ=0;
				}
			
			return t;
			}
		};
	
	typedef Misc::FunctionCall<Transfer*> UserTransferCallback; // Type for functions called when a USB transfer completes
	
	/* Elements: */
	private:
	unsigned int numTransfers; // Number of transfer objects in the pool
	unsigned int numPackets; // Number of packets per transfer objects (0 for bulk transfers)
	size_t packetSize; // Size of one transfer packet in bytes (0 for bulk transfers)
	size_t transferSize; // Total size of one transfer in bytes
	unsigned char* buffer; // Pointer to beginning of memory buffer backing all allocated USB transfer objects
	Transfer* transfers; // Array of pointers to USB transfer objects
	Threads::Spinlock listMutex; // Mutex serializing access to both transfer object lists
	TransferQueue active; // List of transfer objects awaiting data from the USB endpoint
	TransferQueue unused; // List of transfer objects available to receive data
	size_t activeDeficit; // Amount by which the current active transfer pool is smaller than the requested size
	bool cancelling; // Flag to indicate that a transfer request is being cancelled
	UserTransferCallback* userTransferCallback; // Function called when a USB transfer completes
	
	/* Private methods: */
	bool allocateTransfers(void);
	static void transferCallback(libusb_transfer* transfer);
	
	/* Constructors and destructors: */
	public:
	TransferPool(unsigned int sNumTransfers,size_t sTransferSize); // Creates a pool of non-isochronous USB transfer buffers
	TransferPool(unsigned int sNumTransfers,unsigned int sNumPackets,size_t sPacketSize); // Creates a pool of isochronous USB transfer buffers
	~TransferPool(void); // Releases all allocated resources
	
	/* Methods: */
	void submit(Device& device,unsigned int endpoint,unsigned int numActiveTransfers,UserTransferCallback* newUserTransferCallback); // Submits the given number of transfer objects for the given endpoint on the given device
	void cancel(void); // Cancels all active transfer requests and resets the pool for another submit(...) call
	void release(Transfer* transfer); // Releases a locked transfer object back to the unused list
	};

}

#endif

