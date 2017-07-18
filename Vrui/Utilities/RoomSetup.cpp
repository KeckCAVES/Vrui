/***********************************************************************
RoomSetup - Vrui application to calculate basic layout parameters of a
tracked VR environment.
Copyright (c) 2016-2017 Oliver Kreylos

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
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <Misc/FileTests.h>
#include <Misc/FunctionCalls.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/MessageLogger.h>
#include <Threads/TripleBuffer.h>
#include <Geometry/Point.h>
#include <Geometry/AffineCombiner.h>
#include <Geometry/Vector.h>
#include <Geometry/Plane.h>
#include <Geometry/PCACalculator.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GL/GLModels.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/Pager.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Margin.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/TextField.h>
#include <GLMotif/DropdownBox.h>
#include <Vrui/Vrui.h>
#include <Vrui/CoordinateManager.h>
#include <Vrui/Application.h>
#include <Vrui/Internal/Config.h>
#include <Vrui/Internal/VRDeviceState.h>
#include <Vrui/Internal/VRDeviceDescriptor.h>
#include <Vrui/Internal/VRDeviceClient.h>

class RoomSetup:public Vrui::Application
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
	
	enum Modes // Enumerated type for setup modes
		{
		Controller, // Controller calibration
		Floor, // Floor calibration
		Forward, // Forward direction
		Boundary // Environment boundary polygon (screen protector) setup
		};
	
	/* Elements: */
	Vrui::VRDeviceClient* deviceClient; // Connection to the VRDeviceDaemon
	std::vector<const Vrui::VRDeviceDescriptor*> controllers; // List of input devices that have buttons
	Vrui::Point customProbeTip; // Probe tip position defined on the command line
	Vrui::Point probeTip; // Position of probe tip in controller's local coordinate system
	
	/* Current environment definition: */
	std::string rootSectionName; // Name of root section to set up
	Vrui::Scalar meterScale; // Length of one meter in configured Vrui physical units
	Vrui::Point initialDisplayCenter; // Display center read from starting configuration file
	Vrui::Point displayCenter;
	Vrui::Scalar initialDisplaySize; // Display size read from starting configuration file
	Vrui::Scalar displaySize;
	Vrui::Vector forwardDirection;
	Vrui::Vector initialUpDirection; // Up direction read from starting configuration file
	Vrui::Vector upDirection;
	Vrui::Plane floorPlane;
	Vrui::Scalar centerHeight; // Height of initial display center above initial floor
	
	/* Setup state: */
	Modes mode; // Current set-up mode
	std::vector<Vrui::Point> floorPoints; // Vector of floor set-up points; first point is tentative environment center
	Vrui::Vector forwardSampler; // Sampler for forward direction
	std::vector<Vrui::Point> boundaryVertices; // Vector of boundary polygon vertices
	
	/* UI state: */
	GLMotif::PopupWindow* setupDialogPopup; // The setup dialog
	GLMotif::TextField* probeTipTextFields[3]; // Text fields displaying current probe tip position in device coordinates
	GLMotif::TextField* centerTextFields[3]; // Text fields displaying environment center position
	GLMotif::TextField* upTextFields[3]; // Text fields displaying environment up vector
	GLMotif::TextField* forwardTextFields[3]; // Text fields displaying environment forward vector
	
	/* Interaction state: */
	Threads::TripleBuffer<Vrui::TrackerState*> controllerStates; // Triple buffer of arrays of current controller tracking states
	int previousPressedButtonIndex; // Index of the last pressed controller button
	Threads::TripleBuffer<int> pressedButtonIndex; // Triple buffer containing index of the currently pressed controller button, or -1
	Vrui::Point::AffineCombiner pointCombiner; // Accumulator to sample controller positions
	Vrui::Vector vectorCombiner; // Accumulator to sample controller directions
	
	/* Private methods: */
	void setupDialogPageChangedCallback(GLMotif::Pager::PageChangedCallbackData* cbData);
	void controllerTypeValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData);
	void probeTipTextFieldValueChangeCallback(GLMotif::TextField::ValueChangedCallbackData* cbData,const int& textFieldIndex);
	void floorResetButtonCallback(Misc::CallbackData* cbData);
	void boundaryResetButtonCallback(Misc::CallbackData* cbData);
	void saveButtonCallback(Misc::CallbackData* cbData);
	GLMotif::PopupWindow* createSetupDialog(bool haveCustomProbeTip); // Creates a dialog window to control the setup process
	void trackingCallback(Vrui::VRDeviceClient* client); // Called when new tracking data arrives
	Vrui::Point project(const Vrui::Point& p) const // Projects a point to the current floor plane along the current up direction
		{
		return p+upDirection*((displayCenter-p)*upDirection);
		}
	Vrui::Vector project(const Vrui::Vector& v) const // Projects a vector into the current floor plane
		{
		return v-upDirection*(v*upDirection);
		}
	
	/* Constructors and destructors: */
	public:
	RoomSetup(int& argc,char**& argv);
	virtual ~RoomSetup(void);
	
	/* Methods from Vrui::Application: */
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	};

/**************************
Methods of class RoomSetup:
**************************/

void RoomSetup::setupDialogPageChangedCallback(GLMotif::Pager::PageChangedCallbackData* cbData)
	{
	switch(cbData->newCurrentChildIndex)
		{
		case 0:
			mode=Controller;
			break;
		
		case 1:
			mode=Floor;
			break;
		
		case 2:
			mode=Forward;
			break;
		
		case 3:
			mode=Boundary;
			break;
		}
	}

void RoomSetup::controllerTypeValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData)
	{
	/* Update the probe tip position: */
	bool allowEditing=false;
	switch(cbData->newSelectedItem)
		{
		case 0: // Raw from device driver
			probeTip=Point::origin;
			break;
		
		case 1: // Custom controller
			probeTip=customProbeTip;
			allowEditing=true;
			break;
		
		case 2: // Vive DK1 controller
			probeTip=Point(0.0,-0.015,-0.041);
			break;
		
		case 3: // Vive and Vive Pre controller
			probeTip=Point(0.0,-0.075,-0.039);
			break;
		}
	
	/* Update the probe tip text fields: */
	for(int i=0;i<3;++i)
		{
		probeTipTextFields[i]->setEditable(allowEditing);
		probeTipTextFields[i]->setValue(probeTip[i]);
		}
	}

void RoomSetup::probeTipTextFieldValueChangeCallback(GLMotif::TextField::ValueChangedCallbackData* cbData,const int& textFieldIndex)
	{
	/* Store the new custom value and update the current value: */
	probeTip[textFieldIndex]=customProbeTip[textFieldIndex]=atof(cbData->value);
	}

void RoomSetup::floorResetButtonCallback(Misc::CallbackData* cbData)
	{
	/* Reset floor calibration: */
	displayCenter=initialDisplayCenter;
	upDirection=initialUpDirection;
	floorPoints.clear();
	for(int i=0;i<3;++i)
		{
		centerTextFields[i]->setValue(displayCenter[i]);
		upTextFields[i]->setValue(upDirection[i]);
		}
	
	resetNavigation();
	}

void RoomSetup::boundaryResetButtonCallback(Misc::CallbackData* cbData)
	{
	/* Reset boundary setup: */
	displaySize=initialDisplaySize;
	boundaryVertices.clear();
	
	resetNavigation();
	}

void RoomSetup::saveButtonCallback(Misc::CallbackData* cbData)
	{
	/* Overwrite per-user or system-wide configuration file: */
	#if VRUI_INTERNAL_CONFIG_HAVE_USERCONFIGFILE
	const char* home=getenv("HOME");
	if(home==0||home[0]=='\0')
		{
		Misc::userError("Save Layout: No $HOME variable defined; cannot patch per-user configuration file");
		return;
		}
	std::string configDirName=home;
	configDirName.push_back('/');
	configDirName.append(VRUI_INTERNAL_CONFIG_USERCONFIGDIR);
	#else
	std::string configDirName=VRUI_INTERNAL_CONFIG_SYSCONFIGDIR;
	#endif
	std::string configFileName=configDirName;
	configFileName.push_back('/');
	configFileName.append(VRUI_INTERNAL_CONFIG_CONFIGFILENAME);
	configFileName.append(VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX);
	
	try
		{
		/* Check if the target configuration file already exists: */
		if(Misc::doesPathExist(configFileName.c_str()))
			{
			/* Patch the target configuration file: */
			std::string tagPath="Vrui/";
			tagPath.append(rootSectionName);
			tagPath.push_back('/');
			Misc::ConfigurationFile::patchFile(configFileName.c_str(),(tagPath+"displayCenter").c_str(),Misc::ValueCoder<Vrui::Point>::encode(displayCenter+upDirection*centerHeight).c_str());
			Misc::ConfigurationFile::patchFile(configFileName.c_str(),(tagPath+"displaySize").c_str(),Misc::ValueCoder<Vrui::Scalar>::encode(displaySize).c_str());
			Misc::ConfigurationFile::patchFile(configFileName.c_str(),(tagPath+"forwardDirection").c_str(),Misc::ValueCoder<Vrui::Vector>::encode(forwardDirection).c_str());
			Misc::ConfigurationFile::patchFile(configFileName.c_str(),(tagPath+"upDirection").c_str(),Misc::ValueCoder<Vrui::Vector>::encode(upDirection).c_str());
			Misc::ConfigurationFile::patchFile(configFileName.c_str(),(tagPath+"floorPlane").c_str(),Misc::ValueCoder<Vrui::Plane>::encode(floorPlane).c_str());
			
			std::string screenProtectorAreas;
			screenProtectorAreas.push_back('(');
			
			if(boundaryVertices.size()>=3)
				{
				/* Create one boundary rectangle for each boundary line segment: */
				unsigned int i0=boundaryVertices.size()-1;
				for(unsigned int i1=0;i1<boundaryVertices.size();i0=i1,++i1)
					{
					/* Turn the two boundary vertices into a rectangle: */
					std::vector<Vrui::Point> polygon;
					polygon.push_back(project(boundaryVertices[i0]));
					polygon.push_back(project(boundaryVertices[i1]));
					polygon.push_back(project(boundaryVertices[i1])+upDirection*Vrui::Scalar(2.5));
					polygon.push_back(project(boundaryVertices[i0])+upDirection*Vrui::Scalar(2.5));
					if(i1>0)
						screenProtectorAreas.append("                      ");
					screenProtectorAreas.append(Misc::ValueCoder<std::vector<Vrui::Point> >::encode(polygon));
					screenProtectorAreas.append(", \\\n");
					}
				
				/* Create the floor polygon: */
				std::vector<Vrui::Point>::iterator bvIt=boundaryVertices.begin();
				screenProtectorAreas.append("                      (");
				screenProtectorAreas.append(Misc::ValueCoder<Vrui::Point>::encode(project(*bvIt)));
				unsigned int pointsInLine=1;
				for(++bvIt;bvIt!=boundaryVertices.end();++bvIt)
					{
					if(pointsInLine==4)
						{
						screenProtectorAreas.append(", \\\n                       ");
						pointsInLine=0;
						}
					else
						screenProtectorAreas.append(", ");
					screenProtectorAreas.append(Misc::ValueCoder<Vrui::Point>::encode(project(*bvIt)));
					++pointsInLine;
					}
				screenProtectorAreas.push_back(')');
				}
			
			screenProtectorAreas.push_back(')');
			Misc::ConfigurationFile::patchFile(configFileName.c_str(),(tagPath+"screenProtectorAreas").c_str(),screenProtectorAreas.c_str());
			}
		else
			{
			/* Check if the configuration directory already exists: */
			if(!Misc::doesPathExist(configDirName.c_str()))
				if(mkdir(configDirName.c_str(),S_IRWXU|S_IRWXG|S_IRWXO)!=0)
					{
					int error=errno;
					Misc::formattedUserError("Save Layout: Unable to create per-user configuration directory due to error %d (%s)",error,strerror(error));
					return;
					}
			
			/* Write a new configuration file: */
			std::ofstream configFile(configFileName.c_str());
			configFile<<"section Vrui"<<std::endl;
			configFile<<"\tsection "<<rootSectionName<<std::endl;
			
			/* Write basic layout parameters: */
			configFile<<"\t\tdisplayCenter "<<Misc::ValueCoder<Vrui::Point>::encode(displayCenter+upDirection*centerHeight)<<std::endl;
			configFile<<"\t\tdisplaySize "<<Misc::ValueCoder<Vrui::Scalar>::encode(displaySize)<<std::endl;
			configFile<<"\t\tforwardDirection "<<Misc::ValueCoder<Vrui::Vector>::encode(forwardDirection)<<std::endl;
			configFile<<"\t\tupDirection "<<Misc::ValueCoder<Vrui::Vector>::encode(upDirection)<<std::endl;
			configFile<<"\t\tfloorPlane "<<Misc::ValueCoder<Vrui::Plane>::encode(floorPlane)<<std::endl;
			
			/* Write list of screen protector areas: */
			configFile<<"\t\tscreenProtectorAreas (";
			if(boundaryVertices.size()>=3)
				{
				/* Create one boundary rectangle for each boundary line segment: */
				unsigned int i0=boundaryVertices.size()-1;
				for(unsigned int i1=0;i1<boundaryVertices.size();i0=i1,++i1)
					{
					/* Turn the two boundary vertices into a rectangle: */
					std::vector<Vrui::Point> polygon;
					polygon.push_back(project(boundaryVertices[i0]));
					polygon.push_back(project(boundaryVertices[i1]));
					polygon.push_back(project(boundaryVertices[i1])+upDirection*Vrui::Scalar(2.5));
					polygon.push_back(project(boundaryVertices[i0])+upDirection*Vrui::Scalar(2.5));
					if(i1>0)
						configFile<<"\t\t                      ";
					configFile<<Misc::ValueCoder<std::vector<Vrui::Point> >::encode(polygon)<<", \\"<<std::endl;
					}
				
				/* Create the floor polygon: */
				configFile<<"\t\t                      (";
				std::vector<Vrui::Point>::iterator bvIt=boundaryVertices.begin();
				configFile<<Misc::ValueCoder<Vrui::Point>::encode(project(*bvIt));
				unsigned int pointsInLine=1;
				for(++bvIt;bvIt!=boundaryVertices.end();++bvIt)
					{
					if(pointsInLine==4)
						{
						configFile<<", \\"<<std::endl<<"\t\t                       ";
						pointsInLine=0;
						}
					else
						configFile<<", ";
					configFile<<Misc::ValueCoder<Vrui::Point>::encode(project(*bvIt));
					++pointsInLine;
					}
				}
			configFile<<"))"<<std::endl;
			
			configFile<<"\tendsection"<<std::endl;
			configFile<<"endsection"<<std::endl;
			}
		
		#if VRUI_INTERNAL_CONFIG_HAVE_USERCONFIGFILE
		Misc::formattedUserNote("Save Layout: Room layout saved to per-user configuration file %s",configFileName.c_str());
		#else
		Misc::formattedUserNote("Save Layout: Room layout saved to system-wide configuration file %s",configFileName.c_str());
		#endif
		}
	catch(std::runtime_error err)
		{
		Misc::formattedUserError("Save Layout: Unable to save room layout due to exception %s",err.what());
		}
	}

GLMotif::PopupWindow* RoomSetup::createSetupDialog(bool haveCustomProbeTip)
	{
	/* Get the style sheet: */
	const GLMotif::StyleSheet& ss=*Vrui::getWidgetManager()->getStyleSheet();
	
	/* Create the dialog window: */
	GLMotif::PopupWindow* setupDialogPopup=new GLMotif::PopupWindow("SetupDialogPopup",Vrui::getWidgetManager(),"Environment Setup");
	setupDialogPopup->setHideButton(true);
	setupDialogPopup->setResizableFlags(true,false);
	
	GLMotif::RowColumn* setupDialog=new GLMotif::RowColumn("SetupDialog",setupDialogPopup,false);
	setupDialog->setOrientation(GLMotif::RowColumn::VERTICAL);
	setupDialog->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	setupDialog->setNumMinorWidgets(1);
	
	/* Create a multi-page notebook: */
	GLMotif::Pager* pager=new GLMotif::Pager("Pager",setupDialog,false);
	pager->setMarginWidth(ss.size);
	pager->getPageChangedCallbacks().add(this,&RoomSetup::setupDialogPageChangedCallback);
	
	/* Create the controller setup page: */
	pager->setNextPageName("Controller");
	
	GLMotif::Margin* controllerPaneMargin=new GLMotif::Margin("ControllerPaneMargin",pager,false);
	controllerPaneMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::VCENTER));
	
	GLMotif::RowColumn* controllerPane=new GLMotif::RowColumn("ControllerPane",controllerPaneMargin,false);
	controllerPane->setOrientation(GLMotif::RowColumn::VERTICAL);
	controllerPane->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	controllerPane->setNumMinorWidgets(2);
	
	/* Create a drop-down menu to select controller types: */
	new GLMotif::Label("ControllerTypeLabel",controllerPane,"Controller Type");
	
	GLMotif::DropdownBox* controllerTypeBox=new GLMotif::DropdownBox("ControllerTypeBox",controllerPane);
	controllerTypeBox->addItem("From Driver");
	controllerTypeBox->addItem("Custom");
	controllerTypeBox->addItem("Vive DK1");
	controllerTypeBox->addItem("Vive");
	controllerTypeBox->getValueChangedCallbacks().add(this,&RoomSetup::controllerTypeValueChangedCallback);
	controllerTypeBox->setSelectedItem(haveCustomProbeTip?1:0);
	
	/* Create a set of text fields to display the probe tip position: */
	new GLMotif::Label("ProbeTipLabel",controllerPane,"Probe Tip");
	
	GLMotif::RowColumn* probeTipBox=new GLMotif::RowColumn("ProbeTipBox",controllerPane,false);
	probeTipBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	probeTipBox->setPacking(GLMotif::RowColumn::PACK_GRID);
	probeTipBox->setNumMinorWidgets(1);
	
	for(int i=0;i<3;++i)
		{
		char textFieldName[]="ProbeTipTextField0";
		textFieldName[17]='0'+i;
		probeTipTextFields[i]=new GLMotif::TextField(textFieldName,probeTipBox,6);
		probeTipTextFields[i]->setPrecision(3);
		probeTipTextFields[i]->setFloatFormat(GLMotif::TextField::FIXED);
		probeTipTextFields[i]->setValue(probeTip[i]);
		probeTipTextFields[i]->getValueChangedCallbacks().add(this,&RoomSetup::probeTipTextFieldValueChangeCallback,i);
		}
	
	probeTipBox->manageChild();
	
	controllerPane->manageChild();
	
	controllerPaneMargin->manageChild();
	
	/* Create the floor setup page: */
	pager->setNextPageName("Floor Plane");
	
	GLMotif::Margin* floorPaneMargin=new GLMotif::Margin("FloorPaneMargin",pager,false);
	floorPaneMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::VCENTER));
	
	GLMotif::RowColumn* floorPane=new GLMotif::RowColumn("FloorPane",floorPaneMargin,false);
	floorPane->setOrientation(GLMotif::RowColumn::VERTICAL);
	floorPane->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	floorPane->setNumMinorWidgets(1);
	
	GLMotif::RowColumn* floorDisplayBox=new GLMotif::RowColumn("FloorDisplayBox",floorPane,false);
	floorDisplayBox->setOrientation(GLMotif::RowColumn::VERTICAL);
	floorDisplayBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	floorDisplayBox->setNumMinorWidgets(4);
	
	new GLMotif::Label("CenterLabel",floorDisplayBox,"Center");
	for(int i=0;i<3;++i)
		{
		char textFieldName[]="CenterTextField0";
		textFieldName[15]='0'+i;
		centerTextFields[i]=new GLMotif::TextField(textFieldName,floorDisplayBox,8);
		centerTextFields[i]->setPrecision(3);
		centerTextFields[i]->setFloatFormat(GLMotif::TextField::FIXED);
		centerTextFields[i]->setValue(initialDisplayCenter[i]);
		}
	
	new GLMotif::Label("UpLabel",floorDisplayBox,"Up");
	for(int i=0;i<3;++i)
		{
		char textFieldName[]="UpTextField0";
		textFieldName[11]='0'+i;
		upTextFields[i]=new GLMotif::TextField(textFieldName,floorDisplayBox,8);
		upTextFields[i]->setPrecision(3);
		upTextFields[i]->setFloatFormat(GLMotif::TextField::FIXED);
		upTextFields[i]->setValue(initialUpDirection[i]);
		}
	
	for(int i=1;i<4;++i)
		floorDisplayBox->setColumnWeight(i,1.0f);
	floorDisplayBox->manageChild();
	
	GLMotif::Margin* floorButtonMargin=new GLMotif::Margin("FloorButtonMargin",floorPane,false);
	floorButtonMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::RIGHT));
	
	GLMotif::Button* floorResetButton=new GLMotif::Button("FloorResetButton",floorButtonMargin,"Reset");
	floorResetButton->getSelectCallbacks().add(this,&RoomSetup::floorResetButtonCallback);
	
	floorButtonMargin->manageChild();
	
	floorPane->manageChild();
	
	floorPaneMargin->manageChild();
	
	/* Create the forward direction setup page: */
	pager->setNextPageName("Forward Direction");
	
	GLMotif::Margin* forwardPaneMargin=new GLMotif::Margin("ForwardPaneMargin",pager,false);
	forwardPaneMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::VCENTER));
	
	GLMotif::RowColumn* forwardPane=new GLMotif::RowColumn("ForwardPane",forwardPaneMargin,false);
	forwardPane->setOrientation(GLMotif::RowColumn::VERTICAL);
	forwardPane->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	forwardPane->setNumMinorWidgets(4);
	
	new GLMotif::Label("ForwardLabel",forwardPane,"Forward");
	for(int i=0;i<3;++i)
		{
		char textFieldName[]="ForwardTextField0";
		textFieldName[16]='0'+i;
		forwardTextFields[i]=new GLMotif::TextField(textFieldName,forwardPane,8);
		forwardTextFields[i]->setPrecision(3);
		forwardTextFields[i]->setFloatFormat(GLMotif::TextField::FIXED);
		forwardTextFields[i]->setValue(forwardDirection[i]);
		}
	
	for(int i=1;i<4;++i)
		forwardPane->setColumnWeight(i,1.0f);
	forwardPane->manageChild();
	
	forwardPaneMargin->manageChild();
	
	/* Create the boundary polygon setup page: */
	pager->setNextPageName("Boundary Polygon");
	
	GLMotif::Margin* boundaryMargin=new GLMotif::Margin("BoundaryMargin",pager,false);
	boundaryMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::RIGHT,GLMotif::Alignment::VCENTER));
	
	GLMotif::Button* boundaryResetButton=new GLMotif::Button("BoundaryResetButton",boundaryMargin,"Reset");
	boundaryResetButton->getSelectCallbacks().add(this,&RoomSetup::boundaryResetButtonCallback);
	
	boundaryMargin->manageChild();
	
	pager->setCurrentChildIndex(0);
	pager->manageChild();
	
	GLMotif::Margin* buttonMargin=new GLMotif::Margin("ButtonMargin",setupDialog,false);
	buttonMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::RIGHT));
	
	GLMotif::Button* saveButton=new GLMotif::Button("SaveButton",buttonMargin,"Save Layout");
	saveButton->getSelectCallbacks().add(this,&RoomSetup::saveButtonCallback);
	
	buttonMargin->manageChild();
	
	setupDialog->manageChild();
	
	return setupDialogPopup;
	}

void RoomSetup::trackingCallback(Vrui::VRDeviceClient* client)
	{
	/* Lock and retrieve the most recent input device states: */
	deviceClient->lockState();
	const Vrui::VRDeviceState& state=deviceClient->getState();
	
	/* Extract all controller's current tracking states into a new triple buffer slot: */
	Vrui::TrackerState* tss=controllerStates.startNewValue();
	for(unsigned int i=0;i<controllers.size();++i)
		tss[i]=state.getTrackerState(controllers[i]->trackerIndex).positionOrientation;
	
	/* Check if the button state changed: */
	int newPressedButtonIndex=previousPressedButtonIndex;
	if(newPressedButtonIndex==-1)
		{
		/* Check if any controller buttons are pressed: */
		for(unsigned int i=0;i<controllers.size();++i)
			{
			for(int j=0;j<controllers[i]->numButtons;++j)
				{
				int buttonIndex=controllers[i]->buttonIndices[j];
				if(state.getButtonState(buttonIndex))
					newPressedButtonIndex=buttonIndex;
				}
			}
		}
	else
		{
		/* Check if the previous pressed button is still pressed: */
		if(!state.getButtonState(newPressedButtonIndex))
			newPressedButtonIndex=-1;
		}
	if(previousPressedButtonIndex!=newPressedButtonIndex)
		{
		pressedButtonIndex.postNewValue(newPressedButtonIndex);
		previousPressedButtonIndex=newPressedButtonIndex;
		}
	
	/* Release input device state lock: */
	deviceClient->unlockState();
	
	/* Post the new controller states and wake up the main thread: */
	controllerStates.postNewValue();
	Vrui::requestUpdate();
	}

RoomSetup::RoomSetup(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 deviceClient(0),
	 customProbeTip(Point::origin),
	 mode(Floor),
	 setupDialogPopup(0)
	{
	/* Parse command line: */
	const char* serverName="localhost:8555";
	const char* rootSectionNameStr=0;
	bool haveCustomProbeTip=false;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"server")==0)
				{
				++i;
				if(i<argc)
					serverName=argv[i];
				}
			else if(strcasecmp(argv[i]+1,"probe")==0)
				{
				haveCustomProbeTip=true;
				for(int j=0;j<3;++j)
					{
					++i;
					customProbeTip[j]=Vrui::Scalar(atof(argv[i]));
					}
				std::cout<<"Custom probe tip position: "<<customProbeTip[0]<<", "<<customProbeTip[1]<<", "<<customProbeTip[2]<<std::endl;
				}
			}
		else if(rootSectionNameStr==0)
			rootSectionNameStr=argv[i];
		}
	if(rootSectionNameStr==0)
		throw std::runtime_error("RoomSetup::RoomSetup: No root section name provided");
	rootSectionName=rootSectionNameStr;
	
	/* Split the server name into hostname:port: */
	const char* colonPtr=0;
	for(const char* cPtr=serverName;*cPtr!='\0';++cPtr)
		if(*cPtr==':')
			colonPtr=cPtr;
	int portNumber=0;
	if(colonPtr!=0)
		portNumber=atoi(colonPtr+1);
	std::string hostName=colonPtr!=0?std::string(serverName,colonPtr):std::string(serverName);
	
	/* Initialize device client: */
	deviceClient=new Vrui::VRDeviceClient(hostName.c_str(),portNumber);
	
	/* Query a list of virtual devices that have buttons: */
	for(int i=0;i<deviceClient->getNumVirtualDevices();++i)
		{
		/* Store the device as a controller if it has position and direction tracking and at least one button: */
		const Vrui::VRDeviceDescriptor* device=&deviceClient->getVirtualDevice(i);
		if((device->trackType&Vrui::VRDeviceDescriptor::TRACK_POS)&&(device->trackType&Vrui::VRDeviceDescriptor::TRACK_DIR)&&device->numButtons>0)
			controllers.push_back(device);
		}
	
	/* Initialize the probe tip location: */
	probeTip=customProbeTip;

	/* Open the system-wide Vrui configuration file: */
	std::string systemConfigFileName=VRUI_INTERNAL_CONFIG_SYSCONFIGDIR;
	systemConfigFileName.push_back('/');
	systemConfigFileName.append(VRUI_INTERNAL_CONFIG_CONFIGFILENAME);
	systemConfigFileName.append(VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX);
	Misc::ConfigurationFile configFile(systemConfigFileName.c_str());
	
	#if VRUI_INTERNAL_CONFIG_HAVE_USERCONFIGFILE
	/* Merge per-user Vrui configuration file, if it exists: */
	try
		{
		const char* home=getenv("HOME");
		if(home!=0&&home[0]!='\0')
			{
			std::string userConfigFileName=home;
			userConfigFileName.push_back('/');
			userConfigFileName.append(VRUI_INTERNAL_CONFIG_USERCONFIGDIR);
			userConfigFileName.push_back('/');
			userConfigFileName.append(VRUI_INTERNAL_CONFIG_CONFIGFILENAME);
			userConfigFileName.append(VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX);
			configFile.merge(userConfigFileName.c_str());
			}
		}
	catch(std::runtime_error)
		{
		/* Ignore error and carry on... */
		}
	#endif
	
	/* Determine the length of one meter in the physical space unit used in the configuration file: */
	Misc::ConfigurationFileSection rootSection=configFile.getSection("Vrui");
	rootSection.setSection(rootSectionNameStr);
	meterScale=rootSection.retrieveValue<Vrui::Scalar>("./inchScale",Vrui::Scalar(1))/Vrui::Scalar(0.0254);
	meterScale=rootSection.retrieveValue<Vrui::Scalar>("./meterScale",meterScale);
	
	/* Extract current environment layout parameters from the configuration file: */
	displayCenter=initialDisplayCenter=rootSection.retrieveValue<Vrui::Point>("./displayCenter");
	displaySize=initialDisplaySize=rootSection.retrieveValue<Vrui::Scalar>("./displaySize");
	forwardDirection=Geometry::normalize(rootSection.retrieveValue<Vrui::Vector>("./forwardDirection"));
	upDirection=initialUpDirection=Geometry::normalize(rootSection.retrieveValue<Vrui::Vector>("./upDirection"));
	floorPlane=rootSection.retrieveValue<Vrui::Plane>("./floorPlane");
	floorPlane.normalize();
	
	/* Project environment to the floor: */
	displayCenter+=upDirection*((floorPlane.getOffset()-displayCenter*floorPlane.getNormal())/(upDirection*floorPlane.getNormal()));
	centerHeight=Geometry::dist(displayCenter,initialDisplayCenter);
	initialDisplayCenter=displayCenter;
	
	/* Read the list of screen protector areas: */
	typedef std::vector<Vrui::Point> Polygon;
	typedef std::vector<Polygon> Boundary;
	Boundary screenProtectorAreas=rootSection.retrieveValue<Boundary>("./screenProtectorAreas",Boundary());
	
	/* Find the floor polygon in the list of areas: */
	Vrui::Scalar floorTolerance=Vrui::Scalar(0.01)*meterScale; // 1cm expressed in environment physical units
	for(Boundary::iterator spaIt=screenProtectorAreas.begin();spaIt!=screenProtectorAreas.end();++spaIt)
		{
		bool isFloor=true;
		for(Polygon::iterator pIt=spaIt->begin();isFloor&&pIt!=spaIt->end();++pIt)
			isFloor=floorPlane.calcDistance(*pIt)<floorTolerance;
		
		if(isFloor)
			{
			/* Create the initial boundary polygon: */
			boundaryVertices=*spaIt;
			break;
			}
		}
	
	/* Initialize interaction state: */
	for(int i=0;i<3;++i)
		controllerStates.getBuffer(i)=new Vrui::TrackerState[controllers.size()];
	previousPressedButtonIndex=-1;
	
	/* Create and show the setup dialog: */
	setupDialogPopup=createSetupDialog(haveCustomProbeTip);
	Vrui::popupPrimaryWidget(setupDialogPopup);
	
	/* Set up Vrui's navigation-space coordinate unit: */
	if(Math::abs(meterScale-Vrui::Scalar(1.0))<Vrui::Scalar(0.001))
		Vrui::getCoordinateManager()->setUnit(Geometry::LinearUnit(Geometry::LinearUnit::METER,1));
	else if(Math::abs(meterScale-Vrui::Scalar(1000.0/25.4))<Vrui::Scalar(1.0/25.4))
		Vrui::getCoordinateManager()->setUnit(Geometry::LinearUnit(Geometry::LinearUnit::INCH,1));
	
	/* Activate the device client and start streaming: */
	deviceClient->activate();
	deviceClient->startStream(Misc::createFunctionCall(this,&RoomSetup::trackingCallback));
	}

RoomSetup::~RoomSetup(void)
	{
	/* Stop streaming and deactivate the device client: */
	deviceClient->stopStream();
	deviceClient->deactivate();
	
	/* Destroy the setup dialog: */
	delete setupDialogPopup;
	
	/* Clean up: */
	for(int i=0;i<3;++i)
		delete[] controllerStates.getBuffer(i);
	delete deviceClient;
	}

void RoomSetup::frame(void)
	{
	/* Lock the most recent controller state: */
	controllerStates.lockNewValue();
	
	/* Check if a new button was pressed: */
	if(pressedButtonIndex.lockNewValue())
		{
		if(pressedButtonIndex.getLockedValue()>=0)
			{
			switch(mode)
				{
				case Controller:
					// Do nothing yet...
					break;
				
				case Floor:
				case Boundary:
					/* Reset the point combiner: */
					pointCombiner.reset();
					break;
				
				case Forward:
					/* Reset the vector combiner: */
					vectorCombiner=Vector::zero;
					break;
				}
			}
		else
			{
			switch(mode)
				{
				case Controller:
					// Do nothing yet...
					break;
				
				case Floor:
					/* Add the sampled point to the floor points set: */
					floorPoints.push_back(pointCombiner.getPoint());
					
					/* Set the display center: */
					displayCenter=floorPoints.front();
					for(int i=0;i<3;++i)
						centerTextFields[i]->setValue(displayCenter[i]);
					
					/* Update the up direction if three or more floor points were captured: */
					if(floorPoints.size()>=3)
						{
						/* Calculate the floor plane via principal component analysis: */
						Geometry::PCACalculator<3> pca;
						for(std::vector<Vrui::Point>::iterator fIt=floorPoints.begin();fIt!=floorPoints.end();++fIt)
							pca.accumulatePoint(*fIt);
						
						pca.calcCovariance();
						double evs[3];
						pca.calcEigenvalues(evs);
						upDirection=Geometry::normalize(Vrui::Vector(pca.calcEigenvector(evs[2])));
						if(upDirection*initialUpDirection<Vrui::Scalar(0))
							upDirection=-upDirection;
						
						for(int i=0;i<3;++i)
							upTextFields[i]->setValue(upDirection[i]);
						}
					
					/* Update the floor plane: */
					floorPlane=Vrui::Plane(upDirection,displayCenter);
					
					resetNavigation();
					break;
				
				case Forward:
					/* Set the forward direction: */
					forwardDirection=Geometry::normalize(project(vectorCombiner));
					for(int i=0;i<3;++i)
						forwardTextFields[i]->setValue(forwardDirection[i]);
					
					resetNavigation();
					break;
				
				case Boundary:
					/* Add the sampled point to the boundary polygon: */
					boundaryVertices.push_back(pointCombiner.getPoint());
					break;
				}
			}
		}
	if(pressedButtonIndex.getLockedValue()>=0)
		{
		/* Find the controller to which this button belongs: */
		for(unsigned int i=0;i<controllers.size();++i)
			for(int j=0;j<controllers[i]->numButtons;++j)
				if(controllers[i]->buttonIndices[j]==pressedButtonIndex.getLockedValue())
					{
					/* Sample the controller whose button is pressed depending on setup mode: */
					switch(mode)
						{
						case Controller:
							// Do nothing yet...
							break;
						
						case Floor:
						case Boundary:
							/* Accumulate the new controller position: */
							pointCombiner.addPoint(controllerStates.getLockedValue()[i].transform(probeTip));
							break;
						
						case Forward:
							/* Accumulate the controller's pointing direction: */
							vectorCombiner+=controllerStates.getLockedValue()[i].transform(controllers[i]->rayDirection);
							break;
						}
					
					goto foundController;
					}
		
		foundController:
		;
		}
	}

void RoomSetup::display(GLContextData& contextData) const
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT|GL_POINT_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(3.0f);
	glPointSize(7.0f);
	
	glColor(Vrui::getForegroundColor());
	
	/* Set up the floor coordinate system: */
	glPushMatrix();
	Vrui::Vector x=Geometry::normalize(forwardDirection^upDirection);
	Vrui::Vector y=Geometry::normalize(upDirection^x);
	glTranslate(displayCenter-Vrui::Point::origin);
	glRotate(Vrui::Rotation::fromBaseVectors(x,y));
	
	Vrui::Scalar size=Vrui::Scalar(Vrui::getUiSize())*displaySize*Vrui::Scalar(2)/Vrui::getDisplaySize();
	
	/* Draw the display center: */
	glBegin(GL_LINES);
	glVertex2d(-size*2.0,-size*2.0);
	glVertex2d(size*2.0,size*2.0);
	glVertex2d(-size*2.0,size*2.0);
	glVertex2d(size*2.0,-size*2.0);
	glEnd();
	
	/* Draw the display area: */
	glBegin(GL_LINE_LOOP);
	for(int i=0;i<64;++i)
		{
		Vrui::Scalar angle=Vrui::Scalar(2)*Vrui::Scalar(i)*Math::Constants<Vrui::Scalar>::pi/Vrui::Scalar(64);
		glVertex2d(Math::cos(angle)*displaySize,Math::sin(angle)*displaySize);
		}
	glEnd();
	
	/* Draw the forward direction: */
	glBegin(GL_LINE_LOOP);
	glVertex2d(size,0.0);
	glVertex2d(size,displaySize*0.5);
	glVertex2d(size*2.0,displaySize*0.5);
	glVertex2d(0.0,displaySize*0.5+size*2.0);
	glVertex2d(-size*2.0,displaySize*0.5);
	glVertex2d(-size,displaySize*0.5);
	glVertex2d(-size,0.0);
	glEnd();
	
	glPopMatrix();
	
	/* Draw the current boundary polygon: */
	if(boundaryVertices.size()>1)
		{
		glBegin(GL_LINE_LOOP);
		for(std::vector<Vrui::Point>::const_iterator bvIt=boundaryVertices.begin();bvIt!=boundaryVertices.end();++bvIt)
			glVertex(project(*bvIt));
		glEnd();
		}
	else if(boundaryVertices.size()==1)
		{
		glBegin(GL_POINTS);
		glVertex(project(boundaryVertices.front()));
		glEnd();
		}
	
	/* Display the current controller positions: */
	glBegin(GL_POINTS);
	const Vrui::TrackerState* tss=controllerStates.getLockedValue();
	for(unsigned int i=0;i<controllers.size();++i)
		glVertex(project(tss[i].transform(probeTip)));
	glEnd();
	
	/* Reset OpenGL state: */
	glPopAttrib();
	}

void RoomSetup::resetNavigation(void)
	{
	/* Align the environment display: */
	Vrui::NavTransform nav;
	nav=Vrui::NavTransform::translateFromOriginTo(Vrui::getDisplayCenter());
	Vrui::Vector vruiRight=Geometry::normalize(Vrui::getForwardDirection()^Vrui::getUpDirection());
	Vrui::Rotation vruiBase=Vrui::Rotation::fromBaseVectors(vruiRight,Vrui::getUpDirection());
	Vrui::Vector right=Geometry::normalize(forwardDirection^upDirection);
	Vrui::Rotation base=Vrui::Rotation::fromBaseVectors(right,forwardDirection);
	nav*=Vrui::NavTransform::rotate(vruiBase*Geometry::invert(base));
	nav*=Vrui::NavTransform::scale(Vrui::getDisplaySize()/(displaySize*Vrui::Scalar(2)));
	nav*=Vrui::NavTransform::translateToOriginFrom(displayCenter);
	Vrui::setNavigationTransformation(nav);
	}

VRUI_APPLICATION_RUN(RoomSetup)
