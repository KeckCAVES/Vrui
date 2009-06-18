/***********************************************************************
ScalebarNavigationTool - Class to scale navigational coordinates using a
scale bar glyph with an associated settings dialog.
Copyright (c) 2007-2009 Oliver Kreylos

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

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <GL/gl.h>
#include <GL/GLModels.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/ScalebarNavigationTool.h>

namespace Vrui {

/********************************************************
Methods of class ScalebarNavigationToolFactory::DataItem:
********************************************************/

ScalebarNavigationToolFactory::DataItem::DataItem(void)
	{
	/* Create tools' model display list: */
	scalebarEndListId=glGenLists(1);
	}

ScalebarNavigationToolFactory::DataItem::~DataItem(void)
	{
	/* Destroy tools' model display list: */
	glDeleteLists(scalebarEndListId,1);
	}

/**********************************************
Methods of class ScalebarNavigationToolFactory:
**********************************************/

ScalebarNavigationToolFactory::ScalebarNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("ScalebarNavigationTool",toolManager)
	 scalebarWidth(getUiSize()),
	 scalebarMaxHeight(getDisplaySize()),
	 scalebarEndWidth(getUiSize()*Scalar(3)),
	 scalebarEndHeight(getUiSize()*Scalar(0.5)),
	 defaultNavUnit(INCH),
	 defaultScaleMode(NATURAL)
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("NavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	scalebarWidth=cfs.retrieveValue<Scalar>("./scalebarWidth",scalebarWidth);
	scalebarMaxHeight=cfs.retrieveValue<Scalar>("./scalebarMaxHeight",scalebarMaxHeight);
	scalebarEndWidth=cfs.retrieveValue<Scalar>("./scalebarEndWidth",scalebarWidth*Scalar(3));
	scalebarEndHeight=cfs.retrieveValue<Scalar>("./scalebarEndHeight",scalebarWidth*Scalar(0.5));
	
	/* Set tool class' factory pointer: */
	ScalebarNavigationTool::factory=this;
	}

ScalebarNavigationToolFactory::~ScalebarNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	ScalebarNavigationTool::factory=0;
	}

const char* ScalebarNavigationToolFactory::getName(void) const
	{
	return "Scaling via Scale Bar";
	}

Tool* ScalebarNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new ScalebarNavigationTool(this,inputAssignment);
	}

void ScalebarNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}


void ScalebarNavigationToolFactory::initContext(GLContextData& contextData) const
	{
	/* Create a new data item: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Create the scalebar end display list: */
	glNewList(dataItem->modelListId,GL_COMPILE);
	
	/* Render a cylinder of the proper radius and height around the origin: */
	glDrawCylinder(scalebarEndWidth,scalebarEndHeight,16);
	
	glEndList();
	}
extern "C" void resolveScalebarNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("NavigationTool");
	}

extern "C" ToolFactory* createScalebarNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	ScalebarNavigationToolFactory* scalebarNavigationToolFactory=new ScalebarNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return scalebarNavigationToolFactory;
	}

extern "C" void destroyScalebarNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/***********************************************
Static elements of class ScalebarNavigationTool:
***********************************************/

ScalebarNavigationToolFactory* ScalebarNavigationTool::factory=0;

/***************************************
Methods of class ScalebarNavigationTool:
***************************************/

ScalebarNavigationTool::ScalebarNavigationTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(sFactory,inputAssignment),
	 navUnit(factory->defaultNavUnit),
	 scaleMode(factory->defaultScaleMode),
	 scaleFactor(0)
	{
	/* Calculate the navigational unit factor: */
	switch(navUnit)
		{
		case ScalebarNavigationToolFactory::MM:
			unitFactor=getInchFactor()/Scalar(25.4);
			break;
		
		case ScalebarNavigationToolFactory::CM:
			unitFactor=getInchFactor()/Scalar(2.54);
			break;
		
		case ScalebarNavigationToolFactory::M:
			unitFactor=getInchFactor()/Scalar(0.0254);
			break;
		
		case ScalebarNavigationToolFactory::KM:
			unitFactor=getInchFactor()/Scalar(0.0000254);
			break;
		
		case ScalebarNavigationToolFactory::INCH:
			unitFactor=getInchFactor();
			break;
		
		case ScalebarNavigationToolFactory::MILE:
			unitFactor=getInchFactor()*Scalar(63360);
			break;
		}
	
	/* Initialize the scalebar transformation: */
	scalebarTransform=ONTransform::translateFromOriginTo(getDisplayCenter());
	scalebarTransform*=ONTransform::rotate(Rotation::rotateFromTo(Vector(0,1,0),getUpDirection()));
	}

const ToolFactory* ScalebarNavigationTool::getFactory(void) const
	{
	return factory;
	}

void ScalebarNavigationTool::buttonCallback(int,int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		}
	else // Button has just been released
		{
		}
	}

void ScalebarNavigationTool::frame(void)
	{
	/* Get the navigation transformation's current scaling factor: */
	Scalar newScaleFactor=getNavigationTransformation().getScaling();
	if(newScaleFactor!=scaleFactor)
		{
		/* Calculate appropriate scalebar length in navigational coordinates: */
		Scalar unitFactor;
		switch(navUnit)
			{
			case ScalebarNavigationToolFactory::MM:
				break;
			
			}
		}
	}

void ScalebarNavigationTool::display(GLContextData& contextData) const
	{
	/* Get a pointer to the context entry: */
	ScalebarNavigationToolFactory::DataItem* dataItem=contextData.retrieveDataItem<ScalebarNavigationToolFactory::DataItem>(factory);
	
	/* Translate coordinate system to scaling device's position and orientation: */
	glPushMatrix();
	glMultMatrix(getDeviceTransformation(0));
	
	/* Execute the tool model display list: */
	glCallList(dataItem->modelListId);
	
	/* Go back to physical coordinate system: */
	glPopMatrix();
	}

}
