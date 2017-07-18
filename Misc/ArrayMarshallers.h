/***********************************************************************
ArrayMarshallers - Generic marshaller classes for standard C-style
arrays, with implicit or explicit array sizes or array wrapper classes.
Copyright (c) 2010-2017 Oliver Kreylos

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

#ifndef MISC_ARRAYMARSHALLERS_INCLUDED
#define MISC_ARRAYMARSHALLERS_INCLUDED

#include <stddef.h>
#include <Misc/SizedTypes.h>
#include <Misc/FixedArray.h>
#include <Misc/Marshaller.h>

namespace Misc {

template <class ValueParam>
class FixedArrayMarshaller // Marshaller class for arrays with a-priori known sizes
	{
	/* Methods: */
	public:
	static size_t getSize(const ValueParam* elements,size_t numElements)
		{
		size_t result=0;
		for(size_t i=0;i<numElements;++i)
			result+=Marshaller<ValueParam>::getSize(elements[i]);
		return result;
		}
	template <class StoredValueParam>
	static size_t getSize(const StoredValueParam* elements,size_t numElements)
		{
		size_t result=0;
		for(size_t i=0;i<numElements;++i)
			result+=Marshaller<ValueParam>::getSize(ValueParam(elements[i]));
		return result;
		}
	template <class DataSinkParam>
	static void write(const ValueParam* elements,size_t numElements,DataSinkParam& sink)
		{
		for(size_t i=0;i<numElements;++i)
			Marshaller<ValueParam>::write(elements[i],sink);
		}
	template <class StoredValueParam,class DataSinkParam>
	static void write(const StoredValueParam* elements,size_t numElements,DataSinkParam& sink)
		{
		for(size_t i=0;i<numElements;++i)
			Marshaller<ValueParam>::write(ValueParam(elements[i]),sink);
		}
	template <class DataSourceParam>
	static void read(ValueParam* elements,size_t numElements,DataSourceParam& source)
		{
		for(size_t i=0;i<numElements;++i)
			elements[i]=Marshaller<ValueParam>::read(source);
		}
	template <class StoredValueParam,class DataSourceParam>
	static void read(StoredValueParam* elements,size_t numElements,DataSourceParam& source)
		{
		for(size_t i=0;i<numElements;++i)
			elements[i]=StoredValueParam(Marshaller<ValueParam>::read(source));
		}
	};

template <class ValueParam>
class DynamicArrayMarshaller // Marshaller class for arrays with explicit sizes
	{
	/* Methods: */
	public:
	static size_t getSize(const ValueParam* elements,size_t numElements)
		{
		size_t result=sizeof(Misc::UInt32);
		for(size_t i=0;i<numElements;++i)
			result+=Marshaller<ValueParam>::getSize(elements[i]);
		return result;
		}
	template <class StoredValueParam>
	static size_t getSize(const StoredValueParam* elements,size_t numElements)
		{
		size_t result=sizeof(Misc::UInt32);
		for(size_t i=0;i<numElements;++i)
			result+=Marshaller<ValueParam>::getSize(ValueParam(elements[i]));
		return result;
		}
	template <class DataSinkParam>
	static void write(const ValueParam* elements,size_t numElements,DataSinkParam& sink)
		{
		sink.template write<Misc::UInt32>(Misc::UInt32(numElements));
		for(size_t i=0;i<numElements;++i)
			Marshaller<ValueParam>::write(elements[i],sink);
		}
	template <class StoredValueParam,class DataSinkParam>
	static void write(const StoredValueParam* elements,size_t numElements,DataSinkParam& sink)
		{
		sink.template write<Misc::UInt32>(Misc::UInt32(numElements));
		for(size_t i=0;i<numElements;++i)
			Marshaller<ValueParam>::write(ValueParam(elements[i]),sink);
		}
	template <class DataSourceParam>
	static size_t read(ValueParam* elements,size_t maxNumElements,DataSourceParam& source) // Reads at most maxNumElements elements; returns total number of elements to be read
		{
		size_t numElements=size_t(source.template read<Misc::UInt32>());
		if(maxNumElements>numElements)
			maxNumElements=numElements;
		for(size_t i=0;i<maxNumElements;++i)
			elements[i]=Marshaller<ValueParam>::read(source);
		return numElements;
		}
	template <class StoredValueParam,class DataSourceParam>
	static size_t read(StoredValueParam* elements,size_t maxNumElements,DataSourceParam& source) // Ditto, with value type conversion
		{
		size_t numElements=size_t(source.template read<Misc::UInt32>());
		if(maxNumElements>numElements)
			maxNumElements=numElements;
		for(size_t i=0;i<maxNumElements;++i)
			elements[i]=StoredValueParam(Marshaller<ValueParam>::read(source));
		return numElements;
		}
	template <class DataSourceParam>
	static void readMore(ValueParam* elements,size_t numElements,DataSourceParam& source) // Reads additional elements from the source after an initial read filled its buffer
		{
		for(size_t i=0;i<numElements;++i)
			elements[i]=Marshaller<ValueParam>::read(source);
		}
	template <class StoredValueParam,class DataSourceParam>
	static void readMore(StoredValueParam* elements,size_t numElements,DataSourceParam& source) // Ditto, with value type conversion
		{
		for(size_t i=0;i<numElements;++i)
			elements[i]=StoredValueParam(Marshaller<ValueParam>::read(source));
		}
	template <class DataSourceParam>
	static void discard(size_t numElements,DataSourceParam& source) // Discards a number of elements from the source
		{
		for(size_t i=0;i<numElements;++i)
			Marshaller<ValueParam>::read(source);
		}
	template <class DataSourceParam>
	static size_t read(ValueParam*& elements,DataSourceParam& source) // Reads into newly-allocated array of correct size
		{
		size_t numElements=size_t(source.template read<Misc::UInt32>());
		elements=new ValueParam[numElements];
		for(size_t i=0;i<numElements;++i)
			elements[i]=Marshaller<ValueParam>::read(source);
		return numElements;
		}
	template <class StoredValueParam,class DataSourceParam>
	static size_t read(StoredValueParam*& elements,DataSourceParam& source) // Ditto, with value type conversion
		{
		size_t numElements=size_t(source.template read<Misc::UInt32>());
		elements=new ValueParam[numElements];
		for(size_t i=0;i<numElements;++i)
			elements[i]=StoredValueParam(Marshaller<ValueParam>::read(source));
		return numElements;
		}
	};

template <class ElementParam,int sizeParam>
class Marshaller<FixedArray<ElementParam,sizeParam> >
	{
	/* Methods: */
	public:
	static size_t getSize(const FixedArray<ElementParam,sizeParam>& value)
		{
		size_t result=0;
		for(int i=0;i<sizeParam;++i)
			result+=Marshaller<ElementParam>::getSize(value[i]);
		return result;
		}
	template <class StoredElementParam>
	static size_t getSize(const FixedArray<StoredElementParam,sizeParam>& value)
		{
		size_t result=0;
		for(int i=0;i<sizeParam;++i)
			result+=Marshaller<ElementParam>::getSize(ElementParam(value[i]));
		return result;
		}
	template <class DataSinkParam>
	static void write(const FixedArray<ElementParam,sizeParam>& value,DataSinkParam& sink)
		{
		for(int i=0;i<sizeParam;++i)
			Marshaller<ElementParam>::write(value[i],sink);
		}
	template <class StoredElementParam,class DataSinkParam>
	static void write(const FixedArray<StoredElementParam,sizeParam>& value,DataSinkParam& sink)
		{
		for(int i=0;i<sizeParam;++i)
			Marshaller<ElementParam>::write(ElementParam(value[i]),sink);
		}
	template <class DataSourceParam>
	static FixedArray<ElementParam,sizeParam> read(DataSourceParam& source)
		{
		FixedArray<ElementParam,sizeParam> result;
		for(int i=0;i<sizeParam;++i)
			result[i]=Marshaller<ElementParam>::read(source);
		return result;
		}
	template <class StoredElementParam,class DataSourceParam>
	static FixedArray<StoredElementParam,sizeParam> read(DataSourceParam& source)
		{
		FixedArray<StoredElementParam,sizeParam> result;
		for(int i=0;i<sizeParam;++i)
			result[i]=StoredElementParam(Marshaller<ElementParam>::read(source));
		return result;
		}
	};

}

#endif
