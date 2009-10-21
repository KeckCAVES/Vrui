/***********************************************************************
MeasureEnvironment - Utility for guided surveys of a single-screen
VR environment using a Total Station.
Copyright (c) 2009 Oliver Kreylos

This file is part of the Vrui calibration utility package.

The Vrui calibration utility package is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Vrui calibration utility package is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui calibration utility package; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <stdlib.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <Misc/ThrowStdErr.h>
#include <Misc/FileCharacterSource.h>
#include <Misc/TokenSource.h>
#include <Threads/Thread.h>
#include <Threads/Mutex.h>
#include <Math/Constants.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Ray.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/Sphere.h>
#include <GL/gl.h>
#include <GL/GLGeometryWrappers.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/RadioBox.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Tools/TransformTool.h>
#include <Vrui/Tools/GenericToolFactory.h>
#include <Vrui/Application.h>
#include <Vrui/Vrui.h>

#include "TotalStation.h"

class MeasureEnvironment:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	typedef double Scalar;
	typedef Geometry::Point<Scalar,3> Point;
	typedef Geometry::Ray<Scalar,3> Ray;
	typedef std::vector<Point> PointList;
	typedef size_t PickResult;
	
	class PointSnapperTool; // Forward declaration
	typedef Vrui::GenericToolFactory<PointSnapperTool> PointSnapperToolFactory; // Tool class uses the generic factory class
	
	class PointSnapperTool:public Vrui::TransformTool,public Vrui::Application::Tool<MeasureEnvironment> // Tool to snap to measured points
		{
		friend class Vrui::GenericToolFactory<PointSnapperTool>;
		
		/* Elements: */
		private:
		static PointSnapperToolFactory* factory;
		
		/* Constructors and destructors: */
		public:
		PointSnapperTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
			:Vrui::TransformTool(factory,inputAssignment)
			{
			}
		
		/* Methods from Vrui::Tool: */
		virtual void initialize(void);
		virtual const Vrui::ToolFactory* getFactory(void) const
			{
			return factory;
			}
		virtual void frame(void);
		};
	
	/* Elements: */
	TotalStation* totalStation;
	Scalar initialPrismOffset;
	Threads::Thread pointCollectorThread;
	mutable Threads::Mutex measuringMutex;
	int measuringMode;
	TotalStation::Scalar ballRadius;
	PointList floorPoints;
	PointList screenPoints;
	PointList ballPoints;
	
	GLMotif::PopupMenu* mainMenu; // The program's main menu
	
	/* Private methods: */
	GLMotif::PopupMenu* createMainMenu(void); // Creates the program's main menu
	void* pointCollectorThreadMethod(void);
	
	/* Constructors and destructors: */
	public:
	MeasureEnvironment(int& argc,char**& argv,char**& appDefaults);
	~MeasureEnvironment(void);
	
	/* Methods: */
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	
	/* New methods: */
	PickResult pickPoint(const Point& point,Scalar pointSize) const;
	PickResult pickPoint(const Ray& ray,Scalar pointSize) const;
	Point snapToPoint(const Point& point,const PickResult& pickResult) const;
	void changeMeasuringModeCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData);
	};

/*************************************************************
Static elements of class MeasureEnvironment::PointSnapperTool:
*************************************************************/

MeasureEnvironment::PointSnapperToolFactory* MeasureEnvironment::PointSnapperTool::factory=0;

/*****************************************************
Methods of class MeasureEnvironment::PointSnapperTool:
*****************************************************/

void MeasureEnvironment::PointSnapperTool::initialize(void)
	{
	/* Initialize the base tool: */
	TransformTool::initialize();
	
	/* Disable the transformed device's glyph: */
	Vrui::getInputGraphManager()->getInputDeviceGlyph(transformedDevice).disable();
	}

void MeasureEnvironment::PointSnapperTool::frame(void)
	{
	/* Get pointer to input device: */
	Vrui::InputDevice* iDevice=input.getDevice(0);
	
	if(transformEnabled)
		{
		/* Pick a point: */
		Vrui::Scalar pointSize=Vrui::getUiSize()*Vrui::getInverseNavigationTransformation().getScaling();
		Vrui::NavTrackerState transform=Vrui::getDeviceTransformation(iDevice);
		Point pos=transform.getOrigin();
		PickResult pr;
		if(iDevice->isRayDevice())
			pr=application->pickPoint(Ray(pos,transform.transform(iDevice->getDeviceRayDirection())),pointSize);
		else
			pr=application->pickPoint(pos,pointSize);
		
		/* Move the device's origin to the picked point: */
		pos=application->snapToPoint(pos,pr);
		
		/* Set the transformed device's position to the intersection point: */
		Vrui::TrackerState ts=Vrui::TrackerState::translateFromOriginTo(Vrui::getNavigationTransformation().transform(pos));
		transformedDevice->setTransformation(ts);
		}
	else
		transformedDevice->setTransformation(iDevice->getTransformation());
	transformedDevice->setDeviceRayDirection(iDevice->getDeviceRayDirection());
	}

/***********************************
Methods of class MeasureEnvironment:
***********************************/

GLMotif::PopupMenu* MeasureEnvironment::createMainMenu(void)
	{
	/* Create a popup shell to hold the main menu: */
	GLMotif::PopupMenu* mainMenuPopup=new GLMotif::PopupMenu("MainMenuPopup",Vrui::getWidgetManager());
	mainMenuPopup->setTitle("Survey Pal");
	
	GLMotif::RadioBox* measuringModes=new GLMotif::RadioBox("MeasuringModes",mainMenuPopup,false);
	measuringModes->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
	
	measuringModes->addToggle("Measure Floor");
	measuringModes->addToggle("Measure Screen");
	measuringModes->addToggle("Measure Balls");
	
	measuringModes->setSelectedToggle(measuringMode);
	measuringModes->getValueChangedCallbacks().add(this,&MeasureEnvironment::changeMeasuringModeCallback);
	
	/* Finish building the main menu: */
	measuringModes->manageChild();
	
	return mainMenuPopup;
	}

void* MeasureEnvironment::pointCollectorThreadMethod(void)
	{
	Threads::Thread::setCancelState(Threads::Thread::CANCEL_ENABLE);
	Threads::Thread::setCancelType(Threads::Thread::CANCEL_ASYNCHRONOUS);
	
	while(true)
		{
		/* Read the next measurement point from the Total Station: */
		Point p=totalStation->readNextMeasurement();
		
		/* Store the point: */
		{
		Threads::Mutex::Lock measuringLock(measuringMutex);
		switch(measuringMode)
			{
			case 0:
				floorPoints.push_back(p);
				break;
			
			case 1:
				screenPoints.push_back(p);
				break;
			
			case 2:
				ballPoints.push_back(p);
				break;
			}
		}
		
		Vrui::requestUpdate();
		}
	
	return 0;
	}

MeasureEnvironment::MeasureEnvironment(int& argc,char**& argv,char**& appDefaults)
	:Vrui::Application(argc,argv,appDefaults),
	 totalStation(0),
	 initialPrismOffset(0.0),
	 measuringMode(0),
	 ballRadius(TotalStation::Scalar(25.4/4.0)),
	 mainMenu(0)
	{
	/* Register the point snapper tool class with the Vrui tool manager: */
	PointSnapperToolFactory* pointSnapperToolFactory=new PointSnapperToolFactory("PointSnapperTool","Snap To Points",0,*Vrui::getToolManager());
	pointSnapperToolFactory->setNumDevices(1);
	pointSnapperToolFactory->setNumButtons(0,1);
	Vrui::getToolManager()->addClass(pointSnapperToolFactory,Vrui::ToolManager::defaultToolFactoryDestructor);
	
	/* Parse the command line: */
	const char* totalStationDeviceName=0;
	int totalStationBaudRate=19200;
	const char* measurementFileName=0;
	TotalStation::Scalar totalStationUnitScale=TotalStation::Scalar(1);
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"baudRate")==0)
				{
				++i;
				if(i<argc)
					totalStationBaudRate=atoi(argv[i]);
				else
					std::cerr<<"MeasureEnvironment: Ignoring dangling command line switch "<<argv[i]<<std::endl;
				}
			else if(strcasecmp(argv[i]+1,"unitScale")==0)
				{
				++i;
				if(i<argc)
					totalStationUnitScale=TotalStation::Scalar(atof(argv[i]));
				else
					std::cerr<<"MeasureEnvironment: Ignoring dangling command line switch "<<argv[i]<<std::endl;
				}
			else if(strcasecmp(argv[i]+1,"ballRadius")==0)
				{
				++i;
				if(i<argc)
					ballRadius=TotalStation::Scalar(atof(argv[i]));
				else
					std::cerr<<"MeasureEnvironment: Ignoring dangling command line switch "<<argv[i]<<std::endl;
				}
			else
				std::cerr<<"MeasureEnvironment: Unrecognized command line switch "<<argv[i]<<std::endl;
			}
		else if(totalStationDeviceName==0)
			totalStationDeviceName=argv[i];
		else if(measurementFileName==0)
			measurementFileName=argv[i];
		else
			std::cerr<<"MeasureEnvironment: Ignoring command line argument "<<argv[i]<<std::endl;
		}
	
	if(totalStationDeviceName==0)
		Misc::throwStdErr("MeasureEnvironment::MeasureEnvironment: No serial device name provided");
	
	/* Import a measurement file if one is given: */
	if(measurementFileName!=0)
		{
		/* Open the input file: */
		Misc::FileCharacterSource pointFile(measurementFileName);
		Misc::TokenSource tok(pointFile);
		tok.setPunctuation(",\n");
		tok.setQuotes("\"");
		tok.skipWs();
		
		while(!tok.eof())
			{
			Point p;
			for(int i=0;i<3;++i)
				{
				if(i>0)
					{
					tok.readNextToken();
					if(!tok.isToken(","))
						Misc::throwStdErr("MeasureEnvironment::MeasureEnvironment: Format error in input file %s",argv[3]);
					}
				p[i]=Scalar(atof(tok.readNextToken()));
				}
			
			tok.readNextToken();
			if(!tok.isToken(","))
				Misc::throwStdErr("MeasureEnvironment::MeasureEnvironment: Format error in input file %s",argv[3]);
			
			tok.readNextToken();
			if(tok.isCaseToken("FLOOR"))
				floorPoints.push_back(p);
			else if(tok.isCaseToken("SCREEN"))
				screenPoints.push_back(p);
			else if(tok.isCaseToken("BALLS"))
				ballPoints.push_back(p);
			else
				Misc::throwStdErr("MeasureEnvironment::MeasureEnvironment: Unknown point tag \"%s\" in input file %s",tok.getToken(),argv[3]);
			
			tok.readNextToken();
			if(!tok.isToken("\n"))
				Misc::throwStdErr("MeasureEnvironment::MeasureEnvironment: Format error in input file %s",argv[3]);
			}
		}
	
	/* Connect to the Total Station: */
	totalStation=new TotalStation(totalStationDeviceName,totalStationBaudRate);
	totalStation->setUnitScale(totalStationUnitScale);
	
	/* Store the initial prism offset: */
	initialPrismOffset=totalStation->getPrismOffset();
	
	/* Set the prism offset to zero for floor or screen measurements: */
	totalStation->setPrismOffset(TotalStation::Scalar(0));
	
	/* Start the point recording thread: */
	totalStation->startRecording();
	pointCollectorThread.start(this,&MeasureEnvironment::pointCollectorThreadMethod);
	
	/* Create the main menu: */
	mainMenu=createMainMenu();
	Vrui::setMainMenu(mainMenu);
	}

MeasureEnvironment::~MeasureEnvironment(void)
	{
	delete mainMenu;
	
	{
	Threads::Mutex::Lock measuringLock(measuringMutex);
	pointCollectorThread.cancel();
	pointCollectorThread.join();
	}
	
	if(totalStation!=0)
		{
		/* Stop recording points: */
		totalStation->stopRecording();
		
		/* Reset the Total Station's prism offset to its initial value: */
		totalStation->setPrismOffset(initialPrismOffset);
		}
	delete totalStation;
	
	/* Save all measured points: */
	std::ofstream pointFile("MeasuredPoint.csv");
	pointFile.setf(std::ios::fixed);
	pointFile<<std::setprecision(6);
	for(PointList::const_iterator fpIt=floorPoints.begin();fpIt!=floorPoints.end();++fpIt)
		{
		pointFile<<std::setw(12)<<(*fpIt)[0]<<","<<std::setw(12)<<(*fpIt)[1]<<","<<std::setw(12)<<(*fpIt)[2];
		pointFile<<",\"FLOOR\""<<std::endl;
		}
	for(PointList::const_iterator spIt=screenPoints.begin();spIt!=screenPoints.end();++spIt)
		{
		pointFile<<std::setw(12)<<(*spIt)[0]<<","<<std::setw(12)<<(*spIt)[1]<<","<<std::setw(12)<<(*spIt)[2];
		pointFile<<",\"SCREEN\""<<std::endl;
		}
	for(PointList::const_iterator bpIt=ballPoints.begin();bpIt!=ballPoints.end();++bpIt)
		{
		pointFile<<std::setw(12)<<(*bpIt)[0]<<","<<std::setw(12)<<(*bpIt)[1]<<","<<std::setw(12)<<(*bpIt)[2];
		pointFile<<",\"BALLS\""<<std::endl;
		}
	}

void MeasureEnvironment::frame(void)
	{
	}

void MeasureEnvironment::display(GLContextData& contextData) const
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT|GL_POINT_BIT);
	glDisable(GL_LIGHTING);
	glPointSize(3.0f);
	
	{
	Threads::Mutex::Lock measuringLock(measuringMutex);
	
	/* Draw all already collected points: */
	glBegin(GL_POINTS);
	glColor3f(1.0f,0.0f,0.0f);
	for(PointList::const_iterator fpIt=floorPoints.begin();fpIt!=floorPoints.end();++fpIt)
		glVertex(*fpIt);
	glColor3f(0.0f,1.0f,0.0f);
	for(PointList::const_iterator spIt=screenPoints.begin();spIt!=screenPoints.end();++spIt)
		glVertex(*spIt);
	glColor3f(1.0f,0.0f,1.0f);
	for(PointList::const_iterator bpIt=ballPoints.begin();bpIt!=ballPoints.end();++bpIt)
		glVertex(*bpIt);
	glEnd();
	
	}
	
	/* Reset OpenGL state: */
	glPopAttrib();
	}

MeasureEnvironment::PickResult MeasureEnvironment::pickPoint(const MeasureEnvironment::Point& point,MeasureEnvironment::Scalar pointSize) const
	{
	PickResult bestPointIndex=~PickResult(0);
	Scalar minSqrDist=Math::sqr(pointSize);
	
	/* Compare the query point against all points: */
	size_t indexBase=0;
	
	/* Compare against floor points: */
	for(size_t i=0;i<floorPoints.size();++i)
		{
		Scalar sqrDist=Geometry::sqrDist(point,floorPoints[i]);
		if(minSqrDist>sqrDist)
			{
			bestPointIndex=i+indexBase;
			minSqrDist=sqrDist;
			}
		}
	indexBase+=floorPoints.size();
	
	/* Compare against screen points: */
	for(size_t i=0;i<screenPoints.size();++i)
		{
		Scalar sqrDist=Geometry::sqrDist(point,screenPoints[i]);
		if(minSqrDist>sqrDist)
			{
			bestPointIndex=i+indexBase;
			minSqrDist=sqrDist;
			}
		}
	indexBase+=screenPoints.size();
	
	/* Compare against ball points: */
	for(size_t i=0;i<ballPoints.size();++i)
		{
		Scalar sqrDist=Geometry::sqrDist(point,ballPoints[i]);
		if(minSqrDist>sqrDist)
			{
			bestPointIndex=i+indexBase;
			minSqrDist=sqrDist;
			}
		}
	indexBase+=ballPoints.size();
	
	return bestPointIndex;
	}

MeasureEnvironment::PickResult MeasureEnvironment::pickPoint(const MeasureEnvironment::Ray& ray,MeasureEnvironment::Scalar pointSize) const
	{
	PickResult bestPointIndex=~PickResult(0);
	Scalar minLambda=Math::Constants<Scalar>::max;
	
	/* Compare the query ray against all points: */
	size_t indexBase=0;
	
	/* Compare against floor points: */
	for(size_t i=0;i<floorPoints.size();++i)
		{
		Geometry::Sphere<Scalar,3> sphere(floorPoints[i],pointSize);
		Geometry::Sphere<Scalar,3>::HitResult hr=sphere.intersectRay(ray);
		if(hr.isValid()&&minLambda>hr.getParameter())
			{
			bestPointIndex=i+indexBase;
			minLambda=hr.getParameter();
			}
		}
	indexBase+=floorPoints.size();
	
	/* Compare against screen points: */
	for(size_t i=0;i<screenPoints.size();++i)
		{
		Geometry::Sphere<Scalar,3> sphere(screenPoints[i],pointSize);
		Geometry::Sphere<Scalar,3>::HitResult hr=sphere.intersectRay(ray);
		if(hr.isValid()&&minLambda>hr.getParameter())
			{
			bestPointIndex=indexBase;
			minLambda=hr.getParameter();
			}
		}
	indexBase+=screenPoints.size();
	
	/* Compare against ball points: */
	for(size_t i=0;i<ballPoints.size();++i)
		{
		Geometry::Sphere<Scalar,3> sphere(ballPoints[i],pointSize);
		Geometry::Sphere<Scalar,3>::HitResult hr=sphere.intersectRay(ray);
		if(hr.isValid()&&minLambda>hr.getParameter())
			{
			bestPointIndex=i+indexBase;
			minLambda=hr.getParameter();
			}
		}
	indexBase+=ballPoints.size();
	
	return bestPointIndex;
	}

MeasureEnvironment::Point MeasureEnvironment::snapToPoint(const MeasureEnvironment::Point& point,const MeasureEnvironment::PickResult& pickResult) const
	{
	PickResult pr=pickResult;
	
	/* Check if the picked point is a floor point: */
	if(pr<floorPoints.size())
		return floorPoints[pickResult];
	pr-=floorPoints.size();
	
	/* Check if the picked point is a screen point: */
	if(pickResult<screenPoints.size())
		return screenPoints[pickResult];
	pr-=screenPoints.size();
	
	/* Check if the picked point is a ball point: */
	if(pickResult<ballPoints.size())
		return ballPoints[pickResult];
	pr-=ballPoints.size();
	
	/* Otherwise return the unchanged position: */
	return point;
	}

void MeasureEnvironment::changeMeasuringModeCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData)
	{
	Threads::Mutex::Lock measuringLock(measuringMutex);
	
	/* Set the new measuring mode: */
	int newMeasuringMode=cbData->radioBox->getToggleIndex(cbData->newSelectedToggle);
	
	if(newMeasuringMode==2&&measuringMode!=2)
		{
		/* Temporarily shut down the point collector thread: */
		pointCollectorThread.cancel();
		pointCollectorThread.join();
		
		/* Set the Total Station's prism offset to measure balls: */
		totalStation->setPrismOffset(ballRadius);
		
		/* Restart the point collector thread: */
		pointCollectorThread.start(this,&MeasureEnvironment::pointCollectorThreadMethod);
		}
	if(newMeasuringMode!=2&&measuringMode==2)
		{
		/* Temporarily shut down the point collector thread: */
		pointCollectorThread.cancel();
		pointCollectorThread.join();
		
		/* Set the Total Station's prism offset to measure points on the floor or screen: */
		totalStation->setPrismOffset(TotalStation::Scalar(0));
		
		/* Restart the point collector thread: */
		pointCollectorThread.start(this,&MeasureEnvironment::pointCollectorThreadMethod);
		}
	
	measuringMode=newMeasuringMode;
	
	if(measuringMode==1)
		screenPoints.clear();
	}

int main(int argc,char* argv[])
	{
	char** appDefaults=0;
	MeasureEnvironment app(argc,argv,appDefaults);
	app.run();
	
	return 0;
	}
