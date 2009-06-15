/***********************************************************************
ToolManager - Class to manage tool classes, and dynamic assignment of
tools to input devices.
Copyright (c) 2004-2009 Oliver Kreylos

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
#include <iostream>
#include <vector>
#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <GLMotif/Button.h>
#include <GLMotif/CascadeButton.h>
#include <GLMotif/Popup.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Menu.h>
#include <GLMotif/SubMenu.h>
#include <GLMotif/PopupMenu.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/MutexMenu.h>
#include <Vrui/Tools/LocatorTool.h>
#include <Vrui/Tools/DraggingTool.h>
#include <Vrui/Tools/NavigationTool.h>
#include <Vrui/Tools/SurfaceNavigationTool.h>
#include <Vrui/Tools/TransformTool.h>
#include <Vrui/Tools/UserInterfaceTool.h>
#include <Vrui/Tools/MenuTool.h>
#include <Vrui/Tools/InputDeviceTool.h>
#include <Vrui/Tools/PointingTool.h>
#include <Vrui/Tools/UtilityTool.h>
#include <Vrui/ToolKillZone.h>
#include <Vrui/ToolKillZoneBox.h>
#include <Vrui/ToolKillZoneFrustum.h>
#include <Vrui/Vrui.h>

#include <Vrui/ToolManager.h>

namespace Vrui {

/************************************************
Methods of class ToolManager::ToolAssignmentSlot:
************************************************/

ToolManager::ToolAssignmentSlot::ToolAssignmentSlot(void)
	:device(0),slotType(NONE),slotIndex(-1),
	 assigned(false),tool(0),
	 preemptedButtonPress(false)
	{
	}

ToolManager::ToolAssignmentSlot::~ToolAssignmentSlot(void)
	{
	if(device!=0&&slotIndex>=0)
		{
		/* Remove callback from the input device: */
		switch(slotType)
			{
			case NONE:
				/* Ignore... */
				break;
			
			case BUTTON:
				device->getButtonCallbacks(slotIndex).remove(this,&ToolAssignmentSlot::inputDeviceButtonCallback);
				break;
			
			case VALUATOR:
				device->getValuatorCallbacks(slotIndex).remove(this,&ToolAssignmentSlot::inputDeviceValuatorCallback);
				break;
			}
		}
	}

void ToolManager::ToolAssignmentSlot::initialize(InputDevice* sDevice,ToolManager::ToolAssignmentSlot::SlotType sSlotType,int sSlotIndex)
	{
	/* Initialize elements: */
	device=sDevice;
	slotType=sSlotType;
	slotIndex=sSlotIndex;
	
	/* Add callback to the input device: */
	switch(slotType)
		{
		case NONE:
			/* Ignore... */
			break;
		
		case BUTTON:
			device->getButtonCallbacks(slotIndex).add(this,&ToolAssignmentSlot::inputDeviceButtonCallback);
			break;
		
		case VALUATOR:
			device->getValuatorCallbacks(slotIndex).add(this,&ToolAssignmentSlot::inputDeviceValuatorCallback);
			break;
		}
	}

void ToolManager::ToolAssignmentSlot::inputDeviceButtonCallback(InputDevice::ButtonCallbackData* cbData)
	{
	bool interruptCallback=false;
	if(cbData->newButtonState) // Button has just been pressed
		interruptCallback=pressed();
	else // Button has just been released
		interruptCallback=released();
	if(interruptCallback)
		{
		/* Interrupt processing of this callback: */
		cbData->callbackList->requestInterrupt();
		}
	}

void ToolManager::ToolAssignmentSlot::inputDeviceValuatorCallback(InputDevice::ValuatorCallbackData* cbData)
	{
	bool interruptCallback=false;
	if(cbData->oldValuatorValue==0.0&&cbData->newValuatorValue!=0.0) // Valuator has just been moved from the idle position
		interruptCallback=pressed();
	else if(cbData->oldValuatorValue!=0.0&&cbData->newValuatorValue==0.0) // Valuator has just returned to the idle position
		interruptCallback=released();
	else
		interruptCallback=preemptedButtonPress;
	if(interruptCallback)
		{
		/* Interrupt processing of this callback: */
		cbData->callbackList->requestInterrupt();
		}
	}

bool ToolManager::ToolAssignmentSlot::pressed(void)
	{
	bool interruptCallback=false;
	
	/* Get pointer to the tool manager: */
	ToolManager* tm=getToolManager();
	
	if(assigned)
		{
		/* Check if the input device is in (or pointing at) the tool "kill zone:" */
		if(tm->toolKillZone->isDeviceIn(device))
			{
			/* Push a new job onto the tool management queue: */
			ToolManagementQueue::iterator tmqIt=tm->toolManagementQueue.insert(tm->toolManagementQueue.end(),ToolManagementQueueItem());
			tmqIt->itemFunction=ToolManagementQueueItem::DESTROY_TOOL;
			tmqIt->createToolFactory=0;
			tmqIt->tia=0;
			tmqIt->tas=this;
			
			/* Interrupt processing of this callback: */
			preemptedButtonPress=true;
			interruptCallback=true;
			}
		}
	else
		{
		if(tm->activeToolSelectionMenuTool!=0)
			{
			/* Push this tool assignment slot onto the creation slot list: */
			std::vector<ToolAssignmentSlot*>::iterator tcsIt;
			for(tcsIt=tm->toolCreationSlots.begin();tcsIt!=tm->toolCreationSlots.end()&&*tcsIt!=this;++tcsIt)
				;
			if(tcsIt==tm->toolCreationSlots.end())
				tm->toolCreationSlots.push_back(this);
			
			/* Interrupt processing of this callback: */
			preemptedButtonPress=true;
			interruptCallback=true;
			}
		}
	
	return interruptCallback;
	}

bool ToolManager::ToolAssignmentSlot::released(void)
	{
	bool interruptCallback=false;
	
	if(preemptedButtonPress)
		{
		/* Interrupt processing of this callback: */
		preemptedButtonPress=false;
		interruptCallback=true;
		}
	
	return interruptCallback;
	}

/**************************************
Callback wrappers of class ToolManager:
**************************************/

void ToolManager::destroyToolFactoryFunction(ToolFactory* toolFactory)
	{
	delete toolFactory;
	}

/****************************
Methods of class ToolManager:
****************************/

GLMotif::Popup* ToolManager::createToolSubmenu(const Plugins::Factory& factory)
	{
	char popupName[256];
	snprintf(popupName,sizeof(popupName),"%sSubmenuPopup",factory.getClassName());
	GLMotif::Popup* toolSubmenuPopup=new GLMotif::Popup(popupName,getWidgetManager());
	
	GLMotif::SubMenu* toolSubmenu=new GLMotif::SubMenu("ToolSubmenu",toolSubmenuPopup,false);
	
	/* Create entries for all tool subclasses: */
	for(Plugins::Factory::ClassList::const_iterator chIt=factory.childrenBegin();chIt!=factory.childrenEnd();++chIt)
		{
		/* Get a pointer to the tool factory: */
		const ToolFactory* factory=dynamic_cast<const ToolFactory*>(*chIt);
		if(factory==0)
			Misc::throwStdErr("ToolManager::createToolSubmenu: factory class %s is not a Vrui tool factory class",(*chIt)->getClassName());
		
		/* Check if current class is leaf class: */
		if((*chIt)->getChildren().empty())
			{
			/* Create button for tool class: */
			GLMotif::Button* toolButton=new GLMotif::Button((*chIt)->getClassName(),toolSubmenu,factory->getName());
			toolButton->getSelectCallbacks().add(this,&ToolManager::toolMenuSelectionCallback);
			}
		else
			{
			/* Create cascade button and submenu for tool class: */
			GLMotif::CascadeButton* toolCascade=new GLMotif::CascadeButton((*chIt)->getClassName(),toolSubmenu,factory->getName());
			toolCascade->setPopup(createToolSubmenu(**chIt));
			}
		}
	
	toolSubmenu->manageChild();
	
	return toolSubmenuPopup;
	}

GLMotif::PopupMenu* ToolManager::createToolMenu(void)
	{
	/* Create menu shell: */
	GLMotif::PopupMenu* toolSelectionMenuPopup=new GLMotif::PopupMenu("ToolSelectionMenuPopup",getWidgetManager());
	toolSelectionMenuPopup->setTitle("Tool Selection Menu");
	
	GLMotif::Menu* toolSelectionMenu=new GLMotif::Menu("ToolSelectionMenu",toolSelectionMenuPopup,false);
	
	/* Create entries for all root tool classes: */
	for(FactoryIterator fIt=begin();fIt!=end();++fIt)
		{
		/* Check if current class is root class: */
		if(fIt->getParents().empty())
			{
			/* Check if current class is leaf class: */
			if(fIt->getChildren().empty())
				{
				/* Create button for tool class: */
				GLMotif::Button* toolButton=new GLMotif::Button(fIt->getClassName(),toolSelectionMenu,fIt->getName());
				toolButton->getSelectCallbacks().add(this,&ToolManager::toolMenuSelectionCallback);
				}
			else
				{
				/* Create cascade button and submenu for tool class: */
				GLMotif::CascadeButton* toolCascade=new GLMotif::CascadeButton(fIt->getClassName(),toolSelectionMenu,fIt->getName());
				toolCascade->setPopup(createToolSubmenu(*fIt));
				}
			}
		}
	
	toolSelectionMenu->manageChild();
	
	return toolSelectionMenuPopup;
	}

Tool* ToolManager::assignToolSelectionTool(ToolManager::ToolAssignmentSlot& tas)
	{
	/* Check whether the assignment slot is for a button or for a valuator: */
	if(tas.slotType==ToolAssignmentSlot::BUTTON)
		{
		/* Create a menu tool: */
		ToolInputAssignment tia(toolSelectionMenuFactory->getLayout());
		tia.setDevice(0,tas.device);
		tia.setButtonIndex(0,0,tas.slotIndex);
		MenuTool* newTool=dynamic_cast<MenuTool*>(toolSelectionMenuFactory->createTool(tia));
		if(newTool==0)
			Misc::throwStdErr("ToolManager: Tool selection tool class is not a MenuTool");
		
		/* Mark button assignment: */
		tas.assigned=false;
		tas.tool=newTool;
		
		/* Assign tool selection menu: */
		newTool->setMenu(toolMenu);
		
		/* Install tool callbacks: */
		newTool->getActivationCallbacks().add(this,&ToolManager::toolActivationCallback);
		newTool->getDeactivationCallbacks().add(this,&ToolManager::toolDeactivationCallback);
		
		/* Add tool to list: */
		tools.push_back(newTool);
		
		/* Add tool to input graph: */
		inputGraphManager->addTool(newTool);
		
		/* Call tool creation callbacks: */
		ToolCreationCallbackData cbData(newTool);
		toolCreationCallbacks.call(&cbData);
		
		/* Initialize the tool: */
		newTool->initialize();
		
		return newTool;
		}
	else
		{
		/* Can't handle valuator assignment slots yet... */
		return 0;
		}
	}

void ToolManager::assignToolSelectionTools(void)
	{
	/* Assign tool selection menu to all unassigned buttons: */
	for(ToolAssignmentSlotList::iterator tasIt=toolAssignmentSlots.begin();tasIt!=toolAssignmentSlots.end();++tasIt)
		if(tasIt->tool==0)
			assignToolSelectionTool(*tasIt);
	}

void ToolManager::destroyTool(Tool* tool)
	{
	/* De-initialize the tool: */
	tool->deinitialize();
	
	/* Call tool destruction callbacks: */
	ToolDestructionCallbackData cbData(tool);
	toolDestructionCallbacks.call(&cbData);
	
	/* Remove tool from input graph: */
	inputGraphManager->removeTool(tool);
	
	/* Remove tool from list: */
	for(ToolList::iterator tIt=tools.begin();tIt!=tools.end();++tIt)
		if(*tIt==tool)
			{
			tools.erase(tIt);
			break;
			}
	
	/* Destroy the tool: */
	tool->getFactory()->destroyTool(tool);
	}

void ToolManager::unassignTool(Tool* tool)
	{
	/* Find all tool assignments for the given tool: */
	for(ToolAssignmentSlotList::iterator tasIt=toolAssignmentSlots.begin();tasIt!=toolAssignmentSlots.end();++tasIt)
		if(tasIt->tool==tool)
			{
			/* Unassign the tool from the input device/button/valuator combination: */
			tasIt->assigned=false;
			tasIt->tool=0;
			}
	}

void ToolManager::inputDeviceCreationCallback(Misc::CallbackData* cbData)
	{
	/* Get pointer to the new device: */
	InputDevice* newDevice=static_cast<InputDeviceManager::InputDeviceCreationCallbackData*>(cbData)->inputDevice;
	
	/* Create tool assignment slots for the new input device: */
	for(int i=0;i<newDevice->getNumButtons();++i)
		{
		/* Create new tool assignment slot: */
		ToolAssignmentSlotList::iterator tasIt=toolAssignmentSlots.insert(toolAssignmentSlots.end(),ToolAssignmentSlot());
		
		/* Set it to the device/button combination: */
		tasIt->initialize(newDevice,ToolAssignmentSlot::BUTTON,i);
		
		if(toolSelectionMenuFactory!=0)
			{
			/* Create and assign a tool selection tool: */
			assignToolSelectionTool(*tasIt);
			}
		}
	for(int i=0;i<newDevice->getNumValuators();++i)
		{
		/* Create new tool assignment slot: */
		ToolAssignmentSlotList::iterator tasIt=toolAssignmentSlots.insert(toolAssignmentSlots.end(),ToolAssignmentSlot());
		
		/* Set it to the device/valuator combination: */
		tasIt->initialize(newDevice,ToolAssignmentSlot::VALUATOR,i);
		
		if(toolSelectionMenuFactory!=0)
			{
			/* Create and assign a tool selection tool: */
			assignToolSelectionTool(*tasIt);
			}
		}
	}

void ToolManager::inputDeviceDestructionCallback(Misc::CallbackData* cbData)
	{
	/* Get pointer to the new device: */
	InputDevice* device=static_cast<InputDeviceManager::InputDeviceDestructionCallbackData*>(cbData)->inputDevice;
	
	/* Create temporary lists of tools and assignment slots that need to be destroyed: */
	std::vector<Tool*> destroyTools;
	std::vector<ToolAssignmentSlotList::iterator> destroySlotIterators;
	for(ToolAssignmentSlotList::iterator tasIt=toolAssignmentSlots.begin();tasIt!=toolAssignmentSlots.end();++tasIt)
		if(tasIt->device==device)
			{
			if(tasIt->tool!=0)
				destroyTools.push_back(tasIt->tool);
			destroySlotIterators.push_back(tasIt);
			}
	
	/* Destroy all tools in the temporary list: */
	for(std::vector<Tool*>::iterator tIt=destroyTools.begin();tIt!=destroyTools.end();++tIt)
		{
		/* Unassign the tool from its slots: */
		unassignTool(*tIt);
		
		/* Destroy the tool: */
		destroyTool(*tIt);
		}
	
	/* Destroy all tool assignment slots that belong to the destroyed device: */
	for(std::vector<ToolAssignmentSlotList::iterator>::iterator tasItIt=destroySlotIterators.begin();tasItIt!=destroySlotIterators.end();++tasItIt)
		toolAssignmentSlots.erase(*tasItIt);
	
	/* Assign tool selection tools to all slots that have become unassigned: */
	assignToolSelectionTools();
	}

void ToolManager::toolActivationCallback(Misc::CallbackData* cbData)
	{
	/* Get a pointer to the tool: */
	Tool* tool=static_cast<MenuTool::ActivationCallbackData*>(cbData)->tool;
	
	/* Mark the tool as active: */
	activeToolSelectionMenuTool=tool;
	
	/* Push this tool's assignment slot onto the tool creation slot list: */
	for(ToolAssignmentSlotList::iterator tasIt=toolAssignmentSlots.begin();tasIt!=toolAssignmentSlots.end();++tasIt)
		if(tasIt->tool==activeToolSelectionMenuTool)
			{
			toolCreationSlots.push_back(&(*tasIt));
			break;
			}
	}

void ToolManager::toolDeactivationCallback(Misc::CallbackData*)
	{
	/* Stop creating a new tool: */
	toolCreationSlots.clear();
	
	/* Mark the tool as inactive: */
	activeToolSelectionMenuTool=0;
	}

void ToolManager::toolMenuSelectionCallback(Misc::CallbackData* cbData)
	{
	/* Get a pointer to the new tool's factory: */
	ToolFactory* newFactory=loadClass(static_cast<GLMotif::Button::SelectCallbackData*>(cbData)->button->getName());
	
	/* Get the new factory's layout: */
	const ToolInputLayout& layout=newFactory->getLayout();
	
	/* Create an input assignment for the new tool: */
	ToolInputAssignment* tia=new ToolInputAssignment(layout);
	int* numButtons=new int[layout.getNumDevices()];
	int* numValuators=new int[layout.getNumDevices()];
	int numDevices=0;
	for(int i=0;i<layout.getNumDevices();++i)
		{
		numButtons[i]=0;
		numValuators[i]=0;
		}
	
	/* Process all tool creation slots: */
	for(std::vector<ToolAssignmentSlot*>::const_iterator tcsIt=toolCreationSlots.begin();tcsIt!=toolCreationSlots.end();++tcsIt)
		{
		/* Find the index of the slot's device in the input assignment: */
		int deviceIndex;
		for(deviceIndex=0;deviceIndex<numDevices&&tia->getDevice(deviceIndex)!=(*tcsIt)->device;++deviceIndex)
			;
		
		if(deviceIndex<layout.getNumDevices())
			{
			if(deviceIndex==numDevices)
				{
				/* Add the device to the list: */
				tia->setDevice(deviceIndex,(*tcsIt)->device);
				++numDevices;
				}
			
			switch((*tcsIt)->slotType)
				{
				case ToolAssignmentSlot::NONE:
					/* Ignore... */
					break;
				
				case ToolAssignmentSlot::BUTTON:
					if(numButtons[deviceIndex]<layout.getNumButtons(deviceIndex))
						{
						tia->setButtonIndex(deviceIndex,numButtons[deviceIndex],(*tcsIt)->slotIndex);
						++numButtons[deviceIndex];
						}
					break;
				
				case ToolAssignmentSlot::VALUATOR:
					if(numValuators[deviceIndex]<layout.getNumValuators(deviceIndex))
						{
						tia->setValuatorIndex(deviceIndex,numValuators[deviceIndex],(*tcsIt)->slotIndex);
						++numValuators[deviceIndex];
						}
					break;
				}
			}
		}
	
	/* Check if the tool input layout has been satisfied: */
	bool assignmentFits=numDevices==layout.getNumDevices();
	for(int i=0;i<layout.getNumDevices();++i)
		{
		assignmentFits=assignmentFits&&numButtons[i]==layout.getNumButtons(i);
		assignmentFits=assignmentFits&&numValuators[i]==layout.getNumValuators(i);
		}
	delete[] numButtons;
	delete[] numValuators;
	
	if(assignmentFits)
		{
		/* Find a pointer to this tool's assignment slot: */
		ToolAssignmentSlotList::iterator tasIt;
		for(tasIt=toolAssignmentSlots.begin();tasIt->tool!=activeToolSelectionMenuTool;++tasIt)
			;
		
		/* Push a new job onto the tool management queue: */
		ToolManagementQueue::iterator tmqIt=toolManagementQueue.insert(toolManagementQueue.end(),ToolManagementQueueItem());
		tmqIt->itemFunction=ToolManagementQueueItem::CREATE_TOOL;
		tmqIt->createToolFactory=newFactory;
		tmqIt->tia=tia;
		tmqIt->tas=&(*tasIt);
		}
	else
		delete tia;
	}

ToolManager::ToolManager(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& sConfigFileSection)
	:Plugins::FactoryManager<ToolFactory>(sConfigFileSection.retrieveString("./toolDsoNameTemplate",SYSTOOLDSONAMETEMPLATE)),
	 inputGraphManager(sInputDeviceManager->getInputGraphManager()),
	 inputDeviceManager(sInputDeviceManager),
	 configFileSection(sConfigFileSection),
	 toolSelectionMenuFactory(0),
	 toolMenuPopup(0),toolMenu(0),
	 toolKillZone(0),
	 activeToolSelectionMenuTool(0)
	{
	typedef std::vector<std::string> StringList;
	
	/* Get additional search paths from configuration file section and add them to the factory manager: */
	StringList toolSearchPaths=configFileSection.retrieveValue<StringList>("./toolSearchPaths",StringList());
	for(StringList::const_iterator tspIt=toolSearchPaths.begin();tspIt!=toolSearchPaths.end();++tspIt)
		{
		/* Add the path: */
		getDsoLocator().addPath(*tspIt);
		}
	
	/* Instantiate basic tool classes: */
	addClass(new LocatorToolFactory(*this),destroyToolFactoryFunction);
	addClass(new DraggingToolFactory(*this),destroyToolFactoryFunction);
	addClass(new NavigationToolFactory(*this),destroyToolFactoryFunction);
	addClass(new SurfaceNavigationToolFactory(*this),destroyToolFactoryFunction);
	addClass(new TransformToolFactory(*this),destroyToolFactoryFunction);
	addClass(new UserInterfaceToolFactory(*this),destroyToolFactoryFunction);
	addClass(new MenuToolFactory(*this),destroyToolFactoryFunction);
	addClass(new InputDeviceToolFactory(*this),destroyToolFactoryFunction);
	addClass(new PointingToolFactory(*this),destroyToolFactoryFunction);
	addClass(new UtilityToolFactory(*this),destroyToolFactoryFunction);
	
	/* Load default tool classes: */
	StringList toolClassNames=configFileSection.retrieveValue<StringList>("./toolClassNames");
	for(StringList::const_iterator tcnIt=toolClassNames.begin();tcnIt!=toolClassNames.end();++tcnIt)
		{
		/* Load tool class: */
		loadClass(tcnIt->c_str());
		}
	
	/* Call the input device creation callback for all already existing devices: */
	int numInputDevices=inputDeviceManager->getNumInputDevices();
	for(int deviceIndex=0;deviceIndex<numInputDevices;++deviceIndex)
		{
		InputDeviceManager::InputDeviceCreationCallbackData cbData(inputDeviceManager->getInputDevice(deviceIndex));
		inputDeviceCreationCallback(&cbData);
		}
	
	/* Register callbacks with the input device manager: */
	inputDeviceManager->getInputDeviceCreationCallbacks().add(this,&ToolManager::inputDeviceCreationCallback);
	inputDeviceManager->getInputDeviceDestructionCallbacks().add(this,&ToolManager::inputDeviceDestructionCallback);
	
	/* Initialize the tool kill zone: */
	std::string killZoneType=configFileSection.retrieveString("./killZoneType");
	if(killZoneType=="Box")
		toolKillZone=new ToolKillZoneBox(configFileSection);
	else if(killZoneType=="Frustum")
		toolKillZone=new ToolKillZoneFrustum(configFileSection);
	else
		Misc::throwStdErr("ToolManager: Unknown kill zone type \"%s\"",killZoneType.c_str());
	}

ToolManager::~ToolManager(void)
	{
	/* Destroy tool kill zone: */
	delete toolKillZone;
	
	/* Unregister callbacks from the input device manager: */
	inputDeviceManager->getInputDeviceCreationCallbacks().remove(this,&ToolManager::inputDeviceCreationCallback);
	inputDeviceManager->getInputDeviceDestructionCallbacks().remove(this,&ToolManager::inputDeviceDestructionCallback);
	
	/* Delete all tools: */
	for(ToolList::iterator tIt=tools.begin();tIt!=tools.end();++tIt)
		{
		/* Call tool destruction callbacks: */
		ToolDestructionCallbackData cbData(*tIt);
		toolDestructionCallbacks.call(&cbData);
		
		/* Remove the tool from the input graph: */
		inputGraphManager->removeTool(*tIt);
		
		/* Delete the tool: */
		(*tIt)->getFactory()->destroyTool(*tIt);
		}
	
	/* The automatic destructors will take care of the tool list and the list of tool assignment slots */
	
	/* Delete tool menu: */
	delete toolMenu;
	delete toolMenuPopup;
	}

void ToolManager::defaultToolFactoryDestructor(ToolFactory* factory)
	{
	delete factory;
	}

Misc::ConfigurationFileSection ToolManager::getToolClassSection(const char* toolClassName) const
	{
	/* Return the section of the same name under the tool manager's section: */
	return configFileSection.getSection(toolClassName);
	}

Tool* ToolManager::assignTool(ToolFactory* factory,const ToolInputAssignment& tia)
	{
	/* Create tool of given class: */
	Tool* newTool=factory->createTool(tia);
	
	/* Mark tool's buttons and valuators as assigned (and destroy any tool selection tools still attached): */
	const ToolInputLayout& layout=factory->getLayout();
	for(int deviceIndex=0;deviceIndex<layout.getNumDevices();++deviceIndex)
		{
		InputDevice* device=tia.getDevice(deviceIndex);
		
		/* Mark all button assignments: */
		for(int buttonIndex=0;buttonIndex<layout.getNumButtons(deviceIndex);++buttonIndex)
			{
			int deviceButtonIndex=tia.getButtonIndex(deviceIndex,buttonIndex);
			for(ToolAssignmentSlotList::iterator tasIt=toolAssignmentSlots.begin();tasIt!=toolAssignmentSlots.end();++tasIt)
				if(tasIt->isForButton(device,deviceButtonIndex))
					{
					/* Remove any old tools from the assignment slot: */
					if(tasIt->tool!=0)
						destroyTool(tasIt->tool);
					
					/* Assign the new tool to the assignment slot: */
					tasIt->assigned=true;
					tasIt->tool=newTool;
					break;
					}
			}
		
		/* Mark all valuator assignments: */
		for(int valuatorIndex=0;valuatorIndex<layout.getNumValuators(deviceIndex);++valuatorIndex)
			{
			int deviceValuatorIndex=tia.getValuatorIndex(deviceIndex,valuatorIndex);
			for(ToolAssignmentSlotList::iterator tasIt=toolAssignmentSlots.begin();tasIt!=toolAssignmentSlots.end();++tasIt)
				if(tasIt->isForValuator(device,deviceValuatorIndex))
					{
					/* Remove any old tools from the assignment slot: */
					if(tasIt->tool!=0)
						destroyTool(tasIt->tool);
					
					/* Assign the new tool to the assignment slot: */
					tasIt->assigned=true;
					tasIt->tool=newTool;
					break;
					}
			}
		}
	
	/* Add tool to list: */
	tools.push_back(newTool);
	
	/* Add tool to the input graph: */
	inputGraphManager->addTool(newTool);
	
	/* Call tool creation callbacks: */
	ToolCreationCallbackData cbData(newTool);
	toolCreationCallbacks.call(&cbData);
	
	/* Initialize the tool: */
	newTool->initialize();
	
	/* Assign main menu to tool if it is a menu tool and has no menu yet: */
	MenuTool* menuTool=dynamic_cast<MenuTool*>(newTool);
	if(menuTool!=0&&menuTool->getMenu()==0)
		menuTool->setMenu(getMainMenu());
	
	return newTool;
	}

void ToolManager::loadToolBinding(const char* toolSectionName)
	{
	/* Go to tool's section: */
	Misc::ConfigurationFileSection toolSection=configFileSection.getSection(toolSectionName);
	
	/* Get pointer to factory for tool's class: */
	ToolFactory* factory=loadClass(toolSection.retrieveString("./toolClass").c_str());
	const ToolInputLayout& layout=factory->getLayout();
	
	/* Gather tool's input device assignments: */
	ToolInputAssignment tia(layout);
	for(int deviceIndex=0;deviceIndex<layout.getNumDevices();++deviceIndex)
		{
		/* Get pointer to assigned input device: */
		char deviceNameTag[40];
		snprintf(deviceNameTag,sizeof(deviceNameTag),"./deviceName%d",deviceIndex);
		std::string deviceName=toolSection.retrieveString(deviceNameTag);
		InputDevice* device=inputDeviceManager->findInputDevice(deviceName.c_str());
		if(device==0)
			Misc::throwStdErr("ToolManager::loadToolBinding: Input device %s not found",deviceName.c_str());
		
		/* Assign device: */
		tia.setDevice(deviceIndex,device);
		
		/* Initialize button index assignments: */
		char deviceButtonIndexBaseTag[40];
		snprintf(deviceButtonIndexBaseTag,sizeof(deviceButtonIndexBaseTag),"./device%dButtonIndexBase",deviceIndex);
		int deviceButtonIndexBase=toolSection.retrieveValue<int>(deviceButtonIndexBaseTag,0);
		for(int buttonIndex=0;buttonIndex<layout.getNumButtons(deviceIndex);++buttonIndex)
			{
			/* Get index of assigned button: */
			char deviceButtonIndexTag[40];
			snprintf(deviceButtonIndexTag,sizeof(deviceButtonIndexTag),"./device%dButtonIndex%d",deviceIndex,buttonIndex);
			int deviceButtonIndex=toolSection.retrieveValue<int>(deviceButtonIndexTag,deviceButtonIndexBase+buttonIndex);
			if(deviceButtonIndex>=device->getNumButtons())
				Misc::throwStdErr("ToolManager::loadToolBinding: Button index %d out of range for input device %s",deviceButtonIndex,deviceName.c_str());
			
			/* Check if button is already assigned: */
			for(ToolAssignmentSlotList::const_iterator tasIt=toolAssignmentSlots.begin();tasIt!=toolAssignmentSlots.end();++tasIt)
				if(tasIt->isForButton(device,deviceButtonIndex)&&tasIt->assigned)
					Misc::throwStdErr("ToolManager::loadToolBinding: Button %d on input device %s already assigned",deviceButtonIndex,deviceName.c_str());
			
			/* Assign button: */
			tia.setButtonIndex(deviceIndex,buttonIndex,deviceButtonIndex);
			}
		
		/* Initialize valuator index assignments: */
		char deviceValuatorIndexBaseTag[40];
		snprintf(deviceValuatorIndexBaseTag,sizeof(deviceValuatorIndexBaseTag),"./device%dValuatorIndexBase",deviceIndex);
		int deviceValuatorIndexBase=toolSection.retrieveValue<int>(deviceValuatorIndexBaseTag,0);
		for(int valuatorIndex=0;valuatorIndex<layout.getNumValuators(deviceIndex);++valuatorIndex)
			{
			/* Get index of assigned valuator: */
			char deviceValuatorIndexTag[40];
			snprintf(deviceValuatorIndexTag,sizeof(deviceValuatorIndexTag),"./device%dValuatorIndex%d",deviceIndex,valuatorIndex);
			int deviceValuatorIndex=toolSection.retrieveValue<int>(deviceValuatorIndexTag,deviceValuatorIndexBase+valuatorIndex);
			if(deviceValuatorIndex>=device->getNumValuators())
				Misc::throwStdErr("ToolManager::loadToolBinding: Valuator index %d out of range for input device %s",deviceValuatorIndex,deviceName.c_str());
			
			/* Check if valuator is already assigned: */
			for(ToolAssignmentSlotList::const_iterator tasIt=toolAssignmentSlots.begin();tasIt!=toolAssignmentSlots.end();++tasIt)
				if(tasIt->isForValuator(device,deviceValuatorIndex)&&tasIt->assigned)
					Misc::throwStdErr("ToolManager::loadToolBinding: Valuator %d on input device %s already assigned",deviceValuatorIndex,deviceName.c_str());
			
			/* Assign valuator: */
			tia.setValuatorIndex(deviceIndex,valuatorIndex,deviceValuatorIndex);
			}
		}
	
	/* Create tool of given class: */
	assignTool(factory,tia);
	}

void ToolManager::loadDefaultTools(void)
	{
	/* Configure initial tool assignment: */
	typedef std::vector<std::string> StringList;
	StringList toolNames=configFileSection.retrieveValue<StringList>("./toolNames");
	for(StringList::const_iterator tnIt=toolNames.begin();tnIt!=toolNames.end();++tnIt)
		{
		try
			{
			/* Load the tool binding: */
			loadToolBinding(tnIt->c_str());
			}
		catch(std::runtime_error err)
			{
			/* Print a warning message and carry on: */
			std::cout<<"ToolManager::loadDefaultTools: Ignoring tool binding "<<*tnIt<<" due to exception "<<err.what()<<std::endl;
			}
		}
	
	/* Get factory for tool selection menu tools: */
	toolSelectionMenuFactory=loadClass(configFileSection.retrieveString("./toolSelectionMenuToolClass").c_str());
	if(!toolSelectionMenuFactory->isDerivedFrom("MenuTool"))
		Misc::throwStdErr("ToolManager::loadDefaultTools: Tool selection menu tool class is not a menu tool class");
	const ToolInputLayout& menuToolLayout=toolSelectionMenuFactory->getLayout();
	if(menuToolLayout.getNumDevices()!=1||menuToolLayout.getNumButtons(0)!=1||menuToolLayout.getNumValuators(0)!=0)
		Misc::throwStdErr("ToolManager::loadDefaultTools: Tool selection menu tool class has wrong input layout");
	
	/* Create tool selection menu: */
	toolMenuPopup=createToolMenu();
	toolMenu=new MutexMenu(toolMenuPopup);
	
	/* Assign tool selection menu to all unassigned buttons: */
	assignToolSelectionTools();
	}

void ToolManager::update(void)
	{
	/* Process the tool management queue: */
	for(ToolManagementQueue::iterator tmqIt=toolManagementQueue.begin();tmqIt!=toolManagementQueue.end();++tmqIt)
		{
		switch(tmqIt->itemFunction)
			{
			case ToolManagementQueueItem::CREATE_TOOL:
				{
				/* Create the new tool: */
				assignTool(tmqIt->createToolFactory,*tmqIt->tia);
				
				/* Clean up: */
				delete tmqIt->tia;
				break;
				}
			
			case ToolManagementQueueItem::DESTROY_TOOL:
				{
				Tool* tool=tmqIt->tas->tool;
				
				if(tool!=0)
					{
					/* Unassign and destroy the tool currently assigned to the slot: */
					unassignTool(tool);
					destroyTool(tool);
					
					/* Assign tool selection tools to all freed slots: */
					assignToolSelectionTools();
					}
				break;
				}
			}
		}
	
	/* Clear the tool management queue: */
	toolManagementQueue.clear();
	}

void ToolManager::glRenderAction(GLContextData& contextData) const
	{
	/* Render the tool kill zone: */
	toolKillZone->glRenderAction(contextData);
	}

bool ToolManager::doesButtonHaveTool(const InputDevice* device,int buttonIndex) const
	{
	bool result=false;
	
	/* Find the tool assignment slot for the given button on the given device: */
	for(ToolAssignmentSlotList::const_iterator tasIt=toolAssignmentSlots.begin();tasIt!=toolAssignmentSlots.end();++tasIt)
		if(tasIt->isForButton(device,buttonIndex))
			{
			result=tasIt->assigned;
			break;
			}
	
	return result;
	}

bool ToolManager::doesValuatorHaveTool(const InputDevice* device,int valuatorIndex) const
	{
	bool result=false;
	
	/* Find the tool assignment slot for the given valuator on the given device: */
	for(ToolAssignmentSlotList::const_iterator tasIt=toolAssignmentSlots.begin();tasIt!=toolAssignmentSlots.end();++tasIt)
		if(tasIt->isForValuator(device,valuatorIndex))
			{
			result=tasIt->assigned;
			break;
			}
	
	return result;
	}

}
