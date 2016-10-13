/***********************************************************************
TrackingTest - Vrui application to visualize tracking data received
from a VRDeviceDaemon.
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

#include <stdlib.h>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <Misc/ThrowStdErr.h>
#include <Misc/FunctionCalls.h>
#include <Threads/TripleBuffer.h>
#include <Realtime/Time.h>
#include <GL/gl.h>
#include <GL/GLMaterialTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GL/GLModels.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>
#include <Vrui/CoordinateManager.h>
#include <Vrui/Internal/VRDeviceDescriptor.h>
#include <Vrui/Internal/VRDeviceClient.h>

class TrackingTest:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	typedef Vrui::VRDeviceState::TrackerState TS;
	typedef TS::PositionOrientation PO;
	typedef PO::Scalar Scalar;
	typedef PO::Point Point;
	typedef PO::Vector Vector;
	typedef PO::Rotation Rotation;
	typedef TS::LinearVelocity LV;
	typedef TS::AngularVelocity AV;
	
	/* Elements: */
	Vrui::VRDeviceClient* deviceClient; // Connection to the VRDeviceDaemon
	Geometry::LinearUnit trackingUnit; // Linear unit used by the tracking system
	int numTrackers; // Number of trackers served by the VRDeviceDaemon
	Threads::TripleBuffer<TS*> trackerStates; // Triple buffer of arrays of tracking states
	
	/* Private methods: */
	void trackingCallback(Vrui::VRDeviceClient* client); // Called when new tracking data arrives
	void drawFrame(GLfloat arrowLength,GLfloat arrowRadius) const; // Draws a coordinate frame of the given size at the origin
	
	/* Constructors and destructors: */
	public:
	TrackingTest(int& argc,char**& argv);
	virtual ~TrackingTest(void);
	
	/* Methods from Vrui::Application: */
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

/*****************************
Methods of class TrackingTest:
*****************************/

void TrackingTest::trackingCallback(Vrui::VRDeviceClient* client)
	{
	/* Start a new slot in the tracker states triple buffer: */
	TS* tss=trackerStates.startNewValue();
	
	/* Copy new tracking data into the triple buffer slot: */
	deviceClient->lockState();
	const Vrui::VRDeviceState& state=deviceClient->getState();
	for(int i=0;i<numTrackers;++i)
		tss[i]=state.getTrackerState(i);
	deviceClient->unlockState();
	
	/* Post the new tracking data and request a new Vrui frame: */
	trackerStates.postNewValue();
	Vrui::requestUpdate();
	}

void TrackingTest::drawFrame(GLfloat arrowLength,GLfloat arrowRadius) const
	{
	/* Draw a cross of three arrows: */
	glPushMatrix();
	glMaterialAmbientAndDiffuse(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(1.0f,0.5f,0.5f));
	glRotated(90.0,0.0,1.0,0.0);
	glTranslated(0.0,0.0,arrowRadius);
	glDrawArrow(arrowRadius,arrowRadius*1.5f,arrowRadius*3.0f,arrowLength+arrowRadius*1.5f,16);
	glPopMatrix();
	
	glPushMatrix();
	glMaterialAmbientAndDiffuse(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(0.5f,1.0f,0.5f));
	glRotated(-90.0,1.0,0.0,0.0);
	glTranslated(0.0,0.0,arrowRadius);
	glDrawArrow(arrowRadius,arrowRadius*1.5f,arrowRadius*3.0f,arrowLength+arrowRadius*1.5f,16);
	glPopMatrix();
	
	glPushMatrix();
	glMaterialAmbientAndDiffuse(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(0.5f,0.5f,1.0f));
	glTranslated(0.0,0.0,arrowRadius);
	glDrawArrow(arrowRadius,arrowRadius*1.5f,arrowRadius*3.0f,arrowLength+arrowRadius*1.5f,16);
	glPopMatrix();
	}

TrackingTest::TrackingTest(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 deviceClient(0),
	 trackingUnit(Geometry::LinearUnit::INCH,1.0),
	 numTrackers(0)
	{
	/* Parse command line: */
	char* serverName=0;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"unit")==0)
				{
				++i;
				const char* unitName=argv[i];
				++i;
				double unitFactor=atof(argv[i]);
				trackingUnit=Geometry::LinearUnit(unitName,unitFactor);
				}
			}
		else
			serverName=argv[i];
		}
	
	std::cout<<trackingUnit.getInchFactor()<<std::endl;
	
	if(serverName==0)
		Misc::throwStdErr("Usage: %s <servername>:<serverport>",argv[0]);
	
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
	
	/* Initialize device client: */
	deviceClient=new Vrui::VRDeviceClient(serverName,portNumber);
	
	/* Get the number of trackers served by the VRDeviceDaemon: */
	deviceClient->lockState();
	numTrackers=deviceClient->getState().getNumTrackers();
	deviceClient->unlockState();
	
	/* Initialize the tracker state triple buffer: */
	for(int i=0;i<3;++i)
		trackerStates.getBuffer(i)=new TS[numTrackers];
	
	/* Activate the device client and start streaming: */
	deviceClient->activate();
	deviceClient->startStream(Misc::createFunctionCall(this,&TrackingTest::trackingCallback));
	
	/* Set the linear unit of navigational space to the tracking unit: */
	Vrui::getCoordinateManager()->setUnit(trackingUnit);
	}

TrackingTest::~TrackingTest(void)
	{
	/* Stop streaming and deactivate the device client: */
	deviceClient->stopStream();
	deviceClient->deactivate();
	
	/* Clean up: */
	for(int i=0;i<3;++i)
		delete[] trackerStates.getBuffer(i);
	delete deviceClient;
	}

void TrackingTest::frame(void)
	{
	/* Lock the most recently received tracker state: */
	trackerStates.lockNewValue();
	}

void TrackingTest::display(GLContextData& contextData) const
	{
	glPushAttrib(GL_ENABLE_BIT);
	
	float inch=float(trackingUnit.getInchFactor());
	
	/* Display the most recently received tracker state: */
	const TS* tss=trackerStates.getLockedValue();
	
	for(int trackerIndex=0;trackerIndex<numTrackers;++trackerIndex)
		{
		const TS& ts=tss[trackerIndex];
		glPushMatrix();
		
		/* Draw a world coordinate frame for this tracker: */
		glTranslate(ts.positionOrientation.getTranslation());
		drawFrame(inch,inch*0.015f);
		
		/* Draw a local coordinate frame for this tracker: */
		glRotate(ts.positionOrientation.getRotation());
		drawFrame(inch*0.75f,inch*0.02f);
		
		glPopMatrix();
		}
	
	glPopAttrib();
	}

VRUI_APPLICATION_RUN(TrackingTest)
