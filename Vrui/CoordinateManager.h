/***********************************************************************
CoordinateManager - Class to manage the (navigation) coordinate system
of a Vrui application to support system-wide navigation manipulation
interfaces.
Copyright (c) 2007-2008 Oliver Kreylos

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

#ifndef VRUI_COORDINATEMANAGER_INCLUDED
#define VRUI_COORDINATEMANAGER_INCLUDED

#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <Vrui/Geometry.h>

/* Forward declarations: */
namespace Vrui {
class CoordinateTransform;
}

namespace Vrui {

class CoordinateManager
	{
	/* Embedded classes: */
	public:
	enum Unit // Enumerated type for coordinate units
		{
		UNKNOWN=0,
		
		/* Metric units: */
		NANOMETER,MICROMETER,MILLIMETER,CENTIMETER,METER,KILOMETER,
		
		/* Imperial units: */
		POINT,INCH,FOOT,YARD,MILE,
		
		/* Other units: */
		ANGSTROM,POTRZEBIE,ASTRONOMICAL_UNIT,LIGHT_YEAR,PARSEC,
		
		/* List terminator: */
		NUM_COORDINATEUNITS
		};
	
	class CallbackData:public Misc::CallbackData // Base class for coordinate manager events
		{
		/* Constructors and destructors: */
		public:
		CallbackData(void)
			{
			}
		};
	
	class CoordinateTransformChangedCallbackData:public CallbackData // Class for callback data sent when the user coordinate transformation changes
		{
		/* Elements: */
		public:
		CoordinateTransform* oldTransform; // Previous coordinate transformation
		CoordinateTransform* newTransform; // New coordinate transformation (already installed at the time callback is called)
		
		/* Constructors and destructors: */
		CoordinateTransformChangedCallbackData(CoordinateTransform* sOldTransform,CoordinateTransform* sNewTransform)
			:oldTransform(sOldTransform),newTransform(sNewTransform)
			{
			}
		};
	
	/* Elements: */
	private:
	static const char* unitNames[NUM_COORDINATEUNITS]; // Full names of coordinate units for display
	static const char* unitAbbreviations[NUM_COORDINATEUNITS]; // Abbreviated names of coordinate units for display
	static const Scalar unitInchFactors[NUM_COORDINATEUNITS]; // Conversion factors from coordinate units to inches
	static const Scalar unitMeterFactors[NUM_COORDINATEUNITS]; // Conversion factors from coordinate units to meters
	
	/* Current coordinate system settings: */
	Unit unit; // Type of coordinate unit used by application
	Scalar unitFactor; // Multiplication factor for coordinate unit used by application (i.e., one application unit = unitFactor coordinateUnit)
	
	/* Current coordinate transformation: */
	CoordinateTransform* transform; // Coordinate transformation from navigation space to "user interest space," used by measurement tools
	Misc::CallbackList coordinateTransformChangedCallbacks; // List of callbacks to be called when the user coordinate transformation changes
	
	/* Constructors and destructors: */
	public:
	CoordinateManager(void); // Creates coordinate manager with default settings (unknown unit with factor 1)
	~CoordinateManager(void); // Destroys coordinate manager
	
	/* Methods: */
	void setUnit(Unit newUnit,Scalar newUnitFactor); // Sets the application's coordinate unit and scale factor
	const char* getUnitName(void) const // Returns the full name of the current application coordinate unit
		{
		return unitNames[unit];
		}
	const char* getUnitAbbreviation(void) const // Returns the abbreviated name of the current application coordinate unit
		{
		return unitAbbreviations[unit];
		}
	Scalar getUnitInchFactor(void) const // Returns length of an inch in current scaled coordinate units
		{
		return unitInchFactors[unit]/unitFactor;
		}
	Scalar getUnitMeterFactor(void) const // Returns length of a meter in current scaled coordinate units
		{
		return unitMeterFactors[unit]/unitFactor;
		}
	Scalar getUnitFactor(void) const // Returns the unit's multiplication factor
		{
		return unitFactor;
		}
	void setCoordinateTransform(CoordinateTransform* newTransform); // Sets a new coordinate transformation; coordinate manager adopts object
	const CoordinateTransform* getCoordinateTransform(void) const // Returns current coordinate transformation
		{
		return transform;
		}
	CoordinateTransform* getCoordinateTransform(void) // Ditto
		{
		return transform;
		}
	Misc::CallbackList& getCoordinateTransformChangedCallbacks(void) // Returns the list of coordinate transformation change callbacks
		{
		return coordinateTransformChangedCallbacks;
		}
	};

}

#endif
