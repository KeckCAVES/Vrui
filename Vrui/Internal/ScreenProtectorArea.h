/***********************************************************************
ScreenProtectorArea - Class describing an area of physical space that
needs to be protected from penetration by input devices.
Copyright (c) 2000-2016 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_SCREENPROTECTORAREA_INCLUDED
#define VRUI_INTERNAL_SCREENPROTECTORAREA_INCLUDED

#include <vector>
#include <Misc/ValueCoder.h>
#include <Geometry/Plane.h>
#include <Vrui/Geometry.h>

/* Forward declarations: */
class GLContextData;
namespace Vrui {
class VRScreen;
}

namespace Vrui {

class ScreenProtectorArea
	{
	friend class Misc::ValueCoder<ScreenProtectorArea>;
	
	/* Elements: */
	private:
	Plane plane; // Plane equation of the protected area, with normal vector pointing to inside of accessible half-space
	int axis0,axis1; // Indices of the two primary axes most aligned with the area plane, for point-in-polygon tests
	unsigned int numVertices; // Number of vertices bounding the protected area
	Point* vertices; // Array of vertices, in counter-clockwise order seen from accessible half-space, bounding the protected area
	Vector* edges; // Array of normalized direction vectors of polygon edges
	Scalar* edgeLengths; // Array of lengths of polygon edges
	
	/* Private methods: */
	void init(void); // Calculates derived screen protector data during initialization
	
	/* Constructors and destructors: */
	public:
	ScreenProtectorArea(void); // Creates an empty screen protector area
	ScreenProtectorArea(const VRScreen& screen); // Creates a screen protector area from a display screen
	ScreenProtectorArea(const std::vector<Point>& sVertices); // Creates a screen protector area from a list of polygon vertices
	ScreenProtectorArea(const ScreenProtectorArea& source); // Copy constructor
	ScreenProtectorArea& operator=(const ScreenProtectorArea& source); // Assignment operator
	~ScreenProtectorArea(void); // Destroys this screen protector area
	
	/* Methods: */
	bool isValid(void) const // Returns true if this screen protector area is valid
		{
		return numVertices>0U;
		}
	Scalar calcPenetrationDepth(const Point& center,Scalar radius) const; // Returns a value in (0, 1] if a sphere with the given center and radius penetrates this area; returned value represents level of penetration
	void glRenderAction(Scalar gridLineDist) const; // Draws a visual boundary for this area, with given grid line distance in physical coordinate units
	};

}

namespace Misc {

template <>
class ValueCoder<Vrui::ScreenProtectorArea>
	{
	/* Methods: */
	public:
	static std::string encode(const Vrui::ScreenProtectorArea& value);
	static Vrui::ScreenProtectorArea decode(const char* start,const char* end,const char** decodeEnd =0);
	};

}

#endif
