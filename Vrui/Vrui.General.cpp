/***********************************************************************
Environment-independent part of Vrui virtual reality development
toolkit.
Copyright (c) 2000-2008 Oliver Kreylos

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

#define DELAY_NAVIGATIONTRANSFORMATION 1
#define RENDERFRAMETIMES 0
#define SAVESHAREDVRUISTATE 0

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/File.h>
#include <Misc/CreateNumberedFileName.h>
#include <Misc/ValueCoder.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/TimerEventScheduler.h>
#include <Comm/MulticastPipeMultiplexer.h>
#include <Comm/MulticastPipe.h>
#include <Math/Constants.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLLightModelTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLFont.h>
#include <GL/GLValueCoders.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/Event.h>
#include <GLMotif/Widget.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/Separator.h>
#include <GLMotif/Container.h>
#include <GLMotif/Popup.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/Menu.h>
#include <GLMotif/SubMenu.h>
#include <GLMotif/Button.h>
#include <GLMotif/CascadeButton.h>
#ifdef VRUI_USE_OPENAL
#ifdef __DARWIN__
#include <OpenAL/al.h>
#else
#include <AL/al.h>
#endif
#endif
#include <Vrui/TransparentObject.h>
#include <Vrui/VirtualInputDevice.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/InputDeviceAdapter.h>
#include <Vrui/InputDeviceAdapterMouse.h>
#include <Vrui/InputDeviceAdapterDeviceDaemon.h>
#include <Vrui/MultipipeDispatcher.h>
#include <Vrui/LightsourceManager.h>
#include <Vrui/ClipPlaneManager.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRScreen.h>
#include <Vrui/Listener.h>
#include <Vrui/MutexMenu.h>
#include <Vrui/CoordinateManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/ToolKillZone.h>
#include <Vrui/VisletManager.h>
#include <Vrui/InputDeviceDataSaver.h>

#include <Vrui/Vrui.Internal.h>

namespace Misc {

/***********************************************************************
Helper class to read screen protection values from a configuration file:
***********************************************************************/

template <>
class ValueCoder<Vrui::VruiState::ScreenProtector>
	{
	/* Methods: */
	public:
	static std::string encode(const Vrui::VruiState::ScreenProtector& value)
		{
		std::string result="";
		result+="( ";
		result+=ValueCoder<std::string>::encode(value.inputDevice->getDeviceName());
		result+=", ";
		result+=ValueCoder<Vrui::Point>::encode(value.center);
		result+=", ";
		result+=ValueCoder<Vrui::Scalar>::encode(value.radius);
		result+=" )";
		return result;
		}
	static Vrui::VruiState::ScreenProtector decode(const char* start,const char* end,const char** decodeEnd =0)
		{
		Vrui::VruiState::ScreenProtector result;
		const char* cPtr=start;
		try
			{
			if(*cPtr!='(')
				throw DecodingError("Missing opening parenthesis");
			++cPtr;
			cPtr=skipWhitespace(cPtr,end);
			std::string inputDeviceName=ValueCoder<std::string>::decode(cPtr,end,&cPtr);
			result.inputDevice=Vrui::findInputDevice(inputDeviceName.c_str());
			if(result.inputDevice==0)
				Misc::throwStdErr("Input device \"%s\" not found",inputDeviceName.c_str());
			cPtr=skipSeparator(',',cPtr,end);
			result.center=ValueCoder<Vrui::Point>::decode(cPtr,end,&cPtr);
			cPtr=skipSeparator(',',cPtr,end);
			result.radius=ValueCoder<Vrui::Scalar>::decode(cPtr,end,&cPtr);
			cPtr=skipWhitespace(cPtr,end);
			if(*cPtr!=')')
				throw DecodingError("Missing closing parenthesis");
			++cPtr;
			}
		catch(std::runtime_error err)
			{
			throw DecodingError(std::string("Unable to convert \"")+std::string(start,end)+std::string("\" to ScreenProtector due to ")+err.what());
			}
		
		/* Return result: */
		if(decodeEnd!=0)
			*decodeEnd=cPtr;
		return result;
		}
	};

}

namespace Vrui {

/************
Global state:
************/

VruiState* vruiState=0;

#if RENDERFRAMETIMES
const int numFrameTimes=800;
double frameTimes[numFrameTimes];
int frameTimeIndex=-1;
#endif
#if SAVESHAREDVRUISTATE
Misc::File* vruiSharedStateFile=0;
#endif

/**********************
Private Vrui functions:
**********************/

GLMotif::Popup* VruiState::buildViewMenu(void)
	{
	GLMotif::Popup* viewMenuPopup=new GLMotif::Popup("ViewMenuPopup",getWidgetManager());
	
	GLMotif::SubMenu* viewMenu=new GLMotif::SubMenu("View",viewMenuPopup,false);
	
	GLMotif::Button* loadViewButton=new GLMotif::Button("LoadViewButton",viewMenu,"Load View");
	loadViewButton->getSelectCallbacks().add(this,&VruiState::loadViewCallback);
	
	GLMotif::Button* saveViewButton=new GLMotif::Button("SaveViewButton",viewMenu,"Save View");
	saveViewButton->getSelectCallbacks().add(this,&VruiState::saveViewCallback);
	
	GLMotif::Button* restoreViewButton=new GLMotif::Button("RestoreViewButton",viewMenu,"Restore View");
	restoreViewButton->getSelectCallbacks().add(this,&VruiState::restoreViewCallback);
	
	viewMenu->manageChild();
	
	return viewMenuPopup;
	}

void VruiState::buildSystemMenu(GLMotif::Container* parent)
	{
	/* Create the view submenu: */
	GLMotif::CascadeButton* viewMenuCascade=new GLMotif::CascadeButton("ViewMenuCascade",parent,"View");
	viewMenuCascade->setPopup(buildViewMenu());
	
	/* Create buttons to create or destroy virtual input device: */
	GLMotif::Button* createInputDeviceButton=new GLMotif::Button("CreateInputDeviceButton",parent,"Create Input Device");
	createInputDeviceButton->getSelectCallbacks().add(this,&VruiState::createInputDeviceCallback);
	
	GLMotif::Button* destroyInputDeviceButton=new GLMotif::Button("DestroyInputDeviceButton",parent,"Destroy Input Device");
	destroyInputDeviceButton->getSelectCallbacks().add(this,&VruiState::destroyInputDeviceCallback);
	
	/* Create a button to show the scale bar: */
	GLMotif::ToggleButton* showScaleBarToggle=new GLMotif::ToggleButton("ShowScaleBarToggle",parent,"Show Scale Bar");
	showScaleBarToggle->getValueChangedCallbacks().add(this,&VruiState::showScaleBarToggleCallback);
	
	if(visletManager->getNumVislets()>0)
		{
		/* Create the vislet submenu: */
		GLMotif::CascadeButton* visletMenuCascade=new GLMotif::CascadeButton("VisletMenuCascade",parent,"Vislets");
		visletMenuCascade->setPopup(visletManager->buildVisletMenu());
		}
	
	new GLMotif::Separator("QuitSeparator",parent,GLMotif::Separator::HORIZONTAL,0.0f,GLMotif::Separator::LOWERED);
	
	/* Create a button to quit the current application: */
	GLMotif::Button* quitButton=new GLMotif::Button("QuitButton",parent,"Quit Program");
	quitButton->getSelectCallbacks().add(this,&VruiState::quitCallback);
	}

bool VruiState::loadViewpointFile(const char* viewpointFileName)
	{
	bool result=false;
	
	/* Only load viewpoint file on master; on slave nodes, navigation will be updated by main loop: */
	if(master)
		{
		try
			{
			/* Open the viewpoint file: */
			Misc::File viewpointFile(viewpointFileName,"rb",Misc::File::LittleEndian);
			
			/* Check the header: */
			char line[80];
			viewpointFile.gets(line,sizeof(line));
			if(strcmp(line,"Vrui viewpoint file v1.0\n")==0)
				{
				/* Read the environment's center point in navigational coordinates: */
				Point center;
				viewpointFile.read<Scalar>(center.getComponents(),3);
				
				/* Read the environment's size in navigational coordinates: */
				Scalar size=viewpointFile.read<Scalar>();
				
				/* Read the environment's forward direction in navigational coordinates: */
				Vector forward;
				viewpointFile.read<Scalar>(forward.getComponents(),3);
				
				/* Read the environment's up direction in navigational coordinates: */
				Vector up;
				viewpointFile.read<Scalar>(up.getComponents(),3);
				
				/* Construct the navigation transformation: */
				NavTransform nav=NavTransform::identity;
				nav*=NavTransform::translateFromOriginTo(getDisplayCenter());
				nav*=NavTransform::rotate(Rotation::fromBaseVectors(Geometry::cross(getForwardDirection(),getUpDirection()),getForwardDirection()));
				nav*=NavTransform::scale(getDisplaySize()/size);
				nav*=NavTransform::rotate(Geometry::invert(Rotation::fromBaseVectors(Geometry::cross(forward,up),forward)));
				nav*=NavTransform::translateToOriginFrom(center);
				setNavigationTransformation(nav);
				
				result=true;
				}
			}
		catch(std::runtime_error error)
			{
			/* Ignore the error and return a failure code: */
			}
		
		if(pipe!=0)
			{
			/* Send the result code to the slaves: */
			pipe->write<int>(result?1:0);
			pipe->finishMessage();
			}
		}
	else
		{
		/* Read the result code from the master: */
		result=pipe->read<int>()!=0;
		}
	
	return result;
	}

VruiState::VruiState(Comm::MulticastPipeMultiplexer* sMultiplexer,Comm::MulticastPipe* sPipe)
	:multiplexer(sMultiplexer),
	 master(multiplexer==0||multiplexer->isMaster()),
	 pipe(sPipe),
	 inchScale(1.0),
	 meterScale(1000.0/25.4),
	 displayCenter(0.0,0.0,0.0),displaySize(1.0),
	 forwardDirection(0.0,1.0,0.0),
	 upDirection(0.0,0.0,1.0),
	 floorPlane(Vector(0.0,0.0,1.0),0.0),
	 glyphRenderer(0),
	 newInputDevicePosition(0.0,0.0,0.0),
	 virtualInputDevice(0),
	 inputGraphManager(0),
	 inputDeviceManager(0),
	 inputDeviceDataSaver(0),
	 multipipeDispatcher(0),
	 lightsourceManager(0),
	 clipPlaneManager(0),
	 numViewers(0),viewers(0),mainViewer(0),
	 numScreens(0),screens(0),mainScreen(0),
	 numProtectors(0),protectors(0),
	 numListeners(0),listeners(0),mainListener(0),
	 frontplaneDist(1.0),
	 backplaneDist(1000.0),
	 backgroundColor(Color(0.0f,0.0f,0.0f,1.0f)),
	 ambientLightColor(Color(0.2f,0.2f,0.2f)),
	 widgetMaterial(GLMaterial::Color(1.0f,1.0f,1.0f),GLMaterial::Color(0.5f,0.5f,0.5f),25.0f),
	 timerEventScheduler(0),
	 widgetManager(0),
	 popWidgetsOnScreen(false),
	 systemMenuPopup(0),
	 mainMenu(0),
	 navigationTransformationEnabled(false),
	 delayNavigationTransformation(false),
	 navigationTransformationChangedMask(0x0),
	 navigationTransformation(NavTransform::identity),inverseNavigationTransformation(NavTransform::identity),
	 coordinateManager(0),
	 toolManager(0),
	 visletManager(0),
	 frameFunction(0),frameFunctionData(0),
	 displayFunction(0),displayFunctionData(0),
	 perDisplayInitFunction(0),perDisplayInitFunctionData(0),
	 soundFunction(0),soundFunctionData(0),
	 perSoundInitFunction(0),perSoundInitFunctionData(0),
	 randomSeed(0),
	 minimumFrameTime(0.0),
	 numRecentFrameTimes(0),recentFrameTimes(0),nextFrameTimeIndex(0),sortedFrameTimes(0),
	 activeNavigationTool(0),
	 widgetInteraction(false),motionWidget(0),
	 updateContinuously(false)
	{
	#if SAVESHAREDVRUISTATE
	vruiSharedStateFile=new Misc::File("/tmp/VruiSharedState.dat","wb",Misc::File::LittleEndian);
	#endif
	}

VruiState::~VruiState(void)
	{
	#if SAVESHAREDVRUISTATE
	delete vruiSharedStateFile;
	#endif
	
	/* Delete time management: */
	delete[] recentFrameTimes;
	delete[] sortedFrameTimes;
	
	/* Delete vislet management: */
	delete visletManager;
	
	/* Delete tool management: */
	delete toolManager;
	
	/* Delete coordinate manager: */
	delete coordinateManager;
	
	/* Delete widget management: */
	delete systemMenuPopup;
	delete mainMenu;
	delete uiStyleSheet.font;
	delete widgetManager;
	delete timerEventScheduler;
	
	/* Delete listeners: */
	delete[] listeners;
	
	/* Delete screen protection management: */
	delete[] protectors;
	
	/* Delete screen management: */
	delete[] screens;
	
	/* Delete viewer management: */
	delete[] viewers;
	
	/* Delete clipping plane management: */
	delete clipPlaneManager;
	
	/* Delete light source management: */
	delete lightsourceManager;
	
	/* Delete input device management: */
	delete multipipeDispatcher;
	delete inputDeviceDataSaver;
	delete inputDeviceManager;
	
	/* Delete input graph management: */
	delete inputGraphManager;
	delete virtualInputDevice;
	
	/* Delete glyph management: */
	delete glyphRenderer;
	}

void VruiState::initialize(const Misc::ConfigurationFileSection& configFileSection)
	{
	typedef std::vector<std::string> StringList;
	
	if(multiplexer!=0)
		{
		/* Set the multiplexer's timeout values: */
		multiplexer->setConnectionWaitTimeout(configFileSection.retrieveValue<double>("./multipipeConnectionWaitTimeout",0.1));
		multiplexer->setPingTimeout(configFileSection.retrieveValue<double>("./multipipePingTimeout",10.0),configFileSection.retrieveValue<int>("./multipipePingRetries",3));
		multiplexer->setReceiveWaitTimeout(configFileSection.retrieveValue<double>("./multipipeReceiveWaitTimeout",0.01));
		multiplexer->setBarrierWaitTimeout(configFileSection.retrieveValue<double>("./multipipeBarrierWaitTimeout",0.01));
		}
	
	/* Read the conversion factors from Vrui physical coordinate units to inches and meters: */
	inchScale=configFileSection.retrieveValue<Scalar>("./inchScale",inchScale);
	Scalar readMeterScale=configFileSection.retrieveValue<Scalar>("./meterScale",Scalar(0));
	if(readMeterScale>Scalar(0))
		{
		/* Update meter scale, and calculate inch scale: */
		meterScale=readMeterScale;
		inchScale=meterScale*0.0254;
		}
	else
		{
		/* Calculate meter scale: */
		meterScale=inchScale/0.0254;
		}
	
	/* Initialize environment dimensions: */
	displayCenter=configFileSection.retrieveValue<Point>("./displayCenter");
	displaySize=configFileSection.retrieveValue<Scalar>("./displaySize");
	forwardDirection=configFileSection.retrieveValue<Vector>("./forwardDirection",forwardDirection);
	forwardDirection.normalize();
	upDirection=configFileSection.retrieveValue<Vector>("./upDirection",upDirection);
	upDirection.normalize();
	floorPlane=configFileSection.retrieveValue<Plane>("./floorPlane",floorPlane);
	floorPlane.normalize();
	
	/* Initialize the glyph renderer: */
	glyphRenderer=new GlyphRenderer(configFileSection.retrieveValue<GLfloat>("./glyphSize",GLfloat(inchScale)));
	
	/* Initialize input graph manager: */
	newInputDevicePosition=configFileSection.retrieveValue<Point>("./newInputDevicePosition",displayCenter);
	virtualInputDevice=new VirtualInputDevice(glyphRenderer,configFileSection);
	inputGraphManager=new InputGraphManager(glyphRenderer,virtualInputDevice);
	
	/* Initialize input device manager: */
	inputDeviceManager=new InputDeviceManager(inputGraphManager);
	if(master)
		{
		inputDeviceManager->initialize(configFileSection);
		
		/* Check if the user wants to save input device data: */
		std::string iddsSectionName=configFileSection.retrieveString("./inputDeviceDataSaver","");
		if(iddsSectionName!="")
			{
			/* Go to input device data saver's section: */
			Misc::ConfigurationFileSection iddsSection=configFileSection.getSection(iddsSectionName.c_str());
			
			/* Initialize the input device data saver: */
			inputDeviceDataSaver=new InputDeviceDataSaver(iddsSection,*inputDeviceManager);
			}
		}
	if(multiplexer!=0)
		multipipeDispatcher=new MultipipeDispatcher(pipe,inputDeviceManager);
	
	/* Initialize the update regime: */
	if(master)
		updateContinuously=configFileSection.retrieveValue<bool>("./updateContinuously",updateContinuously);
	else
		updateContinuously=true; // Slave nodes always run in continuous mode; they will block on updates from the master
	
	/* Initialize the light source manager: */
	lightsourceManager=new LightsourceManager;
	
	/* Initialize the clipping plane manager: */
	clipPlaneManager=new ClipPlaneManager;
	
	/* Initialize the viewers: */
	StringList viewerNames=configFileSection.retrieveValue<StringList>("./viewerNames");
	numViewers=viewerNames.size();
	viewers=new Viewer[numViewers];
	for(int i=0;i<numViewers;++i)
		{
		/* Go to viewer's section: */
		Misc::ConfigurationFileSection viewerSection=configFileSection.getSection(viewerNames[i].c_str());
		
		/* Initialize viewer: */
		viewers[i].initialize(viewerSection);
		}
	mainViewer=&viewers[0];
	
	/* Initialize the screens: */
	StringList screenNames=configFileSection.retrieveValue<StringList>("./screenNames");
	numScreens=screenNames.size();
	screens=new VRScreen[numScreens];
	for(int i=0;i<numScreens;++i)
		{
		/* Go to screen's section: */
		Misc::ConfigurationFileSection screenSection=configFileSection.getSection(screenNames[i].c_str());
		
		/* Initialize screen: */
		screens[i].initialize(screenSection);
		}
	mainScreen=&screens[0];
	
	/* Initialize screen protection: */
	typedef std::vector<ScreenProtector> ScreenProtectorList;
	ScreenProtectorList spl=configFileSection.retrieveValue<ScreenProtectorList>("./screenProtectors",ScreenProtectorList());
	numProtectors=spl.size();
	protectors=new ScreenProtector[numProtectors];
	for(int i=0;i<numProtectors;++i)
		protectors[i]=spl[i];
	
	/* Initialize the listeners: */
	StringList listenerNames=configFileSection.retrieveValue<StringList>("./listenerNames",StringList());
	numListeners=listenerNames.size();
	listeners=new Listener[numListeners];
	for(int i=0;i<numListeners;++i)
		{
		/* Go to listener's section: */
		Misc::ConfigurationFileSection listenerSection=configFileSection.getSection(listenerNames[i].c_str());
		
		/* Initialize listener: */
		listeners[i].initialize(listenerSection);
		}
	mainListener=&listeners[0];
	
	/* Initialize rendering parameters: */
	frontplaneDist=configFileSection.retrieveValue<Scalar>("./frontplaneDist",frontplaneDist);
	backplaneDist=configFileSection.retrieveValue<Scalar>("./backplaneDist",backplaneDist);
	backgroundColor=configFileSection.retrieveValue<Color>("./backgroundColor",backgroundColor);
	ambientLightColor=configFileSection.retrieveValue<Color>("./ambientLightColor",ambientLightColor);
	
	/* Initialize widget management: */
	widgetMaterial=configFileSection.retrieveValue<GLMaterial>("./widgetMaterial",widgetMaterial);
	
	/* Create Vrui's default widget style sheet: */
	GLFont* font=loadFont(configFileSection.retrieveString("./uiFontName","CenturySchoolbookBoldItalic").c_str());
	font->setTextHeight(configFileSection.retrieveValue<double>("./uiFontTextHeight",1.0*inchScale));
	font->setAntialiasing(configFileSection.retrieveValue<bool>("./uiFontAntialiasing",true));
	uiStyleSheet.setFont(font);
	uiStyleSheet.setSize(configFileSection.retrieveValue<float>("./uiSize",uiStyleSheet.size));
	uiStyleSheet.borderColor=uiStyleSheet.bgColor=configFileSection.retrieveValue<Color>("./uiBgColor",uiStyleSheet.bgColor);
	uiStyleSheet.fgColor=configFileSection.retrieveValue<Color>("./uiFgColor",uiStyleSheet.fgColor);
	uiStyleSheet.textfieldBgColor=configFileSection.retrieveValue<Color>("./uiTextFieldBgColor",uiStyleSheet.textfieldBgColor);
	uiStyleSheet.textfieldFgColor=configFileSection.retrieveValue<Color>("./uiTextFieldFgColor",uiStyleSheet.textfieldFgColor);
	uiStyleSheet.titlebarBgColor=configFileSection.retrieveValue<Color>("./uiTitleBarBgColor",uiStyleSheet.titlebarBgColor);
	uiStyleSheet.titlebarFgColor=configFileSection.retrieveValue<Color>("./uiTitleBarFgColor",uiStyleSheet.titlebarFgColor);
	uiStyleSheet.sliderHandleWidth=configFileSection.retrieveValue<double>("./uiSliderWidth",uiStyleSheet.sliderHandleWidth);
	uiStyleSheet.sliderHandleColor=configFileSection.retrieveValue<Color>("./uiSliderHandleColor",uiStyleSheet.sliderHandleColor);
	uiStyleSheet.sliderShaftColor=configFileSection.retrieveValue<Color>("./uiSliderShaftColor",uiStyleSheet.sliderShaftColor);
	timerEventScheduler=new Misc::TimerEventScheduler;
	widgetManager=new GLMotif::WidgetManager;
	widgetManager->setStyleSheet(&uiStyleSheet);
	widgetManager->setTimerEventScheduler(timerEventScheduler);
	widgetManager->setDrawOverlayWidgets(configFileSection.retrieveValue<bool>("./drawOverlayWidgets",widgetManager->getDrawOverlayWidgets()));
	popWidgetsOnScreen=configFileSection.retrieveValue<bool>("./popWidgetsOnScreen",popWidgetsOnScreen);
	
	/* Create the coordinate manager: */
	coordinateManager=new CoordinateManager;
	
	/* Go to tool manager's section: */
	Misc::ConfigurationFileSection toolSection=configFileSection.getSection(configFileSection.retrieveString("./tools").c_str());
	
	/* Initialize tool manager: */
	toolManager=new ToolManager(inputDeviceManager,toolSection);
	
	try
		{
		/* Go to vislet manager's section: */
		Misc::ConfigurationFileSection visletSection=configFileSection.getSection(configFileSection.retrieveString("./vislets").c_str());
		
		/* Initialize vislet manager: */
		visletManager=new VisletManager(visletSection);
		}
	catch(std::runtime_error err)
		{
		/* Ignore error and continue... */
		}
	
	/* Initialize random number management: */
	if(master)
		randomSeed=(unsigned int)time(0);
	if(multiplexer!=0)
		{
		pipe->broadcast<unsigned int>(randomSeed);
		pipe->finishMessage();
		}
	srand(randomSeed);
	
	/* Initialize the application timer: */
	if(master)
		lastFrame=appTime.peekTime();
	if(multiplexer!=0)
		{
		pipe->broadcast<double>(lastFrame);
		pipe->finishMessage();
		}
	lastFrameDelta=0.0;
	
	/* Check if there is a frame rate limit: */
	double maxFrameRate=configFileSection.retrieveValue<double>("./maximumFrameRate",0.0);
	if(maxFrameRate>0.0)
		{
		/* Calculate the minimum frame time: */
		minimumFrameTime=1.0/maxFrameRate;
		}
	
	/* Set the current application time in the timer event scheduler: */
	timerEventScheduler->triggerEvents(lastFrame);
	
	/* Initialize the frame time calculator: */
	numRecentFrameTimes=5;
	recentFrameTimes=new double[numRecentFrameTimes];
	for(int i=0;i<numRecentFrameTimes;++i)
		recentFrameTimes[i]=1.0;
	nextFrameTimeIndex=0;
	sortedFrameTimes=new double[numRecentFrameTimes];
	currentFrameTime=1.0;
	}

void VruiState::createSystemMenu(void)
	{
	/* Create the Vrui system menu and install it as the main menu: */
	systemMenuPopup=new GLMotif::PopupMenu("VruiSystemMenuPopup",widgetManager);
	systemMenuPopup->setTitle("Vrui System");
	GLMotif::Menu* systemMenu=new GLMotif::Menu("VruiSystemMenu",systemMenuPopup,false);
	buildSystemMenu(systemMenu);
	systemMenu->manageChild();
	mainMenu=new MutexMenu(systemMenuPopup);
	}

void VruiState::initTools(const Misc::ConfigurationFileSection&)
	{
	#if DELAY_NAVIGATIONTRANSFORMATION
	/* Start delaying the navigation transformation at this point: */
	delayNavigationTransformation=true;
	#endif
	
	/* Create default tool assignment: */
	toolManager->loadDefaultTools();
	}

DisplayState* VruiState::registerContext(GLContextData& contextData) const
	{
	/* Create a new display state mapper data item: */
	DisplayStateMapper::DataItem* dataItem=new DisplayStateMapper::DataItem;
	
	/* Associate it with the OpenGL context: */
	contextData.addDataItem(&displayStateMapper,dataItem);
	
	/* Return a pointer to the display state structure: */
	return &dataItem->displayState;
	}

void VruiState::update(void)
	{
	/* Take an application timer snapshot: */
	double lastLastFrame=lastFrame;
	lastFrame=appTime.peekTime(); // Result is only used on master node
	
	int navBroadcastMask=navigationTransformationChangedMask;
	if(master)
		{
		if(minimumFrameTime>0.0)
			{
			/* Check if the time for the last frame was less than the allowed minimum: */
			if(lastFrame-lastLastFrame<minimumFrameTime)
				{
				/* Sleep for a while to reach the minimum frame time: */
				vruiDelay(minimumFrameTime-(lastFrame-lastLastFrame));
				
				/* Take another application timer snapshot: */
				lastFrame=appTime.peekTime();
				}
			}
		
		/* Update all physical input devices: */
		inputDeviceManager->updateInputDevices();
		
		/* Save input device states to data file if requested: */
		if(inputDeviceDataSaver!=0)
			inputDeviceDataSaver->saveCurrentState(lastFrame);
		
		/* Update the Vrui application timer and the frame time history: */
		recentFrameTimes[nextFrameTimeIndex]=lastFrame-lastLastFrame;
		++nextFrameTimeIndex;
		if(nextFrameTimeIndex==numRecentFrameTimes)
			nextFrameTimeIndex=0;
		
		/* Calculate current median frame time: */
		for(int i=0;i<numRecentFrameTimes;++i)
			{
			int j;
			for(j=i-1;j>=0&&sortedFrameTimes[j]>recentFrameTimes[i];--j)
				sortedFrameTimes[j+1]=sortedFrameTimes[j];
			sortedFrameTimes[j+1]=recentFrameTimes[i];
			}
		currentFrameTime=sortedFrameTimes[numRecentFrameTimes/2];
		
		#if DELAY_NAVIGATIONTRANSFORMATION
		if(navigationTransformationEnabled&&(navigationTransformationChangedMask&0x1))
			{
			/* Update the navigation transformation from the last frame: */
			navigationTransformation=newNavigationTransformation;
			inverseNavigationTransformation=Geometry::invert(navigationTransformation);
			navigationTransformationChangedMask=0x0;
			}
		#endif
		}
	
	if(multiplexer!=0)
		{
		/* Broadcast application time and current median frame time: */
		pipe->broadcast<double>(lastFrame);
		pipe->broadcast<double>(currentFrameTime);
		
		/* Broadcast the current navigation transformation and/or display center/size: */
		pipe->broadcast<int>(navBroadcastMask);
		if(navBroadcastMask&0x1)
			{
			if(master)
				{
				/* Send the new navigation transformation: */
				pipe->write<Scalar>(navigationTransformation.getTranslation().getComponents(),3);
				pipe->write<Scalar>(navigationTransformation.getRotation().getQuaternion(),4);
				pipe->write<Scalar>(navigationTransformation.getScaling());
				}
			else
				{
				/* Receive the new navigation transformation: */
				Vector translation;
				pipe->read<Scalar>(translation.getComponents(),3);
				Scalar rotationQuaternion[4];
				pipe->read<Scalar>(rotationQuaternion,4);
				Scalar scaling=pipe->read<Scalar>();
				
				/* Update the navigation transformation: */
				navigationTransformationEnabled=true;
				navigationTransformation=NavTransform(translation,Rotation::fromQuaternion(rotationQuaternion),scaling);
				inverseNavigationTransformation=Geometry::invert(navigationTransformation);
				}
			}
		if(navBroadcastMask&0x2)
			{
			/* Broadcast the new display center and size: */
			pipe->broadcast<Scalar>(displayCenter.getComponents(),3);
			pipe->broadcast<Scalar>(displaySize);
			}
		if(navBroadcastMask&0x4)
			{
			if(master)
				{
				/* Send the tool kill zone's new center: */
				pipe->write<Scalar>(toolManager->getToolKillZone()->getCenter().getComponents(),3);
				}
			else
				{
				/* Receive the tool kill zone' new center: */
				Point newCenter;
				pipe->read<Scalar>(newCenter.getComponents(),3);
				toolManager->getToolKillZone()->setCenter(newCenter);
				}
			}
		
		/* Broadcast the state of all physical input devices and other ancillary data: */
		multipipeDispatcher->dispatchState();
		pipe->finishMessage();
		}
	
	/* Calculate the current frame time delta: */
	lastFrameDelta=lastFrame-lastLastFrame;
	
	#if SAVESHAREDVRUISTATE
	/* Save shared state to a local file for post-mortem analysis purposes: */
	vruiSharedStateFile->write<double>(lastFrame);
	vruiSharedStateFile->write<double>(currentFrameTime);
	int numInputDevices=inputDeviceManager->getNumInputDevices();
	vruiSharedStateFile->write<int>(numInputDevices);
	for(int i=0;i<numInputDevices;++i)
		{
		InputDevice* id=inputDeviceManager->getInputDevice(i);
		vruiSharedStateFile->write<Scalar>(id->getPosition().getComponents(),3);
		vruiSharedStateFile->write<Scalar>(id->getOrientation().getQuaternion(),4);
		}
	#endif
	
	#if RENDERFRAMETIMES
	/* Update the frame time graph: */
	++frameTimeIndex;
	if(frameTimeIndex==numFrameTimes)
		frameTimeIndex=0;
	frameTimes[frameTimeIndex]=lastFrame-lastLastFrame;
	#endif
	
	/* Set the widget manager's time: */
	widgetManager->setTime(lastFrame);
	
	/* Trigger all due timer events: */
	timerEventScheduler->triggerEvents(lastFrame);
	
	/* Update the input graph: */
	inputGraphManager->update();
	
	/* Update the tool manager: */
	toolManager->update();
	
	/* Update viewer states: */
	for(int i=0;i<numViewers;++i)
		viewers[i].update();
	
	/* Update listener states: */
	for(int i=0;i<numListeners;++i)
		listeners[i].update();
	
	/* Call frame functions of all loaded vislets: */
	if(visletManager!=0)
		visletManager->frame();
	
	/* Call frame function: */
	frameFunction(frameFunctionData);
	
	/* Finish any pending messages on the main pipe, in case an application didn't clean up: */
	if(multiplexer!=0)
		pipe->finishMessage();
	}

void VruiState::display(DisplayState* displayState,GLContextData& contextData) const
	{
	/* Initialize standard OpenGL settings: */
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,GL_TRUE);
	glDisable(GL_COLOR_MATERIAL);
	
	/* Clear the display and Z-buffer: */
	glClearColor(backgroundColor);
	glClearDepth(1.0); // Clear depth is "infinity"
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); // Clear color- and Z-buffer
	
	/* Enable ambient light source: */
	glLightModelAmbient(ambientLightColor);
	
	/* Go to physical coordinates: */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrix(displayState->modelviewPhysical);
	
	/* Set light sources: */
	if(navigationTransformationEnabled)
		lightsourceManager->setLightsources(displayState,contextData);
	else
		lightsourceManager->setLightsources(contextData);
	
	/* Render input graph state: */
	inputGraphManager->glRenderAction(contextData);
	
	/* Render tool manager's state: */
	toolManager->glRenderAction(contextData);
	
	/* Display any realized widgets: */
	glMaterial(GLMaterialEnums::FRONT,widgetMaterial);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
	widgetManager->draw(contextData);
	glDisable(GL_COLOR_MATERIAL);
	
	/* Set clipping planes: */
	if(navigationTransformationEnabled)
		clipPlaneManager->setClipPlanes(displayState,contextData);
	else
		clipPlaneManager->setClipPlanes(contextData);
	
	/* Display all loaded vislets: */
	if(visletManager!=0)
		visletManager->display(contextData);
	
	/* Call the user display function: */
	if(displayFunction!=0)
		{
		if(navigationTransformationEnabled)
			{
			/* Go to navigational coordinates: */
			glLoadIdentity();
			glMultMatrix(displayState->modelviewNavigational);
			}
		displayFunction(contextData,displayFunctionData);
		if(navigationTransformationEnabled)
			{
			/* Go back to physical coordinates: */
			glLoadIdentity();
			glMultMatrix(displayState->modelviewPhysical);
			}
		}
	
	/* Execute the transparency rendering pass: */
	if(TransparentObject::needRenderPass())
		{
		/* Set up OpenGL state for transparency: */
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
		
		/* Execute transparent rendering pass: */
		TransparentObject::transparencyPass(contextData);
		}
	
	/* Disable all clipping planes: */
	clipPlaneManager->disableClipPlanes(contextData);
	}

void VruiState::sound(ALContextData& contextData) const
	{
	#ifdef VRUI_USE_OPENAL
	/* Call the user sound function: */
	if(soundFunction!=0)
		soundFunction(contextData,soundFunctionData);
	#endif
	}

void VruiState::fileSelectionDialogCancelCallback(GLMotif::FileSelectionDialog::CancelCallbackData* cbData)
	{
	/* Destroy the file selection dialog: */
	getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
	}

void VruiState::loadViewOKCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	/* Load the selected file only if there are no active navigation tools: */
	if(activeNavigationTool==0)
		{
		/* Load the selected viewpoint file: */
		loadViewpointFile(cbData->selectedFileName.c_str());
		}
	
	/* Destroy the file selection dialog: */
	getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
	}

void VruiState::loadViewCallback(Misc::CallbackData* cbData)
	{
	/* Create a file selection dialog to select a viewpoint file: */
	GLMotif::FileSelectionDialog* loadViewDialog=new GLMotif::FileSelectionDialog(getWidgetManager(),"Load View...",0,".view",openPipe());
	loadViewDialog->getOKCallbacks().add(this,&VruiState::loadViewOKCallback);
	loadViewDialog->getCancelCallbacks().add(this,&VruiState::fileSelectionDialogCancelCallback);
	
	/* Show the file selection dialog: */
	popupPrimaryWidget(loadViewDialog,getNavigationTransformation().transform(getDisplayCenter()));
	}

void VruiState::saveViewCallback(Misc::CallbackData* cbData)
	{
	/* Push the current navigation transformation onto the stack of navigation transformations: */
	storedNavigationTransformations.push_back(getNavigationTransformation());
	
	if(master)
		{
		try
			{
			/* Create a uniquely named viewpoint file: */
			char numberedFileName[40];
			Misc::File viewpointFile(Misc::createNumberedFileName("SavedViewpoint.view",4,numberedFileName),"wb",Misc::File::LittleEndian);
			
			/* Write a header identifying this as an environment-independent viewpoint file: */
			fprintf(viewpointFile.getFilePtr(),"Vrui viewpoint file v1.0\n");
			
			/* Write the environment's center point in navigational coordinates: */
			Point center=getInverseNavigationTransformation().transform(getDisplayCenter());
			viewpointFile.write<Scalar>(center.getComponents(),3);
			
			/* Write the environment's size in navigational coordinates: */
			Scalar size=getDisplaySize()*getInverseNavigationTransformation().getScaling();
			viewpointFile.write<Scalar>(size);
			
			/* Write the environment's forward direction in navigational coordinates: */
			Vector forward=getInverseNavigationTransformation().transform(getForwardDirection());
			viewpointFile.write<Scalar>(forward.getComponents(),3);
			
			/* Write the environment's up direction in navigational coordinates: */
			Vector up=getInverseNavigationTransformation().transform(getUpDirection());
			viewpointFile.write<Scalar>(up.getComponents(),3);
			}
		catch(Misc::File::OpenError err)
			{
			/* Ignore errors if viewpoint file could not be created */
			}
		}
	}

void VruiState::restoreViewCallback(Misc::CallbackData* cbData)
	{
	/* Only restore if no navigation tools are active and the stack is not empty: */
	if(activeNavigationTool==0&&!storedNavigationTransformations.empty())
		{
		/* Pop the most recently stored navigation transformation off the stack: */
		setNavigationTransformation(storedNavigationTransformations.back());
		storedNavigationTransformations.pop_back();
		}
	}

void VruiState::createInputDeviceCallback(Misc::CallbackData* cbData)
	{
	/* Create a new one-button virtual input device: */
	createdVirtualInputDevices.push_back(addVirtualInputDevice("VirtualInputDevice",1,0));
	}

void VruiState::destroyInputDeviceCallback(Misc::CallbackData* cbData)
	{
	/* Destroy the oldest virtual input device: */
	if(!createdVirtualInputDevices.empty())
		{
		getInputDeviceManager()->destroyInputDevice(createdVirtualInputDevices.front());
		createdVirtualInputDevices.pop_front();
		}
	}

void VruiState::showScaleBarToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	}

void VruiState::quitCallback(Misc::CallbackData* cbData)
	{
	/* Request Vrui to shut down cleanly: */
	shutdown();
	}

/********************************
Global Vrui kernel API functions:
********************************/

void vruiDelay(double interval)
	{
	#ifdef __SGI_IRIX__
	long intervalCount=(long)(interval*(double)CLK_TCK+0.5);
	while(intervalCount>0)
		intervalCount=sginap(intervalCount);
	#else
	int seconds=int(floor(interval));
	interval-=double(seconds);
	int microseconds=int(floor(interval*1000000.0+0.5));
	struct timeval tv;
	tv.tv_sec=seconds;
	tv.tv_usec=microseconds;
	select(0,0,0,0,&tv);
	#endif
	}

void synchronize(double applicationTime)
	{
	#if 0
	/* Calculate the drift in true and intended application time: */
	double delta=applicationTime-vruiState->lastFrame;
	if(delta>0.0)
		{
		/* Block to make up for the difference: */
		struct timeval timeout;
		timeout.tv_sec=long(Math::floor(delta));
		timeout.tv_usec=long(Math::floor((delta-double(timeout.tv_sec))*1000000.0+0.5));
		select(0,0,0,0,&timeout);
		}
	#endif
	
	/* Update the true application time: */
	vruiState->lastFrame=applicationTime;
	}

void setDisplayCenter(const Point& newDisplayCenter,Scalar newDisplaySize)
	{
	vruiState->displayCenter=newDisplayCenter;
	vruiState->displaySize=newDisplaySize;
	vruiState->navigationTransformationChangedMask|=0x2;
	}

/**********************************
Call-in functions for user program:
**********************************/

void setFrameFunction(FrameFunctionType frameFunction,void* userData)
	{
	vruiState->frameFunction=frameFunction;
	vruiState->frameFunctionData=userData;
	}

void setDisplayFunction(DisplayFunctionType displayFunction,void* userData)
	{
	vruiState->displayFunction=displayFunction;
	vruiState->displayFunctionData=userData;
	}

void setSoundFunction(SoundFunctionType soundFunction,void* userData)
	{
	vruiState->soundFunction=soundFunction;
	vruiState->soundFunctionData=userData;
	}

bool isMaster(void)
	{
	return vruiState->master;
	}

int getNodeIndex(void)
	{
	if(vruiState->multiplexer!=0)
		return vruiState->multiplexer->getNodeIndex();
	else
		return 0;
	}

int getNumNodes(void)
	{
	if(vruiState->multiplexer!=0)
		return vruiState->multiplexer->getNumNodes();
	else
		return 1;
	}

Comm::MulticastPipe* getMainPipe(void)
	{
	return vruiState->pipe;
	}

Comm::MulticastPipe* openPipe(void)
	{
	if(vruiState->multiplexer!=0)
		return vruiState->multiplexer->openPipe();
	else
		return 0;
	}

GlyphRenderer* getGlyphRenderer(void)
	{
	return vruiState->glyphRenderer;
	}

void renderGlyph(const Glyph& glyph,const OGTransform& transformation,GLContextData& contextData)
	{
	vruiState->glyphRenderer->renderGlyph(glyph,transformation,vruiState->glyphRenderer->getContextDataItem(contextData));
	}

VirtualInputDevice* getVirtualInputDevice(void)
	{
	return vruiState->virtualInputDevice;
	}

InputGraphManager* getInputGraphManager(void)
	{
	return vruiState->inputGraphManager;
	}

InputDeviceManager* getInputDeviceManager(void)
	{
	return vruiState->inputDeviceManager;
	}

int getNumInputDevices(void)
	{
	return vruiState->inputDeviceManager->getNumInputDevices();
	}

InputDevice* getInputDevice(int index)
	{
	return vruiState->inputDeviceManager->getInputDevice(index);
	}

InputDevice* findInputDevice(const char* name)
	{
	return vruiState->inputDeviceManager->findInputDevice(name);
	}

InputDevice* addVirtualInputDevice(const char* name,int numButtons,int numValuators)
	{
	InputDevice* newDevice=vruiState->inputDeviceManager->createInputDevice(name,InputDevice::TRACK_POS|InputDevice::TRACK_DIR|InputDevice::TRACK_ORIENT,numButtons,numValuators);
	newDevice->setTransformation(TrackerState::translateFromOriginTo(vruiState->newInputDevicePosition));
	newDevice->setDeviceRayDirection(Vector(0.0,1.0,0.0));
	vruiState->inputGraphManager->getInputDeviceGlyph(newDevice).enable(Glyph::BOX,vruiState->widgetMaterial);
	return newDevice;
	}

LightsourceManager* getLightsourceManager(void)
	{
	return vruiState->lightsourceManager;
	}

ClipPlaneManager* getClipPlaneManager(void)
	{
	return vruiState->clipPlaneManager;
	}

Viewer* getMainViewer(void)
	{
	return vruiState->mainViewer;
	}

int getNumViewers(void)
	{
	return vruiState->numViewers;
	}

Viewer* getViewer(int index)
	{
	return &vruiState->viewers[index];
	}

Viewer* findViewer(const char* name)
	{
	Viewer* result=0;
	for(int i=0;i<vruiState->numViewers;++i)
		if(strcmp(name,vruiState->viewers[i].getName())==0)
			{
			result=&vruiState->viewers[i];
			break;
			}
	return result;
	}

VRScreen* getMainScreen(void)
	{
	return vruiState->mainScreen;
	}

int getNumScreens(void)
	{
	return vruiState->numScreens;
	}

VRScreen* getScreen(int index)
	{
	return vruiState->screens+index;
	}

VRScreen* findScreen(const char* name)
	{
	VRScreen* result=0;
	for(int i=0;i<vruiState->numScreens;++i)
		if(strcmp(name,vruiState->screens[i].getName())==0)
			{
			result=&vruiState->screens[i];
			break;
			}
	return result;
	}

std::pair<VRScreen*,Scalar> findScreen(const Ray& ray)
	{
	/* Find the closest intersection with any screen: */
	VRScreen* closestScreen=0;
	Scalar closestLambda=Math::Constants<Scalar>::max;
	for(int screenIndex=0;screenIndex<vruiState->numScreens;++screenIndex)
		{
		VRScreen* screen=&vruiState->screens[screenIndex];
		
		/* Calculate screen plane: */
		ONTransform t=screen->getScreenTransformation();
		Vector screenNormal=t.getDirection(2);
		Scalar screenOffset=screenNormal*t.getOrigin();
		
		/* Intersect selection ray with screen plane: */
		Scalar divisor=screenNormal*ray.getDirection();
		if(divisor!=Scalar(0))
			{
			Scalar lambda=(screenOffset-screenNormal*ray.getOrigin())/divisor;
			if(lambda>=Scalar(0)&&lambda<closestLambda)
				{
				/* Check if the ray intersects the screen: */
				Point screenPos=t.inverseTransform(ray.getOrigin()+ray.getDirection()*lambda);
				if(screen->isOffAxis())
					{
					/* Check the intersection point against the projected screen quadrilateral: */
					VRScreen::PTransform2::Point sp(screenPos[0],screenPos[1]);
					sp=screen->getScreenHomography().inverseTransform(sp);
					if(sp[0]>=Scalar(0)&&sp[0]<=screen->getWidth()&&sp[1]>=Scalar(0)&&sp[1]<=screen->getHeight())
						{
						/* Save the intersection: */
						closestScreen=screen;
						closestLambda=lambda;
						}
					}
				else
					{
					/* Check the intersection point against the upright screen rectangle: */
					if(screenPos[0]>=Scalar(0)&&screenPos[0]<=screen->getWidth()&&screenPos[1]>=Scalar(0)&&screenPos[1]<=screen->getHeight())
						{
						/* Save the intersection: */
						closestScreen=screen;
						closestLambda=lambda;
						}
					}
				}
			}
		}
	
	return std::pair<VRScreen*,Scalar>(closestScreen,closestLambda);
	}

Listener* getMainListener(void)
	{
	return vruiState->mainListener;
	}

int getNumListeners(void)
	{
	return vruiState->numListeners;
	}

Listener* getListener(int index)
	{
	return &vruiState->listeners[index];
	}

Listener* findListener(const char* name)
	{
	Listener* result=0;
	for(int i=0;i<vruiState->numListeners;++i)
		if(strcmp(name,vruiState->listeners[i].getName())==0)
			{
			result=&vruiState->listeners[i];
			break;
			}
	return result;
	}

Scalar getInchFactor(void)
	{
	return vruiState->inchScale;
	}

Scalar getMeterFactor(void)
	{
	return vruiState->meterScale;
	}

Scalar getDisplaySize(void)
	{
	return vruiState->displaySize;
	}

const Point& getDisplayCenter(void)
	{
	return vruiState->displayCenter;
	}

const Vector& getForwardDirection(void)
	{
	return vruiState->forwardDirection;
	}

const Vector& getUpDirection(void)
	{
	return vruiState->upDirection;
	}

const Plane& getFloorPlane(void)
	{
	return vruiState->floorPlane;
	}

void setFrontplaneDist(Scalar newFrontplaneDist)
	{
	vruiState->frontplaneDist=newFrontplaneDist;
	}

Scalar getFrontplaneDist(void)
	{
	return vruiState->frontplaneDist;
	}

void setBackplaneDist(Scalar newBackplaneDist)
	{
	vruiState->backplaneDist=newBackplaneDist;
	}

Scalar getBackplaneDist(void)
	{
	return vruiState->backplaneDist;
	}

void setBackgroundColor(const Color& newBackgroundColor)
	{
	vruiState->backgroundColor=newBackgroundColor;
	}

const Color& getBackgroundColor(void)
	{
	return vruiState->backgroundColor;
	}

GLFont* loadFont(const char* fontName)
	{
	return new GLFont(fontName);
	}

const GLMotif::StyleSheet* getUiStyleSheet(void)
	{
	return &vruiState->uiStyleSheet;
	}

float getUiSize(void)
	{
	return vruiState->uiStyleSheet.size;
	}

const Color& getUiBgColor(void)
	{
	return vruiState->uiStyleSheet.bgColor;
	}

const Color& getUiFgColor(void)
	{
	return vruiState->uiStyleSheet.fgColor;
	}

const Color& getUiTextFieldBgColor(void)
	{
	return vruiState->uiStyleSheet.textfieldBgColor;
	}

const Color& getUiTextFieldFgColor(void)
	{
	return vruiState->uiStyleSheet.textfieldFgColor;
	}

GLFont* getUiFont(void)
	{
	return vruiState->uiStyleSheet.font;
	}

void setWidgetMaterial(const GLMaterial& newWidgetMaterial)
	{
	vruiState->widgetMaterial=newWidgetMaterial;
	}

const GLMaterial& getWidgetMaterial(void)
	{
	return vruiState->widgetMaterial;
	}

void setMainMenu(GLMotif::PopupMenu* newMainMenu)
	{
	/* Delete old main menu shell and system menu popup: */
	delete vruiState->mainMenu;
	delete vruiState->systemMenuPopup;
	vruiState->systemMenuPopup=0;
	
	/* Add the Vrui system menu to the end of the given main menu: */
	GLMotif::Menu* menuChild=dynamic_cast<GLMotif::Menu*>(newMainMenu->getChild());
	if(menuChild!=0)
		{
		/* Create the Vrui system menu (not saved, because it's deleted automatically by the cascade button): */
		GLMotif::Popup* systemMenuPopup=new GLMotif::Popup("VruiSystemMenuPopup",vruiState->widgetManager);
		GLMotif::SubMenu* systemMenu=new GLMotif::SubMenu("VruiSystemMenu",systemMenuPopup,false);
		vruiState->buildSystemMenu(systemMenu);
		systemMenu->manageChild();
		
		/* Create a cascade button at the end of the main menu: */
		new GLMotif::Separator("VruiSystemMenuSeparator",menuChild,GLMotif::Separator::HORIZONTAL,0.0f,GLMotif::Separator::LOWERED);
		
		GLMotif::CascadeButton* systemMenuCascade=new GLMotif::CascadeButton("VruiSystemMenuCascade",menuChild,"Vrui System");
		systemMenuCascade->setPopup(systemMenuPopup);
		}
	
	/* Create new main menu shell: */
	vruiState->mainMenu=new MutexMenu(newMainMenu);
	}

MutexMenu* getMainMenu(void)
	{
	return vruiState->mainMenu;
	}

Misc::TimerEventScheduler* getTimerEventScheduler(void)
	{
	return vruiState->timerEventScheduler;
	}

GLMotif::WidgetManager* getWidgetManager(void)
	{
	return vruiState->widgetManager;
	}

void popupPrimaryWidget(GLMotif::Widget* topLevel,const Point& hotSpot)
	{
	typedef GLMotif::WidgetManager::Transformation WTransform;
	typedef WTransform::Point WPoint;
	typedef WTransform::Vector WVector;
	
	WPoint globalHotSpot;
	if(vruiState->navigationTransformationEnabled)
		globalHotSpot=vruiState->inverseNavigationTransformation.transform(hotSpot);
	else
		globalHotSpot=hotSpot;
	
	WTransform widgetTransformation;
	if(vruiState->popWidgetsOnScreen)
		{
		/* Project the global hot spot into the screen plane: */
		const ONTransform screenT=vruiState->mainScreen->getScreenTransformation();
		Point screenHotSpot=screenT.inverseTransform(Point(globalHotSpot));
		// screenHotSpot[1]=Scalar(0);
		
		/* Align the widget with the main screen's plane: */
		widgetTransformation=WTransform(screenT);
		widgetTransformation*=WTransform::translateFromOriginTo(screenHotSpot);
		}
	else
		{
		/* Align the widget with the viewing direction: */
		WVector viewDirection=globalHotSpot-vruiState->mainViewer->getHeadPosition();
		WVector x=Geometry::cross(viewDirection,vruiState->upDirection);
		WVector y=Geometry::cross(x,viewDirection);
		widgetTransformation=WTransform::translateFromOriginTo(globalHotSpot);
		WTransform::Rotation rot=WTransform::Rotation::fromBaseVectors(x,y);
		widgetTransformation*=WTransform::rotate(rot);
		}
	
	/* Center the widget on the given hot spot: */
	WVector widgetOffset;
	for(int i=0;i<3;++i)
		widgetOffset[i]=topLevel->getExterior().origin[i]+0.5*topLevel->getExterior().size[i];
	widgetTransformation*=WTransform::translate(-widgetOffset);
	
	/* Pop up the widget: */
	vruiState->widgetManager->popupPrimaryWidget(topLevel,widgetTransformation);
	}

void popupPrimaryScreenWidget(GLMotif::Widget* topLevel,Scalar x,Scalar y)
	{
	typedef GLMotif::WidgetManager::Transformation WTransform;
	typedef WTransform::Vector WVector;
	
	/* Calculate a transformation moving the widget to its given position on the screen: */
	Scalar screenX=x*(vruiState->mainScreen->getWidth()-topLevel->getExterior().size[0]);
	Scalar screenY=y*(vruiState->mainScreen->getHeight()-topLevel->getExterior().size[1]);
	WTransform widgetTransformation=vruiState->mainScreen->getTransform();
	widgetTransformation*=WTransform::translate(WVector(screenX,screenY,vruiState->inchScale));
	
	/* Pop up the widget: */
	vruiState->widgetManager->popupPrimaryWidget(topLevel,widgetTransformation);
	}

void popdownPrimaryWidget(GLMotif::Widget* topLevel)
	{
	/* Pop down the widget: */
	vruiState->widgetManager->popdownWidget(topLevel);
	}

void setNavigationTransformation(const NavTransform& newNavigationTransformation)
	{
	vruiState->navigationTransformationEnabled=true;
	#if DELAY_NAVIGATIONTRANSFORMATION
	if(vruiState->delayNavigationTransformation)
		{
		/* Schedule a change in navigation transformation for the next frame: */
		vruiState->newNavigationTransformation=newNavigationTransformation;
		vruiState->newNavigationTransformation.renormalize();
		vruiState->navigationTransformationChangedMask|=0x1;
		requestUpdate();
		}
	else
		{
		/* Change the navigation transformation right away: */
		vruiState->navigationTransformation=newNavigationTransformation;
		}
	#else
	vruiState->navigationTransformation=newNavigationTransformation;
	#endif
	}

void setNavigationTransformation(const Point& center,Scalar radius)
	{
	NavTransform t=NavTransform::translateFromOriginTo(vruiState->displayCenter);
	t*=NavTransform::scale(vruiState->displaySize/radius);
	t*=NavTransform::translateToOriginFrom(center);
	vruiState->navigationTransformationEnabled=true;
	#if DELAY_NAVIGATIONTRANSFORMATION
	if(vruiState->delayNavigationTransformation)
		{
		/* Schedule a change in navigation transformation for the next frame: */
		vruiState->newNavigationTransformation=t;
		vruiState->navigationTransformationChangedMask|=0x1;
		requestUpdate();
		}
	else
		{
		/* Change the navigation transformation right away: */
		vruiState->navigationTransformation=t;
		}
	#else
	vruiState->navigationTransformation=t;
	#endif
	}

void setNavigationTransformation(const Point& center,Scalar radius,const Vector& up)
	{
	NavTransform t=NavTransform::translateFromOriginTo(vruiState->displayCenter);
	t*=NavTransform::scale(vruiState->displaySize/radius);
	t*=NavTransform::rotate(NavTransform::Rotation::rotateFromTo(up,vruiState->upDirection));
	t*=NavTransform::translateToOriginFrom(center);
	vruiState->navigationTransformationEnabled=true;
	#if DELAY_NAVIGATIONTRANSFORMATION
	if(vruiState->delayNavigationTransformation)
		{
		/* Schedule a change in navigation transformation for the next frame: */
		vruiState->newNavigationTransformation=t;
		vruiState->navigationTransformationChangedMask|=0x1;
		requestUpdate();
		}
	else
		{
		/* Change the navigation transformation right away: */
		vruiState->navigationTransformation=t;
		}
	#else
	vruiState->navigationTransformation=t;
	#endif
	}

void concatenateNavigationTransformation(const NavTransform& t)
	{
	#if DELAY_NAVIGATIONTRANSFORMATION
	if(vruiState->delayNavigationTransformation)
		{
		/* Schedule a change in navigation transformation for the next frame: */
		if((vruiState->navigationTransformationChangedMask&0x1)==0)
			vruiState->newNavigationTransformation=vruiState->navigationTransformation;
		vruiState->newNavigationTransformation*=t;
		vruiState->newNavigationTransformation.renormalize();
		vruiState->navigationTransformationChangedMask|=0x1;
		requestUpdate();
		}
	else
		{
		/* Change the navigation transformation right away: */
		vruiState->navigationTransformation*=t;
		vruiState->navigationTransformation.renormalize();
		}
	#else
	vruiState->navigationTransformation*=t;
	vruiState->navigationTransformation.renormalize();
	#endif
	}

void concatenateNavigationTransformationLeft(const NavTransform& t)
	{
	#if DELAY_NAVIGATIONTRANSFORMATION
	if(vruiState->delayNavigationTransformation)
		{
		/* Schedule a change in navigation transformation for the next frame: */
		if((vruiState->navigationTransformationChangedMask&0x1)==0)
			vruiState->newNavigationTransformation=vruiState->navigationTransformation;
		vruiState->newNavigationTransformation.leftMultiply(t);
		vruiState->newNavigationTransformation.renormalize();
		vruiState->navigationTransformationChangedMask|=0x1;
		requestUpdate();
		}
	else
		{
		/* Change the navigation transformation right away: */
		vruiState->navigationTransformation.leftMultiply(t);
		vruiState->navigationTransformation.renormalize();
		}
	#else
	vruiState->navigationTransformation.leftMultiply(t);
	vruiState->navigationTransformation.renormalize();
	#endif
	}

const NavTransform& getNavigationTransformation(void)
	{
	if(vruiState->navigationTransformationEnabled)
		return vruiState->navigationTransformation;
	else
		return NavTransform::identity;
	}

const NavTransform& getInverseNavigationTransformation(void)
	{
	if(vruiState->navigationTransformationEnabled)
		return vruiState->inverseNavigationTransformation;
	else
		return NavTransform::identity;
	}

void disableNavigationTransformation(void)
	{
	vruiState->navigationTransformationEnabled=false;
	}

Point getHeadPosition(void)
	{
	if(vruiState->navigationTransformationEnabled)
		return vruiState->inverseNavigationTransformation.transform(vruiState->mainViewer->getHeadPosition());
	else
		return vruiState->mainViewer->getHeadPosition();
	}

Vector getViewDirection(void)
	{
	if(vruiState->navigationTransformationEnabled)
		return vruiState->inverseNavigationTransformation.transform(vruiState->mainViewer->getViewDirection());
	else
		return vruiState->mainViewer->getViewDirection();
	}

Point getDevicePosition(InputDevice* device)
	{
	if(vruiState->navigationTransformationEnabled)
		return vruiState->inverseNavigationTransformation.transform(device->getPosition());
	else
		return device->getPosition();
	}

NavTrackerState getDeviceTransformation(InputDevice* device)
	{
	if(vruiState->navigationTransformationEnabled)
		return vruiState->inverseNavigationTransformation*NavTransform(device->getTransformation());
	else
		return device->getTransformation();
	}

CoordinateManager* getCoordinateManager(void)
	{
	return vruiState->coordinateManager;
	}

ToolManager* getToolManager(void)
	{
	return vruiState->toolManager;
	}

bool activateNavigationTool(const Tool* tool)
	{
	/* Can not activate the given tool if navigation is disabled: */
	if(!vruiState->navigationTransformationEnabled)
		return false;
	
	/* Can not activate the given tool if another navigation tool is already active: */
	if(vruiState->activeNavigationTool!=0&&vruiState->activeNavigationTool!=tool)
		return false;
	
	/* Activate the given tool: */
	vruiState->activeNavigationTool=tool;
	return true;
	}

void deactivateNavigationTool(const Tool* tool)
	{
	/* If the given tool is currently active, deactivate it: */
	if(vruiState->activeNavigationTool==tool)
		vruiState->activeNavigationTool=0;
	}

VisletManager* getVisletManager(void)
	{
	return vruiState->visletManager;
	}

double getApplicationTime(void)
	{
	return vruiState->lastFrame;
	}

double getFrameTime(void)
	{
	return vruiState->lastFrameDelta;
	}

double getCurrentFrameTime(void)
	{
	return vruiState->currentFrameTime;
	}

void updateContinuously(void)
	{
	vruiState->updateContinuously=true;
	}

const DisplayState& getDisplayState(GLContextData& contextData)
	{
	/* Retrieve the display state mapper's data item from the OpenGL context: */
	VruiState::DisplayStateMapper::DataItem* dataItem=contextData.retrieveDataItem<VruiState::DisplayStateMapper::DataItem>(&vruiState->displayStateMapper);
	
	/* Return the embedded display state object: */
	return dataItem->displayState;
	}

}
