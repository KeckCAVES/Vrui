/***********************************************************************
ElevationGridNode - Class for quad-based height fields as renderable
geometry.
Copyright (c) 2009 Oliver Kreylos

This file is part of the Simple Scene Graph Renderer (SceneGraph).

The Simple Scene Graph Renderer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Simple Scene Graph Renderer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Simple Scene Graph Renderer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SCENEGRAPH_ELEVATIONGRIDNODE_INCLUDED
#define SCENEGRAPH_ELEVATIONGRIDNODE_INCLUDED

#include <GL/gl.h>
#include <GL/GLObject.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GeometryNode.h>
#include <SceneGraph/TextureCoordinateNode.h>
#include <SceneGraph/ColorNode.h>
#include <SceneGraph/NormalNode.h>

namespace SceneGraph {

class ElevationGridNode:public GeometryNode,public GLObject
	{
	/* Embedded classes: */
	public:
	typedef SF<TextureCoordinateNodePointer> SFTextureCoordinateNode;
	typedef SF<ColorNodePointer> SFColorNode;
	typedef SF<NormalNodePointer> SFNormalNode;
	
	/* Elements: */
	
	protected:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint vertexBufferObjectId; // ID of vertex buffer object containing the vertices, if supported
		GLuint indexBufferObjectId; // ID of index buffer object containing the vertex indices, if supported
		unsigned int version; // Version of point set stored in vertex buffer object
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Fields: */
	public:
	SFTextureCoordinateNode texCoord;
	SFColorNode color;
	SFBool colorPerVertex;
	SFNormalNode normal;
	SFBool normalPerVertex;
	SFFloat creaseAngle;
	SFInt xDimension;
	SFFloat xSpacing;
	SFInt zDimension;
	SFFloat zSpacing;
	MFFloat height;
	SFBool ccw;
	SFBool solid;
	
	/* Derived state: */
	protected:
	unsigned int version; // Version number of elevation grid
	
	/* Private methods: */
	void uploadVerticesSmooth(void) const; // Uploads vertices to create a smooth (normal-per-vertex) elevation grid
	void uploadVerticesFaceted(void) const; // Uploads vertices to create a faceted (normal-per-face) elevation grid
	
	/* Constructors and destructors: */
	public:
	ElevationGridNode(void); // Creates a default elevation grid
	
	/* Methods from Node: */
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	
	/* Methods from GeometryNode: */
	virtual Box calcBoundingBox(void) const;
	virtual void glRenderAction(GLRenderState& renderState) const;
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

}

#endif
