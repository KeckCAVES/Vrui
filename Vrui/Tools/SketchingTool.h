/***********************************************************************
SketchingTool - Tool to create and edit 3D curves.
Copyright (c) 2009-2017 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef VRUI_SKETCHINGTOOL_INCLUDED
#define VRUI_SKETCHINGTOOL_INCLUDED

#include <string>
#include <vector>
#include <Geometry/Point.h>
#include <Geometry/Box.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GLMotif/RadioBox.h>
#include <GLMotif/NewButton.h>
#include <GLMotif/TextFieldSlider.h>
#include <GLMotif/FileSelectionDialog.h>
#include <Vrui/Geometry.h>
#include <Vrui/UtilityTool.h>

/* Forward declarations: */
namespace IO {
class ValueSource;
class OStream;
}
namespace GLMotif {
class PopupWindow;
class RowColumn;
class FileSelectionHelper;
}

namespace Vrui {

class SketchingTool;

class SketchingToolFactory:public ToolFactory
	{
	friend class SketchingTool;
	
	/* Elements: */
	private:
	Scalar detailSize; // Minimal length of line segments in curves in physical coordinate units
	Vector brushAxis; // Direction of brush axis in input device local coordinates
	std::string curvesFileName; // Default name for curve files
	GLMotif::FileSelectionHelper* curvesSelectionHelper; // Helper object to load and save curve files
	
	/* Constructors and destructors: */
	public:
	SketchingToolFactory(ToolManager& toolManager);
	virtual ~SketchingToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	
	/* New methods: */
	GLMotif::FileSelectionHelper* getCurvesSelectionHelper(void); // Returns pointer to a file selection helper for curve files
	};

class SketchingTool:public UtilityTool
	{
	friend class SketchingToolFactory;
	
	/* Embedded classes: */
	private:
	typedef GLColor<GLubyte,4> Color; // Type for colors
	typedef Geometry::Box<Scalar,3> Box; // Type for bounding boxes
	
	struct SketchObject // Base class for sketching objects
		{
		/* Elements: */
		public:
		GLfloat lineWidth; // Curve's cosmetic line width
		Color color; // Curve's color
		Box boundingBox; // Bounding box around the curve for selection purposes
		
		/* Constructors and destructors: */
		SketchObject(GLfloat sLineWidth,const Color& sColor)
			:lineWidth(sLineWidth),color(sColor),
			 boundingBox(Box::empty)
			{
			}
		virtual ~SketchObject(void);
		
		/* Methods: */
		virtual bool pick(const Point& p,Scalar radius2) const =0; // Returns true if the given point is closer to the sketching object than the given squared radius
		virtual void write(IO::OStream& os) const; // Writes object state to file
		virtual void read(IO::ValueSource& vs); // Reads object state from file
		virtual void glRenderAction(GLContextData& contextData) const =0; // Method to render the sketching object into the current OpenGL context
		};
	
	struct Curve:public SketchObject // Structure to represent single-stroke curves
		{
		/* Embedded classes: */
		public:
		struct ControlPoint // Structure for curve control points
			{
			/* Elements: */
			public:
			Point pos; // Control point position
			Scalar t; // Control point sample time
			};
		
		/* Elements: */
		std::vector<ControlPoint> controlPoints; // The curve's control points
		
		/* Constructors and destructors: */
		Curve(GLfloat sLineWidth,const Color& sColor)
			:SketchObject(sLineWidth,sColor)
			{
			}
		
		/* Methods from SketchObject: */
		virtual bool pick(const Point& p,Scalar radius2) const;
		virtual void write(IO::OStream& os) const;
		virtual void read(IO::ValueSource& vs);
		virtual void glRenderAction(GLContextData& contextData) const;
		
		/* New methods: */
		static void setGLState(GLContextData& contextData); // Sets up OpenGL for curve rendering
		static void resetGLState(GLContextData& contextData); // Undoes changes to OpenGL
		};
	
	struct Polyline:public SketchObject // Structure to represent polylines
		{
		/* Elements: */
		public:
		std::vector<Point> vertices; // The polyline's vertices
		
		/* Constructors and destructors: */
		Polyline(GLfloat sLineWidth,const Color& sColor)
			:SketchObject(sLineWidth,sColor)
			{
			}
		
		/* Methods from SketchObject: */
		virtual bool pick(const Point& p,Scalar radius2) const;
		virtual void write(IO::OStream& os) const;
		virtual void read(IO::ValueSource& vs);
		virtual void glRenderAction(GLContextData& contextData) const;
		
		/* New methods: */
		static void setGLState(GLContextData& contextData); // Sets up OpenGL for polyline rendering
		static void resetGLState(GLContextData& contextData); // Undoes changes to OpenGL
		};
	
	struct BrushStroke:public SketchObject // Structure to represent broad brush strokes
		{
		/* Embedded classes: */
		public:
		struct ControlPoint // Structure for brush stroke control points
			{
			/* Elements: */
			public:
			Point pos; // Control point position
			Vector brushAxis; // Scaled control point brush axis vector
			Vector normal; // Control point normal vector
			};
		
		/* Elements: */
		std::vector<ControlPoint> controlPoints; // The brush stroke's control points
		
		/* Constructors and destructors: */
		BrushStroke(GLfloat sLineWidth,const Color& sColor)
			:SketchObject(sLineWidth,sColor)
			{
			}
		
		/* Methods from SketchObject: */
		virtual bool pick(const Point& p,Scalar radius2) const;
		virtual void write(IO::OStream& os) const;
		virtual void read(IO::ValueSource& vs);
		virtual void glRenderAction(GLContextData& contextData) const;
		
		/* New methods: */
		static void setGLState(GLContextData& contextData); // Sets up OpenGL for brush stroke rendering
		static void resetGLState(GLContextData& contextData); // Undoes changes to OpenGL
		};
	
	public:
	enum SketchMode // Enumerated type for sketching modes
		{
		CURVE=0,POLYLINE,BRUSHSTROKE,ERASER
		};
	
	/* Elements: */
	private:
	static SketchingToolFactory* factory; // Pointer to the factory object for this class
	static const Color colors[8]; // Standard line color palette
	GLMotif::PopupWindow* controlDialogPopup;
	GLMotif::RadioBox* sketchObjectType;
	GLMotif::TextFieldSlider* lineWidthSlider;
	GLMotif::RowColumn* colorBox;
	std::vector<Curve*> curves; // List of curves
	std::vector<Polyline*> polylines; // List of polylines
	std::vector<BrushStroke*> brushStrokes; // List of brush strokes
	SketchMode sketchMode; // Current sketching mode
	GLfloat newLineWidth; // Line width for new sketch objects
	Color newColor; // Color for new sketch objects
	bool active; // Flag whether the tool is currently creating a sketching object
	Curve* currentCurve; // Pointer to the currently created curve
	Polyline* currentPolyline; // Pointer to the currently created polyline
	BrushStroke* currentBrushStroke; // Pointer to the currently created brush stroke
	Point lastPoint; // The last point appended to the current sketching object
	Point currentPoint; // The current dragging position
	
	/* Constructors and destructors: */
	public:
	SketchingTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment);
	virtual ~SketchingTool(void);
	
	/* Methods from Tool: */
	virtual void configure(const Misc::ConfigurationFileSection& configFileSection);
	virtual void storeState(Misc::ConfigurationFileSection& configFileSection) const;
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	
	/* New methods: */
	void sketchModeCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData);
	void lineWidthSliderCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	void colorButtonSelectCallback(GLMotif::NewButton::SelectCallbackData* cbData);
	void saveCurvesCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData);
	void loadCurvesCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData);
	void deleteAllCurvesCallback(Misc::CallbackData* cbData);
	};

}

#endif
