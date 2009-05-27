/***********************************************************************
GroupNode - Base class for nodes that contain child nodes.
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

#ifndef SCENEGRAPH_GROUPNODE_INCLUDED
#define SCENEGRAPH_GROUPNODE_INCLUDED

#include <vector>
#include <Misc/Autopointer.h>
#include <Geometry/ComponentArray.h>
#include <Geometry/Point.h>
#include <Geometry/Box.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GraphNode.h>

namespace SceneGraph {

class GroupNode:public GraphNode
	{
	/* Embedded classes: */
	protected:
	typedef MF<GraphNodePointer> MFGraphNode;
	
	/* Elements: */
	
	/* Fields: */
	MFGraphNode children; // List of this node's children
	SFPoint bboxCenter; // Center of explicit bounding box
	SFSize bboxSize; // Size of explicit bounding box
	
	/* Derived state: */
	bool haveExplicitBoundingBox; // Flag whether the node has an explicit bounding box
	Box explicitBoundingBox; // The explicit bounding box, if it exists
	
	/* Constructors and destructors: */
	public:
	GroupNode(void); // Creates an empty group node
	
	/* Methods from Node: */
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	
	/* Methods from GraphNode: */
	virtual Box calcBoundingBox(void) const;
	virtual void glRenderAction(GLRenderState& renderState) const;
	
	/* New methods: */
	int getNumChildren(void) const // Returns the number of children of the group node
		{
		return children.getNumValues();
		}
	virtual int addChild(GraphNodePointer newChild); // Adds another child node to the group node; returns new child's index
	GraphNode* getChild(int childIndex) const // Returns a pointer to the child of the given index
		{
		return children.getValue(childIndex).getPointer();
		}
	virtual void removeChild(int childIndex); // Removes the child of the given index from the group node
	virtual void setBoundingBox(const Box& newBoundingBox); // Sets an explicit bounding box
	virtual void unsetBoundingBox(void); // Removes an explicit bounding box
	};

typedef Misc::Autopointer<GroupNode> GroupNodePointer;

}

#endif
