/***********************************************************************
MeasurementTool - Tool to measure positions, distances and angles in
physical or navigational coordinates.
Copyright (c) 2006-2009 Oliver Kreylos

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

#include <stdio.h>
#include <Misc/File.h>
#include <Misc/CreateNumberedFileName.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/Blind.h>
#include <GLMotif/Separator.h>
#include <GLMotif/Label.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/TextField.h>
#include <GLMotif/RowColumn.h>
#include <Vrui/CoordinateTransform.h>
#include <Vrui/ToolManager.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/MeasurementTool.h>

namespace Misc {

/****************************************
Helper class to decode measurement modes:
****************************************/

template <>
class ValueCoder<Vrui::MeasurementToolFactory::MeasurementMode>
	{
	/* Methods: */
	public:
	static std::string encode(const Vrui::MeasurementToolFactory::MeasurementMode& value)
		{
		switch(value)
			{
			case Vrui::MeasurementToolFactory::POSITION:
				return "Position";
			
			case Vrui::MeasurementToolFactory::DISTANCE:
				return "Distance";
			
			case Vrui::MeasurementToolFactory::ANGLE:
				return "Angle";
			
			default:
				return ""; // Never reached; just to make compiler happy
			}
		}
	static Vrui::MeasurementToolFactory::MeasurementMode decode(const char* start,const char* end,const char** decodeEnd =0)
		{
		if(end-start>=8&&strncasecmp(start,"Position",8)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+8;
			return Vrui::MeasurementToolFactory::POSITION;
			}
		else if(end-start>=8&&strncasecmp(start,"Distance",8)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+8;
			return Vrui::MeasurementToolFactory::DISTANCE;
			}
		else if(end-start>=5&&strncasecmp(start,"Angle",5)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+5;
			return Vrui::MeasurementToolFactory::ANGLE;
			}
		else
			throw DecodingError(std::string("Unable to convert \"")+std::string(start,end)+std::string("\" to MeasurementToolFactory::MeasurementMode"));
		}
	};

/***************************************
Helper class to decode coordinate modes:
***************************************/

template <>
class ValueCoder<Vrui::MeasurementToolFactory::CoordinateMode>
	{
	/* Methods: */
	public:
	static std::string encode(const Vrui::MeasurementToolFactory::CoordinateMode& value)
		{
		switch(value)
			{
			case Vrui::MeasurementToolFactory::PHYSICAL:
				return "Physical";
			
			case Vrui::MeasurementToolFactory::NAVIGATIONAL:
				return "Navigational";
			
			case Vrui::MeasurementToolFactory::USER:
				return "User";
			
			default:
				return ""; // Never reached; just to make compiler happy
			}
		}
	static Vrui::MeasurementToolFactory::CoordinateMode decode(const char* start,const char* end,const char** decodeEnd =0)
		{
		if(end-start>=8&&strncasecmp(start,"Physical",8)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+8;
			return Vrui::MeasurementToolFactory::PHYSICAL;
			}
		else if(end-start>=12&&strncasecmp(start,"Navigational",12)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+12;
			return Vrui::MeasurementToolFactory::NAVIGATIONAL;
			}
		else if(end-start>=4&&strncasecmp(start,"User",4)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+4;
			return Vrui::MeasurementToolFactory::USER;
			}
		else
			throw DecodingError(std::string("Unable to convert \"")+std::string(start,end)+std::string("\" to MeasurementToolFactory::CoordinateMode"));
		}
	};

}

namespace Vrui {

/***************************************
Methods of class MeasurementToolFactory:
***************************************/

MeasurementToolFactory::MeasurementToolFactory(ToolManager& toolManager)
	:ToolFactory("MeasurementTool",toolManager),
	 defaultMeasurementMode(POSITION),
	 defaultCoordinateMode(USER),
	 markerSize(getUiSize()),
	 saveMeasurements(false),
	 measurementFileName("MeasurementTool.dat"),
	 measurementFile(0)
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("UtilityTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	defaultMeasurementMode=cfs.retrieveValue<MeasurementMode>("./defaultMeasurementMode",defaultMeasurementMode);
	defaultCoordinateMode=cfs.retrieveValue<CoordinateMode>("./defaultCoordinateMode",defaultCoordinateMode);
	markerSize=cfs.retrieveValue<Scalar>("./markerSize",markerSize);
	saveMeasurements=cfs.retrieveValue<bool>("./saveMeasurements",saveMeasurements);
	measurementFileName=cfs.retrieveString("./measurementFileName",measurementFileName);
	
	/* Set tool class' factory pointer: */
	MeasurementTool::factory=this;
	}

MeasurementToolFactory::~MeasurementToolFactory(void)
	{
	/* Close the measurement file: */
	delete measurementFile;
	
	/* Reset tool class' factory pointer: */
	MeasurementTool::factory=0;
	}

const char* MeasurementToolFactory::getName(void) const
	{
	return "Measurement Tool";
	}

Tool* MeasurementToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new MeasurementTool(this,inputAssignment);
	}

void MeasurementToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveMeasurementToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("UtilityTool");
	}

extern "C" ToolFactory* createMeasurementToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	MeasurementToolFactory* measurementToolFactory=new MeasurementToolFactory(*toolManager);
	
	/* Return factory object: */
	return measurementToolFactory;
	}

extern "C" void destroyMeasurementToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/****************************************
Static elements of class MeasurementTool:
****************************************/

MeasurementToolFactory* MeasurementTool::factory=0;

/********************************
Methods of class MeasurementTool:
********************************/

void MeasurementTool::resetTool(void)
	{
	/* Reset the measurement state: */
	numPoints=0;
	
	/* Clear all coordinate fields: */
	for(int i=0;i<3;++i)
		for(int j=0;j<3;++j)
			pos[i][j]->setLabel("");
	for(int i=0;i<2;++i)
		dist[i]->setLabel("");
	angle->setLabel("");
	}

void MeasurementTool::changeMeasurementModeCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData)
	{
	/* Change the measurement mode: */
	switch(cbData->radioBox->getToggleIndex(cbData->newSelectedToggle))
		{
		case 0:
			measurementMode=MeasurementToolFactory::POSITION;
			numMeasurementPoints=1;
			break;
		
		case 1:
			measurementMode=MeasurementToolFactory::DISTANCE;
			numMeasurementPoints=2;
			break;
		
		case 2:
			measurementMode=MeasurementToolFactory::ANGLE;
			numMeasurementPoints=3;
			break;
		}
	
	/* Reset the tool: */
	resetTool();
	}

void MeasurementTool::changeCoordinateModeCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData)
	{
	/* Change the coordinate mode: */
	switch(cbData->radioBox->getToggleIndex(cbData->newSelectedToggle))
		{
		case 0:
			coordinateMode=MeasurementToolFactory::PHYSICAL;
			break;
		
		case 1:
			coordinateMode=MeasurementToolFactory::NAVIGATIONAL;
			break;
		
		case 2:
			if(userTransform!=0)
				coordinateMode=MeasurementToolFactory::USER;
			else
				coordinateMode=MeasurementToolFactory::NAVIGATIONAL;
			break;
		}
	
	/* Reset the tool: */
	resetTool();
	}

void MeasurementTool::coordTransformChangedCallback(CoordinateManager::CoordinateTransformChangedCallbackData* cbData)
	{
	/* Update the measurement dialog: */
	if(userTransform==0&&cbData->newTransform!=0)
		{
		/* Create a new toggle to select user coordinate mode: */
		coordinateModes->addToggle("User");
		}
	else if(userTransform!=0&&cbData->newTransform==0)
		{
		/* Go back to navigational coordinate mode if in user mode: */
		if(coordinateMode==MeasurementToolFactory::USER)
			{
			coordinateMode=MeasurementToolFactory::NAVIGATIONAL;
			coordinateModes->setSelectedToggle(1);
			}
		
		/* Remove the user coordinate mode toggle: */
		coordinateModes->removeWidgets(2);
		}
	
	/* Update the user transformation: */
	userTransform=cbData->newTransform;
	
	/* Reset the tool: */
	resetTool();
	}

void MeasurementTool::printPosition(Misc::File& file,const Point& pos) const
	{
	if(coordinateMode==MeasurementToolFactory::USER)
		{
		Point printPos=userTransform->transform(pos);
		fprintf(file.getFilePtr()," (%12.6g, %12.6g, %12.6g)\n",printPos[0],printPos[1],printPos[2]);
		}
	else
		fprintf(file.getFilePtr()," (%12.6g, %12.6g, %12.6g)\n",pos[0],pos[1],pos[2]);
	}

MeasurementTool::MeasurementTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:UtilityTool(sFactory,inputAssignment),
	 measurementDialogPopup(0),
	 coordinateModes(0),
	 angle(0),
	 measurementMode(factory->defaultMeasurementMode),
	 coordinateMode(factory->defaultCoordinateMode),
	 userTransform(getCoordinateManager()->getCoordinateTransform()),
	 numPoints(0),dragging(false)
	{
	for(int i=0;i<3;++i)
		for(int j=0;j<3;++j)
			pos[i][j]=0;
	for(int i=0;i<2;++i)
		dist[i]=0;
	
	/* Don't use user coordinate mode if there are no user coordinates: */
	if(coordinateMode==MeasurementToolFactory::USER&&userTransform==0)
		coordinateMode=MeasurementToolFactory::NAVIGATIONAL;
	
	/* Create the measurement dialog window: */
	measurementDialogPopup=new GLMotif::PopupWindow("MeasurementDialogPopup",getWidgetManager(),"Measurement Dialog");
	measurementDialogPopup->setResizableFlags(true,false);
	
	GLMotif::RowColumn* measurementDialog=new GLMotif::RowColumn("MeasurementDialog",measurementDialogPopup,false);
	
	GLMotif::RowColumn* modeBox=new GLMotif::RowColumn("ModeBox",measurementDialog,false);
	modeBox->setNumMinorWidgets(2);
	
	new GLMotif::Label("MeasurementMode",modeBox,"Measurement Mode");
	
	GLMotif::RadioBox* measurementModes=new GLMotif::RadioBox("MeasurementModes",modeBox,false);
	measurementModes->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	measurementModes->setPacking(GLMotif::RowColumn::PACK_GRID);
	measurementModes->setAlignment(GLMotif::Alignment::LEFT);
	measurementModes->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
	
	measurementModes->addToggle("Position");
	measurementModes->addToggle("Distance");
	measurementModes->addToggle("Angle");
	
	switch(measurementMode)
		{
		case MeasurementToolFactory::POSITION:
			measurementModes->setSelectedToggle(0);
			numMeasurementPoints=1;
			break;
		
		case MeasurementToolFactory::DISTANCE:
			measurementModes->setSelectedToggle(1);
			numMeasurementPoints=2;
			break;
		
		case MeasurementToolFactory::ANGLE:
			measurementModes->setSelectedToggle(2);
			numMeasurementPoints=3;
			break;
		}
	measurementModes->getValueChangedCallbacks().add(this,&MeasurementTool::changeMeasurementModeCallback);
	measurementModes->manageChild();
	
	new GLMotif::Label("CoordinateMode",modeBox,"Coordinate Mode");
	
	coordinateModes=new GLMotif::RadioBox("CoordinateModes",modeBox,false);
	coordinateModes->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	coordinateModes->setPacking(GLMotif::RowColumn::PACK_GRID);
	coordinateModes->setAlignment(GLMotif::Alignment::LEFT);
	coordinateModes->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
	
	coordinateModes->addToggle("Physical");
	coordinateModes->addToggle("Navigational");
	if(userTransform!=0)
		coordinateModes->addToggle("User");
	
	switch(coordinateMode)
		{
		case MeasurementToolFactory::PHYSICAL:
			coordinateModes->setSelectedToggle(0);
			break;
		
		case MeasurementToolFactory::NAVIGATIONAL:
			coordinateModes->setSelectedToggle(1);
			break;
		
		case MeasurementToolFactory::USER:
			coordinateModes->setSelectedToggle(2);
			break;
		}
	coordinateModes->getValueChangedCallbacks().add(this,&MeasurementTool::changeCoordinateModeCallback);
	coordinateModes->manageChild();
	
	modeBox->manageChild();
	
	new GLMotif::Separator("Separator1",measurementDialog,GLMotif::Separator::HORIZONTAL,0.0f,GLMotif::Separator::LOWERED);
	
	GLMotif::RowColumn* measurementBox=new GLMotif::RowColumn("MeasurementBox",measurementDialog,false);
	measurementBox->setNumMinorWidgets(2);
	
	new GLMotif::Label("Pos1Label",measurementBox,"Position 1");
	
	GLMotif::RowColumn* pos1Box=new GLMotif::RowColumn("Pos1Box",measurementBox,false);
	pos1Box->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	pos1Box->setPacking(GLMotif::RowColumn::PACK_GRID);
	
	for(int i=0;i<3;++i)
		{
		char labelName[40];
		snprintf(labelName,sizeof(labelName),"Pos1-%d",i+1);
		pos[0][i]=new GLMotif::TextField(labelName,pos1Box,12);
		pos[0][i]->setPrecision(6);
		}
	
	pos1Box->manageChild();
	
	new GLMotif::Label("Pos2Label",measurementBox,"Position 2");
	
	GLMotif::RowColumn* pos2Box=new GLMotif::RowColumn("Pos2Box",measurementBox,false);
	pos2Box->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	pos2Box->setPacking(GLMotif::RowColumn::PACK_GRID);
	
	for(int i=0;i<3;++i)
		{
		char labelName[40];
		snprintf(labelName,sizeof(labelName),"Pos2-%d",i+1);
		pos[1][i]=new GLMotif::TextField(labelName,pos2Box,12);
		pos[1][i]->setPrecision(6);
		}
	
	pos2Box->manageChild();
	
	new GLMotif::Label("Dist1Label",measurementBox,"Distance 1");
	
	GLMotif::RowColumn* dist1Box=new GLMotif::RowColumn("Dist1Box",measurementBox,false);
	dist1Box->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	dist1Box->setPacking(GLMotif::RowColumn::PACK_GRID);
	
	dist[0]=new GLMotif::TextField("Dist1",dist1Box,16);
	dist[0]->setPrecision(10);
	
	new GLMotif::Blind("Blind",dist1Box);
	
	dist1Box->manageChild();
	
	new GLMotif::Label("Pos3Label",measurementBox,"Position 3");
	
	GLMotif::RowColumn* pos3Box=new GLMotif::RowColumn("Pos3Box",measurementBox,false);
	pos3Box->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	pos3Box->setPacking(GLMotif::RowColumn::PACK_GRID);
	
	for(int i=0;i<3;++i)
		{
		char labelName[40];
		snprintf(labelName,sizeof(labelName),"Pos3-%d",i+1);
		pos[2][i]=new GLMotif::TextField(labelName,pos3Box,12);
		pos[2][i]->setPrecision(6);
		}
	
	pos3Box->manageChild();
	
	new GLMotif::Label("Dist2Label",measurementBox,"Distance 2");
	
	GLMotif::RowColumn* dist2Box=new GLMotif::RowColumn("Dist2Box",measurementBox,false);
	dist2Box->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	dist2Box->setPacking(GLMotif::RowColumn::PACK_GRID);
	
	dist[1]=new GLMotif::TextField("Dist2",dist2Box,16);
	dist[1]->setPrecision(10);
	
	new GLMotif::Blind("Blind",dist2Box);
	
	dist2Box->manageChild();
	
	new GLMotif::Label("AngleLabel",measurementBox,"Angle");
	
	GLMotif::RowColumn* angleBox=new GLMotif::RowColumn("AngleBox",measurementBox,false);
	angleBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	angleBox->setPacking(GLMotif::RowColumn::PACK_GRID);
	
	angle=new GLMotif::TextField("Angle",angleBox,16);
	angle->setPrecision(10);
	
	new GLMotif::Blind("Blind",angleBox);
	
	angleBox->manageChild();
	
	measurementBox->manageChild();
	
	measurementDialog->manageChild();
	
	/* Initialize the tool's state: */
	resetTool();
	
	/* Pop up the measurement dialog: */
	popupPrimaryWidget(measurementDialogPopup,getNavigationTransformation().transform(getDisplayCenter()));
	
	/* Register a callback with the coordinate manager: */
	getCoordinateManager()->getCoordinateTransformChangedCallbacks().add(this,&MeasurementTool::coordTransformChangedCallback);
	}

MeasurementTool::~MeasurementTool(void)
	{
	/* Unregister the callback from the coordinate manager: */
	getCoordinateManager()->getCoordinateTransformChangedCallbacks().remove(this,&MeasurementTool::coordTransformChangedCallback);
	
	/* Delete the measurement dialog: */
	delete measurementDialogPopup;
	}

const ToolFactory* MeasurementTool::getFactory(void) const
	{
	return factory;
	}

void MeasurementTool::buttonCallback(int,int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Go to the next measurement point: */
		if(numPoints==numMeasurementPoints)
			{
			/* Reset the tool: */
			resetTool();
			}
		++numPoints;
		
		/* Start dragging the current measurement point: */
		dragging=true;
		}
	else // Button has just been released
		{
		/* Stop dragging the current measurement point: */
		dragging=false;
		
		/* Check if a measurement entry has to be written to the measurement file: */
		if(factory->saveMeasurements&&numPoints==numMeasurementPoints&&isMaster())
			{
			/* Ensure the measurement file is open: */
			if(factory->measurementFile==0)
				{
				try
					{
					/* Create a uniquely named file based on the base name: */
					char numberedFileName[1024];
					factory->measurementFile=new Misc::File(Misc::createNumberedFileName(factory->measurementFileName.c_str(),4,numberedFileName),"wt");
					}
				catch(Misc::File::OpenError)
					{
					/* Just don't open the file, then! */
					}
				}
			
			/* Write the measurement to file if there is a file: */
			if(factory->measurementFile!=0)
				{
				switch(coordinateMode)
					{
					case MeasurementToolFactory::PHYSICAL:
						fprintf(factory->measurementFile->getFilePtr(),"Physical");
						break;
					
					case MeasurementToolFactory::NAVIGATIONAL:
						fprintf(factory->measurementFile->getFilePtr(),"Navigational");
						break;
					
					case MeasurementToolFactory::USER:
						fprintf(factory->measurementFile->getFilePtr(),"User");
						break;
					}
				
				switch(measurementMode)
					{
					case MeasurementToolFactory::POSITION:
						fprintf(factory->measurementFile->getFilePtr()," position");
						printPosition(*factory->measurementFile,points[0]);
						break;
					
					case MeasurementToolFactory::DISTANCE:
						fprintf(factory->measurementFile->getFilePtr()," distance");
						printPosition(*factory->measurementFile,points[0]);
						printPosition(*factory->measurementFile,points[1]);
						fprintf(factory->measurementFile->getFilePtr()," %16.10g\n",Geometry::dist(points[0],points[1]));
						break;
					
					case MeasurementToolFactory::ANGLE:
						{
						fprintf(factory->measurementFile->getFilePtr()," angle   ");
						printPosition(*factory->measurementFile,points[0]);
						printPosition(*factory->measurementFile,points[1]);
						Vector d1=points[1]-points[0];
						Scalar d1Len=Geometry::mag(d1);
						fprintf(factory->measurementFile->getFilePtr()," %16.10g",d1Len);
						printPosition(*factory->measurementFile,points[2]);
						Vector d2=points[2]-points[0];
						Scalar d2Len=Geometry::mag(d2);
						fprintf(factory->measurementFile->getFilePtr()," %16.10g",d2Len);
						Scalar angleValue=(d1*d2)/(d1Len*d2Len);
						if(angleValue<=Scalar(-1))
							angleValue=Scalar(180);
						else if(angleValue>=Scalar(1))
							angleValue=Scalar(0);
						else
							angleValue=Math::deg(Math::acos(angleValue));
						fprintf(factory->measurementFile->getFilePtr()," %16.10g\n",angleValue);
						break;
						}
					}
				}
			}
		}
	}

void MeasurementTool::frame(void)
	{
	if(dragging)
		{
		/* Calculate the device position in the appropriate coordinate system: */
		points[numPoints-1]=getDevicePosition(0);
		if(coordinateMode==MeasurementToolFactory::NAVIGATIONAL||coordinateMode==MeasurementToolFactory::USER)
			points[numPoints-1]=getInverseNavigationTransformation().transform(points[numPoints-1]);
		
		/* Update the measurement dialog: */
		Point displayPoint=points[numPoints-1];
		if(coordinateMode==MeasurementToolFactory::USER)
			displayPoint=userTransform->transform(displayPoint);
		for(int j=0;j<3;++j)
			pos[numPoints-1][j]->setValue(double(displayPoint[j]));
		
		if(numPoints>=2)
			{
			/* Calculate the distance between the last and the first measurement points: */
			Scalar distValue=Geometry::dist(points[0],points[numPoints-1]);
			dist[numPoints-2]->setValue(double(distValue));
			}
		
		if(numPoints==3)
			{
			/* Calculate the angle between the three measurement points: */
			Vector d1=points[1]-points[0];
			Vector d2=points[2]-points[0];
			Scalar angleValue=(d1*d2)/(d1.mag()*d2.mag());
			if(angleValue<=Scalar(-1))
				angleValue=Scalar(180);
			else if(angleValue>=Scalar(1))
				angleValue=Scalar(0);
			else
				angleValue=Math::deg(Math::acos(angleValue));
			angle->setValue(double(angleValue));
			}
		}
	}

void MeasurementTool::display(GLContextData& contextData) const
	{
	/* Set up and save OpenGL state: */
	GLboolean lightingEnabled=glIsEnabled(GL_LIGHTING);
	if(lightingEnabled)
		glDisable(GL_LIGHTING);
	GLfloat lineWidth;
	glGetFloatv(GL_LINE_WIDTH,&lineWidth);
	glLineWidth(1.0f);
	
	/* Calculate the marker size: */
	Scalar markerSize=factory->markerSize;
	if(coordinateMode==MeasurementToolFactory::NAVIGATIONAL||coordinateMode==MeasurementToolFactory::USER)
		{
		markerSize/=getNavigationTransformation().getScaling();
		
		/* Go to navigational coordinates: */
		glPushMatrix();
		glLoadIdentity();
		glMultMatrix(getDisplayState(contextData).modelviewNavigational);
		}
	
	/* Determine the marker color: */
	Color bgColor=getBackgroundColor();
	Color fgColor;
	for(int i=0;i<3;++i)
		fgColor[i]=1.0f-bgColor[i];
	fgColor[3]=bgColor[3];
	
	glColor(fgColor);
	glBegin(GL_LINES);
	
	/* Mark all measurement points: */
	for(int i=0;i<numPoints;++i)
		{
		glVertex(points[i][0]-markerSize,points[i][1],points[i][2]);
		glVertex(points[i][0]+markerSize,points[i][1],points[i][2]);
		glVertex(points[i][0],points[i][1]-markerSize,points[i][2]);
		glVertex(points[i][0],points[i][1]+markerSize,points[i][2]);
		glVertex(points[i][0],points[i][1],points[i][2]-markerSize);
		glVertex(points[i][0],points[i][1],points[i][2]+markerSize);
		}
	
	/* Draw all distance lines: */
	for(int i=1;i<numPoints;++i)
		{
		glVertex(points[0]);
		glVertex(points[i]);
		}
	
	glEnd();
	
	/* Restore OpenGL state: */
	if(coordinateMode==MeasurementToolFactory::NAVIGATIONAL||coordinateMode==MeasurementToolFactory::USER)
		glPopMatrix();
	glLineWidth(lineWidth);
	if(lightingEnabled)
		glEnable(GL_LIGHTING);
	}

}
