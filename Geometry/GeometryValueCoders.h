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

#ifndef GEOMETRY_GEOMETRYVALUECODERS_INCLUDED
#define GEOMETRY_GEOMETRYVALUECODERS_INCLUDED

#include <Misc/ValueCoder.h>

/* Forward declarations: */
namespace Geometry {
template <class ScalarParam,int dimensionParam>
class ComponentArray;
template <class ScalarParam,int dimensionParam>
class Vector;
template <class ScalarParam,int dimensionParam>
class Point;
template <class ScalarParam,int dimensionParam>
class HVector;
template <class ScalarParam,int dimensionParam>
class Box;
template <class ScalarParam,int dimensionParam>
class Plane;
template <class ScalarParam,int dimensionParam>
class Rotation;
template <class ScalarParam,int dimensionParam>
class OrthonormalTransformation;
template <class ScalarParam,int dimensionParam>
class OrthogonalTransformation;
}

namespace Misc {

template <class ScalarParam,int dimensionParam>
class ValueCoder<Geometry::ComponentArray<ScalarParam,dimensionParam> >
	{
	/* Methods: */
	public:
	static std::string encode(const Geometry::ComponentArray<ScalarParam,dimensionParam>& value);
	static Geometry::ComponentArray<ScalarParam,dimensionParam> decode(const char* start,const char* end,const char** decodeEnd =0);
	};

template <class ScalarParam,int dimensionParam>
class ValueCoder<Geometry::Vector<ScalarParam,dimensionParam> >
	{
	/* Methods: */
	public:
	static std::string encode(const Geometry::Vector<ScalarParam,dimensionParam>& value);
	static Geometry::Vector<ScalarParam,dimensionParam> decode(const char* start,const char* end,const char** decodeEnd =0);
	};

template <class ScalarParam,int dimensionParam>
class ValueCoder<Geometry::Point<ScalarParam,dimensionParam> >
	{
	/* Methods: */
	public:
	static std::string encode(const Geometry::Point<ScalarParam,dimensionParam>& value);
	static Geometry::Point<ScalarParam,dimensionParam> decode(const char* start,const char* end,const char** decodeEnd =0);
	};

template <class ScalarParam,int dimensionParam>
class ValueCoder<Geometry::HVector<ScalarParam,dimensionParam> >
	{
	/* Methods: */
	public:
	static std::string encode(const Geometry::HVector<ScalarParam,dimensionParam>& value);
	static Geometry::HVector<ScalarParam,dimensionParam> decode(const char* start,const char* end,const char** decodeEnd =0);
	};

template <class ScalarParam,int dimensionParam>
class ValueCoder<Geometry::Box<ScalarParam,dimensionParam> >
	{
	/* Methods: */
	public:
	static std::string encode(const Geometry::Box<ScalarParam,dimensionParam>& value);
	static Geometry::Box<ScalarParam,dimensionParam> decode(const char* start,const char* end,const char** decodeEnd =0);
	};

template <class ScalarParam,int dimensionParam>
class ValueCoder<Geometry::Plane<ScalarParam,dimensionParam> >
	{
	/* Methods: */
	public:
	static std::string encode(const Geometry::Plane<ScalarParam,dimensionParam>& value);
	static Geometry::Plane<ScalarParam,dimensionParam> decode(const char* start,const char* end,const char** decodeEnd =0);
	};

template <class ScalarParam>
class ValueCoder<Geometry::Rotation<ScalarParam,2> >
	{
	/* Methods: */
	public:
	static std::string encode(const Geometry::Rotation<ScalarParam,2>& value);
	static Geometry::Rotation<ScalarParam,2> decode(const char* start,const char* end,const char** decodeEnd =0);
	};

template <class ScalarParam>
class ValueCoder<Geometry::Rotation<ScalarParam,3> >
	{
	/* Methods: */
	public:
	static std::string encode(const Geometry::Rotation<ScalarParam,3>& value);
	static Geometry::Rotation<ScalarParam,3> decode(const char* start,const char* end,const char** decodeEnd =0);
	};

template <class ScalarParam,int dimensionParam>
class ValueCoder<Geometry::OrthonormalTransformation<ScalarParam,dimensionParam> >
	{
	/* Methods: */
	public:
	static std::string encode(const Geometry::OrthonormalTransformation<ScalarParam,dimensionParam>& value);
	static Geometry::OrthonormalTransformation<ScalarParam,dimensionParam> decode(const char* start,const char* end,const char** decodeEnd =0);
	};

template <class ScalarParam,int dimensionParam>
class ValueCoder<Geometry::OrthogonalTransformation<ScalarParam,dimensionParam> >
	{
	/* Methods: */
	public:
	static std::string encode(const Geometry::OrthogonalTransformation<ScalarParam,dimensionParam>& value);
	static Geometry::OrthogonalTransformation<ScalarParam,dimensionParam> decode(const char* start,const char* end,const char** decodeEnd =0);
	};

}

#if defined(NONSTANDARD_TEMPLATES) && !defined(GEOMETRY_GEOMETRYVALUECODERS_IMPLEMENTATION)
#include <Geometry/GeometryValueCoders.cpp>
#endif

#endif
