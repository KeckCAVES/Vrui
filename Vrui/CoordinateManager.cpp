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

#include <Vrui/CoordinateTransform.h>

#include <Vrui/CoordinateManager.h>

namespace Vrui {

/******************************************
Static elements of class CoordinateManager:
******************************************/

const char* CoordinateManager::unitNames[]=
	{
	"",
	
	"nanometer","micrometer","millimeter","centimeter","meter","kilometer",
	
	"point","inch","foot","yard","mile",
	
	"Angstrom","potrzebie","astronomical unit","light year","parsec"
	};

const char* CoordinateManager::unitAbbreviations[]=
	{
	"",
	
	"nm","um","mm","cm","m","km",
	
	"pt","in","ft","yd","mi",
	
	"A","pz","au","ly","pc"
	};

const Scalar CoordinateManager::unitInchFactors[]=
	{
	Scalar(1),
	
	Scalar(25.4e6),Scalar(25.4e3),Scalar(25.4),Scalar(25.4e-1),Scalar(25.4e-3),Scalar(25.4e-6),
	
	Scalar(72),Scalar(1),Scalar(1.0/12.0),Scalar(1.0/36.0),Scalar(1.0/(36.0*1760.0)),
	
	Scalar(25.4e7),Scalar(25.4/2.263348517438173216473),Scalar(25.4e-3/149597870691.0),
	Scalar(25.4e-6/9460730472580.8),Scalar(25.4e-3/3.085678e16)
	};

const Scalar CoordinateManager::unitMeterFactors[]=
	{
	Scalar(1.0e3/25.4),
	
	Scalar(1.0e9),Scalar(1.0e6),Scalar(1.0e3),Scalar(1.0e2),Scalar(1.0),Scalar(1.0e-3),
	
	Scalar(1.0e3*72.0/25.4),Scalar(1.0e3/25.4),Scalar(1.0e3/(25.4*12.0)),Scalar(1.0e3/(25.4*36.0)),Scalar(1.0e3/(25.4*36.0*1760.0)),
	
	Scalar(1.0e10),Scalar(1.0e3/2.263348517438173216473),Scalar(1.0/149597870691.0),
	Scalar(1.0e-3/9460730472580.8),Scalar(1.0/3.085678e16)
	};

/**********************************
Methods of class CoordinateManager:
**********************************/

CoordinateManager::CoordinateManager(void)
	:unit(UNKNOWN),
	 unitFactor(1),
	 transform(0)
	{
	}

CoordinateManager::~CoordinateManager(void)
	{
	delete transform;
	}

void CoordinateManager::setUnit(CoordinateManager::Unit newUnit,Scalar newUnitFactor)
	{
	unit=newUnit;
	unitFactor=newUnitFactor;
	}

void CoordinateManager::setCoordinateTransform(CoordinateTransform* newTransform)
	{
	/* Delete the previous coordinate transformation: */
	delete transform;
	
	/* Adopt the new coordinate transformation: */
	transform=newTransform;
	}

}
