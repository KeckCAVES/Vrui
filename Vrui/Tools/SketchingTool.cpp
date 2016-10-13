/***********************************************************************
SketchingTool - Tool to create and edit 3D curves.
Copyright (c) 2009-2015 Oliver Kreylos

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

#include <Vrui/Tools/SketchingTool.h>

#include <Misc/SelfDestructArray.h>
#include <Misc/File.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/MessageLogger.h>
#include <IO/ValueSource.h>
#include <IO/OStream.h>
#include <Math/Math.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/TextField.h>
#include <GLMotif/FileSelectionHelper.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>
#include <Vrui/DisplayState.h>
#include <Vrui/OpenFile.h>

namespace Vrui {

/*************************************
Methods of class SketchingToolFactory:
*************************************/

SketchingToolFactory::SketchingToolFactory(ToolManager& toolManager)
	:ToolFactory("SketchingTool",toolManager),
	 detailSize(getUiSize()),
	 curvesFileName("SketchingTool.curves"),
	 curvesSelectionHelper(0)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("UtilityTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	detailSize=cfs.retrieveValue<Scalar>("./detailSize",detailSize);
	curvesFileName=cfs.retrieveString("./curvesFileName",curvesFileName);
	
	/* Set tool class' factory pointer: */
	SketchingTool::factory=this;
	}

SketchingToolFactory::~SketchingToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	SketchingTool::factory=0;
	
	delete curvesSelectionHelper;
	}

const char* SketchingToolFactory::getName(void) const
	{
	return "Curve Editor";
	}

const char* SketchingToolFactory::getButtonFunction(int) const
	{
	return "Draw Curves";
	}

Tool* SketchingToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new SketchingTool(this,inputAssignment);
	}

void SketchingToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

GLMotif::FileSelectionHelper* SketchingToolFactory::getCurvesSelectionHelper(void)
	{
	/* Create a new file selection helper if there isn't one yet: */
	if(curvesSelectionHelper==0)
		curvesSelectionHelper=new GLMotif::FileSelectionHelper(getWidgetManager(),curvesFileName.c_str(),".curves",openDirectory("."));
	
	return curvesSelectionHelper;
	}

extern "C" void resolveSketchingToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("UtilityTool");
	}

extern "C" ToolFactory* createSketchingToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	SketchingToolFactory* sketchingToolFactory=new SketchingToolFactory(*toolManager);
	
	/* Return factory object: */
	return sketchingToolFactory;
	}

extern "C" void destroySketchingToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/********************************************
Methods of class SketchingTool::SketchObject:
********************************************/

SketchingTool::SketchObject::~SketchObject(void)
	{
	}

void SketchingTool::SketchObject::write(IO::OStream& os) const
	{
	/* Write line width and color: */
	os<<lineWidth<<", "<<(unsigned int)(color[0])<<' '<<(unsigned int)(color[1])<<' '<<(unsigned int)(color[2])<<'\n';
	}

void SketchingTool::SketchObject::read(IO::ValueSource& vs)
	{
	/* Read line width and color: */
	lineWidth=GLfloat(vs.readNumber());
	if(vs.readChar()!=',')
		Misc::throwStdErr("File is not a curve file");
	for(int i=0;i<3;++i)
		color[i]=Math::min(Color::Scalar(vs.readUnsignedInteger()),Color::Scalar(255U));
	color[3]=Color::Scalar(255U);
	}

/*************************************
Methods of class SketchingTool::Curve:
*************************************/

void SketchingTool::Curve::write(IO::OStream& os) const
	{
	/* Write the object type: */
	os<<"\nCurve\n";
	
	/* Call base class method: */
	SketchObject::write(os);
	
	/* Write the curve's control points: */
	os<<controlPoints.size()<<'\n';
	for(std::vector<ControlPoint>::const_iterator cpIt=controlPoints.begin();cpIt!=controlPoints.end();++cpIt)
		os<<cpIt->t<<", "<<cpIt->pos[0]<<' '<<cpIt->pos[1]<<' '<<cpIt->pos[2]<<'\n';
	}

void SketchingTool::Curve::read(IO::ValueSource& vs)
	{
	/* Call base class method: */
	SketchObject::read(vs);
	
	/* Read the list of control points: */
	unsigned int numControlPoints=vs.readUnsignedInteger();
	controlPoints.reserve(numControlPoints);
	for(unsigned int controlPointIndex=0;controlPointIndex<numControlPoints;++controlPointIndex)
		{
		ControlPoint cp;
		cp.t=Scalar(vs.readNumber());
		if(vs.readChar()!=',')
			Misc::throwStdErr("File is not a curve file");
		for(int i=0;i<3;++i)
			cp.pos[i]=Point::Scalar(vs.readNumber());
		controlPoints.push_back(cp);
		}
	}

void SketchingTool::Curve::glRenderAction(GLContextData& contextData) const
	{
	/* Draw the curve's control points as a polyline: */
	glLineWidth(lineWidth);
	glColor(color);
	glBegin(GL_LINE_STRIP);
	for(std::vector<ControlPoint>::const_iterator cpIt=controlPoints.begin();cpIt!=controlPoints.end();++cpIt)
		glVertex(cpIt->pos);
	glEnd();
	}

/****************************************
Methods of class SketchingTool::Polyline:
****************************************/

void SketchingTool::Polyline::write(IO::OStream& os) const
	{
	/* Write the object type: */
	os<<"\nPolyline\n";
	
	/* Call base class method: */
	SketchObject::write(os);
	
	/* Write the polyline's vertices: */
	os<<vertices.size()<<'\n';
	for(std::vector<Point>::const_iterator vIt=vertices.begin();vIt!=vertices.end();++vIt)
		os<<(*vIt)[0]<<' '<<(*vIt)[1]<<' '<<(*vIt)[2]<<'\n';
	}

void SketchingTool::Polyline::read(IO::ValueSource& vs)
	{
	/* Call base class method: */
	SketchObject::read(vs);
	
	/* Read the list of vertices: */
	unsigned int numVertices=vs.readUnsignedInteger();
	vertices.reserve(numVertices);
	for(unsigned int vertexIndex=0;vertexIndex<numVertices;++vertexIndex)
		{
		Point pos;
		for(int i=0;i<3;++i)
			pos[i]=Point::Scalar(vs.readNumber());
		vertices.push_back(pos);
		}
	}

void SketchingTool::Polyline::glRenderAction(GLContextData& contextData) const
	{
	/* Draw the polyline's vertices: */
	if(vertices.size()==1)
		{
		glPointSize(lineWidth+2.0f);
		glColor(color);
		glBegin(GL_POINTS);
		glVertex(vertices[0]);
		glEnd();
		}
	else
		{
		glLineWidth(lineWidth);
		glColor(color);
		glBegin(GL_LINE_STRIP);
		for(std::vector<Point>::const_iterator vIt=vertices.begin();vIt!=vertices.end();++vIt)
			glVertex(*vIt);
		glEnd();
		}
	}

/**************************************
Static elements of class SketchingTool:
**************************************/

SketchingToolFactory* SketchingTool::factory=0;
const SketchingTool::Color SketchingTool::colors[8]=
	{
	SketchingTool::Color(0,0,0),SketchingTool::Color(255,0,0),
	SketchingTool::Color(255,255,0),SketchingTool::Color(0,255,0),
	SketchingTool::Color(0,255,255),SketchingTool::Color(0,0,255),
	SketchingTool::Color(255,0,255),SketchingTool::Color(255,255,255)
	};

/******************************
Methods of class SketchingTool:
******************************/

SketchingTool::SketchingTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:UtilityTool(sFactory,inputAssignment),
	 controlDialogPopup(0),colorBox(0),
	 newSketchObjectType(0),newLineWidth(3.0f),newColor(255,0,0),
	 active(false),
	 currentCurve(0),currentPolyline(0)
	{
	/* Get the style sheet: */
	const GLMotif::StyleSheet* ss=getWidgetManager()->getStyleSheet();
	
	/* Build the tool control dialog: */
	controlDialogPopup=new GLMotif::PopupWindow("SketchingToolControlDialog",getWidgetManager(),"Curve Editor Settings");
	controlDialogPopup->setResizableFlags(false,false);
	
	GLMotif::RowColumn* controlDialog=new GLMotif::RowColumn("ControlDialog",controlDialogPopup,false);
	controlDialog->setNumMinorWidgets(1);

	GLMotif::RowColumn* settingsBox=new GLMotif::RowColumn("SettingsBox",controlDialog,false);
	settingsBox->setNumMinorWidgets(2);
	
	/* Create a radio box to select sketching object types: */
	new GLMotif::Label("SketchObjectTypeLabel",settingsBox,"Object Type");
	
	GLMotif::RadioBox* sketchObjectType=new GLMotif::RadioBox("SketchObjectType",settingsBox,false);
	sketchObjectType->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	sketchObjectType->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	sketchObjectType->addToggle("Curve");
	sketchObjectType->addToggle("Polyline");
	sketchObjectType->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
	sketchObjectType->setSelectedToggle(newSketchObjectType);
	sketchObjectType->getValueChangedCallbacks().add(this,&SketchingTool::sketchObjectTypeCallback);
	sketchObjectType->manageChild();
	
	/* Create a slider to set the line width: */
	new GLMotif::Label("LineWidthLabel",settingsBox,"Line Width");
	
	GLMotif::TextFieldSlider* lineWidthSlider=new GLMotif::TextFieldSlider("LineWidthSlider",settingsBox,4,ss->fontHeight*5.0f);
	lineWidthSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	lineWidthSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	lineWidthSlider->setValueRange(0.5,11.0,0.5);
	lineWidthSlider->setValue(newLineWidth);
	lineWidthSlider->getValueChangedCallbacks().add(this,&SketchingTool::lineWidthSliderCallback);
	
	/* Create a radio box to set the line color: */
	new GLMotif::Label("ColorLabel",settingsBox,"Color");
	
	colorBox=new GLMotif::RowColumn("ColorBox",settingsBox,false);
	colorBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	colorBox->setPacking(GLMotif::RowColumn::PACK_GRID);
	colorBox->setAlignment(GLMotif::Alignment::LEFT);
	
	/* Add the color buttons: */
	for(int i=0;i<8;++i)
		{
		char colorButtonName[16];
		snprintf(colorButtonName,sizeof(colorButtonName),"ColorButton%d",i);
		GLMotif::NewButton* colorButton=new GLMotif::NewButton(colorButtonName,colorBox,GLMotif::Vector(ss->fontHeight,ss->fontHeight,0.0f));
		colorButton->setBackgroundColor(GLMotif::Color(colors[i]));
		colorButton->getSelectCallbacks().add(this,&SketchingTool::colorButtonSelectCallback);
		}
	
	colorBox->manageChild();
	
	settingsBox->manageChild();
	
	GLMotif::RowColumn* buttonBox=new GLMotif::RowColumn("ButtonBox",controlDialog,false);
	buttonBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	buttonBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	buttonBox->setAlignment(GLMotif::Alignment::RIGHT);
	
	GLMotif::Button* saveCurvesButton=new GLMotif::Button("SaveCurvesButton",buttonBox,"Save Sketch...");
	factory->getCurvesSelectionHelper()->addSaveCallback(saveCurvesButton,this,&SketchingTool::saveCurvesCallback);
	
	GLMotif::Button* loadCurvesButton=new GLMotif::Button("LoadCurvesButton",buttonBox,"Load Sketch...");
	factory->getCurvesSelectionHelper()->addLoadCallback(loadCurvesButton,this,&SketchingTool::loadCurvesCallback);
	
	GLMotif::Button* deleteAllCurvesButton=new GLMotif::Button("DeleteAllCurvesButton",buttonBox,"Delete All");
	deleteAllCurvesButton->getSelectCallbacks().add(this,&SketchingTool::deleteAllCurvesCallback);
	
	buttonBox->manageChild();
	
	controlDialog->manageChild();
	
	/* Pop up the control dialog: */
	popupPrimaryWidget(controlDialogPopup);
	}

SketchingTool::~SketchingTool(void)
	{
	delete controlDialogPopup;
	
	/* Delete all sketching objects: */
	for(std::vector<SketchObject*>::iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
		delete *soIt;
	}

void SketchingTool::buttonCallback(int,InputDevice::ButtonCallbackData* cbData)
	{
	/* Check if the button has just been pressed: */
	if(cbData->newButtonState)
		{
		switch(newSketchObjectType)
			{
			case 0: // Curve
				{
				/* Start a new curve: */
				currentCurve=new Curve;
				currentCurve->lineWidth=newLineWidth;
				currentCurve->color=newColor;
				sketchObjects.push_back(currentCurve);
				
				/* Append the curve's first control point: */
				Curve::ControlPoint cp;
				const NavTransform& invNav=getInverseNavigationTransformation();
				cp.pos=lastPoint=invNav.transform(getButtonDevicePosition(0));
				cp.t=getApplicationTime();
				currentCurve->controlPoints.push_back(cp);
				
				break;
				}
			
			case 1: // Polyline
				{
				/* Create a new polyline if there isn't one yet: */
				if(currentPolyline==0)
					{
					/* Start a new polyline: */
					currentPolyline=new Polyline;
					currentPolyline->lineWidth=newLineWidth;
					currentPolyline->color=newColor;
					sketchObjects.push_back(currentPolyline);
					}
				
				/* Append the polyline's next vertex: */
				const NavTransform& invNav=getInverseNavigationTransformation();
				lastPoint=invNav.transform(getButtonDevicePosition(0));
				if(!currentPolyline->vertices.empty()&&Geometry::sqrDist(currentPolyline->vertices.front(),lastPoint)<Math::sqr(getPointPickDistance()))
					lastPoint=currentPolyline->vertices.front();
				currentPolyline->vertices.push_back(lastPoint);
				
				break;
				}
			}
		
		/* Activate the tool: */
		active=true;
		}
	else
		{
		switch(newSketchObjectType)
			{
			case 0: // Curve
				{
				/* Append the final control point to the curve: */
				Curve::ControlPoint cp;
				cp.pos=currentPoint;
				cp.t=getApplicationTime();
				currentCurve->controlPoints.push_back(cp);
				
				currentCurve=0;
				break;
				}
			
			case 1: // Polyline
				/* Finish the polyline if the final vertex is the first vertex: */
				if(currentPolyline->vertices.size()>1&&currentPolyline->vertices.front()==currentPolyline->vertices.back())
					currentPolyline=0;
				break;
			}
		
		/* Deactivate the tool: */
		active=false;
		}
	}

void SketchingTool::frame(void)
	{
	if(active)
		{
		/* Get the current dragging point: */
		const NavTransform& invNav=getInverseNavigationTransformation();
		currentPoint=invNav.transform(getButtonDevicePosition(0));
		
		if(currentCurve!=0)
			{
			/* Snap the dragging point to the first curve control point: */
			if(currentCurve->controlPoints.size()>1&&Geometry::sqrDist(currentCurve->controlPoints.front().pos,currentPoint)<Math::sqr(Vrui::getPointPickDistance()))
				currentPoint=currentCurve->controlPoints.front().pos;
			
			/* Check if the dragging point is far enough away from the most recent curve vertex: */
			if(Geometry::sqrDist(currentPoint,lastPoint)>=Math::sqr(factory->detailSize*invNav.getScaling()))
				{
				/* Append the current dragging point to the curve: */
				Curve::ControlPoint cp;
				cp.pos=currentPoint;
				cp.t=getApplicationTime();
				currentCurve->controlPoints.push_back(cp);
				
				/* Remember the last added point: */
				lastPoint=currentPoint;
				}
			}
		
		if(currentPolyline!=0)
			{
			/* Snap the dragging point to the first polyline vertex: */
			if(currentPolyline->vertices.size()>1&&Geometry::sqrDist(currentPolyline->vertices.front(),currentPoint)<Math::sqr(Vrui::getPointPickDistance()))
				currentPoint=currentPolyline->vertices.front();
			
			/* Update the last polyline vertex: */
			currentPolyline->vertices.back()=currentPoint;
			}
		}
	}

void SketchingTool::display(GLContextData& contextData) const
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT|GL_POINT_BIT);
	glDisable(GL_LIGHTING);
	
	/* Go to navigational coordinates: */
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glMultMatrix(getDisplayState(contextData).modelviewNavigational);
	
	/* Render all sketching objects: */
	for(std::vector<SketchObject*>::const_iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
		(*soIt)->glRenderAction(contextData);
	
	/* Go back to physical coordinates: */
	glPopMatrix();
	
	/* Reset OpenGL state: */
	glPopAttrib();
	}

void SketchingTool::sketchObjectTypeCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData)
	{
	/* Deactivate the tool just in case: */
	active=false;
	currentCurve=0;
	currentPolyline=0;
	
	/* Set the new sketch object type: */
	newSketchObjectType=cbData->radioBox->getToggleIndex(cbData->newSelectedToggle);
	}

void SketchingTool::lineWidthSliderCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	/* Get the new line width: */
	newLineWidth=GLfloat(cbData->value);
	}

void SketchingTool::colorButtonSelectCallback(GLMotif::NewButton::SelectCallbackData* cbData)
	{
	/* Set the new line color: */
	newColor=colors[colorBox->getChildIndex(cbData->button)];
	}

void SketchingTool::saveCurvesCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	try
		{
		/* Open the curve file: */
		IO::OStream curveFile(cbData->selectedDirectory->openFile(cbData->selectedFileName,IO::File::WriteOnly));
		
		/* Write the curve file header: */
		curveFile<<"Vrui Curve Editor Tool Curve File"<<std::endl;
		
		/* Write all sketch objects: */
		curveFile<<sketchObjects.size()<<std::endl;
		for(std::vector<SketchObject*>::const_iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
			(*soIt)->write(curveFile);
		}
	catch(std::runtime_error err)
		{
		/* Show an error message: */
		Misc::formattedUserError("Save Curves...: Could not save curves to file %s due to exception %s",cbData->selectedFileName,err.what());
		}
	}	

void SketchingTool::loadCurvesCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	/* Deactivate the tool just in case: */
	active=false;
	currentCurve=0;
	currentPolyline=0;
	
	std::vector<SketchObject*> newSketchObjects;
	try
		{
		/* Open the curve file: */
		IO::ValueSource curvesSource(cbData->selectedDirectory->openFile(cbData->selectedFileName));
		curvesSource.setPunctuation(',',true);
		
		/* Read the curve file header: */
		if(!curvesSource.isString("Vrui Curve Editor Tool Curve File"))
			Misc::throwStdErr("File is not a curve file");
		
		/* Read all sketch objects from the file: */
		unsigned int numSketchObjects=curvesSource.readUnsignedInteger();
		newSketchObjects.reserve(numSketchObjects);
		for(unsigned int sketchObjectIndex=0;sketchObjectIndex<numSketchObjects;++sketchObjectIndex)
			{
			/* Read the sketch object type: */
			std::string sot=curvesSource.readString();
			if(sot=="Curve")
				{
				/* Create a curve: */
				newSketchObjects.push_back(new Curve);
				}
			else if(sot=="Polyline")
				{
				/* Create a polyline: */
				newSketchObjects.push_back(new Polyline);
				}
			else
				Misc::throwStdErr("Unrecognized sketch object type %s",sot.c_str());
			
			/* Read the new sketch object: */
			newSketchObjects.back()->read(curvesSource);
			}
		
		/* Install the new sketch object list: */
		std::swap(sketchObjects,newSketchObjects);
		}
	catch(std::runtime_error err)
		{
		/* Show an error message: */
		Misc::formattedUserError("Load Curves...: Could not load curves from file %s due to exception %s",cbData->selectedFileName,err.what());
		}
	
	/* Delete all new or old sketch objects: */
	for(std::vector<SketchObject*>::iterator soIt=newSketchObjects.begin();soIt!=newSketchObjects.end();++soIt)
		delete *soIt;
	}

void SketchingTool::deleteAllCurvesCallback(Misc::CallbackData* cbData)
	{
	/* Deactivate the tool just in case: */
	active=false;
	currentCurve=0;
	currentPolyline=0;
	
	/* Delete all curves: */
	for(std::vector<SketchObject*>::iterator soIt=sketchObjects.begin();soIt!=sketchObjects.end();++soIt)
		delete *soIt;
	sketchObjects.clear();
	}

}
