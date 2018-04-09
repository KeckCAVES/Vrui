/***********************************************************************
XMLDocument - Class representing the structure and contents of an XML
entity as a tree.
Copyright (c) 2018 Oliver Kreylos

This file is part of the I/O Support Library (IO).

The I/O Support Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The I/O Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the I/O Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <IO/XMLDocument.h>

#include <stdexcept>
#include <Misc/SelfDestructPointer.h>
#include <IO/Directory.h>
#include <IO/UTF8.h>
#include <IO/XMLSource.h>

namespace IO {

/************************
Methods of class XMLNode:
************************/

XMLNode::~XMLNode(void)
	{
	/* Nothing to do, incidentally: */
	}

/****************************
Methods of class XMLNodeList:
****************************/

XMLNodeList::~XMLNodeList(void)
	{
	/* Delete all nodes: */
	while(head!=0)
		{
		/* Delete the head and make its younger sibling the new head: */
		XMLNode* sibling=head->sibling;
		delete head;
		head=sibling;
		}
	}

unsigned int XMLNodeList::size(void) const
	{
	/* Count the number of nodes in the list: */
	unsigned int result=0;
	for(const XMLNode* nPtr=head;nPtr!=0;nPtr=nPtr->sibling,++result)
		;
	
	return result;
	}

const XMLNode* XMLNodeList::operator[](unsigned int index) const
	{
	/* Find the node of the given index: */
	const XMLNode* nPtr;
	for(nPtr=head;nPtr!=0&&index>0;nPtr=nPtr->sibling,--index)
		;
	if(nPtr==0)
		throw std::runtime_error("IO::XMLNodelist::operator[]: Index out of bounds");
	
	return nPtr;
	}

XMLNode* XMLNodeList::operator[](unsigned int index)
	{
	/* Find the node of the given index: */
	XMLNode* nPtr;
	for(nPtr=head;nPtr!=0&&index>0;nPtr=nPtr->sibling,--index)
		;
	if(nPtr==0)
		throw std::runtime_error("IO::XMLNodelist::operator[]: Index out of bounds");
	
	return nPtr;
	}

void XMLNodeList::push_back(XMLNode* node)
	{
	/* Find the last node: */
	XMLNode* pred=0;
	for(XMLNode* nPtr=head;nPtr!=0;pred=nPtr,nPtr=nPtr->sibling)
		;
	
	/* Link the new node: */
	if(pred!=0)
		{
		/* Link the node after the predecessor: */
		node->sibling=pred->sibling;
		pred->sibling=node;
		}
	else
		{
		/* Link the node as this node's first element: */
		node->sibling=head;
		head=node;
		}
	}

void XMLNodeList::insert(unsigned int index,XMLNode* node)
	{
	/* Find the node preceding the given index: */
	XMLNode* pred=0;
	for(XMLNode* nPtr=head;nPtr!=0&&index>0;pred=nPtr,nPtr=nPtr->sibling,--index)
		;
	
	/* Check if the index was valid: */
	if(index>0)
		throw std::runtime_error("IO::XMLNodelist::insert: Index out of bounds");
		
	/* Link the new node: */
	if(pred!=0)
		{
		/* Link the node after the predecessor: */
		node->sibling=pred->sibling;
		pred->sibling=node;
		}
	else
		{
		/* Link the node as this node's first element: */
		node->sibling=head;
		head=node;
		}
	}

XMLNode* XMLNodeList::pop_back(void)
	{
	/* Check if the list is empty: */
	if(head==0)
		throw std::runtime_error("IO::XMLNodeList::pop_back: List is empty");
	
	/* Find the node preceding the last node: */
	XMLNode* pred=0;
	XMLNode* nPtr;
	for(nPtr=head;nPtr->sibling!=0;pred=nPtr,nPtr=nPtr->sibling)
		;
	
	/* Unlink the found node: */
	if(pred!=0)
		pred->sibling=nPtr->sibling;
	else
		head=nPtr->sibling;
	
	/* Make the unlinked node a single and return it: */
	nPtr->sibling=0;
	return nPtr;
	}

XMLNode* XMLNodeList::erase(unsigned int index)
	{
	/* Find the node preceding the node of the given index: */
	XMLNode* pred=0;
	XMLNode* nPtr;
	for(nPtr=head;nPtr!=0&&index>0;pred=nPtr,nPtr=nPtr->sibling,--index)
		;
	
	/* Check if the index was valid: */
	if(index>0)
		throw std::runtime_error("IO::XMLNodelist::erase: Index out of bounds");
	
	/* Unlink the found node: */
	if(pred!=0)
		pred->sibling=nPtr->sibling;
	else
		head=nPtr->sibling;
	
	/* Make the unlinked node a single and return it: */
	nPtr->sibling=0;
	return nPtr;
	}

void XMLNodeList::erase(XMLNode* node)
	{
	/* Find the node preceding the given node: */
	XMLNode* pred=0;
	XMLNode* nPtr;
	for(nPtr=head;nPtr!=0&&nPtr!=node;pred=nPtr,nPtr=nPtr->sibling)
		;
	
	/* Check if the node was found: */
	if(nPtr!=0)
		{
		/* Unlink the found node: */
		if(pred!=0)
			pred->sibling=nPtr->sibling;
		else
			head=nPtr->sibling;
		}
	
	/* Make the given node a single: */
	node->sibling=0;
	}

/*********************************
Methods of class XMLCharacterData:
*********************************/

XMLCharacterData::XMLCharacterData(XMLSource& source)
	{
	/* Read the character data: */
	source.readUTF8(data);
	}

bool XMLCharacterData::isSpace(void) const
	{
	/* Check all characters: */
	std::string::const_iterator dIt;
	for(dIt=data.begin();dIt!=data.end()&&isSpace(*dIt);++dIt)
		;
	
	return dIt==data.end();
	}

/***************************
Methods of class XMLComment:
***************************/

XMLComment::XMLComment(XMLSource& source)
	{
	/* Read the comment: */
	source.readUTF8(comment);
	}

/*****************************************
Methods of class XMLProcessingInstruction:
*****************************************/

XMLProcessingInstruction::XMLProcessingInstruction(XMLSource& source)
	{
	/* Read the processing instruction target and instruction: */
	source.readUTF8(target);
	source.readUTF8(instruction);
	}

/***************************
Methods of class XMLElement:
***************************/

XMLElement::XMLElement(XMLSource& source)
	:attributes(5)
	{
	/* Read the element name: */
	source.readUTF8(name);
	
	/* Read all attribute/value pairs: */
	while(source.isAttributeName())
		{
		/* Read the name/value pair and store it in the attribute map: */
		std::string name,value;
		source.readUTF8(name);
		source.readUTF8(value);
		attributes[name]=value;
		}
	
	/* Check if the tag has content and a closing tag: */
	empty=source.wasSelfClosingTag();
	if(!empty)
		{
		/* Read the element's content: */
		while(true)
			{
			/* Proceed based on the type of the current syntactic element: */
			XMLNode* node=0;
			if(source.isCharacterData())
				{
				/* Read a character data node: */
				node=new XMLCharacterData(source);
				}
			else if(source.isComment())
				{
				/* Read a comment node: */
				node=new XMLComment(source);
				}
			else if(source.isPITarget())
				{
				/* Read a processing instruction node: */
				node=new XMLProcessingInstruction(source);
				}
			else if(source.isTagName())
				{
				/* Check if this is an opening tag: */
				if(source.isOpeningTag())
					{
					/* Read an element node: */
					node=new XMLElement(source);
					}
				else
					{
					/* Check that the closing tag matches this element's name: */
					std::string tagName;
					source.readUTF8(tagName);
					if(tagName!=name)
						throw XMLSource::WellFormedError(source,"Mismatching closing tag name");
					
					/* Stop reading content: */
					break;
					}
				}
			else if(source.eof())
				throw XMLSource::WellFormedError(source,"Unterminated element");
			
			/* Add the content node as a child of this node: */
			getChildren().push_back(node);
			}
		}
	}

void XMLElement::setAttributeValue(const std::string& attributeName,const std::string attributeValue)
	{
	/* Add or replace the name/value association in the map: */
	attributes[attributeName]=attributeValue;
	}

void XMLElement::removeAttribute(const std::string& attributeName)
	{
	/* Remove the name/value association from the map: */
	attributes.removeEntry(attributeName);
	}

const XMLElement* XMLElement::findNextElement(const char* name,const XMLNode* afterChild) const
	{
	/* Get the first node to check: */
	const XMLNode* cPtr=afterChild!=0?afterChild->getSibling():getChildren().front();
	while(cPtr!=0)
		{
		/* Check if the child is an element and matches the requested name: */
		const XMLElement* element=dynamic_cast<const XMLElement*>(cPtr);
		if(element!=0&&element->getName()==name)
			return element;
		}
	
	/* No matching element found: */
	return 0;
	}

XMLElement* XMLElement::findNextElement(const char* name,XMLNode* afterChild)
	{
	/* Get the first node to check: */
	XMLNode* cPtr=afterChild!=0?afterChild->getSibling():getChildren().front();
	while(cPtr!=0)
		{
		/* Check if the child is an element and matches the requested name: */
		XMLElement* element=dynamic_cast<XMLElement*>(cPtr);
		if(element!=0&&element->getName()==name)
			return element;
		}
	
	/* No matching element found: */
	return 0;
	}

/****************************
Methods of class XMLDocument:
****************************/

XMLDocument::XMLDocument(IO::Directory& directory,const char* xmlFileName)
	:root(0)
	{
	/* Open the XML file and wrap it in a low-level XML processor: */
	XMLSource source(directory.openFile(xmlFileName));
	
	/* Read comments and processing instructions preceding the root element: */
	while(!source.isTagName())
		{
		/* Proceed based on the type of the current syntactic element: */
		XMLNode* node=0;
		if(source.eof())
			throw XMLSource::WellFormedError(source,"No root element in XML document");
		else if(source.isComment())
			{
			/* Read a comment node: */
			node=new XMLComment(source);
			}
		else if(source.isPITarget())
			{
			/* Read a processing instruction node: */
			node=new XMLProcessingInstruction(source);
			}
		else
			throw XMLSource::WellFormedError(source,"Illegal syntactic element in XML prolog");
		
		/* Add the read node to the prolog: */
		prolog.push_back(node);
		}
	
	/* Check if the tag is an opening tag: */
	if(!source.isOpeningTag())
		throw XMLSource::WellFormedError(source,"Missing opening tag for root element");
	
	/* Read the root element: */
	Misc::SelfDestructPointer<XMLElement> tempRoot(new XMLElement(source));
	
	/* Read comments and processing instructions succeeding the root element: */
	while(!source.eof())
		{
		/* Proceed based on the type of the current syntactic element: */
		XMLNode* node=0;
		if(source.isComment())
			{
			/* Read a comment node: */
			node=new XMLComment(source);
			}
		else if(source.isPITarget())
			{
			/* Read a processing instruction node: */
			node=new XMLProcessingInstruction(source);
			}
		else
			throw XMLSource::WellFormedError(source,"Illegal syntactic element in XML epilog");
		
		/* Add the read node to the epilog: */
		epilog.push_back(node);
		}
	
	/* Retain the root element: */
	root=tempRoot.getTarget();
	}

XMLDocument::~XMLDocument(void)
	{
	/* Delete the root element and its subtree: */
	delete root;
	}

}
