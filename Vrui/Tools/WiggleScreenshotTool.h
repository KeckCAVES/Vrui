/***********************************************************************
WiggleScreenshotTool - Class for tools to save save a sequence of
screenshots from different viewpoints to generate a "wigglegif."
Copyright (c) 2016 Oliver Kreylos

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

#ifndef VRUI_WIGGLESCREENSHOTTOOL_INCLUDED
#define VRUI_WIGGLESCREENSHOTTOOL_INCLUDED

#include <string>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/UtilityTool.h>

/* Forward declarations: */
namespace Vrui {
class VRWindow;
}

namespace Vrui {

class WiggleScreenshotTool;

class WiggleScreenshotToolFactory:public ToolFactory
	{
	friend class WiggleScreenshotTool;
	
	/* Embedded classes: */
	private:
	struct Configuration // Structure containing tool settings
		{
		/* Elements: */
		public:
		std::string screenshotFileName; // Name of file into which screenshots are saved
		int windowIndex; // Index of master node window from which to save screenshots
		unsigned int numFrames; // Number of wiggle animation frames to produce
		Scalar angleIncrement; // Angle increment between subsequent frames in radians
		
		/* Constructors and destructors: */
		Configuration(void); // Creates default configuration
		
		/* Methods: */
		void read(const Misc::ConfigurationFileSection& cfs); // Overrides configuration from configuration file section
		void write(Misc::ConfigurationFileSection& cfs) const; // Writes configuration to configuration file section
		};
	
	/* Elements: */
	Configuration configuration; // Default configuration for all tools
	
	/* Constructors and destructors: */
	public:
	WiggleScreenshotToolFactory(ToolManager& toolManager);
	virtual ~WiggleScreenshotToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class WiggleScreenshotTool:public UtilityTool
	{
	friend class WiggleScreenshotToolFactory;
	
	/* Elements: */
	private:
	static WiggleScreenshotToolFactory* factory; // Pointer to the factory object for this class
	WiggleScreenshotToolFactory::Configuration configuration; // Private configuration of this tool
	
	/* Master node state: */
	VRWindow* window; // Pointer to the window from which to save screenshots
	
	/* Transient state: */
	NavTransform initialNavTransform; // The navigation transformation at the time a screenshot was requested
	unsigned int frameIndex; // Index of the next wiggle animation frame to save
	
	/* Constructors and destructors: */
	public:
	WiggleScreenshotTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods from Tool: */
	virtual void configure(const Misc::ConfigurationFileSection& configFileSection);
	virtual void storeState(Misc::ConfigurationFileSection& configFileSection) const;
	virtual void initialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	};

}

#endif
