/***********************************************************************
InterSense - Class for InterSense IS-900 hybrid inertial/sonic 6-DOF
tracking devices.
Copyright (c) 2004-2005 Oliver Kreylos

This file is part of the Vrui VR Device Driver Daemon (VRDeviceDaemon).

The Vrui VR Device Driver Daemon is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vrui VR Device Driver Daemon is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui VR Device Driver Daemon; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <Misc/Endianness.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/Time.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/GeometryValueCoders.h>

#include "VRDeviceManager.h"

#include "InterSense.h"

namespace {

/************************************************
Helper class to store transmitter configurations:
************************************************/

struct Transmitter
	{
	/* Embedded classes: */
	public:
	typedef Geometry::Point<float,3> Point; // Type for transmitter positions
	typedef Geometry::Vector<float,3> Vector; // Type for transmitter directions

	/* Elements: */
	public:
	Point pos; // Transmitter position
	Vector dir; // Normalized transmitter direction
	};

}	

namespace Misc {

/************************************************
Value coder class for transmitter configurations:
************************************************/

template <>
class ValueCoder<Transmitter>
	{
	/* Methods: */
	public:
	static std::string encode(const Transmitter& t)
		{
		std::string result=ValueCoder<Transmitter::Point>::encode(t.pos);
		result+=", ";
		result+=ValueCoder<Transmitter::Vector>::encode(t.dir);
		
		return result;
		};
	static Transmitter decode(const char* start,const char* end,const char** decodeEnd =0)
		{
		try
			{
			Transmitter result;

			/* Decode transmitter position: */
			result.pos=ValueCoder<Transmitter::Point>::decode(start,end,&start);
			
			/* Skip whitespace and check for separating comma: */
			while(start!=end&&isspace(*start))
				++start;
			if(start==end||*start!=',')
				throw 42;
			++start;
			while(start!=end&&isspace(*start))
				++start;
			
			/* Decode transmitter direction: */
			result.dir=ValueCoder<Transmitter::Vector>::decode(start,end,&start);
			
			if(decodeEnd!=0)
				*decodeEnd=start;
			
			return result;
			}
		catch(...)
			{
			throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to Transmitter"));
			}
		};
	};

}

/***************************
Methods of class InterSense:
***************************/

float InterSense::readFloat(const char* lsb) const
	{
	float result=*reinterpret_cast<const float*>(lsb);
	#if __BYTE_ORDER==__BIG_ENDIAN
	Misc::swapEndianness(result);
	#endif
	return result;
	}

char* InterSense::readLine(int lineBufferSize,char* lineBuffer,const Misc::Time& timeout)
	{
	/* Calculate a deadline by which the complete line has to have arrived: */
	Misc::Time deadline=Misc::Time::now()+timeout;
	
	/* Read bytes from the serial port until CR/LF has been read or deadline is reached: */
	int state=0;
	char* bufPtr=lineBuffer;
	int bytesLeft=lineBufferSize-1;
	while(state<2)
		{
		/* Wait for next byte, but not too long: */
		if(!serialPort.waitForByte(deadline-Misc::Time::now()))
			break;
		
		/* Read next byte: */
		int input=serialPort.readByte();
		
		/* Process byte: */
		switch(state)
			{
			case 0: // Still waiting for CR
				if(input==13) // CR read?
					{
					/* Go to next state: */
					state=1;
					}
				else if(bytesLeft>0) // Still room in buffer?
					{
					/* Store byte in buffer: */
					*bufPtr=char(input);
					++bufPtr;
					--bytesLeft;
					}
				break;
			
			case 1: // Waiting for LF
				if(input==10) // LF read?
					{
					/* Go to final state: */
					state=2;
					}
				else
					{
					/* Continue reading characters: */
					state=0;
					}
				break;
			}
		}
	
	/* Terminate and return read string: */
	*bufPtr='\0';
	return lineBuffer;
	}

bool InterSense::readStatusReply(void)
	{
	/* Create a deadline by which the complete status reply has to have arrived: */
	Misc::Time deadline=Misc::Time::now()+Misc::Time(10.0);
	
	/* Read bytes from the serial port until the status report's prefix has been matched or the deadline is reached: */
	int state=0;
	while(state<4)
		{
		/* Wait for next byte, but not too long: */
		if(!serialPort.waitForByte(deadline-Misc::Time::now()))
			break;
		
		/* Read next byte and try matching status reply's prefix: */
		int input=serialPort.readByte();
		switch(state)
			{
			case 0: // Haven't matched anything
				if(input=='2')
					state=1;
				break;
			
			case 1: // Have matched "2"
				if(input=='2')
					state=2;
				else if(input>='1'&&input<='4')
					state=3;
				else
					state=0;
				break;
			
			case 2: // Have matched "22"
				if(input=='S')
					state=4;
				else if(input=='2')
					state=2;
				else if(input>='1'&&input<='4')
					state=3;
				else
					state=0;
				break;
			
			case 3: // Have matched "2[1,3,4]"
				if(input=='S')
					state=4;
				else if(input=='2')
					state=1;
				else
					state=0;
				break;
			}
		}
	
	/* Fail if we timed out while trying to match the prefix: */
	if(state!=4)
		return false;
	
	/* Read rest of status reply until final CR/LF pair: */
	char buffer[256];
	readLine(sizeof(buffer),buffer,deadline-Misc::Time::now());
	#ifdef VERBOSE
	printf("InterSense: Received status reply\n  %s",buffer);
	fflush(stdout);
	#endif
	
	#if 0
	/* Check if it's really an InterSense tracker: */
	if(strncmp(buffer+21,"IS900VWT",8)!=0)
		return false;
	#endif
	
	return true;
	}

void InterSense::processBuffer(int station,const char* recordBuffer)
	{
	Vrui::VRDeviceState::TrackerState ts;
		
	/* Calculate raw position and orientation: */
	typedef PositionOrientation::Vector Vector;
	typedef Vector::Scalar VScalar;
	Vector v;
	v[0]=VScalar(readFloat(recordBuffer+8));
	v[1]=VScalar(readFloat(recordBuffer+12));
	v[2]=VScalar(readFloat(recordBuffer+16));

	typedef PositionOrientation::Rotation Rotation;
	typedef Rotation::Scalar RScalar;
	RScalar angles[3];
	angles[0]=Math::rad(RScalar(readFloat(recordBuffer+20)));
	angles[1]=Math::rad(RScalar(readFloat(recordBuffer+24)));
	angles[2]=Math::rad(RScalar(readFloat(recordBuffer+28)));
	Rotation o=Rotation::identity;
	o*=Rotation::rotateZ(angles[0]);
	o*=Rotation::rotateY(angles[1]);
	o*=Rotation::rotateX(angles[2]);
	
	/* Set new position and orientation: */
	ts.positionOrientation=Vrui::VRDeviceState::TrackerState::PositionOrientation(v,o);
	
	/* Calculate linear and angular velocities: */
	timers[station].elapse();
	if(notFirstMeasurements[station])
		{
		/* Estimate velocities by dividing position/orientation differences by elapsed time since last measurement: */
		double time=timers[station].getTime();
		ts.linearVelocity=(v-oldPositionOrientations[station].getTranslation())/Vrui::VRDeviceState::TrackerState::LinearVelocity::Scalar(time);
		Rotation dO=o*Geometry::invert(oldPositionOrientations[station].getRotation());
		ts.angularVelocity=dO.getScaledAxis()/Vrui::VRDeviceState::TrackerState::AngularVelocity::Scalar(time);
		}
	else
		{
		/* Force initial velocities to zero: */
		ts.linearVelocity=Vrui::VRDeviceState::TrackerState::LinearVelocity::zero;
		ts.angularVelocity=Vrui::VRDeviceState::TrackerState::AngularVelocity::zero;
		notFirstMeasurements[station]=true;
		}
	oldPositionOrientations[station]=ts.positionOrientation;
	
	/* Update button states: */
	for(int i=0;i<stations[station].numButtons;++i)
		setButtonState(stations[station].firstButtonIndex+i,recordBuffer[32]&(1<<i));
	
	/* Update valuator states: */
	if(stations[station].joystick)
		{
		float x=(float(*(unsigned char*)(recordBuffer+33))-127.5f)/127.5f;
		float y=(float(*(unsigned char*)(recordBuffer+34))-127.5f)/127.5f;
		setValuatorState(stations[station].firstValuatorIndex+0,x);
		setValuatorState(stations[station].firstValuatorIndex+1,y);
		}
	
	/* Update tracker state: */
	setTrackerState(station,ts);
	}

int InterSense::readRecordSync(char* recordBuffer)
	{
	int station=-1;
	int state=0;
	while(state<5)
		{
		int input=serialPort.readByte();
		switch(state)
			{
			case 0: // Haven't matched anything
				if(input==13)
					state=1;
				else
					state=0;
				break;
			
			case 1: // Have matched CR
				if(input==10)
					state=2;
				else if(input==13)
					state=1;
				else
					state=0;
				break;
			
			case 2: // Have matched CR/LF
				if(input=='0')
					state=3;
				else if(input==13)
					state=1;
				else
					state=0;
				break;
			
			case 3: // Have matched CR/LF + 0
				if(input==13)
					state=1;
				else if(input>='1'&&input<='9')
					{
					station=input-'0';
					state=4;
					}
				else if(input>='A'&&input<='Z')
					{
					station=input-'A'+10;
					state=4;
					}
				else if(input>='a'&&input<='z')
					{
					station=input-'a'+10;
					state=4;
					}
				else
					{
					station=0;
					state=0;
					}
				break;
			
			case 4: // Have matched CR/LF + 0 + <receiver number>
				if(input==' '||(input>='A'&&input<='Z')||(input>='a'&&input<='z'))
					state=5;
				else if(input==13)
					state=1;
				else
					state=0;
				break;
			}
		}
	
	/* Read rest of record: */
	serialPort.readBytes(27,recordBuffer+8);
	
	/* Return station ID: */
	return station;
	}

int InterSense::readRecordNoSync(char* recordBuffer)
	{
	/* Read complete record's worth of data: */
	serialPort.readBytes(32,recordBuffer+3);
	
	/* Check if we're still synchronized: */
	bool nSync=true;
	nSync=nSync&&recordBuffer[3]==char(13);
	nSync=nSync&&recordBuffer[4]==char(10);
	nSync=nSync&&recordBuffer[5]=='0';
	
	/* Decode station ID: */
	int stationId=0;
	if(recordBuffer[6]>='1'&&recordBuffer[6]<='9')
		stationId=recordBuffer[6]-'0';
	else if(recordBuffer[6]>='A'&&recordBuffer[6]<='Z')
		stationId=recordBuffer[6]-'A'+10;
	else if(recordBuffer[6]>='a'&&recordBuffer[6]<='z')
		stationId=recordBuffer[6]-'a'+10;
	
	nSync=nSync&&stationId>0;
	nSync=nSync&&(recordBuffer[7]==' '||(recordBuffer[7]>='A'&&recordBuffer[7]<='Z')||(recordBuffer[7]>='a'&&recordBuffer[7]<='z'));
	if(!nSync)
		return 0;
	
	/* Return station ID: */
	return stationId;
	}

void InterSense::deviceThreadMethod(void)
	{
	/* Reset first measurement flags: */
	for(int i=0;i<numTrackers;++i)
		notFirstMeasurements[i]=false;
	
	/* Read first record in synchronizing mode: */
	int stationId;
	char recordBuffer[256];
	stationId=readRecordSync(recordBuffer);
	
	/* Parse read buffer: */
	processBuffer(stationIdToIndex[stationId],recordBuffer);
	
	while(true)
		{
		/* Try reading record in synchronized mode: */
		if((stationId=readRecordNoSync(recordBuffer))==0)
			{
			/* Fall back to synchronizing mode: */
			#ifdef VERBOSE
			printf("InterSense: Resynchronizing with tracker stream\n");
			fflush(stdout);
			#endif
			stationId=readRecordSync(recordBuffer);
			}
		
		/* Parse read buffer: */
		processBuffer(stationIdToIndex[stationId],recordBuffer);
		}
	}

InterSense::InterSense(VRDevice::Factory* sFactory,VRDeviceManager* sDeviceManager,Misc::ConfigurationFile& configFile)
	:VRDevice(sFactory,sDeviceManager,configFile),
	 serialPort(configFile.retrieveString("./serialPort").c_str()),
	 stations(0),
	 timers(0),notFirstMeasurements(0),oldPositionOrientations(0)
	{
	/* Set device port parameters: */
	int deviceBaudRate=configFile.retrieveValue<int>("./deviceBaudRate");
	serialPort.setSerialSettings(deviceBaudRate,8,Comm::SerialPort::PARITY_NONE,1,false);
	serialPort.setRawMode(1,0);
	
	if(configFile.retrieveValue<bool>("./resetDevice",false))
		{
		/* Reset device: */
		#ifdef VERBOSE
		printf("InterSense: Resetting device\n");
		fflush(stdout);
		#endif
		serialPort.writeByte('\31');
		delay(15.0);
		}
	else
		{
		/* Stop continuous mode (in case it's still active): */
		#ifdef VERBOSE
		printf("InterSense: Disabling continuous mode\n");
		fflush(stdout);
		#endif
		serialPort.writeByte('c');
		}
	
	/* Request status record to check if device is okey-dokey: */
	#ifdef VERBOSE
	printf("InterSense: Requesting status record\n");
	fflush(stdout);
	#endif
	serialPort.writeByte('S');
	if(!readStatusReply())
		{
		/* Try resetting the device, seeing if that helps: */
		#ifdef VERBOSE
		printf("InterSense: Resetting device\n");
		fflush(stdout);
		#endif
		serialPort.writeByte('\31');
		delay(15.0);
		
		/* Request another status record: */
		#ifdef VERBOSE
		printf("InterSense: Re-requesting status record\n");
		fflush(stdout);
		#endif
		serialPort.writeByte('S');
		if(!readStatusReply())
			Misc::throwStdErr("InterSense: Device not responding");
		}
	
	/* Get the array of station enable flags: */
	#ifdef VERBOSE
	printf("InterSense: Detecting enabled stations\n");
	fflush(stdout);
	#endif
	serialPort.writeString("l*\r\n");
	delay(0.1);
	char buffer[256];
	readLine(sizeof(buffer),buffer,Misc::Time(1.0));
	if(strncmp(buffer,"21l",3)!=0)
		Misc::throwStdErr("InterSense: Unable to detect enabled stations");
	
	/* Disable all stations: */
	for(int i=0;i<32;++i)
		{
		stationIdToIndex[i]=-1;
		if(buffer[i+3]=='1')
			{
			char command[20];
			snprintf(command,sizeof(command),"l%d,0\r\n",i+1);
			serialPort.writeString(command);
			delay(0.1);
			}
		}
	
	/* Probe the constellation configuration if asked: */
	if(configFile.retrieveValue<bool>("./probeConstellation",false))
		{
		/* Check if the device has a valid constellation configuration: */
		#ifdef VERBOSE
		printf("InterSense: Probing constellation configuration\n");
		fflush(stdout);
		#endif
		serialPort.writeString("MCF\r\n");
		delay(0.1);
		int numTransmitters=0;
		while(true)
			{
			/* Read the next transmitter line: */
			readLine(sizeof(buffer),buffer,Misc::Time(1.0));
			
			/* Check if it's a valid transmitter line: */
			int transmitterNum,transmitterId;
			double posX,posY,posZ,dirX,dirY,dirZ;
			if(sscanf(buffer,"31F %d %lf %lf %lf %lf %lf %lf %d",&transmitterNum,&posX,&posY,&posZ,&dirX,&dirY,&dirZ,&transmitterId)!=8)
				break;
			
			/* Increase the number of transmitters: */
			++numTransmitters;
			}
		#ifdef VERBOSE
		printf("InterSense: Detected %d configured transmitters\n",numTransmitters);
		fflush(stdout);
		#endif
		}
	
	/* Upload a constellation configuration if asked: */
	std::string constellationName=configFile.retrieveString("./uploadConstellationConfiguration","");
	if(constellationName!="")
		{
		#ifdef VERBOSE
		printf("InterSense: Uploading constellation configuration %s\n",constellationName.c_str());
		fflush(stdout);
		#endif
		
		/* Go to constellation configuration's section: */
		configFile.setCurrentSection(constellationName.c_str());
		
		Transmitter* transmitters=0;
		
		try
			{
			/* Get the number of configured transmitters and the base number for internal IDs: */
			int numTransmitters=configFile.retrieveValue<int>("./numTransmitters");
			int transmitterIdBase=configFile.retrieveValue<int>("./transmitterIdBase",5001);
			
			/* Get the conversion factor from configuration units to meters: */
			float unitSize=configFile.retrieveValue<float>("./unitSize",1.0f);
			
			/* Read all transmitter configurations to check for validity first: */
			transmitters=new Transmitter[numTransmitters];
			for(int i=0;i<numTransmitters;++i)
				{
				/* Read the transmitter's configuration: */
				char transmitterName[20];
				snprintf(transmitterName,sizeof(transmitterName),"./MCF%d",i+1);
				transmitters[i]=configFile.retrieveValue<Transmitter>(transmitterName);
				
				/* Convert the transmitter's position to meters: */
				for(int j=0;j<3;++j)
					transmitters[i].pos[j]*=unitSize;
				
				/* Normalize the transmitter's direction: */
				transmitters[i].dir.normalize();
				}
			
			/* Upload constellation configuration to device: */
			serialPort.writeString("MCC\r\n");
			delay(0.1);
			for(int i=0;i<numTransmitters;++i)
				{
				const Transmitter& t=transmitters[i];
				char transmitterLine[256];
				snprintf(transmitterLine,sizeof(transmitterLine),
				         "MCF%d, %8.4f, %8.4f, %8.4f, %6.3f, %6.3f, %6.3f, %d\r\n",
				         i+1,t.pos[0],t.pos[1],t.pos[2],t.dir[0],t.dir[1],t.dir[2],i+transmitterIdBase);
				serialPort.writeString(transmitterLine);
				delay(0.1);
				}
			serialPort.writeString("MCe\r\n");
			delay(0.1);
			}
		catch(std::runtime_error err)
			{
			printf("InterSense: Ignoring constellation configuration %s due to exception %s\n",constellationName.c_str(),err.what());
			}
		
		/* Clean up: */
		delete[] transmitters;
		
		/* Go back to device's section: */
		configFile.setCurrentSection("..");
		}
	
	/* Retrieve list of station names: */
	typedef std::vector<std::string> StringList;
	StringList stationNames=configFile.retrieveValue<StringList>("./stationNames");
	setNumTrackers(stationNames.size(),configFile);
	stations=new Station[numTrackers];
	int totalNumButtons=0;
	int totalNumValuators=0;
	
	/* Initialize all tracked stations: */
	#ifdef VERBOSE
	printf("InterSense: Initializing tracked stations\n");
	fflush(stdout);
	#endif
	for(int i=0;i<numTrackers;++i)
		{
		char command[80];
		
		/* Go to station's section: */
		configFile.setCurrentSection(stationNames[i].c_str());
		
		/* Read station's configuration: */
		stations[i].id=configFile.retrieveValue<int>("./id",i+1);
		stationIdToIndex[stations[i].id]=i;
		stations[i].numButtons=configFile.retrieveValue<int>("./numButtons",0);
		stations[i].firstButtonIndex=totalNumButtons;
		totalNumButtons+=stations[i].numButtons;
		stations[i].joystick=configFile.retrieveValue<bool>("./joystick",false);
		stations[i].firstValuatorIndex=totalNumValuators;
		if(stations[i].joystick)
			totalNumValuators+=2;
		
		/* Enable station: */
		snprintf(command,sizeof(command),"l%d,1\r\n",stations[i].id);
		serialPort.writeString(command);
		delay(0.1);
		
		/* Reset station's alignment frame: */
		snprintf(command,sizeof(command),"R%d\r\n",stations[i].id);
		serialPort.writeString(command);
		delay(0.1);
		
		/* Disable boresight mode: */
		snprintf(command,sizeof(command),"b%d\r\n",stations[i].id);
		serialPort.writeString(command);
		delay(0.1);
		
		/* Reset station's tip offset: */
		snprintf(command,sizeof(command),"N%d,%8.4f,%8.4f,%8.4f\r\n",stations[i].id,0.0f,0.0f,0.0f);
		serialPort.writeString(command);
		delay(0.1);
		
		/* Set station's output format: */
		snprintf(command,sizeof(command),"O%d,2,4,22,23,1\r\n",stations[i].id);
		serialPort.writeString(command);
		delay(0.1);
		
		/* Set stations' motion prediction: */
		int predictionTime=configFile.retrieveValue<int>("./predictionTime",0);
		snprintf(command,sizeof(command),"Mp%d,%d\r\n",stations[i].id,predictionTime);
		serialPort.writeString(command);
		delay(0.1);
		
		/* Set stations' perceptual enhancement level: */
		int perceptualEnhancement=configFile.retrieveValue<int>("./perceptualEnhancement",2);
		snprintf(command,sizeof(command),"MF%d,%d\r\n",stations[i].id,perceptualEnhancement);
		serialPort.writeString(command);
		delay(0.1);
		
		/* Set station's rotational sensitivity: */
		int rotationalSensitivity=configFile.retrieveValue<int>("./rotationalSensitivity",3);
		snprintf(command,sizeof(command),"MQ%d,%d\r\n",stations[i].id,rotationalSensitivity);
		serialPort.writeString(command);
		delay(0.1);
		
		/* Go back to device's section: */
		configFile.setCurrentSection("..");
		}
	
	/* Enable/disable sonistrip LEDs: */
	if(configFile.retrieveValue<bool>("./enableLEDs",true))
		{
		/* Enable LEDs: */
		#ifdef VERBOSE
		printf("InterSense: Enabling sonistrip LEDs\n");
		fflush(stdout);
		#endif
		serialPort.writeString("ML1\r\n");
		delay(0.1);
		}
	else
		{
		/* Enable LEDs: */
		#ifdef VERBOSE
		printf("InterSense: Disabling sonistrip LEDs\n");
		fflush(stdout);
		#endif
		serialPort.writeString("ML0\r\n");
		delay(0.1);
		}
	
	/* Set unit mode to inches: */
	#ifdef VERBOSE
	printf("InterSense: Setting unit mode\n");
	fflush(stdout);
	#endif
	serialPort.writeByte('U');
	delay(0.1);
	
	/* Enable binary mode: */
	#ifdef VERBOSE
	printf("InterSense: Enabling binary mode\n");
	fflush(stdout);
	#endif
	serialPort.writeByte('f');
	
	/* Set number of buttons and valuators: */
	setNumButtons(totalNumButtons,configFile);
	setNumValuators(totalNumValuators,configFile);
	
	/* Create free-running timers: */
	timers=new Misc::Timer[numTrackers];
	notFirstMeasurements=new bool[numTrackers];
	oldPositionOrientations=new PositionOrientation[numTrackers];
	}

InterSense::~InterSense(void)
	{
	delete[] stations;
	delete[] timers;
	delete[] notFirstMeasurements;
	delete[] oldPositionOrientations;
	}

void InterSense::start(void)
	{
	/* Start device communication thread: */
	startDeviceThread();
	
	/* Enable continuous mode: */
	#ifdef VERBOSE
	printf("InterSense: Enabling continuous mode\n");
	fflush(stdout);
	#endif
	serialPort.writeByte('C');
	}

void InterSense::stop(void)
	{
	/* Disable continuous mode: */
	#ifdef VERBOSE
	printf("InterSense: Disabling continuous mode\n");
	fflush(stdout);
	#endif
	serialPort.writeByte('c');
	
	/* Stop device communication thread: */
	stopDeviceThread();
	}

/*************************************
Object creation/destruction functions:
*************************************/

extern "C" VRDevice* createObjectInterSense(VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager,Misc::ConfigurationFile& configFile)
	{
	VRDeviceManager* deviceManager=static_cast<VRDeviceManager::DeviceFactoryManager*>(factoryManager)->getDeviceManager();
	return new InterSense(factory,deviceManager,configFile);
	}

extern "C" void destroyObjectInterSense(VRDevice* device,VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager)
	{
	delete device;
	}
