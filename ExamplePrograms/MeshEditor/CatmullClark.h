/***********************************************************************
CatmullClark - Functions to perform Catmull-Clark subdivision on polygon
meshes.
Copyright (c) 2001-2006 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef CATMULLCLARK_INCLUDED
#define CATMULLCLARK_INCLUDED

#include <list>

#include "PolygonMesh.h"

template <class PointType>
PolygonMesh<PointType>& subdividePolyhedron(PolygonMesh<PointType>& mesh);
template <class PointType>
PolygonMesh<PointType>& subdividePolyhedronWithLists(PolygonMesh<PointType>& mesh,std::list<typename PolygonMesh<PointType>::VertexIterator>& vertexPoints,std::list<typename PolygonMesh<PointType>::VertexIterator>& facePoints);
template <class PointType>
PolygonMesh<PointType>& liftingStep(PolygonMesh<PointType>& mesh,std::list<typename PolygonMesh<PointType>::VertexIterator>& vertices,double a,double b);
template <class PointType>
PolygonMesh<PointType>& subdivideCatmullClark(PolygonMesh<PointType>& mesh);
template <class PointType>
PolygonMesh<PointType>& snapCatmullClark(PolygonMesh<PointType>& mesh);

#ifndef CATMULLCLARK_IMPLEMENTATION
#include "CatmullClark.cpp"
#endif

#endif
