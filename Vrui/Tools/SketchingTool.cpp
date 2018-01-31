/***********************************************************************
SketchingTool - Tool to create and edit 3D curves.
Copyright (c) 2009-2017 Oliver Kreylos

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
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLValueCoders.h>
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
#include <GLMotif/WidgetStateHelper.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>
#include <Vrui/DisplayState.h>
#include <Vrui/OpenFile.h>

namespace Misc {

/**************************************
Helper class to decode sketching modes:
**************************************/

template <>
class ValueCoder<Vrui::SketchingTool::SketchMode>
	{
	/* Methods: */
	public:
	static std::string encode(const Vrui::SketchingTool::SketchMode& value)
		{
		switch(value)
			{
			case Vrui::SketchingTool::CURVE:
				return "Curve";
			
			case Vrui::SketchingTool::POLYLINE:
				return "Polyline";
			
			case Vrui::SketchingTool::BRUSHSTROKE:
				return "BrushStroke";
			
			case Vrui::SketchingTool::ERASER:
				return "Eraser";
			
			default:
				return ""; // Never reached; just to make compiler happy
			}
		}
	static Vrui::SketchingTool::SketchMode decode(const char* start,const char* end,const char** decodeEnd =0)
		{
		if(end-start>=5&&strncasecmp(start,"Curve",5)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+5;
			return Vrui::SketchingTool::CURVE;
			}
		else if(end-start>=8&&strncasecmp(start,"Polyline",8)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+8;
			return Vrui::SketchingTool::POLYLINE;
			}
		else if(end-start>=11&&strncasecmp(start,"BrushStroke",11)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+11;
			return Vrui::SketchingTool::BRUSHSTROKE;
			}
		else if(end-start>=6&&strncasecmp(start,"Eraser",6)==0)
			{
			if(decodeEnd!=0)
				*decodeEnd=start+6;
			return Vrui::SketchingTool::ERASER;
			}
		else
			throw DecodingError(std::string("Unable to convert \"")+std::string(start,end)+std::string("\" to SketchingTool::SketchMode"));
		}
	};

}

namespace Vrui {

/*************************************
Methods of class SketchingToolFactory:
*************************************/

SketchingToolFactory::SketchingToolFactory(ToolManager& toolManager)
	:ToolFactory("SketchingTool",toolManager),
	 detailSize(getUiSize()),
	 brushAxis(1,0,0),
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
	brushAxis=cfs.retrieveValue<Vector>("./brushAxis",brushAxis);
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

bool SketchingTool::Curve::pick(const Point& p,Scalar radius2) const
	{
	/* Check the point's distance from the bounding box: */
	if(boundingBox.sqrDist(p)>radius2)
		return false;
	
	/* Check every control point against the given point: */
	for(std::vector<ControlPoint>::const_iterator cpIt=controlPoints.begin();cpIt!=controlPoints.end();++cpIt)
		if(Geometry::sqrDist(p,cpIt->pos)<=radius2)
			return true;
	
	return false;
	}

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
	
	/* Read the list of control points and compute the bounding box: */
	boundingBox=Box::empty;
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
		boundingBox.addPoint(cp.pos);
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

void SketchingTool::Curve::setGLState(GLContextData& contextData)
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	}

void SketchingTool::Curve::resetGLState(GLContextData& contextData)
	{
	/* Reset OpenGL: */
	glPopAttrib();
	}

/****************************************
Methods of class SketchingTool::Polyline:
****************************************/

bool SketchingTool::Polyline::pick(const Point& p,Scalar radius2) const
	{
	/* Check the point's distance from the bounding box: */
	if(boundingBox.sqrDist(p)>radius2)
		return false;
	
	/* Check the beginning vertex against the given point: */
	std::vector<Point>::const_iterator v0It=vertices.begin();
	if(Geometry::sqrDist(p,*v0It)<=radius2)
		return true;
	
	/* Check every polyline segment against the given point: */
	for(std::vector<Point>::const_iterator v1It=v0It+1;v1It!=vertices.end();v0It=v1It,++v1It)
		{
		/* Check the segment's end vertex against the given point: */
		if(Geometry::sqrDist(p,*v1It)<=radius2)
			return true;
		
		/* Check the line segment against the given point: */
		Vector segDir=*v1It-*v0It;
		Scalar segLength2=segDir.sqr();
		if(segLength2>=radius2)
			{
			/* Check if the point is inside the segment's extents: */
			Vector pv0=p-*v0It;
			Scalar y=segDir*pv0;
			Scalar y2=Math::sqr(y)/segLength2;
			if(y>=Scalar(0)&&y2<=segLength2)
				{
				/* Check the distance from the given point to the segment's line: */
				Scalar dist2=Geometry::sqr(pv0)-y2;
				if(dist2<=radius2)
					return true;
				}
			}
		}
	
	return false;
	}

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
	
	/* Read the list of vertices and compute the bounding box: */
	boundingBox=Box::empty;
	unsigned int numVertices=vs.readUnsignedInteger();
	vertices.reserve(numVertices);
	for(unsigned int vertexIndex=0;vertexIndex<numVertices;++vertexIndex)
		{
		Point pos;
		for(int i=0;i<3;++i)
			pos[i]=Point::Scalar(vs.readNumber());
		vertices.push_back(pos);
		boundingBox.addPoint(pos);
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

void SketchingTool::Polyline::setGLState(GLContextData& contextData)
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT|GL_POINT_BIT);
	glDisable(GL_LIGHTING);
	}

void SketchingTool::Polyline::resetGLState(GLContextData& contextData)
	{
	/* Reset OpenGL: */
	glPopAttrib();
	}

/*******************************************
Methods of class SketchingTool::BrushStroke:
*******************************************/

bool SketchingTool::BrushStroke::pick(const Point& p,Scalar radius2) const
	{
	/* Check the point's distance from the bounding box: */
	if(boundingBox.sqrDist(p)>radius2)
		return false;
	
	/* Check every control point against the given point: */
	for(std::vector<ControlPoint>::const_iterator cpIt=controlPoints.begin();cpIt!=controlPoints.end();++cpIt)
		if(Geometry::sqrDist(p,cpIt->pos)<=radius2)
			return true;
	
	return false;
	}

void SketchingTool::BrushStroke::write(IO::OStream& os) const
	{
	/* Write the object type: */
	os<<"\nBrushStroke\n";
	
	/* Call base class method: */
	SketchObject::write(os);
	
	/* Write the brush stroke's control points: */
	os<<controlPoints.size()<<'\n';
	for(std::vector<ControlPoint>::const_iterator cpIt=controlPoints.begin();cpIt!=controlPoints.end();++cpIt)
		{
		os<<cpIt->pos[0]<<' '<<cpIt->pos[1]<<' '<<cpIt->pos[2]<<", ";
		os<<cpIt->brushAxis[0]<<' '<<cpIt->brushAxis[1]<<' '<<cpIt->brushAxis[2]<<", ";
		os<<cpIt->normal[0]<<' '<<cpIt->normal[1]<<' '<<cpIt->normal[2]<<std::endl;
		}
	}

void SketchingTool::BrushStroke::read(IO::ValueSource& vs)
	{
	/* Call base class method: */
	SketchObject::read(vs);
	
	/* Read the list of control points and compute the bounding box: */
	boundingBox=Box::empty;
	unsigned int numControlPoints=vs.readUnsignedInteger();
	controlPoints.reserve(numControlPoints);
	for(unsigned int controlPointIndex=0;controlPointIndex<numControlPoints;++controlPointIndex)
		{
		ControlPoint cp;
		for(int i=0;i<3;++i)
			cp.pos[i]=Point::Scalar(vs.readNumber());
		if(vs.readChar()!=',')
			Misc::throwStdErr("File is not a curve file");
		for(int i=0;i<3;++i)
			cp.brushAxis[i]=Vector::Scalar(vs.readNumber());
		if(vs.readChar()!=',')
			Misc::throwStdErr("File is not a curve file");
		for(int i=0;i<3;++i)
			cp.normal[i]=Vector::Scalar(vs.readNumber());
		controlPoints.push_back(cp);
		boundingBox.addPoint(cp.pos-cp.brushAxis);
		boundingBox.addPoint(cp.pos+cp.brushAxis);
		}
	}

void SketchingTool::BrushStroke::glRenderAction(GLContextData& contextData) const
	{
	/* Draw the brush stroke as a quad strip: */
	glColor(color);
	glBegin(GL_QUAD_STRIP);
	for(std::vector<ControlPoint>::const_iterator cpIt=controlPoints.begin();cpIt!=controlPoints.end();++cpIt)
		{
		glNormal(cpIt->normal);
		glVertex(cpIt->pos+cpIt->brushAxis);
		glVertex(cpIt->pos-cpIt->brushAxis);
		}
	glEnd();
	}

void SketchingTool::BrushStroke::setGLState(GLContextData& contextData)
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LIGHTING_BIT);
	glEnable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
	}

void SketchingTool::BrushStroke::resetGLState(GLContextData& contextData)
	{
	/* Reset OpenGL: */
	glPopAttrib();
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
	 sketchMode(CURVE),newLineWidth(3.0f),newColor(255,0,0),
	 active(false),
	 currentCurve(0),currentPolyline(0),currentBrushStroke(0)
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
	
	sketchObjectType=new GLMotif::RadioBox("SketchObjectType",settingsBox,false);
	sketchObjectType->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	sketchObjectType->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	sketchObjectType->addToggle("Curve");
	sketchObjectType->addToggle("Polyline");
	sketchObjectType->addToggle("Brush Stroke");
	sketchObjectType->addToggle("Eraser");
	sketchObjectType->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
	sketchObjectType->setSelectedToggle(int(sketchMode));
	sketchObjectType->getValueChangedCallbacks().add(this,&SketchingTool::sketchModeCallback);
	sketchObjectType->manageChild();
	
	/* Create a slider to set the line width: */
	new GLMotif::Label("LineWidthLabel",settingsBox,"Line Width");
	
	lineWidthSlider=new GLMotif::TextFieldSlider("LineWidthSlider",settingsBox,4,ss->fontHeight*5.0f);
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
	for(std::vector<Curve*>::iterator cIt=curves.begin();cIt!=curves.end();++cIt)
		delete *cIt;
	for(std::vector<Polyline*>::iterator pIt=polylines.begin();pIt!=polylines.end();++pIt)
		delete *pIt;
	for(std::vector<BrushStroke*>::iterator bsIt=brushStrokes.begin();bsIt!=brushStrokes.end();++bsIt)
		delete *bsIt;
	}

void SketchingTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Read the sketch mode: */
	sketchMode=configFileSection.retrieveValue<SketchMode>("./sketchMode",sketchMode);
	
	/* Read current line width and color: */
	newLineWidth=configFileSection.retrieveValue<GLfloat>("./lineWidth",newLineWidth);
	newColor=configFileSection.retrieveValue<Color>("./color",newColor);
	
	/* Update the control dialog: */
	sketchObjectType->setSelectedToggle(int(sketchMode));
	lineWidthSlider->setValue(newLineWidth);
	
	/* Read the control dialog's position, orientation, and size: */
	GLMotif::readTopLevelPosition(controlDialogPopup,configFileSection);
	}

void SketchingTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write the sketch mode: */
	configFileSection.storeValue<SketchMode>("./sketchMode",sketchMode);
	
	/* Write current line width and color: */
	configFileSection.storeValue<GLfloat>("./lineWidth",newLineWidth);
	configFileSection.storeValue<Color>("./color",newColor);
	
	/* Write the control dialog's current position, orientation, and size: */
	GLMotif::writeTopLevelPosition(controlDialogPopup,configFileSection);
	}

const ToolFactory* SketchingTool::getFactory(void) const
	{
	return factory;
	}

void SketchingTool::buttonCallback(int,InputDevice::ButtonCallbackData* cbData)
	{
	/* Check if the button has just been pressed: */
	if(cbData->newButtonState)
		{
		switch(sketchMode)
			{
			case CURVE:
				{
				/* Start a new curve: */
				currentCurve=new Curve(newLineWidth,newColor);
				curves.push_back(currentCurve);
				
				/* Append the curve's first control point: */
				Curve::ControlPoint cp;
				const NavTransform& invNav=getInverseNavigationTransformation();
				cp.pos=lastPoint=invNav.transform(getButtonDevicePosition(0));
				cp.t=getApplicationTime();
				currentCurve->controlPoints.push_back(cp);
				currentCurve->boundingBox.addPoint(cp.pos);
				
				/* Append the curve's tentative last control point: */
				currentCurve->controlPoints.push_back(cp);
				break;
				}
			
			case POLYLINE:
				{
				/* Create a new polyline if there isn't one yet: */
				if(currentPolyline==0)
					{
					/* Start a new polyline: */
					currentPolyline=new Polyline(newLineWidth,newColor);
					polylines.push_back(currentPolyline);
					}
				
				/* Append the polyline's next vertex: */
				const NavTransform& invNav=getInverseNavigationTransformation();
				lastPoint=invNav.transform(getButtonDevicePosition(0));
				if(!currentPolyline->vertices.empty()&&Geometry::sqrDist(currentPolyline->vertices.front(),lastPoint)<Math::sqr(getPointPickDistance()))
					lastPoint=currentPolyline->vertices.front();
				currentPolyline->vertices.push_back(lastPoint);
				
				break;
				}
			
			case BRUSHSTROKE:
				{
				/* Start a new brush stroke: */
				currentBrushStroke=new BrushStroke(newLineWidth,newColor);
				brushStrokes.push_back(currentBrushStroke);
				
				/* Append the brush stroke's first control point: */
				BrushStroke::ControlPoint cp;
				const NavTransform& invNav=getInverseNavigationTransformation();
				cp.pos=lastPoint=invNav.transform(getButtonDevicePosition(0));
				cp.brushAxis=invNav.transform(getButtonDeviceTransformation(0).transform(factory->brushAxis))*(getUiSize()*Scalar(newLineWidth));
				cp.normal=Geometry::normal(cp.brushAxis);
				currentBrushStroke->controlPoints.push_back(cp);
				currentBrushStroke->boundingBox.addPoint(cp.pos+cp.brushAxis);
				currentBrushStroke->boundingBox.addPoint(cp.pos-cp.brushAxis);
				
				/* Append the brush stroke's tentative last control point: */
				currentBrushStroke->controlPoints.push_back(cp);
				break;
				}
			
			case ERASER:
				break;
			}
		
		/* Activate the tool: */
		active=true;
		}
	else
		{
		switch(sketchMode)
			{
			case CURVE:
				{
				/* Append the final control point to the curve's bounding box: */
				currentCurve->boundingBox.addPoint(currentCurve->controlPoints.back().pos);
				
				/* Finish the curve: */
				currentCurve=0;
				break;
				}
			
			case POLYLINE:
				/* Add the final vertex to the polyline's bounding box: */
				currentPolyline->boundingBox.addPoint(currentPolyline->vertices.back());
				
				/* Finish the polyline if the final vertex is the first vertex: */
				if(currentPolyline->vertices.size()>1&&currentPolyline->vertices.front()==currentPolyline->vertices.back())
					currentPolyline=0;
				break;
			
			case BRUSHSTROKE:
				{
				/* Append the final control point to the brush stroke's bounding box: */
				currentBrushStroke->boundingBox.addPoint(currentBrushStroke->controlPoints.back().pos+currentBrushStroke->controlPoints.back().brushAxis);
				currentBrushStroke->boundingBox.addPoint(currentBrushStroke->controlPoints.back().pos-currentBrushStroke->controlPoints.back().brushAxis);
				
				/* Finish the brush stroke: */
				currentBrushStroke=0;
				break;
				}
			
			case ERASER:
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
			#if 0
			/* Snap the dragging point to the first curve control point: */
			if(currentCurve->controlPoints.size()>2&&Geometry::sqrDist(currentCurve->controlPoints.front().pos,currentPoint)<Math::sqr(getPointPickDistance()))
				currentPoint=currentCurve->controlPoints.front().pos;
			#endif
			
			/* Set the tentative last control point: */
			Curve::ControlPoint& cp=currentCurve->controlPoints.back();
			cp.pos=currentPoint;
			cp.t=getApplicationTime();
			
			/* Check if the dragging point is far enough away from the most recent curve vertex: */
			if(Geometry::sqrDist(currentPoint,lastPoint)>=Math::sqr(factory->detailSize*invNav.getScaling()))
				{
				/* Fix the tentative last control point: */
				currentCurve->controlPoints.push_back(cp);
				currentCurve->boundingBox.addPoint(cp.pos);
				
				/* Remember the last added point: */
				lastPoint=currentPoint;
				}
			}
		
		if(currentPolyline!=0)
			{
			/* Snap the dragging point to the first polyline vertex: */
			if(currentPolyline->vertices.size()>1&&Geometry::sqrDist(currentPolyline->vertices.front(),currentPoint)<Math::sqr(getPointPickDistance()))
				currentPoint=currentPolyline->vertices.front();
			
			/* Update the last polyline vertex: */
			currentPolyline->vertices.back()=currentPoint;
			}
		
		if(currentBrushStroke!=0)
			{
			/* Update the brush stroke's final normal vectors: */
			std::vector<BrushStroke::ControlPoint>::iterator cpIt=currentBrushStroke->controlPoints.end();
			unsigned int length=currentBrushStroke->controlPoints.size();
			if(length>2)
				cpIt[-2].normal=Geometry::normalize((cpIt[-1].pos-cpIt[-3].pos)^cpIt[-2].brushAxis);
			else
				cpIt[-2].normal=Geometry::normalize((cpIt[-1].pos-cpIt[-2].pos)^cpIt[-2].brushAxis);
			cpIt[-1].normal=Geometry::normalize((cpIt[-1].pos-cpIt[-2].pos)^cpIt[-1].brushAxis);
			
			/* Set the tentative last control point: */
			BrushStroke::ControlPoint& cp=currentBrushStroke->controlPoints.back();
			cp.pos=currentPoint;
			cp.brushAxis=invNav.transform(getButtonDeviceTransformation(0).transform(factory->brushAxis))*(getUiSize()*Scalar(newLineWidth));
			cp.normal=Geometry::normal(cp.brushAxis);
			
			/* Check if the dragging point is far enough away from the most recent curve vertex: */
			if(Geometry::sqrDist(currentPoint,lastPoint)>=Math::sqr(factory->detailSize*invNav.getScaling()))
				{
				/* Fix the tentative last control point: */
				currentBrushStroke->controlPoints.push_back(cp);
				currentBrushStroke->boundingBox.addPoint(cp.pos+cp.brushAxis);
				currentBrushStroke->boundingBox.addPoint(cp.pos-cp.brushAxis);
				
				/* Remember the last added point: */
				lastPoint=currentPoint;
				}
			}
		
		if(currentCurve==0&&currentPolyline==0&&currentBrushStroke==0&&sketchMode==ERASER)
			{
			/* Delete all sketching objects inside the eraser's influence area: */
			Scalar radius2=Math::sqr(getPointPickDistance());
			
			/* Check all sketching objects: */
			for(std::vector<Curve*>::iterator cIt=curves.begin();cIt!=curves.end();++cIt)
				if((*cIt)->pick(currentPoint,radius2))
					{
					/* Delete the curve: */
					delete *cIt;
					*cIt=curves.back();
					curves.pop_back();
					--cIt;
					}
			for(std::vector<Polyline*>::iterator pIt=polylines.begin();pIt!=polylines.end();++pIt)
				if((*pIt)->pick(currentPoint,radius2))
					{
					/* Delete the polyline: */
					delete *pIt;
					*pIt=polylines.back();
					polylines.pop_back();
					--pIt;
					}
			for(std::vector<BrushStroke*>::iterator bsIt=brushStrokes.begin();bsIt!=brushStrokes.end();++bsIt)
				if((*bsIt)->pick(currentPoint,radius2))
					{
					/* Delete the brush stroke: */
					delete *bsIt;
					*bsIt=brushStrokes.back();
					brushStrokes.pop_back();
					--bsIt;
					}
			}
		}
	}

void SketchingTool::display(GLContextData& contextData) const
	{
	/* Go to navigational coordinates: */
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glMultMatrix(getDisplayState(contextData).modelviewNavigational);
	
	/* Render all sketching objects: */
	Curve::setGLState(contextData);
	for(std::vector<Curve*>::const_iterator cIt=curves.begin();cIt!=curves.end();++cIt)
		(*cIt)->glRenderAction(contextData);
	Curve::resetGLState(contextData);
	
	Polyline::setGLState(contextData);
	for(std::vector<Polyline*>::const_iterator pIt=polylines.begin();pIt!=polylines.end();++pIt)
		(*pIt)->glRenderAction(contextData);
	Polyline::resetGLState(contextData);
	
	BrushStroke::setGLState(contextData);
	for(std::vector<BrushStroke*>::const_iterator bsIt=brushStrokes.begin();bsIt!=brushStrokes.end();++bsIt)
		(*bsIt)->glRenderAction(contextData);
	BrushStroke::resetGLState(contextData);
	
	/* Go back to physical coordinates: */
	glPopMatrix();
	
	if(sketchMode==BRUSHSTROKE&&!active)
		{
		/* Draw the brush: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(3.0f);
		
		glBegin(GL_LINES);
		glColor(newColor);
		Point pos=getButtonDevice(0)->getPosition();
		Vector brushAxis=getButtonDeviceTransformation(0).transform(factory->brushAxis)*(getUiSize()*Scalar(newLineWidth));
		glVertex(pos+brushAxis);
		glVertex(pos-brushAxis);
		glEnd();
		
		glPopAttrib();
		}
	}

void SketchingTool::sketchModeCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData)
	{
	/* Deactivate the tool just in case: */
	active=false;
	currentCurve=0;
	currentPolyline=0;
	
	/* Set the new sketch object type: */
	switch(cbData->radioBox->getToggleIndex(cbData->newSelectedToggle))
		{
		case 0:
			sketchMode=CURVE;
			break;
		
		case 1:
			sketchMode=POLYLINE;
			break;
		
		case 2:
			sketchMode=BRUSHSTROKE;
			break;
		
		case 3:
			sketchMode=ERASER;
			break;
		}
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
		
		/* Write all sketching objects: */
		curveFile<<curves.size()+polylines.size()+brushStrokes.size()<<std::endl;
		for(std::vector<Curve*>::iterator cIt=curves.begin();cIt!=curves.end();++cIt)
			(*cIt)->write(curveFile);
		for(std::vector<Polyline*>::iterator pIt=polylines.begin();pIt!=polylines.end();++pIt)
			(*pIt)->write(curveFile);
		for(std::vector<BrushStroke*>::iterator bsIt=brushStrokes.begin();bsIt!=brushStrokes.end();++bsIt)
			(*bsIt)->write(curveFile);
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
	currentBrushStroke=0;
	
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
				newSketchObjects.push_back(new Curve(0.0f,Color(0,0,0)));
				}
			else if(sot=="Polyline")
				{
				/* Create a polyline: */
				newSketchObjects.push_back(new Polyline(0.0f,Color(0,0,0)));
				}
			else if(sot=="BrushStroke")
				{
				/* Create a brush stroke: */
				newSketchObjects.push_back(new BrushStroke(0.0f,Color(0,0,0)));
				}
			else
				Misc::throwStdErr("Unrecognized sketch object type %s",sot.c_str());
			
			/* Read the new sketch object: */
			newSketchObjects.back()->read(curvesSource);
			}
		
		/* Distribute the new sketching objects to the per-type lists: */
		curves.clear();
		polylines.clear();
		brushStrokes.clear();
		for(std::vector<SketchObject*>::iterator soIt=newSketchObjects.begin();soIt!=newSketchObjects.end();++soIt)
			{
			Curve* c=dynamic_cast<Curve*>(*soIt);
			if(c!=0)
				curves.push_back(c);
			
			Polyline* p=dynamic_cast<Polyline*>(*soIt);
			if(p!=0)
				polylines.push_back(p);
			
			BrushStroke* bs=dynamic_cast<BrushStroke*>(*soIt);
			if(bs!=0)
				brushStrokes.push_back(bs);
			}
		}
	catch(std::runtime_error err)
		{
		/* Delete the list of read sketching objects: */
		for(std::vector<SketchObject*>::iterator soIt=newSketchObjects.begin();soIt!=newSketchObjects.end();++soIt)
			delete *soIt;
		
		/* Show an error message: */
		Misc::formattedUserError("Load Curves...: Could not load curves from file %s due to exception %s",cbData->selectedFileName,err.what());
		}
	}

void SketchingTool::deleteAllCurvesCallback(Misc::CallbackData* cbData)
	{
	/* Deactivate the tool just in case: */
	active=false;
	currentCurve=0;
	currentPolyline=0;
	currentBrushStroke=0;
	
	/* Delete all sketching objects: */
	for(std::vector<Curve*>::iterator cIt=curves.begin();cIt!=curves.end();++cIt)
		delete *cIt;
	curves.clear();
	for(std::vector<Polyline*>::iterator pIt=polylines.begin();pIt!=polylines.end();++pIt)
		delete *pIt;
	polylines.clear();
	for(std::vector<BrushStroke*>::iterator bsIt=brushStrokes.begin();bsIt!=brushStrokes.end();++bsIt)
		delete *bsIt;
	brushStrokes.clear();
	}

}
