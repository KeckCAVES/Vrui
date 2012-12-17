/***********************************************************************
Filming - Vislet class to assist shooting of video inside an immersive
environment by providing run-time control over viewers and environment
settings.
Copyright (c) 2012 Oliver Kreylos

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

#include <Vrui/Vislets/Filming.h>

#include <Misc/PrintInteger.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLMatrixTemplates.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/Margin.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Blind.h>
#include <GLMotif/Label.h>
#include <Vrui/InputDevice.h>
#include <Vrui/Lightsource.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRWindow.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/VisletManager.h>

namespace Vrui {

namespace Vislets {

/*******************************
Methods of class FilmingFactory:
*******************************/

FilmingFactory::FilmingFactory(VisletManager& visletManager)
	:VisletFactory("Filming",visletManager),
	 initialViewerPosition(getDisplayCenter()-getForwardDirection()*getDisplaySize())
	{
	#if 0
	/* Insert class into class hierarchy: */
	VisletFactory* visletFactory=visletManager.loadClass("Vislet");
	visletFactory->addChildClass(this);
	addParentClass(visletFactory);
	#endif
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=visletManager.getVisletClassSection(getClassName());
	initialViewerPosition=cfs.retrieveValue<Point>("./initialViewerPosition",initialViewerPosition);
	
	/* Initialize the filming tool classes: */
	Filming::MoveViewerToolFactory* moveViewerToolFactory=new Filming::MoveViewerToolFactory("FilmingMoveViewerTool","Move Filming Viewer",0,*getToolManager());
	moveViewerToolFactory->setNumValuators(3);
	moveViewerToolFactory->setValuatorFunction(0,"Move X");
	moveViewerToolFactory->setValuatorFunction(1,"Move Y");
	moveViewerToolFactory->setValuatorFunction(2,"Move Z");
	getToolManager()->addClass(moveViewerToolFactory,ToolManager::defaultToolFactoryDestructor);
	
	/* Set tool class' factory pointer: */
	Filming::factory=this;
	}

FilmingFactory::~FilmingFactory(void)
	{
	/* Reset tool class' factory pointer: */
	Filming::factory=0;
	}

Vislet* FilmingFactory::createVislet(int numArguments,const char* const arguments[]) const
	{
	return new Filming(numArguments,arguments);
	}

void FilmingFactory::destroyVislet(Vislet* vislet) const
	{
	delete vislet;
	}

extern "C" void resolveFilmingDependencies(Plugins::FactoryManager<VisletFactory>& manager)
	{
	#if 0
	/* Load base classes: */
	manager.loadClass("Vislet");
	#endif
	}

extern "C" VisletFactory* createFilmingFactory(Plugins::FactoryManager<VisletFactory>& manager)
	{
	/* Get pointer to vislet manager: */
	VisletManager* visletManager=static_cast<VisletManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	FilmingFactory* factory=new FilmingFactory(*visletManager);
	
	/* Return factory object: */
	return factory;
	}

extern "C" void destroyFilmingFactory(VisletFactory* factory)
	{
	delete factory;
	}

/************************************************
Static elements of class Filming::MoveViewerTool:
************************************************/

Filming::MoveViewerToolFactory* Filming::MoveViewerTool::factory=0;

/****************************************
Methods of class Filming::MoveViewerTool:
****************************************/

Filming::MoveViewerTool::MoveViewerTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:Vrui::Tool(sFactory,inputAssignment)
	{
	}

const Vrui::ToolFactory* Filming::MoveViewerTool::getFactory(void) const
	{
	return factory;
	}

void Filming::MoveViewerTool::frame(void)
	{
	if(vislet!=0)
		{
		/* Adjust the vislet's fixed viewer position: */
		bool changed=false;
		for(int axis=0;axis<3;++axis)
			if(getValuatorState(axis)!=0.0)
				{
				vislet->viewerPosition[axis]+=getValuatorState(axis)*getInchFactor()*getFrameTime();
				vislet->posSliders[axis]->setValue(vislet->viewerPosition[axis]);
				changed=true;
				}
		
		if(changed&&vislet->viewerDevice==0)
			{
			/* Update the filming viewer: */
			vislet->viewer->detachFromDevice(TrackerState::translateFromOriginTo(vislet->viewerPosition));
			}
		}
	}

/********************************
Static elements of class Filming:
********************************/

FilmingFactory* Filming::factory=0;

/************************
Methods of class Filming:
************************/

void Filming::viewerDeviceMenuCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData)
	{
	if(cbData->newSelectedItem==0)
		{
		/* Disable head tracking: */
		viewerDevice=0;
		viewer->detachFromDevice(TrackerState::translateFromOriginTo(viewerPosition));
		}
	else
		{
		/* Find the new viewer device: */
		viewerDevice=findInputDevice(cbData->getItem());
		if(viewerDevice!=0)
			{
			/* Enable head tracking: */
			viewer->attachToDevice(viewerDevice);
			}
		else
			{
			/* Disable head tracking: */
			viewerDevice=0;
			viewer->detachFromDevice(TrackerState::translateFromOriginTo(viewerPosition));
			cbData->dropdownBox->setSelectedItem(0);
			}
		}
	}

void Filming::posSliderCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData,const int& sliderIndex)
	{
	/* Update the filming viewer's position: */
	viewerPosition[sliderIndex]=cbData->value;
	
	if(viewerDevice==0)
		{
		/* Update the filming viewer: */
		viewer->detachFromDevice(TrackerState::translateFromOriginTo(viewerPosition));
		}
	}

void Filming::windowToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData,const int& windowIndex)
	{
	/* Select or deselect the window: */
	windowFilmings[windowIndex]=cbData->set;
	if(active&&getWindow(windowIndex)!=0)
		{
		if(windowFilmings[windowIndex])
			{
			/* Set the window's viewer to the filming viewer: */
			getWindow(windowIndex)->setViewer(viewer);
			}
		else
			{
			/* Reset the window's viewers to the original values: */
			for(int i=0;i<2;++i)
				getWindow(windowIndex)->setViewer(i,windowViewers[windowIndex*2+i]);
			}
		}
	}

void Filming::headlightToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData,const int& viewerIndex)
	{
	headlightStates[viewerIndex]=cbData->set;
	
	if(active)
		{
		/* Set the headlight to the new state: */
		if(viewerIndex==0)
			viewer->setHeadlightState(cbData->set);
		else
			getViewer(viewerIndex-1)->setHeadlightState(cbData->set);
		}
	}

void Filming::backgroundColorSelectorCallback(GLMotif::HSVColorSelector::ValueChangedCallbackData* cbData)
	{
	backgroundColor=cbData->newColor;
	
	if(active)
		{
		/* Set the environment's background color: */
		setBackgroundColor(backgroundColor);
		}
	}

void Filming::drawGridToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	drawGrid=cbData->set;
	}

void Filming::drawDevicesToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	drawDevices=cbData->set;
	}

void Filming::buildFilmingControls(void)
	{
	/* Build the graphical user interface: */
	const GLMotif::StyleSheet& ss=*getUiStyleSheet();
	
	dialogWindow=new GLMotif::PopupWindow("FilmingControlDialog",getWidgetManager(),"Filming Controls");
	dialogWindow->setHideButton(true);
	dialogWindow->setResizableFlags(true,false);
	
	GLMotif::RowColumn* filmingControls=new GLMotif::RowColumn("FilmingControls",dialogWindow,false);
	filmingControls->setOrientation(GLMotif::RowColumn::VERTICAL);
	filmingControls->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	filmingControls->setNumMinorWidgets(2);
	
	/* Create a drop-down menu to select a tracking device for the filming viewer: */
	new GLMotif::Label("ViewerDeviceLabel",filmingControls,"Viewer Device");
	GLMotif::DropdownBox* viewerDeviceMenu=new GLMotif::DropdownBox("ViewerDeviceMenu",filmingControls);
	viewerDeviceMenu->addItem("Fixed Position");
	for(int deviceIndex=0;deviceIndex<getNumInputDevices();++deviceIndex)
		if(getInputGraphManager()->isReal(getInputDevice(deviceIndex)))
			viewerDeviceMenu->addItem(getInputDevice(deviceIndex)->getDeviceName());
	viewerDeviceMenu->setSelectedItem(0);
	viewerDeviceMenu->getValueChangedCallbacks().add(this,&Filming::viewerDeviceMenuCallback);
	
	/* Create three sliders to set the filming viewer's position: */
	new GLMotif::Label("ViewerPositionLabel",filmingControls,"Viewer Position");
	GLMotif::RowColumn* viewerPositionBox=new GLMotif::RowColumn("ViewerPositionBox",filmingControls,false);
	
	for(int i=0;i<3;++i)
		{
		char psName[11]="PosSlider ";
		psName[9]=char(i+'0');
		posSliders[i]=new GLMotif::TextFieldSlider(psName,viewerPositionBox,7,ss.fontHeight*10.0f);
		posSliders[i]->getTextField()->setFieldWidth(6);
		posSliders[i]->getTextField()->setPrecision(1);
		posSliders[i]->getTextField()->setFloatFormat(GLMotif::TextField::FIXED);
		posSliders[i]->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
		posSliders[i]->setValueType(GLMotif::TextFieldSlider::FLOAT);
		posSliders[i]->setValueRange(getDisplayCenter()[i]-getDisplaySize()*Scalar(4),getDisplayCenter()[i]+getDisplaySize()*Scalar(4),0.1);
		posSliders[i]->setValue(viewerPosition[i]);
		posSliders[i]->getValueChangedCallbacks().add(this,&Filming::posSliderCallback,i);
		}
	
	viewerPositionBox->manageChild();
	
	/* Create toggle buttons to select filming windows: */
	new GLMotif::Label("WindowButtonLabel",filmingControls,"Filming Windows");
	GLMotif::RowColumn* windowButtonBox=new GLMotif::RowColumn("WindowButtonBox",filmingControls,false);
	windowButtonBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	windowButtonBox->setPacking(GLMotif::RowColumn::PACK_GRID);
	windowButtonBox->setAlignment(GLMotif::Alignment::LEFT);
	windowButtonBox->setNumMinorWidgets(1);
	
	for(int windowIndex=0;windowIndex<getNumWindows();++windowIndex)
		{
		char windowToggleName[15]="WindowToggle  ";
		char* wtnPtr=Misc::print(windowIndex,windowToggleName+14);
		while(wtnPtr>windowToggleName+12)
			*(wtnPtr--)='0';
		char windowToggleLabel[3];
		GLMotif::ToggleButton* windowToggle=new GLMotif::ToggleButton(windowToggleName,windowButtonBox,Misc::print(windowIndex+1,windowToggleLabel+2));
		windowToggle->setToggle(windowFilmings[windowIndex]);
		windowToggle->getValueChangedCallbacks().add(this,&Filming::windowToggleCallback,windowIndex);
		}
	
	windowButtonBox->manageChild();
	
	/* Create toggle buttons to toggle viewer headlights: */
	new GLMotif::Label("HeadlightButtonLabel",filmingControls,"Headlights");
	GLMotif::RowColumn* headlightButtonBox=new GLMotif::RowColumn("HeadlightButtonBox",filmingControls,false);
	headlightButtonBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	headlightButtonBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	headlightButtonBox->setAlignment(GLMotif::Alignment::LEFT);
	headlightButtonBox->setNumMinorWidgets(1);
	
	for(int viewerIndex=0;viewerIndex<getNumViewers()+1;++viewerIndex)
		{
		char headlightToggleName[18]="HeadlightToggle  ";
		char* htnPtr=Misc::print(viewerIndex,headlightToggleName+17);
		while(htnPtr>headlightToggleName+15)
			*(htnPtr--)='0';
		Viewer* v=viewerIndex==0?viewer:getViewer(viewerIndex-1);
		GLMotif::ToggleButton* headlightToggle=new GLMotif::ToggleButton(headlightToggleName,headlightButtonBox,viewerIndex==0?"FilmingViewer":v->getName());
		headlightToggle->setToggle(v->getHeadlight().isEnabled());
		headlightToggle->getValueChangedCallbacks().add(this,&Filming::headlightToggleCallback,viewerIndex);
		}
	
	headlightButtonBox->manageChild();
	
	/* Create a color selector to change the environment's background color: */
	new GLMotif::Label("BackgroundColorLabel",filmingControls,"Background Color");
	GLMotif::Margin* backgroundColorMargin=new GLMotif::Margin("BackgroundColorMargin",filmingControls,false);
	backgroundColorMargin->setAlignment(GLMotif::Alignment::LEFT);
	
	GLMotif::HSVColorSelector* backgroundColorSelector=new GLMotif::HSVColorSelector("BackgroundColorSelector",backgroundColorMargin);
	backgroundColorSelector->setPreferredSize(ss.fontHeight*4.0f);
	backgroundColorSelector->setCurrentColor(backgroundColor);
	backgroundColorSelector->getValueChangedCallbacks().add(this,&Filming::backgroundColorSelectorCallback);
	
	backgroundColorMargin->manageChild();
	
	/* Create toggles for various flags: */
	new GLMotif::Blind("ToggleBoxBlind",filmingControls);
	GLMotif::RowColumn* toggleBox=new GLMotif::RowColumn("ToggleBox",filmingControls,false);
	toggleBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	toggleBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	toggleBox->setAlignment(GLMotif::Alignment::LEFT);
	toggleBox->setNumMinorWidgets(1);
	
	GLMotif::ToggleButton* drawGridToggle=new GLMotif::ToggleButton("DrawGridToggle",toggleBox,"Draw Grid");
	drawGridToggle->setToggle(drawGrid);
	drawGridToggle->getValueChangedCallbacks().add(this,&Filming::drawGridToggleCallback);
	
	GLMotif::ToggleButton* drawDevicesToggle=new GLMotif::ToggleButton("DrawDevicesToggle",toggleBox,"Draw Devices");
	drawDevicesToggle->setToggle(drawDevices);
	drawDevicesToggle->getValueChangedCallbacks().add(this,&Filming::drawDevicesToggleCallback);
	
	toggleBox->manageChild();
	
	filmingControls->manageChild();
	}

void Filming::toolCreationCallback(ToolManager::ToolCreationCallbackData* cbData)
	{
	/* Check if the new tool is a filming tool: */
	Tool* tool=dynamic_cast<Tool*>(cbData->tool);
	if(tool!=0)
		{
		/* Associate the tool with this vislet: */
		tool->setVislet(this);
		}
	}

Filming::Filming(int numArguments,const char* const arguments[])
	:viewer(0),viewerDevice(0),viewerPosition(factory->initialViewerPosition),
	 windowViewers(0),windowFilmings(0),
	 originalHeadlightStates(0),headlightStates(0),
	 drawGrid(false),drawDevices(false),
	 dialogWindow(0)
	{
	/* Create the private filming viewer: */
	viewer=new Viewer;
	viewer->setEyes(Vector(0,1,0),Point::origin,Vector::zero);
	viewer->setHeadlightState(false);
	viewer->detachFromDevice(TrackerState::translateFromOriginTo(viewerPosition));
	
	/* Initialize the grid transformation: */
	gridTransform=ONTransform::translateFromOriginTo(getDisplayCenter());
	gridTransform*=ONTransform::rotate(Rotation::fromBaseVectors(Geometry::cross(getUpDirection(),getForwardDirection()),getUpDirection()));
	
	/* Install callbacks with the tool manager: */
	getToolManager()->getToolCreationCallbacks().add(this,&Filming::toolCreationCallback);
	}

Filming::~Filming(void)
	{
	delete dialogWindow;
	
	/* Uninstall tool manager callbacks: */
	getToolManager()->getToolCreationCallbacks().remove(this,&Filming::toolCreationCallback);
	
	delete viewer;
	delete[] windowViewers;
	delete[] windowFilmings;
	delete[] originalHeadlightStates;
	delete[] headlightStates;
	}

VisletFactory* Filming::getFactory(void) const
	{
	return factory;
	}

void Filming::disable(void)
	{
	/* Reset the viewers of all filming windows: */
	for(int windowIndex=0;windowIndex<getNumWindows();++windowIndex)
		if(windowFilmings[windowIndex]&&getWindow(windowIndex)!=0)
			{
			for(int i=0;i<2;++i)
				getWindow(windowIndex)->setViewer(i,windowViewers[windowIndex*2+i]);
			}
	
	/* Reset all viewer's headlight states: */
	viewer->setHeadlightState(false);
	for(int viewerIndex=0;viewerIndex<getNumViewers();++viewerIndex)
		getViewer(viewerIndex)->setHeadlightState(originalHeadlightStates[viewerIndex]);
	
	/* Restore the environment's background color: */
	setBackgroundColor(originalBackgroundColor);
	
	active=false;
	}

void Filming::enable(void)
	{
	/* Check if the filming control GUI needs to be created: */
	if(dialogWindow==0)
		{
		/* Initialize the filming viewer associations: */
		windowViewers=new Viewer*[getNumWindows()*2];
		windowFilmings=new bool[getNumWindows()];
		for(int windowIndex=0;windowIndex<getNumWindows();++windowIndex)
			windowFilmings[windowIndex]=true;
		
		/* Initialize the headlight state arrays: */
		originalHeadlightStates=new bool[getNumViewers()];
		for(int viewerIndex=0;viewerIndex<getNumViewers();++viewerIndex)
			originalHeadlightStates[viewerIndex]=getViewer(viewerIndex)->getHeadlight().isEnabled();
		headlightStates=new bool[getNumViewers()+1];
		headlightStates[0]=viewer->getHeadlight().isEnabled();
		for(int viewerIndex=0;viewerIndex<getNumViewers();++viewerIndex)
			headlightStates[viewerIndex+1]=originalHeadlightStates[viewerIndex];
		
		/* Save the environment's background color: */
		originalBackgroundColor=getBackgroundColor();
		backgroundColor=originalBackgroundColor;
		
		/* Build and show the GUI: */
		buildFilmingControls();
		popupPrimaryWidget(dialogWindow);
		}
	else
		{
		/* Store the viewers currently attached to each window and override the viewers of filming windows: */
		for(int windowIndex=0;windowIndex<getNumWindows();++windowIndex)
			{
			if(getWindow(windowIndex)!=0)
				{
				for(int i=0;i<2;++i)
					windowViewers[windowIndex*2+i]=getWindow(windowIndex)->getViewer(i);
				if(windowFilmings[windowIndex])
					getWindow(windowIndex)->setViewer(viewer);
				}
			else
				windowViewers[windowIndex*2+0]=windowViewers[windowIndex*2+1]=0;
			}
		
		/* Set all viewer's headlight states: */
		viewer->setHeadlightState(headlightStates[0]);
		for(int viewerIndex=0;viewerIndex<getNumViewers();++viewerIndex)
			getViewer(viewerIndex)->setHeadlightState(headlightStates[viewerIndex+1]);
		
		/* Override the environment's background color: */
		setBackgroundColor(backgroundColor);
		
		active=true;
		}
	}

void Filming::frame(void)
	{
	/* Update the filming viewer: */
	viewer->update();
	}

void Filming::display(GLContextData& contextData) const
	{
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(1.0f);
	
	if(drawGrid)
		{
		/* Draw the calibration grid: */
		glPushMatrix();
		glMultMatrix(gridTransform);
		
		float gs=float(getDisplaySize())*3.0f;
		glColor3f(1.0f,1.0f,0.0f);
		glBegin(GL_LINES);
		for(int x=-8;x<=8;++x)
			{
			glVertex3f(float(x)*gs*0.125f,-gs,0.0f);
			glVertex3f(float(x)*gs*0.125f,gs,0.0f);
			}
		for(int y=-8;y<=8;++y)
			{
			glVertex3f(-gs,float(y)*gs*0.125f,0.0f);
			glVertex3f(gs,float(y)*gs*0.125f,0.0f);
			}
		glEnd();
		
		glPopMatrix();
		}
	
	if(drawDevices)
		{
		/* Draw coordinate axes for each real device: */
		for(int i=0;i<Vrui::getNumInputDevices();++i)
			{
			Vrui::InputDevice* id=Vrui::getInputDevice(i);
			if(id->is6DOFDevice()&&getInputGraphManager()->isReal(id))
				{
				glPushMatrix();
				glMultMatrix(id->getTransformation());
				glScale(getInchFactor());
				glBegin(GL_LINES);
				glColor3f(1.0f,0.0f,0.0f);
				glVertex3f(-5.0f,0.0f,0.0f);
				glVertex3f( 5.0f,0.0f,0.0f);
				glColor3f(0.0f,1.0f,0.0f);
				glVertex3f(0.0f,-5.0f,0.0f);
				glVertex3f(0.0f, 5.0f,0.0f);
				glColor3f(0.0f,0.0f,1.0f);
				glVertex3f(0.0f,0.0f,-5.0f);
				glVertex3f(0.0f,0.0f, 5.0f);
				glEnd();
				glPopMatrix();
				}
			}
		}
	
	glPopAttrib();
	}

}

}
