/***********************************************************************
ElevationGrid - Class implementing ray intersection tests with regular
integer-lattice 2D elevation grids embedded in 3D space.
Copyright (c) 2017 Oliver Kreylos

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

#ifndef GEOMETRY_ELEVATIONGRID_INCLUDED
#define GEOMETRY_ELEVATIONGRID_INCLUDED

/* Forward declarations: */
namespace Geometry {
template <class ScalarParam,int dimensionParam>
class Point;
template <class ScalarParam,int dimensionParam>
class Vector;
}

namespace Geometry {

template <class ScalarParam,class ElevationScalarParam>
class ElevationGrid
	{
	/* Embedded classes: */
	public:
	typedef ScalarParam Scalar; // Scalar type for points and vectors
	typedef Geometry::Point<Scalar,3> Point; // Type for affine points
	typedef Geometry::Vector<Scalar,3> Vector; // Type for vectors
	typedef ElevationScalarParam ElevationScalar; // Scalar type of elevations stored in grid
	
	/* Elements: */
	private:
	Scalar foo;
	ElevationScalar bar;
	int size[2]; // Width and height of the attached grid storage
	const ElevationScalar* grid; // Pointer to the base vertex of the attached grid storage
	Scalar elevationMin,elevationMax; // Range of elevations in the attached grid storage, if known
	
	/* Private methods: */
	bool restrictInterval(const Point& p0,const Point& p1,Scalar& lambda0,Scalar& lambda1) const; // Restricts the given line interval to the elevation grid's domain; returns false if result interval is empty
	
	/* Constructors and destructors: */
	public:
	ElevationGrid(void) // Creates an elevation grid with no attached grid storage
		:grid(0)
		{
		}
	ElevationGrid(const int sSize[2],const ElevationScalar* sGrid) // Creates an elevation grid with the given grid storage attached
		:grid(0)
		{
		/* Attach the given grid storage: */
		setGrid(sSize,sGrid);
		}
	ElevationGrid(const int sSize[2],const ElevationScalar* sGrid,Scalar sElevationMin,Scalar sElevationMax) // Creates an elevation grid with the given grid storage attached and the given elevation range
		:grid(0)
		{
		/* Attach the given grid storage with the given elevation range: */
		setGrid(sSize,sGrid,sElevationMin,sElevationMax);
		}
	
	/* Methods: */
	void setGrid(const int sSize[2],const ElevationScalar* sGrid); // Attaches the given grid with no known elevation range
	void setGrid(const int sSize[2],const ElevationScalar* sGrid,Scalar sElevationMin,Scalar sElevationMax); // Ditto, with known elevation range
	Scalar intersectRay(const Point& p0,const Point& p1) const; // Intersects the elevation grid with a ray from the first to the second point; intersection is valid if result in [0, 1)
	};

}

#if defined(GEOMETRY_NONSTANDARD_TEMPLATES) && !defined(GEOMETRY_ELEVATIONGRID_IMPLEMENTATION)
#include <Geometry/ElevationGrid.icpp>
#endif

#endif
