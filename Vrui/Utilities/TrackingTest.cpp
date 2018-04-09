/***********************************************************************
TrackingTest - Vrui application to visualize tracking data received
from a VRDeviceDaemon.
Copyright (c) 2016-2018 Oliver Kreylos

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
#include <Realtime/Time.h>
#include <Threads/TripleBuffer.h>
#include <Realtime/Time.h>
#include <Math/Math.h>
#include <Geometry/PCACalculator.h>
#include <GL/gl.h>
#include <GL/GLMaterialTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GL/GLModels.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/ToggleButton.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>
#include <Vrui/CoordinateManager.h>
#include <Vrui/ObjectSnapperTool.h>
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
	typedef Vrui::VRDeviceState::TimeStamp TimeStamp;
	
	struct TrackerState // Structure representing the state of a tracker and its noise history
		{
		/* Elements: */
		public:
		TS state; // The tracker's current instantaneous state reported by the device daemon
		std::vector<Point> samples; // List of tracker position samples received in the last second
		Point center; // Average tracker position
		Rotation rot; // Rotation of error ellipsoid
		Scalar axes[3]; // Semi-lengths of the error ellipsoid's three axes
		};
	
	struct TrackerSample // Structure representing a time-stamped tracker position sample
		{
		/* Elements: */
		public:
		TimeStamp time; // Driver time stamp of sample
		Point pos; // Position of sample
		};
	
	struct TrackerSampleBuffer // Structure for ring buffers of time-stamped tracker position samples
		{
		/* Elements: */
		public:
		unsigned int bufferSize; // Maximum number of elements in buffer
		TrackerSample* buffer; // Array of buffer elements
		unsigned int tail; // Index of oldest element in buffer
		unsigned int size; // Number of items currently in buffer
		TimeStamp lastTimeStamp; // Driver time stamp of most recent item entered into buffer
		
		/* Constructors and destructors: */
		TrackerSampleBuffer(void) // Creates a default-sized buffer
			:bufferSize(65536),
			 buffer(new TrackerSample[bufferSize]),
			 tail(0),size(0),
			 lastTimeStamp(0)
			{
			}
		~TrackerSampleBuffer(void) // Destroys a buffer
			{
			delete[] buffer;
			}
		
		/* Methods: */
		unsigned int getSize(void) const // Returns the number of samples in the buffer
			{
			return size;
			}
		void clear(void) // Clears the buffer
			{
			size=0;
			}
		void addSample(TimeStamp timeStamp,const TS& trackerState) // Enters a new tracking sample into the buffer, if it is new
			{
			/* Check if the sample is new: */
			if(lastTimeStamp!=timeStamp)
				{
				/* Store the time stamp and tracker position: */
				TrackerSample& back=buffer[(tail+size)%bufferSize];
				back.time=timeStamp;
				back.pos=trackerState.positionOrientation.getOrigin();
				
				/* Make the buffer longer, or, if it is full, ditch the oldest element: */
				if(size<bufferSize)
					++size;
				else
					tail=(tail+1)%bufferSize;
				
				/* Remember this sample: */
				lastTimeStamp=timeStamp;
				}
			}
		void removeOldSamples(TimeStamp maxAge) // Removes all tracking samples older than maxAge, relative to the given time
			{
			while(size>0&&lastTimeStamp-buffer[tail].time>maxAge)
				{
				tail=(tail+1)%bufferSize;
				--size;
				}
			}
		void copySamples(std::vector<Point>& points) const // Copies all tracking samples into the given list
			{
			/* Clear the points list and make room: */
			points.clear();
			points.reserve(size);
			
			/* Copy all samples: */
			for(unsigned int i=0;i<size;++i)
				points.push_back(buffer[(tail+i)%bufferSize].pos);
			}
		void doThePToTheCToTheA(TrackerState& ts) const; // Fits an error ellipsoid to tracking data in the buffer
		};
	
	/* Elements: */
	Vrui::VRDeviceClient* deviceClient; // Connection to the VRDeviceDaemon
	Geometry::LinearUnit trackingUnit; // Linear unit used by the tracking system
	Scalar frameSize; // Size of coordinate frame axes in tracking system units
	TimeStamp historyAge; // Amount of time samples remain in the sample history buffers
	int numTrackers; // Number of trackers served by the VRDeviceDaemon
	TrackerSampleBuffer* trackerSampleBuffers; // Array of tracker sample buffers
	Realtime::TimePointMonotonic nextUpdateTime; // Next time at which to update the main thread with new tracking data
	Threads::TripleBuffer<TrackerState*> trackerStates; // Triple buffer of arrays of tracking states
	bool drawTrackerFrames; // Flag whether to draw each tracker's coordinate frame
	bool drawWorldFrames; // Flag whether to draw a world-aligned coordinate frame alongside each tracker's frame
	bool drawVelocities; // Flag whether to draw each tracker's linear and angular velocities
	bool drawErrorEllipsoids; // Flag whether to draw each tracker's  error ellipsoid
	bool drawSampleHistory; // Flag whether to draw each tracker's sample history
	double printErrorTime; // Application time at which to print the size of the trackers' error ellipsoids
	bool clearHistory; // Flag to clear the history buffer when the next sample arrives
	GLMotif::PopupMenu* mainMenu; // The application's main menu
	
	/* Private methods: */
	void trackingCallback(Vrui::VRDeviceClient* client); // Called when new tracking data arrives
	void drawFrame(GLfloat arrowLength,GLfloat arrowRadius) const; // Draws a coordinate frame of the given size at the origin
	void snapRequest(Vrui::ObjectSnapperToolFactory::SnapRequest& snapRequest); // Callback to snap a virtual input device against application objects
	void mainMenuCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData,const int& itemIndex);
	GLMotif::PopupMenu* createMainMenu(void); // Creates the application's main menu
	
	/* Constructors and destructors: */
	public:
	TrackingTest(int& argc,char**& argv);
	virtual ~TrackingTest(void);
	
	/* Methods from Vrui::Application: */
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	virtual void eventCallback(EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData);
	virtual void resetNavigation(void);
	};

/**************************************************
Methods of class TrackingTest::TrackerSampleBuffer:
**************************************************/

void TrackingTest::TrackerSampleBuffer::doThePToTheCToTheA(TrackingTest::TrackerState& ts) const
	{
	/* Bail out if there aren't enough samples: */
	if(size<10) // Arbitrary cut-off
		{
		ts.center=Point::origin;
		ts.rot=Rotation::identity;
		ts.axes[2]=ts.axes[1]=ts.axes[0]=Scalar(0);
		return;
		}
	
	/* Add all samples to a PCA accumulator: */
	Geometry::PCACalculator<3> pca;
	for(unsigned int i=0;i<size;++i)
		pca.accumulatePoint(buffer[(tail+i)%bufferSize].pos);
	
	/* Extract error shape from the PCA accumulator: */
	ts.center=Point(pca.calcCentroid());
	pca.calcCovariance();
	double evs[3];
	pca.calcEigenvalues(evs);
	Vector x=Vector(pca.calcEigenvector(evs[0]));
	Vector y=Vector(pca.calcEigenvector(evs[1]));
	ts.rot=Rotation::fromBaseVectors(x,y);
	for(int i=0;i<3;++i)
		ts.axes[i]=Scalar(evs[i]);
	}

/*****************************
Methods of class TrackingTest:
*****************************/

void TrackingTest::trackingCallback(Vrui::VRDeviceClient* client)
	{
	/* Get the current time: */
	Realtime::TimePointMonotonic now;
	
	/* Extract new tracking data from the device client: */
	deviceClient->lockState();
	const Vrui::VRDeviceState& state=deviceClient->getState();
	bool ch=clearHistory;
	clearHistory=false;
	for(int i=0;i<numTrackers;++i)
		{
		/* Clear the buffer if requested: */
		if(ch)
			trackerSampleBuffers[i].clear();
		
		/* Enter the tracker's position into its buffer, if it is new: */
		trackerSampleBuffers[i].addSample(state.getTrackerTimeStamp(i),state.getTrackerState(i));
		}
	
	/* Check if it is time to update the main thread: */
	if(now>=nextUpdateTime)
		{
		/* Start a new slot in the tracker states triple buffer: */
		TrackerState* tss=trackerStates.startNewValue();
		
		/* Prepare a new state packet for each tracker: */
		for(int i=0;i<numTrackers;++i)
			{
			/* Copy the current tracker state: */
			tss[i].state=state.getTrackerState(i);
			
			/* Remove all old samples from the tracker's history buffer: */
			trackerSampleBuffers[i].removeOldSamples(historyAge);
			
			/* Copy all recent samples into the state packet: */
			trackerSampleBuffers[i].copySamples(tss[i].samples);
			
			/* Do some statistics: */
			trackerSampleBuffers[i].doThePToTheCToTheA(tss[i]);
			}
		
		/* Post the new tracking data and request a new Vrui frame: */
		trackerStates.postNewValue();
		Vrui::requestUpdate();
		
		/* Schedule the next update: */
		nextUpdateTime+=Realtime::TimeVector(1.0/60.0);
		}
	deviceClient->unlockState();
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

void TrackingTest::snapRequest(Vrui::ObjectSnapperToolFactory::SnapRequest& snapRequest)
	{
	if(snapRequest.rayBased)
		{
		/* Check all tracker states against the snap request's selection ray: */
		const TrackerState* tss=trackerStates.getLockedValue();
		for(int trackerIndex=0;trackerIndex<numTrackers;++trackerIndex)
			{
			const TS& ts=tss[trackerIndex].state;
			Vrui::Vector tpo=Vrui::Point(ts.positionOrientation.getOrigin())-snapRequest.snapRay.getOrigin();
			Vrui::Scalar lambda=tpo*snapRequest.snapRay.getDirection();
			if(lambda>=Vrui::Scalar(0)&&lambda<snapRequest.snapRayMax&&lambda>=snapRequest.snapRayCosine*Geometry::mag(tpo))
				{
				snapRequest.snapRayMax=lambda;
				snapRequest.snapped=true;
				snapRequest.snapResult=ts.positionOrientation;
				}
			}
		}
	else
		{
		/* Check all tracker states against the snap request's selection sphere: */
		const TrackerState* tss=trackerStates.getLockedValue();
		for(int trackerIndex=0;trackerIndex<numTrackers;++trackerIndex)
			{
			const TS& ts=tss[trackerIndex].state;
			Vrui::Scalar d2=Geometry::sqrDist(Vrui::Point(ts.positionOrientation.getOrigin()),snapRequest.snapPosition);
			if(d2<Math::sqr(snapRequest.snapRadius))
				{
				snapRequest.snapRadius=Math::sqrt(d2);
				snapRequest.snapped=true;
				snapRequest.snapResult=ts.positionOrientation;
				}
			}
		}
	}

void TrackingTest::mainMenuCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData,const int& itemIndex)
	{
	switch(itemIndex)
		{
		case 0:
			drawTrackerFrames=cbData->set;
			break;
		
		case 1:
			drawWorldFrames=cbData->set;
			break;
		
		case 2:
			drawVelocities=cbData->set;
			break;
		
		case 3:
			drawErrorEllipsoids=cbData->set;
			break;
		
		case 4:
			drawSampleHistory=cbData->set;
			break;
		}
	}

GLMotif::PopupMenu* TrackingTest::createMainMenu(void)
	{
	/* Create the main menu shell: */
	GLMotif::PopupMenu* mainMenu=new GLMotif::PopupMenu("MainMenu",Vrui::getWidgetManager());
	mainMenu->setTitle("Tracking Test");
	
	GLMotif::ToggleButton* drawTrackerFramesToggle=new GLMotif::ToggleButton("DrawTrackerFramesToggle",mainMenu,"Draw Tracker Frames");
	drawTrackerFramesToggle->setToggle(drawTrackerFrames);
	drawTrackerFramesToggle->getValueChangedCallbacks().add(this,&TrackingTest::mainMenuCallback,0);
	
	GLMotif::ToggleButton* drawWorldAxesToggle=new GLMotif::ToggleButton("DrawWorldAxesToggle",mainMenu,"Draw World Axes");
	drawWorldAxesToggle->setToggle(drawWorldFrames);
	drawWorldAxesToggle->getValueChangedCallbacks().add(this,&TrackingTest::mainMenuCallback,1);
	
	GLMotif::ToggleButton* drawVelocitiesToggle=new GLMotif::ToggleButton("DrawVelocitiesToggle",mainMenu,"Draw Velocities");
	drawVelocitiesToggle->setToggle(drawVelocities);
	drawVelocitiesToggle->getValueChangedCallbacks().add(this,&TrackingTest::mainMenuCallback,2);
	
	GLMotif::ToggleButton* drawErrorEllipsoidsToggle=new GLMotif::ToggleButton("DrawErrorEllipsoidsToggle",mainMenu,"Draw Error Ellipsoids");
	drawErrorEllipsoidsToggle->setToggle(drawErrorEllipsoids);
	drawErrorEllipsoidsToggle->getValueChangedCallbacks().add(this,&TrackingTest::mainMenuCallback,3);
	
	GLMotif::ToggleButton* drawSampleHistoryToggle=new GLMotif::ToggleButton("DrawSampleHistoryToggle",mainMenu,"Draw Sample History");
	drawSampleHistoryToggle->setToggle(drawSampleHistory);
	drawSampleHistoryToggle->getValueChangedCallbacks().add(this,&TrackingTest::mainMenuCallback,4);
	
	mainMenu->manageMenu();
	return mainMenu;
	}

TrackingTest::TrackingTest(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 deviceClient(0),
	 trackingUnit(Geometry::LinearUnit::INCH,1.0),frameSize(1),
	 historyAge(1000000),
	 numTrackers(0),trackerSampleBuffers(0),
	 drawTrackerFrames(true),drawWorldFrames(true),drawVelocities(true),
	 drawErrorEllipsoids(true),drawSampleHistory(true),
	 printErrorTime(5.0),
	 clearHistory(false),
	 mainMenu(0)
	{
	/* Parse command line: */
	char defaultServerName[]="localhost:8555";
	char* serverName=defaultServerName;
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
				frameSize=Scalar(trackingUnit.getInchFactor());
				}
			else if(strcasecmp(argv[i]+1,"frameSize")==0)
				{
				++i;
				frameSize=Scalar(atof(argv[i]));
				}
			else if(strcasecmp(argv[i]+1,"historyAge")==0)
				{
				++i;
				historyAge=TimeStamp(Math::floor(atof(argv[i])*1.0e6+0.5));
				}
			}
		else
			serverName=argv[i];
		}
	
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
	
	/* Initialize the tracker sample buffers: */
	trackerSampleBuffers=new TrackerSampleBuffer[numTrackers];
	
	/* Initialize the tracker state triple buffer: */
	for(int i=0;i<3;++i)
		trackerStates.getBuffer(i)=new TrackerState[numTrackers];
	
	/* Activate the device client and start streaming: */
	deviceClient->activate();
	deviceClient->startStream(Misc::createFunctionCall(this,&TrackingTest::trackingCallback));
	
	/* Initialize the update timer: */
	nextUpdateTime=Realtime::TimePointMonotonic()+Realtime::TimeVector(1,0); // Wait one second to build up tracking history
	
	/* Create the main menu: */
	mainMenu=createMainMenu();
	Vrui::setMainMenu(mainMenu);
	
	/* Set the linear unit of navigational space to the tracking unit: */
	Vrui::getCoordinateManager()->setUnit(trackingUnit);
	
	/* Register with the object snapper tool class: */
	Vrui::ObjectSnapperTool::addSnapCallback(Misc::createFunctionCall(this,&TrackingTest::snapRequest));
	
	/* Create a tool class to clear the history buffer: */
	addEventTool("Clear History",0,0);
	}

TrackingTest::~TrackingTest(void)
	{
	/* Stop streaming and deactivate the device client: */
	deviceClient->stopStream();
	deviceClient->deactivate();
	
	/* Clean up: */
	delete mainMenu;
	for(int i=0;i<3;++i)
		delete[] trackerStates.getBuffer(i);
	delete[] trackerSampleBuffers;
	delete deviceClient;
	}

void TrackingTest::frame(void)
	{
	/* Lock the most recently received tracker state: */
	trackerStates.lockNewValue();
	
	if(Vrui::getApplicationTime()>=printErrorTime)
		{
		/* Print the size of each tracker's error ellipsoids: */
		const TrackerState* tss=trackerStates.getLockedValue();
		for(int trackerIndex=0;trackerIndex<numTrackers;++trackerIndex)
			{
			std::cout<<"Tracker "<<trackerIndex<<":";
			for(int j=0;j<3;++j)
				std::cout<<' '<<Math::sqrt(tss[trackerIndex].axes[j])*3.0;
			std::cout<<std::endl;
			}
		
		/* Print again when the buffer has renewed: */
		printErrorTime+=double(historyAge)*1.0e-6;
		}
	}

void TrackingTest::display(GLContextData& contextData) const
	{
	glPushAttrib(GL_ENABLE_BIT|GL_POINT_BIT);
	glPointSize(3.0f);
	
	/* Display the most recently received tracker state: */
	const TrackerState* tss=trackerStates.getLockedValue();
	
	/* Draw all trackers' coordinate frames: */
	for(int trackerIndex=0;trackerIndex<numTrackers;++trackerIndex)
		{
		glPushMatrix();
		const TS& ts=tss[trackerIndex].state;
		glTranslate(ts.positionOrientation.getTranslation());
		
		if(drawWorldFrames)
			{
			/* Draw a world coordinate frame for this tracker: */
			drawFrame(frameSize,frameSize*0.015f);
			}
		
		if(drawTrackerFrames)
			{
			/* Draw a local coordinate frame for this tracker: */
			glPushMatrix();
			glRotate(ts.positionOrientation.getRotation());
			drawFrame(frameSize*0.75f,frameSize*0.02f);
			glPopMatrix();
			}
		
		if(drawVelocities)
			{
			/* Draw linear velocity: */
			{
			glPushMatrix();
			glMaterialAmbientAndDiffuse(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(1.0f,1.0f,0.0f));
			glRotate(Rotation::rotateFromTo(Vector(0,0,1),ts.linearVelocity));
			float arrowLength=Geometry::mag(ts.linearVelocity)*frameSize*10.0f;
			glTranslatef(0.0f,0.0f,arrowLength*0.5f);
			glDrawArrow(frameSize*0.01f,frameSize*0.015f,frameSize*0.03f,arrowLength,16);
			glPopMatrix();
			}
			
			/* Draw angular velocity: */
			{
			glPushMatrix();
			glMaterialAmbientAndDiffuse(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(0.0f,1.0f,1.0f));
			glRotate(Rotation::rotateFromTo(Vector(0,0,1),ts.angularVelocity));
			float arrowLength=Geometry::mag(ts.angularVelocity)*frameSize*1.0f;
			glTranslatef(0.0f,0.0f,arrowLength*0.5f);
			glDrawArrow(frameSize*0.01f,frameSize*0.015f,frameSize*0.03f,arrowLength,16);
			glPopMatrix();
			}
			}
		
		glPopMatrix();
		}
	
	if(drawErrorEllipsoids)
		{
		/* Draw all trackers' error ellipsoids: */
		for(int trackerIndex=0;trackerIndex<numTrackers;++trackerIndex)
			{
			glPushMatrix();
			glTranslate(tss[trackerIndex].center-Point::origin);
			glRotate(tss[trackerIndex].rot);
			glScaled(Math::sqrt(tss[trackerIndex].axes[0])*3.0,Math::sqrt(tss[trackerIndex].axes[1])*3.0,Math::sqrt(tss[trackerIndex].axes[2])*3.0);
			
			glMaterialAmbientAndDiffuse(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(0.7f,0.4f,0.7f));
			glDrawSphereIcosahedron(1.0f,5);
			
			glPopMatrix();
			}
		}
	
	if(drawSampleHistory)
		{
		/* Draw all trackers' tracking history: */
		glDisable(GL_LIGHTING);
		glEnableClientState(GL_VERTEX_ARRAY);
		glColor3f(1.0f,1.0f,1.0f);
		for(int trackerIndex=0;trackerIndex<numTrackers;++trackerIndex)
			{
			glVertexPointer(0,&tss[trackerIndex].samples.front());
			glDrawArrays(GL_POINTS,0,tss[trackerIndex].samples.size());
			}
		glDisableClientState(GL_VERTEX_ARRAY);
		}
	
	glPopAttrib();
	}


void TrackingTest::eventCallback(Vrui::Application::EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState)
		{
		/* Request clearing the history buffer: */
		clearHistory=true;
		
		/* Print error ellipsoid sizes when the buffer is full: */
		printErrorTime=Vrui::getApplicationTime()+double(historyAge)*1.0e-6*1.1;
		}
	}

void TrackingTest::resetNavigation(void)
	{
	/* Return to physical space: */
	Vrui::setNavigationTransformation(Vrui::NavTransform::identity);
	}

VRUI_APPLICATION_RUN(TrackingTest)
