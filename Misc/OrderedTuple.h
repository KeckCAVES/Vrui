/***********************************************************************
OrderedTuple - Class for ordered tuples; intended to be used as hash
table keys.
Copyright (c) 2008 Oliver Kreylos

This file is part of the Miscellaneous Support Library (Misc).

The Miscellaneous Support Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Miscellaneous Support Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Miscellaneous Support Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef MISC_ORDEREDTUPLE_INCLUDED
#define MISC_ORDEREDTUPLE_INCLUDED

namespace Misc {

template <int dimensionParam>
class OrderedTuple
	{
	/* Embedded classes: */
	public:
	static const int dimension=dimensionParam; // Dimension of the tuple
	
	/* Elements: */
	private:
	int elements[dimension]; // The tuple's elements
	
	/* Constructors and destructors: */
	public:
	OrderedTuple(void) // Dummy constructor
		{
		}
	OrderedTuple(const int sElements[dimensionParam]) // Construction from C-style array
		{
		/* Copy the source array: */
		for(int i=0;i<dimension;++i)
			elements[i]=sElements[i];
		}
	
	/* Methods: */
	const int* getElements(void) const // Returns element array
		{
		return elements;
		}
	int operator[](int index) const // Returns element
		{
		return elements[index];
		}
	void set(int index,int newElement) // Sets an element to a new value
		{
		elements[index]=newElement;
		}
	static size_t hash(const OrderedTuple& source,size_t tableSize) // Calculates a hash function for the given tuple and table size
		{
		size_t result=size_t(source.elements[0]);
		for(int i=1;i<dimension;++i)
			result=result*10000003+size_t(source.elements[i]);
		return result%tableSize;
		}
	};

/**********************************************
Specialized versions of the OrderedTuple class:
**********************************************/

template <>
class OrderedTuple<2>
	{
	/* Embedded classes: */
	public:
	static const int dimension=2; // Dimension of the tuple
	
	/* Elements: */
	private:
	int elements[dimension]; // The tuple's elements
	
	/* Constructors and destructors: */
	public:
	OrderedTuple(void) // Dummy constructor
		{
		}
	OrderedTuple(int sElement0,int sElement1) // Construction from two elements
		{
		elements[0]=sElement0;
		elements[1]=sElement1;
		}
	OrderedTuple(const int sElements[2]) // Construction from C-style array
		{
		elements[0]=sElements[0];
		elements[1]=sElements[1];
		}
	
	/* Methods: */
	const int* getElements(void) const // Returns element array
		{
		return elements;
		}
	int operator[](int index) const // Returns element
		{
		return elements[index];
		}
	void set(int index,int newElement) // Sets an element to a new value
		{
		elements[index]=newElement;
		}
	static size_t hash(const OrderedTuple& source,size_t tableSize) // Calculates a hash function for the given tuple and table size
		{
		return (size_t(source.elements[0])*10000003+size_t(source.elements[1]))%tableSize;
		}
	};

template <>
class OrderedTuple<3>
	{
	/* Embedded classes: */
	public:
	static const int dimension=3; // Dimension of the tuple
	
	/* Elements: */
	private:
	int elements[dimension]; // The tuple's elements
	
	/* Constructors and destructors: */
	public:
	OrderedTuple(void) // Dummy constructor
		{
		}
	OrderedTuple(int sElement0,int sElement1,int sElement2) // Construction from three elements
		{
		elements[0]=sElement0;
		elements[1]=sElement1;
		elements[2]=sElement2;
		}
	OrderedTuple(const int sElements[3]) // Construction from C-style array
		{
		elements[0]=sElements[0];
		elements[1]=sElements[1];
		elements[2]=sElements[2];
		}
	
	/* Methods: */
	const int* getElements(void) const // Returns element array
		{
		return elements;
		}
	int operator[](int index) const // Returns element
		{
		return elements[index];
		}
	void set(int index,int newElement) // Sets an element to a new value
		{
		elements[index]=newElement;
		}
	static size_t hash(const OrderedTuple& source,size_t tableSize) // Calculates a hash function for the given tuple and table size
		{
		return ((size_t(source.elements[0])*10000003+size_t(source.elements[1]))*10000003+size_t(source.elements[2]))%tableSize;
		}
	};

/********************************
Operations on class OrderedTuple:
********************************/

template <int dimensionParam>
inline bool operator==(const OrderedTuple<dimensionParam>& ot1,const OrderedTuple<dimensionParam>& ot2) // Equality operator
	{
	bool result=true;
	for(int i=0;i<dimensionParam&&result;++i)
		result=ot1[i]==ot2[i];
	return result;
	}

template <int dimensionParam>
inline bool operator!=(const OrderedTuple<dimensionParam>& ot1,const OrderedTuple<dimensionParam>& ot2) // Equality operator
	{
	bool result=false;
	for(int i=0;i<dimensionParam&&!result;++i)
		result=ot1[i]!=ot2[i];
	return result;
	}

template <>
inline bool operator==(const OrderedTuple<2>& ot1,const OrderedTuple<2>& ot2) // Equality operator
	{
	return ot1[0]==ot2[0]&&ot1[1]==ot2[1];
	}

template <>
inline bool operator!=(const OrderedTuple<2>& ot1,const OrderedTuple<2>& ot2) // Equality operator
	{
	return ot1[0]!=ot2[0]||ot1[1]!=ot2[1];
	}

template <>
inline bool operator==(const OrderedTuple<3>& ot1,const OrderedTuple<3>& ot2) // Equality operator
	{
	return ot1[0]==ot2[0]&&ot1[1]==ot2[1]&&ot1[2]==ot2[2];
	}

template <>
inline bool operator!=(const OrderedTuple<3>& ot1,const OrderedTuple<3>& ot2) // Equality operator
	{
	return ot1[0]!=ot2[0]||ot1[1]!=ot2[1]||ot1[2]!=ot2[2];
	}

}

#endif
