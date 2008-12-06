/***********************************************************************
GeodeticCoordinateTransform - Coordinate transformation class to be used
when navigation space is geocentric Cartesian space, and users are
interested in geodetic coordinates (latitude, longitude, elevation).
Copyright (c) 2008 Oliver Kreylos

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

#include <Misc/ThrowStdErr.h>
#include <Math/Math.h>

#include <Vrui/GeodeticCoordinateTransform.h>

namespace Vrui {

/********************************************
Methods of class GeodeticCoordinateTransform:
********************************************/

GeodeticCoordinateTransform::GeodeticCoordinateTransform(double scaleFactor)
	:radius(6378137.0*scaleFactor),
	 flatteningFactor(1.0/298.257223563),
	 e2((2.0-flatteningFactor)*flatteningFactor),
	 colatitude(false),radians(false),depth(false)
	{
	}

const char* GeodeticCoordinateTransform::getComponentName(int componentIndex) const
	{
	switch(componentIndex)
		{
		case 0:
			return colatitude?"Colatitude":"Latitude";
			break;
		
		case 1:
			return "Longitude";
			break;
		
		case 2:
			return depth?"Depth":"Height";
			break;
		
		default:
			Misc::throwStdErr("GeodeticCoordinateTransform::getComponentName: Invalid component index %d",componentIndex);
			return ""; // Never reached; just to make compiler happy
		}
	}

Point GeodeticCoordinateTransform::transform(const Point& navigationPoint) const
	{
	double spherical[3];
	
	double xy=Math::sqrt(Math::sqr(double(navigationPoint[0]))+Math::sqr(double(navigationPoint[1])));
	spherical[0]=Math::atan2(double(navigationPoint[2]),(1.0-e2)*xy);
	double lats=Math::sin(spherical[0]);
	double nu=radius/Math::sqrt(1.0-e2*Math::sqr(lats));
	for(int i=0;i<6;++i)
		{
		spherical[0]=atan2(double(navigationPoint[2])+e2*nu*lats,xy);
		lats=Math::sin(spherical[0]);
		nu=radius/Math::sqrt(1.0-e2*Math::sqr(lats));
		}
	spherical[1]=Math::atan2(double(navigationPoint[1]),double(navigationPoint[0]));
	if(Math::abs(spherical[0])<=Math::rad(45.0))
		spherical[2]=xy/Math::cos(spherical[0])-nu;
	else
		spherical[2]=double(navigationPoint[2])/lats-(1.0-e2)*nu;
	
	if(colatitude)
		spherical[0]=Math::rad(90.0)-spherical[0];
	if(!radians)
		{
		spherical[0]=Math::deg(spherical[0]);
		spherical[1]=Math::deg(spherical[1]);
		}
	if(depth)
		spherical[2]=-spherical[2];
	
	return Point(spherical);
	}

void GeodeticCoordinateTransform::setGeoid(double newRadius,double newFlatteningFactor)
	{
	/* Set the geoid parameters: */
	radius=newRadius;
	flatteningFactor=newFlatteningFactor;
	
	/* Calculate the derived geoid parameters: */
	e2=(2.0-flatteningFactor)*flatteningFactor;
	}

void GeodeticCoordinateTransform::setColatitude(bool newColatitude)
	{
	colatitude=newColatitude;
	}

void GeodeticCoordinateTransform::setRadians(bool newRadians)
	{
	radians=newRadians;
	}

void GeodeticCoordinateTransform::setDepth(bool newDepth)
	{
	depth=newDepth;
	}

}
