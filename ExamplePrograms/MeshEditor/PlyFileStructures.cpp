/***********************************************************************
PlyFileStructures - Data structures to read 3D polygon files in PLY
format.
Copyright (c) 2004-2011 Oliver Kreylos
***********************************************************************/

#include "PlyFileStructures.h"

#include <Misc/ThrowStdErr.h>
#include <IO/ValueSource.h>

namespace {

/******************************************************
Templatized class to define PLY file value type traits:
******************************************************/

template <int dataTypeParam>
class PLYDataValueTypes
	{
	};

template <>
class PLYDataValueTypes<PLY_CHAR>
	{
	/* Embedded classes: */
	public:
	static const int dataType=PLY_CHAR;
	typedef char FileType;
	typedef int MemoryType;
	};

template <>
class PLYDataValueTypes<PLY_UCHAR>
	{
	/* Embedded classes: */
	public:
	static const int dataType=PLY_UCHAR;
	typedef unsigned char FileType;
	typedef unsigned int MemoryType;
	};

template <>
class PLYDataValueTypes<PLY_SHORT>
	{
	/* Embedded classes: */
	public:
	static const int dataType=PLY_SHORT;
	typedef short FileType;
	typedef int MemoryType;
	};

template <>
class PLYDataValueTypes<PLY_USHORT>
	{
	/* Embedded classes: */
	public:
	static const int dataType=PLY_USHORT;
	typedef unsigned short FileType;
	typedef unsigned int MemoryType;
	};

template <>
class PLYDataValueTypes<PLY_INT>
	{
	/* Embedded classes: */
	public:
	static const int dataType=PLY_INT;
	typedef int FileType;
	typedef int MemoryType;
	};

template <>
class PLYDataValueTypes<PLY_UINT>
	{
	/* Embedded classes: */
	public:
	static const int dataType=PLY_UINT;
	typedef unsigned int FileType;
	typedef unsigned int MemoryType;
	};

template <>
class PLYDataValueTypes<PLY_FLOAT>
	{
	/* Embedded classes: */
	public:
	static const int dataType=PLY_FLOAT;
	typedef float FileType;
	typedef double MemoryType;
	};

template <>
class PLYDataValueTypes<PLY_DOUBLE>
	{
	/* Embedded classes: */
	public:
	static const int dataType=PLY_DOUBLE;
	typedef double FileType;
	typedef double MemoryType;
	};

/************************************************************
Templatized helper class to read values from ASCII PLY files:
************************************************************/

template <typename MemoryTypeParam>
class AsciiFileReader
	{
	};

template <>
class AsciiFileReader<int>
	{
	/* Methods: */
	public:
	static int readValue(IO::ValueSource& asciiFile)
		{
		return asciiFile.readInteger();
		}
	};

template <>
class AsciiFileReader<unsigned int>
	{
	/* Methods: */
	public:
	static unsigned int readValue(IO::ValueSource& asciiFile)
		{
		return asciiFile.readUnsignedInteger();
		}
	};

template <>
class AsciiFileReader<double>
	{
	/* Methods: */
	public:
	static double readValue(IO::ValueSource& asciiFile)
		{
		return asciiFile.readNumber();
		}
	};

/*****************************************************
Templatized class for data values read from PLY files:
*****************************************************/

template <int dataTypeParam>
class PLYDataValueTemplate:public PLYDataValue,public PLYDataValueTypes<dataTypeParam>
	{
	/* Embedded classes: */
	public:
	typedef PLYDataValue Base1;
	typedef PLYDataValueTypes<dataTypeParam> Base2;
	
	/* Elements: */
	private:
	typename Base2::MemoryType value; // Data value
	
	/* Methods: */
	virtual PLYDataValue* clone(void) const
		{
		return new PLYDataValueTemplate(*this);
		}
	virtual size_t getFileSize(void) const
		{
		return sizeof(typename Base2::FileType);
		}
	virtual size_t getMemorySize(void) const
		{
		return sizeof(typename Base2::MemoryType);
		}
	virtual void read(IO::File& binaryPlyFile)
		{
		value=typename Base2::MemoryType(binaryPlyFile.read<typename Base2::FileType>());
		}
	virtual void read(IO::ValueSource& asciiPlyFile)
		{
		value=AsciiFileReader<typename Base2::MemoryType>::readValue(asciiPlyFile);
		}
	virtual int getInt(void) const
		{
		return int(value);
		}
	virtual unsigned int getUnsignedInt(void) const
		{
		return (unsigned int)(value);
		}
	virtual double getDouble(void) const
		{
		return double(value);
		}
	};

/******************************************
Factory class to create data value readers:
******************************************/

class PLYDataValueFactory
	{
	/* Methods: */
	public:
	static PLYDataValue* newDataValue(PLYDataType dataType)
		{
		switch(dataType)
			{
			case PLY_CHAR:
				return new PLYDataValueTemplate<PLY_CHAR>;
			
			case PLY_UCHAR:
				return new PLYDataValueTemplate<PLY_UCHAR>;
			
			case PLY_SHORT:
				return new PLYDataValueTemplate<PLY_SHORT>;
			
			case PLY_USHORT:
				return new PLYDataValueTemplate<PLY_USHORT>;
			
			case PLY_INT:
				return new PLYDataValueTemplate<PLY_INT>;
			
			case PLY_UINT:
				return new PLYDataValueTemplate<PLY_UINT>;
			
			case PLY_FLOAT:
				return new PLYDataValueTemplate<PLY_FLOAT>;
			
			case PLY_DOUBLE:
				return new PLYDataValueTemplate<PLY_DOUBLE>;
			}
		}
	};

}

/***********************************
Methods of class PLYProperty::Value:
***********************************/

PLYProperty::Value::Value(const PLYProperty& property)
	:propertyType(property.getPropertyType()),
	 scalar(0),
	 listSize(0)
	{
	if(propertyType==SCALAR)
		{
		/* Allocate space for scalar: */
		scalar=PLYDataValueFactory::newDataValue(property.getScalarType());
		}
	else
		{
		/* Allocate space for list size: */
		listSize=PLYDataValueFactory::newDataValue(property.getListSizeType());

		/* Create one list element to get started: */
		listElements.push_back(PLYDataValueFactory::newDataValue(property.getListElementType()));
		}
	}

PLYProperty::Value::~Value(void)
	{
	delete scalar;
	delete listSize;
	for(std::vector<PLYDataValue*>::iterator eIt=listElements.begin();eIt!=listElements.end();++eIt)
		delete *eIt;
	}

void PLYProperty::Value::skip(IO::ValueSource& plyFile)
	{
	if(propertyType==SCALAR)
		{
		/* Skip scalar: */
		scalar->read(plyFile);
		}
	else
		{
		/* Read list size: */
		listSize->read(plyFile);
		unsigned int listSizeValue=listSize->getUnsignedInt();
		
		/* Skip all list elements: */
		for(unsigned int i=0;i<listSizeValue;++i)
			listElements[0]->read(plyFile);
		}
	}

void PLYProperty::Value::skip(IO::File& plyFile)
	{
	if(propertyType==SCALAR)
		{
		/* Skip scalar: */
		plyFile.skip<char>(scalar->getFileSize());
		}
	else
		{
		/* Read list size: */
		listSize->read(plyFile);
		unsigned int listSizeValue=listSize->getUnsignedInt();
		
		/* Skip all list elements: */
		plyFile.skip<char>(listElements[0]->getFileSize()*listSizeValue);
		}
	}

/****************************
Methods of class PLYProperty:
****************************/

PLYDataType PLYProperty::parseDataType(const std::string& tag)
	{
	static const char* dataTypeTags[]={"char","uchar","short","ushort","int","uint","float","double"};
	static const PLYDataType dataTypes[]={PLY_CHAR,PLY_UCHAR,PLY_SHORT,PLY_USHORT,PLY_INT,PLY_UINT,PLY_FLOAT,PLY_DOUBLE};
	int i;
	for(i=0;i<8;++i)
		if(tag==dataTypeTags[i])
			break;
	if(i>=8)
		Misc::throwStdErr("PLYProperty::parseDataType: Unknown data type %s",tag.c_str());
	return dataTypes[i];
	}

PLYProperty::PLYProperty(IO::ValueSource& plyFile)
	{
	/* Read the property type: */
	std::string tag=plyFile.readString();
	
	if(tag=="list")
		{
		/* Parse a list property: */
		propertyType=LIST;
		listSizeType=parseDataType(plyFile.readString());
		listElementType=parseDataType(plyFile.readString());
		}
	else
		{
		/* Parse a scalar property: */
		propertyType=SCALAR;
		scalarType=parseDataType(tag);
		}
	
	/* Read the property name: */
	name=plyFile.readLine();
	plyFile.skipWs();
	}

/**********************************
Methods of class PLYElement::Value:
**********************************/

PLYElement::Value::Value(const PLYElement& element)
	{
	/* Initialize vector of property values: */
	for(PropertyList::const_iterator plIt=element.propertiesBegin();plIt!=element.propertiesEnd();++plIt)
		propertyValues.push_back(new PLYProperty::Value(*plIt));
	}

PLYElement::Value::~Value(void)
	{
	/* Destroy vector of property values: */
	for(PropertyValueList::iterator pvIt=propertyValues.begin();pvIt!=propertyValues.end();++pvIt)
		delete *pvIt;
	}

/******************************
Methods of class PLYFileHeader:
******************************/

PLYFileHeader::PLYFileHeader(IO::File& plyFile)
	:valid(false),fileType(Unknown),fileEndianness(IO::File::DontCare)
	{
	/* Attach a new value source to the PLY file: */
	IO::ValueSource ply(plyFile);
	ply.skipWs();
	
	/* Process the PLY file header: */
	std::vector<PLYElement>::iterator currentElement=elements.end();
	bool isPly=false;
	bool haveEndHeader=false;
	while(!ply.eof())
		{
		/* Read the next tag: */
		std::string tag=ply.readString();
		if(tag=="ply")
			isPly=true;
		else if(tag=="format")
			{
			/* Read the format type and version number: */
			std::string format=ply.readString();
			if(format=="ascii")
				fileType=Ascii;
			else if(format=="binary_little_endian")
				{
				fileType=Binary;
				fileEndianness=IO::File::LittleEndian;
				}
			else if(format=="binary_big_endian")
				{
				fileType=Binary;
				fileEndianness=IO::File::BigEndian;
				}
			else
				{
				/* Unknown format; bail out: */
				break;
				}
			double version=ply.readNumber();
			if(version!=1.0)
				break;
			}
		else if(tag=="comment")
			{
			/* Skip the rest of the line: */
			ply.skipLine();
			ply.skipWs();
			}
		else if(tag=="element")
			{
			/* Read the element type and number of elements: */
			std::string elementType=ply.readString();
			size_t numElements=ply.readUnsignedInteger();
			
			/* Append a new element: */
			elements.push_back(PLYElement(elementType,numElements));
			currentElement=elements.end()-1;
			}
		else if(tag=="property")
			{
			if(currentElement!=elements.end())
				{
				/* Parse a property: */
				currentElement->addProperty(ply);
				}
			else
				{
				/* Skip the property: */
				ply.skipLine();
				ply.skipWs();
				}
			}
		else if(tag=="end_header")
			{
			haveEndHeader=true;
			break;
			}
		else
			{
			/* Skip the unknown tag: */
			ply.skipLine();
			ply.skipWs();
			}
		}
	
	/* Check if the header was read completely: */
	valid=isPly&&haveEndHeader&&fileType!=Unknown;
	}

/****************
Global functions:
****************/

void skipElement(const PLYElement& element,IO::File& plyFile)
	{
	/* Check if the element has variable size: */
	PLYElement::Value value(element);
	if(element.hasListProperty())
		{
		/* Need to skip each value separately: */
		for(size_t i=0;i<element.getNumValues();++i)
			value.skip(plyFile);
		}
	else
		{
		/* Calculate the file size of each value of the element: */
		size_t valueSize=value.getFileSize();
		plyFile.skip<char>(valueSize*element.getNumValues());
		}
	}

void skipElement(const PLYElement& element,IO::ValueSource& plyFile)
	{
	/* Skip one line for each value of the element: */
	for(size_t i=0;i<element.getNumValues();++i)
		plyFile.skipLine();
	plyFile.skipWs();
	}
