/***********************************************************************
BufferedSerialPort - Class for high-performance reading/writing from/to
serial port.
Copyright (c) 2011 Oliver Kreylos

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

#include <Comm/BufferedSerialPort.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/FdSet.h>

namespace Comm {

/***********************************
Methods of class BufferedSerialPort:
***********************************/

size_t BufferedSerialPort::readData(IO::File::Byte* buffer,size_t bufferSize)
	{
	/* Read more data from source: */
	ssize_t readResult;
	do
		{
		readResult=::read(fd,buffer,bufferSize);
		}
	while(readResult<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR));
	
	/* Handle the result from the read call: */
	if(readResult<0)
		{
		/* Unknown error; probably a bad thing: */
		Misc::throwStdErr("Comm::BufferedSerialPort: Fatal error while reading from source");
		}
	
	return size_t(readResult);
	}

void BufferedSerialPort::writeData(const IO::File::Byte* buffer,size_t bufferSize)
	{
	while(bufferSize>0)
		{
		ssize_t writeResult=::write(fd,buffer,bufferSize);
		if(writeResult>0)
			{
			/* Prepare to write more data: */
			buffer+=writeResult;
			bufferSize-=writeResult;
			}
		else if(writeResult<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR))
			{
			/* Do nothing */
			}
		else if(writeResult==0)
			{
			/* Sink has reached end-of-file: */
			throw WriteError(bufferSize);
			}
		else
			{
			/* Unknown error; probably a bad thing: */
			Misc::throwStdErr("Comm::BufferedSerialPort: Fatal error while writing to sink");
			}
		}
	}

BufferedSerialPort::BufferedSerialPort(const char* deviceName)
	:Pipe(ReadWrite),
	 fd(-1)
	{
	/* Open the device file: */
	fd=open(deviceName,O_RDWR|O_NOCTTY|O_NDELAY);
	if(fd<0)
		throw OpenError(Misc::printStdErrMsg("Comm::BufferedSerialPort: Unable to open device %s",deviceName));
	
	/* Configure as "raw" port: */
	struct termios term;
	tcgetattr(fd,&term);
	cfmakeraw(&term);
	term.c_iflag|=IGNBRK; // Don't generate signals
	term.c_cflag|=CREAD|CLOCAL; // Enable receiver; no modem line control
	term.c_cc[VMIN]=1; // Block read() until at least a single byte is read
	term.c_cc[VTIME]=0; // No timeout on read()
	if(tcsetattr(fd,TCSANOW,&term)!=0)
		throw OpenError(Misc::printStdErrMsg("Comm::BufferedSerialPort: Unable to configure device %s",deviceName));
	
	/* Flush both queues: */
	tcflush(fd,TCIFLUSH);
	tcflush(fd,TCOFLUSH);
	}

BufferedSerialPort::~BufferedSerialPort(void)
	{
	if(fd>=0)
		close(fd);
	}

int BufferedSerialPort::getFd(void) const
	{
	return fd;
	}

bool BufferedSerialPort::waitForData(void) const
	{
	/* Check if there is unread data in the buffer: */
	if(getUnreadDataSize()>0)
		return true;
	
	/* Wait for data on the socket and return whether data is available: */
	Misc::FdSet readFds(fd);
	return Misc::pselect(&readFds,0,0,0)>=0&&readFds.isSet(fd);
	}

bool BufferedSerialPort::waitForData(const Misc::Time& timeout) const
	{
	/* Check if there is unread data in the buffer: */
	if(getUnreadDataSize()>0)
		return true;
	
	/* Wait for data on the socket and return whether data is available: */
	Misc::FdSet readFds(fd);
	return Misc::pselect(&readFds,0,0,timeout)>=0&&readFds.isSet(fd);
	}

void BufferedSerialPort::shutdown(bool read,bool write)
	{
	/* Flush the write buffer: */
	flush();
	
	if(write)
		{
		/* Drain the port's buffer: */
		tcdrain(fd);
		}
	}

void BufferedSerialPort::setPortSettings(int portSettingsMask)
	{
	/* Retrieve current flags: */
	int fileFlags=fcntl(fd,F_GETFL);
	
	/* Change flags according to given parameter: */
	if(portSettingsMask&NonBlocking)
		fileFlags|=FNDELAY|FNONBLOCK;
	else
		fileFlags&=~(FNDELAY|FNONBLOCK);
	
	/* Set new flags: */
	if(fcntl(fd,F_SETFL,fileFlags)!=0)
		Misc::throwStdErr("Comm::BufferedSerialPort::setPortSettings: Unable to configure device");
	}

void BufferedSerialPort::setSerialSettings(int bitRate,int charLength,BufferedSerialPort::Parity parity,int numStopbits,bool enableHandshake)
	{
	/* Initialize a termios structure: */
	struct termios term;
	if(tcgetattr(fd,&term)!=0)
		Misc::throwStdErr("Comm::BufferedSerialPort::setSerialSettings: Unable to read device configuration");
	
	/* Set rate of bits per second: */
	#ifdef __SGI_IRIX__
	term.c_ospeed=bitRate;
	term.c_ispeed=bitRate;
	#else
	/* Find the closest existing bitrate to the given one: */
	static speed_t bitRates[][2]={{0,B0},{50,B50},{75,B75},{110,B110},{134,B134},{150,B150},
	                              {200,B200},{300,B300},{600,B600},{1200,B1200},{1800,B1800},
	                              {2400,B2400},{4800,B4800},{9600,B9600},{19200,B19200},
	                              {38400,B38400},{57600,B57600},{115200,B115200},{230400,B230400}};
	int l=0;
	int r=19;
	while(r-l>1)
		{
		int m=(l+r)>>1;
		if(speed_t(bitRate)>=bitRates[m][0])
			l=m;
		else
			r=m;
		}
	cfsetspeed(&term,bitRates[l][1]);
	#endif
	
	/* Set character size: */
	term.c_cflag&=~CSIZE;
	switch(charLength)
		{
		case 5:
			term.c_cflag|=CS5;
			break;
		
		case 6:
			term.c_cflag|=CS6;
			break;
		
		case 7:
			term.c_cflag|=CS7;
			break;
		
		case 8:
			term.c_cflag|=CS8;
			break;
		}
	
	/* Set parity settings: */
	term.c_cflag&=~(PARENB|PARODD);
	switch(parity)
		{
		case OddParity:
			term.c_cflag|=PARENB|PARODD;
			break;
		
		case EvenParity:
			term.c_cflag|=PARENB;
			break;
		
		default:
			;
		}
	
	/* Set stop bit settings: */
	term.c_cflag&=~CSTOPB;
	if(numStopbits==2)
		term.c_cflag|=CSTOPB;
	
	/* Set handshake settings: */
	#ifdef __SGI_IRIX__
	term.c_cflag&=~CNEW_RTSCTS;
	#else
	term.c_cflag&=~CRTSCTS;
	#endif
	if(enableHandshake)
		{
		#ifdef __SGI_IRIX__
		term.c_cflag|=CNEW_RTSCTS;
		#else
		term.c_cflag|=CRTSCTS;
		#endif
		}
		
	/* Configure the port: */
	if(tcsetattr(fd,TCSADRAIN,&term)!=0)
		Misc::throwStdErr("Comm::BufferedSerialPort::setSerialSettings: Unable to configure device");
	}

void BufferedSerialPort::setRawMode(int minNumBytes,int timeOut)
	{
	/* Read the current port settings: */
	struct termios term;
	if(tcgetattr(fd,&term)!=0)
		Misc::throwStdErr("Comm::BufferedSerialPort::setRawMode: Unable to read device configuration");
	
	/* Disable canonical mode: */
	term.c_lflag&=~ICANON;
	
	/* Set the min/time parameters: */
	term.c_cc[VMIN]=cc_t(minNumBytes);
	term.c_cc[VTIME]=cc_t(timeOut);
	
	/* Set the port: */
	if(tcsetattr(fd,TCSANOW,&term)!=0)
		Misc::throwStdErr("Comm::BufferedSerialPort::setRawMode: Unable to configure device");
	}

void BufferedSerialPort::setCanonicalMode(void)
	{
	/* Read the current port settings: */
	struct termios term;
	if(tcgetattr(fd,&term)!=0)
		Misc::throwStdErr("Comm::BufferedSerialPort::setCanonicalMode: Unable to read device configuration");
	
	/* Enable canonical mode: */
	term.c_lflag|=ICANON;
	
	/* Set the port: */
	if(tcsetattr(fd,TCSANOW,&term)!=0)
		Misc::throwStdErr("Comm::BufferedSerialPort::setCanonicalMode: Unable to configure device");
	}

void BufferedSerialPort::setLineControl(bool respectModemLines,bool hangupOnClose)
	{
	/* Read the current port settings: */
	struct termios term;
	if(tcgetattr(fd,&term)!=0)
		Misc::throwStdErr("Comm::BufferedSerialPort::setLineControl: Unable to read device configuration");
	
	if(respectModemLines)
		term.c_cflag&=~CLOCAL;
	else
		term.c_cflag|=CLOCAL;
	if(hangupOnClose)
		term.c_cflag|=HUPCL;
	else
		term.c_cflag&=~HUPCL;
	
	/* Set the port: */
	if(tcsetattr(fd,TCSANOW,&term)!=0)
		Misc::throwStdErr("Comm::BufferedSerialPort::setLineControl: Unable to configure device");
	}

}
