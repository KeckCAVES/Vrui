/***********************************************************************
ShowEarthModel - Simple Vrui application to render a model of Earth,
with the ability to additionally display earthquake location data and
other geology-related stuff.
Copyright (c) 2005-2006 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#define CLIP_SCREEN 0

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <vector>
#include <Misc/FunctionCalls.h>
#include <Misc/File.h>
#include <Misc/ThrowStdErr.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Geoid.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLMatrixTemplates.h>
#include <GL/GLMaterial.h>
#include <GL/GLContextData.h>
#include <GL/GLModels.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLPolylineTube.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/Blind.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/CascadeButton.h>
#include <GLMotif/Menu.h>
#include <GLMotif/SubMenu.h>
#include <GLMotif/Popup.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/TextField.h>
#include <Images/RGBImage.h>
#include <Images/ReadImageFile.h>
#include <Vrui/CoordinateManager.h>
#if CLIP_SCREEN
#include <Vrui/VRScreen.h>
#endif
#include <Vrui/Tools/SurfaceNavigationTool.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>

#include "EarthFunctions.h"
#include "EarthquakeSet.h"
#include "EarthquakeTool.h"
#include "PointSet.h"
#include "SeismicPath.h"

#include "ShowEarthModel.h"

/*******************************************************************
Methods of class ShowEarthModel::RotatedGeodeticCoordinateTransform:
*******************************************************************/

ShowEarthModel::RotatedGeodeticCoordinateTransform::RotatedGeodeticCoordinateTransform(void)
	:Vrui::GeodeticCoordinateTransform(0.001),
	 rotationAngle(0),raSin(0),raCos(1)
	{
	}

Vrui::Point ShowEarthModel::RotatedGeodeticCoordinateTransform::transform(const Vrui::Point& navigationPoint) const
	{
	/* First undo the rotation: */
	Vrui::Point p;
	p[0]=raCos*navigationPoint[0]+raSin*navigationPoint[1];
	p[1]=raCos*navigationPoint[1]-raSin*navigationPoint[0];
	p[2]=navigationPoint[2];
	
	/* Then convert the point to geodetic coordinates: */
	return Vrui::GeodeticCoordinateTransform::transform(p);
	}

void ShowEarthModel::RotatedGeodeticCoordinateTransform::setRotationAngle(Vrui::Scalar newRotationAngle)
	{
	rotationAngle=newRotationAngle;
	raSin=Math::sin(Math::rad(rotationAngle));
	raCos=Math::cos(Math::rad(rotationAngle));
	}

/*****************************************
Methods of class ShowEarthModel::DataItem:
*****************************************/

ShowEarthModel::DataItem::DataItem(void)
	:hasVertexBufferObjectExtension(false), // GLARBVertexBufferObject::isSupported()),
	 surfaceVertexBufferObjectId(0),surfaceIndexBufferObjectId(0),
	 surfaceTextureObjectId(0),
	 displayListIdBase(0)
	{
	/* Use buffer objects for the Earth surface if supported: */
	if(hasVertexBufferObjectExtension)
		{
		/* Initialize the vertex buffer object extension: */
		GLARBVertexBufferObject::initExtension();
		
		/* Create vertex buffer objects: */
		GLuint bufferObjectIds[2];
		glGenBuffersARB(2,bufferObjectIds);
		surfaceVertexBufferObjectId=bufferObjectIds[0];
		surfaceIndexBufferObjectId=bufferObjectIds[1];
		}
	
	/* Generate a texture object for the Earth's surface texture: */
	glGenTextures(1,&surfaceTextureObjectId);
	
	/* Generate display lists for the Earth model components: */
	displayListIdBase=glGenLists(4);
	}

ShowEarthModel::DataItem::~DataItem(void)
	{
	if(hasVertexBufferObjectExtension)
		{
		/* Delete vertex buffer objects: */
		GLuint bufferObjectIds[2];
		bufferObjectIds[0]=surfaceVertexBufferObjectId;
		bufferObjectIds[1]=surfaceIndexBufferObjectId;
		glDeleteBuffersARB(2,bufferObjectIds);
		}
	
	/* Delete the Earth surface texture object: */
	glDeleteTextures(1,&surfaceTextureObjectId);
	
	/* Delete the Earth model components display lists: */
	glDeleteLists(displayListIdBase,4);
	}

/********************************************
Methods of class ShowEarthModel::BaseLocator:
********************************************/

ShowEarthModel::BaseLocator::BaseLocator(Vrui::LocatorTool* sLocatorTool,ShowEarthModel* sApplication)
	:Vrui::LocatorToolAdapter(sLocatorTool),
	 application(sApplication)
	{
	}

void ShowEarthModel::BaseLocator::glRenderAction(GLContextData& contextData) const
	{
	}

/********************************************
Methods of class ShowEarthModel::DataLocator:
********************************************/

ShowEarthModel::DataLocator::DataLocator(Vrui::LocatorTool* sLocatorTool,ShowEarthModel* sApplication)
	:BaseLocator(sLocatorTool,sApplication),
	 dataDialog(0),selectedEvent(0)
	{
	const GLMotif::StyleSheet& ss=*Vrui::getWidgetManager()->getStyleSheet();
	
	/* Create the data dialog window: */
	dataDialog=new GLMotif::PopupWindow("DataDialog",Vrui::getWidgetManager(),"Earthquake Data",ss.font);
	
	GLMotif::RowColumn* data=new GLMotif::RowColumn("Data",dataDialog,false);
	data->setOrientation(GLMotif::RowColumn::VERTICAL);
	data->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	data->setNumMinorWidgets(2);
	data->setSpacing(ss.size);
	
	new GLMotif::Label("TimeLabel",data,"Time",ss.font);
	timeTextField=new GLMotif::TextField("TimeValue",data,ss.font,19);
	
	new GLMotif::Label("MagnitudeLabel",data,"Magnitude",ss.font);
	magnitudeTextField=new GLMotif::TextField("MagnitudeValue",data,ss.font,5);
	magnitudeTextField->setFieldWidth(5);
	magnitudeTextField->setPrecision(2);
	
	GLMotif::Button* setTimeButton=new GLMotif::Button("SetTimeButton",data,"Set Time",ss.font);
	setTimeButton->getSelectCallbacks().add(this,&ShowEarthModel::DataLocator::setTimeButtonSelectCallback);
	
	data->manageChild();
	
	/* Pop up the data dialog: */
	Vrui::popupPrimaryWidget(dataDialog,Vrui::getNavigationTransformation().transform(Vrui::getDisplayCenter()));
	}

ShowEarthModel::DataLocator::~DataLocator(void)
	{
	/* Pop down the data dialog: */
	Vrui::popdownPrimaryWidget(dataDialog);
	
	/* Delete the data dialog: */
	delete dataDialog;
	}

void ShowEarthModel::DataLocator::buttonPressCallback(Vrui::LocatorTool::ButtonPressCallbackData* cbData)
	{
	/* Find the event closest to the locator position: */
	Geometry::Rotation<float,3> rot=Geometry::Rotation<float,3>::rotateAxis(Geometry::Rotation<float,3>::Vector(0.0f,0.0f,1.0f),Math::rad(application->rotationAngle));
	EarthquakeSet::Point pos=rot.inverseTransform(EarthquakeSet::Point(cbData->currentTransformation.getOrigin()));
	
	selectedEvent=0;
	float minDist2=Math::Constants<float>::max;
	for(std::vector<EarthquakeSet*>::const_iterator esIt=application->earthquakeSets.begin();esIt!=application->earthquakeSets.end();++esIt)
		{
		const EarthquakeSet::Event* event=(*esIt)->selectEvent(pos,Math::sqrt(minDist2));
		if(event!=0)
			{
			selectedEvent=event;
			minDist2=Geometry::sqrDist(pos,event->position);
			}
		}
	
	/* Update the data dialog: */
	if(selectedEvent!=0)
		{
		/* Display the event time: */
		time_t tT=time_t(selectedEvent->time);
		struct tm tTm;
		localtime_r(&tT,&tTm);
		char tBuffer[20];
		snprintf(tBuffer,sizeof(tBuffer),"%04d/%02d/%02d %02d:%02d:%02d",tTm.tm_year+1900,tTm.tm_mon+1,tTm.tm_mday,tTm.tm_hour,tTm.tm_min,tTm.tm_sec);
		timeTextField->setLabel(tBuffer);
		
		/* Display the event magnitude: */
		magnitudeTextField->setValue(selectedEvent->magnitude);
		}
	else
		{
		timeTextField->setLabel("");
		magnitudeTextField->setLabel("");
		}
	}

void ShowEarthModel::DataLocator::glRenderAction(GLContextData& contextData) const
	{
	if(selectedEvent!=0)
		{
		GLboolean lightingEnabled=glIsEnabled(GL_LIGHTING);
		if(lightingEnabled)
			glDisable(GL_LIGHTING);
		GLfloat pointSize;
		glGetFloatv(GL_POINT_SIZE,&pointSize);
		glPointSize(3.0f);
		
		glBegin(GL_POINTS);
		glColor3f(1.0f,1.0f,1.0f);
		glVertex(selectedEvent->position);
		glEnd();
		
		glPointSize(pointSize);
		if(lightingEnabled)
			glEnable(GL_LIGHTING);
		}
	}

void ShowEarthModel::DataLocator::setTimeButtonSelectCallback(Misc::CallbackData* cbData)
	{
	if(selectedEvent!=0)
		{
		/* Set the current animation time to the event's time: */
		application->currentTime=selectedEvent->time;
		application->updateCurrentTime();
		application->currentTimeSlider->setValue(application->currentTime);
		
		/* Request to redraw the display: */
		Vrui::requestUpdate();
		}
	}

/*******************************
Methods of class ShowEarthModel:
*******************************/

GLMotif::Popup* ShowEarthModel::createRenderTogglesMenu(void)
	{
	/* Create the submenu's top-level shell: */
	GLMotif::Popup* renderTogglesMenuPopup=new GLMotif::Popup("RenderTogglesMenuPopup",Vrui::getWidgetManager());
	
	/* Create the array of render toggle buttons inside the top-level shell: */
	GLMotif::SubMenu* renderTogglesMenu=new GLMotif::SubMenu("RenderTogglesMenu",renderTogglesMenuPopup,false);
	
	/* Create a toggle button to render the Earth's surface: */
	GLMotif::ToggleButton* showSurfaceToggle=new GLMotif::ToggleButton("ShowSurfaceToggle",renderTogglesMenu,"Show Surface");
	showSurfaceToggle->setToggle(showSurface);
	showSurfaceToggle->getValueChangedCallbacks().add(this,&ShowEarthModel::menuToggleSelectCallback);
	
	/* Create a toggle button to render the Earth's surface transparently: */
	GLMotif::ToggleButton* surfaceTransparentToggle=new GLMotif::ToggleButton("SurfaceTransparentToggle",renderTogglesMenu,"Surface Transparent");
	surfaceTransparentToggle->setToggle(surfaceTransparent);
	surfaceTransparentToggle->getValueChangedCallbacks().add(this,&ShowEarthModel::menuToggleSelectCallback);
	
	/* Create a toggle button to render the lat/long grid: */
	GLMotif::ToggleButton* showGridToggle=new GLMotif::ToggleButton("ShowGridToggle",renderTogglesMenu,"Show Grid");
	showGridToggle->setToggle(showGrid);
	showGridToggle->getValueChangedCallbacks().add(this,&ShowEarthModel::menuToggleSelectCallback);
	
	/* Create toggles for each earthquake set: */
	for(unsigned int i=0;i<earthquakeSets.size();++i)
		{
		/* Create a unique name for the toggle button widget: */
		char toggleName[256];
		snprintf(toggleName,sizeof(toggleName),"ShowEarthquakeSetToggle%04d",i);
		
		/* Create a label to display in the submenu: */
		char toggleLabel[256];
		snprintf(toggleLabel,sizeof(toggleLabel),"Show Earthquake Set %d",i);
		
		/* Create a toggle button to render the earthquake set: */
		GLMotif::ToggleButton* showEarthquakeSetToggle=new GLMotif::ToggleButton(toggleName,renderTogglesMenu,toggleLabel);
		showEarthquakeSetToggle->setToggle(showEarthquakeSets[i]);
		showEarthquakeSetToggle->getValueChangedCallbacks().add(this,&ShowEarthModel::menuToggleSelectCallback);
		}
	
	/* Create toggles for each additional point set: */
	for(unsigned int i=0;i<pointSets.size();++i)
		{
		/* Create a unique name for the toggle button widget: */
		char toggleName[256];
		snprintf(toggleName,sizeof(toggleName),"ShowPointSetToggle%04d",i);
		
		/* Create a label to display in the submenu: */
		char toggleLabel[256];
		snprintf(toggleLabel,sizeof(toggleLabel),"Show Point Set %d",i);
		
		/* Create a toggle button to render the additional point set: */
		GLMotif::ToggleButton* showPointSetToggle=new GLMotif::ToggleButton(toggleName,renderTogglesMenu,toggleLabel);
		showPointSetToggle->setToggle(showPointSets[i]);
		showPointSetToggle->getValueChangedCallbacks().add(this,&ShowEarthModel::menuToggleSelectCallback);
		}
	
	/* Check if there are seismic paths: */
	if(seismicPaths.size()>0)
		{
		/* Create a toggle button to render all seismic paths: */
		GLMotif::ToggleButton* showSeismicPathsToggle=new GLMotif::ToggleButton("ShowSeismicPathsToggle",renderTogglesMenu,"Show Seismic Paths");
		showSeismicPathsToggle->setToggle(showSeismicPaths);
		showSeismicPathsToggle->getValueChangedCallbacks().add(this,&ShowEarthModel::menuToggleSelectCallback);
		}
	
	/* Create a toggle button to render the outer core: */
	GLMotif::ToggleButton* showOuterCoreToggle=new GLMotif::ToggleButton("ShowOuterCoreToggle",renderTogglesMenu,"Show Outer Core");
	showOuterCoreToggle->setToggle(showOuterCore);
	showOuterCoreToggle->getValueChangedCallbacks().add(this,&ShowEarthModel::menuToggleSelectCallback);
	
	/* Create a toggle button to render the outer core transparently: */
	GLMotif::ToggleButton* outerCoreTransparentToggle=new GLMotif::ToggleButton("OuterCoreTransparentToggle",renderTogglesMenu,"Outer Core Transparent");
	outerCoreTransparentToggle->setToggle(outerCoreTransparent);
	outerCoreTransparentToggle->getValueChangedCallbacks().add(this,&ShowEarthModel::menuToggleSelectCallback);
	
	/* Create a toggle button to render the inner core: */
	GLMotif::ToggleButton* showInnerCoreToggle=new GLMotif::ToggleButton("ShowInnerCoreToggle",renderTogglesMenu,"Show Inner Core");
	showInnerCoreToggle->setToggle(showInnerCore);
	showInnerCoreToggle->getValueChangedCallbacks().add(this,&ShowEarthModel::menuToggleSelectCallback);
	
	/* Create a toggle button to render the inner core transparently: */
	GLMotif::ToggleButton* innerCoreTransparentToggle=new GLMotif::ToggleButton("InnerCoreTransparentToggle",renderTogglesMenu,"Inner Core Transparent");
	innerCoreTransparentToggle->setToggle(innerCoreTransparent);
	innerCoreTransparentToggle->getValueChangedCallbacks().add(this,&ShowEarthModel::menuToggleSelectCallback);
	
	/* Calculate the submenu's proper layout: */
	renderTogglesMenu->manageChild();
	
	/* Return the created top-level shell: */
	return renderTogglesMenuPopup;
	}

GLMotif::PopupMenu* ShowEarthModel::createMainMenu(void)
	{
	/* Create a top-level shell for the main menu: */
	GLMotif::PopupMenu* mainMenuPopup=new GLMotif::PopupMenu("MainMenuPopup",Vrui::getWidgetManager());
	mainMenuPopup->setTitle("Interactive Globe");
	
	/* Create the actual menu inside the top-level shell: */
	GLMotif::Menu* mainMenu=new GLMotif::Menu("MainMenu",mainMenuPopup,false);
	
	/* Create a cascade button to show the "Rendering Modes" submenu: */
	GLMotif::CascadeButton* renderTogglesCascade=new GLMotif::CascadeButton("RenderTogglesCascade",mainMenu,"Rendering Modes");
	renderTogglesCascade->setPopup(createRenderTogglesMenu());
	
	/* Create a toggle button to rotate the Earth model: */
	GLMotif::ToggleButton* rotateEarthToggle=new GLMotif::ToggleButton("RotateEarthToggle",mainMenu,"Rotate Earth");
	rotateEarthToggle->setToggle(rotateEarth);
	rotateEarthToggle->getValueChangedCallbacks().add(this,&ShowEarthModel::menuToggleSelectCallback);
	
	/* Create a toggle button to lock navigation coordinates to a fixed-radius sphere: */
	GLMotif::ToggleButton* lockToSphereToggle=new GLMotif::ToggleButton("LockToSphereToggle",mainMenu,"Lock to Sphere");
	lockToSphereToggle->setToggle(lockToSphere);
	lockToSphereToggle->getValueChangedCallbacks().add(this,&ShowEarthModel::menuToggleSelectCallback);
	
	/* Create a button to reset the navigation coordinates to the default (showing the entire Earth): */
	GLMotif::Button* centerDisplayButton=new GLMotif::Button("CenterDisplayButton",mainMenu,"Center Display");
	centerDisplayButton->getSelectCallbacks().add(this,&ShowEarthModel::centerDisplayCallback);
	
	/* Create a toggle button to show the render settings dialog: */
	GLMotif::ToggleButton* showRenderDialogToggle=new GLMotif::ToggleButton("ShowRenderDialogToggle",mainMenu,"Show Render Dialog");
	showRenderDialogToggle->setToggle(false);
	showRenderDialogToggle->getValueChangedCallbacks().add(this,&ShowEarthModel::menuToggleSelectCallback);
	
	/* Create a toggle button to show the animation dialog: */
	GLMotif::ToggleButton* showAnimationDialogToggle=new GLMotif::ToggleButton("ShowAnimationDialogToggle",mainMenu,"Show Animation Dialog");
	showAnimationDialogToggle->setToggle(false);
	showAnimationDialogToggle->getValueChangedCallbacks().add(this,&ShowEarthModel::menuToggleSelectCallback);
	
	/* Calculate the main menu's proper layout: */
	mainMenu->manageChild();
	
	/* Return the created top-level shell: */
	return mainMenuPopup;
	}

GLMotif::PopupWindow* ShowEarthModel::createRenderDialog(void)
	{
	const GLMotif::StyleSheet& ss=*Vrui::getWidgetManager()->getStyleSheet();
	
	GLMotif::PopupWindow* renderDialogPopup=new GLMotif::PopupWindow("RenderDialogPopup",Vrui::getWidgetManager(),"Display Settings");
	renderDialogPopup->setResizableFlags(true,false);
	
	GLMotif::RowColumn* renderDialog=new GLMotif::RowColumn("RenderDialog",renderDialogPopup,false);
	renderDialog->setOrientation(GLMotif::RowColumn::VERTICAL);
	renderDialog->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	renderDialog->setNumMinorWidgets(2);
	
	GLMotif::ToggleButton* showSurfaceToggle=new GLMotif::ToggleButton("ShowSurfaceToggle",renderDialog,"Show Surface");
	showSurfaceToggle->setBorderWidth(0.0f);
	showSurfaceToggle->setMarginWidth(0.0f);
	showSurfaceToggle->setHAlignment(GLFont::Left);
	showSurfaceToggle->setToggle(showSurface);
	showSurfaceToggle->getValueChangedCallbacks().add(this,&ShowEarthModel::menuToggleSelectCallback);
	
	new GLMotif::Blind("Blind1",renderDialog);
	
	new GLMotif::Label("SurfaceTransparencyLabel",renderDialog,"Surface Transparency");
	
	GLMotif::Slider* surfaceTransparencySlider=new GLMotif::Slider("SurfaceTransparencySlider",renderDialog,GLMotif::Slider::HORIZONTAL,ss.fontHeight*5.0f);
	surfaceTransparencySlider->setValueRange(0.0,1.0,0.001);
	surfaceTransparencySlider->setValue(surfaceMaterial.diffuse[3]);
	surfaceTransparencySlider->getValueChangedCallbacks().add(this,&ShowEarthModel::sliderCallback);
	
	new GLMotif::Label("GridTransparencyLabel",renderDialog,"Grid Transparency");
	
	GLMotif::Slider* gridTransparencySlider=new GLMotif::Slider("GridTransparencySlider",renderDialog,GLMotif::Slider::HORIZONTAL,ss.fontHeight*5.0f);
	gridTransparencySlider->setValueRange(0.0,1.0,0.001);
	gridTransparencySlider->setValue(0.1);
	gridTransparencySlider->getValueChangedCallbacks().add(this,&ShowEarthModel::sliderCallback);
	
	GLMotif::ToggleButton* showOuterCoreToggle=new GLMotif::ToggleButton("ShowOuterCoreToggle",renderDialog,"Show Outer Core");
	showOuterCoreToggle->setBorderWidth(0.0f);
	showOuterCoreToggle->setMarginWidth(0.0f);
	showOuterCoreToggle->setHAlignment(GLFont::Left);
	showOuterCoreToggle->setToggle(showOuterCore);
	showOuterCoreToggle->getValueChangedCallbacks().add(this,&ShowEarthModel::menuToggleSelectCallback);
	
	new GLMotif::Blind("Blind2",renderDialog);
	
	new GLMotif::Label("OuterCoreTransparencyLabel",renderDialog,"Outer Core Transparency");
	
	GLMotif::Slider* outerCoreTransparencySlider=new GLMotif::Slider("OuterCoreTransparencySlider",renderDialog,GLMotif::Slider::HORIZONTAL,ss.fontHeight*5.0f);
	outerCoreTransparencySlider->setValueRange(0.0,1.0,0.001);
	outerCoreTransparencySlider->setValue(outerCoreMaterial.diffuse[3]);
	outerCoreTransparencySlider->getValueChangedCallbacks().add(this,&ShowEarthModel::sliderCallback);
	
	GLMotif::ToggleButton* showInnerCoreToggle=new GLMotif::ToggleButton("ShowInnerCoreToggle",renderDialog,"Show Inner Core");
	showInnerCoreToggle->setBorderWidth(0.0f);
	showInnerCoreToggle->setMarginWidth(0.0f);
	showInnerCoreToggle->setHAlignment(GLFont::Left);
	showInnerCoreToggle->setToggle(showInnerCore);
	showInnerCoreToggle->getValueChangedCallbacks().add(this,&ShowEarthModel::menuToggleSelectCallback);
	
	new GLMotif::Blind("Blind3",renderDialog);
	
	new GLMotif::Label("InnerCoreTransparencyLabel",renderDialog,"Inner Core Transparency");
	
	GLMotif::Slider* innerCoreTransparencySlider=new GLMotif::Slider("InnerCoreTransparencySlider",renderDialog,GLMotif::Slider::HORIZONTAL,ss.fontHeight*5.0f);
	innerCoreTransparencySlider->setValueRange(0.0,1.0,0.001);
	innerCoreTransparencySlider->setValue(innerCoreMaterial.diffuse[3]);
	innerCoreTransparencySlider->getValueChangedCallbacks().add(this,&ShowEarthModel::sliderCallback);
	
	new GLMotif::Label("EarthquakePointSizeLabel",renderDialog,"Earthquake Point Size");
	
	GLMotif::Slider* earthquakePointSizeSlider=new GLMotif::Slider("EarthquakePointSizeSlider",renderDialog,GLMotif::Slider::HORIZONTAL,ss.fontHeight*5.0f);
	earthquakePointSizeSlider->setValueRange(1.0,10.0,0.5);
	earthquakePointSizeSlider->setValue(earthquakePointSize);
	earthquakePointSizeSlider->getValueChangedCallbacks().add(this,&ShowEarthModel::sliderCallback);
	
	renderDialog->manageChild();
	
	return renderDialogPopup;
	}

void ShowEarthModel::updateCurrentTime(void)
	{
	time_t ctT=time_t(currentTime);
	struct tm ctTm;
	localtime_r(&ctT,&ctTm);
	char ctBuffer[20];
	snprintf(ctBuffer,sizeof(ctBuffer),"%04d/%02d/%02d %02d:%02d:%02d",ctTm.tm_year+1900,ctTm.tm_mon+1,ctTm.tm_mday,ctTm.tm_hour,ctTm.tm_min,ctTm.tm_sec);
	currentTimeValue->setLabel(ctBuffer);
	
	for(std::vector<EarthquakeSet*>::iterator esIt=earthquakeSets.begin();esIt!=earthquakeSets.end();++esIt)
		(*esIt)->selectEvents(currentTime-playSpeed,currentTime);
	}

GLMotif::PopupWindow* ShowEarthModel::createAnimationDialog(void)
	{
	const GLMotif::StyleSheet& ss=*Vrui::getWidgetManager()->getStyleSheet();
	
	GLMotif::PopupWindow* animationDialogPopup=new GLMotif::PopupWindow("AnimationDialogPopup",Vrui::getWidgetManager(),"Animation");
	animationDialogPopup->setResizableFlags(true,false);
	
	GLMotif::RowColumn* animationDialog=new GLMotif::RowColumn("AnimationDialog",animationDialogPopup,false);
	animationDialog->setNumMinorWidgets(3);
	
	new GLMotif::Label("CurrentTimeLabel",animationDialog,"Current Time");
	
	currentTimeValue=new GLMotif::TextField("CurrentTimeValue",animationDialog,19);
	updateCurrentTime();
	
	currentTimeSlider=new GLMotif::Slider("CurrentTimeSlider",animationDialog,GLMotif::Slider::HORIZONTAL,ss.fontHeight*15.0f);
	currentTimeSlider->setValueRange(earthquakeTimeRange.first,earthquakeTimeRange.second,playSpeed);
	currentTimeSlider->setValue(currentTime);
	currentTimeSlider->getValueChangedCallbacks().add(this,&ShowEarthModel::sliderCallback);
	
	new GLMotif::Label("PlaySpeedLabel",animationDialog,"Playback Speed");
	
	playSpeedValue=new GLMotif::TextField("PlaySpeedValue",animationDialog,6);
	playSpeedValue->setFieldWidth(6);
	playSpeedValue->setPrecision(3);
	playSpeedValue->setValue(Math::log10(playSpeed));
	
	playSpeedSlider=new GLMotif::Slider("PlaySpeedSlider",animationDialog,GLMotif::Slider::HORIZONTAL,ss.fontHeight*10.0f);
	playSpeedSlider->setValueRange(0.0,9.0,0.1);
	playSpeedSlider->setValue(Math::log10(playSpeed));
	playSpeedSlider->getValueChangedCallbacks().add(this,&ShowEarthModel::sliderCallback);
	
	playToggle=new GLMotif::ToggleButton("PlayToggle",animationDialog,"Playback");
	playToggle->setToggle(play);
	playToggle->getValueChangedCallbacks().add(this,&ShowEarthModel::menuToggleSelectCallback);
	
	animationDialog->manageChild();
	
	return animationDialogPopup;
	}

GLPolylineTube* ShowEarthModel::readSensorPathFile(const char* sensorPathFileName,double scaleFactor)
	{
	/* Open the file: */
	Misc::File sensorPathFile(sensorPathFileName,"rt");
	
	/* Read the file header: */
	unsigned int numSamples;
	char line[256];
	while(true)
		{
		/* Read next line from file: */
		sensorPathFile.gets(line,sizeof(line));
		
		/* Parse header line: */
		if(strncmp(line,"PROF_ID=",8)==0) // This line contains number of samples
			{
			if(sscanf(line+8,"%u",&numSamples)!=1)
				Misc::throwStdErr("ShowEarthModel::readSensorPathFile: Unable to parse number of samples in sensor path file %s",sensorPathFileName);
			}
		else if(strncmp(line,"NUMOBS=",7)==0) // This line marks the end of the header (for now)
			break;
		}
	
	/* Create the result sensor path: */
	GLPolylineTube* result=new GLPolylineTube(0.1f,numSamples);
	result->setNumTubeSegments(12);
	
	/* Read the samples: */
	GLPolylineTube::Point lastPos;
	for(unsigned int i=0;i<numSamples;++i)
		{
		/* Read next line from file: */
		sensorPathFile.gets(line,sizeof(line));
		
		/* Get the next sample: */
		float lon,lat,depth,value;
		
		/* Parse the sample's values from the line just read: */
		if(sscanf(line,"%f %f %f %f",&lon,&lat,&depth,&value)!=4)
			Misc::throwStdErr("ShowEarthModel::readSensorPathFile: Error while reading sensor path file %s",sensorPathFileName);
		
		/* Convert position to Cartesian: */
		GLPolylineTube::Point pos;
		calcDepthPos(Math::rad(lat),Math::rad(lon),depth*1000.0f,scaleFactor,pos.getComponents());
		
		/* Store sample point: */
		if(i==0||pos!=lastPos)
			result->addVertex(pos);
		lastPos=pos;
		}
	
	return result;
	}

ShowEarthModel::ShowEarthModel(int& argc,char**& argv,char**& appDefaults)
	:Vrui::Application(argc,argv,appDefaults),
	 scaleToEnvironment(true),
	 rotateEarth(true),
	 lastFrameTime(0.0),rotationAngle(0.0f),rotationSpeed(5.0f),
	 userTransform(0),
	 showSurface(true),surfaceTransparent(false),
	 surfaceMaterial(GLMaterial::Color(1.0f,1.0f,1.0f,0.333f),GLMaterial::Color(0.333f,0.333f,0.333f),10.0f),
	 showGrid(true),
	 showSeismicPaths(false),
	 showOuterCore(false),outerCoreTransparent(true),
	 outerCoreMaterial(GLMaterial::Color(1.0f,0.5f,0.0f,0.333f),GLMaterial::Color(1.0f,1.0f,1.0f),50.0f),
	 showInnerCore(false),innerCoreTransparent(true),
	 innerCoreMaterial(GLMaterial::Color(1.0f,0.0f,0.0f,0.333f),GLMaterial::Color(1.0f,1.0f,1.0f),50.0f),
	 earthquakePointSize(3.0f),
	 sensorPathMaterial(GLMaterial::Color(1.0f,1.0f,0.0f),GLMaterial::Color(1.0f,1.0f,1.0f),50.0f),
	 lockToSphere(false),
	 mainMenu(0),renderDialog(0),animationDialog(0)
	{
	/* Parse the command line: */
	enum FileMode
		{
		POINTSETFILE,EARTHQUAKESETFILE,SEISMICPATHFILE,SENSORPATHFILE
		} fileMode=POINTSETFILE; // Treat all file names as point set files initially
	float colorMask[3]={1.0f,1.0f,1.0f}; // Initial color mask for point set files
	for(int i=1;i<argc;++i)
		{
		/* Check if the current command line argument is a switch or a file name: */
		if(argv[i][0]=='-')
			{
			/* Determine the kind of switch and change the file mode: */
			if(strcasecmp(argv[i]+1,"points")==0)
				fileMode=POINTSETFILE; // Treat all following file names as point set files
			else if(strcasecmp(argv[i]+1,"quakes")==0)
				fileMode=EARTHQUAKESETFILE; // Treat all following file names as earthquake set files
			else if(strcasecmp(argv[i]+1,"seismicpath")==0)
				fileMode=SEISMICPATHFILE; // Treat all following file names as seismic path files
			else if(strcasecmp(argv[i]+1,"sensorpath")==0)
				fileMode=SENSORPATHFILE; // Treat all following file names as sensor path files
			else if(strcasecmp(argv[i]+1,"rotate")==0)
				rotateEarth=true;
			else if(strcasecmp(argv[i]+1,"norotate")==0)
				rotateEarth=false;
			else if(strcasecmp(argv[i]+1,"scale")==0)
				scaleToEnvironment=true;
			else if(strcasecmp(argv[i]+1,"noscale")==0)
				scaleToEnvironment=false;
			else if(strcasecmp(argv[i]+1,"pointsize")==0)
				{
				++i;
				earthquakePointSize=atoi(argv[i]);
				}
			else if(strcasecmp(argv[i]+1,"color")==0)
				{
				for(int j=0;j<3;++j)
					{
					++i;
					colorMask[j]=float(atof(argv[i]));
					}
				}
			else
				std::cout<<"Unrecognized switch "<<argv[i]<<std::endl;
			}
		else
			{
			/* Load the file of the given name using the current file mode: */
			switch(fileMode)
				{
				case POINTSETFILE:
					{
					/* Load a point set: */
					PointSet* pointSet=new PointSet(argv[i],1.0e-3,colorMask);
					pointSets.push_back(pointSet);
					showPointSets.push_back(false);
					break;
					}
				
				case EARTHQUAKESETFILE:
					{
					/* Load an earthquake set: */
					EarthquakeSet* earthquakeSet=new EarthquakeSet(argv[i],1.0e-3);
					earthquakeSets.push_back(earthquakeSet);
					showEarthquakeSets.push_back(false);
					break;
					}
				
				case SEISMICPATHFILE:
					{
					/* Load a seismic path: */
					SeismicPath* path=new SeismicPath(argv[i],1.0e-3);
					seismicPaths.push_back(path);
					break;
					}
				
				case SENSORPATHFILE:
					{
					/* Load a sensor path: */
					GLPolylineTube* path=readSensorPathFile(argv[i],1.0e-3);
					sensorPaths.push_back(path);
					break;
					}
				}
			}
		}
	
	/* Calculate the time range of all earthquake events: */
	if(!earthquakeSets.empty())
		{
		std::vector<EarthquakeSet*>::const_iterator esIt=earthquakeSets.begin();
		earthquakeTimeRange=(*esIt)->getTimeRange();
		for(++esIt;esIt!=earthquakeSets.end();++esIt)
			{
			std::pair<double,double> range=(*esIt)->getTimeRange();
			if(earthquakeTimeRange.first>range.first)
				earthquakeTimeRange.first=range.first;
			if(earthquakeTimeRange.second<range.second)
				earthquakeTimeRange.second=range.second;
			}
		}
	else
		{
		earthquakeTimeRange.first=0.0;
		earthquakeTimeRange.second=0.0;
		}
	
	/* Initialize the earthquake animation: */
	currentTime=earthquakeTimeRange.first;
	playSpeed=365.0*24.0*60.0*60.0; // One second per year
	play=false;
	
	/* Create the user interface: */
	mainMenu=createMainMenu();
	Vrui::setMainMenu(mainMenu);
	renderDialog=createRenderDialog();
	animationDialog=createAnimationDialog();
	
	/* Initialize Vrui navigation transformation: */
	centerDisplayCallback(0);
	
	if(!earthquakeSets.empty())
		{
		/* Register the custom tool class with the Vrui tool manager: */
		EarthquakeToolFactory* earthquakeToolFactory=new EarthquakeToolFactory(*Vrui::getToolManager(),float(Vrui::getUiSize())*5.0f,0.005f,earthquakeSets[0]);
		Vrui::getToolManager()->addClass(earthquakeToolFactory,EarthquakeToolFactory::factoryDestructor);
		}
	
	/* Register a geodetic coordinate transformer with Vrui's coordinate manager: */
	userTransform=new RotatedGeodeticCoordinateTransform;
	Vrui::getCoordinateManager()->setCoordinateTransform(userTransform); // userTransform now owned by coordinate manager
	}

ShowEarthModel::~ShowEarthModel(void)
	{
	/* Delete all earthquake sets: */
	for(std::vector<EarthquakeSet*>::iterator esIt=earthquakeSets.begin();esIt!=earthquakeSets.end();++esIt)
		delete *esIt;
	
	/* Delete all point sets: */
	for(std::vector<PointSet*>::iterator psIt=pointSets.begin();psIt!=pointSets.end();++psIt)
		delete *psIt;
	
	/* Delete all seismic paths: */
	for(std::vector<SeismicPath*>::iterator pIt=seismicPaths.begin();pIt!=seismicPaths.end();++pIt)
		delete *pIt;
	
	/* Delete all sensor paths: */
	for(std::vector<GLPolylineTube*>::iterator pIt=sensorPaths.begin();pIt!=sensorPaths.end();++pIt)
		delete *pIt;
	
	/* Delete the user interface: */
	delete mainMenu;
	delete renderDialog;
	delete animationDialog;
	}

void ShowEarthModel::initContext(GLContextData& contextData) const
	{
	/* Create a new context data item: */
	DataItem* dataItem=new DataItem();
	contextData.addDataItem(this,dataItem);
	
	/* Load the Earth surface texture image from an image file: */
	Images::RGBImage earthTexture=Images::readImageFile(SHOWEARTHMODEL_TOPOGRAPHY_IMAGEFILENAME);
	
	/* Select the Earth surface texture object: */
	glBindTexture(GL_TEXTURE_2D,dataItem->surfaceTextureObjectId);
	
	/* Upload the Earth surface texture image: */
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	earthTexture.glTexImage2D(GL_TEXTURE_2D,0,GL_RGB);
	
	/* Protect the Earth surface texture object: */
	glBindTexture(GL_TEXTURE_2D,0);
	
	/* Create the Earth surface display list: */
	glNewList(dataItem->displayListIdBase+0,GL_COMPILE);
	if(dataItem->hasVertexBufferObjectExtension)
		drawEarth(90,180,1.0e-3,dataItem->surfaceVertexBufferObjectId,dataItem->surfaceIndexBufferObjectId);
	else
		drawEarth(90,180,1.0e-3);
	glEndList();
	
	/* Create the lat/long grid display list: */
	glNewList(dataItem->displayListIdBase+1,GL_COMPILE);
	drawGrid(18,36,10,1.0e-3); // Grid lines every ten degrees, with ten intermediate points
	glEndList();
	
	/* Create the outer core display list: */
	glNewList(dataItem->displayListIdBase+2,GL_COMPILE);
	glDrawSphereIcosahedron(3480.0f,8);
	glEndList();
	
	/* Create the inner core display list: */
	glNewList(dataItem->displayListIdBase+3,GL_COMPILE);
	glDrawSphereIcosahedron(1221.0f,8);
	glEndList();
	}

void ShowEarthModel::toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData)
	{
	/* Check if the new tool is a locator tool: */
	Vrui::LocatorTool* locatorTool=dynamic_cast<Vrui::LocatorTool*>(cbData->tool);
	if(locatorTool!=0)
		{
		/* Create a new data locator: */
		BaseLocator* newLocator=new DataLocator(locatorTool,this);
		
		/* Add new locator to list: */
		baseLocators.push_back(newLocator);
		}
	
	/* Check if the new tool is a surface navigation tool: */
	Vrui::SurfaceNavigationTool* surfaceNavigationTool=dynamic_cast<Vrui::SurfaceNavigationTool*>(cbData->tool);
	if(surfaceNavigationTool!=0)
		{
		/* Set the new tool's alignment function: */
		surfaceNavigationTool->setAlignFunction(Misc::createFunctionCall<Vrui::NavTransform&,ShowEarthModel>(this,&ShowEarthModel::alignSurfaceFrame));
		}
	}

void ShowEarthModel::toolDestructionCallback(Vrui::ToolManager::ToolDestructionCallbackData* cbData)
	{
	/* Check if the to-be-destroyed tool is a locator tool: */
	Vrui::LocatorTool* locatorTool=dynamic_cast<Vrui::LocatorTool*>(cbData->tool);
	if(locatorTool!=0)
		{
		/* Find the base locator associated with the tool in the list: */
		for(BaseLocatorList::iterator blIt=baseLocators.begin();blIt!=baseLocators.end();++blIt)
			if((*blIt)->getTool()==locatorTool)
				{
				/* Remove the data locator: */
				delete *blIt;
				baseLocators.erase(blIt);
				break;
				}
		}
	}

void ShowEarthModel::frame(void)
	{
	/* Get the current application time: */
	double newFrameTime=Vrui::getApplicationTime();
	
	/* Check if Earth animation is enabled: */
	if(rotateEarth)
		{
		/* Update the rotation angle: */
		rotationAngle+=rotationSpeed*float(newFrameTime-lastFrameTime);
		if(rotationAngle>=360.0f)
			rotationAngle-=360.0f;
		userTransform->setRotationAngle(Vrui::Scalar(rotationAngle));
		
		/* Request to redraw the display: */
		Vrui::requestUpdate();
		}
	
	/* Animate the earthquake sets: */
	if(play)
		{
		currentTime+=playSpeed*(newFrameTime-lastFrameTime);
		if(currentTime>earthquakeTimeRange.second)
			{
			currentTime=earthquakeTimeRange.first;
			play=false;
			playToggle->setToggle(false);
			}
		updateCurrentTime();
		currentTimeSlider->setValue(currentTime);
		
		/* Request to redraw the display: */
		Vrui::requestUpdate();
		}
	
	/* Align the navigation transformation with a sphere of fixed radius: */
	if(lockToSphere)
		{
		/* Calculate display center and up vector in locked navigation coordinates: */
		Vrui::Point center=Vrui::getInverseNavigationTransformation().transform(Vrui::getDisplayCenter());
		center=sphereTransform.inverseTransform(center);
		Vrui::Vector up=Vrui::getInverseNavigationTransformation().transform(Vrui::getUpDirection());
		up=sphereTransform.inverseTransform(up);
		up.normalize();
		
		/* Calculate current distance from Earth's center: */
		Vrui::Vector rad=center-Vrui::Point::origin;
		Vrui::Scalar radius=Geometry::mag(rad);
		rad/=radius;
		
		/* Align the up direction with a radial vector from the Earth's center: */
		sphereTransform*=Vrui::NavTransform::translateFromOriginTo(center);
		sphereTransform*=Vrui::NavTransform::rotate(Vrui::Rotation::rotateFromTo(rad,up));
		sphereTransform*=Vrui::NavTransform::translateToOriginFrom(center);
		sphereTransform*=Vrui::NavTransform::translate(up*(radius-sphereRadius));
		sphereTransform.renormalize();
		}
	
	/* Store the current application time: */
	lastFrameTime=newFrameTime;
	}

void ShowEarthModel::display(GLContextData& contextData) const
	{
	#if 0
	/* Print the modelview and projection matrices: */
	GLdouble mv[16],p[16];
	glGetDoublev(GL_MODELVIEW_MATRIX,mv);
	glGetDoublev(GL_PROJECTION_MATRIX,p);
	
	for(int i=0;i<4;++i)
		{
		for(int j=0;j<4;++j)
			std::cout<<" "<<std::setw(12)<<mv[i+j*4];
		#if 0
		std::cout<<"        ";
		for(int j=0;j<4;++j)
			std::cout<<" "<<std::setw(12)<<p[i+j*4];
		#endif
		std::cout<<std::endl;
		}
	std::cout<<std::endl;
	#endif
	
	/* Get context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Save OpenGL state: */
	glPushAttrib(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_LIGHTING_BIT|GL_POLYGON_BIT);
	
	if(lockToSphere)
		{
		glPushMatrix();
		glMultMatrix(sphereTransform);
		}
	
	#if CLIP_SCREEN
	/* Add a clipping plane in the screen plane: */
	Vrui::VRScreen* screen=Vrui::getMainScreen();
	Vrui::ONTransform screenT=screen->getScreenTransformation();
	Vrui::Vector screenNormal=Vrui::getInverseNavigationTransformation().transform(screenT.getDirection(2));
	Vrui::Scalar screenOffset=screenNormal*Vrui::getInverseNavigationTransformation().transform(screenT.getOrigin());
	GLdouble cuttingPlane[4];
	for(int i=0;i<3;++i)
		cuttingPlane[i]=screenNormal[i];
	cuttingPlane[3]=-screenOffset;
	glEnable(GL_CLIP_PLANE0);
	glClipPlane(GL_CLIP_PLANE0,cuttingPlane);
	#endif
	
	/* Rotate all 3D models by the Earth rotation angle: */
	glPushMatrix();
	glRotate(rotationAngle,0.0f,0.0f,1.0f);
	
	/* Render all opaque surfaces: */
	glDisable(GL_CULL_FACE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
	if(showSurface&&!surfaceTransparent)
		{
		/* Set up OpenGL to render the Earth's surface: */
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,dataItem->surfaceTextureObjectId);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
		glMaterial(GLMaterialEnums::FRONT_AND_BACK,surfaceMaterial);
		
		/* Call the Earth surface display list: */
		glCallList(dataItem->displayListIdBase+0);
		
		/* Reset OpenGL state: */
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SINGLE_COLOR);
		glBindTexture(GL_TEXTURE_2D,0);
		glDisable(GL_TEXTURE_2D);
		}
	if(showOuterCore&&!outerCoreTransparent)
		{
		/* Set up OpenGL to render the outer core: */
		glMaterial(GLMaterialEnums::FRONT_AND_BACK,outerCoreMaterial);
		
		/* Call the outer core's display list: */
		glCallList(dataItem->displayListIdBase+2);
		}
	if(showInnerCore&&!innerCoreTransparent)
		{
		/* Set up OpenGL to render the inner core: */
		glMaterial(GLMaterialEnums::FRONT_AND_BACK,innerCoreMaterial);
		
		/* Call the inner core's display list: */
		glCallList(dataItem->displayListIdBase+3);
		}
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
	glEnable(GL_CULL_FACE);
	
	/* Disable lighting to render point/line models: */
	glDisable(GL_LIGHTING);
	
	// DEBUGGING
	#if 0
	glPushMatrix();
	glMultMatrix(navFrame);
	glBegin(GL_LINES);
	glColor3f(1.0f,0.0f,0.0f);
	glVertex3f(0.0f,0.0f,0.0f);
	glVertex3f(1.0f,0.0f,0.0f);
	
	glColor3f(0.0f,1.0f,0.0f);
	glVertex3f(0.0f,0.0f,0.0f);
	glVertex3f(0.0f,1.0f,0.0f);
	
	glColor3f(0.0f,0.0f,1.0f);
	glVertex3f(0.0f,0.0f,0.0f);
	glVertex3f(0.0f,0.0f,1.0f);
	glEnd();
	glPopMatrix();
	#endif
	
	/* Render all earthquake sets: */
	glPointSize(earthquakePointSize);
	for(unsigned int i=0;i<earthquakeSets.size();++i)
		if(showEarthquakeSets[i])
			earthquakeSets[i]->glRenderAction(contextData);
	
	/* Render all additional point sets: */
	static const GLColor<GLfloat,3> pointSetColors[14]=
		{
		GLColor<GLfloat,3>(1.0f,0.0f,0.0f),
		GLColor<GLfloat,3>(1.0f,1.0f,0.0f),
		GLColor<GLfloat,3>(0.0f,1.0f,0.0f),
		GLColor<GLfloat,3>(0.5f,0.5f,0.5f),
		GLColor<GLfloat,3>(0.0f,0.0f,1.0f),
		GLColor<GLfloat,3>(1.0f,0.0f,1.0f),
		GLColor<GLfloat,3>(0.7f,0.7f,0.7f),
		GLColor<GLfloat,3>(1.0f,0.5f,0.5f),
		GLColor<GLfloat,3>(1.0f,1.0f,0.5f),
		GLColor<GLfloat,3>(0.5f,1.0f,0.5f),
		GLColor<GLfloat,3>(0.5f,1.0f,1.0f),
		GLColor<GLfloat,3>(0.5f,0.5f,1.0f),
		GLColor<GLfloat,3>(1.0f,0.5f,1.0f),
		GLColor<GLfloat,3>(0.0f,0.0f,0.0f)
		};
	glPointSize(3.0f);
	for(unsigned int i=0;i<pointSets.size();++i)
		if(showPointSets[i])
			{
			/* Set the color for the point set: */
			glColor(pointSetColors[i%14]);
			
			/* Render the point set: */
			pointSets[i]->glRenderAction(contextData);
			}
	glPointSize(1.0f);
	
	/* Render all seismic paths: */
	if(showSeismicPaths)
		{
		glLineWidth(1.0f);
		glColor3f(1.0f,1.0f,1.0f);
		for(std::vector<SeismicPath*>::const_iterator pIt=seismicPaths.begin();pIt!=seismicPaths.end();++pIt)
			(*pIt)->glRenderAction(contextData);
		}
	
	/* Enable lighting again to render transparent surfaces: */
	glEnable(GL_LIGHTING);
	
	/* Render all sensor paths: */
	glMaterial(GLMaterialEnums::FRONT,sensorPathMaterial);
	for(std::vector<GLPolylineTube*>::const_iterator spIt=sensorPaths.begin();spIt!=sensorPaths.end();++spIt)
		(*spIt)->glRenderAction(contextData);
	
	/* Render all locators: */
	for(BaseLocatorList::const_iterator blIt=baseLocators.begin();blIt!=baseLocators.end();++blIt)
		(*blIt)->glRenderAction(contextData);
	
	/* Render transparent surfaces in back-to-front order: */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
	
	/* Render back parts of surfaces: */
	glCullFace(GL_FRONT);
	if(showSurface&&surfaceTransparent)
		{
		/* Set up OpenGL to render the Earth's surface: */
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,dataItem->surfaceTextureObjectId);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
		glMaterial(GLMaterialEnums::FRONT_AND_BACK,surfaceMaterial);
		
		/* Call the Earth surface display list: */
		glCallList(dataItem->displayListIdBase+0);
		
		/* Reset OpenGL state: */
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SINGLE_COLOR);
		glBindTexture(GL_TEXTURE_2D,0);
		glDisable(GL_TEXTURE_2D);
		}
	if(showGrid)
		{
		glDisable(GL_LIGHTING);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		glLineWidth(1.0f);
		glColor4f(0.0f,1.0f,0.0f,0.1f);
		
		/* Call the lat/long grid display list: */
		glCallList(dataItem->displayListIdBase+1);
		
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_LIGHTING);
		}
	if(showOuterCore&&outerCoreTransparent)
		{
		/* Set up OpenGL to render the outer core: */
		glMaterial(GLMaterialEnums::FRONT_AND_BACK,outerCoreMaterial);
		
		/* Call the outer core's display list: */
		glCallList(dataItem->displayListIdBase+2);
		}
	if(showInnerCore&&innerCoreTransparent)
		{
		/* Set up OpenGL to render the inner core: */
		glMaterial(GLMaterialEnums::FRONT_AND_BACK,innerCoreMaterial);
		
		/* Call the inner core's display list: */
		glCallList(dataItem->displayListIdBase+3);
		}
	
	/* Render front parts of surfaces: */
	glCullFace(GL_BACK);
	if(showInnerCore&&innerCoreTransparent)
		{
		/* Set up OpenGL to render the inner core: */
		glMaterial(GLMaterialEnums::FRONT_AND_BACK,innerCoreMaterial);
		
		/* Call the inner core's display list: */
		glCallList(dataItem->displayListIdBase+3);
		}
	if(showOuterCore&&outerCoreTransparent)
		{
		/* Set up OpenGL to render the outer core: */
		glMaterial(GLMaterialEnums::FRONT_AND_BACK,outerCoreMaterial);
		
		/* Call the outer core's display list: */
		glCallList(dataItem->displayListIdBase+2);
		}
	if(showSurface&&surfaceTransparent)
		{
		/* Set up OpenGL to render the Earth's surface: */
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,dataItem->surfaceTextureObjectId);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
		glMaterial(GLMaterialEnums::FRONT_AND_BACK,surfaceMaterial);
		
		/* Call the Earth surface display list: */
		glCallList(dataItem->displayListIdBase+0);
		
		/* Reset OpenGL state: */
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SINGLE_COLOR);
		glBindTexture(GL_TEXTURE_2D,0);
		glDisable(GL_TEXTURE_2D);
		}
	
	/* Disable blending: */
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	
	/* Go back to original coordinate system: */
	glPopMatrix();
	
	/* Restore OpenGL state: */
	#if CLIP_SCREEN
	glDisable(GL_CLIP_PLANE0);
	#endif
	if(lockToSphere)
		glPopMatrix();
	glPopAttrib();
	}

void ShowEarthModel::alignSurfaceFrame(Vrui::NavTransform& surfaceFrame)
	{
	/* Create a geoid: */
	typedef Geometry::Geoid<Vrui::Scalar> Geoid;
	Geoid geoid(6378.14,1.0/298.247); // Same geoid as used in EarthFunctions.cpp
	
	/* Convert the surface frame's base point to geodetic latitude/longitude: */
	Geoid::Point geodeticBase=geoid.cartesianToGeodetic(surfaceFrame.getOrigin());
	
	/* Snap the base point to the surface: */
	geodeticBase[2]=Geoid::Scalar(0);
	
	/* Create an Earth-aligned coordinate frame at the snapped base point's position: */
	Geoid::Frame frame=geoid.geodeticToCartesianFrame(geodeticBase);
	
	/* Update the passed frame: */
	surfaceFrame=Vrui::NavTransform(frame.getTranslation(),frame.getRotation(),surfaceFrame.getScaling());
	// DEBUGGING
	//navFrame=surfaceFrame;
	}

void ShowEarthModel::menuToggleSelectCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	/* Adjust program state based on which toggle button changed state: */
	if(strcmp(cbData->toggle->getName(),"ShowSurfaceToggle")==0)
		showSurface=cbData->set;
	else if(strcmp(cbData->toggle->getName(),"SurfaceTransparentToggle")==0)
		surfaceTransparent=cbData->set;
	else if(strcmp(cbData->toggle->getName(),"ShowGridToggle")==0)
		showGrid=cbData->set;
	else if(strncmp(cbData->toggle->getName(),"ShowEarthquakeSetToggle",23)==0)
		{
		int earthquakeSetIndex=atoi(cbData->toggle->getName()+23);
		showEarthquakeSets[earthquakeSetIndex]=cbData->set;
		}
	else if(strncmp(cbData->toggle->getName(),"ShowPointSetToggle",18)==0)
		{
		int pointSetIndex=atoi(cbData->toggle->getName()+18);
		showPointSets[pointSetIndex]=cbData->set;
		}
	else if(strcmp(cbData->toggle->getName(),"ShowSeismicPathsToggle")==0)
		showSeismicPaths=cbData->set;
	else if(strcmp(cbData->toggle->getName(),"ShowOuterCoreToggle")==0)
		showOuterCore=cbData->set;
	else if(strcmp(cbData->toggle->getName(),"OuterCoreTransparentToggle")==0)
		outerCoreTransparent=cbData->set;
	else if(strcmp(cbData->toggle->getName(),"ShowInnerCoreToggle")==0)
		showInnerCore=cbData->set;
	else if(strcmp(cbData->toggle->getName(),"InnerCoreTransparentToggle")==0)
		innerCoreTransparent=cbData->set;
	else if(strcmp(cbData->toggle->getName(),"RotateEarthToggle")==0)
		{
		rotateEarth=cbData->set;
		if(rotateEarth)
			lastFrameTime=Vrui::getApplicationTime();
		}
	else if(strcmp(cbData->toggle->getName(),"LockToSphereToggle")==0)
		{
		if(cbData->set)
			{
			/* Calculate display center and up vector in navigation coordinates: */
			Vrui::Point center=Vrui::getInverseNavigationTransformation().transform(Vrui::getDisplayCenter());
			Vrui::Vector up=Vrui::getInverseNavigationTransformation().transform(Vrui::getUpDirection());
			
			/* Calculate current distance from Earth's center: */
			Vrui::Vector rad=center-Vrui::Point::origin;
			sphereRadius=Geometry::mag(rad);
			rad/=sphereRadius;
			
			/* Align the up direction with a radial vector from the Earth's center: */
			sphereTransform=Vrui::NavTransform::identity;
			sphereTransform*=Vrui::NavTransform::translateFromOriginTo(center);
			sphereTransform*=Vrui::NavTransform::rotate(Vrui::Rotation::rotateFromTo(rad,up));
			sphereTransform*=Vrui::NavTransform::translateToOriginFrom(center);
			
			/* Enable sphere locking: */
			lockToSphere=true;
			}
		else
			{
			/* Update the navigation transformation to reflect the current visible transformation: */
			Vrui::concatenateNavigationTransformation(sphereTransform);
			
			/* Disable sphere locking: */
			lockToSphere=false;
			}
		}
	else if(strcmp(cbData->toggle->getName(),"ShowRenderDialogToggle")==0)
		{
		if(cbData->set)
			{
			/* Open the render dialog at the same position as the main menu: */
			Vrui::getWidgetManager()->popupPrimaryWidget(renderDialog,Vrui::getWidgetManager()->calcWidgetTransformation(mainMenu));
			}
		else
			{
			/* Close the render dialog: */
			Vrui::popdownPrimaryWidget(renderDialog);
			}
		}
	else if(strcmp(cbData->toggle->getName(),"ShowAnimationDialogToggle")==0)
		{
		if(cbData->set)
			{
			/* Open the animation dialog at the same position as the main menu: */
			Vrui::getWidgetManager()->popupPrimaryWidget(animationDialog,Vrui::getWidgetManager()->calcWidgetTransformation(mainMenu));
			}
		else
			{
			/* Close the animation dialog: */
			Vrui::popdownPrimaryWidget(animationDialog);
			}
		}
	else if(strcmp(cbData->toggle->getName(),"PlayToggle")==0)
		play=cbData->set;
	}

void ShowEarthModel::sliderCallback(GLMotif::Slider::ValueChangedCallbackData* cbData)
	{
	if(strcmp(cbData->slider->getName(),"SurfaceTransparencySlider")==0)
		{
		surfaceTransparent=cbData->value<1.0;
		surfaceMaterial.diffuse[3]=cbData->value;
		}
	else if(strcmp(cbData->slider->getName(),"GridTransparencySlider")==0)
		{
		}
	else if(strcmp(cbData->slider->getName(),"OuterCoreTransparencySlider")==0)
		{
		outerCoreTransparent=cbData->value<1.0;
		outerCoreMaterial.diffuse[3]=cbData->value;
		}
	else if(strcmp(cbData->slider->getName(),"InnerCoreTransparencySlider")==0)
		{
		innerCoreTransparent=cbData->value<1.0;
		innerCoreMaterial.diffuse[3]=cbData->value;
		}
	else if(strcmp(cbData->slider->getName(),"EarthquakePointSizeSlider")==0)
		{
		earthquakePointSize=float(cbData->value);
		}
	else if(strcmp(cbData->slider->getName(),"CurrentTimeSlider")==0)
		{
		currentTime=cbData->value;
		
		updateCurrentTime();
		}
	else if(strcmp(cbData->slider->getName(),"PlaySpeedSlider")==0)
		{
		playSpeed=Math::pow(10.0,double(cbData->value));
		playSpeedValue->setValue(Math::log10(playSpeed));
		
		currentTimeSlider->setValueRange(earthquakeTimeRange.first,earthquakeTimeRange.second,playSpeed);
		
		updateCurrentTime();
		}
	}

void ShowEarthModel::centerDisplayCallback(Misc::CallbackData* cbData)
	{
	if(scaleToEnvironment)
		{
		/* Center the Earth model in the available display space: */
		Vrui::setNavigationTransformation(Vrui::Point::origin,Vrui::Scalar(3.0*6.4e3));
		}
	else
		{
		/* Center the Earth model in the available display space, but do not scale it: */
		Vrui::NavTransform nav=Vrui::NavTransform::identity;
		nav*=Vrui::NavTransform::translateFromOriginTo(Vrui::getDisplayCenter());
		nav*=Vrui::NavTransform::scale(Vrui::Scalar(8)*Vrui::getInchFactor()/Vrui::Scalar(6.4e3));
		Vrui::setNavigationTransformation(nav);
		}
	}

int main(int argc,char* argv[])
	{
	try
		{
		/* Create the Vrui application object: */
		char** appDefaults=0;
		ShowEarthModel app(argc,argv,appDefaults);
		
		/* Run the Vrui application: */
		app.run();
		
		/* Return to the OS: */
		return 0;
		}
	catch(std::runtime_error err)
		{
		/* Print an error message and return to the OS: */
		std::cerr<<"Caught exception "<<err.what()<<std::endl;
		return 1;
		}
	}
