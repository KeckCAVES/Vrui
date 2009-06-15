/***********************************************************************
GeometryValueCoders - Value coder classes for templatized geometry
objects.
Copyright (c) 2003-2005 Oliver Kreylos

This file is part of the Templatized Geometry Library (TGL).

The Templatized Geometry Library is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Templatized Geometry Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Templatized Geometry Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#define GEOMETRY_GEOMETRYVALUECODERS_IMPLEMENTATION

#ifndef METHODPREFIX
	#ifdef NONSTANDARD_TEMPLATES
		#define METHODPREFIX inline
	#else
		#define METHODPREFIX
	#endif
#endif

#include <Geometry/GeometryValueCoders.h>

#include <ctype.h>
#include <string.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <Math/Math.h>
#include <Geometry/ComponentArray.h>
#include <Geometry/Vector.h>
#include <Geometry/Point.h>
#include <Geometry/HVector.h>
#include <Geometry/Box.h>
#include <Geometry/Plane.h>
#include <Geometry/Matrix.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/AffineTransformation.h>
#include <Geometry/ProjectiveTransformation.h>

namespace Misc {

/**********************************************************************************
Methods of class ValueCoder<Geometry::ComponentArray<ScalarParam,dimensionParam> >:
**********************************************************************************/

template <class ScalarParam,int dimensionParam>
METHODPREFIX
std::string
ValueCoder<Geometry::ComponentArray<ScalarParam,dimensionParam> >::encode(
	const Geometry::ComponentArray<ScalarParam,dimensionParam>& value)
	{
	return ValueCoderArray<ScalarParam>::encode(dimensionParam,value.getComponents());
	}

template <class ScalarParam,int dimensionParam>
METHODPREFIX
Geometry::ComponentArray<ScalarParam,dimensionParam>
ValueCoder<Geometry::ComponentArray<ScalarParam,dimensionParam> >::decode(
	const char* start,
	const char* end,
	const char** decodeEnd)
	{
	try
		{
		Geometry::ComponentArray<ScalarParam,dimensionParam> result;
		int numComponents=ValueCoderArray<ScalarParam>::decode(dimensionParam,result.getComponents(),start,end,decodeEnd);
		if(numComponents!=dimensionParam)
			throw 42;
		return result;
		}
	catch(...)
		{
		throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to Geometry::ComponentArray<Scalar,dimension>"));
		}
	}

/**************************************************************************
Methods of class ValueCoder<Geometry::Vector<ScalarParam,dimensionParam> >:
**************************************************************************/

template <class ScalarParam,int dimensionParam>
METHODPREFIX
std::string
ValueCoder<Geometry::Vector<ScalarParam,dimensionParam> >::encode(
	const Geometry::Vector<ScalarParam,dimensionParam>& value)
	{
	return ValueCoderArray<ScalarParam>::encode(dimensionParam,value.getComponents());
	}

template <class ScalarParam,int dimensionParam>
METHODPREFIX
Geometry::Vector<ScalarParam,dimensionParam>
ValueCoder<Geometry::Vector<ScalarParam,dimensionParam> >::decode(
	const char* start,
	const char* end,
	const char** decodeEnd)
	{
	try
		{
		Geometry::Vector<ScalarParam,dimensionParam> result;
		int numComponents=ValueCoderArray<ScalarParam>::decode(dimensionParam,result.getComponents(),start,end,decodeEnd);
		if(numComponents!=dimensionParam)
			throw 42;
		return result;
		}
	catch(...)
		{
		throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to Geometry::Vector<Scalar,dimension>"));
		}
	}

/*************************************************************************
Methods of class ValueCoder<Geometry::Point<ScalarParam,dimensionParam> >:
*************************************************************************/

template <class ScalarParam,int dimensionParam>
METHODPREFIX
std::string
ValueCoder<Geometry::Point<ScalarParam,dimensionParam> >::encode(
	const Geometry::Point<ScalarParam,dimensionParam>& value)
	{
	return ValueCoderArray<ScalarParam>::encode(dimensionParam,value.getComponents());
	}

template <class ScalarParam,int dimensionParam>
METHODPREFIX
Geometry::Point<ScalarParam,dimensionParam>
ValueCoder<Geometry::Point<ScalarParam,dimensionParam> >::decode(
	const char* start,
	const char* end,
	const char** decodeEnd)
	{
	try
		{
		Geometry::Point<ScalarParam,dimensionParam> result;
		int numComponents=ValueCoderArray<ScalarParam>::decode(dimensionParam,result.getComponents(),start,end,decodeEnd);
		if(numComponents!=dimensionParam)
			throw 42;
		return result;
		}
	catch(...)
		{
		throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to Geometry::Point<Scalar,dimension>"));
		}
	}

/**************************************************************************
Methods of class ValueCoder<Geometry::HVector<ScalarParam,dimensionParam> >:
**************************************************************************/

template <class ScalarParam,int dimensionParam>
METHODPREFIX
std::string
ValueCoder<Geometry::HVector<ScalarParam,dimensionParam> >::encode(
	const Geometry::HVector<ScalarParam,dimensionParam>& value)
	{
	return ValueCoderArray<ScalarParam>::encode(dimensionParam+1,value.getComponents());
	}

template <class ScalarParam,int dimensionParam>
METHODPREFIX
Geometry::HVector<ScalarParam,dimensionParam>
ValueCoder<Geometry::HVector<ScalarParam,dimensionParam> >::decode(
	const char* start,
	const char* end,
	const char** decodeEnd)
	{
	try
		{
		Geometry::HVector<ScalarParam,dimensionParam> result;
		int numComponents=ValueCoderArray<ScalarParam>::decode(dimensionParam+1,result.getComponents(),start,end,decodeEnd);
		if(numComponents!=dimensionParam+1)
			throw 42;
		return result;
		}
	catch(...)
		{
		throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to Geometry::HVector<Scalar,dimension>"));
		}
	}

/***********************************************************************
Methods of class ValueCoder<Geometry::Box<ScalarParam,dimensionParam> >:
***********************************************************************/

template <class ScalarParam,int dimensionParam>
METHODPREFIX
std::string
ValueCoder<Geometry::Box<ScalarParam,dimensionParam> >::encode(
	const Geometry::Box<ScalarParam,dimensionParam>& value)
	{
	typedef Geometry::Box<ScalarParam,dimensionParam> Box;
	std::string result="";
	result+=ValueCoder<typename Box::Point>::encode(value.getOrigin());
	result+=", ";
	result+=ValueCoder<typename Box::Size>::encode(value.getSize());
	return result;
	}

template <class ScalarParam,int dimensionParam>
METHODPREFIX
Geometry::Box<ScalarParam,dimensionParam>
ValueCoder<Geometry::Box<ScalarParam,dimensionParam> >::decode(
	const char* start,
	const char* end,
	const char** decodeEnd)
	{
	typedef Geometry::Box<ScalarParam,dimensionParam> Box;
	try
		{
		/* Parse the box's origin: */
		const char* tokenPtr=start;
		typename Box::Point origin=ValueCoder<typename Box::Point>::decode(tokenPtr,end,&tokenPtr);
		
		/* Skip whitespace and check for separating comma: */
		while(tokenPtr!=end&&isspace(*tokenPtr))
			++tokenPtr;
		if(tokenPtr==end||*tokenPtr!=',')
			throw 42;
		++tokenPtr;
		while(tokenPtr!=end&&isspace(*tokenPtr))
			++tokenPtr;
		
		/* Parse the box's size: */
		typename Box::Size size=ValueCoder<typename Box::Size>::decode(tokenPtr,end,&tokenPtr);
		
		if(decodeEnd!=0)
			*decodeEnd=tokenPtr;
		return Box(origin,size);
		}
	catch(...)
		{
		throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to Geometry::Box<Scalar,dimension>"));
		}
	}

/*************************************************************************
Methods of class ValueCoder<Geometry::Plane<ScalarParam,dimensionParam> >:
*************************************************************************/

template <class ScalarParam,int dimensionParam>
METHODPREFIX
std::string
ValueCoder<Geometry::Plane<ScalarParam,dimensionParam> >::encode(
	const Geometry::Plane<ScalarParam,dimensionParam>& value)
	{
	typedef Geometry::Plane<ScalarParam,dimensionParam> Plane;
	std::string result="";
	result+=ValueCoder<typename Plane::Vector>::encode(value.getNormal());
	result+=", ";
	result+=ValueCoder<typename Plane::Scalar>::encode(value.getOffset());
	return result;
	}

template <class ScalarParam,int dimensionParam>
METHODPREFIX
Geometry::Plane<ScalarParam,dimensionParam>
ValueCoder<Geometry::Plane<ScalarParam,dimensionParam> >::decode(
	const char* start,
	const char* end,
	const char** decodeEnd)
	{
	typedef Geometry::Plane<ScalarParam,dimensionParam> Plane;
	try
		{
		/* Parse the plane's normal vector: */
		const char* tokenPtr=start;
		typename Plane::Vector normal=ValueCoder<typename Plane::Vector>::decode(tokenPtr,end,&tokenPtr);
		
		/* Skip whitespace and check for separating comma: */
		while(tokenPtr!=end&&isspace(*tokenPtr))
			++tokenPtr;
		if(tokenPtr==end||*tokenPtr!=',')
			throw 42;
		++tokenPtr;
		while(tokenPtr!=end&&isspace(*tokenPtr))
			++tokenPtr;
		
		/* Parse the plane's offset: */
		typename Plane::Scalar offset=ValueCoder<typename Plane::Scalar>::decode(tokenPtr,end,&tokenPtr);
		
		if(decodeEnd!=0)
			*decodeEnd=tokenPtr;
		return Plane(normal,offset);
		}
	catch(...)
		{
		throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to Geometry::Plane<Scalar,dimension>"));
		}
	}

/****************************************************************************************
Methods of class ValueCoder<Geometry::Matrix<ScalarParam,numRowsParam,numColumnsParam> >:
****************************************************************************************/

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX
std::string
ValueCoder<Geometry::Matrix<ScalarParam,numRowsParam,numColumnsParam> >::encode(
	const Geometry::Matrix<ScalarParam,numRowsParam,numColumnsParam>& value)
	{
	std::string result="(";
	for(int j=0;j<numColumnsParam;++j)
		{
		if(j>0)
			result+=", ";
		Geometry::ComponentArray<ScalarParam,numRowsParam> col;
		for(int i=0;i<numRowsParam;++i)
			col[i]=value(i,j);
		result+=ValueCoder<Geometry::ComponentArray<ScalarParam,numRowsParam> >::encode(col);
		}
	result+=")";
	return result;
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX
Geometry::Matrix<ScalarParam,numRowsParam,numColumnsParam>
ValueCoder<Geometry::Matrix<ScalarParam,numRowsParam,numColumnsParam> >::decode(
	const char* start,
	const char* end,
	const char** decodeEnd)
	{
	try
		{
		Geometry::Matrix<ScalarParam,numRowsParam,numColumnsParam> result;
		
		/* Check for the opening parenthesis and skip whitespace: */
		const char* tokenPtr=start;
		if(tokenPtr==end||*tokenPtr!='(')
			throw 42;
		++tokenPtr;
		while(tokenPtr!=end&&isspace(*tokenPtr))
			++tokenPtr;
		
		/* Read matrix column-wise: */
		for(int j=0;j<numColumnsParam;++j)
			{
			if(j>0)
				{
				/* Check for the separating comma and skip whitespace: */
				if(tokenPtr==end||*tokenPtr!=',')
					throw 42;
				++tokenPtr;
				while(tokenPtr!=end&&isspace(*tokenPtr))
					++tokenPtr;
				}
			
			/* Read the column: */
			Geometry::ComponentArray<ScalarParam,numRowsParam> col=ValueCoder<Geometry::ComponentArray<ScalarParam,numRowsParam> >::decode(tokenPtr,end,&tokenPtr);
			
			/* Store the column in the matrix: */
			for(int i=0;i<numRowsParam;++i)
				result(i,j)=col[i];
			
			/* Skip whitespace: */
			while(tokenPtr!=end&&isspace(*tokenPtr))
				++tokenPtr;
			}
		
		/* Check for the closing parenthesis: */
		if(tokenPtr==end||*tokenPtr!=')')
			throw 42;
		++tokenPtr;
		
		if(decodeEnd!=0)
			*decodeEnd=tokenPtr;
		return result;
		}
	catch(...)
		{
		throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to Geometry::Matrix<Scalar,numRows,numColumns>"));
		}
	}

/***************************************************************
Methods of class ValueCoder<Geometry::Rotation<ScalarParam,2> >:
***************************************************************/

template <class ScalarParam>
METHODPREFIX
std::string
ValueCoder<Geometry::Rotation<ScalarParam,2> >::encode(
	const Geometry::Rotation<ScalarParam,2>& value)
	{
	return ValueCoder<ScalarParam>::encode(value.getAngle());
	}

template <class ScalarParam>
METHODPREFIX
Geometry::Rotation<ScalarParam,2>
ValueCoder<Geometry::Rotation<ScalarParam,2> >::decode(
	const char* start,
	const char* end,
	const char** decodeEnd)
	{
	try
		{
		ScalarParam angle=ValueCoder<ScalarParam>::decode(start,end,decodeEnd);
		return Geometry::Rotation<ScalarParam,2>(Math::rad(angle));
		}
	catch(...)
		{
		throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to Geometry::Rotation<Scalar,2>"));
		}
	}

/***************************************************************
Methods of class ValueCoder<Geometry::Rotation<ScalarParam,3> >:
***************************************************************/

template <class ScalarParam>
METHODPREFIX
std::string
ValueCoder<Geometry::Rotation<ScalarParam,3> >::encode(
	const Geometry::Rotation<ScalarParam,3>& value)
	{
	std::string result="";
	result+=ValueCoder<Geometry::Vector<ScalarParam,3> >::encode(value.getAxis());
	result+=", ";
	result+=ValueCoder<ScalarParam>::encode(Math::deg(value.getAngle()));
	return result;
	}

template <class ScalarParam>
METHODPREFIX
Geometry::Rotation<ScalarParam,3>
ValueCoder<Geometry::Rotation<ScalarParam,3> >::decode(
	const char* start,
	const char* end,
	const char** decodeEnd)
	{
	try
		{
		const char* tokenPtr=start;
		Geometry::Vector<ScalarParam,3> axis=ValueCoder<Geometry::Vector<ScalarParam,3> >::decode(tokenPtr,end,&tokenPtr);
		while(tokenPtr!=end&&isspace(*tokenPtr))
			++tokenPtr;
		if(tokenPtr==end||*tokenPtr!=',')
			throw 42;
		++tokenPtr;
		while(tokenPtr!=end&&isspace(*tokenPtr))
			++tokenPtr;
		ScalarParam angle=ValueCoder<ScalarParam>::decode(tokenPtr,end,&tokenPtr);
		
		if(decodeEnd!=0)
			*decodeEnd=tokenPtr;
		return Geometry::Rotation<ScalarParam,3>(axis,Math::rad(angle));
		}
	catch(...)
		{
		throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to Geometry::Rotation<Scalar,3>"));
		}
	}

/*********************************************************************************************
Methods of class ValueCoder<Geometry::OrthonormalTransformation<ScalarParam,dimensionParam> >:
*********************************************************************************************/

template <class ScalarParam,int dimensionParam>
METHODPREFIX
std::string
ValueCoder<Geometry::OrthonormalTransformation<ScalarParam,dimensionParam> >::encode(
	const Geometry::OrthonormalTransformation<ScalarParam,dimensionParam>& value)
	{
	std::string result="";
	result+="translate ";
	result+=ValueCoder<Geometry::Vector<ScalarParam,dimensionParam> >::encode(value.getTranslation());
	result+=" * ";
	result+="rotate ";
	result+=ValueCoder<Geometry::Rotation<ScalarParam,dimensionParam> >::encode(value.getRotation());
	return result;
	}

template <class ScalarParam,int dimensionParam>
METHODPREFIX
Geometry::OrthonormalTransformation<ScalarParam,dimensionParam>
ValueCoder<Geometry::OrthonormalTransformation<ScalarParam,dimensionParam> >::decode(
	const char* start,
	const char* end,
	const char** decodeEnd)
	{
	typedef Geometry::OrthonormalTransformation<ScalarParam,dimensionParam> ONT;
	
	try
		{
		ONT result=ONT::identity;
		const char* tokenPtr=start;
		while(true)
			{
			if(end-tokenPtr>=9&&strncasecmp(tokenPtr,"translate",9)==0)
				{
				/* Parse a translation transformation: */
				tokenPtr+=9;
				while(tokenPtr!=end&&isspace(*tokenPtr))
					++tokenPtr;
				Geometry::Vector<ScalarParam,dimensionParam> t=ValueCoder<Geometry::Vector<ScalarParam,dimensionParam> >::decode(tokenPtr,end,&tokenPtr);
				result*=ONT::translate(t);
				}
			else if(end-tokenPtr>=6&&strncasecmp(tokenPtr,"rotate",6)==0)
				{
				/* Parse a rotation transformation: */
				tokenPtr+=6;
				while(tokenPtr!=end&&isspace(*tokenPtr))
					++tokenPtr;
				Geometry::Rotation<ScalarParam,dimensionParam> r=ValueCoder<Geometry::Rotation<ScalarParam,dimensionParam> >::decode(tokenPtr,end,&tokenPtr);
				result*=ONT::rotate(r);
				}
			else
				{
				throw 42;
				}
			
			/* Check for multiplication sign: */
			while(tokenPtr!=end&&isspace(*tokenPtr))
				++tokenPtr;
			if(tokenPtr==end||*tokenPtr!='*')
				break;
			++tokenPtr;
			while(tokenPtr!=end&&isspace(*tokenPtr))
				++tokenPtr;
			}
		
		if(decodeEnd!=0)
			*decodeEnd=tokenPtr;
		return result;
		}
	catch(...)
		{
		throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to Geometry::OrthonormalTransformation<Scalar,dimension>"));
		}
	}

/********************************************************************************************
Methods of class ValueCoder<Geometry::OrthogonalTransformation<ScalarParam,dimensionParam> >:
********************************************************************************************/

template <class ScalarParam,int dimensionParam>
METHODPREFIX
std::string
ValueCoder<Geometry::OrthogonalTransformation<ScalarParam,dimensionParam> >::encode(
	const Geometry::OrthogonalTransformation<ScalarParam,dimensionParam>& value)
	{
	std::string result="";
	result+="translate ";
	result+=ValueCoder<Geometry::Vector<ScalarParam,dimensionParam> >::encode(value.getTranslation());
	result+=" * ";
	result+="rotate ";
	result+=ValueCoder<Geometry::Rotation<ScalarParam,dimensionParam> >::encode(value.getRotation());
	result+=" * ";
	result+="scale ";
	result+=ValueCoder<ScalarParam>::encode(value.getScaling());
	return result;
	}

template <class ScalarParam,int dimensionParam>
METHODPREFIX
Geometry::OrthogonalTransformation<ScalarParam,dimensionParam>
ValueCoder<Geometry::OrthogonalTransformation<ScalarParam,dimensionParam> >::decode(
	const char* start,
	const char* end,
	const char** decodeEnd)
	{
	typedef Geometry::OrthogonalTransformation<ScalarParam,dimensionParam> OGT;
	
	try
		{
		OGT result=OGT::identity;
		const char* tokenPtr=start;
		while(true)
			{
			if(end-tokenPtr>=9&&strncasecmp(tokenPtr,"translate",9)==0)
				{
				/* Parse a translation transformation: */
				tokenPtr+=9;
				while(tokenPtr!=end&&isspace(*tokenPtr))
					++tokenPtr;
				Geometry::Vector<ScalarParam,dimensionParam> t=ValueCoder<Geometry::Vector<ScalarParam,dimensionParam> >::decode(tokenPtr,end,&tokenPtr);
				result*=OGT::translate(t);
				}
			else if(end-tokenPtr>=6&&strncasecmp(tokenPtr,"rotate",6)==0)
				{
				/* Parse a rotation transformation: */
				tokenPtr+=6;
				while(tokenPtr!=end&&isspace(*tokenPtr))
					++tokenPtr;
				Geometry::Rotation<ScalarParam,dimensionParam> r=ValueCoder<Geometry::Rotation<ScalarParam,dimensionParam> >::decode(tokenPtr,end,&tokenPtr);
				result*=OGT::rotate(r);
				}
			else if(end-tokenPtr>=5&&strncasecmp(tokenPtr,"scale",5)==0)
				{
				/* Parse a scaling transformation: */
				tokenPtr+=5;
				while(tokenPtr!=end&&isspace(*tokenPtr))
					++tokenPtr;
				ScalarParam scaling=ValueCoder<ScalarParam>::decode(tokenPtr,end,&tokenPtr);
				result*=OGT::scale(scaling);
				}
			else
				{
				throw 42;
				}
			
			/* Check for multiplication sign: */
			while(tokenPtr!=end&&isspace(*tokenPtr))
				++tokenPtr;
			if(tokenPtr==end||*tokenPtr!='*')
				break;
			++tokenPtr;
			while(tokenPtr!=end&&isspace(*tokenPtr))
				++tokenPtr;
			}
		
		if(decodeEnd!=0)
			*decodeEnd=tokenPtr;
		return result;
		}
	catch(...)
		{
		throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to Geometry::OrthogonalTransformation<Scalar,dimension>"));
		}
	}

/****************************************************************************************
Methods of class ValueCoder<Geometry::AffineTransformation<ScalarParam,dimensionParam> >:
****************************************************************************************/

template <class ScalarParam,int dimensionParam>
METHODPREFIX
std::string
ValueCoder<Geometry::AffineTransformation<ScalarParam,dimensionParam> >::encode(
	const Geometry::AffineTransformation<ScalarParam,dimensionParam>& value)
	{
	return ValueCoder<Geometry::Matrix<ScalarParam,dimensionParam,dimensionParam+1> >::encode(value.getMatrix());
	}

template <class ScalarParam,int dimensionParam>
METHODPREFIX
Geometry::AffineTransformation<ScalarParam,dimensionParam>
ValueCoder<Geometry::AffineTransformation<ScalarParam,dimensionParam> >::decode(
	const char* start,
	const char* end,
	const char** decodeEnd)
	{
	try
		{
		Geometry::AffineTransformation<ScalarParam,dimensionParam> result;
		result.getMatrix()=ValueCoder<Geometry::Matrix<ScalarParam,dimensionParam,dimensionParam+1> >::decode(start,end,decodeEnd);
		return result;
		}
	catch(...)
		{
		throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to Geometry::AffineTransformation<Scalar,dimension>"));
		}
	}

/********************************************************************************************
Methods of class ValueCoder<Geometry::ProjectiveTransformation<ScalarParam,dimensionParam> >:
********************************************************************************************/

template <class ScalarParam,int dimensionParam>
METHODPREFIX
std::string
ValueCoder<Geometry::ProjectiveTransformation<ScalarParam,dimensionParam> >::encode(
	const Geometry::ProjectiveTransformation<ScalarParam,dimensionParam>& value)
	{
	return ValueCoder<Geometry::Matrix<ScalarParam,dimensionParam+1,dimensionParam+1> >::encode(value.getMatrix());
	}

template <class ScalarParam,int dimensionParam>
METHODPREFIX
Geometry::ProjectiveTransformation<ScalarParam,dimensionParam>
ValueCoder<Geometry::ProjectiveTransformation<ScalarParam,dimensionParam> >::decode(
	const char* start,
	const char* end,
	const char** decodeEnd)
	{
	try
		{
		Geometry::ProjectiveTransformation<ScalarParam,dimensionParam> result;
		result.getMatrix()=ValueCoder<Geometry::Matrix<ScalarParam,dimensionParam+1,dimensionParam+1> >::decode(start,end,decodeEnd);
		return result;
		}
	catch(...)
		{
		throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to Geometry::ProjectiveTransformation<Scalar,dimension>"));
		}
	}

#if !defined(NONSTANDARD_TEMPLATES)

/******************************************************
Force instantiation of all standard ValueCoder classes:
******************************************************/

template class ValueCoder<Geometry::ComponentArray<int,2> >;
template class ValueCoder<Geometry::Vector<int,2> >;
template class ValueCoder<Geometry::Point<int,2> >;
template class ValueCoder<Geometry::HVector<int,2> >;
template class ValueCoder<Geometry::Box<int,2> >;
template class ValueCoder<Geometry::Matrix<int,2,2> >;
template class ValueCoder<Geometry::Matrix<int,2,3> >;

template class ValueCoder<Geometry::ComponentArray<int,3> >;
template class ValueCoder<Geometry::Vector<int,3> >;
template class ValueCoder<Geometry::Point<int,3> >;
template class ValueCoder<Geometry::HVector<int,3> >;
template class ValueCoder<Geometry::Box<int,3> >;
template class ValueCoder<Geometry::Matrix<int,3,3> >;
template class ValueCoder<Geometry::Matrix<int,3,4> >;

template class ValueCoder<Geometry::ComponentArray<int,4> >;
template class ValueCoder<Geometry::Matrix<int,4,4> >;

template class ValueCoder<Geometry::ComponentArray<float,2> >;
template class ValueCoder<Geometry::Vector<float,2> >;
template class ValueCoder<Geometry::Point<float,2> >;
template class ValueCoder<Geometry::HVector<float,2> >;
template class ValueCoder<Geometry::Box<float,2> >;
template class ValueCoder<Geometry::Plane<float,2> >;
template class ValueCoder<Geometry::Matrix<float,2,2> >;
template class ValueCoder<Geometry::Matrix<float,2,3> >;
template class ValueCoder<Geometry::Rotation<float,2> >;
template class ValueCoder<Geometry::OrthonormalTransformation<float,2> >;
template class ValueCoder<Geometry::OrthogonalTransformation<float,2> >;
template class ValueCoder<Geometry::AffineTransformation<float,2> >;
template class ValueCoder<Geometry::ProjectiveTransformation<float,2> >;

template class ValueCoder<Geometry::ComponentArray<float,3> >;
template class ValueCoder<Geometry::Vector<float,3> >;
template class ValueCoder<Geometry::Point<float,3> >;
template class ValueCoder<Geometry::HVector<float,3> >;
template class ValueCoder<Geometry::Box<float,3> >;
template class ValueCoder<Geometry::Plane<float,3> >;
template class ValueCoder<Geometry::Matrix<float,3,3> >;
template class ValueCoder<Geometry::Matrix<float,3,4> >;
template class ValueCoder<Geometry::Rotation<float,3> >;
template class ValueCoder<Geometry::OrthonormalTransformation<float,3> >;
template class ValueCoder<Geometry::OrthogonalTransformation<float,3> >;
template class ValueCoder<Geometry::AffineTransformation<float,3> >;
template class ValueCoder<Geometry::ProjectiveTransformation<float,3> >;

template class ValueCoder<Geometry::ComponentArray<float,4> >;
template class ValueCoder<Geometry::Matrix<float,4,4> >;

template class ValueCoder<Geometry::ComponentArray<double,2> >;
template class ValueCoder<Geometry::Vector<double,2> >;
template class ValueCoder<Geometry::Point<double,2> >;
template class ValueCoder<Geometry::HVector<double,2> >;
template class ValueCoder<Geometry::Box<double,2> >;
template class ValueCoder<Geometry::Plane<double,2> >;
template class ValueCoder<Geometry::Matrix<double,2,2> >;
template class ValueCoder<Geometry::Matrix<double,2,3> >;
template class ValueCoder<Geometry::Rotation<double,2> >;
template class ValueCoder<Geometry::OrthonormalTransformation<double,2> >;
template class ValueCoder<Geometry::OrthogonalTransformation<double,2> >;
template class ValueCoder<Geometry::AffineTransformation<double,2> >;
template class ValueCoder<Geometry::ProjectiveTransformation<double,2> >;

template class ValueCoder<Geometry::ComponentArray<double,3> >;
template class ValueCoder<Geometry::Vector<double,3> >;
template class ValueCoder<Geometry::Point<double,3> >;
template class ValueCoder<Geometry::HVector<double,3> >;
template class ValueCoder<Geometry::Box<double,3> >;
template class ValueCoder<Geometry::Plane<double,3> >;
template class ValueCoder<Geometry::Matrix<double,3,3> >;
template class ValueCoder<Geometry::Matrix<double,3,4> >;
template class ValueCoder<Geometry::Rotation<double,3> >;
template class ValueCoder<Geometry::OrthonormalTransformation<double,3> >;
template class ValueCoder<Geometry::OrthogonalTransformation<double,3> >;
template class ValueCoder<Geometry::AffineTransformation<double,3> >;
template class ValueCoder<Geometry::ProjectiveTransformation<double,3> >;

template class ValueCoder<Geometry::ComponentArray<double,4> >;
template class ValueCoder<Geometry::Matrix<double,4,4> >;

#endif

}
