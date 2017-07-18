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

#include <Geometry/ElevationGrid.icpp>

namespace Geometry {

/***********************************************************************
Force instantiation of all standard ElevationGrid classes and functions:
***********************************************************************/

template class ElevationGrid<float,unsigned short>;
template class ElevationGrid<float,signed short>;
template class ElevationGrid<float,unsigned int>;
template class ElevationGrid<float,signed int>;
template class ElevationGrid<float,float>;
template class ElevationGrid<float,double>;
template class ElevationGrid<double,unsigned short>;
template class ElevationGrid<double,signed short>;
template class ElevationGrid<double,unsigned int>;
template class ElevationGrid<double,signed int>;
template class ElevationGrid<double,float>;
template class ElevationGrid<double,double>;

}
