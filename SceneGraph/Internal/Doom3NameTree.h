/***********************************************************************
Doom3NameTree - Generic class to store named items in a hierarchical
fashion, basically as in a file system directory tree.
Copyright (c) 2007-2010 Oliver Kreylos

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

#ifndef SCENEGRAPH_INTERNAL_DOOM3NAMETREE_INCLUDED
#define SCENEGRAPH_INTERNAL_DOOM3NAMETREE_INCLUDED

#include <string>
#include <vector>

namespace SceneGraph {

template <class LeafNodeValueParam>
class Doom3NameTree
	{
	/* Embedded classes: */
	public:
	typedef LeafNodeValueParam LeafNodeValue; // Type of data to store with every leaf
	
	private:
	struct LeafNode // Structure for leaf nodes in the name tree
		{
		/* Elements: */
		public:
		std::string name; // Name of the leaf node
		LeafNodeValue value; // Value stored with this leaf node
		
		/* Constructors and destructors: */
		LeafNode(const char* sName,const LeafNodeValue& sValue) // Creates a leaf node of the given name
			:name(sName),value(sValue)
			{
			}
		};
	
	struct InteriorNode // Structure for interior nodes in the name tree
		{
		/* Elements: */
		public:
		std::string name; // Name of the interior node
		std::vector<InteriorNode*> childNodes; // List of child nodes that are themselves interior nodes
		std::vector<LeafNode*> leafNodes; // List of child nodes that are leaf nodes
		
		/* Constructors and destructors: */
		InteriorNode(std::string sName) // Creates an interior node of the given name
			:name(sName)
			{
			}
		~InteriorNode(void); // Destroys an interior node and its subtree
		
		/* Methods: */
		template <class TraversalFunctorParam>
		void traverseTree(TraversalFunctorParam& tf) const
			{
			/* Traverse all child interior nodes: */
			for(typename std::vector<InteriorNode*>::const_iterator inIt=childNodes.begin();inIt!=childNodes.end();++inIt)
				{
				tf.enterInteriorNode((*inIt)->name);
				(*inIt)->traverseTree(tf);
				tf.leaveInteriorNode((*inIt)->name);
				}
			
			/* Process all leaf nodes: */
			for(typename std::vector<LeafNode*>::const_iterator lnIt=leafNodes.begin();lnIt!=leafNodes.end();++lnIt)
				tf((*lnIt)->name,(*lnIt)->value);
			}
		template <class TraversalFunctorParam>
		void forEachLeaf(TraversalFunctorParam& tf) const
			{
			/* Traverse all child interior nodes: */
			for(typename std::vector<InteriorNode*>::const_iterator inIt=childNodes.begin();inIt!=childNodes.end();++inIt)
				(*inIt)->forEachLeaf(tf);
			
			/* Process all leaf nodes: */
			for(typename std::vector<LeafNode*>::const_iterator lnIt=leafNodes.begin();lnIt!=leafNodes.end();++lnIt)
				tf((*lnIt)->name,(*lnIt)->value);
			}
		template <class TraversalFunctorParam>
		void forEachLeaf(TraversalFunctorParam& tf)
			{
			/* Traverse all child interior nodes: */
			for(typename std::vector<InteriorNode*>::iterator inIt=childNodes.begin();inIt!=childNodes.end();++inIt)
				(*inIt)->forEachLeaf(tf);
			
			/* Process all leaf nodes: */
			for(typename std::vector<LeafNode*>::iterator lnIt=leafNodes.begin();lnIt!=leafNodes.end();++lnIt)
				tf((*lnIt)->name,(*lnIt)->value);
			}
		};
	
	public:
	class LeafID // Class to give access to leaf nodes to clients of the class
		{
		friend class Doom3NameTree;
		
		/* Elements: */
		private:
		const LeafNode* leaf; // Pointer to the leaf node
		
		/* Constructors and destructors: */
		public:
		LeafID(void) // Creates invalid leaf ID
			:leaf(0)
			{
			}
		private:
		LeafID(const LeafNode* sLeaf) // Creates ID for given leaf
			:leaf(sLeaf)
			{
			}
		
		/* Methods: */
		public:
		bool isValid(void) const // Returns true if the ID corresponds to a valid leaf
			{
			return leaf!=0;
			}
		friend bool operator==(const LeafID& lid1,const LeafID& lid2) // Comparison operator
			{
			return lid1.leaf==lid2.leaf;
			}
		static size_t hash(const LeafID& value,size_t tableSize) // Hash function
			{
			return size_t(value.leaf)%tableSize;
			}
		};
	
	/* Elements: */
	private:
	InteriorNode root; // The root node of the tree
	
	/* Constructors and destructors: */
	public:
	Doom3NameTree(void) // Constructs an empty name tree
		:root("")
		{
		}
	
	/* Methods: */
	LeafID insertLeaf(const char* nodeName,const LeafNodeValue& nodeValue); // Inserts a node into the tree; replaces previous value if node already exists
	LeafID findLeaf(const char* nodeName) const; // Returns the ID of the leaf node of the given name
	const LeafNodeValue& getLeafValue(const LeafID& leafId) const // Returns the value stored with the referenced leaf
		{
		return leafId.leaf->value;
		}
	LeafNodeValue& getLeafValue(const LeafID& leafId) // Ditto
		{
		return const_cast<LeafNode*>(leafId.leaf)->value;
		}
	template <class TraversalFunctorParam>
	void traverseTree(TraversalFunctorParam& tf) const // Traverses the entire tree and calls functor methods upon entering/leaving each interior node below the root and encountering each leaf node
		{
		/* Traverse the root node: */
		root.traverseTree(tf);
		}
	template <class TraversalFunctorParam>
	void forEachLeaf(TraversalFunctorParam& tf) const // Traverses the entire tree and calls functor method for each leaf node
		{
		/* Traverse the root node: */
		root.forEachLeaf(tf);
		}
	template <class TraversalFunctorParam>
	void forEachLeaf(TraversalFunctorParam& tf) // Ditto
		{
		/* Traverse the root node: */
		root.forEachLeaf(tf);
		}
	};

}

#ifndef SCENEGRAPH_INTERNAL_DOOM3NAMETREE_IMPLEMENTATION
#include <SceneGraph/Internal/Doom3NameTree.icpp>
#endif

#endif
