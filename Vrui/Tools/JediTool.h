/***********************************************************************
JediTool - Class for tools using light sabers to point out features in a
3D display.
Copyright (c) 2007-2018 Oliver Kreylos

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

#ifndef VRUI_JEDITOOL_INCLUDED
#define VRUI_JEDITOOL_INCLUDED

#include <string>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <Images/BaseImage.h>
#include <Vrui/Geometry.h>
#include <Vrui/TransparentObject.h>

#include <Vrui/PointingTool.h>

/* Forward declarations: */
namespace Vrui {
class Lightsource;
}

namespace Vrui {

class JediTool;

class JediToolFactory:public ToolFactory
	{
	friend class JediTool;
	
	/* Elements: */
	private:
	Scalar lightsaberLength; // Length of light saber billboard
	Scalar lightsaberWidth; // Width of light saber billboard
	Scalar baseOffset; // Amount by how much the light saber billboard is shifted towards the hilt
	ONTransform hiltTransform; // Transformation from the controlling device's transformation to the light saber's hilt
	Scalar hiltLength; // Length of light saber hilt in physical coordinate units
	Scalar hiltRadius; // Radius of light saber hilt in physical coordinate units
	std::string lightsaberImageFileName; // Name of image file containing light saber texture
	unsigned int numLightsources; // Number of OpenGL lightsources to add to the light saber blade to create a glowing effect
	Scalar lightRadius; // Distance in physical coordinate units at which the glow intensity diminishes to 1%
	
	/* Constructors and destructors: */
	public:
	JediToolFactory(ToolManager& toolManager);
	virtual ~JediToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class JediTool:public PointingTool,public GLObject,public TransparentObject
	{
	friend class JediToolFactory;
	
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint textureObjectId; // ID of the light saber texture object
		GLuint hiltVertexBufferId; // ID of the vertex array to render the light saber hilt
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	static JediToolFactory* factory; // Pointer to the factory object for this class
	Images::BaseImage lightsaberImage; // The light saber texture image
	Lightsource** lightsources; // Array of light sources allocated for the light saber blade
	
	/* Transient state: */
	bool active; // Flag if the light saber is active
	double activationTime; // Time at which the light saber was activated
	Point origin[2]; // Origin point of the light saber blade on last and current frame
	Vector axis[2]; // Current light saber blade axis vector on last and current frame
	Scalar length[2]; // Current light saber blade length on last and current frame
	
	/* Constructors and destructors: */
	public:
	JediTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~JediTool(void);
	
	/* Methods from Tool: */
	virtual void initialize(void);
	virtual void deinitialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* Methods from TransparentObject: */
	virtual void glRenderActionTransparent(GLContextData& contextData) const;
	};

}

#endif
