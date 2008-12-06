/***********************************************************************
PanelMenuTool - Class for menu tools that attach the program's main menu
to an input device and allow any widget interaction tool to select items
from it.
Copyright (c) 2004-2008 Oliver Kreylos

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

#include <GLMotif/WidgetManager.h>
#include <GLMotif/PopupMenu.h>
#include <Vrui/MutexMenu.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/PanelMenuTool.h>

namespace Vrui {

/*************************************
Methods of class PanelMenuToolFactory:
*************************************/

PanelMenuToolFactory::PanelMenuToolFactory(ToolManager& toolManager)
	:ToolFactory("PanelMenuTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,0);
	
	/* Insert class into class hierarchy: */
	ToolFactory* menuToolFactory=toolManager.loadClass("MenuTool");
	menuToolFactory->addChildClass(this);
	addParentClass(menuToolFactory);
	
	/* Set tool class' factory pointer: */
	PanelMenuTool::factory=this;
	}

PanelMenuToolFactory::~PanelMenuToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	PanelMenuTool::factory=0;
	}

Tool* PanelMenuToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new PanelMenuTool(this,inputAssignment);
	}

void PanelMenuToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolvePanelMenuToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("MenuTool");
	}

extern "C" ToolFactory* createPanelMenuToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	PanelMenuToolFactory* panelMenuToolFactory=new PanelMenuToolFactory(*toolManager);
	
	/* Return factory object: */
	return panelMenuToolFactory;
	}

extern "C" void destroyPanelMenuToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/**************************************
Static elements of class PanelMenuTool:
**************************************/

PanelMenuToolFactory* PanelMenuTool::factory=0;

/******************************
Methods of class PanelMenuTool:
******************************/

PanelMenuTool::PanelMenuTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:MenuTool(factory,inputAssignment)
	{
	}

PanelMenuTool::~PanelMenuTool(void)
	{
	if(isActive())
		{
		/* Pop down the menu: */
		getWidgetManager()->popdownWidget(menu->getPopup());
		
		/* Deactivate the tool again: */
		deactivate();
		}
	}

const ToolFactory* PanelMenuTool::getFactory(void) const
	{
	return factory;
	}

void PanelMenuTool::frame(void)
	{
	if(isActive())
		{
		/* Get a pointer to the input device: */
		InputDevice* device=input.getDevice(0);
		
		/* Calculate the menu transformation: */
		GLMotif::WidgetManager::Transformation menuTransformation=device->getTransformation();
		GLMotif::Vector topLeft=menu->getPopup()->getExterior().getCorner(2);
		menuTransformation*=GLMotif::WidgetManager::Transformation::translate(-Vector(topLeft.getXyzw()));
		
		/* Set the menu's position: */
		getWidgetManager()->setPrimaryWidgetTransformation(menu->getPopup(),menuTransformation);
		}
	}

void PanelMenuTool::setMenu(MutexMenu* newMenu)
	{
	/* Call the base class method first: */
	MenuTool::setMenu(newMenu);
	
	/* Try activating this tool (it will grab the main menu until it is destroyed): */
	if(activate())
		{
		/* Get a pointer to the input device: */
		InputDevice* device=input.getDevice(0);
		
		/* Calculate the menu transformation: */
		GLMotif::WidgetManager::Transformation menuTransformation=device->getTransformation();
		GLMotif::Vector topLeft=menu->getPopup()->getExterior().getCorner(2);
		menuTransformation*=GLMotif::WidgetManager::Transformation::translate(-Vector(topLeft.getXyzw()));
		
		/* Pop up the menu: */
		getWidgetManager()->popupPrimaryWidget(menu->getPopup(),menuTransformation);
		}
	}

}
