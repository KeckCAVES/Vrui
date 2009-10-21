/***********************************************************************
ESRIShapeFileNode - Class to represent an ESRI shape file as a
collection of line sets, point sets, or face sets (each shape file can
only contain a single type of primitives).
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

#include <SceneGraph/ESRIShapeFileNode.h>

#include <string.h>
#include <Misc/SelfDestructPointer.h>
#include <Misc/File.h>
#include <Misc/FileCharacterSource.h>
#include <Misc/ValueSource.h>
#include <Misc/XBaseTable.h>
#include <Geometry/Point.h>
#include <Geometry/AffineCombiner.h>
#include <Geometry/Geoid.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/ShapeNode.h>
#include <SceneGraph/ColorNode.h>
#include <SceneGraph/CoordinateNode.h>
#include <SceneGraph/PointSetNode.h>
#include <SceneGraph/IndexedLineSetNode.h>
#include <SceneGraph/LabelSetNode.h>

namespace SceneGraph {

namespace {

/**************
Helper classes:
**************/

struct GeographicProjection // Structure for geographic map projections
	{
	/* Elements: */
	public:
	Geometry::Geoid<double> geoid; // Reference ellipsoid
	bool longitudeFirst; // Flag whether points are (longitude, latitude) or reversed
	double longitudeFactor; // Conversion factor from longitude units to radians
	double latitudeFactor; // Conversion factor from latitude units to radians
	double primeMeridianOffset; // Offset to WGS 84 prime meridian in radians
	
	/* Methods: */
	Point toCartesian(double x,double y,double z) const // Transforms a point in geographic coordinates to Cartesian coordinates
		{
		/* Assemble the source point's proper geodetic coordinates: */
		Geometry::Geoid<double>::Point geodetic;
		if(longitudeFirst)
			{
			geodetic[0]=x;
			geodetic[1]=y;
			}
		else
			{
			geodetic[0]=y;
			geodetic[1]=x;
			}
		geodetic[0]*=longitudeFactor;
		geodetic[0]+=primeMeridianOffset;
		geodetic[1]*=latitudeFactor;
		geodetic[2]=z;
		
		/* Convert the point to Cartesian: */
		return geoid.geodeticToCartesian(geodetic);
		}
	};

class MapProjection // Base class for map projections
	{
	/* Elements: */
	protected:
	GeographicProjection geoProjection; // Transformation from geodetic coordinates to Cartesian coordinates
	
	/* Constructors and destructors: */
	public:
	virtual ~MapProjection(void)
		{
		}
	
	/* Methods: */
	void setGeoProjection(const GeographicProjection& newGeoProjection) // Sets the map projection's geographic projection
		{
		geoProjection=newGeoProjection;
		}
	virtual Point toCartesian(double x,double y,double z) const // Transforms a point in projected coordinates to Cartesian coordinates
		{
		/* Pass the point directly to the geodetic projection: */
		return geoProjection.toCartesian(x,y,z);
		}
	};

class AlbersProjection:public MapProjection // Class for Albers equal-area conic projection
	{
	/* Elements: */
	public:
	double centralMeridian; // Central meridian in radians
	double centralParallel; // Central parallel in radians
	double standardParallels[2]; // Lower and upper standard parallels in radians
	double offset[2]; // False easting and northing in linear units
	double unitFactor; // Conversion factor from linear units to meters
	double a,e2,e,n,c,rho0; // Derived projection coefficients
	
	/* Methods from MapProjection: */
	virtual Point toCartesian(double x,double y,double z) const
		{
		x=(x-offset[0])*unitFactor;
		y=(y-offset[1])*unitFactor;
		
		double longitude=centralMeridian+Math::atan(x/(rho0-y))/n;
		
		double rho=Math::sqrt(Math::sqr(x)+Math::sqr(rho0-y));
		double q=(c-Math::sqr(rho*n/a))/n;
		double beta=Math::asin(q/(1.0-((1.0-e2)/(2.0*e))*Math::log((1.0-e)/(1.0+e))));
		double latitude=beta
		                +(e2*(1.0/3.0+e2*(31.0/180.0+e2*517.0/5040.0)))*Math::sin(2.0*beta)
		                +(e2*e2*(23.0/360.0+e2*251.0/3780.0))*Math::sin(4.0*beta)
		                +(e2*e2*e2*761.0/45360.0)*Math::sin(6.0*beta);
		return geoProjection.toCartesian(longitude,latitude,z);
		}
	
	/* New methods: */
	void update(void) // Updates derived projection coefficients
		{
		a=geoProjection.geoid.radius;
		e2=geoProjection.geoid.e2;
		e=Math::sqrt(e2);
		double sPhi0=Math::sin(centralParallel);
		double sPhi1=Math::sin(standardParallels[0]);
		double sPhi2=Math::sin(standardParallels[1]);
		double cPhi1=Math::cos(standardParallels[0]);
		double cPhi2=Math::cos(standardParallels[1]);
		double q0=(1.0-e2)*(sPhi0/(1.0-e2*Math::sqr(sPhi0))-(0.5/e)*Math::log((1.0-e*sPhi0)/(1.0+e*sPhi0)));
		double q1=(1.0-e2)*(sPhi1/(1.0-e2*Math::sqr(sPhi1))-(0.5/e)*Math::log((1.0-e*sPhi1)/(1.0+e*sPhi1)));
		double q2=(1.0-e2)*(sPhi2/(1.0-e2*Math::sqr(sPhi2))-(0.5/e)*Math::log((1.0-e*sPhi2)/(1.0+e*sPhi2)));
		double m1=cPhi1/Math::sqrt(1.0-e2*Math::sqr(sPhi1));
		double m2=cPhi2/Math::sqrt(1.0-e2*Math::sqr(sPhi2));
		n=(Math::sqr(m1)-Math::sqr(m2))/(q2-q1);
		c=Math::sqr(m1)+n*q1;
		rho0=a*Math::sqrt(c-n*q0)/n;
		geoProjection.longitudeFirst=true;
		geoProjection.longitudeFactor=1.0;
		geoProjection.latitudeFactor=1.0;
		}
	};

enum ESRIShapeType
	{
	NULLSHAPE=0,
	POINT=1,POLYLINE=3,POLYGON=5,MULTIPOINT=8,
	POINTZ=11,POLYLINEZ=13,POLYGONZ=15,MULTIPOINTZ=18,
	POINTM=21,POLYLINEM=23,POLYGONM=25,MULTIPOINTM=28,
	MULTIPATCH=31
	};

/****************
Helper functions:
****************/

void skipKeyword(Misc::ValueSource& prjFile)
	{
	/* Check for opening bracket: */
	if(prjFile.peekc()=='['||prjFile.peekc()=='(')
		{
		/* Skip the opening bracket: */
		prjFile.skipString();
		
		/* Read tokens until the next matching closing bracket or end of file: */
		size_t bracketLevel=1;
		while(!prjFile.eof()&&bracketLevel>0)
			{
			/* Check if the next token is an opening or closing bracket: */
			if(prjFile.peekc()=='['||prjFile.peekc()=='(')
				{
				/* Increase the bracket level: */
				++bracketLevel;
				}
			else if(prjFile.peekc()==']'||prjFile.peekc()==')')
				{
				/* Decrease the bracket level: */
				--bracketLevel;
				}
			
			/* Skip the token: */
			prjFile.skipString();
			}
		
		/* Check if the keyword was properly closed: */
		if(bracketLevel>0)
			throw 2; // Missing closing bracket
		}
	}

void skipOptionalFields(Misc::ValueSource& prjFile)
	{
	/* Read tokens until the next matching closing bracket or end of file: */
	size_t bracketLevel=1;
	while(!prjFile.eof()&&bracketLevel>0)
		{
		/* Check if the next token is an opening or closing bracket: */
		if(prjFile.peekc()=='['||prjFile.peekc()=='(')
			{
			/* Increase the bracket level: */
			++bracketLevel;
			}
		else if(prjFile.peekc()==']'||prjFile.peekc()==')')
			{
			/* Decrease the bracket level: */
			--bracketLevel;
			}
		
		/* Skip the token: */
		prjFile.skipString();
		}
	
	/* Check if the keyword was properly closed: */
	if(bracketLevel>0)
		throw 2; // Missing closing bracket
	}

Geometry::Geoid<double> parseSpheroid(Misc::ValueSource& prjFile)
	{
	/* Check for and skip the opening bracket: */
	if(prjFile.eof()||(prjFile.peekc()!='['&&prjFile.peekc()!='('))
		throw 1; // Missing opening bracket
	prjFile.skipString();
	
	/* Skip the spheroid name: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	prjFile.skipString();
	
	/* Check for and skip the field separator: */
	if(prjFile.peekc()!=',')
		throw 3; // Missing separator
	prjFile.skipString();
	
	/* Read the semi-major axis: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	double semimajorAxis=prjFile.readNumber();
	
	/* Check for and skip the field separator: */
	if(prjFile.peekc()!=',')
		throw 3; // Missing separator
	prjFile.skipString();
	
	/* Read the inverse flattening factor: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	double inverseFlatteningFactor=prjFile.readNumber();
	
	/* Skip optional fields: */
	skipOptionalFields(prjFile);
	
	/* Return the reference ellipsoid: */
	return Geometry::Geoid<double>(semimajorAxis,1.0/inverseFlatteningFactor);
	}

Geometry::Geoid<double> parseDatum(Misc::ValueSource& prjFile)
	{
	/* Check for and skip the opening bracket: */
	if(prjFile.eof()||(prjFile.peekc()!='['&&prjFile.peekc()!='('))
		throw 1; // Missing opening bracket
	prjFile.skipString();
	
	/* Skip the datum name: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	prjFile.skipString();
	
	/* Check for and skip the field separator: */
	if(prjFile.peekc()!=',')
		throw 3; // Missing separator
	prjFile.skipString();
	
	/* Read the reference ellipsoid: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	if(prjFile.readString()!="SPHEROID")
		throw 4; // Missing required value
	Geometry::Geoid<double> geoid=parseSpheroid(prjFile);
	
	/* Skip optional fields: */
	skipOptionalFields(prjFile);
	
	/* Return the reference ellipsoid: */
	return geoid;
	}

double parsePrimeMeridian(Misc::ValueSource& prjFile)
	{
	/* Check for and skip the opening bracket: */
	if(prjFile.eof()||(prjFile.peekc()!='['&&prjFile.peekc()!='('))
		throw 1; // Missing opening bracket
	prjFile.skipString();
	
	/* Skip the prime meridian name: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	prjFile.skipString();
	
	/* Check for and skip the field separator: */
	if(prjFile.peekc()!=',')
		throw 3; // Missing separator
	prjFile.skipString();
	
	/* Read the prime meridian's offset: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	double offset=prjFile.readNumber();
	
	/* Skip optional fields: */
	skipOptionalFields(prjFile);
	
	return offset;
	}

double parseUnit(Misc::ValueSource& prjFile)
	{
	/* Check for and skip the opening bracket: */
	if(prjFile.eof()||(prjFile.peekc()!='['&&prjFile.peekc()!='('))
		throw 1; // Missing opening bracket
	prjFile.skipString();
	
	/* Skip the linear or angular unit name: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	prjFile.skipString();
	
	/* Check for and skip the field separator: */
	if(prjFile.peekc()!=',')
		throw 3; // Missing separator
	prjFile.skipString();
	
	/* Read the linear or angular unit's conversion factor to meters or radians, respectively: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	double unitFactor=prjFile.readNumber();
	
	/* Skip optional fields: */
	skipOptionalFields(prjFile);
	
	return unitFactor;
	}

int parseAxis(Misc::ValueSource& prjFile)
	{
	/* Check for and skip the opening bracket: */
	if(prjFile.eof()||(prjFile.peekc()!='['&&prjFile.peekc()!='('))
		throw 1; // Missing opening bracket
	prjFile.skipString();
	
	/* Skip the axis name: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	prjFile.skipString();
	
	/* Check for and skip the field separator: */
	if(prjFile.peekc()!=',')
		throw 3; // Missing separator
	prjFile.skipString();
	
	/* Read the axis keyword: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	std::string axisKeyword=prjFile.readString();
	int axis=-1;
	if(axisKeyword=="NORTH")
		axis=0;
	else if(axisKeyword=="SOUTH")
		axis=1;
	else if(axisKeyword=="EAST")
		axis=2;
	else if(axisKeyword=="WEST")
		axis=3;
	else if(axisKeyword=="UP")
		axis=4;
	else if(axisKeyword=="DOWN")
		axis=5;
	
	/* Check for and skip the closing bracket: */
	if(prjFile.eof()||(prjFile.peekc()!=']'&&prjFile.peekc()!=')'))
		throw 2; // Missing closing bracket
	prjFile.skipString();
	
	return axis;
	}

GeographicProjection parseGeogcs(Misc::ValueSource& prjFile)
	{
	/* Check for and skip the opening bracket: */
	if(prjFile.eof()||(prjFile.peekc()!='['&&prjFile.peekc()!='('))
		throw 1; // Missing opening bracket
	prjFile.skipString();
	
	/* Skip the coordinate system name: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	prjFile.skipString();
	
	/* Check for and skip the field separator: */
	if(prjFile.peekc()!=',')
		throw 3; // Missing separator
	prjFile.skipString();
	
	/* Read the geographic datum: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	if(prjFile.readString()!="DATUM")
		throw 4; // Missing required value
	Geometry::Geoid<double> geoid=parseDatum(prjFile);
	
	/* Check for and skip the field separator: */
	if(prjFile.peekc()!=',')
		throw 3; // Missing separator
	prjFile.skipString();
	
	/* Read the prime meridian: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	if(prjFile.readString()!="PRIMEM")
		throw 4; // Missing required value
	double primeMeridianOffset=parsePrimeMeridian(prjFile);
	
	/* Check for and skip the field separator: */
	if(prjFile.peekc()!=',')
		throw 3; // Missing separator
	prjFile.skipString();
	
	/* Read the angular unit: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	if(prjFile.readString()!="UNIT")
		throw 4; // Missing required value
	double angularUnitFactor=parseUnit(prjFile);
	
	/* Check for optional axis specifications: */
	bool longitudeFirst=true;
	bool negateLongitude=false;
	bool negateLatitude=false;
	if(prjFile.peekc()==',')
		{
		/* Skip the field separator: */
		prjFile.skipString();
		
		/* Read the first axis specification: */
		if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
			throw 4; // Missing required value
		if(prjFile.readString()!="UNIT")
			throw 4; // Missing required value
		int axis0=parseAxis(prjFile);
		
		/* Check for and skip the field separator: */
		if(prjFile.peekc()!=',')
			throw 3; // Missing separator
		prjFile.skipString();
		
		/* Read the second axis specification: */
		if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
			throw 4; // Missing required value
		if(prjFile.readString()!="UNIT")
			throw 4; // Missing required value
		int axis1=parseAxis(prjFile);
		
		/* Set the geodetic coordinate flags: */
		if(axis0<0||axis0>3||axis1<0||axis1>3)
			throw 5; // Semantic error
		if((axis0<2&&axis1<2)||(axis0>=2&&axis1>=2))
			throw 5; // Semantic error
		if(axis0==0||axis0==1)
			longitudeFirst=false;
		if(axis0==3||axis1==3)
			negateLongitude=true;
		if(axis0==1||axis1==1)
			negateLatitude=true;
		}
	
	/* Skip optional fields: */
	skipOptionalFields(prjFile);
	
	/* Return the projection: */
	GeographicProjection result;
	result.geoid=geoid;
	result.longitudeFirst=longitudeFirst;
	result.longitudeFactor=negateLongitude?-angularUnitFactor:angularUnitFactor;
	result.latitudeFactor=negateLatitude?-angularUnitFactor:angularUnitFactor;
	result.primeMeridianOffset=primeMeridianOffset*angularUnitFactor;
	return result;
	}

MapProjection* parseAlbersProjection(const GeographicProjection& geogcs,Misc::ValueSource& prjFile)
	{
	/* Create the result projection: */
	Misc::SelfDestructPointer<AlbersProjection> result(new AlbersProjection);
	result->setGeoProjection(geogcs);
	
	/* Read all projection parameters: */
	while(prjFile.peekc()==',')
		{
		/* Skip the separator: */
		prjFile.skipString();
		
		/* Read the next keyword: */
		if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
			throw 4; // Missing required value
		
		std::string keyword=prjFile.readString();
		if(keyword=="PARAMETER")
			{
			/* Check for and skip the opening bracket: */
			if(prjFile.eof()||(prjFile.peekc()!='['&&prjFile.peekc()!='('))
				throw 1; // Missing opening bracket
			prjFile.skipString();
			
			/* Read the parameter name: */
			if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
				throw 4; // Missing required value
			std::string parameterName=prjFile.readString();
			
			/* Check for and skip the field separator: */
			if(prjFile.peekc()!=',')
				throw 3; // Missing separator
			prjFile.skipString();
			
			/* Read the parameter value: */
			if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
				throw 4; // Missing required value
			double parameterValue=prjFile.readNumber();
			
			/* Process the parameter: */
			if(parameterName=="Central_Meridian")
				result->centralMeridian=parameterValue*geogcs.longitudeFactor;
			else if(parameterName=="Latitude_Of_Origin")
				result->centralParallel=parameterValue*geogcs.latitudeFactor;
			else if(parameterName=="Standard_Parallel_1")
				result->standardParallels[0]=parameterValue*geogcs.latitudeFactor;
			else if(parameterName=="Standard_Parallel_2")
				result->standardParallels[1]=parameterValue*geogcs.latitudeFactor;
			else if(parameterName=="False_Easting")
				result->offset[0]=parameterValue;
			else if(parameterName=="False_Northing")
				result->offset[1]=parameterValue;
			else
				throw 5; // Unrecognized parameter
			
			/* Check for and skip the closing bracket: */
			if(prjFile.eof()||(prjFile.peekc()!=']'&&prjFile.peekc()!=')'))
				throw 2; // Missing closing bracket
			prjFile.skipString();
			}
		else if(keyword=="UNIT")
			{
			/* Read the linear unit's conversion factor to meters: */
			result->unitFactor=parseUnit(prjFile);
			}
		else if(keyword=="AXIS")
			{
			/* Parse the axis: */
			int axis=parseAxis(prjFile);
			}
		else
			{
			/* Skip the keyword: */
			skipKeyword(prjFile);
			}
		}
	
	/* Check for and skip the closing bracket: */
	if(prjFile.eof()||(prjFile.peekc()!=']'&&prjFile.peekc()!=')'))
		throw 2; // Missing closing bracket
	prjFile.skipString();
	
	result->update();
	return result.releaseTarget();
	}

MapProjection* parseProjcs(Misc::ValueSource& prjFile)
	{
	/* Check for and skip the opening bracket: */
	if(prjFile.eof()||(prjFile.peekc()!='['&&prjFile.peekc()!='('))
		throw 1; // Missing opening bracket
	prjFile.skipString();
	
	/* Skip the coordinate system name: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	prjFile.skipString();
	
	/* Check for and skip the field separator: */
	if(prjFile.peekc()!=',')
		throw 3; // Missing separator
	prjFile.skipString();
	
	/* Read the geographic projection: */
	if(prjFile.readString()!="GEOGCS")
		throw 4; // Missing required value
	GeographicProjection geogcs=parseGeogcs(prjFile);
	
	/* Check for and skip the field separator: */
	if(prjFile.peekc()!=',')
		throw 3; // Missing separator
	prjFile.skipString();
	
	/* Read the projection type: */
	if(prjFile.readString()!="PROJECTION")
		throw 4; // Missing required value
	
	/* Check for and skip the opening bracket: */
	if(prjFile.eof()||(prjFile.peekc()!='['&&prjFile.peekc()!='('))
		throw 1; // Missing opening bracket
	prjFile.skipString();
	
	/* Read the projection type name: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	std::string projectionName=prjFile.readString();
	
	/* Check for and skip the closing bracket: */
	if(prjFile.eof()||(prjFile.peekc()!=']'&&prjFile.peekc()!=')'))
		throw 2; // Missing closing bracket
	prjFile.skipString();
	
	/* Create a map projection: */
	MapProjection* result=0;
	if(projectionName=="Albers")
		result=parseAlbersProjection(geogcs,prjFile);
	else
		throw 5; // Unknown projection type
	
	return result;
	}

MapProjection* parseProjectionFile(Misc::ValueSource& prjFile)
	{
	MapProjection* result=0;
	
	/* Read tokens until the end of file: */
	while(!prjFile.eof())
		{
		/* Read the next keyword: */
		std::string keyword=prjFile.readString();
		
		if(keyword=="GEOGCS")
			{
			/* Parse a geographic coordinate system: */
			result=new MapProjection;
			result->setGeoProjection(parseGeogcs(prjFile));
			}
		else if(keyword=="PROJCS")
			{
			/* Parse a projected coordinate system: */
			result=parseProjcs(prjFile);
			}
		else
			{
			/* Skip the keyword: */
			skipKeyword(prjFile);
			}
		}
	
	if(result==0)
		throw 4; // Missing required value
	
	return result;
	}

void readPointArray(Misc::File& shapeFile,int numPoints,bool readZ,bool readM,const MapProjection* projection,CoordinateNode* coord)
	{
	/* Read all points into a temporary array: */
	Geometry::Point<double,3>* ps=new Geometry::Point<double,3>[numPoints];
	for(int i=0;i<numPoints;++i)
		{
		/* Read a single point: */
		ps[i][0]=shapeFile.read<double>();
		ps[i][1]=shapeFile.read<double>();
		ps[i][2]=0.0;
		}
	
	if(readZ)
		{
		/* Ignore the points' z range: */
		double zRange[2];
		shapeFile.read(zRange,2);
		
		/* Read the points' z coordinates: */
		for(int i=0;i<numPoints;++i)
			ps[i][2]=shapeFile.read<double>();
		}
	
	if(readM)
		{
		/* Ignore the points' measurement range: */
		double mRange[2];
		shapeFile.read(mRange,2);
		
		/* Ignore the points' measurements: */
		for(int i=0;i<numPoints;++i)
			shapeFile.read<double>();
		}
	
	/* Store all points in the point set: */
	if(projection!=0)
		{
		for(int i=0;i<numPoints;++i)
			coord->point.appendValue(projection->toCartesian(ps[i][0],ps[i][1],ps[i][2]));
		}
	else
		{
		for(int i=0;i<numPoints;++i)
			coord->point.appendValue(ps[i]);
		}
	
	delete[] ps;
	}

}

/**********************************
Methods of class ESRIShapeFileNode:
**********************************/

ESRIShapeFileNode::ESRIShapeFileNode(void)
	:transformToCartesian(false),pointSize(1.0f),lineWidth(1.0f)
	{
	}

void ESRIShapeFileNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"url")==0)
		{
		vrmlFile.parseField(url);
		}
	else if(strcmp(fieldName,"appearance")==0)
		{
		vrmlFile.parseSFNode(appearance);
		}
	else if(strcmp(fieldName,"labelField")==0)
		{
		vrmlFile.parseField(labelField);
		}
	else if(strcmp(fieldName,"fontStyle")==0)
		{
		vrmlFile.parseSFNode(fontStyle);
		}
	else if(strcmp(fieldName,"transformToCartesian")==0)
		{
		vrmlFile.parseField(transformToCartesian);
		}
	else if(strcmp(fieldName,"pointSize")==0)
		{
		vrmlFile.parseField(pointSize);
		}
	else if(strcmp(fieldName,"lineWidth")==0)
		{
		vrmlFile.parseField(lineWidth);
		}
	else
		GroupNode::parseField(fieldName,vrmlFile);
	}

void ESRIShapeFileNode::update(void)
	{
	/* Do nothing if there is no shape file name: */
	if(url.getNumValues()==0)
		return;
	
	/* Read an optional projection to Cartesian coordinates: */
	MapProjection* projection=0;
	bool applyProjection=transformToCartesian.getValue();
	if(applyProjection)
		{
		std::string prjFileName=url.getValue(0);
		prjFileName.append(".prj");
		try
			{
			/* Open the projection file: */
			Misc::FileCharacterSource prjFileSource(prjFileName.c_str());
			Misc::ValueSource prjFile(prjFileSource);
			prjFile.setPunctuation("[](),");
			prjFile.setQuotes("\"");
			prjFile.skipWs();
			
			/* Parse the projection file: */
			projection=parseProjectionFile(prjFile);
			}
		catch(std::runtime_error err)
			{
			Misc::throwStdErr("ESRIShapeFile::update: Unable to read projection file %s due to exception %s",prjFileName.c_str(),err.what());
			}
		catch(int errorCode)
			{
			Misc::throwStdErr("ESRIShapeFile::update: Malformed projection file %s",prjFileName.c_str());
			}
		}
	
	/* Open the shape file: */
	std::string shapeFileName=url.getValue(0);
	shapeFileName.append(".shp");
	Misc::File shapeFile(shapeFileName.c_str(),"rb");
	
	/****************************
	Read the shape file's header:
	****************************/
	
	/* The first set of fields are big-endian: */
	shapeFile.setEndianness(Misc::File::BigEndian);
	
	/* Check the file's magic number: */
	if(shapeFile.read<int>()!=9994)
		Misc::throwStdErr("ESRIShapeFile::update: Invalid magic number in file %s",shapeFileName.c_str());
	
	int dummy[5];
	shapeFile.read(dummy,5);
	
	/* Read the file size: */
	Misc::File::Offset fileSize=Misc::File::Offset(shapeFile.read<int>())*Misc::File::Offset(sizeof(short)); // File size in bytes
	
	/* The rest of the fields are little-endian: */
	shapeFile.setEndianness(Misc::File::LittleEndian);
	
	/* Check the file's version number: */
	if(shapeFile.read<int>()!=1000)
		Misc::throwStdErr("ESRIShapeFile::update: Unsupported version number in file %s",shapeFileName.c_str());
	
	/* Read the shape type: */
	int fileShapeType=shapeFile.read<unsigned int>();
	
	/* Read the bounding box: */
	Geometry::Box<double,3> shapeBBox;
	shapeBBox.min[0]=shapeFile.read<double>();
	shapeBBox.min[1]=shapeFile.read<double>();
	shapeBBox.max[0]=shapeFile.read<double>();
	shapeBBox.max[1]=shapeFile.read<double>();
	shapeBBox.min[2]=shapeFile.read<double>();
	shapeBBox.max[2]=shapeFile.read<double>();
	
	/* Skip the file's measurement range: */
	double mRange[2];
	shapeFile.read(mRange,2);
	
	/* Open the attribute file: */
	std::string attributeFileName=url.getValue(0);
	attributeFileName.append(".dbf");
	Misc::XBaseTable attributeFile(attributeFileName.c_str());
	
	/* Check if we need to create labels: */
	bool haveLabels=!labelField.getValue().empty();
	size_t labelFieldIndex=0;
	if(haveLabels)
		{
		/* Search the label field in the attribute file: */
		for(labelFieldIndex=0;labelFieldIndex<attributeFile.getNumFields();++labelFieldIndex)
			if(labelField.getValue()==attributeFile.getFieldName(labelFieldIndex))
				break;
		if(labelFieldIndex>=attributeFile.getNumFields())
			haveLabels=false;
		}
	
	/* Prepare the nodes retrieving geometry from shape file records: */
	ShapeNode* pointsShape=new ShapeNode;
	pointsShape->appearance.setValue(appearance.getValue());
	PointSetNode* points=new PointSetNode;
	pointsShape->geometry.setValue(points);
	CoordinateNode* pointsCoord=new CoordinateNode;
	points->coord.setValue(pointsCoord);
	points->pointSize.setValue(pointSize.getValue());
	
	ShapeNode* polylinesShape=new ShapeNode;
	polylinesShape->appearance.setValue(appearance.getValue());
	IndexedLineSetNode* polylines=new IndexedLineSetNode;
	polylinesShape->geometry.setValue(polylines);
	CoordinateNode* polylinesCoord=new CoordinateNode;
	polylines->coord.setValue(polylinesCoord);
	polylines->lineWidth.setValue(lineWidth.getValue());
	
	ShapeNode* labelsShape=0;
	LabelSetNode* labels=0;
	CoordinateNode* labelsCoord=0;
	if(haveLabels)
		{
		labelsShape=new ShapeNode;
		labelsShape->appearance.setValue(appearance.getValue());
		labels=new LabelSetNode;
		labelsShape->geometry.setValue(labels);
		labelsCoord=new CoordinateNode;
		labels->coord.setValue(labelsCoord);
		labels->fontStyle.setValue(fontStyle.getValue());
		}
	
	/* Read all records from the file: */
	Misc::XBaseTable::Record attributeRecord=attributeFile.makeRecord();
	size_t attributeRecordIndex=0;
	Misc::File::Offset filePos=shapeFile.tell();
	while(filePos<fileSize)
		{
		/* Read the next record header (which is big endian): */
		shapeFile.setEndianness(Misc::File::BigEndian);
		int recordNumber=shapeFile.read<int>();
		Misc::File::Offset recordSize=Misc::File::Offset(shapeFile.read<int>())*Misc::File::Offset(sizeof(short))+Misc::File::Offset(2*sizeof(int)); // Rexord size including header in bytes
		
		if(haveLabels)
			{
			/* Read the record's attribute record: */
			attributeFile.readRecord(attributeRecordIndex,attributeRecord);
			}
		
		/* Read the record itself (which is little endian): */
		shapeFile.setEndianness(Misc::File::LittleEndian);
		
		/* Read the shape type in the record and the shape definition: */
		int recordShapeType=shapeFile.read<int>();
		size_t recordFirstPointIndex=pointsCoord->point.getNumValues();
		size_t recordFirstPolylineIndex=polylinesCoord->point.getNumValues();
		bool isPolyline=false;
		size_t recordNumPoints=0;
		switch(recordShapeType)
			{
			case NULLSHAPE:
				/* No need to read anything: */
				break;
			
			case POINT:
			case POINTZ:
			case POINTM:
				{
				/* Read a single point: */
				double px=shapeFile.read<double>();
				double py=shapeFile.read<double>();
				double pz=0.0;
				
				if(recordShapeType==POINTZ)
					{
					/* Read the point's z component: */
					pz=shapeFile.read<double>();
					}
				
				if(recordShapeType==POINTZ||recordShapeType==POINTM)
					{
					/* Ignore the point's measurement: */
					shapeFile.read<double>();
					}
				
				/* Store the point in the point set: */
				isPolyline=false;
				recordNumPoints=1;
				if(projection!=0)
					pointsCoord->point.appendValue(projection->toCartesian(px,py,pz));
				else
					pointsCoord->point.appendValue(Point(px,py,pz));
				
				break;
				}
			
			case MULTIPOINT:
			case MULTIPOINTZ:
			case MULTIPOINTM:
				{
				/* Ignore the multi point set's bounding box: */
				double dummy[4];
				shapeFile.read(dummy,4);
				
				/* Read the number of points: */
				recordNumPoints=size_t(shapeFile.read<int>());
				
				/* Determine if the points have measurements: */
				bool readM=false;
				size_t minSize=sizeof(int)+4*sizeof(double)+sizeof(int); // Size of fixed record header
				minSize+=recordNumPoints*(2*sizeof(double)); // Size of 2D point array
				if(recordShapeType==MULTIPOINTZ)
					minSize+=2*sizeof(double)+recordNumPoints*sizeof(double); // Size of Z range and Z values
				minSize+=2*sizeof(double)+recordNumPoints*sizeof(double); // Size of M range and M values
				readM=(recordShapeType==MULTIPOINTZ||recordShapeType==MULTIPOINTM)&&recordSize>=Misc::File::Offset(minSize);
				
				/* Read the points and add them to the point set: */
				isPolyline=false;
				readPointArray(shapeFile,recordNumPoints,recordShapeType==MULTIPOINTZ,readM,projection,pointsCoord);
				
				break;
				}
			
			case POLYLINE:
			case POLYLINEZ:
			case POLYLINEM:
				{
				/* Ignore the polyline's bounding box: */
				double dummy[4];
				shapeFile.read(dummy,4);
				
				/* Read the number of parts and points: */
				int numParts=shapeFile.read<int>();
				recordNumPoints=size_t(shapeFile.read<int>());
				
				/* Read the start point indices for each part: */
				int* partStartIndices=new int[numParts+1];
				shapeFile.read(partStartIndices,numParts);
				partStartIndices[numParts]=int(recordNumPoints);
				
				/* Add vertex indices for all parts to the polyline set: */
				int polylinesIndexBase=polylinesCoord->point.getNumValues();
				for(int i=0;i<numParts;++i)
					{
					/* Add indices for vertices in this polyline: */
					for(int j=partStartIndices[i];j<partStartIndices[i+1];++j)
						polylines->coordIndex.appendValue(j+polylinesIndexBase);
					
					/* Terminate the polyline: */
					polylines->coordIndex.appendValue(-1);
					}
				delete[] partStartIndices;
				
				/* Determine if the points have measurements: */
				bool readM=false;
				size_t minSize=sizeof(int)+4*sizeof(double)+sizeof(int)+sizeof(int); // Size of fixed record header
				minSize+=numParts*sizeof(int); // Size of part start index array
				minSize+=recordNumPoints*(2*sizeof(double)); // Size of 2D point array
				if(recordShapeType==POLYLINEZ)
					minSize+=2*sizeof(double)+recordNumPoints*sizeof(double); // Size of Z range and Z values
				minSize+=2*sizeof(double)+recordNumPoints*sizeof(double); // Size of M range and M values
				readM=(recordShapeType==POLYLINEZ||recordShapeType==POLYLINEM)&&recordSize>=Misc::File::Offset(minSize);
				
				/* Read the points and add them to the polyline set: */
				isPolyline=true;
				readPointArray(shapeFile,recordNumPoints,recordShapeType==POLYLINEZ,readM,projection,polylinesCoord);
				
				break;
				}
			
			case POLYGON:
			case POLYGONZ:
			case POLYGONM:
				{
				/* Ignore the polygon's bounding box: */
				double dummy[4];
				shapeFile.read(dummy,4);
				
				/* Read the number of parts and points: */
				int numParts=shapeFile.read<int>();
				recordNumPoints=size_t(shapeFile.read<int>());
				
				/* Read the start point indices for each part: */
				int* partStartIndices=new int[numParts+1];
				shapeFile.read(partStartIndices,numParts);
				partStartIndices[numParts]=int(recordNumPoints);
				
				/* Add vertex indices for all parts to the polyline set: */
				int polylinesIndexBase=polylinesCoord->point.getNumValues();
				for(int i=0;i<numParts;++i)
					{
					/* Add indices for vertices in this polyline: */
					for(int j=partStartIndices[i];j<partStartIndices[i+1];++j)
						polylines->coordIndex.appendValue(j+polylinesIndexBase);
					
					/* Terminate the polyline: */
					polylines->coordIndex.appendValue(-1);
					}
				delete[] partStartIndices;
				
				/* Determine if the points have measurements: */
				bool readM=false;
				size_t minSize=sizeof(int)+4*sizeof(double)+sizeof(int)+sizeof(int); // Size of fixed record header
				minSize+=numParts*sizeof(int); // Size of part start index array
				minSize+=recordNumPoints*(2*sizeof(double)); // Size of 2D point array
				if(recordShapeType==POLYGONZ)
					minSize+=2*sizeof(double)+recordNumPoints*sizeof(double); // Size of Z range and Z values
				minSize+=2*sizeof(double)+recordNumPoints*sizeof(double); // Size of M range and M values
				readM=(recordShapeType==POLYGONZ||recordShapeType==POLYGONM)&&recordSize>=Misc::File::Offset(minSize);
				
				/* Read the points and add them to the polyline set: */
				isPolyline=true;
				readPointArray(shapeFile,recordNumPoints,recordShapeType==POLYGONZ,readM,projection,polylinesCoord);
				
				break;
				}
			
			case MULTIPATCH:
				{
				/* Ignore the polygon's bounding box: */
				double dummy[4];
				shapeFile.read(dummy,4);
				
				/* Read the number of parts and points: */
				int numParts=shapeFile.read<int>();
				recordNumPoints=size_t(shapeFile.read<int>());
				
				/* Read the start point indices for each part: */
				int* partStartIndices=new int[numParts+1];
				shapeFile.read(partStartIndices,numParts);
				partStartIndices[numParts]=int(recordNumPoints);
				
				/* Read the part types for each part: */
				int* partTypes=new int[numParts];
				shapeFile.read(partTypes,numParts);
				
				/* Add vertex indices for all parts to the polyline set: */
				int polylinesIndexBase=polylinesCoord->point.getNumValues();
				for(int i=0;i<numParts;++i)
					{
					switch(partTypes[i])
						{
						case 0: // Triangle strip
							/* Create a polyline following the strip's interior edges: */
							for(int j=partStartIndices[i]+1;j<partStartIndices[i+1]-1;++j)
								polylines->coordIndex.appendValue(j+polylinesIndexBase);
							polylines->coordIndex.appendValue(-1);
							
							/* Create a polyline following the strip's boundary: */
							for(int j=partStartIndices[i];j<partStartIndices[i+1];j+=2)
								polylines->coordIndex.appendValue(j+polylinesIndexBase);
							for(int j=partStartIndices[i+1]-1-(partStartIndices[i+1]-partStartIndices[i])%2;j>=partStartIndices[i];j-=2)
								polylines->coordIndex.appendValue(j+polylinesIndexBase);
							polylines->coordIndex.appendValue(partStartIndices[i]+polylinesIndexBase);
							polylines->coordIndex.appendValue(-1);
							break;
						
						case 1: // Triangle fan
							/* Create polylines for the fan's interior edges: */
							for(int j=partStartIndices[i]+2;j<partStartIndices[i+1]-1;++j)
								{
								polylines->coordIndex.appendValue(partStartIndices[i]+polylinesIndexBase);
								polylines->coordIndex.appendValue(j+polylinesIndexBase);
								polylines->coordIndex.appendValue(-1);
								}
							
							/* Create a polyline following the fan's boundary: */
							for(int j=partStartIndices[i];j<partStartIndices[i+1];++j)
								polylines->coordIndex.appendValue(j+polylinesIndexBase);
							polylines->coordIndex.appendValue(partStartIndices[i]+polylinesIndexBase);
							polylines->coordIndex.appendValue(-1);
							break;
						
						case 2: // Outer ring
						case 3: // Inner ring
						case 4: // First ring
						case 5: // Ring
							/* Add indices for vertices in this polygon: */
							for(int j=partStartIndices[i];j<partStartIndices[i+1];++j)
								polylines->coordIndex.appendValue(j+polylinesIndexBase);

							/* Terminate the polyline: */
							polylines->coordIndex.appendValue(-1);
							break;
						}
					
					}
				delete[] partStartIndices;
				delete[] partTypes;
				
				/* Determine if the points have measurements: */
				bool readM=false;
				size_t minSize=sizeof(int)+4*sizeof(double)+sizeof(int)+sizeof(int); // Size of fixed record header
				minSize+=numParts*sizeof(int); // Size of part start index array
				minSize+=numParts*sizeof(int); // Size of part type array
				minSize+=recordNumPoints*(2*sizeof(double)); // Size of 2D point array
				minSize+=2*sizeof(double)+recordNumPoints*sizeof(double); // Size of Z range and Z values
				minSize+=2*sizeof(double)+recordNumPoints*sizeof(double); // Size of M range and M values
				readM=recordSize>=Misc::File::Offset(minSize);
				
				/* Read the points and add them to the polyline set: */
				isPolyline=true;
				readPointArray(shapeFile,recordNumPoints,true,readM,projection,polylinesCoord);
				
				break;
				}
			}
				
		if(haveLabels&&recordNumPoints>0)
			{
			/* Create a label for the record: */
			Misc::XBaseTable::Maybe<std::string> label=attributeFile.getFieldString(attributeRecord,labelFieldIndex);
			if(label.defined)
				{
				labels->string.appendValue(label.value);

				/* Calculate the record's centroid: */
				Point::AffineCombiner cc;
				if(isPolyline)
					{
					for(size_t i=0;i<recordNumPoints;++i)
						cc.addPoint(polylinesCoord->point.getValue(recordFirstPolylineIndex+i));
					}
				else
					{
					for(size_t i=0;i<recordNumPoints;++i)
						cc.addPoint(pointsCoord->point.getValue(recordFirstPointIndex+i));
					}
				labelsCoord->point.appendValue(cc.getPoint());
				}
			}
		
		/* Go to the next record: */
		filePos+=recordSize;
		if(filePos!=shapeFile.tell())
			Misc::throwStdErr("ESRIShapeFile::update: Record with invalid size %u in file %s",(unsigned int)recordSize,shapeFileName.c_str());
		++attributeRecordIndex;
		}
	
	/* Finalize the generated nodes: */
	pointsCoord->update();
	points->update();
	pointsShape->update();
	polylinesCoord->update();
	polylines->update();
	polylinesShape->update();
	if(haveLabels)
		{
		labelsCoord->update();
		labels->update();
		labelsShape->update();
		}
	
	/* Store all generated nodes as children: */
	if(pointsCoord->point.getNumValues()>0)
		children.appendValue(pointsShape);
	if(polylinesCoord->point.getNumValues()>0)
		children.appendValue(polylinesShape);
	if(haveLabels&&labelsCoord->point.getNumValues()>0)
		children.appendValue(labelsShape);
	GroupNode::update();
	
	/* Clean up: */
	delete projection;
	}

}
