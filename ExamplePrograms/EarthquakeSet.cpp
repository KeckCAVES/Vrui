/***********************************************************************
EarthquakeSet - Class to represent and render sets of earthquakes with
3D locations, magnitude and event time.
Copyright (c) 2006-2011 Oliver Kreylos

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

#include "EarthquakeSet.h"

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <algorithm>
#include <Misc/ThrowStdErr.h>
#include <Misc/FileNameExtensions.h>
#include <IO/ValueSource.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/HVector.h>
#include <Geometry/ValuedPoint.h>
#include <Geometry/ArrayKdTree.h>
#include <GL/gl.h>
#include <GL/GLVertexArrayTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBMultitexture.h>
#include <GL/Extensions/GLARBPointParameters.h>
#include <GL/Extensions/GLARBPointSprite.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/GLShader.h>
#define NONSTANDARD_GLVERTEX_TEMPLATES
#include <GL/GLGeometryVertex.h>

#include "EarthFunctions.h"

namespace {

/***********************************************************************
Helper classes and functions to parse spreadsheet files in text format:
***********************************************************************/

bool strequal(std::string s1,const char* s2)
	{
	std::string::const_iterator s1It=s1.begin();
	const char* s2Ptr=s2;
	while(s1It!=s1.end()&&*s2Ptr!='\0'&&tolower(*s1It)==tolower(*s2Ptr))
		{
		++s1It;
		++s2Ptr;
		}
	return s1It==s1.end()&&*s2Ptr=='\0';
	}

double parseDateTime(const char* date,const char* time)
	{
	struct tm dateTime;
	
	/* Parse the date: */
	int d[3];
	char* nextPtr=const_cast<char*>(date); // Ugly, but required by API
	for(int i=0;i<3;++i)
		{
		d[i]=strtol(nextPtr,&nextPtr,10);
		if((i<2&&*nextPtr!='/')||(i==2&&*nextPtr!='\0'))
			Misc::throwStdErr("Format error in date string %s",date);
		++nextPtr;
		}
	if(d[0]>=1&&d[0]<=31&&d[1]>=1&&d[1]<=31)
		{
		/* Format is month, day, year: */
		dateTime.tm_mday=d[1];
		dateTime.tm_mon=d[0]-1;
		dateTime.tm_year=d[2]-1900;
		}
	else if(d[1]>=1&&d[1]<=31&&d[2]>=1&&d[2]<=31)
		{
		/* Format is year, month, day: */
		dateTime.tm_mday=d[2];
		dateTime.tm_mon=d[1]-1;
		dateTime.tm_year=d[0]-1900;
		}
	else
		Misc::throwStdErr("Format error in date string %s",date);
	
	/* Parse the time: */
	int t[3];
	nextPtr=const_cast<char*>(time); // Ugly, but required by API
	for(int i=0;i<3;++i)
		{
		t[i]=strtol(nextPtr,&nextPtr,10);
		if((i<2&&*nextPtr!=':')||(i==2&&*nextPtr!='.'&&*nextPtr!='\0'))
			Misc::throwStdErr("Format error in time string %s",time);
		++nextPtr;
		}
	
	/* Format is hour, minute, second: */
	if(t[0]<0||t[0]>23||t[1]<0||t[1]>59||t[2]<0||t[2]>60)
		Misc::throwStdErr("Format error in time string %s",time);
	dateTime.tm_sec=t[2];
	dateTime.tm_min=t[1];
	dateTime.tm_hour=t[0];
	
	/* Convert the date/time structure to seconds since the epoch: */
	dateTime.tm_isdst=-1;
	return double(mktime(&dateTime));
	}

}

/****************************************
Methods of class EarthquakeSet::DataItem:
****************************************/

EarthquakeSet::DataItem::DataItem(void)
	:vertexBufferObjectId(0),pointRenderer(0),
	 scaledPointRadiusLocation(-1),highlightTimeLocation(-1),
	 currentTimeLocation(-1),pointTextureLocation(-1),
	 pointTextureObjectId(0),
	 eyePos(Point::origin),
	 sortedPointIndicesBufferObjectId(0)
	{
	/* Check if the vertex buffer object extension is supported: */
	if(GLARBVertexBufferObject::isSupported())
		{
		/* Initialize the vertex buffer object extension: */
		GLARBVertexBufferObject::initExtension();
		
		/* Create a vertex buffer object: */
		glGenBuffersARB(1,&vertexBufferObjectId);
		
		/* Check if GLSL shaders are supported: */
		if(GLARBMultitexture::isSupported()&&GLARBPointParameters::isSupported()&&GLARBPointSprite::isSupported()&&GLShader::isSupported())
			{
			/* Initialize the basic OpenGL extensions: */
			GLARBMultitexture::initExtension();
			GLARBPointParameters::initExtension();
			GLARBPointSprite::initExtension();

			/* Create the shader object: */
			pointRenderer=new GLShader;

			/* Create the point texture object: */
			glGenTextures(1,&pointTextureObjectId);

			/* Create the sorted point index buffer: */
			glGenBuffersARB(1,&sortedPointIndicesBufferObjectId);
			}
		}
	}

EarthquakeSet::DataItem::~DataItem(void)
	{
	/* Check if the vertex buffer object extension is supported: */
	if(vertexBufferObjectId>0)
		{
		/* Destroy the vertex buffer object: */
		glDeleteBuffersARB(1,&vertexBufferObjectId);
		
		/* Check if GLSL shaders are supported: */
		if(pointRenderer!=0)
			{
			/* Destroy the shader object: */
			delete pointRenderer;
			
			/* Delete the point texture object: */
			glDeleteTextures(1,&pointTextureObjectId);
			
			/* Delete the sorted point index buffer: */
			glDeleteBuffersARB(1,&sortedPointIndicesBufferObjectId);
			}
		}
	}

/******************************
Methods of class EarthquakeSet:
******************************/

void EarthquakeSet::loadANSSFile(IO::FilePtr earthquakeFile,double scaleFactor)
	{
	/* Wrap a value source around the input file: */
	IO::ValueSource source(earthquakeFile);
	source.setPunctuation("\n");
	source.skipWs();
	
	/* Skip the two header lines: */
	for(int i=0;i<2;++i)
		{
		source.skipLine();
		source.skipWs();
		}
	
	/* Read the rest of the file: */
	while(!source.eof())
		{
		/* Read the next line: */
		std::string line=source.readLine();
		source.skipWs();
		
		/* Skip empty lines: */
		if(line.empty()||line[0]=='\r')
			continue;
		
		/* Parse an event from the line: */
		Event e;
		
		/* Read date and time: */
		std::string date(line,0,10);
		std::string time(line,11,11);
		e.time=parseDateTime(date.c_str(),time.c_str());
		
		/* Read event position: */
		float sphericalCoordinates[3];
		
		/* Read latitude: */
		std::string lat(line,23,8);
		sphericalCoordinates[0]=Math::rad(float(atof(lat.c_str())));
		
		/* Read longitude: */
		std::string lon(line,32,9);
		sphericalCoordinates[1]=Math::rad(float(atof(lon.c_str())));
		
		/* Read depth: */
		std::string dep(line,42,6);
		sphericalCoordinates[2]=float(atof(dep.c_str()));
		
		/* Convert the spherical position to Cartesian: */
		calcDepthPos<float>(sphericalCoordinates[0],sphericalCoordinates[1],sphericalCoordinates[2]*1000.0f,scaleFactor,e.position.getComponents());
		
		/* Read magnitude: */
		std::string mag(line,49,5);
		e.magnitude=float(atof(mag.c_str()));
		
		/* Save the event: */
		events.push_back(e);
		}
	}

void EarthquakeSet::loadCSVFile(const char* earthquakeFileName,IO::FilePtr earthquakeFile,double scaleFactor)
	{
	/* Wrap a value source around the input file: */
	IO::ValueSource source(earthquakeFile);
	source.setPunctuation(",\n");
	source.setQuotes("\"");
	source.skipWs();
	
	/*********************************************************************
	Parse the point file's header line:
	*********************************************************************/
	
	/* Remember the column indices of important columns: */
	int latIndex=-1;
	int lngIndex=-1;
	int radiusIndex=-1;
	enum RadiusMode // Enumerated types for radius coordinate modes
		{
		RADIUS,DEPTH,NEGDEPTH
		} radiusMode=RADIUS;
	int dateIndex=-1;
	int timeIndex=-1;
	int magIndex=-1;
	
	/* Read the header line's columns: */
	int column=0;
	while(true)
		{
		/* Read the next column header: */
		std::string header=!source.eof()&&source.peekc()!='\n'&&source.peekc()!=','?source.readString():"";
		
		/* Parse the column header: */
		if(strequal(header,"Latitude")||strequal(header,"Lat"))
			latIndex=column;
		else if(strequal(header,"Longitude")||strequal(header,"Long")||strequal(header,"Lon"))
			lngIndex=column;
		else if(strequal(header,"Radius"))
			{
			radiusIndex=column;
			radiusMode=RADIUS;
			}
		else if(strequal(header,"Depth"))
			{
			radiusIndex=column;
			radiusMode=DEPTH;
			}
		else if(strequal(header,"Negative Depth")||strequal(header,"Neg Depth")||strequal(header,"NegDepth"))
			{
			radiusIndex=column;
			radiusMode=NEGDEPTH;
			}
		else if(strequal(header,"Date"))
			dateIndex=column;
		else if(strequal(header,"Time"))
			timeIndex=column;
		else if(strequal(header,"Magnitude")||strequal(header,"Mag"))
			magIndex=column;
		
		++column;
		
		/* Check for end of line: */
		if(source.eof()||source.peekc()=='\n')
			break;
		
		/* Skip an optional comma: */
		if(source.peekc()==',')
			source.skipString();
		}
	
	/* Determine the number of fields: */
	int maxIndex=latIndex;
	if(maxIndex<lngIndex)
		maxIndex=lngIndex;
	if(maxIndex<radiusIndex)
		maxIndex=radiusIndex;
	if(maxIndex<dateIndex)
		maxIndex=dateIndex;
	if(maxIndex<timeIndex)
		maxIndex=timeIndex;
	if(maxIndex<magIndex)
		maxIndex=magIndex;
	
	/* Skip the newline: */
	source.skipLine();
	source.skipWs();
	
	/* Check if all required portions have been detected: */
	if(latIndex<0||lngIndex<0||radiusIndex<0||dateIndex<0||timeIndex<0||magIndex<0)
		Misc::throwStdErr("EarthquakeSet::EarthquakeSet: Missing earthquake components in input file %s",earthquakeFileName);
	
	/* Read lines from the file: */
	int lineNumber=2;
	while(!source.eof())
		{
		float sphericalCoordinates[3]={0.0f,0.0f,0.0f};
		std::string date,time;
		float magnitude=0.0f;
		int column=0;
		try
			{
			while(true)
				{
				/* Read the next field: */
				if(!source.eof()&&source.peekc()!='\n'&&source.peekc()!=',')
					{
					if(column==latIndex)
						sphericalCoordinates[0]=Math::rad(float(source.readNumber()));
					else if(column==lngIndex)
						sphericalCoordinates[1]=Math::rad(float(source.readNumber()));
					else if(column==radiusIndex)
						sphericalCoordinates[2]=float(source.readNumber());
					else if(column==dateIndex)
						date=source.readString();
					else if(column==timeIndex)
						time=source.readString();
					else if(column==magIndex)
						magnitude=float(source.readNumber());
					else
						source.skipString();
					}
				
				++column;
				
				/* Check for end of line: */
				if(source.eof()||source.peekc()=='\n')
					break;
				
				/* Skip an optional comma: */
				if(source.peekc()==',')
					source.skipString();
				}
			}
		catch(IO::ValueSource::NumberError err)
			{
			/* Ignore the error and the malformed event */
			}
		
		/* Skip the newline: */
		source.skipLine();
		source.skipWs();
		
		/* Check if all fields were read: */
		if(column>maxIndex)
			{
			/* Create an event: */
			Event e;
			
			/* Convert the read spherical coordinates to Cartesian coordinates: */
			switch(radiusMode)
				{
				case RADIUS:
					calcRadiusPos<float>(sphericalCoordinates[0],sphericalCoordinates[1],sphericalCoordinates[2]*1000.0f,scaleFactor,e.position.getComponents());
					break;
				
				case DEPTH:
					calcDepthPos<float>(sphericalCoordinates[0],sphericalCoordinates[1],sphericalCoordinates[2]*1000.0f,scaleFactor,e.position.getComponents());
					break;
				
				case NEGDEPTH:
					calcDepthPos<float>(sphericalCoordinates[0],sphericalCoordinates[1],-sphericalCoordinates[2]*1000.0f,scaleFactor,e.position.getComponents());
					break;
				}
			
			/* Calculate the event time: */
			e.time=parseDateTime(date.c_str(),time.c_str());
			
			/* Store the event magnitude: */
			e.magnitude=magnitude;
			
			/* Append the event to the earthquake set: */
			events.push_back(e);
			}
		
		++lineNumber;
		}
	}

void EarthquakeSet::drawBackToFront(int left,int right,int splitDimension,const Point& eyePos,GLuint*& bufferPtr) const
	{
	/* Get the current node index: */
	int mid=(left+right)>>1;
	
	int childSplitDimension=splitDimension+1;
	if(childSplitDimension==3)
		childSplitDimension=0;
	
	/* Traverse into the subtree on the far side of the split plane first: */
	if(eyePos[splitDimension]>events[treePointIndices[mid]].position[splitDimension])
		{
		/* Traverse left child: */
		if(left<mid)
			drawBackToFront(left,mid-1,childSplitDimension,eyePos,bufferPtr);
		
		/* Draw the point: */
		*bufferPtr=GLuint(mid);
		++bufferPtr;
		
		/* Traverse right child: */
		if(right>mid)
			drawBackToFront(mid+1,right,childSplitDimension,eyePos,bufferPtr);
		}
	else
		{
		/* Traverse right child: */
		if(right>mid)
			drawBackToFront(mid+1,right,childSplitDimension,eyePos,bufferPtr);
		
		/* Draw the point: */
		*bufferPtr=GLuint(mid);
		++bufferPtr;
		
		/* Traverse left child: */
		if(left<mid)
			drawBackToFront(left,mid-1,childSplitDimension,eyePos,bufferPtr);
		}
	}

void EarthquakeSet::createShader(EarthquakeSet::DataItem* dataItem) const
	{
	/* Create the point rendering shader: */
	static const char* vertexProgram="\
		uniform float scaledPointRadius; \
		uniform float highlightTime; \
		uniform float currentTime; \
		uniform vec4 frontSphereCenter; \
		uniform float frontSphereRadius2; \
		uniform bool frontSphereTest; \
		 \
		void main() \
			{ \
			/* Check if the point is inside the front sphere: */ \
			bool valid=dot(gl_Vertex-frontSphereCenter,gl_Vertex-frontSphereCenter)>=frontSphereRadius2; \
			if(frontSphereTest) \
				valid=!valid; \
			if(valid) \
				{ \
				/* Transform the vertex to eye coordinates: */ \
				vec4 vertexEye=gl_ModelViewMatrix*gl_Vertex; \
				 \
				/* Calculate point size based on vertex' eye distance along z direction and event magnitude: */ \
				float pointSize=scaledPointRadius*2.0*vertexEye.w/vertexEye.z; \
				if(gl_MultiTexCoord0.x>5.0) \
					pointSize*=gl_MultiTexCoord0.x-4.0; \
				 \
				/* Adapt point size based on current time and time scale: */ \
				float highlightFactor=gl_MultiTexCoord0.y-(currentTime-highlightTime); \
				if(highlightFactor>0.0&&highlightFactor<=highlightTime) \
					pointSize*=2.0*highlightFactor/highlightTime+1.0; \
				 \
				/* Set point size: */ \
				gl_PointSize=pointSize; \
				 \
				/* Use standard color: */ \
				gl_FrontColor=gl_Color; \
				} \
			else \
				{ \
				/* Set point size to zero and color to invisible: */ \
				gl_PointSize=0.0; \
				gl_FrontColor=vec4(0.0,0.0,0.0,0.0); \
				} \
			 \
			/* Use standard vertex position for fragment generation: */ \
			gl_Position=ftransform(); \
			}";
	static const char* vertexProgramFog="\
		uniform float scaledPointRadius; \
		uniform float highlightTime; \
		uniform float currentTime; \
		uniform vec4 frontSphereCenter; \
		uniform float frontSphereRadius2; \
		uniform bool frontSphereTest; \
		 \
		void main() \
			{ \
			/* Check if the point is inside the front sphere: */ \
			bool valid=dot(gl_Vertex-frontSphereCenter,gl_Vertex-frontSphereCenter)>=frontSphereRadius2; \
			if(frontSphereTest) \
				valid=!valid; \
			if(valid) \
				{ \
				/* Transform the vertex to eye coordinates: */ \
				vec4 vertexEye=gl_ModelViewMatrix*gl_Vertex; \
				 \
				/* Calculate point size based on vertex' eye distance along z direction and event magnitude: */ \
				float pointSize=scaledPointRadius*2.0*vertexEye.w/vertexEye.z; \
				if(gl_MultiTexCoord0.x>5.0) \
					pointSize*=gl_MultiTexCoord0.x-4.0; \
				 \
				/* Adapt point size based on current time and time scale: */ \
				float highlightFactor=gl_MultiTexCoord0.y-(currentTime-highlightTime); \
				if(highlightFactor>0.0&&highlightFactor<=highlightTime) \
					pointSize*=2.0*highlightFactor/highlightTime+1.0; \
				 \
				/* Set point size: */ \
				gl_PointSize=pointSize; \
				 \
				/* Calculate vertex-eye distance for fog computation: */ \
				float eyeDist=-vertexEye.z/vertexEye.w; \
				 \
				/* Calculate fog attenuation: */ \
				float fogFactor=clamp((eyeDist-gl_Fog.start)/(gl_Fog.end-gl_Fog.start),0.0,1.0); \
				 \
				/* Use standard color attenuated by fog: */ \
				gl_FrontColor=mix(gl_Color,gl_Fog.color,fogFactor); \
				} \
			else \
				{ \
				/* Set point size to zero and color to invisible: */ \
				gl_PointSize=0.0; \
				gl_FrontColor=vec4(0.0,0.0,0.0,0.0); \
				} \
			 \
			/* Use standard vertex position for fragment generation: */ \
			gl_Position=ftransform(); \
			}";
	static const char* fragmentProgram="\
		uniform sampler2D pointTexture; \
		 \
		void main() \
			{ \
			gl_FragColor=texture2D(pointTexture,gl_TexCoord[0].xy)*gl_Color; \
			}";
	
	/* Compile the vertex and fragment programs: */
	dataItem->fog=glIsEnabled(GL_FOG);
	if(dataItem->fog)
		dataItem->pointRenderer->compileVertexShaderFromString(vertexProgramFog);
	else
		dataItem->pointRenderer->compileVertexShaderFromString(vertexProgram);
	dataItem->pointRenderer->compileFragmentShaderFromString(fragmentProgram);
	
	/* Link the shader: */
	dataItem->pointRenderer->linkShader();
	
	/* Get the locations of all uniform variables: */
	dataItem->scaledPointRadiusLocation=dataItem->pointRenderer->getUniformLocation("scaledPointRadius");
	dataItem->highlightTimeLocation=dataItem->pointRenderer->getUniformLocation("highlightTime");
	dataItem->scaledPointRadiusLocation=dataItem->pointRenderer->getUniformLocation("scaledPointRadius");
	dataItem->currentTimeLocation=dataItem->pointRenderer->getUniformLocation("currentTime");
	dataItem->frontSphereCenterLocation=dataItem->pointRenderer->getUniformLocation("frontSphereCenter");
	dataItem->frontSphereRadius2Location=dataItem->pointRenderer->getUniformLocation("frontSphereRadius2");
	dataItem->frontSphereTestLocation=dataItem->pointRenderer->getUniformLocation("frontSphereTest");
	dataItem->pointTextureLocation=dataItem->pointRenderer->getUniformLocation("pointTexture");
	}

EarthquakeSet::EarthquakeSet(const char* earthquakeFileName,IO::FilePtr earthquakeFile,double scaleFactor)
	:treePointIndices(0),
	 pointRadius(1.0f),highlightTime(1.0),currentTime(0.0)
	{
	/* Check the earthquake file name's extension: */
	if(Misc::hasCaseExtension(earthquakeFileName,".anss"))
		{
		/* Read an earthquake database snapshot in "readable" ANSS format: */
		loadANSSFile(earthquakeFile,scaleFactor);
		}
	else
		{
		/* Read an earthquake event file in space- or comma-separated format: */
		loadCSVFile(earthquakeFileName,earthquakeFile,scaleFactor);
		}
	
	/* Create a temporary kd-tree to sort the events for back-to-front traversal: */
	Geometry::ArrayKdTree<Geometry::ValuedPoint<Point,int> > sortTree(events.size());
	Geometry::ValuedPoint<Point,int>* stPtr=sortTree.accessPoints();
	int i=0;
	for(std::vector<Event>::const_iterator eIt=events.begin();eIt!=events.end();++eIt,++stPtr,++i)
		{
		*stPtr=eIt->position;
		stPtr->value=i;
		}
	sortTree.releasePoints(8);
	
	/* Retrieve the sorted event indices: */
	treePointIndices=new int[events.size()];
	stPtr=sortTree.accessPoints();
	for(int i=0;i<sortTree.getNumNodes();++i,++stPtr)
		treePointIndices[i]=stPtr->value;
	}

EarthquakeSet::~EarthquakeSet(void)
	{
	delete[] treePointIndices;
	}

void EarthquakeSet::initContext(GLContextData& contextData) const
	{
	/* Create a context data item and store it in the context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	if(dataItem->vertexBufferObjectId>0)
		{
		typedef GLGeometry::Vertex<float,2,GLubyte,4,void,float,3> Vertex;
		
		/* Create a vertex buffer object to store the events: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferObjectId);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,events.size()*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
		Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
		int numPoints=int(events.size());
		for(int i=0;i<numPoints;++i,++vPtr)
			{
			/* Get a reference to the event in kd-tree order: */
			const Event& e=events[treePointIndices[i]];
			
			/* Copy the event's magnitude and time: */
			vPtr->texCoord[0]=Vertex::TexCoord::Scalar(e.magnitude);
			vPtr->texCoord[1]=Vertex::TexCoord::Scalar(e.time);
			
			/* Map the event's magnitude to color: */
			float magnitudeMin=5.0f;
			float magnitudeMax=9.0f;
			const int numColors=5;
			#if 0
			static const Vertex::Color magColorMap[numColors]=
				{
				/* Reduced-saturation color map for anaglyphic viewing: */
				Vertex::Color(96,160,96),
				Vertex::Color(96,160,160),
				Vertex::Color(96,96,160),
				Vertex::Color(160,96,160),
				Vertex::Color(160,96,96)
				};
			#else
			static const Vertex::Color magColorMap[numColors]=
				{
				Vertex::Color(0,255,0),
				Vertex::Color(0,255,255),
				Vertex::Color(0,0,255),
				Vertex::Color(255,0,255),
				Vertex::Color(255,0,0)
				};
			#endif
			if(e.magnitude<=magnitudeMin)
				vPtr->color=magColorMap[0];
			else if(e.magnitude>=magnitudeMax)
				vPtr->color=magColorMap[numColors-1];
			else
				{
				int baseIndex=int(e.magnitude-magnitudeMin);
				float weight=(e.magnitude-magnitudeMin)-float(baseIndex);
				for(int i=0;i<4;++i)
					vPtr->color[i]=GLubyte(float(magColorMap[baseIndex][i])*(1.0f-weight)+float(magColorMap[baseIndex+1][i]*weight)+0.5f);
				}
			
			/* Copy the event's position: */
			vPtr->position=e.position;
			}
		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
		
		/* Protect the vertex buffer object: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		}
	
	if(dataItem->pointRenderer!=0)
		{
		/* Create the point rendering shader: */
		createShader(dataItem);
		
		/* Create the point rendering texture: */
		GLfloat texImage[32][32][4];
		for(int y=0;y<32;++y)
			for(int x=0;x<32;++x)
				{
				texImage[y][x][0]=texImage[y][x][1]=texImage[y][x][2]=1.0f;
				float r2=Math::sqr((float(x)-15.5f)/15.5f)+Math::sqr((float(y)-15.5f)/15.5f);
				if(r2<1.0f)
					{
					// float l=Math::exp(-r2*2.0f)-Math::exp(-2.0f);
					float l=1.0f-r2;
					texImage[y][x][3]=l;
					}
				else
					texImage[y][x][3]=0.0f;
				}
		
		/* Upload particle texture: */
		glBindTexture(GL_TEXTURE_2D,dataItem->pointTextureObjectId);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,32,32,0,GL_RGBA,GL_FLOAT,&texImage[0][0][0]);
		glBindTexture(GL_TEXTURE_2D,0);
		
		/* Create an index buffer to render points in depth order: */
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,dataItem->sortedPointIndicesBufferObjectId);
		glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,events.size()*sizeof(GLuint),0,GL_STREAM_DRAW_ARB);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
		}
	}

std::pair<double,double> EarthquakeSet::getTimeRange(void) const
	{
	std::pair<double,double> result;
	
	std::vector<Event>::const_iterator eIt=events.begin();
	result.first=result.second=eIt->time;
	for(++eIt;eIt!=events.end();++eIt)
		{
		if(result.first>eIt->time)
			result.first=eIt->time;
		else if(result.second<eIt->time)
			result.second=eIt->time;
		}
	
	return result;
	}

void EarthquakeSet::setPointRadius(float newPointRadius)
	{
	pointRadius=newPointRadius;
	}

void EarthquakeSet::setHighlightTime(double newHighlightTime)
	{
	highlightTime=newHighlightTime;
	}

void EarthquakeSet::setCurrentTime(double newCurrentTime)
	{
	currentTime=newCurrentTime;
	}

void EarthquakeSet::glRenderAction(GLContextData& contextData) const
	{
	/* Get a pointer to the data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Save OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT);
	
	if(dataItem->pointRenderer!=0)
		{
		/* Enable point sprites: */
		glEnable(GL_POINT_SPRITE_ARB);
		glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
		
		/* Bind the point rendering texture: */
		glEnable(GL_TEXTURE_2D);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_2D,dataItem->pointTextureObjectId);
		glTexEnvi(GL_POINT_SPRITE_ARB,GL_COORD_REPLACE_ARB,GL_TRUE);
		
		/* Check if the point renderer program conforms to current OpenGL state: */
		if(glIsEnabled(GL_FOG)!=dataItem->fog)
			{
			/* Rebuild the point rendering shader: */
			createShader(dataItem);
			}
		
		/* Enable the point renderer program: */
		dataItem->pointRenderer->useProgram();
		
		/* Set the uniform variables: */
		glUniform1fARB(dataItem->scaledPointRadiusLocation,pointRadius);
		glUniform1fARB(dataItem->highlightTimeLocation,float(highlightTime));
		glUniform1fARB(dataItem->currentTimeLocation,float(currentTime));
		glUniform1iARB(dataItem->pointTextureLocation,0);
		}
	else
		{
		/* Set up standard point rendering: */
		glDisable(GL_LIGHTING);
		}
	
	if(dataItem->vertexBufferObjectId>0)
		{
		typedef GLGeometry::Vertex<float,2,GLubyte,4,void,float,3> Vertex;
		
		/* Bind the point set's vertex buffer object: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferObjectId);
		GLVertexArrayParts::enable(Vertex::getPartsMask());
		glVertexPointer(static_cast<Vertex*>(0));
		
		/* Render the vertex array: */
		glDrawArrays(GL_POINTS,0,events.size());
		
		/* Protect the vertex buffer object: */
		GLVertexArrayParts::disable(Vertex::getPartsMask());
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		}
	else
		{
		/* Render the earthquake set as a regular vertex array of points: */
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3,GL_FLOAT,sizeof(Event),&events[0].position);
		glDrawArrays(GL_POINTS,0,events.size());
		glDisableClientState(GL_VERTEX_ARRAY);
		}
	
	if(dataItem->pointRenderer!=0)
		{
		/* Unbind the point rendering texture: */
		glBindTexture(GL_TEXTURE_2D,0);
		
		/* Disable the point rendering shader: */
		GLShader::disablePrograms();
		}
	
	/* Reset OpenGL state: */
	glPopAttrib();
	}

void EarthquakeSet::glRenderAction(const EarthquakeSet::Point& eyePos,bool front,GLContextData& contextData) const
	{
	/* Get a pointer to the data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Save OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT);
	
	if(dataItem->pointRenderer!=0)
		{
		/* Enable point sprites: */
		glEnable(GL_POINT_SPRITE_ARB);
		glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
		
		/* Bind the point rendering texture: */
		glEnable(GL_TEXTURE_2D);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_2D,dataItem->pointTextureObjectId);
		glTexEnvi(GL_POINT_SPRITE_ARB,GL_COORD_REPLACE_ARB,GL_TRUE);
		
		/* Enable the point renderer program: */
		dataItem->pointRenderer->useProgram();
		
		/* Calculate the front sphere: */
		Geometry::HVector<Point::Scalar,3> frontSphereCenter=Geometry::mid(eyePos,Point::origin);
		float frontSphereRadius2=Geometry::sqrDist(eyePos,Point::origin)*0.25f;
		
		/* Set the uniform variables: */
		glUniform1fARB(dataItem->scaledPointRadiusLocation,pointRadius);
		glUniform1fARB(dataItem->highlightTimeLocation,float(highlightTime));
		glUniform1fARB(dataItem->currentTimeLocation,float(currentTime));
		glUniform4fvARB(dataItem->frontSphereCenterLocation,1,frontSphereCenter.getComponents());
		glUniform1fARB(dataItem->frontSphereRadius2Location,frontSphereRadius2);
		glUniform1iARB(dataItem->frontSphereTestLocation,front);
		glUniform1iARB(dataItem->pointTextureLocation,0);
		}
	else
		{
		/* Set up standard point rendering: */
		glDisable(GL_LIGHTING);
		}
	
	if(dataItem->vertexBufferObjectId>0)
		{
		typedef GLGeometry::Vertex<float,2,GLubyte,4,void,float,3> Vertex;
		
		/* Bind the point set's vertex buffer object: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferObjectId);
		GLVertexArrayParts::enable(Vertex::getPartsMask());
		glVertexPointer(static_cast<Vertex*>(0));
		
		if(dataItem->sortedPointIndicesBufferObjectId>0)
			{
			/* Bind the point indices buffer: */
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,dataItem->sortedPointIndicesBufferObjectId);
			
			/* Check if the eye position changed since the last rendering pass: */
			if(dataItem->eyePos!=eyePos)
				{
				/* Re-sort the points according to the new eye position: */
				GLuint* bufferPtr=static_cast<GLuint*>(glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
				drawBackToFront(0,int(events.size())-1,0,eyePos,bufferPtr);
				glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
				
				dataItem->eyePos=eyePos;
				}
			
			/* Render the vertex array in back-to-front order: */
			glDrawElements(GL_POINTS,events.size(),GL_UNSIGNED_INT,0);
			
			/* Protect the point indices buffer: */
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
			}
		else
			{
			/* Render the points in arbitrary order: */
			glDrawArrays(GL_POINTS,0,events.size());
			}
		
		/* Protect the vertex buffer object: */
		GLVertexArrayParts::disable(Vertex::getPartsMask());
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
		}
	else
		{
		/* Render the earthquake set as a regular vertex array of points: */
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3,GL_FLOAT,sizeof(Event),&events[0].position);
		glDrawArrays(GL_POINTS,0,events.size());
		glDisableClientState(GL_VERTEX_ARRAY);
		}
	
	if(dataItem->pointRenderer!=0)
		{
		/* Unbind the point rendering texture: */
		glBindTexture(GL_TEXTURE_2D,0);
		
		/* Disable the point rendering shader: */
		GLShader::disablePrograms();
		}
	
	/* Reset OpenGL state: */
	glPopAttrib();
	}

const EarthquakeSet::Event* EarthquakeSet::selectEvent(const EarthquakeSet::Point& pos,float maxDist) const
	{
	const Event* result=0;
	
	float minDist2=Math::sqr(maxDist);
	for(std::vector<Event>::const_iterator eIt=events.begin();eIt!=events.end();++eIt)
		{
		float dist2=Geometry::sqrDist(pos,eIt->position);
		if(minDist2>dist2)
			{
			result=&(*eIt);
			minDist2=dist2;
			}
		}
	
	return result;
	}

const EarthquakeSet::Event* EarthquakeSet::selectEvent(const EarthquakeSet::Ray& ray,float coneAngleCos) const
	{
	const Event* result=0;
	
	float coneAngleCos2=Math::sqr(coneAngleCos);
	float lambdaMin=Math::Constants<float>::max;
	for(std::vector<Event>::const_iterator eIt=events.begin();eIt!=events.end();++eIt)
		{
		Ray::Vector sp=eIt->position-ray.getOrigin();
		float x=sp*ray.getDirection();
		if(x>=0.0f&&x<lambdaMin)
			{
			if(Math::sqr(x)>coneAngleCos2*Geometry::sqr(sp))
				{
				result=&(*eIt);
				lambdaMin=x;
				}
			}
		}
	
	return result;
	}
