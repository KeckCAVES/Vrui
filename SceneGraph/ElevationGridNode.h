/***********************************************************************
ElevationGridNode - Class for quad-based height fields as renderable
geometry.
Copyright (c) 2009-2011 Oliver Kreylos

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

/* Forward declarations: */
namespace Cluster {
class Multiplexer;
}

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
	SFPoint origin;
	SFInt xDimension;
	SFFloat xSpacing;
	SFInt zDimension;
	SFFloat zSpacing;
	MFFloat height;
	MFString heightUrl; // URL of external file(s) containing the height array in several file formats
	MFString heightUrlFormat; // Format of external height file: "BIL", "ARC/INFO ASCII GRID"
	SFBool heightIsY;
	SFBool ccw;
	SFBool solid;
	
	/* Derived state: */
	protected:
	Cluster::Multiplexer* multiplexer; // Pointer to a multicast pipe multiplexer when parsing VRML files in a cluster environment
	bool valid; // Flag whether the elevation grid has a valid renderable representation
	bool indexed; // Flag whether the elevation grid is represented as a set of indexed quad strips or a set of quads
	unsigned int version; // Version number of elevation grid
	
	/* Private methods: */
	Vector calcVertexNormal(int x,int z) const; // Calculates a vertex' normal vector using central differencing
	void uploadIndexedQuadStripSet(void) const; // Uploads the elevation grid as a set of indexed quad strips
	void uploadQuadSet(void) const; // Uploads the elevation grid as a set of quads
	
	/* Constructors and destructors: */
	public:
	ElevationGridNode(void); // Creates a default elevation grid
	
	/* Methods from Node: */
	static const char* getStaticClassName(void);
	virtual const char* getClassName(void) const;
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
