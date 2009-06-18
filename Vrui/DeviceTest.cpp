/***********************************************************************
DeviceTest - Program to test the connection to a Vrui VR Device Daemon
and to dump device positions/orientations and button states.
Copyright (c) 2002-2006 Oliver Kreylos

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

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <Misc/Timer.h>
#include <Misc/ConfigurationFile.h>
#include <Vrui/VRDeviceClient.h>

typedef Vrui::VRDeviceState::TrackerState TrackerState;
typedef TrackerState::PositionOrientation PositionOrientation;
typedef PositionOrientation::Scalar Scalar;
typedef PositionOrientation::Point Point;
typedef PositionOrientation::Vector Vector;
typedef PositionOrientation::Rotation Rotation;

void printTrackerPos(const Vrui::VRDeviceState& state,int trackerIndex)
	{
	const TrackerState& ts=state.getTrackerState(trackerIndex);
	Point pos=ts.positionOrientation.getOrigin();
	std::cout.setf(std::ios::fixed);
	std::cout.precision(3);
	std::cout<<"("<<std::setw(8)<<pos[0]<<" "<<std::setw(8)<<pos[1]<<" "<<std::setw(8)<<pos[2]<<")";
	}

void printTrackerPosOrient(const Vrui::VRDeviceState& state,int trackerIndex)
	{
	const TrackerState& ts=state.getTrackerState(trackerIndex);
	Point pos=ts.positionOrientation.getOrigin();
	Rotation rot=ts.positionOrientation.getRotation();
	Vector axis=rot.getScaledAxis();
	Scalar angle=Math::deg(rot.getAngle());
	std::cout.setf(std::ios::fixed);
	std::cout.precision(3);
	std::cout<<"("<<std::setw(8)<<pos[0]<<", "<<std::setw(8)<<pos[1]<<", "<<std::setw(8)<<pos[2]<<") ";
	std::cout<<"("<<std::setw(8)<<axis[0]<<", "<<std::setw(8)<<axis[1]<<", "<<std::setw(8)<<axis[2]<<") ";
	std::cout<<std::setw(8)<<angle;
	}

void printTrackerFrame(const Vrui::VRDeviceState& state,int trackerIndex)
	{
	const TrackerState& ts=state.getTrackerState(trackerIndex);
	Point pos=ts.positionOrientation.getOrigin();
	Rotation rot=ts.positionOrientation.getRotation();
	Vector x=rot.getDirection(0);
	Vector y=rot.getDirection(1);
	Vector z=rot.getDirection(2);
	std::cout.setf(std::ios::fixed);
	std::cout.precision(4);
	std::cout<<"("<<std::setw(8)<<pos[0]<<" "<<std::setw(8)<<pos[1]<<" "<<std::setw(8)<<pos[2]<<") ";
	std::cout<<"("<<std::setw(8)<<x[0]<<" "<<std::setw(8)<<x[1]<<" "<<std::setw(8)<<x[2]<<") ";
	std::cout<<"("<<std::setw(8)<<y[0]<<" "<<std::setw(8)<<y[1]<<" "<<std::setw(8)<<y[2]<<") ";
	std::cout<<"("<<std::setw(8)<<z[0]<<" "<<std::setw(8)<<z[1]<<" "<<std::setw(8)<<z[2]<<")";
	}

void printButtons(const Vrui::VRDeviceState& state)
	{
	for(int i=0;i<state.getNumButtons();++i)
		{
		if(i>0)
			std::cout<<" ";
		if(state.getButtonState(i))
			std::cout<<"X";
		else
			std::cout<<'.';
		}
	}

void printValuators(const Vrui::VRDeviceState& state)
	{
	std::cout.setf(std::ios::fixed);
	std::cout.precision(3);
	for(int i=0;i<state.getNumValuators();++i)
		{
		if(i>0)
			std::cout<<" ";
		std::cout<<std::setw(6)<<state.getValuatorState(i);
		}
	}

int main(int argc,char* argv[])
	{
	/* Parse command line: */
	char* serverName=0;
	int trackerIndex=0;
	int printMode=0;
	bool printButtonStates=false;
	bool printNewlines=false;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i],"-t")==0||strcasecmp(argv[i],"--trackerIndex")==0)
				{
				++i;
				trackerIndex=atoi(argv[i]);
				}
			else if(strcasecmp(argv[i],"-alltrackers")==0)
				trackerIndex=-1;
			else if(strcasecmp(argv[i],"-p")==0)
				printMode=0;
			else if(strcasecmp(argv[i],"-o")==0)
				printMode=1;
			else if(strcasecmp(argv[i],"-f")==0)
				printMode=2;
			else if(strcasecmp(argv[i],"-v")==0)
				printMode=3;
			else if(strcasecmp(argv[i],"-b")==0)
				printButtonStates=true;
			else if(strcasecmp(argv[i],"-n")==0)
				printNewlines=true;
			}
		else
			serverName=argv[i];
		}
	
	if(serverName==0)
		{
		std::cerr<<"Usage: "<<argv[0]<<" [(-t | --trackerIndex) <trackerIndex>] [-p | -o | -f | -v] [-b] <serverName:serverPort>"<<std::endl;
		return 1;
		}
	
	/* Initialize device client: */
	Vrui::VRDeviceClient* deviceClient=0;
	try
		{
		/* Split the server name into hostname:port: */
		char* colonPtr=0;
		for(char* cPtr=serverName;*cPtr!='\0';++cPtr)
			if(*cPtr==':')
				colonPtr=cPtr;
		int portNumber=0;
		if(colonPtr!=0)
			{
			portNumber=atoi(colonPtr+1);
			*colonPtr='\0';
			}
		deviceClient=new Vrui::VRDeviceClient(serverName,portNumber);
		}
	catch(std::runtime_error error)
		{
		std::cerr<<"Caught exception "<<error.what()<<" while initializing VR device client"<<std::endl;
		return 1;
		}
	
	/* Disable printing of tracking information if there are no trackers: */
	deviceClient->lockState();
	if(printMode==0&&deviceClient->getState().getNumTrackers()==0)
		printMode=-1;
	deviceClient->unlockState();
	
	/* Print output header line: */
	switch(printMode)
		{
		case 0:
			std::cout<<"  Pos X    Pos Y    Pos Z   "<<std::endl;
			break;
		
		case 1:
			std::cout<<"  Pos X    Pos Y    Pos Z      Axis X   Axis Y   Axis Z    Angle  "<<std::endl;
			break;
		
		case 2:
			std::cout<<" Pos X  Pos Y  Pos Z     XA X   XA Y   XA Z     YA X   YA Y   YA Z     ZA X   ZA Y   ZA Z  "<<std::endl;
			break;
		}
	
	/* Run main loop: */
	deviceClient->activate();
	deviceClient->startStream();
	bool loop=true;
	Misc::Timer t;
	int numPackets=0;
	while(loop)
		{
		/* Print new device state: */
		if(!printNewlines)
			std::cout<<"\r";
		deviceClient->lockState();
		const Vrui::VRDeviceState& state=deviceClient->getState();
		switch(printMode)
			{
			case 0:
				if(trackerIndex<0)
					{
					printTrackerPos(state,0);
					for(int i=1;i<state.getNumTrackers();++i)
						{
						std::cout<<" ";
						printTrackerPos(state,i);
						}
					}
				else
					printTrackerPos(state,trackerIndex);
				break;
			
			case 1:
				printTrackerPosOrient(state,trackerIndex);
				break;
			
			case 2:
				printTrackerFrame(state,trackerIndex);
				break;
			
			case 3:
				printValuators(state);
				break;
			
			default:
				; // Print nothing; nothing, I say!
			}
		if(printButtonStates)
			{
			std::cout<<" ";
			printButtons(state);
			}
		deviceClient->unlockState();
		if(printNewlines)
			std::cout<<std::endl;
		else
			std::cout<<std::flush;
		
		/* Check for a key press event: */
		fd_set readFdSet;
		FD_ZERO(&readFdSet);
		FD_SET(fileno(stdin),&readFdSet);
		struct timeval timeout;
		timeout.tv_sec=0;
		timeout.tv_usec=0;
		bool dataWaiting=select(fileno(stdin)+1,&readFdSet,0,0,&timeout)>=0&&FD_ISSET(fileno(stdin),&readFdSet);
		if(dataWaiting)
			loop=false;
		
		if(loop)
			{
			/* Wait for next packet: */
			deviceClient->getPacket();
			++numPackets;
			}
		}
	std::cout<<std::endl;
	t.elapse();
	std::cout<<"Received "<<numPackets<<" device data packets in "<<t.getTime()*1000.0<<" ms ("<<double(numPackets)/t.getTime()<<" packets/s)"<<std::endl;
	deviceClient->stopStream();
	deviceClient->deactivate();
	
	/* Clean up and terminate: */
	delete deviceClient;
	return 0;
	}
