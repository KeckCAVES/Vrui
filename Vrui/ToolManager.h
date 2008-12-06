/***********************************************************************
ToolManager - Class to manage tool classes, and dynamic assignment of
tools to input devices.
Copyright (c) 2004-2005 Oliver Kreylos

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

#ifndef VRUI_TOOLMANAGER_INCLUDED
#define VRUI_TOOLMANAGER_INCLUDED

#include <vector>
#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <Misc/ConfigurationFile.h>
#include <Plugins/FactoryManager.h>
#include <Vrui/InputDevice.h>
#include <Vrui/Tools/Tool.h>

/* Forward declarations: */
class GLContextData;
namespace GLMotif {
class Popup;
class PopupMenu;
}
namespace Vrui {
class InputGraphManager;
class InputDeviceManager;
class MutexMenu;
class ToolKillZone;
}

namespace Vrui {

class ToolManager:public Plugins::FactoryManager<ToolFactory>
	{
	/* Embedded classes: */
	private:
	typedef std::vector<Tool*> ToolList; // Data type for list of tools
	
	class ToolAssignmentSlot // Class to store assignments of tools to input device buttons or valuators
		{
		friend class ToolManager;
		
		/* Embedded classes: */
		public:
		enum SlotType // Enumerated type for assignment slot types
			{
			NONE,BUTTON,VALUATOR
			};
		
		/* Elements: */
		private:
		InputDevice* device; // Pointer to input device
		SlotType slotType; // Type of slot
		int slotIndex; // Index of button or valuator on input device
		bool assigned; // Flag if the button or valuator has an application tool assigned
		Tool* tool; // Pointer to assigned application tool or tool selection tool
		bool preemptedButtonPress; // Flag whether this slot has preempted a button press event and needs to preempt the corresponding button release event
		
		/* Constructors and destructors: */
		public:
		ToolAssignmentSlot(void); // Creates an unassigned slot
		~ToolAssignmentSlot(void); // Removes any callbacks the slot installed
		
		/* Methods: */
		private:
		bool isForButton(InputDevice* queryDevice,int queryButtonIndex) const // Returns true if slot is responsible for given button on given device
			{
			return device==queryDevice&&slotType==BUTTON&&slotIndex==queryButtonIndex;
			}
		bool isForValuator(InputDevice* queryDevice,int queryValuatorIndex) const // Returns true if slot is responsible for given valuator on given device
			{
			return device==queryDevice&&slotType==VALUATOR&&slotIndex==queryValuatorIndex;
			}
		void initialize(InputDevice* sDevice,SlotType sSlotType,int sSlotIndex); // Initializes a slot and installs callbacks
		void inputDeviceButtonCallback(InputDevice::ButtonCallbackData* cbData);
		void inputDeviceValuatorCallback(InputDevice::ValuatorCallbackData* cbData);
		bool pressed(void);
		bool released(void);
		};
	
	friend class ToolAssignmentSlot;
	
	typedef std::list<ToolAssignmentSlot> ToolAssignmentSlotList;
	
	struct ToolManagementQueueItem // Structure for items in the tool management queue
		{
		/* Embedded classes: */
		public:
		enum ItemFunction // Enumerated types for queue item functions
			{
			CREATE_TOOL,DESTROY_TOOL
			};
		
		/* Elements: */
		ItemFunction itemFunction; // Function of this item
		ToolFactory* createToolFactory; // Pointer to a tool factory if the item function is CREATE_TOOL
		ToolInputAssignment* tia; // Pointer to a tool input assignment if the item function is CREATE_TOOL
		ToolAssignmentSlot* tas; // Pointer to the affected tool assignment slot
		};
	
	typedef std::vector<ToolManagementQueueItem> ToolManagementQueue;
	
	public:
	class ToolCreationCallbackData:public Misc::CallbackData // Callback data sent when a tool is created
		{
		/* Elements: */
		public:
		Tool* tool; // Pointer to newly created tool
		
		/* Constructors and destructors: */
		ToolCreationCallbackData(Tool* sTool)
			:tool(sTool)
			{
			}
		};
	
	class ToolDestructionCallbackData:public Misc::CallbackData // Callback data sent when a tool is destroyed
		{
		/* Elements: */
		public:
		Tool* tool; // Pointer to tool to be destroyed
		
		/* Constructors and destructors: */
		ToolDestructionCallbackData(Tool* sTool)
			:tool(sTool)
			{
			}
		};
	
	/* Elements: */
	private:
	InputGraphManager* inputGraphManager; // Pointer to the input graph manager
	InputDeviceManager* inputDeviceManager; // Pointer to input device manager
	Misc::ConfigurationFileSection configFileSection; // The tool manager's configuration file section - valid throughout the manager's entire lifetime
	ToolList tools; // List of currently instantiated tools
	ToolAssignmentSlotList toolAssignmentSlots; // Assignments of tools to input device buttons
	ToolFactory* toolSelectionMenuFactory; // Factory for tool selection menu tools
	GLMotif::PopupMenu* toolMenuPopup; // Hierarchical popup menu for tool selection
	MutexMenu* toolMenu; // Shell for tool selection menu
	ToolKillZone* toolKillZone; // Pointer to tool "kill zone"
	Tool* activeToolSelectionMenuTool; // Pointer to the currently active tool selection tool
	std::vector<ToolAssignmentSlot*> toolCreationSlots; // List of pointers to tool assignment slots that are to be used for creating the new tool
	ToolManagementQueue toolManagementQueue; // Queue of management tasks that have to be performed on the next call to update
	Misc::CallbackList toolCreationCallbacks; // List of callbacks to be called after a new tool has been created
	Misc::CallbackList toolDestructionCallbacks; // List of callbacks to be called before a tool will be destroyed
	
	/* Callback wrappers: */
	static void destroyToolFactoryFunction(ToolFactory* toolFactory); // Destruction function for tool classes added explicitly
	
	/* Private methods: */
	GLMotif::Popup* createToolSubmenu(const Plugins::Factory& factory); // Returns submenu containing all subclasses of the given class
	GLMotif::PopupMenu* createToolMenu(void); // Returns top level of tool selection menu
	Tool* assignToolSelectionTool(ToolAssignmentSlot& tas);
	void assignToolSelectionTools(void); // Assigns tool selection tools to all empty tool assignment slots
	void destroyTool(Tool* tool);
	void unassignTool(Tool* tool);
	void inputDeviceCreationCallback(Misc::CallbackData* cbData); // Callback called when a new input device is created
	void inputDeviceDestructionCallback(Misc::CallbackData* cbData); // Callback called when an input device is destroyed
	void toolActivationCallback(Misc::CallbackData* cbData); // Callback called when a tool selection menu tool becomes active
	void toolDeactivationCallback(Misc::CallbackData* cbData); // Callback called when a tool selection menu tool becomes inactive
	void toolMenuSelectionCallback(Misc::CallbackData* cbData); // Callback called when a tool class is selected from the selection menu
	
	/* Constructors and destructors: */
	public:
	ToolManager(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& sConfigFileSection); // Initializes tool manager by reading given configuration file section
	~ToolManager(void); // Destroys tool manager
	
	/* Methods: */
	Misc::ConfigurationFileSection getToolClassSection(const char* toolClassName) const; // Returns the configuration file section a tool class should use for its initialization
	MutexMenu* getToolMenu(void) // Returns tool menu
		{
		return toolMenu;
		}
	Tool* assignTool(ToolFactory* factory,const ToolInputAssignment& tia); // Assigns a new tool created by the given factory
	void loadDefaultTools(void); // Creates default tool associations
	void update(void); // Called once every frame so that the tool manager has a well-defined place to create new tools
	void glRenderAction(GLContextData& contextData) const; // Renders the tool manager (not the tools)
	Misc::CallbackList& getToolCreationCallbacks(void) // Returns list of tool creation callbacks
		{
		return toolCreationCallbacks;
		}
	Misc::CallbackList& getToolDestructionCallbacks(void) // Returns list of tool destruction callbacks
		{
		return toolDestructionCallbacks;
		}
	ToolKillZone* getToolKillZone(void) // Returns pointer to the tool kill zone
		{
		return toolKillZone;
		}
	};

}

#endif
