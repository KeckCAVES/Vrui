/***********************************************************************
LatencyTester - Vislet class to measure the frame-to-display latency of
arbitrary Vrui applications using an Oculus latency tester.
Copyright (c) 2016 Oliver Kreylos

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

#include <Vrui/Vislets/LatencyTester.h>

#include <stdexcept>
#include <Misc/SizedTypes.h>
#include <Misc/MessageLogger.h>
#include <RawHID/BusType.h>
#include <RawHID/Device.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLContextData.h>
#include <Vrui/Vrui.h>
#include <Vrui/VisletManager.h>

// DEBUGGING
#include <iostream>

namespace Vrui {

namespace Vislets {

/*************************************
Methods of class LatencyTesterFactory:
*************************************/

LatencyTesterFactory::LatencyTesterFactory(VisletManager& visletManager)
	:VisletFactory("LatencyTester",visletManager)
	{
	#if 0
	/* Insert class into class hierarchy: */
	VisletFactory* visletFactory=visletManager.loadClass("Vislet");
	visletFactory->addChildClass(this);
	addParentClass(visletFactory);
	#endif
	
	/* Set vislet class' factory pointer: */
	LatencyTester::factory=this;
	}

LatencyTesterFactory::~LatencyTesterFactory(void)
	{
	/* Reset vislet class' factory pointer: */
	LatencyTester::factory=0;
	}

Vislet* LatencyTesterFactory::createVislet(int numArguments,const char* const arguments[]) const
	{
	return new LatencyTester(numArguments,arguments);
	}

void LatencyTesterFactory::destroyVislet(Vislet* vislet) const
	{
	delete vislet;
	}

extern "C" void resolveLatencyTesterDependencies(Plugins::FactoryManager<VisletFactory>& manager)
	{
	#if 0
	/* Load base classes: */
	manager.loadClass("Vislet");
	#endif
	}

extern "C" VisletFactory* createLatencyTesterFactory(Plugins::FactoryManager<VisletFactory>& manager)
	{
	/* Get pointer to vislet manager: */
	VisletManager* visletManager=static_cast<VisletManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	LatencyTesterFactory* factory=new LatencyTesterFactory(*visletManager);
	
	/* Return factory object: */
	return factory;
	}

extern "C" void destroyLatencyTesterFactory(VisletFactory* factory)
	{
	delete factory;
	}

namespace {

/***********************************************************************
Helper classes and functions to communicate with Oculus' latency tester:
***********************************************************************/

struct Color
	{
	/* Elements: */
	public:
	Misc::UInt8 r,g,b;
	
	/* Constructors and destructors: */
	Color(void)
		{
		}
	Color(Misc::UInt8 sR,Misc::UInt8 sG,Misc::UInt8 sB)
		:r(sR),g(sG),b(sB)
		{
		}
	};

inline Misc::UInt8 decodeUInt8(Misc::UInt8*& bufPtr)
	{
	return *(bufPtr++);
	}

inline Misc::UInt16 decodeUInt16(Misc::UInt8*& bufPtr)
	{
	Misc::UInt16 result=Misc::UInt16(bufPtr[0])|(Misc::UInt16(bufPtr[1])<<8);
	bufPtr+=2;
	return result;
	}

inline Color decodeColor(Misc::UInt8*& bufPtr)
	{
	Color result;
	result.r=*(bufPtr++);
	result.g=*(bufPtr++);
	result.b=*(bufPtr++);
	return result;
	}

}

/*******************************************
Class representing an Oculus latency tester:
*******************************************/

class LatencyTesterDevice:public RawHID::Device
	{
	/* Elements: */
	private:
	Misc::UInt16 nextTestId; // ID to associate test requests and their results
	
	/* Constructors and destructors: */
	public:
	LatencyTesterDevice(int busTypeMask,unsigned short sVendorId,unsigned short sProductId,unsigned int index)
		:RawHID::Device(busTypeMask,sVendorId,sProductId,index),
		 nextTestId(1U)
		{
		}
	
	/* Methods: */
	void setLatencyConfiguration(bool sendSamples,const Color& threshold)
		{
		/* Assemble the feature report: */
		Misc::UInt8 packet[5];
		packet[0]=0x05U;
		packet[1]=sendSamples?0x01U:0x00U;
		packet[2]=threshold.r;
		packet[3]=threshold.g;
		packet[4]=threshold.b;
		
		/* Send the feature report: */
		writeFeatureReport(packet,sizeof(packet));
		}
	void setLatencyCalibration(const Color& calibration)
		{
		/* Assemble the feature report: */
		Misc::UInt8 packet[4];
		packet[0]=0x07U;
		packet[1]=calibration.r;
		packet[2]=calibration.g;
		packet[3]=calibration.b;
		
		/* Send the feature report: */
		writeFeatureReport(packet,sizeof(packet));
		}
	void startLatencyTest(const Color& target)
		{
		/* Assemble the feature report: */
		Misc::UInt8 packet[6];
		packet[0]=0x08U;
		packet[1]=Misc::UInt8(nextTestId&0xffU);
		packet[2]=Misc::UInt8((nextTestId>>8)&0xffU);
		packet[3]=target.r;
		packet[4]=target.g;
		packet[5]=target.b;
		
		/* Send the feature report: */
		writeFeatureReport(packet,sizeof(packet));
		
		/* Increment the test ID: */
		++nextTestId;
		}
	void setLatencyDisplay(Misc::UInt8 mode,Misc::UInt32 value)
		{
		/* Assemble the feature report: */
		Misc::UInt8 packet[6];
		packet[0]=0x09U;
		packet[1]=mode;
		for(int i=0;i<4;++i)
			{
			packet[2+i]=Misc::UInt8(value&0xffU);
			value>>=8;
			}
		
		/* Send the feature report: */
		writeFeatureReport(packet,sizeof(packet));
		}
	};

/**************************************
Static elements of class LatencyTester:
**************************************/

LatencyTesterFactory* LatencyTester::factory=0;

/******************************
Methods of class LatencyTester:
******************************/

void* LatencyTester::communicationThreadMethod(void)
	{
	unsigned int accumR,accumG,accumB;
	unsigned int accumSum;
	
	while(true)
		{
		/* Wait for and read the next raw HID report: */
		Misc::UInt8 buffer[64]; // 64 is largest message size
		size_t messageSize=device->readReport(buffer,sizeof(buffer));
		
		/* Process the report: */
		Misc::UInt8* bufPtr=buffer+1;
		switch(buffer[0])
			{
			case 0x01U: // Sample report
				if(messageSize==64)
					{
					/* Read the number of samples in this report and the sample timestamp: */
					unsigned int numSamples=decodeUInt8(bufPtr);
					unsigned int timeStamp=decodeUInt16(bufPtr);
					
					/* Add all samples to the accumulator: */
					for(unsigned int i=0;i<numSamples;++i)
						{
						Color sample=decodeColor(bufPtr);
						accumR+=sample.r;
						accumG+=sample.g;
						accumB+=sample.b;
						++accumSum;
						}
					}
				else
					Misc::consoleWarning("Received malformed sample report");
				break;
			
			case 0x02U: // Color_detected report
				if(messageSize==13)
					{
					/* Read the command ID, timestamp, and elapsed time: */
					unsigned int commandId=decodeUInt16(bufPtr);
					unsigned int timeStamp=decodeUInt16(bufPtr);
					unsigned int elapsed=decodeUInt16(bufPtr);
					
					/* Read the trigger and target colors: */
					Color trigger=decodeColor(bufPtr);
					Color target=decodeColor(bufPtr);
					
					if(testState==WAITING_BLACK)
						{
						std::cout<<"White to black: "<<elapsed<<"ms"<<std::endl;
						testState=PREPARE_WHITE1;
						Vrui::requestUpdate();
						}
					else if(testState==WAITING_WHITE)
						{
						std::cout<<"Black to white: "<<elapsed<<"ms"<<std::endl;
						testState=PREPARE_BLACK1;
						Vrui::requestUpdate();
						}
					}
				else
					Misc::consoleWarning("Received malformed color_detected report");
				break;
			
			case 0x03U: // Test_started report
				if(messageSize==8)
					{
					/* Read the command ID and timestamp: */
					unsigned int commandId=decodeUInt16(bufPtr);
					unsigned int timeStamp=decodeUInt16(bufPtr);
					
					/* Read the target color: */
					Color target=decodeColor(bufPtr);
					}
				else
					Misc::consoleWarning("Received malformed test_started report");
				break;
			
			case 0x04U: // Button report
				if(messageSize==5)
					{
					/* Read the command ID and timestamp: */
					unsigned int commandId=decodeUInt16(bufPtr);
					unsigned int timeStamp=decodeUInt16(bufPtr);
					
					/* Start a new latency test run if idle, or stop a going one: */
					if(testState==IDLE)
						testState=SAMPLING_BLACK_PREP1;
					else if(testState>SAMPLING_WHITE)
						testState=FINISH;
					Vrui::requestUpdate();
					}
				else
					Misc::consoleWarning("Received malformed button report");
				break;
			}
		}
	
	return 0;
	}

LatencyTester::LatencyTester(int numArguments,const char* const arguments[])
	:device(0),testState(IDLE),
	 override(false),color(1.0f,0.0f,1.0f,1.0f)
	{
	/* Parse the command line: */
	for(int i=0;i<numArguments;++i)
		{
		if(arguments[i][0]=='-')
			{
			if(false)
				{
				}
			else
				Misc::formattedConsoleError("LatencyTester: Ignoring unknown %s option",arguments[i]);
			}
		else
			Misc::formattedConsoleError("LatencyTester: Ignoring unknown %s parameter",arguments[i]);
		}
	
	try
		{
		/* Connect to the first latency tester device on the USB bus: */
		device=new LatencyTesterDevice(RawHID::BUSTYPE_USB,0x2833U,0x0101U,0);
		Misc::formattedConsoleNote("Vrui::LatencyTester: Connected to Oculus Rift latency tester with serial # %s",device->getSerialNumber().c_str());
		device->setLatencyConfiguration(false,Color(128,128,128));
		device->setLatencyDisplay(2,0x40400040U);
		
		/* Start the communication thread: */
		communicationThread.start(this,&LatencyTester::communicationThreadMethod);
		}
	catch(std::runtime_error err)
		{
		Misc::formattedConsoleError("Vrui::LatencyTester: Unable to connect to Oculus Rift latency tester due to exception %s",err.what());
		}
	}

LatencyTester::~LatencyTester(void)
	{
	if(!communicationThread.isJoined())
		{
		/* Stop the communication thread: */
		communicationThread.cancel();
		communicationThread.join();
		}
	
	/* Disconnect from the latency tester: */
	delete device;
	}

VisletFactory* LatencyTester::getFactory(void) const
	{
	return factory;
	}

void LatencyTester::frame(void)
	{
	switch(testState)
		{
		case SAMPLING_BLACK_PREP1:
			override=true;
			color=GLColor<GLfloat,4>(0.0f,0.0f,0.0f);
			testState=SAMPLING_BLACK_PREP2;
			stateAdvanceTime=Vrui::getApplicationTime()+0.15; // 150ms for new color to settle
			Vrui::scheduleUpdate(stateAdvanceTime);
			break;
		
		case SAMPLING_BLACK_PREP2:
			if(Vrui::getApplicationTime()>=stateAdvanceTime)
				{
				// device->setLatencyCalibration(Color(0,0,0));
				testState=SAMPLING_BLACK;
				stateAdvanceTime=Vrui::getApplicationTime()+0.3; // 300ms for calibration to finish
				}
			Vrui::scheduleUpdate(stateAdvanceTime);
			break;
		
		case SAMPLING_BLACK:
			if(Vrui::getApplicationTime()>=stateAdvanceTime)
				{
				color=GLColor<GLfloat,4>(1.0f,1.0f,1.0f);
				testState=SAMPLING_WHITE_PREP;
				stateAdvanceTime=Vrui::getApplicationTime()+0.15; // 150ms for new color to settle
				}
			Vrui::scheduleUpdate(stateAdvanceTime);
			break;
		
		case SAMPLING_WHITE_PREP:
			if(Vrui::getApplicationTime()>=stateAdvanceTime)
				{
				// device->setLatencyCalibration(Color(255,255,255));
				testState=SAMPLING_WHITE;
				stateAdvanceTime=Vrui::getApplicationTime()+0.3; // 300ms for calibration to finish
				}
			Vrui::scheduleUpdate(stateAdvanceTime);
			break;
		
		case SAMPLING_WHITE:
			if(Vrui::getApplicationTime()>=stateAdvanceTime)
				{
				color=GLColor<GLfloat,4>(0.0f,0.0f,0.0f);
				device->startLatencyTest(Color(0,0,0));
				testState=WAITING_BLACK;
				Vrui::requestUpdate();
				}
			else
				Vrui::scheduleUpdate(stateAdvanceTime);
			break;
		
		case WAITING_BLACK:
			Vrui::requestUpdate();
			break;
		
		case PREPARE_WHITE1:
			testState=PREPARE_WHITE2;
			stateAdvanceTime=Vrui::getApplicationTime()+0.1;
			break;
		
		case PREPARE_WHITE2:
			if(Vrui::getApplicationTime()>=stateAdvanceTime)
				{
				color=GLColor<GLfloat,4>(1.0f,1.0f,1.0f);
				device->startLatencyTest(Color(255,255,255));
				testState=WAITING_WHITE;
				}
			Vrui::requestUpdate();
			break;
		
		case WAITING_WHITE:
			Vrui::requestUpdate();
			break;
		
		case PREPARE_BLACK1:
			testState=PREPARE_BLACK2;
			stateAdvanceTime=Vrui::getApplicationTime()+0.1;
			break;
		
		case PREPARE_BLACK2:
			if(Vrui::getApplicationTime()>=stateAdvanceTime)
				{
				color=GLColor<GLfloat,4>(0.0f,0.0f,0.0f);
				device->startLatencyTest(Color(0,0,0));
				testState=WAITING_BLACK;
				}
			Vrui::requestUpdate();
			break;
		
		case FINISH:
			override=false;
			testState=IDLE;
			break;
		
		default:
			;
		}
	}

void LatencyTester::display(GLContextData& contextData) const
	{
	if(override)
		{
		/* Retrieve the data item and activate the display override shader: */
		DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
		dataItem->displayOverrideShader.useProgram();
		
		/* Set the draw color: */
		glUniformARB(dataItem->displayOverrideShaderUniforms[0],color);
		
		/* Draw a display-filling quad on the near plane: */
		glBegin(GL_QUADS);
		glVertex3i(-1,-1,-1);
		glVertex3i( 1,-1,-1);
		glVertex3i( 1, 1,-1);
		glVertex3i(-1, 1,-1);
		glEnd();
		
		/* Disable the shader: */
		GLShader::disablePrograms();
		}
	}

void LatencyTester::initContext(GLContextData& contextData) const
	{
	/* Create a context state item and associate it with the context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Create the display override shader: */
	static const char* displayOverrideShaderVertexProgramSource="\
		void main()\n\
			{\n\
			gl_Position=gl_Vertex;\n\
			}\n";
	static const char* displayOverrideShaderFragmentProgramSource="\
		uniform vec4 color;\n\
		\n\
		void main()\n\
			{\n\
			gl_FragColor=color;\n\
			}\n";
	dataItem->displayOverrideShader.compileVertexShaderFromString(displayOverrideShaderVertexProgramSource);
	dataItem->displayOverrideShader.compileFragmentShaderFromString(displayOverrideShaderFragmentProgramSource);
	dataItem->displayOverrideShader.linkShader();
	dataItem->displayOverrideShaderUniforms[0]=dataItem->displayOverrideShader.getUniformLocation("color");
	}

}

}
