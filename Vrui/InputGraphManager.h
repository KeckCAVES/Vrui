/***********************************************************************
InputGraphManager - Class to maintain the bipartite input device / tool
graph formed by tools being assigned to input devices, and input devices
in turn being grabbed by tools.
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

#ifndef VRUI_INPUTGRAPHMANAGER_INCLUDED
#define VRUI_INPUTGRAPHMANAGER_INCLUDED

#include <vector>
#include <Misc/HashTable.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Geometry.h>
#include <Vrui/GlyphRenderer.h>

/* Forward declarations: */
namespace Vrui {
class InputDevice;
class Tool;
class VirtualInputDevice;
}

namespace Vrui {

class InputGraphManager
	{
	/* Embedded classes: */
	private:
	struct GraphTool;
	
	struct GraphInputDevice // Structure to represent an input device in the input graph
		{
		/* Elements: */
		public:
		InputDevice* device; // Pointer to the input device
		Glyph deviceGlyph; // Glyph used to visualize the device's position and orientation
		int level; // Index of the graph level containing the input device
		bool navigational; // Flag whether this device, if ungrabbed, follows the navigation transformation
		NavTrackerState fromNavTransform; // Transformation from navigation coordinates to device's coordinates while device is in navigational mode
		GraphInputDevice* levelPred; // Pointer to the previous input device in the same graph level
		GraphInputDevice* levelSucc; // Pointer to the next input device in the same graph level
		GraphTool* grabber; // Pointer to the tool currently holding a grab on the input device
		};
	
	struct GraphTool // Structure to represent a tool in the input graph
		{
		/* Elements: */
		public:
		Tool* tool; // Pointer to the tool
		int level; // Index of the graph level containing the tool
		GraphTool* levelPred; // Pointer to the previous tool in the same graph level
		GraphTool* levelSucc; // Pointer to the next tool in the same graph level
		};
	
	typedef Misc::HashTable<InputDevice*,GraphInputDevice*> DeviceMap; // Hash table to map from input devices to graph input devices
	typedef Misc::HashTable<Tool*,GraphTool*> ToolMap; // Hash table to map from tools to graph tools
	
	/* Elements: */
	GlyphRenderer* glyphRenderer; // Pointer to the glyph renderer
	VirtualInputDevice* virtualInputDevice; // Pointer to helper class for handling ungrabbed virtual input devices
	GraphTool inputDeviceManager; // A fake graph tool to grab physical input devices
	DeviceMap deviceMap; // Hash table mapping from input devices to graph input devices
	ToolMap toolMap; // Hash table mapping from tools to graph tools
	int maxGraphLevel; // Maximum level in the input graph that has input devices or tools
	std::vector<GraphInputDevice*> deviceLevels; // Vector of pointers to the first input device in each graph level
	std::vector<GraphTool*> toolLevels; // Vector of pointers to the first tool in each graph level
	
	/* Private methods: */
	void linkInputDevice(GraphInputDevice* gid); // Links a graph input device to its current graph level
	void unlinkInputDevice(GraphInputDevice* gid); // Unlinks a graph input device from its current graph level
	void linkTool(GraphTool* gt); // Links a graph tool to its current graph level
	void unlinkTool(GraphTool* gt); // Unlinks a graph tool from its current graph level
	void growInputGraph(int level); // Grows the input graph to represent the given level
	void shrinkInputGraph(void); // Removes all empty levels from the end of the input graph
	void updateInputGraph(void); // Reorders graph levels after input device grab/release
	
	/* Constructors and destructors: */
	public:
	InputGraphManager(GlyphRenderer* sGlyphRenderer,VirtualInputDevice* sVirtualInputDevice); // Creates an empty input graph manager using the given glyph renderer and virtual input device
	private:
	InputGraphManager(const InputGraphManager& source); // Prohibit copy constructor
	InputGraphManager& operator=(const InputGraphManager& source); // Prohibit assignment operator
	public:
	~InputGraphManager(void);
	
	/* Methods: */
	void addInputDevice(InputDevice* newDevice); // Adds an ungrabbed input device to the graph
	bool isNavigational(InputDevice* device) const; // Returns whether the given device will follow navigation coordinates while ungrabbed
	void setNavigational(InputDevice* device,bool newNavigational); // Sets whether the given device will follow navigation coordinates while ungrabbed
	Glyph& getInputDeviceGlyph(InputDevice* device); // Returns the glyph associated with the given input device
	InputDevice* getFirstInputDevice(void); // Returns pointer to the first ungrabbed input device
	InputDevice* getNextInputDevice(InputDevice* device); // Returns pointer to the next input device in the same level after the given one
	InputDevice* findInputDevice(const Point& position,bool ungrabbedOnly =true); // Finds an ungrabbed input device based on a position in physical coordinates
	InputDevice* findInputDevice(const Ray& ray,bool ungrabbedOnly =true); // Finds an ungrabbed input device based on a ray in physical coordinates
	bool grabInputDevice(InputDevice* device,Tool* grabber); // Allows a tool (or physical layer if tool==0) to grab an input device; returns true on success
	void releaseInputDevice(InputDevice* device,Tool* grabber); // Allows the current grabber of an input device to release the grab
	void removeInputDevice(InputDevice* device); // Removes an input device from the graph
	void addTool(Tool* newTool); // Adds a tool to the input graph, based on its current input assignment
	void removeTool(Tool* tool); // Removes a tool from the input graph
	void update(void); // Updates state of all tools and non-physical input devices in the graph
	void glRenderAction(GLContextData& contextData) const; // Renders current state of all input devices and tools
	};

}

#endif
