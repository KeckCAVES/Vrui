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

#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <Misc/Time.h>

#include <Comm/SerialPort.h>

namespace Comm {

/***************************
Methods of class SerialPort:
***************************/

void SerialPort::readBlocking(size_t numBytes,char* bytes)
	{
	while(numBytes>0)
		{
		ssize_t bytesReceived=read(port,bytes,numBytes);
		if(bytesReceived>=0)
			{
			numBytes-=bytesReceived;
			bytes+=bytesReceived;
			totalBytesReceived+=bytesReceived;
			if(numBytes>0)
				++numReadSpins;
			}
		else if(errno!=EAGAIN)
			throw ReadError();
		}
	}

void SerialPort::writeBlocking(size_t numBytes,const char* bytes)
	{
	while(numBytes>0)
		{
		ssize_t bytesSent=write(port,bytes,numBytes);
		if(bytesSent>=0)
			{
			numBytes-=bytesSent;
			bytes+=bytesSent;
			totalBytesSent+=bytesSent;
			if(numBytes>0)
				++numWriteSpins;
			}
		else if(errno!=EAGAIN)
			throw WriteError();
		}
	}

SerialPort::SerialPort(const char* deviceName)
	:port(open(deviceName,O_RDWR|O_NOCTTY)),
	 totalBytesReceived(0),totalBytesSent(0),
	 numReadSpins(0),numWriteSpins(0)
	{
	if(port<0)
		throw OpenError(deviceName);
	
	/* Configure as "raw" port: */
	struct termios term;
	tcgetattr(port,&term);
	term.c_iflag=IGNBRK|IGNPAR; // Don't generate signals or parity errors
	term.c_oflag=0; // No output processing
	term.c_cflag|=CREAD|CLOCAL; // Enable receiver; no modem line control
	term.c_lflag=0; // Don't generate signals or echos
	term.c_cc[VMIN]=1; // Block read() until at least a single byte is read
	term.c_cc[VTIME]=0; // No timeout on read()
	tcsetattr(port,TCSANOW,&term);
	
	/* Flush both queues: */
	tcflush(port,TCIFLUSH);
	tcflush(port,TCOFLUSH);
	}

SerialPort::~SerialPort(void)
	{
	close(port);
	}

void SerialPort::setPortSettings(int portSettingsMask)
	{
	/* Retrieve current flags: */
	int fileFlags=fcntl(port,F_GETFL);
	
	/* Change flags according to given parameter: */
	if(portSettingsMask&NONBLOCKING)
		fileFlags|=FNDELAY|FNONBLOCK;
	else
		fileFlags&=~(FNDELAY|FNONBLOCK);
	
	/* Set new flags: */
	fcntl(port,F_SETFL,fileFlags);
	}

void SerialPort::setSerialSettings(int bitRate,int charLength,SerialPort::ParitySettings parity,int numStopbits,bool enableHandshake)
	{
	/* Initialize a termios structure: */
	struct termios term;
	tcgetattr(port,&term);
	
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
	cfsetospeed(&term,bitRates[l][1]);
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
	switch(parity)
		{
		case PARITY_ODD:
			term.c_cflag|=PARENB|PARODD;
			break;
		
		case PARITY_EVEN:
			term.c_cflag|=PARENB;
			break;
		
		default:
			;
		}
	
	/* Set stop bit settings: */
	if(numStopbits==2)
		term.c_cflag|=CSTOPB;
	
	/* Set handshake settings: */
	if(enableHandshake)
		{
		#ifdef __SGI_IRIX__
		term.c_cflag|=CNEW_RTSCTS;
		#else
		term.c_cflag|=CRTSCTS;
		#endif
		}
		
	/* Set the port: */
	tcsetattr(port,TCSADRAIN,&term);
	}

void SerialPort::setRawMode(int minNumBytes,int timeOut)
	{
	/* Read the current port settings: */
	struct termios term;
	tcgetattr(port,&term);
	
	/* Disable canonical mode: */
	term.c_lflag&=~ICANON;
	
	/* Set the min/time parameters: */
	term.c_cc[VMIN]=cc_t(minNumBytes);
	term.c_cc[VTIME]=cc_t(timeOut);
	
	/* Set the port: */
	tcsetattr(port,TCSANOW,&term);
	}

void SerialPort::setCanonicalMode(void)
	{
	/* Read the current port settings: */
	struct termios term;
	tcgetattr(port,&term);
	
	/* Enable canonical mode: */
	term.c_lflag|=ICANON;
	
	/* Set the port: */
	tcsetattr(port,TCSANOW,&term);
	}

void SerialPort::setLineControl(bool respectModemLines,bool hangupOnClose)
	{
	/* Read the current port settings: */
	struct termios term;
	tcgetattr(port,&term);
	
	if(respectModemLines)
		term.c_cflag&=~CLOCAL;
	else
		term.c_cflag|=CLOCAL;
	if(hangupOnClose)
		term.c_cflag|=HUPCL;
	else
		term.c_cflag&=~HUPCL;
	
	/* Set the port: */
	tcsetattr(port,TCSANOW,&term);
	}

void SerialPort::resetStatistics(void)
	{
	totalBytesReceived=0;
	totalBytesSent=0;
	numReadSpins=0;
	numWriteSpins=0;
	}

bool SerialPort::waitForByte(const Misc::Time& timeout)
	{
	/* Prepare parameters for select: */
	fd_set readFdSet;
	FD_ZERO(&readFdSet);
	FD_SET(port,&readFdSet);
	struct timeval tv=timeout;
	
	/* Wait for an event on the port and return: */
	return select(port+1,&readFdSet,0,0,&tv)>0&&FD_ISSET(port,&readFdSet);
	}

std::pair<char,bool> SerialPort::readByteNonBlocking(void)
	{
	char readByte;
	ssize_t bytesReceived=read(port,&readByte,1);
	if(bytesReceived<0&&errno!=EAGAIN)
		throw ReadError();
	return std::pair<char,bool>(readByte,bytesReceived==1);
	}

size_t SerialPort::readBytesRaw(size_t maxNumBytes,char* bytes)
	{
	ssize_t bytesRead=read(port,bytes,maxNumBytes);
	if(bytesRead>=0)
		return bytesRead;
	else if(errno==EAGAIN)
		return 0;
	else
		throw ReadError();
	}

void SerialPort::writeString(const char* string)
	{
	writeBlocking(strlen(string),string);
	}

void SerialPort::flush(void)
	{
	tcflush(port,TCOFLUSH);
	}

void SerialPort::drain(void)
	{
	tcdrain(port);
	}

}
