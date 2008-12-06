/***********************************************************************
MeshGenerators - Functions to create meshes for several basic polyhedra,
and to load meshes from a variety of file formats.
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

#ifndef MESHGENERATORS_INCLUDED
#define MESHGENERATORS_INCLUDED

#include "PolygonMesh.h"

template <class MeshPoint>
PolygonMesh<MeshPoint>* createTetrahedron(void);
template <class MeshPoint>
PolygonMesh<MeshPoint>* createHexahedron(void);
template <class MeshPoint>
PolygonMesh<MeshPoint>* createOctahedron(void);
template <class MeshPoint>
PolygonMesh<MeshPoint>* createDodecahedron(void);
template <class MeshPoint>
PolygonMesh<MeshPoint>* createIcosahedron(void);
template <class MeshPoint>
PolygonMesh<MeshPoint>* createTeapotahedron(void);
template <class MeshPoint>
PolygonMesh<MeshPoint>* loadMeshfile(const char* meshfileName);
template <class MeshPoint>
PolygonMesh<MeshPoint>* loadGtsMeshfile(const char* gtsMeshfileName);
template <class MeshPoint>
PolygonMesh<MeshPoint>* loadPlyMeshfile(const char* plyMeshfileName);
template <class MeshPoint>
void saveMeshfile(const char* meshfileName,const PolygonMesh<MeshPoint>& mesh);

#ifndef MESHGENERATORS_IMPLEMENTATION
#include "MeshGenerators.cpp"
#endif

#endif
