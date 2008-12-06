/***********************************************************************
VRScreen - Class for display screens (fixed and head-mounted) in VR
environments.
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

#ifndef VRUI_VRSCREEN_INCLUDED
#define VRUI_VRSCREEN_INCLUDED

#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/Geometry.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
namespace Vrui {
class InputDevice;
}

namespace Vrui {

class VRScreen
	{
	/* Elements: */
	private:
	char* screenName; // Name for the screen
	bool deviceMounted; // Flag if this screen is attached to an input device
	const InputDevice* device; // Pointer to the input device this screen is attached to
	Scalar screenSize[2]; // Screen width and height in physical units
	ONTransform transform; // Transformation from screen to physical or device coordinates
	ONTransform inverseTransform; // Transformation from physical or device to screen coordinates
	
	/* Constructors and destructors: */
	public:
	VRScreen(void); // Creates uninitialized screen
	~VRScreen(void);
	
	/* Methods: */
	void initialize(const Misc::ConfigurationFileSection& configFileSection); // Initializes screen by reading current section of configuration file
	void attachToDevice(const InputDevice* newDevice); // Attaches the screen to an input device if !=0; otherwise, creates fixed screen
	void setSize(Scalar newWidth,Scalar newHeight); // Adjusts the screen's size in physical units; maintains the current center position
	void setTransform(const ONTransform& newTransform); // Sets the transformation from screen to physical or device coordinates
	const char* getName(void) const // Returns screen's name
		{
		return screenName;
		}
	const Scalar* getScreenSize(void) const // Returns size of screen in physical units
		{
		return screenSize;
		}
	Scalar getWidth(void) const // Returns width of screen in physical units
		{
		return screenSize[0];
		}
	Scalar getHeight(void) const // Returns height of screen in physical units
		{
		return screenSize[1];
		}
	const ONTransform& getTransform(void) const // Returns screen transformation from physical or device coordinates
		{
		return transform;
		}
	ONTransform getScreenTransformation(void) const; // Returns screen transformation from physical coordinates
	void setScreenTransform(void) const; // Sets up OpenGL matrices to render directly onto the screen
	void resetScreenTransform(void) const; // Resets OpenGL matrices back to state before calling setScreenTransform()
	};

}

#endif
