/***********************************************************************
Geoid - Class to represent geoids, actually reference ellipsoids, to
support coordinate system transformations between several spherical or
ellipsoidal coordinate systems commonly used in geodesy.
Copyright (c) 2009 Oliver Kreylos

This file is part of the Templatized Geometry Library (TGL).

The Templatized Geometry Library is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Templatized Geometry Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Templatized Geometry Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#define GEOMETRY_GEOID_IMPLEMENTATION

#ifndef METHODPREFIX
	#ifdef NONSTANDARD_TEMPLATES
		#define METHODPREFIX inline
	#else
		#define METHODPREFIX
	#endif
#endif

#include <Geometry/Geoid.h>

#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Vector.h>
#include <Geometry/Rotation.h>

namespace Geometry {

/**********************
Methods of class Geoid:
**********************/

template <class ScalarParam>
METHODPREFIX
Geoid<ScalarParam>::Geoid(
	void)
	:radius(6378137.0),
	 flatteningFactor(1.0/298.257223563),
	 b(radius*(1.0-flatteningFactor)),
	 e2((2.0-flatteningFactor)*flatteningFactor),
	 ep2(e2/Math::sqr(1.0-flatteningFactor))
	{
	}

template <class ScalarParam>
METHODPREFIX
Geoid<ScalarParam>::Geoid(
	double sRadius,
	double sFlatteningFactor)
	:radius(sRadius),
	 flatteningFactor(sFlatteningFactor),
	 b(radius*(1.0-flatteningFactor)),
	 e2((2.0-flatteningFactor)*flatteningFactor),
	 ep2(e2/Math::sqr(1.0-flatteningFactor))
	{
	}

template <class ScalarParam>
METHODPREFIX
typename Geoid<ScalarParam>::Frame
Geoid<ScalarParam>::geodeticToCartesianFrame(
	const typename Geoid<ScalarParam>::Point& geodeticBase) const
	{
	/* Convert the base point to a translation in Cartesian coordinates: */
	double sLon=Math::sin(double(geodeticBase[0]));
	double cLon=Math::cos(double(geodeticBase[0]));
	double sLat=Math::sin(double(geodeticBase[1]));
	double cLat=Math::cos(double(geodeticBase[1]));
	double elev=double(geodeticBase[2]);
	double chi=Math::sqrt(1.0-e2*sLat*sLat);
	typename Frame::Vector t(Scalar((radius/chi+elev)*cLat*cLon),Scalar((radius/chi+elev)*cLat*sLon),Scalar((radius*(1.0-e2)/chi+elev)*sLat));
	
	/* Create a tangential coordinate frame at the base point, with y pointing north and z pointing up: */
	typename Frame::Rotation r=Frame::Rotation::rotateZ(Scalar(0.5*Math::Constants<double>::pi+double(geodeticBase[0])));
	r*=Frame::Rotation::rotateX(Scalar(0.5*Math::Constants<double>::pi-double(geodeticBase[1])));
	
	return Frame(t,r);
	}

template <class ScalarParam>
METHODPREFIX
typename Geoid<ScalarParam>::Point
Geoid<ScalarParam>::cartesianToGeodetic(
	const typename Geoid<ScalarParam>::Point& cartesian) const
	{
	/* I'm not even pretending to understand this formula; it's from Wikipedia, and I found it fast and accurate: */
	double r2=Math::sqr(double(cartesian[0]))+Math::sqr(double(cartesian[1]));
	double Z2=Math::sqr(double(cartesian[2]));
	double r=Math::sqrt(r2);
	double E2=radius*radius*e2;
	double F=54.0*b*b*Z2;
	double G=r2+(1.0-e2)*Z2-e2*E2;
	double c=(e2*e2*F*r2)/(G*G*G);
	double s=Math::pow(1.0+c+Math::sqrt(c*(c+2.0)),1.0/3.0);
	double P=F/(3.0*Math::sqr(s+1.0/s+1.0)*G*G);
	double Q=Math::sqrt(1.0+2.0*e2*e2*P);
	double ro=-(e2*P*r)/(1.0+Q)+Math::sqrt((radius*radius/2.0)*(1.0+1.0/Q)-((1.0-e2)*P*Z2)/(Q*(1.0+Q))-P*r2/2.0);
	double tmp=Math::sqr(r-e2*ro);
	double U=Math::sqrt(tmp+Z2);
	double V=Math::sqrt(tmp+(1.0-e2)*Z2);
	double zo=(b*b*double(cartesian[2]))/(radius*V);
	
	return Point(Scalar(Math::atan2(double(cartesian[1]),double(cartesian[0]))),Scalar(Math::atan((double(cartesian[2])+ep2*zo)/r)),Scalar(U*(1.0-b*b/(radius*V))));
	}

#if !defined(NONSTANDARD_TEMPLATES)

/***************************************************************
Force instantiation of all standard Geoid classes and functions:
***************************************************************/

template class Geoid<float>;
template class Geoid<double>;

#endif

}
