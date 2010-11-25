/***********************************************************************
EarthquakeSet - Class to represent and render sets of earthquakes with
3D locations, magnitude and event time.
Copyright (c) 2006-2010 Oliver Kreylos

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
#include <Geometry/Ray.h>
#include <GL/gl.h>
#include <GL/GLObject.h>

/* Forward declarations: */
class GLShader;

class EarthquakeSet:public GLObject
	{
	/* Embedded classes: */
	public:
	typedef Geometry::Point<float,3> Point; // Type for points
	typedef Geometry::Ray<float,3> Ray; // Type for rays
	
	struct Event // Structure for events (earthquakes)
		{
		/* Elements: */
		public:
		Point position; // 3D earthquake position in Cartesian coordinates
		double time; // Earthquake time in seconds since the epoch (UTC)
		float magnitude; // Earthquake magnitude
		};
	
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint vertexBufferObjectId; // ID of vertex buffer object that contains the earthquake set (0 if extension not supported)
		GLShader* pointRenderer; // Pointer to GLSL shader to render properly scaled, texture-mapped points (0 if extension not supported)
		bool fog; // Flag whether fog blending is enabled in the current shader object
		GLint scaledPointRadiusLocation; // Location of point radius uniform variable in shader program
		GLint highlightTimeLocation; // Location of highlight time uniform variable in shader program
		GLint currentTimeLocation; // Location of current time uniform variable in shader programs
		GLint frontSphereCenterLocation;
		GLint frontSphereRadius2Location;
		GLint frontSphereTestLocation;
		GLint pointTextureLocation; // Location of texture sample uniform variable in shader program
		GLuint pointTextureObjectId; // ID of the point texture object
		Point eyePos; // The eye position for which the points have been sorted in depth order
		GLuint sortedPointIndicesBufferObjectId; // ID of index buffer containing the indices of points, sorted in depth order from the current eye position
		
		/* Constructors and destructors: */
		public:
		DataItem(void); // Creates a data item
		virtual ~DataItem(void); // Destroys a data item
		};
	
	/* Elements: */
	std::vector<Event> events; // Vector of earthquakes
	int* treePointIndices; // Array of event indices in kd-tree order
	float pointRadius; // Point radius in model space
	double highlightTime; // Time span (in real time) for which earthquake events are highlighted during animation
	double currentTime; // Current event time during animation
	
	/* Private methods: */
	void loadANSSFile(const char* earthquakeFileName,double scaleFactor); // Loads an earthquake event file in ANSS readable database snapshot format
	void loadCSVFile(const char* earthquakeFileName,double scaleFactor); // Loads an earthquake event file in space- or comma-separated format
	void drawBackToFront(int left,int right,int splitDimension,const Point& eyePos,GLuint*& bufferPtr) const; // Renders the given kd-tree subtree in back-to-front order
	void createShader(DataItem* dataItem) const; // Creates the particle rendering shader based on current OpenGL settings
	
	/* Constructors and destructors: */
	public:
	EarthquakeSet(const char* earthquakeFileName,double scaleFactor); // Creates an earthquake set by reading a file; applies scale factor to Cartesian coordinates
	~EarthquakeSet(void);
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New methods: */
	std::pair<double,double> getTimeRange(void) const; // Returns the range of event times
	void setPointRadius(float newPointRadius); // Sets the point radius in model space
	void setHighlightTime(double newHighlightTime); // Sets the time span for which events are highlighted during animation
	void setCurrentTime(double newCurrentTime); // Sets the current event time during animation
	void glRenderAction(GLContextData& contextData) const; // Renders the earthquake set
	void glRenderAction(const Point& eyePos,bool front,GLContextData& contextData) const; // Renders the earthquake set in blending order from the given eye point
	const Event* selectEvent(const Point& pos,float maxDist) const; // Returns the event closest to the given query point (or null pointer)
	const Event* selectEvent(const Ray& ray,float coneAngleCos) const; // Ditto, for query ray
	};

#endif
