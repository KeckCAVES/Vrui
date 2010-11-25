/***********************************************************************
ArtDTrack - Class for ART DTrack tracking devices.
Copyright (c) 2004-2010 Oliver Kreylos

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

#include <VRDeviceDaemon/VRDevices/ArtDTrack.h>

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <Misc/Time.h>
#include <Misc/Endianness.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/Matrix.h>

#include <VRDeviceDaemon/VRDeviceManager.h>

namespace Misc {

/**********************************
Value coder class for data formats:
**********************************/

template <>
class ValueCoder<ArtDTrack::DataFormat>
	{
	/* Methods: */
	public:
	static std::string encode(const ArtDTrack::DataFormat& df)
		{
		std::string result;
		switch(df)
			{
			case ArtDTrack::ASCII:
				result="ASCII";
				break;
			
			case ArtDTrack::BINARY:
				result="Binary";
				break;
			}
		
		return result;
		}
	static ArtDTrack::DataFormat decode(const char* start,const char* end,const char** decodeEnd =0)
		{
		if(end-start>=5&&strncasecmp(start,"ASCII",5)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+5;
			return ArtDTrack::ASCII;
			}
		else if(end-start>=6&&strncasecmp(start,"Binary",6)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+6;
			return ArtDTrack::BINARY;
			}
		else
			throw DecodingError(std::string("Unable to convert \"")+std::string(start,end)+std::string("\" to ArtDTrack::DataFormat"));
		}
	};

}

namespace {

/***************************************************************
Helper functions to extract data from DTrack ASCII message body:
***************************************************************/

int readInt(char*& mPtr)
	{
	/* Parse an integer: */
	char* intEnd;
	int result=int(strtol(mPtr,&intEnd,10));
	mPtr=intEnd;
	return result;
	}

unsigned int readUint(char*& mPtr)
	{
	/* Parse an integer: */
	char* intEnd;
	unsigned int result=(unsigned int)(strtoul(mPtr,&intEnd,10));
	mPtr=intEnd;
	return result;
	}

double readFloat(char*& mPtr)
	{
	/* Parse a float: */
	char* floatEnd;
	double result=strtod(mPtr,&floatEnd);
	mPtr=floatEnd;
	return result;
	}

/****************************************************************
Helper functions to extract data from DTrack binary message body:
****************************************************************/

template <class DataParam>
inline
DataParam
extractData(
	const char*& dataPtr)
	{
	/* Extract data item: */
	DataParam result=*reinterpret_cast<const DataParam*>(dataPtr);
	
	#if __BYTE_ORDER==__BIG_ENDIAN
	/* Convert endianness of data item: */
	Misc::swapEndianness(result);
	#endif
	
	/* Advance data pointer: */
	dataPtr+=sizeof(DataParam);
	
	/* Return data item: */
	return result;
	}

template <class DataParam>
inline
void
skipData(
	const char*& dataPtr)
	{
	/* Advance data pointer: */
	dataPtr+=sizeof(DataParam);
	}

}

/**************************
Methods of class ArtDTrack:
**************************/

void ArtDTrack::processAsciiData(void)
	{
	while(true)
		{
		/* Wait for the next data message from the DTrack daemon: */
		char messageBuffer[4096];
		size_t messageSize=dataSocket.receiveMessage(messageBuffer,sizeof(messageBuffer)-1);
		
		/* Newline-terminate the message: */
		messageBuffer[messageSize]='\n';
		
		/* Parse the received message: */
		char* mPtr=messageBuffer;
		char* mEnd=messageBuffer+messageSize;
		while(mPtr<mEnd)
			{
			/* Find the end of the current line: */
			char* lineEnd;
			for(lineEnd=mPtr;*lineEnd!='\n';++lineEnd)
				;
			
			/* Skip whitespace: */
			while(mPtr<lineEnd&&isspace(*mPtr))
				++mPtr;
			
			/* Get the line identifier: */
			char* idStart=mPtr;
			while(mPtr<lineEnd&&!isspace(*mPtr))
				++mPtr;
			
			/* Determine the type of the line: */
			int lineType=-1;
			if(mPtr-idStart==2&&strncasecmp(idStart,"6d",2)==0)
				lineType=0;
			else if(mPtr-idStart==3&&strncasecmp(idStart,"6df",3)==0)
				lineType=1;
			else if(mPtr-idStart==4&&strncasecmp(idStart,"6df2",4)==0)
				lineType=2;
			else if(mPtr-idStart==4&&strncasecmp(idStart,"6dmt",4)==0)
				lineType=3;
			else if(mPtr-idStart==2&&strncasecmp(idStart,"3d",2)==0)
				lineType=4;
			
			/* Process the line: */
			if(lineType>=0)
				{
				if(lineType==2)
					{
					/* Skip the number of defined flysticks: */
					readInt(mPtr);
					}
				
				/* Read the number of reported bodies: */
				int numBodies=readInt(mPtr);
				
				/* Parse each body: */
				for(int body=0;body<numBodies;++body)
					{
					/* Find the first opening bracket: */
					while(mPtr<lineEnd&&isspace(*mPtr))
						++mPtr;
					if(*mPtr!='[')
						break; // Ignore the rest of the line
					++mPtr;
					
					/* Read the body's ID: */
					int id=readInt(mPtr);
					Device* device=deviceIdToIndex[id]>=0?&devices[deviceIdToIndex[id]]:0;
					
					/* Skip the quality value: */
					float(readFloat(mPtr));
					
					if(lineType==1||lineType==3)
						{
						/* Read the button bit flag: */
						unsigned int bits=readUint(mPtr);
						
						if(device!=0)
							{
							/* Set the button states: */
							for(int i=0;i<32&&i<device->numButtons;++i)
								{
								setButtonState(device->firstButtonIndex+i,(bits&0x1)!=0x0);
								bits>>=1;
								}
							}
						}
					
					int numButtons=0;
					int numValuators=0;
					if(lineType==2)
						{
						/* Read the number of buttons and valuators: */
						numButtons=readInt(mPtr);
						numValuators=readInt(mPtr);
						}
					
					/* Find the first closing bracket: */
					while(mPtr<lineEnd&&isspace(*mPtr))
						++mPtr;
					if(*mPtr!=']')
						break; // Ignore the rest of the line
					++mPtr;
					
					/* Find the second opening bracket: */
					while(mPtr<lineEnd&&isspace(*mPtr))
						++mPtr;
					if(*mPtr!='[')
						break; // Ignore the rest of the line
					++mPtr;
					
					/* Read the body's position: */
					Vector pos;
					for(int i=0;i<3;++i)
						pos[i]=VScalar(readFloat(mPtr));
					
					Rotation orient=Rotation::identity;
					if(lineType==0||lineType==1)
						{
						/* Read the body's orientation angles: */
						VScalar angles[3];
						for(int i=0;i<3;++i)
							angles[i]=VScalar(readFloat(mPtr));
						
						/* Calculate the body's orientation quaternion: */
						orient*=Rotation::rotateX(Math::rad(angles[0]));
						orient*=Rotation::rotateY(Math::rad(angles[1]));
						orient*=Rotation::rotateZ(Math::rad(angles[2]));
						}
					
					/* Find the second closing bracket: */
					while(mPtr<lineEnd&&isspace(*mPtr))
						++mPtr;
					if(*mPtr!=']')
						break; // Ignore the rest of the line
					++mPtr;
					
					if(lineType!=4)
						{
						/* Find the third opening bracket: */
						while(mPtr<lineEnd&&isspace(*mPtr))
							++mPtr;
						if(*mPtr!='[')
							break; // Ignore the rest of the line
						++mPtr;
						}
					
					if(lineType==2||lineType==3)
						{
						/* Read the body's orientation matrix (yuck!): */
						Geometry::Matrix<VScalar,3,3> matrix;
						for(int j=0;j<3;++j)
							for(int i=0;i<3;++i)
								matrix(i,j)=VScalar(readFloat(mPtr));
						
						/* Calculate the body's orientation quaternion: */
						orient=Rotation::fromMatrix(matrix);
						
						/* Find the third closing bracket: */
						while(mPtr<lineEnd&&isspace(*mPtr))
							++mPtr;
						if(*mPtr!=']')
							break; // Ignore the rest of the line
						++mPtr;
						}
					else if(lineType!=4)
						{
						/* Ignore the body's orientation matrix: */
						while(mPtr<lineEnd&&*mPtr!=']')
							++mPtr;
						if(*mPtr!=']') // Ignore the rest of the line
							break;
						}
					
					if(lineType==2)
						{
						/* Find the fourth opening bracket: */
						while(mPtr<lineEnd&&isspace(*mPtr))
							++mPtr;
						if(*mPtr!='[')
							break; // Ignore the rest of the line
						++mPtr;
						
						/* Read the flystick's button bits: */
						for(int bitIndex=0;bitIndex<numButtons;bitIndex+=32)
							{
							/* Read the next button bit mask: */
							unsigned int bits=readUint(mPtr);
							
							if(device!=0)
								{
								/* Set the device's button values: */
								for(int i=0;i<32;++i)
									{
									/* Set the button state if the button is valid: */
									if(bitIndex+i<device->numButtons)
										setButtonState(device->firstButtonIndex+bitIndex+i,(bits&0x1)!=0x0);
									bits>>=1;
									}
								}
							}
						
						/* Read the flystick's valuator values: */
						for(int i=0;i<numValuators;++i)
							{
							/* Read the next valuator value: */
							float value=float(readFloat(mPtr));
							
							/* Set the valuator value if the valuator is valid: */
							if(device!=0&&i<device->numValuators)
								setValuatorState(device->firstValuatorIndex+i,value);
							}
						
						/* Find the fourth closing bracket: */
						while(mPtr<lineEnd&&isspace(*mPtr))
							++mPtr;
						if(*mPtr!=']')
							break; // Ignore the rest of the line
						++mPtr;
						}
					
					/* Check if this body has been configured as a device: */
					if(device!=0)
						{
						/* Set the device's tracker state: */
						trackerStates[deviceIdToIndex[id]].positionOrientation=PositionOrientation(pos,orient);
						}
					}
				}
			
			/* Go to the start of the next line: */
			mPtr=lineEnd+1;
			}
		
		/* Update all tracker states (including those that were not updated): */
		for(int i=0;i<getNumTrackers();++i)
			setTrackerState(i,trackerStates[i]);
		}
	}

void ArtDTrack::processBinaryData(void)
	{
	while(true)
		{
		/* Wait for the next data message from the DTrack daemon: */
		char messageBuffer[1024];
		dataSocket.receiveMessage(messageBuffer,sizeof(messageBuffer));
		
		/* Parse the received message: */
		const char* mPtr=messageBuffer;
		// unsigned int frameNr=extractData<unsigned int>(mPtr);
		skipData<unsigned int>(mPtr); // Skip frame number
		int numBodies=extractData<int>(mPtr);
		for(int i=0;i<numBodies;++i)
			{
			/* Read body's ID and measurement quality: */
			int trackerId=int(extractData<unsigned int>(mPtr));
			// float quality=extractData<float>(mPtr);
			skipData<float>(mPtr); // Skip measurement quality
			
			/* Read body's position: */
			Vector pos;
			for(int j=0;j<3;++j)
				pos[j]=VScalar(extractData<float>(mPtr));
			
			/* Read body's orientation as Euler angles: */
			RScalar angles[3];
			for(int j=0;j<3;++j)
				angles[j]=Math::rad(extractData<float>(mPtr));
			
			/* Convert Euler angles to rotation: */
			Rotation o=Rotation::identity;
			o*=Rotation::rotateX(angles[0]);
			o*=Rotation::rotateY(angles[1]);
			o*=Rotation::rotateZ(angles[2]);
			
			/* Skip body's orientation as rotation matrix: */
			for(int j=0;j<9;++j)
				extractData<float>(mPtr);
			
			/* Set tracker position and orientation: */
			if(trackerId<getNumTrackers())
				trackerStates[trackerId].positionOrientation=PositionOrientation(pos,o);
			}
		
		/* Update all tracker states (including those that were not updated): */
		for(int i=0;i<getNumTrackers();++i)
			setTrackerState(i,trackerStates[i]);
		}
	}

void ArtDTrack::deviceThreadMethod(void)
	{
	/* Select the appropriate processing method based on data format: */
	switch(dataFormat)
		{
		case ASCII:
			processAsciiData();
			break;
		
		case BINARY:
			processBinaryData();
			break;
		}
	}

ArtDTrack::ArtDTrack(VRDevice::Factory* sFactory,VRDeviceManager* sDeviceManager,Misc::ConfigurationFile& configFile)
	:VRDevice(sFactory,sDeviceManager,configFile),
	 controlSocket(-1,configFile.retrieveString("./serverName"),configFile.retrieveValue<int>("./serverControlPort")),
	 dataSocket(configFile.retrieveValue<int>("./serverDataPort"),0),
	 dataFormat(configFile.retrieveValue<DataFormat>("./dataFormat",BINARY)),
	 devices(0),maxDeviceId(0),deviceIdToIndex(0),
	 trackerStates(0)
	{
	/* Retrieve list of device names: */
	typedef std::vector<std::string> StringList;
	StringList deviceNames=configFile.retrieveValue<StringList>("./deviceNames");
	setNumTrackers(deviceNames.size(),configFile);
	devices=new Device[numTrackers];
	int totalNumButtons=0;
	int totalNumValuators=0;
	
	/* Initialize all configured devices: */
	#ifdef VERBOSE
	printf("ArtDTrack: Initializing tracked devices\n");
	fflush(stdout);
	#endif
	for(int i=0;i<numTrackers;++i)
		{
		/* Go to tracked device's section: */
		configFile.setCurrentSection(deviceNames[i].c_str());
		
		/* Read tracked device's configuration: */
		devices[i].id=configFile.retrieveValue<int>("./id",i+1);
		if(maxDeviceId<devices[i].id)
			maxDeviceId=devices[i].id;
		devices[i].numButtons=configFile.retrieveValue<int>("./numButtons",0);
		devices[i].firstButtonIndex=totalNumButtons;
		totalNumButtons+=devices[i].numButtons;
		devices[i].numValuators=configFile.retrieveValue<int>("./numValuators",0);
		devices[i].firstValuatorIndex=totalNumValuators;
		totalNumValuators+=devices[i].numValuators;
		
		/* Go back to device's section: */
		configFile.setCurrentSection("..");
		}
	
	/* Create the device ID to index mapping: */
	deviceIdToIndex=new int[maxDeviceId+1];
	for(int i=0;i<=maxDeviceId;++i)
		deviceIdToIndex[i]=-1;
	for(int i=0;i<numTrackers;++i)
		deviceIdToIndex[devices[i].id]=i;
	
	/* Set number of buttons and valuators: */
	setNumButtons(totalNumButtons,configFile);
	setNumValuators(totalNumValuators,configFile);
	
	/* Initialize tracker states: */
	trackerStates=new Vrui::VRDeviceState::TrackerState[getNumTrackers()];
	for(int i=0;i<getNumTrackers();++i)
		{
		trackerStates[i].positionOrientation=PositionOrientation(Vector::zero,Rotation::identity);
		trackerStates[i].linearVelocity=Vrui::VRDeviceState::TrackerState::LinearVelocity::zero;
		trackerStates[i].angularVelocity=Vrui::VRDeviceState::TrackerState::AngularVelocity::zero;
		}
	}

ArtDTrack::~ArtDTrack(void)
	{
	if(isActive())
		stop();
	delete[] devices;
	delete[] deviceIdToIndex;
	delete[] trackerStates;
	}

void ArtDTrack::start(void)
	{
	/* Start device communication thread: */
	startDeviceThread();
	
	/* Activate DTrack: */
	#ifdef VERBOSE
	printf("ArtDTrack: Activating cameras and reconstruction\n");
	#endif
	const char* msg1="dtrack 10 3";
	controlSocket.sendMessage(msg1,strlen(msg1)+1);
	
	Misc::sleep(0.5);
	
	/* Start sending measurements: */
	#ifdef VERBOSE
	printf("ArtDTrack: Starting continuous update mode\n");
	#endif
	const char* msg2="dtrack 31";
	controlSocket.sendMessage(msg2,strlen(msg2)+1);
	}

void ArtDTrack::stop(void)
	{
	/* Stop sending measurements: */
	#ifdef VERBOSE
	printf("ArtDTrack: Stopping continuous update mode\n");
	#endif
	const char* msg1="dtrack 32";
	controlSocket.sendMessage(msg1,strlen(msg1)+1);
	
	Misc::sleep(0.5);
	
	/* Deactivate DTrack: */
	#ifdef VERBOSE
	printf("ArtDTrack: Deactivating cameras and reconstruction\n");
	#endif
	const char* msg2="dtrack 10 0";
	controlSocket.sendMessage(msg2,strlen(msg2)+1);
	
	/* Stop device communication thread: */
	stopDeviceThread();
	}

/*************************************
Object creation/destruction functions:
*************************************/

extern "C" VRDevice* createObjectArtDTrack(VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager,Misc::ConfigurationFile& configFile)
	{
	VRDeviceManager* deviceManager=static_cast<VRDeviceManager::DeviceFactoryManager*>(factoryManager)->getDeviceManager();
	return new ArtDTrack(factory,deviceManager,configFile);
	}

extern "C" void destroyObjectArtDTrack(VRDevice* device,VRFactory<VRDevice>* factory,VRFactoryManager<VRDevice>* factoryManager)
	{
	delete device;
	}
