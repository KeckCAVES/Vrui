/***********************************************************************
RenderPolygonMeshGL - Functions to render polygon meshes using direct
OpenGL calls.
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

#ifndef RENDERPOLYGONMESHGL_INCLUDED
#define RENDERPOLYGONMESHGL_INCLUDED

#include <GL/gl.h>
#include <GL/GLVertex.h>
#include <GLTriangleStripSet.h>

/* Forward declarations: */
template <class PointType>
class PolygonMesh;
template <class PointType>
class AutoTriangleMesh;

template <class PointType>
void renderMeshWireframe(const PolygonMesh<PointType>& mesh,GLfloat lineWidth,GLfloat pointSize);
template <class PointType>
void renderMeshTriangles(const AutoTriangleMesh<PointType>& mesh);
template <class PointType>
void renderMeshTrianglesSmooth(const AutoTriangleMesh<PointType>& mesh);
template <class PointType>
void renderMeshTriangleStrips(const PolygonMesh<PointType>& mesh);
template <class PointType>
void renderMeshTriangleStrips(const PolygonMesh<PointType>& mesh,GLTriangleStripSet<GLVertex<void,0,void,0,GLfloat,GLfloat,3> >& triangleStripSet);
template <class PointType>
void renderMeshTriangleStripsOnTheFly(const AutoTriangleMesh<PointType>& mesh,int maxNumStrips =0);

#ifndef RENDERPOLYGONMESHGL_IMPLEMENTATION
#include "RenderPolygonMeshGL.cpp"
#endif

#endif
