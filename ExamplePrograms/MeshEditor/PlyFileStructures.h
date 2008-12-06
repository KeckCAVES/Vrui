/***********************************************************************
PlyFileStructures - Data structures to read 3D polygon files in PLY
format.
Copyright (c) 2004-2006 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef PLYFILESTRUCTURES_INCLUDED
#define PLYFILESTRUCTURES_INCLUDED

#include <string.h>
#include <stdio.h>
#include <Misc/File.h>

enum PlyFileMode
	{
	PLY_WRONGFORMAT,PLY_ASCII,PLY_BINARY
	};

enum DataType
	{
	CHAR,UCHAR,SHORT,USHORT,INT,UINT,FLOAT,DOUBLE
	};

template <int dataTypeParam>
class DataValueTypes
	{
	};

template <>
class DataValueTypes<CHAR>
	{
	/* Embedded classes: */
	public:
	static const int dataType=CHAR;
	typedef char FileType;
	typedef int MemoryType;
	};

template <>
class DataValueTypes<UCHAR>
	{
	/* Embedded classes: */
	public:
	static const int dataType=UCHAR;
	typedef unsigned char FileType;
	typedef unsigned int MemoryType;
	};

template <>
class DataValueTypes<SHORT>
	{
	/* Embedded classes: */
	public:
	static const int dataType=SHORT;
	typedef short FileType;
	typedef int MemoryType;
	};

template <>
class DataValueTypes<USHORT>
	{
	/* Embedded classes: */
	public:
	static const int dataType=USHORT;
	typedef unsigned short FileType;
	typedef unsigned int MemoryType;
	};

template <>
class DataValueTypes<INT>
	{
	/* Embedded classes: */
	public:
	static const int dataType=INT;
	typedef int FileType;
	typedef int MemoryType;
	};

template <>
class DataValueTypes<UINT>
	{
	/* Embedded classes: */
	public:
	static const int dataType=UINT;
	typedef unsigned int FileType;
	typedef unsigned int MemoryType;
	};

template <>
class DataValueTypes<FLOAT>
	{
	/* Embedded classes: */
	public:
	static const int dataType=FLOAT;
	typedef float FileType;
	typedef double MemoryType;
	};

template <>
class DataValueTypes<DOUBLE>
	{
	/* Embedded classes: */
	public:
	static const int dataType=DOUBLE;
	typedef double FileType;
	typedef double MemoryType;
	};

template <typename MemoryTypeParam>
class AsciiFileReader
	{
	};

template <>
class AsciiFileReader<int>
	{
	/* Methods: */
	public:
	static int readValue(Misc::File& asciiFile)
		{
		int result;
		fscanf(asciiFile.getFilePtr(),"%d",&result);
		return result;
		};
	};

template <>
class AsciiFileReader<unsigned int>
	{
	/* Methods: */
	public:
	static unsigned int readValue(Misc::File& asciiFile)
		{
		unsigned int result;
		fscanf(asciiFile.getFilePtr(),"%u",&result);
		return result;
		};
	};

template <>
class AsciiFileReader<double>
	{
	/* Methods: */
	public:
	static double readValue(Misc::File& asciiFile)
		{
		double result;
		fscanf(asciiFile.getFilePtr(),"%lf",&result);
		return result;
		};
	};

class DataValue
	{
	/* Constructors and destructors: */
	public:
	virtual ~DataValue(void)
		{
		};
	
	/* Methods: */
	virtual size_t getFileSize(void) const =0;
	virtual size_t getMemorySize(void) const =0;
	virtual void read(Misc::File& plyFile,PlyFileMode plyFileMode) =0;
	virtual int getInt(void) const =0;
	virtual unsigned int getUnsignedInt(void) const =0;
	virtual double getDouble(void) const =0;
	};

template <int dataTypeParam>
class DataValueTemplate:public DataValue,public DataValueTypes<dataTypeParam>
	{
	/* Embedded classes: */
	public:
	typedef DataValue Base1;
	typedef DataValueTypes<dataTypeParam> Base2;
	
	/* Elements: */
	private:
	typename Base2::MemoryType value; // Data value
	
	/* Methods: */
	virtual size_t getFileSize(void) const
		{
		return sizeof(typename Base2::FileType);
		};
	virtual size_t getMemorySize(void) const
		{
		return sizeof(typename Base2::MemoryType);
		};
	virtual void read(Misc::File& plyFile,PlyFileMode plyFileMode)
		{
		switch(plyFileMode)
			{
			case PLY_ASCII:
				value=AsciiFileReader<typename Base2::MemoryType>::readValue(plyFile);
				break;
			
			case PLY_BINARY:
				{
				typename Base2::FileType fileValue;
				plyFile.read(fileValue);
				value=typename Base2::MemoryType(fileValue);
				break;
				}
			}
		};
	virtual int getInt(void) const
		{
		return int(value);
		};
	virtual unsigned int getUnsignedInt(void) const
		{
		return (unsigned int)(value);
		};
	virtual double getDouble(void) const
		{
		return double(value);
		};
	};

class DataValueFactory
	{
	/* Methods: */
	public:
	static DataValue* newDataValue(DataType dataType)
		{
		switch(dataType)
			{
			case CHAR:
				return new DataValueTemplate<CHAR>;
			
			case UCHAR:
				return new DataValueTemplate<UCHAR>;
			
			case SHORT:
				return new DataValueTemplate<SHORT>;
			
			case USHORT:
				return new DataValueTemplate<USHORT>;
			
			case INT:
				return new DataValueTemplate<INT>;
			
			case UINT:
				return new DataValueTemplate<UINT>;
			
			case FLOAT:
				return new DataValueTemplate<FLOAT>;
			
			case DOUBLE:
				return new DataValueTemplate<DOUBLE>;
			}
		};
	};

class Property
	{
	/* Embedded classes: */
	public:
	enum PropertyType
		{
		SCALAR,LIST
		};
	class Value
		{
		/* Elements: */
		private:
		const Property* property; // Pointer to property definition for this value
		DataValue* scalar; // Pointer to scalar value for scalar properties
		DataValue* listSize; // List size value for list properties
		std::vector<DataValue*> listElements; // Vector of pointers to list elements for list properties
		
		/* Private methods: */
		void clearListElements(void)
			{
			for(std::vector<DataValue*>::iterator eIt=listElements.begin();eIt!=listElements.end();++eIt)
				delete *eIt;
			listElements.clear();
			};
		
		/* Constructors and destructors: */
		public:
		Value(const Property* sProperty) // Creates empty value structure for the given property
			:property(sProperty),
			 scalar(0),
			 listSize(0)
			{
			if(property->getPropertyType()==Property::LIST)
				{
				/* Allocate space for list size: */
				listSize=DataValueFactory::newDataValue(property->getListSizeType());
				}
			else
				{
				/* Allocate space for scalar: */
				scalar=DataValueFactory::newDataValue(property->getScalarType());
				}
			};
		Value(const Value& source)
			:property(source.property),
			 scalar(0),
			 listSize(0)
			{
			if(property->getPropertyType()==Property::LIST)
				{
				/* Allocate space for list size: */
				listSize=DataValueFactory::newDataValue(property->getListSizeType());
				}
			else
				{
				/* Allocate space for scalar: */
				scalar=DataValueFactory::newDataValue(property->getScalarType());
				}
			};
		~Value(void)
			{
			if(property->getPropertyType()==Property::LIST)
				{
				delete listSize;
				clearListElements();
				}
			else
				delete scalar;
			};
		
		/* Methods: */
		void read(Misc::File& plyFile,PlyFileMode plyFileMode) // Reads value from PLY file
			{
			if(property->getPropertyType()==Property::LIST)
				{
				/* Read list size: */
				listSize->read(plyFile,plyFileMode);
				unsigned int listSizeValue=listSize->getUnsignedInt();
				
				/* Read all list elements: */
				clearListElements();
				listElements.reserve(listSizeValue);
				for(unsigned int i=0;i<listSizeValue;++i)
					{
					listElements[i]=DataValueFactory::newDataValue(property->getListElementType());
					listElements[i]->read(plyFile,plyFileMode);
					}
				}
			else
				{
				/* Read scalar: */
				scalar->read(plyFile,plyFileMode);
				}
			};
		const DataValue* getScalar(void) const
			{
			return scalar;
			};
		const DataValue* getListSize(void) const
			{
			return listSize;
			};
		const DataValue* getListElement(unsigned int listElementIndex) const
			{
			return listElements[listElementIndex];
			};
		};
	
	/* Elements: */
	private:
	PropertyType propertyType; // Type of this property (scalar/list)
	DataType scalarType; // Data type for scalar properties
	DataType listSizeType; // Data type for list sizes for list properties
	DataType listElementType; // Data type for list elements for list properties
	std::string name; // Property name
	
	/* Private methods: */
	static DataType parseDataType(const char* tPtr)
		{
		static const char* dataTypeTags[]={"char","uchar","short","ushort","int","uint","float","double"};
		static const DataType dataTypes[]={CHAR,UCHAR,SHORT,USHORT,INT,UINT,FLOAT,DOUBLE};
		int i;
		for(i=0;i<8;++i)
			if(strncmp(tPtr,dataTypeTags[i],strlen(dataTypeTags[i]))==0)
				break;
		return dataTypes[i];
		};
	static const char* nextTag(const char* tPtr)
		{
		while(*tPtr!='\0'&&!isspace(*tPtr))
			++tPtr;
		while(*tPtr!='\0'&&isspace(*tPtr))
			++tPtr;
		return tPtr;
		};
	
	/* Constructors and destructors: */
	public:
	Property(const char* propertyDefinition)
		{
		/* Check whether it's a list or scalar property: */
		if(strncmp(propertyDefinition,"list",4)==0)
			{
			propertyDefinition=nextTag(propertyDefinition);
			
			/* Parse list property: */
			propertyType=LIST;
			listSizeType=parseDataType(propertyDefinition);
			propertyDefinition=nextTag(propertyDefinition);
			listElementType=parseDataType(propertyDefinition);
			}
		else
			{
			/* Parse scalar property: */
			propertyType=SCALAR;
			scalarType=parseDataType(propertyDefinition);
			}
		
		/* Extract property name: */
		propertyDefinition=nextTag(propertyDefinition);
		const char* nameEndPtr;
		for(nameEndPtr=propertyDefinition;*nameEndPtr!='\n'&&*nameEndPtr!='\0';++nameEndPtr)
			;
		name=std::string(propertyDefinition,nameEndPtr-propertyDefinition);
		};
	
	/* Methods: */
	PropertyType getPropertyType(void) const // Returns property's type
		{
		return propertyType;
		};
	DataType getScalarType(void) const // Returns scalar property's scalar type
		{
		return scalarType;
		};
	DataType getListSizeType(void) const // Returns list property's size type
		{
		return listSizeType;
		};
	DataType getListElementType(void) const // Returns list property's element type
		{
		return listElementType;
		};
	std::string getName(void) const // Returns property's name
		{
		return name;
		};
	};

class Element
	{
	/* Embedded classes: */
	public:
	typedef std::vector<Property> PropertyList;
	class Value
		{
		/* Embedded classes: */
		public:
		typedef std::vector<Property::Value> PropertyValueList;
		
		/* Elements: */
		private:
		const Element* element; // Pointer to element definition for this value
		PropertyValueList propertyValues; // Vector of values for the properties of this element
		
		/* Constructors and destructors: */
		public:
		Value(const Element* sElement)
			:element(sElement)
			{
			/* Initialize vector of property values: */
			for(PropertyList::const_iterator plIt=element->propertiesBegin();plIt!=element->propertiesEnd();++plIt)
				{
				propertyValues.push_back(Property::Value(&(*plIt)));
				}
			};
		Value(const Value& source)
			:element(source.element)
			{
			/* Initialize vector of property values: */
			for(PropertyList::const_iterator plIt=element->propertiesBegin();plIt!=element->propertiesEnd();++plIt)
				{
				propertyValues.push_back(Property::Value(&(*plIt)));
				}
			};
		
		/* Methods: */
		void read(Misc::File& plyFile,PlyFileMode plyFileMode) // Reads element from PLY file
			{
			for(PropertyValueList::iterator pvIt=propertyValues.begin();pvIt!=propertyValues.end();++pvIt)
				pvIt->read(plyFile,plyFileMode);
			};
		const Property::Value& getValue(unsigned int propertyIndex) const
			{
			return propertyValues[propertyIndex];
			};
		};
	
	/* Elements: */
	private:
	std::string name; // Name of this element
	PropertyList properties; // Vector of properties of this element
	
	/* Constructors and destructors: */
	public:
	Element(std::string sName)
		:name(sName)
		{
		};
	
	/* Methods: */
	void addProperty(const char* propertyDefinition)
		{
		properties.push_back(Property(propertyDefinition));
		};
	PropertyList::const_iterator propertiesBegin(void) const
		{
		return properties.begin();
		};
	PropertyList::const_iterator propertiesEnd(void) const
		{
		return properties.end();
		};
	unsigned int getPropertyIndex(std::string propertyName) const
		{
		unsigned int result=0;
		for(PropertyList::const_iterator pIt=properties.begin();pIt!=properties.end();++pIt,++result)
			if(pIt->getName()==propertyName)
				break;
		return result;
		};
	};

#endif
