/***********************************************************************
ScreenMouseTool - Class to create a virtual mouse from two valuators
and a screen rectangle.
Copyright (c) 2015 Oliver Kreylos

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

#ifndef VRUI_SCREENMOUSETOOL_INCLUDED
#define VRUI_SCREENMOUSETOOL_INCLUDED

#include <string>
#include <Misc/FixedArray.h>
#include <Vrui/TransformTool.h>

/* Forward declarations: */
namespace Vrui {
class VRScreen;
}

namespace Vrui {

class ScreenMouseTool;

class ScreenMouseToolFactory:public ToolFactory
	{
	friend class ScreenMouseTool;
	
	/* Embedded classes: */
	private:
	struct Configuration // Structure containing tool settings
		{
		/* Elements: */
		public:
		std::string screenName; // Name of the screen inside which the created device moves
		Scalar deadZone; // Minimum valuator vector length before any motion happens
		Scalar exponent; // Exponent to create non-linear valuator response curves
		Misc::FixedArray<Scalar,2> velocityFactors; // Conversion factors from valuator values to device movement velocities in physical units/second
		
		/* Constructors and destructors: */
		Configuration(void); // Creates default configuration
		
		/* Methods: */
		void load(const Misc::ConfigurationFileSection& cfs); // Overrides configuration from configuration file section
		void save(Misc::ConfigurationFileSection& cfs) const; // Writes configuration to configuration file section
		};
	
	/* Elements: */
	Configuration config; // Default configuration for all tools
	
	/* Constructors and destructors: */
	public:
	ScreenMouseToolFactory(ToolManager& toolManager);
	virtual ~ScreenMouseToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual const char* getValuatorFunction(int valuatorSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class ScreenMouseTool:public TransformTool
	{
	friend class ScreenMouseToolFactory;
	
	/* Elements: */
	private:
	static ScreenMouseToolFactory* factory; // Pointer to the factory object for this class
	
	/* Configuration state: */
	ScreenMouseToolFactory::Configuration config; // The tool configuration
	VRScreen* screen; // Screen inside which the created device moves
	
	/* Transient state: */
	Point screenPos; // Position of virtual input device in screen space
	
	/* Constructors and destructors: */
	public:
	ScreenMouseTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods from Tool: */
	virtual void configure(const Misc::ConfigurationFileSection& configFileSection);
	virtual void storeState(Misc::ConfigurationFileSection& configFileSection) const;
	virtual void initialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void frame(void);
	};

}

#endif
