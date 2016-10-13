/***********************************************************************
PlaneProjectorTool - Class to create virtual input devices at the
intersection of a device ray and a controllable 2D plane.
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

#ifndef VRUI_PLANEPROJECTORTOOL_INCLUDED
#define VRUI_PLANEPROJECTORTOOL_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Rotation.h>
#include <Vrui/TransformTool.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}

namespace Vrui {

class PlaneProjectorTool;

class PlaneProjectorToolFactory:public ToolFactory
	{
	friend class PlaneProjectorTool;
	
	/* Embedded classes: */
	private:
	struct Configuration // Structure containing tool settings
		{
		/* Elements: */
		public:
		bool snapOrientation; // Flag whether to snap the transformed device's orientation to the projection plane
		
		/* Constructors and destructors: */
		Configuration(void); // Creates a default configuration
		
		/* Methods: */
		void read(const Misc::ConfigurationFileSection& cfs); // Overrides configuration from configuration file section
		void write(Misc::ConfigurationFileSection& cfs) const; // Writes configuration to configuration file section
		};
	
	/* Elements: */
	Configuration configuration; // Default configuration for all tools
	
	/* Constructors and destructors: */
	public:
	PlaneProjectorToolFactory(ToolManager& toolManager);
	virtual ~PlaneProjectorToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class PlaneProjectorTool:public TransformTool
	{
	friend class PlaneProjectorToolFactory;
	
	/* Elements: */
	private:
	static PlaneProjectorToolFactory* factory; // Pointer to the factory object for this class
	PlaneProjectorToolFactory::Configuration config; // The tool configuration
	Point center; // The current center point of the projection plane in navigational coordinates
	Vector normal; // The current normal vector of the projection plane in navigational coordinates
	Rotation rotation; // Rotation from navigational space into current projection plane space
	Point centerPhys; // Ditto in physical coordinates
	Vector normalPhys; // Ditto in physical coordinates
	Rotation rotationPhys; // Ditto in physical coordinates
	
	/* Constructors and destructors: */
	public:
	PlaneProjectorTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~PlaneProjectorTool(void);
	
	/* Methods from Tool: */
	virtual void configure(const Misc::ConfigurationFileSection& configFileSection);
	virtual void storeState(Misc::ConfigurationFileSection& configFileSection) const;
	virtual void initialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

}

#endif
