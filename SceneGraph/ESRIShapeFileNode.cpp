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
#include <Misc/File.h>
#include <Misc/FileCharacterSource.h>
#include <Misc/ValueSource.h>
#include <Geometry/Geoid.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/ShapeNode.h>
#include <SceneGraph/ColorNode.h>
#include <SceneGraph/CoordinateNode.h>
#include <SceneGraph/PointSetNode.h>
#include <SceneGraph/IndexedLineSetNode.h>

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

double parseAngularUnit(Misc::ValueSource& prjFile)
	{
	/* Check for and skip the opening bracket: */
	if(prjFile.eof()||(prjFile.peekc()!='['&&prjFile.peekc()!='('))
		throw 1; // Missing opening bracket
	prjFile.skipString();
	
	/* Skip the angular unit name: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	prjFile.skipString();
	
	/* Check for and skip the field separator: */
	if(prjFile.peekc()!=',')
		throw 3; // Missing separator
	prjFile.skipString();
	
	/* Read the angular unit's conversion factor to radians: */
	if(prjFile.eof()||prjFile.peekc()==']'||prjFile.peekc()==')')
		throw 4; // Missing required value
	double radiansFactor=prjFile.readNumber();
	
	/* Skip optional fields: */
	skipOptionalFields(prjFile);
	
	return radiansFactor;
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

GeographicProjection parseGgcs(Misc::ValueSource& prjFile)
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
	double angularUnitFactor=parseAngularUnit(prjFile);
	
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

GeographicProjection parseProjectionFile(Misc::ValueSource& prjFile)
	{
	GeographicProjection result;
	bool haveProjection=false;
	
	/* Read tokens until the end of file: */
	while(!prjFile.eof())
		{
		/* Read the next keyword: */
		std::string keyword=prjFile.readString();
		
		if(keyword=="GEOGCS")
			{
			/* Parse a geographic coordinate system: */
			result=parseGgcs(prjFile);
			haveProjection=true;
			}
		else
			{
			/* Skip the keyword: */
			skipKeyword(prjFile);
			}
		}
	
	if(!haveProjection)
		throw 4; // Missing required value
	
	return result;
	}

void readPointArray(Misc::File& shapeFile,int numPoints,bool readZ,bool readM,const GeographicProjection& projection,bool applyProjection,CoordinateNode* coord)
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
		shapeFile.read(mRange[2]);
		
		/* Ignore the points' measurements: */
		for(int i=0;i<numPoints;++i)
			shapeFile.read<double>();
		}
	
	/* Store all points in the point set: */
	if(applyProjection)
		{
		for(int i=0;i<numPoints;++i)
			coord->point.appendValue(projection.toCartesian(ps[i][0],ps[i][1],ps[i][2]));
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
	GeographicProjection projection;
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
	
	/* Read all records from the file: */
	Misc::File::Offset filePos=shapeFile.tell();
	while(filePos<fileSize)
		{
		/* Read the next record header (which is big endian): */
		shapeFile.setEndianness(Misc::File::BigEndian);
		int recordNumber=shapeFile.read<int>();
		Misc::File::Offset recordSize=Misc::File::Offset(shapeFile.read<int>())*Misc::File::Offset(sizeof(short))+Misc::File::Offset(2*sizeof(int)); // Rexord size including header in bytes
		
		/* Read the record itself (which is little endian): */
		shapeFile.setEndianness(Misc::File::LittleEndian);
		
		/* Read the shape type in the record and the shape definition: */
		int recordShapeType=shapeFile.read<int>();
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
				if(applyProjection)
					pointsCoord->point.appendValue(projection.toCartesian(px,py,pz));
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
				int numPoints=shapeFile.read<int>();
				
				/* Determine if the points have measurements: */
				bool readM=false;
				size_t minSize=sizeof(int)+4*sizeof(double)+sizeof(int); // Size of fixed record header
				minSize+=numPoints*(2*sizeof(double)); // Size of 2D point array
				if(recordShapeType==MULTIPOINTZ)
					minSize+=2*sizeof(double)+numPoints*sizeof(double); // Size of Z range and Z values
				minSize+=2*sizeof(double)+numPoints*sizeof(double); // Size of M range and M values
				readM=(recordShapeType==MULTIPOINTZ||recordShapeType==MULTIPOINTM)&&recordSize>=Misc::File::Offset(minSize);
				
				/* Read the points and add them to the point set: */
				readPointArray(shapeFile,numPoints,recordShapeType==MULTIPOINTZ,readM,projection,applyProjection,pointsCoord);
				
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
				int numPoints=shapeFile.read<int>();
				
				/* Read the start point indices for each part: */
				int* partStartIndices=new int[numParts+1];
				shapeFile.read(partStartIndices,numParts);
				partStartIndices[numParts]=numPoints;
				
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
				minSize+=numPoints*(2*sizeof(double)); // Size of 2D point array
				if(recordShapeType==POLYLINEZ)
					minSize+=2*sizeof(double)+numPoints*sizeof(double); // Size of Z range and Z values
				minSize+=2*sizeof(double)+numPoints*sizeof(double); // Size of M range and M values
				readM=(recordShapeType==POLYLINEZ||recordShapeType==POLYLINEM)&&recordSize>=Misc::File::Offset(minSize);
				
				/* Read the points and add them to the polyline set: */
				readPointArray(shapeFile,numPoints,recordShapeType==POLYLINEZ,readM,projection,applyProjection,polylinesCoord);
				
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
				int numPoints=shapeFile.read<int>();
				
				/* Read the start point indices for each part: */
				int* partStartIndices=new int[numParts+1];
				shapeFile.read(partStartIndices,numParts);
				partStartIndices[numParts]=numPoints;
				
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
				minSize+=numPoints*(2*sizeof(double)); // Size of 2D point array
				if(recordShapeType==POLYGONZ)
					minSize+=2*sizeof(double)+numPoints*sizeof(double); // Size of Z range and Z values
				minSize+=2*sizeof(double)+numPoints*sizeof(double); // Size of M range and M values
				readM=(recordShapeType==POLYGONZ||recordShapeType==POLYGONM)&&recordSize>=Misc::File::Offset(minSize);
				
				/* Read the points and add them to the polyline set: */
				readPointArray(shapeFile,numPoints,recordShapeType==POLYGONZ,readM,projection,applyProjection,polylinesCoord);
				
				break;
				}
			
			case MULTIPATCH:
				{
				/* Ignore the polygon's bounding box: */
				double dummy[4];
				shapeFile.read(dummy,4);
				
				/* Read the number of parts and points: */
				int numParts=shapeFile.read<int>();
				int numPoints=shapeFile.read<int>();
				
				/* Read the start point indices for each part: */
				int* partStartIndices=new int[numParts+1];
				shapeFile.read(partStartIndices,numParts);
				partStartIndices[numParts]=numPoints;
				
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
				minSize+=numPoints*(2*sizeof(double)); // Size of 2D point array
				minSize+=2*sizeof(double)+numPoints*sizeof(double); // Size of Z range and Z values
				minSize+=2*sizeof(double)+numPoints*sizeof(double); // Size of M range and M values
				readM=recordSize>=Misc::File::Offset(minSize);
				
				/* Read the points and add them to the polyline set: */
				readPointArray(shapeFile,numPoints,true,readM,projection,applyProjection,polylinesCoord);
				
				break;
				}
			}
		
		/* Go to the next record: */
		filePos+=recordSize;
		if(filePos!=shapeFile.tell())
			Misc::throwStdErr("ESRIShapeFile::update: Record with invalid size %u in file %s",(unsigned int)recordSize,shapeFileName.c_str());
		}
	
	/* Finalize the generated nodes: */
	pointsCoord->update();
	points->update();
	pointsShape->update();
	polylinesCoord->update();
	polylines->update();
	polylinesShape->update();
	
	/* Store all generated nodes as children: */
	if(pointsCoord->point.getNumValues()>0)
		children.appendValue(pointsShape);
	if(polylinesCoord->point.getNumValues()>0)
	children.appendValue(polylinesShape);
	GroupNode::update();
	}

}
