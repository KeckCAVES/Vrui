/***********************************************************************
UnorderedTuple - Class for unordered tuples; intended to be used as hash
table keys. Implemented by sorting tuple elements.
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

#ifndef MISC_UNORDEREDTUPLE_INCLUDED
#define MISC_UNORDEREDTUPLE_INCLUDED

namespace Misc {

template <int dimensionParam>
class UnorderedTuple
	{
	/* Embedded classes: */
	public:
	static const int dimension=dimensionParam; // Dimension of the tuple
	
	/* Elements: */
	private:
	int elements[dimension]; // The tuple's elements, sorted in ascending order
	
	/* Private methods: */
	void sortTuple(void) // Sorts the tuple after a change
		{
		/* Simple insertion sort; dimensions are expected to be small: */
		for(int test=1;test<dimension;++test)
			{
			int testElement=elements[test];
			int insertPos;
			for(insertPos=test;insertPos>0&&testElement<elements[insertPos-1];--insertPos)
				elements[insertPos]=elements[insertPos-1];
			elements[insertPos]=testElement;
			}
		}
	
	/* Constructors and destructors: */
	public:
	UnorderedTuple(void) // Dummy constructor
		{
		}
	UnorderedTuple(const int sElements[dimensionParam) // Construction from C-style array
		{
		/* Copy the source array: */
		for(int i=0;i<dimension;++i)
			elements[i]=sElements[i];
		
		/* Sort the elements: */
		sortTuple();
		}
	
	/* Methods: */
	const int* getElements(void) const // Returns element array
		{
		return elements;
		}
	int operator[](int index) const // Returns element as rvalue
		{
		return elements[index];
		}
	void set(int index,int newElement) // Sets an element to a new value
		{
		elements[index]=newElement;
		
		/* Sort the tuple: */
		sortTuple();
		}
	static size_t hash(const UnorderedTuple& source,size_t tableSize) // Calculates a hash function for the given tuple and table size
		{
		size_t result=size_t(source.elements[0]);
		for(int i=1;i<dimension;++i)
			result=result*10000003+size_t(source.elements[i]);
		return result%tableSize;
		}
	};

/************************************************
Specialized versions of the UnorderedTuple class:
************************************************/

template <>
class UnorderedTuple<2>
	{
	/* Embedded classes: */
	public:
	static const int dimension=2; // Dimension of the tuple
	
	/* Elements: */
	private:
	int elements[dimension]; // The tuple's elements
	
	/* Private methods: */
	void sortTuple(void) // Sorts the tuple after a change
		{
		if(elements[0]>elements[1])
			{
			int temp=elements[0];
			elements[0]=elements[1];
			elements[1]=temp;
			}
		}
	
	/* Constructors and destructors: */
	public:
	UnorderedTuple(void) // Dummy constructor
		{
		}
	UnorderedTuple(int sElement0,int sElement1) // Construction from two elements
		{
		elements[0]=sElement0;
		elements[1]=sElement1;
		
		/* Sort the tuple: */
		sortTuple();
		}
	UnorderedTuple(const int sElements[2]) // Construction from C-style array
		{
		elements[0]=sElements[0];
		elements[1]=sElements[1];
		
		/* Sort the tuple: */
		sortTuple();
		}
	
	/* Methods: */
	const int* getElements(void) const // Returns element array
		{
		return elements;
		}
	int operator[](int index) const // Returns element as rvalue
		{
		return elements[index];
		}
	void set(int index,int newElement) // Sets an element to a new value
		{
		elements[index]=newElement;
		
		/* Sort the tuple: */
		sortTuple();
		}
	static size_t hash(const UnorderedTuple& source,size_t tableSize) // Calculates a hash function for the given tuple and table size
		{
		return (size_t(source.elements[0])*10000003+size_t(source.elements[1]))%tableSize;
		}
	};

template <>
class UnorderedTuple<3>
	{
	/* Embedded classes: */
	public:
	static const int dimension=3; // Dimension of the tuple
	
	/* Elements: */
	private:
	int elements[dimension]; // The tuple's elements
	
	/* Private methods: */
	void sortPair(int i1,int i2) // Sorts a pair of elements; i1<i2
		{
		if(elements[i1]>elements[i2])
			{
			int temp=elements[i1];
			elements[i1]=elements[i2];
			elements[i2]=temp;
			}
		}
	void sortTuple(void) // Sorts the tuple after a change
		{
		/* Bubble sort for the win: */
		sortPair(0,1);
		sortPair(1,2);
		sortPair(0,1);
		}
	
	/* Constructors and destructors: */
	public:
	UnorderedTuple(void) // Dummy constructor
		{
		}
	UnorderedTuple(int sElement0,int sElement1,int sElement2) // Construction from three elements
		{
		elements[0]=sElement0;
		elements[1]=sElement1;
		elements[2]=sElement2;
		
		/* Sort the tuple: */
		sortTuple();
		}
	UnorderedTuple(const int sElements[3]) // Construction from C-style array
		{
		elements[0]=sElements[0];
		elements[1]=sElements[1];
		elements[2]=sElements[2];
		
		/* Sort the tuple: */
		sortTuple();
		}
	
	/* Methods: */
	const int* getElements(void) const // Returns element array
		{
		return elements;
		}
	int operator[](int index) const // Returns element as rvalue
		{
		return elements[index];
		}
	void set(int index,int newElement) // Sets an element to a new value
		{
		elements[index]=newElement;
		
		/* Sort the tuple: */
		sortTuple();
		}
	static size_t hash(const UnorderedTuple& source,size_t tableSize) // Calculates a hash function for the given tuple and table size
		{
		return ((size_t(source.elements[0])*10000003+size_t(source.elements[1]))*10000003+size_t(source.elements[2]))%tableSize;
		}
	};

/********************************
Operations on class UnorderedTuple:
********************************/

template <int dimensionParam>
inline bool operator==(const UnorderedTuple<dimensionParam>& ut1,const UnorderedTuple<dimensionParam>& ut2) // Equality operator
	{
	bool result=true;
	for(int i=0;i<dimensionParam&&result;++i)
		result=ut1[i]==ut2[i];
	return result;
	}

template <int dimensionParam>
inline bool operator!=(const UnorderedTuple<dimensionParam>& ut1,const UnorderedTuple<dimensionParam>& ut2) // Equality operator
	{
	bool result=false;
	for(int i=0;i<dimensionParam&&!result;++i)
		result=ut1[i]!=ut2[i];
	return result;
	}

template <>
inline bool operator==(const UnorderedTuple<2>& ut1,const UnorderedTuple<2>& ut2) // Equality operator
	{
	return ut1[0]==ut2[0]&&ut1[1]==ut2[1];
	}

template <>
inline bool operator!=(const UnorderedTuple<2>& ut1,const UnorderedTuple<2>& ut2) // Equality operator
	{
	return ut1[0]!=ut2[0]||ut1[1]!=ut2[1];
	}

template <>
inline bool operator==(const UnorderedTuple<3>& ut1,const UnorderedTuple<3>& ut2) // Equality operator
	{
	return ut1[0]==ut2[0]&&ut1[1]==ut2[1]&&ut1[2]==ut2[2];
	}

template <>
inline bool operator!=(const UnorderedTuple<3>& ut1,const UnorderedTuple<3>& ut2) // Equality operator
	{
	return ut1[0]!=ut2[0]||ut1[1]!=ut2[1]||ut1[2]!=ut2[2];
	}

}

#endif
