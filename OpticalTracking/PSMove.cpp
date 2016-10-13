/***********************************************************************
PSMove - Class to represent a PlayStation Move game controller as an
inertially-tracked input device.
Copyright (c) 2013-2016 Oliver Kreylos

This file is part of the optical/inertial sensor fusion tracking
package.

The optical/inertial sensor fusion tracking package is free software;
you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation;
either version 2 of the License, or (at your option) any later version.

The optical/inertial sensor fusion tracking package is distributed in
the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the optical/inertial sensor fusion tracking package; if not, write
to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
Boston, MA 02111-1307 USA
***********************************************************************/

#include "PSMove.h"

#include <string.h>
#include <string>
#include <stdexcept>
#include <Misc/SizedTypes.h>
#include <Misc/FunctionCalls.h>
#include <Misc/MessageLogger.h>
#include <IO/File.h>
#include <IO/OpenFile.h>
#include <RawHID/BusType.h>
#include <Math/Constants.h>

#include "TimeStampSource.h"

/***********************
Methods of class PSMove:
***********************/

void PSMove::initialize(void)
	{
	/* Initialize the calibration data structure: */
	calibrationData.magnetometer=true;
	
	/* Try loading calibration data from a calibration file: */
	std::string calibrationFileName="Calibration-PSMove-";
	calibrationFileName.append(RawHID::Device::getSerialNumber());
	try
		{
		IO::FilePtr calibFile=IO::openFile(calibrationFileName.c_str());
		loadCalibrationData(*calibFile);
		}
	catch(std::runtime_error)
		{
		/* Ignore the error and reset calibration data to the default: */
		initCalibrationData(getAccelerometerScale(),getGyroscopeScale(),getMagnetometerScale());
		}
	
	/* Negate the magnetometer's x and z axes: */
	for(int j=0;j<4;++j)
		{
		calibrationData.magnetometerMatrix(0,j)=-calibrationData.magnetometerMatrix(0,j);
		calibrationData.magnetometerMatrix(2,j)=-calibrationData.magnetometerMatrix(2,j);
		}
	
	/* Initialize the LED ball color: */
	for(int i=0;i<3;++i)
		ledColor[i]=0x00U;
	ledColorChanged=true;
	
	showSamplingError=true;
	}

namespace {

/**************
Helper classes:
**************/

class SensorData // Input report: Receive sensor data from PS Move's IMU
	{
	/* Elements: */
	private:
	Misc::UInt8 pktBuffer[49]; // Buffer to unpack sensor data messages
	public:
	static const TimeStamp packetInterval=11299435; // True update rate between input reports in nanoseconds; update rate 177/2Hz
	static const TimeStamp sampleInterval=5649718; // True update rate between IMU samples in nanoseconds; update rate 177Hz
	Misc::UInt16 sequenceNumber;
	Misc::UInt16 timeStamp;
	int batteryState; // Reported battery state
	int temperature; // Reported device temperature
	
	/* Methods: */
	void get(RawHID::Device& device,TimeStampSource& timeStampSource) // Reads next sensor data packet from given raw HID device and initializes given time stamp source
		{
		/* Read the next input report: */
		memset(pktBuffer,0,sizeof(pktBuffer));
		pktBuffer[0]=0x01U;
		device.readReport(pktBuffer,sizeof(pktBuffer));
		
		/* Unpack the report sequence number and time stamp: */
		sequenceNumber=Misc::UInt16(pktBuffer[4]&0x0fU);
		timeStamp=(Misc::UInt16(pktBuffer[11])<<8)|Misc::UInt16(pktBuffer[43]);
		
		struct // Helper bitfield structure to sign-extend a 12-bit integer
			{
			signed int si:12;
			} s;
		
		/* Unpack the battery and temperature state: */
		batteryState=int(pktBuffer[12]);
		temperature=s.si=((int(pktBuffer[37])<<4)|(int(pktBuffer[38])>>4))&0x0fff;
		
		/* Initialize the time stamp source: */
		timeStampSource.set();
		}
	unsigned int get(RawHID::Device& device,IMU::RawSample rawSamples[2],PSMove::FeatureState& featureState,TimeStampSource& timeStampSource) // Reads next sensor data packet from given raw HID device directly into the given raw sample structures, updates given time stamp source; returns number of lost and received packets since last call
		{
		/* Read the next input report: */
		memset(pktBuffer,0,sizeof(pktBuffer));
		pktBuffer[0]=0x01U;
		device.readReport(pktBuffer,sizeof(pktBuffer));
		
		/* Extract set of button states and valuator state: */
		featureState.buttons[0]=(pktBuffer[1]&0x01U)!=0;
		featureState.buttons[1]=(pktBuffer[1]&0x08U)!=0;
		featureState.buttons[2]=(pktBuffer[2]&0x10U)!=0;
		featureState.buttons[3]=(pktBuffer[2]&0x20U)!=0;
		featureState.buttons[4]=(pktBuffer[2]&0x40U)!=0;
		featureState.buttons[5]=(pktBuffer[2]&0x80U)!=0;
		featureState.buttons[6]=(pktBuffer[3]&0x01U)!=0;
		featureState.buttons[7]=(pktBuffer[3]&0x08U)!=0;
		featureState.buttons[8]=(pktBuffer[3]&0x10U)!=0;
		
		featureState.valuators[0]=pktBuffer[6];
		
		/* Unpack the report sequence number and time stamp: */
		Misc::UInt16 newSequenceNumber=Misc::UInt16(pktBuffer[4]&0x0fU);
		Misc::UInt16 sequenceNumberDelta=(newSequenceNumber-sequenceNumber)&0x0f;
		sequenceNumber=newSequenceNumber;
		timeStamp=(Misc::UInt16(pktBuffer[11])<<8)|Misc::UInt16(pktBuffer[43]);
		
		/* Advance the time stamp source: */
		timeStampSource.advance(TimeStamp(sequenceNumberDelta)*packetInterval);
		
		struct // Helper bitfield structure to sign-extend a 12-bit integer
			{
			signed int si:12;
			} s;
		
		/* Unpack the battery and temperature state: */
		batteryState=int(pktBuffer[12]);
		temperature=s.si=((int(pktBuffer[37])<<4)|(int(pktBuffer[38])>>4))&0x0fff;
		
		/* Fill in the raw sample structure for the first half-sample: */
		for(int i=0;i<3;++i)
			rawSamples[0].accelerometer[i]=((int(pktBuffer[14+i*2])<<8)|int(pktBuffer[13+i*2]))-0x8000;
		for(int i=0;i<3;++i)
			rawSamples[0].gyroscope[i]=((int(pktBuffer[26+i*2])<<8)|int(pktBuffer[25+i*2]))-0x8000;
		rawSamples[0].magnetometer[0]=s.si=((int(pktBuffer[38])<<8)|int(pktBuffer[39]))&0x0fff;
		rawSamples[0].magnetometer[1]=s.si=((int(pktBuffer[40])<<4)|(int(pktBuffer[41])>>4))&0x0fff;
		rawSamples[0].magnetometer[2]=s.si=((int(pktBuffer[41])<<8)|int(pktBuffer[42]))&0x0fff;
		
		/* Fill in the raw sample callback structure for the second half-sample: */
		for(int i=0;i<3;++i)
			rawSamples[1].accelerometer[i]=((int(pktBuffer[20+i*2])<<8)|int(pktBuffer[19+i*2]))-0x8000;
		for(int i=0;i<3;++i)
			rawSamples[1].gyroscope[i]=((int(pktBuffer[32+i*2])<<8)|int(pktBuffer[31+i*2]))-0x8000;
		for(int i=0;i<3;++i)
			rawSamples[1].magnetometer[i]=rawSamples[0].magnetometer[i];
		
		return sequenceNumberDelta;
		}
	};

}

void* PSMove::samplingThreadMethod(void)
	{
	/* Enable thread cancellation: */
	Threads::Thread::setCancelState(Threads::Thread::CANCEL_ENABLE);
	// Threads::Thread::setCancelType(Threads::Thread::CANCEL_ASYNCHRONOUS);
	
	try
		{
		/*******************************************************************
		Warm-up period: Collect an initial set of samples to establish an
		initial offset between the PS Move's internal clock and the CPU's
		wall clock.
		*******************************************************************/
		
		SensorData sensorData;
		int oldBatteryState=-1;
		TimeStampSource timeStampSource(1000000,1000);
		
		/* Read the first input report to initialize time stamp synchronization: */
		sensorData.get(*this,timeStampSource);
		
		/* Read the first batch of input reports until raw time stamps stabilize: */
		RawSample rawSamples[2];
		FeatureState featureState;
		for(int i=0;i<2;++i)
			rawSamples[i].warmup=true;
		for(int numReports=0;numReports<10;++numReports)
			{
			/* Read the next input report: */
			sensorData.get(*this,rawSamples,featureState,timeStampSource);
			
			/* Send off each raw sample: */
			rawSamples[0].timeStamp=timeStampSource.get()-SensorData::sampleInterval;
			sendSample(rawSamples[0]);
			rawSamples[1].timeStamp=timeStampSource.get();
			sendSample(rawSamples[1]);
			
			/* Send off new feature state if requested: */
			if(featureStateCallback!=0)
				(*featureStateCallback)(featureState);
			
			/* Check battery state: */
			if(oldBatteryState!=sensorData.batteryState)
				{
				if(batteryStateCallback!=0)
					(*batteryStateCallback)(sensorData.batteryState);
				else if(sensorData.batteryState==0)
					Misc::userWarning("PSMove: Battery is critically low");
				else if(sensorData.batteryState==0xee)
					Misc::userNote("PSMove: Battery is charging");
				else if(sensorData.batteryState==0xef)
					Misc::userNote("PSMove: Battery is fully charged");
				
				oldBatteryState=sensorData.batteryState;
				}
			}
		
		/*******************************************************************
		Main tracking loop: Collect and distribute samples while keeping the
		two timers synchronized.
		*******************************************************************/
		
		/* Process further samples in "regular mode" until interrupted: */
		for(int i=0;i<2;++i)
			rawSamples[i].warmup=false;
		int timeToLedUpdate=0;
		while(keepSampling)
			{
			if(ledColorChanged||timeToLedUpdate<=0)
				{
				/* Send a setLED report: */
				unsigned char setLedReport[49];
				memset(setLedReport,0,sizeof(setLedReport));
				setLedReport[0]=0x02U;
				for(int i=0;i<3;++i)
					setLedReport[2+i]=ledColor[i];
				setLedReport[5]=0U;
				setLedReport[6]=0U;
				try
					{
					writeReport(setLedReport,sizeof(setLedReport));
					}
				catch(std::runtime_error)
					{
					/* Bug in new kernel's hidraw bluetooth stack; returns 0 on successful write */
					}
				
				/* Send another LED update in 2 seconds: */
				timeToLedUpdate=177;
				ledColorChanged=false;
				}
			
			/* Read the next input report: */
			timeToLedUpdate-=sensorData.get(*this,rawSamples,featureState,timeStampSource);
			
			/* Send off each raw sample: */
			rawSamples[0].timeStamp=timeStampSource.get()-SensorData::sampleInterval;
			sendSample(rawSamples[0]);
			rawSamples[1].timeStamp=timeStampSource.get();
			sendSample(rawSamples[1]);
			
			/* Send off new feature state if requested: */
			if(featureStateCallback!=0)
				(*featureStateCallback)(featureState);
			
			/* Check battery state: */
			if(oldBatteryState!=sensorData.batteryState)
				{
				if(batteryStateCallback!=0)
					(*batteryStateCallback)(sensorData.batteryState);
				else if(sensorData.batteryState==0)
					Misc::formattedUserWarning("PSMove: Battery on device %s is critically low",getSerialNumber().c_str());
				else if(sensorData.batteryState==0xee)
					Misc::formattedUserNote("PSMove: Battery on device %s is charging",getSerialNumber().c_str());
				else if(sensorData.batteryState==0xef)
					Misc::formattedUserNote("PSMove: Battery on device %s is fully charged",getSerialNumber().c_str());
				
				oldBatteryState=sensorData.batteryState;
				}
			}
		}
	catch(std::runtime_error err)
		{
		if(showSamplingError)
			Misc::formattedUserError("PSMove::samplingThreadMethod: Terminating due to exception %s",err.what());
		}
	
	return 0;
	}

PSMove::PSMove(const char* devnode,const char* serialNumber)
	:RawHID::Device(devnode,RawHID::BUSTYPE_BLUETOOTH,0x054cU,0x03d5U,serialNumber),
	 featureStateCallback(0),batteryStateCallback(0),
	 keepSampling(false)
	{
	initialize();
	}

PSMove::PSMove(unsigned int deviceIndex)
	:RawHID::Device(RawHID::BUSTYPE_BLUETOOTH,0x054cU,0x03d5U,deviceIndex),
	 featureStateCallback(0),batteryStateCallback(0),
	 keepSampling(false)
	{
	initialize();
	}

PSMove::PSMove(const std::string& deviceSerialNumber)
	:RawHID::Device(RawHID::BUSTYPE_BLUETOOTH,0x054cU,0x03d5U,deviceSerialNumber),
	 featureStateCallback(0),batteryStateCallback(0),
	 keepSampling(false)
	{
	initialize();
	}

PSMove::~PSMove(void)
	{
	/* Shut down the background sampling thread if it is still active: */
	if(keepSampling)
		{
		keepSampling=false;
		samplingThread.join();
		}
	
	delete featureStateCallback;
	delete batteryStateCallback;
	}

std::string PSMove::getSerialNumber(void) const
	{
	/* Prefix the HID device's serial number with the device class: */
	std::string result="PSMove-";
	result.append(RawHID::Device::getSerialNumber());
	return result;
	}

IMU::Scalar PSMove::getAccelerometerScale(void) const
	{
	/* One g corresponds to 4096 in raw units: */
	return Scalar(9.81/4096.0);
	}

IMU::Scalar PSMove::getGyroscopeScale(void) const
	{
	/* Raw measurements are in decidegrees/s: */
	return Scalar(0.1)*Math::Constants<Scalar>::pi/Scalar(180);
	}

IMU::Scalar PSMove::getMagnetometerScale(void) const
	{
	/* Raw measurements are 1/3 uT: */
	return Scalar(1.0/3.0);
	}

void PSMove::startStreamingRaw(IMU::RawSampleCallback* newRawSampleCallback)
	{
	/* Install the new raw sample callback: */
	IMU::startStreamingRaw(newRawSampleCallback);
	
	/* Start the background sampling thread: */
	keepSampling=true;
	samplingThread.start(this,&PSMove::samplingThreadMethod);
	}

void PSMove::startStreamingCalibrated(IMU::CalibratedSampleCallback* newCalibratedSampleCallback)
	{
	/* Install the new calibrated sample callback: */
	IMU::startStreamingCalibrated(newCalibratedSampleCallback);
	
	/* Start the background sampling thread: */
	keepSampling=true;
	samplingThread.start(this,&PSMove::samplingThreadMethod);
	}

void PSMove::stopStreaming(void)
	{
	if(!keepSampling)
		return;
	
	/* Shut down the background sampling thread: */
	keepSampling=false;
	samplingThread.join();
	
	/* Delete the streaming callback: */
	IMU::stopStreaming();
	}

void PSMove::disableSamplingError(void)
	{
	showSamplingError=false;
	}

void PSMove::setFeatureStateCallback(PSMove::FeatureStateCallback* newFeatureStateCallback)
	{
	if(keepSampling)
		throw std::runtime_error("PSMove::setFeatureStateCallback: Cannot change callbacks while streaming is active");
	
	delete featureStateCallback;
	featureStateCallback=newFeatureStateCallback;
	}

void PSMove::setBatteryStateCallback(PSMove::BatteryStateCallback* newBatteryStateCallback)
	{
	if(keepSampling)
		throw std::runtime_error("PSMove::setBatteryStateCallback: Cannot change callbacks while streaming is active");
	
	delete batteryStateCallback;
	batteryStateCallback=newBatteryStateCallback;
	}

void PSMove::setLedColor(unsigned char red,unsigned char green,unsigned char blue)
	{
	/* Set the new LED color and notify the sampling thread: */
	ledColor[0]=red;
	ledColor[1]=green;
	ledColor[2]=blue;
	ledColorChanged=true;
	
	if(!keepSampling) // Sampling thread isn't actually running; send a feature report directly and hope for the best
		{
		/* Send a setLED report: */
		unsigned char setLedReport[49];
		memset(setLedReport,0,sizeof(setLedReport));
		setLedReport[0]=0x02U;
		for(int i=0;i<3;++i)
			setLedReport[2+i]=ledColor[i];
		setLedReport[5]=0U;
		setLedReport[6]=0U;
		try
			{
			writeReport(setLedReport,sizeof(setLedReport));
			}
		catch(std::runtime_error)
			{
			/* Bug in new kernel's hidraw bluetooth stack; returns 0 on successful write */
			}
		
		ledColorChanged=false;
		}
	}
