/***********************************************************************
SerialPort - Class to simplify using serial ports.
Copyright (c) 2001-2005 Oliver Kreylos

This file is part of the Portable Communications Library (Comm).

The Portable Communications Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Portable Communications Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Portable Communications Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef COMM_SERIALPORT_INCLUDED
#define COMM_SERIALPORT_INCLUDED

#include <utility>
#include <stdexcept>

/* Forward declarations: */
namespace Misc {
class Time;
}

namespace Comm {

class SerialPort
	{
	/* Embedded classes: */
	public:
	enum ParitySettings
		{
		PARITY_NONE,PARITY_EVEN,PARITY_ODD
		};
	
	enum PortSettings
		{
		BLOCKING=0x0,NONBLOCKING=0x1
		};
	
	class OpenError:public std::runtime_error // Exception class to report port opening errors
		{
		/* Constructors and destructors: */
		public:
		OpenError(const char* portName)
			:std::runtime_error(std::string("Error opening serial port ")+std::string(portName))
			{
			}
		};
	
	class ReadError:public std::runtime_error // Exception class to report errors reading from port
		{
		/* Constructors and destructors: */
		public:
		ReadError(void)
			:std::runtime_error("Error reading from serial port ")
			{
			}
		};
	
	class WriteError:public std::runtime_error // Exception class to report errors writing to port
		{
		/* Constructors and destructors: */
		public:
		WriteError(void)
			:std::runtime_error("Error writing to serial port ")
			{
			}
		};
	
	/* Elements: */
	private:
	int port; // I/O handle for the serial port
	size_t totalBytesReceived,totalBytesSent; // Number of bytes sent/received
	size_t numReadSpins,numWriteSpins; // Number of unsuccessful read/write attempts
	
	/* Private methods: */
	void readBlocking(size_t numBytes,char* bytes); // Blocking read
	void writeBlocking(size_t numBytes,const char* bytes); // Blocking write
	
	/* Constructors and destructors: */
	public:
	SerialPort(const char* deviceName); // Opens the given device as a "raw" port for I/O
	~SerialPort(void); // Closes the serial port
	
	/* Methods: */
	int getFd(void) const // Returns low-level file descriptor for serial port
		{
		return port;
		}
	void setPortSettings(int portSettingsMask); // Sets port file descriptor settings
	void setSerialSettings(int bitRate,int charLength,ParitySettings parity,int numStopbits,bool enableHandshake); // Sets serial port parameters
	void setRawMode(int minNumBytes,int timeout); // Switches port to "raw" mode and sets burst parameters
	void setCanonicalMode(void); // Switches port to canonical mode
	void setLineControl(bool respectModemLines,bool hangupOnClose); // Sets line control parameters
	size_t getTotalBytesReceived(void) const
		{
		return totalBytesReceived;
		}
	size_t getTotalBytesSent(void) const
		{
		return totalBytesSent;
		}
	size_t getNumReadSpins(void) const
		{
		return numReadSpins;
		}
	size_t getNumWriteSpins(void) const
		{
		return numWriteSpins;
		}
	void resetStatistics(void); // Resets the byte and spin counters
	bool waitForByte(const Misc::Time& timeout); // Waits for a byte to appear on the serial port within the given timeout, returns true if byte can be read
	std::pair<char,bool> readByteNonBlocking(void); // Reads at most one byte from the port, second pair item is true if a byte was read
	char readByte(void) // Reads a single byte from the port
		{
		char result;
		readBlocking(1,&result);
		return result;
		}
	char* readBytes(size_t numBytes) // Reads a block of bytes; allocates a buffer using new[]
		{
		char* bytes=new char[numBytes];
		readBlocking(numBytes,bytes);
		return bytes;
		}
	char* readBytes(size_t numBytes,char* bytes) // Reads a block of bytes
		{
		readBlocking(numBytes,bytes);
		return bytes;
		}
	size_t readBytesRaw(size_t maxNumBytes,char* bytes); // Reads up to maximum number of available bytes; returns number of bytes read
	void writeByte(char byte) // Writes a single byte to the port
		{
		writeBlocking(1,&byte);
		}
	void writeBytes(size_t numBytes,const char* bytes) // Writes a block of bytes
		{
		writeBlocking(numBytes,bytes);
		}
	void writeString(const char* string); // Writes a null-terminated string to the port
	void flush(void); // Flushes the output queue
	void drain(void); // Waits until all pending writes have completed
	};

}

#endif
