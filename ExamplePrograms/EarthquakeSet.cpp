/***********************************************************************
EarthquakeSet - Class to represent and render sets of earthquakes with
3D locations, magnitude and event time.
Copyright (c) 2006-2007 Oliver Kreylos

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

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <algorithm>
#include <Misc/ThrowStdErr.h>
#include <Misc/File.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>

#include "EarthFunctions.h"

#include "EarthquakeSet.h"

namespace {

/***********************************************************************
Helper classes and functions to parse spreadsheet files in text format:
***********************************************************************/

int getNextValue(Misc::File& file,int nextChar,char* valueBuffer,size_t valueBufferSize)
	{
	/* Prepare writing to the value buffer: */
	char* vPtr=valueBuffer;
	
	if(nextChar=='"')
		{
		/* Read characters until next quotation mark: */
		while(true)
			{
			/* Read the next character and check for quote: */
			nextChar=file.getc();
			if(nextChar=='"')
				break;
			
			/* Store next character if there is room: */
			if(valueBufferSize>1)
				{
				*vPtr=char(nextChar);
				++vPtr;
				--valueBufferSize;
				}
			}
		
		/* Read the character after the closing quote: */
		nextChar=file.getc();
		}
	else
		{
		while(nextChar!=EOF&&nextChar!=','&&!isspace(nextChar))
			{
			/* Store next character if there is room: */
			if(valueBufferSize>1)
				{
				*vPtr=char(nextChar);
				++vPtr;
				--valueBufferSize;
				}
			
			/* Read the next character: */
			nextChar=file.getc();
			}
		}
	
	/* Terminate the value buffer: */
	*vPtr='\0';
	
	/* Skip whitespace: */
	while(nextChar!=EOF&&nextChar!='\n'&&isspace(nextChar))
		nextChar=file.getc();
	
	/* Check for a separating comma: */
	if(nextChar==',')
		{
		/* Skip the comma: */
		nextChar=file.getc();
		
		/* Skip whitespace: */
		while(nextChar!=EOF&&nextChar!='\n'&&isspace(nextChar))
			nextChar=file.getc();
		}
	
	return nextChar;
	}

}

/****************************************
Methods of class EarthquakeSet::DataItem:
****************************************/

EarthquakeSet::DataItem::DataItem(void)
	:vertexBufferObjectId(0)
	{
	/* Check if the vertex buffer object extension is supported: */
	if(GLARBVertexBufferObject::isSupported())
		{
		/* Initialize the vertex buffer object extension: */
		GLARBVertexBufferObject::initExtension();
		
		/* Create a vertex buffer object: */
		glGenBuffersARB(1,&vertexBufferObjectId);
		}
	}

EarthquakeSet::DataItem::~DataItem(void)
	{
	/* Check if the vertex buffer object extension is supported: */
	if(vertexBufferObjectId>0)
		{
		/* Destroy the vertex buffer object: */
		glDeleteBuffersARB(1,&vertexBufferObjectId);
		}
	}

/******************************
Methods of class EarthquakeSet:
******************************/

EarthquakeSet::EarthquakeSet(const char* earthquakeFileName,double scaleFactor)
	:selectedBegin(0),selectedEnd(0)
	{
	char valueBuffer[256];
	
	/* Open the earthquake file: */
	Misc::File earthquakeFile(earthquakeFileName,"rt");
	
	/* Skip initial whitespace: */
	int nextChar;
	do
		{
		nextChar=earthquakeFile.getc();
		}
	while(nextChar!=EOF&&isspace(nextChar));
	if(nextChar==EOF)
		Misc::throwStdErr("EarthquakeSet::EarthquakeSet: Early end of file in input file \"%s\"",earthquakeFileName);
	
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
		} radiusMode;
	int dateIndex=-1;
	int timeIndex=-1;
	int magIndex=-1;
	
	/* Read the header line's columns: */
	int index=0;
	while(nextChar!=EOF&&nextChar!='\n')
		{
		/* Read the next column header: */
		nextChar=getNextValue(earthquakeFile,nextChar,valueBuffer,sizeof(valueBuffer));
		
		/* Parse the column header: */
		if(strcasecmp(valueBuffer,"Latitude")==0||strcasecmp(valueBuffer,"Lat")==0)
			latIndex=index;
		else if(strcasecmp(valueBuffer,"Longitude")==0||strcasecmp(valueBuffer,"Long")==0||strcasecmp(valueBuffer,"Lon")==0)
			lngIndex=index;
		else if(strcasecmp(valueBuffer,"Radius")==0)
			{
			radiusIndex=index;
			radiusMode=RADIUS;
			}
		else if(strcasecmp(valueBuffer,"Depth")==0)
			{
			radiusIndex=index;
			radiusMode=DEPTH;
			}
		else if(strcasecmp(valueBuffer,"Negative Depth")==0||strcasecmp(valueBuffer,"Neg Depth")==0||strcasecmp(valueBuffer,"NegDepth")==0)
			{
			radiusIndex=index;
			radiusMode=NEGDEPTH;
			}
		else if(strcasecmp(valueBuffer,"Date")==0)
			dateIndex=index;
		else if(strcasecmp(valueBuffer,"Time")==0)
			timeIndex=index;
		else if(strcasecmp(valueBuffer,"Magnitude")==0||strcasecmp(valueBuffer,"Mag")==0)
			magIndex=index;
		++index;
		}
	
	/* Check if all required portions have been detected: */
	if(latIndex<0||lngIndex<0||radiusIndex<0||dateIndex<0||timeIndex<0||magIndex<0)
		Misc::throwStdErr("EarthquakeSet::EarthquakeSet: Missing earthquake components in input file \"%s\"",earthquakeFileName);
	
	/* Read all events from the earthquake file: */
	while(nextChar!=EOF)
		{
		/* Skip initial whitespace: */
		do
			{
			nextChar=earthquakeFile.getc();
			}
		while(nextChar!=EOF&&isspace(nextChar));
		
		/* Read the next line from the input file: */
		float sphericalCoordinates[3];
		struct tm timeStruct;
		float magnitude;
		int index=0;
		int parsedComponentsMask=0x0;
		while(nextChar!=EOF&&nextChar!='\n')
			{
			/* Read the next attribute: */
			nextChar=getNextValue(earthquakeFile,nextChar,valueBuffer,sizeof(valueBuffer));
			
			/* Process the attribute: */
			if(valueBuffer[0]=='\0')
				{
				/* Just ignore empty fields... */
				}
			else if(index==latIndex)
				{
				sphericalCoordinates[0]=Math::rad(float(atof(valueBuffer)));
				parsedComponentsMask|=0x1;
				}
			else if(index==lngIndex)
				{
				sphericalCoordinates[1]=Math::rad(float(atof(valueBuffer)));
				parsedComponentsMask|=0x2;
				}
			else if(index==radiusIndex)
				{
				sphericalCoordinates[2]=float(atof(valueBuffer));
				parsedComponentsMask|=0x4;
				}
			else if(index==dateIndex)
				{
				char* yearPtr=valueBuffer;
				char* monthPtr;
				for(monthPtr=yearPtr;*monthPtr!='\0'&&*monthPtr!='/';++monthPtr)
					;
				if(*monthPtr=='/')
					{
					*monthPtr='\0';
					++monthPtr;
					char* dayPtr;
					for(dayPtr=monthPtr;*dayPtr!='\0'&&*dayPtr!='/';++dayPtr)
						;
					if(*dayPtr=='/')
						{
						*dayPtr='\0';
						++dayPtr;
						
						char* endPtr;
						for(endPtr=dayPtr;*endPtr!='\0';++endPtr)
							;
						
						if(monthPtr-yearPtr==3&&dayPtr-monthPtr==3&&endPtr-dayPtr==4)
							{
							timeStruct.tm_mday=atoi(monthPtr);
							timeStruct.tm_mon=atoi(yearPtr)-1;
							timeStruct.tm_year=atoi(dayPtr)-1900;
							}
						else
							{
							timeStruct.tm_mday=atoi(dayPtr);
							timeStruct.tm_mon=atoi(monthPtr)-1;
							timeStruct.tm_year=atoi(yearPtr)-1900;
							}
						parsedComponentsMask|=0x8;
						}
					}
				}
			else if(index==timeIndex)
				{
				char* hourPtr=valueBuffer;
				char* minutePtr;
				for(minutePtr=hourPtr;*minutePtr!='\0'&&*minutePtr!=':';++minutePtr)
					;
				if(*minutePtr==':')
					{
					*minutePtr='\0';
					++minutePtr;
					char* secondPtr;
					for(secondPtr=minutePtr;*secondPtr!='\0'&&*secondPtr!=':';++secondPtr)
						;
					if(*secondPtr==':')
						{
						*secondPtr='\0';
						++secondPtr;
						char* fractionPtr;
						for(fractionPtr=secondPtr;*fractionPtr!='\0'&&*fractionPtr!='.';++fractionPtr)
							;
						*fractionPtr='\0';
						
						timeStruct.tm_sec=atoi(secondPtr);
						timeStruct.tm_min=atoi(minutePtr);
						timeStruct.tm_hour=atoi(hourPtr);
						
						parsedComponentsMask|=0x10;
						}
					}
				}
			else if(index==magIndex)
				{
				magnitude=float(atof(valueBuffer));
				parsedComponentsMask|=0x20;
				}
			
			++index;
			}
		
		/* Check if a complete set of coordinates has been parsed: */
		if(parsedComponentsMask==0x3f)
			{
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
			timeStruct.tm_isdst=-1;
			e.time=double(mktime(&timeStruct));
			
			/* Store the event magnitude: */
			e.magnitude=magnitude;
			
			/* Append the event to the earthquake set: */
			events.push_back(e);
			}
		}
	
	/* Sort the events by time: */
	std::sort(events.begin(),events.end());
	}

void EarthquakeSet::initContext(GLContextData& contextData) const
	{
	/* Create a context data item and store it in the context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Check if the vertex buffer object extension is supported: */
	float magnitudeMin=5.0f;
	float magnitudeMax=9.0f;
	if(dataItem->vertexBufferObjectId>0)
		{
		/* Create a vertex buffer object to store the points' coordinates: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferObjectId);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,events.size()*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
		
		/* Copy all events: */
		Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
		for(std::vector<Event>::const_iterator eIt=events.begin();eIt!=events.end();++eIt,++vPtr)
			{
			/* Map the event's magnitude to color: */
			const int numColors=5;
			static const Vertex::Color magColorMap[numColors]=
				{
				Vertex::Color(0,255,0),
				Vertex::Color(0,255,255),
				Vertex::Color(0,0,255),
				Vertex::Color(255,0,255),
				Vertex::Color(255,0,0)
				};
			if(eIt->magnitude<=magnitudeMin)
				vPtr->color=magColorMap[0];
			else if(eIt->magnitude>=magnitudeMax)
				vPtr->color=magColorMap[numColors-1];
			else
				{
				int baseIndex=int(Math::floor(eIt->magnitude-magnitudeMin));
				float weight=(eIt->magnitude-magnitudeMin)-float(baseIndex);
				for(int i=0;i<4;++i)
					vPtr->color[i]=GLubyte(Math::floor(float(magColorMap[baseIndex][i])*(1.0f-weight)+float(magColorMap[baseIndex+1][i]*weight)+0.5f));
				}
			
			/* Copy the event's position: */
			for(int i=0;i<3;++i)
				vPtr->position[i]=GLfloat(eIt->position[i]);
			}
		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
		
		/* Protect the vertex buffer object: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
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

void EarthquakeSet::selectEvents(double eventTime1,double eventTime2)
	{
	int l,r;
	
	/* Find the first event later than the selection range start: */
	l=0;
	r=events.size();
	while(l<r)
		{
		int m=(l+r)>>1;
		if(events[m].time<eventTime1)
			l=m+1;
		else
			r=m;
		}
	selectedBegin=l;
	
	/* Find the first event later than the selection range end: */
	l=0;
	r=events.size();
	while(l<r)
		{
		int m=(l+r)>>1;
		if(events[m].time<eventTime2)
			l=m+1;
		else
			r=m;
		}
	selectedEnd=l;
	}

void EarthquakeSet::glRenderAction(GLContextData& contextData) const
	{
	/* Get a pointer to the data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Save and set up OpenGL state: */
	GLboolean lightingEnabled=glIsEnabled(GL_LIGHTING);
	if(lightingEnabled)
		glDisable(GL_LIGHTING);
	
	/* Check if the vertex buffer object extension is supported: */
	if(dataItem->vertexBufferObjectId>0)
		{
		/* Bind the point set's vertex buffer object: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->vertexBufferObjectId);
		
		glInterleavedArrays(GL_C4UB_V3F,0,0);
		
		/* Render the point set as a vertex array of points: */
		glDrawArrays(GL_POINTS,0,events.size());
		
		/* Highlight the selected points: */
		if(selectedEnd>selectedBegin)
			{
			GLfloat pointSize;
			glGetFloatv(GL_POINT_SIZE,&pointSize);
			glPointSize(5.0f);
			glDrawArrays(GL_POINTS,selectedBegin,selectedEnd-selectedBegin);
			glPointSize(pointSize);
			}
		
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		
		/* Protect the vertex buffer object: */
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
	
	/* Restore OpenGL state: */
	if(lightingEnabled)
		glEnable(GL_LIGHTING);
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

const EarthquakeSet::Event* EarthquakeSet::selectEvent(const EarthquakeSet::Ray& ray,float coneAngle) const
	{
	const Event* result=0;
	
	float coneAngle2=Math::sqr(coneAngle);
	float lambdaMin=Math::Constants<float>::max;
	for(std::vector<Event>::const_iterator eIt=events.begin();eIt!=events.end();++eIt)
		{
		Ray::Vector sp=eIt->position-ray.getOrigin();
		float x=sp*ray.getDirection();
		if(x>=0.0f&&x<lambdaMin)
			{
			float y2=Geometry::sqr(Geometry::cross(sp,ray.getDirection()));
			if(y2/Math::sqr(x)<=coneAngle2)
				{
				result=&(*eIt);
				lambdaMin=x;
				}
			}
		}
	
	return result;
	}
