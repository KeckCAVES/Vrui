/***********************************************************************
FPSArmTool - Class to simulate an avatar arm in an FPS-like setting.
Copyright (c) 2014 Oliver Kreylos

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

#ifndef VRUI_FPSARMTOOL_INCLUDED
#define VRUI_FPSARMTOOL_INCLUDED

#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/TransformTool.h>

namespace Vrui {

class FPSArmTool;

class FPSArmToolFactory:public ToolFactory
	{
	friend class FPSArmTool;
	
	/* Embedded classes: */
	private:
	struct Configuration // Structure containing tool settings
		{
		/* Elements: */
		public:
		ONTransform lowPosition; // "Low" virtual device position, i.e., at the hip
		ONTransform highPosition; // "High" virtual device position, i.e., aiming down the sights
		Scalar transitionTime; // Transition time between high and low positions in seconds
		bool followPitch; // Flag whether the virtual device's Y direction follows the main view direction's pitch angle
		bool followYaw; // Flag whether the virtual device's Y direction follows the main view direction's yaw angle
		
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
	FPSArmToolFactory(ToolManager& toolManager);
	virtual ~FPSArmToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class FPSArmTool:public TransformTool
	{
	friend class FPSArmToolFactory;
	
	/* Elements: */
	private:
	static FPSArmToolFactory* factory; // Pointer to the factory object for this class
	
	/* Constructors and destructors: */
	public:
	FPSArmTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~FPSArmTool(void);
	
	/* Methods from class Tool: */
	virtual void configure(const Misc::ConfigurationFileSection& configFileSection);
	virtual void storeState(Misc::ConfigurationFileSection& configFileSection) const;
	virtual void initialize(void); // Called right after a tool has been created and is fully installed
	virtual void deinitialize(void); // Called right before a tool is destroyed during runtime
	virtual const ToolFactory* getFactory(void) const;
	virtual void frame(void);
	};

}

#endif
