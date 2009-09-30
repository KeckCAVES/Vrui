/***********************************************************************
NodeCreator - Class to create node objects based on a node type name.
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

#include <SceneGraph/NodeCreator.h>

#include <SceneGraph/NodeFactory.h>
#include <SceneGraph/GroupNode.h>
#include <SceneGraph/TransformNode.h>
#include <SceneGraph/BillboardNode.h>
#include <SceneGraph/ReferenceEllipsoidNode.h>
#include <SceneGraph/GeodeticToCartesianTransformNode.h>
#include <SceneGraph/InlineNode.h>
#include <SceneGraph/MaterialNode.h>
#include <SceneGraph/ImageTextureNode.h>
#include <SceneGraph/AppearanceNode.h>
#include <SceneGraph/GeodeticToCartesianPointTransformNode.h>
#include <SceneGraph/BoxNode.h>
#include <SceneGraph/ConeNode.h>
#include <SceneGraph/CylinderNode.h>
#include <SceneGraph/TextureCoordinateNode.h>
#include <SceneGraph/ColorNode.h>
#include <SceneGraph/NormalNode.h>
#include <SceneGraph/CoordinateNode.h>
#include <SceneGraph/PointSetNode.h>
#include <SceneGraph/IndexedLineSetNode.h>
#include <SceneGraph/ElevationGridNode.h>
#include <SceneGraph/IndexedFaceSetNode.h>
#include <SceneGraph/ShapeNode.h>
#include <SceneGraph/FontStyleNode.h>
#include <SceneGraph/TextNode.h>
#include <SceneGraph/LabelSetNode.h>
#include <SceneGraph/TSurfFileNode.h>
#include <SceneGraph/ArcInfoExportFileNode.h>
#include <SceneGraph/ESRIShapeFileNode.h>

namespace SceneGraph {

/****************************
Methods of class NodeCreator:
****************************/

NodeCreator::NodeCreator(void)
	:nodeFactoryMap(31)
	{
	/* Register the standard node types: */
	registerNodeType("Group",new GenericNodeFactory<GroupNode>());
	registerNodeType("Transform",new GenericNodeFactory<TransformNode>());
	registerNodeType("Billboard",new GenericNodeFactory<BillboardNode>());
	registerNodeType("ReferenceEllipsoid",new GenericNodeFactory<ReferenceEllipsoidNode>());
	registerNodeType("GeodeticToCartesianTransform",new GenericNodeFactory<GeodeticToCartesianTransformNode>());
	registerNodeType("Inline",new GenericNodeFactory<InlineNode>());
	registerNodeType("Material",new GenericNodeFactory<MaterialNode>());
	registerNodeType("ImageTexture",new GenericNodeFactory<ImageTextureNode>());
	registerNodeType("Appearance",new GenericNodeFactory<AppearanceNode>());
	registerNodeType("GeodeticToCartesianPointTransform",new GenericNodeFactory<GeodeticToCartesianPointTransformNode>());
	registerNodeType("Box",new GenericNodeFactory<BoxNode>());
	registerNodeType("Cone",new GenericNodeFactory<ConeNode>());
	registerNodeType("Cylinder",new GenericNodeFactory<CylinderNode>());
	registerNodeType("TextureCoordinate",new GenericNodeFactory<TextureCoordinateNode>());
	registerNodeType("Color",new GenericNodeFactory<ColorNode>());
	registerNodeType("Normal",new GenericNodeFactory<NormalNode>());
	registerNodeType("Coordinate",new GenericNodeFactory<CoordinateNode>());
	registerNodeType("PointSet",new GenericNodeFactory<PointSetNode>());
	registerNodeType("IndexedLineSet",new GenericNodeFactory<IndexedLineSetNode>());
	registerNodeType("ElevationGrid",new GenericNodeFactory<ElevationGridNode>());
	registerNodeType("IndexedFaceSet",new GenericNodeFactory<IndexedFaceSetNode>());
	registerNodeType("Shape",new GenericNodeFactory<ShapeNode>());
	registerNodeType("FontStyle",new GenericNodeFactory<FontStyleNode>());
	registerNodeType("Text",new GenericNodeFactory<TextNode>());
	registerNodeType("LabelSet",new GenericNodeFactory<LabelSetNode>());
	registerNodeType("TSurfFile",new GenericNodeFactory<TSurfFileNode>());
	registerNodeType("ArcInfoExportFile",new GenericNodeFactory<ArcInfoExportFileNode>());
	registerNodeType("ESRIShapeFile",new GenericNodeFactory<ESRIShapeFileNode>());
	}

NodeCreator::~NodeCreator(void)
	{
	/* Destroy all node factories: */
	for(NodeFactoryMap::Iterator nfmIt=nodeFactoryMap.begin();!nfmIt.isFinished();++nfmIt)
		delete nfmIt->getDest();
	}

void NodeCreator::registerNodeType(const char* nodeTypeName,NodeFactory* nodeFactory)
	{
	nodeFactoryMap.setEntry(NodeFactoryMap::Entry(nodeTypeName,nodeFactory));
	}

Node* NodeCreator::createNode(const char* nodeType)
	{
	NodeFactoryMap::Iterator nfIt=nodeFactoryMap.findEntry(nodeType);
	if(nfIt.isFinished())
		return 0;
	else
		return nfIt->getDest()->createNode();
	}

}
