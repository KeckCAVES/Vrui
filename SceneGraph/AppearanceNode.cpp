/***********************************************************************
AppearanceNode - Class defining the appearance (material properties,
textures) of a shape node.
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

#include <SceneGraph/AppearanceNode.h>

#include <string.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/*******************************
Methods of class AppearanceNode:
*******************************/

AppearanceNode::AppearanceNode(void)
	{
	}

void AppearanceNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"material")==0)
		{
		vrmlFile.parseSFNode(material);
		}
	else if(strcmp(fieldName,"texture")==0)
		{
		vrmlFile.parseSFNode(texture);
		}
	else if(strcmp(fieldName,"textureTransform")==0)
		{
		vrmlFile.parseSFNode(textureTransform);
		}
	else
		AttributeNode::parseField(fieldName,vrmlFile);
	}

void AppearanceNode::update(void)
	{
	}

void AppearanceNode::setGLState(GLRenderState& renderState) const
	{
	if(material.getValue()!=0)
		material.getValue()->setGLState(renderState);
	else
		{
		renderState.disableMaterials();
		renderState.emissiveColor=GLRenderState::Color(0.0f,0.0f,0.0f);
		}
	
	if(texture.getValue()!=0)
		texture.getValue()->setGLState(renderState);
	else
		renderState.disableTextures();
	}

void AppearanceNode::resetGLState(GLRenderState& renderState) const
	{
	if(material.getValue()!=0)
		material.getValue()->resetGLState(renderState);
	
	if(texture.getValue()!=0)
		texture.getValue()->resetGLState(renderState);
	}

}
