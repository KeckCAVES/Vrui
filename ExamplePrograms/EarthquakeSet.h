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

#ifndef EARTHQUAKESET_INCLUDED
#define EARTHQUAKESET_INCLUDED

#include <utility>
#include <vector>
#include <Geometry/Point.h>
#include <GL/gl.h>
#include <GL/GLVertex.h>
#include <GL/GLObject.h>

class EarthquakeSet:public GLObject
	{
	/* Embedded classes: */
	public:
	typedef Geometry::Point<float,3> Point; // Type for points
	
	struct Event // Structure for events (earthquakes)
		{
		/* Elements: */
		public:
		Point position; // 3D earthquake position in Cartesian coordinates
		double time; // Earthquake time in seconds since the epoch (UTC)
		float magnitude; // Earthquake magnitude
		
		/* Methods: */
		friend bool operator<(const Event& e1,const Event& e2)
			{
			return e1.time<e2.time;
			};
		};
	
	private:
	typedef GLVertex<void,0,GLubyte,4,void,GLfloat,3> Vertex; // Type for rendered vertices
	
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint vertexBufferObjectId; // ID of vertex buffer object that contains the earthquake set (0 if extension not supported)
		
		/* Constructors and destructors: */
		public:
		DataItem(void); // Creates a data item
		virtual ~DataItem(void); // Destroys a data item
		};
	
	/* Elements: */
	std::vector<Event> events; // Vector of earthquakes
	int selectedBegin; // Index of first selected earthquake
	int selectedEnd; // Index one behind last selected earthquake
	
	/* Constructors and destructors: */
	public:
	EarthquakeSet(const char* earthquakeFileName,double scaleFactor); // Creates an earthquake set by reading a file; applies scale factor to Cartesian coordinates
	
	/* Methods: */
	virtual void initContext(GLContextData& contextData) const;
	std::pair<double,double> getTimeRange(void) const; // Returns the range of event times
	void selectEvents(double eventTime1,double eventTime2); // Selects a set of earthquake events in the given time range
	void glRenderAction(GLContextData& contextData) const; // Renders the earthquake set
	const Event* selectEvent(const Point& pos,float maxDist) const; // Returns the event closest to the given query point (or null pointer)
	};

#endif
