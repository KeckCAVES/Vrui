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

#include <SceneGraph/GroupNode.h>

#include <string.h>
#include <SceneGraph/VRMLFile.h>

namespace SceneGraph {

/**************************
Methods of class GroupNode:
**************************/

GroupNode::GroupNode(void)
	:bboxCenter(Point::origin),
	 bboxSize(Size(-1,-1,-1)),
	 haveExplicitBoundingBox(false)
	{
	}

void GroupNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"children")==0)
		{
		vrmlFile.parseMFNode(children);
		}
	else if(strcmp(fieldName,"bboxCenter")==0)
		{
		vrmlFile.parseField(bboxCenter);
		}
	else if(strcmp(fieldName,"bboxSize")==0)
		{
		vrmlFile.parseField(bboxSize);
		}
	else
		GraphNode::parseField(fieldName,vrmlFile);
	}

void GroupNode::update(void)
	{
	haveExplicitBoundingBox=bboxSize.getValue()[0]>=Scalar(0)&&bboxSize.getValue()[1]>=Scalar(0)&&bboxSize.getValue()[2]>=Scalar(0);
	if(haveExplicitBoundingBox)
		{
		Point pmin=bboxCenter.getValue();
		Point pmax=bboxCenter.getValue();
		for(int i=0;i<3;++i)
			{
			pmin[i]-=Math::div2(bboxSize.getValue()[i]);
			pmax[i]+=Math::div2(bboxSize.getValue()[i]);
			}
		explicitBoundingBox=Box(pmin,pmax);
		}
	}

Box GroupNode::calcBoundingBox(void) const
	{
	/* Return the explicit bounding box if there is one: */
	if(haveExplicitBoundingBox)
		return explicitBoundingBox;
	else
		{
		/* Calculate the group's bounding box as the union of the children's boxes: */
		Box result=Box::empty;
		for(MFGraphNode::ValueList::const_iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
			result.addBox((*chIt)->calcBoundingBox());
		return result;
		}
	}

void GroupNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Call the render actions of all children in order: */
	for(MFGraphNode::ValueList::const_iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
		(*chIt)->glRenderAction(renderState);
	}

int GroupNode::addChild(GraphNodePointer newChild)
	{
	/* Add the new child to the list: */
	int result=children.getNumValues();
	children.appendValue(newChild);
	return result;
	}

void GroupNode::removeChild(int childIndex)
	{
	children.removeValue(childIndex);
	}

void GroupNode::setBoundingBox(const Box& newBoundingBox)
	{
	/* Set the explicit bounding box: */
	haveExplicitBoundingBox=true;
	explicitBoundingBox=newBoundingBox;
	}

void GroupNode::unsetBoundingBox(void)
	{
	/* Disable the explicit bounding box: */
	haveExplicitBoundingBox=false;
	}

}
