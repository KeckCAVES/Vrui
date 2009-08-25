/***********************************************************************
Internal declaration for the Vrui virtual reality development toolkit.
Copyright (c) 2000-2008 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_INCLUDED
#define VRUI_INTERNAL_INCLUDED

#include <vector>
#include <deque>
#include <Misc/Timer.h>
#include <Threads/Mutex.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/Plane.h>
#include <GL/gl.h>
#include <GL/GLMaterial.h>
#include <GL/GLObject.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/FileSelectionDialog.h>
#include <Vrui/Vrui.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/DisplayState.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
class CallbackData;
class TimerEventScheduler;
}
namespace Comm {
class MulticastPipeMultiplexer;
class MulticastPipe;
}
namespace GLMotif {
class Container;
class Popup;
class PopupMenu;
}
namespace Vrui {
class InputDeviceDataSaver;
class MultipipeDispatcher;
class VisletManager;
}

namespace Vrui {

/********************
Global program state:
********************/

struct VruiState
	{
	/* Embedded classes: */
	public:
	struct ScreenProtector // Structure describing an input device that needs to be protected from bumping into a screen
		{
		/* Elements: */
		public:
		InputDevice* inputDevice; // Pointer to input device
		Point center; // Center of protective sphere in input device's coordinates
		Scalar radius; // Radius of protective sphere around input device's position
		};
	
	class DisplayStateMapper:public GLObject // Helper class to associate DisplayState objects with each VRWindow's GL context
		{
		/* Embedded classes: */
		public:
		struct DataItem:public GLObject::DataItem
			{
			/* Elements: */
			public:
			DisplayState displayState; // The display state object
			};
		
		/* Methods from GLObject: */
		void initContext(GLContextData& contextData) const
			{
			/* No need to do anything */
			}
		};
	
	/* Elements: */
	
	/* Multipipe management: */
	Comm::MulticastPipeMultiplexer* multiplexer;
	bool master;
	Comm::MulticastPipe* pipe;
	
	/* Environment dimensions: */
	Scalar inchScale; // Length of an inch expressed in the (arbitrary) Vrui physical coordinate units
	Scalar meterScale; // Length of a meter expressed in the (arbitrary) Vrui physical coordinate units
	Point displayCenter;
	Scalar displaySize;
	Vector forwardDirection;
	Vector upDirection;
	Plane floorPlane;
	
	/* Glyph management: */
	GlyphRenderer* glyphRenderer;
	
	/* Input graph management: */
	Point newInputDevicePosition;
	VirtualInputDevice* virtualInputDevice;
	InputGraphManager* inputGraphManager;
	
	/* Input device management: */
	InputDeviceManager* inputDeviceManager;
	InputDeviceDataSaver* inputDeviceDataSaver;
	MultipipeDispatcher* multipipeDispatcher;
	
	/* Light source management: */
	LightsourceManager* lightsourceManager;
	
	/* Clipping plane management: */
	ClipPlaneManager* clipPlaneManager;
	
	/* Viewer management: */
	int numViewers;
	Viewer* viewers;
	Viewer* mainViewer;
	
	/* Screen management: */
	int numScreens;
	VRScreen* screens;
	VRScreen* mainScreen;
	
	/* Screen protection management: */
	int numProtectors;
	ScreenProtector* protectors;
	
	/* Window management: */
	DisplayStateMapper displayStateMapper;
	
	/* Listener management: */
	int numListeners;
	Listener* listeners;
	Listener* mainListener;
	
	/* Rendering parameters: */
	Scalar frontplaneDist;
	Scalar backplaneDist;
	Color backgroundColor;
	Color ambientLightColor;
	
	/* Widget management: */
	GLMaterial widgetMaterial;
	GLMotif::StyleSheet uiStyleSheet;
	Misc::TimerEventScheduler* timerEventScheduler; // Scheduler for timer events
	GLMotif::WidgetManager* widgetManager;
	bool popWidgetsOnScreen;
	GLMotif::PopupMenu* systemMenuPopup;
	MutexMenu* mainMenu;
	
	/* Navigation transformation management: */
	std::string viewpointFileName;
	bool navigationTransformationEnabled;
	bool delayNavigationTransformation; // Flag whether changes to the navigation transformation are delayed until the beginning of the next frame
	int navigationTransformationChangedMask; // Bit mask for changed parts of the navigation transformation (0x1-transform,0x2-display center/size)
	NavTransform newNavigationTransformation;
	NavTransform navigationTransformation,inverseNavigationTransformation;
	std::vector<NavTransform> storedNavigationTransformations;
	CoordinateManager* coordinateManager;
	
	/* Tool management: */
	ToolManager* toolManager;
	
	/* Vislet management: */
	VisletManager* visletManager;
	
	/* Application function callbacks: */
	FrameFunctionType frameFunction;
	void* frameFunctionData;
	DisplayFunctionType displayFunction;
	void* displayFunctionData;
	PerDisplayInitFunctionType perDisplayInitFunction;
	void* perDisplayInitFunctionData;
	SoundFunctionType soundFunction;
	void* soundFunctionData;
	PerSoundInitFunctionType perSoundInitFunction;
	void* perSoundInitFunctionData;
	
	/* Random number management: */
	unsigned int randomSeed; // Seed value for random number generator
	
	/* Time management: */
	Misc::Timer appTime; // Free-running application timer
	double minimumFrameTime; // Lower limit on frame times; Vrui's main loop will block to pad frame times to this minimum
	double lastFrame; // Application time at which the last frame was started
	double lastFrameDelta; // Duration of last frame
	int numRecentFrameTimes; // Number of recent frame times to average from
	double* recentFrameTimes; // Array of recent times to complete a frame
	int nextFrameTimeIndex; // Index at which the next frame time is stored in the array
	double* sortedFrameTimes; // Helper array to calculate median of frame times
	double currentFrameTime; // Current frame time average
	
	/* Transient dragging/moving/scaling state: */
	const Tool* activeNavigationTool;
	
	/* Transient popup menu/primary widget interaction state: */
	bool widgetInteraction;
	GLMotif::Widget* motionWidget;
	
	/* List of created virtual input devices: */
	std::deque<InputDevice*> createdVirtualInputDevices;
	
	/* Rendering management state: */
	bool updateContinuously; // Flag if the inner Vrui loop never blocks
	
	/* Private methods: */
	GLMotif::Popup* buildViewMenu(void); // Builds the view submenu
	void buildSystemMenu(GLMotif::Container* parent); // Builds the Vrui system menu inside the given container widget
	bool loadViewpointFile(const char* viewpointFileName); // Overrides the navigation transformation with viewpoint data stored in the given viewpoint file
	
	/* Constructors and destructors: */
	VruiState(Comm::MulticastPipeMultiplexer* sMultiplexer,Comm::MulticastPipe* sPipe); // Initializes basic Vrui state
	~VruiState(void);
	
	/* Methods: */
	
	/* Initialization methods: */
	void initialize(const Misc::ConfigurationFileSection& configFileSection); // Initializes complete Vrui state
	void createSystemMenu(void); // Creates Vrui's system menu
	void initTools(const Misc::ConfigurationFileSection& configFileSection); // Initializes all tools listed in the configuration file
	DisplayState* registerContext(GLContextData& contextData) const; // Registers a newly created OpenGL context with the Vrui state object
	
	/* Frame processing methods: */
	void update(void); // Update Vrui state for current frame
	void display(DisplayState* displayState,GLContextData& contextData) const; // Vrui display function
	void sound(ALContextData& contextData) const; // Vrui sound function
	
	/* System menu callback methods: */
	void fileSelectionDialogCancelCallback(GLMotif::FileSelectionDialog::CancelCallbackData* cbData);
	void loadViewOKCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData);
	void loadViewCallback(Misc::CallbackData* cbData);
	void saveViewCallback(Misc::CallbackData* cbData);
	void restoreViewCallback(Misc::CallbackData* cbData);
	void createInputDeviceCallback(Misc::CallbackData* cbData);
	void destroyInputDeviceCallback(Misc::CallbackData* cbData);
	void showScaleBarToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void quitCallback(Misc::CallbackData* cbData);
	};

extern VruiState* vruiState;

/********************************
Private Vrui function prototypes:
********************************/

extern void vruiDelay(double interval);
extern void synchronize(double applicationTime); // Blocks execution until the given application time; can only be called by an input device adapter during its updateInputDevices() method
extern void setDisplayCenter(const Point& newDisplayCenter,Scalar newDisplaySize); // Sets the center and size of Vrui's display environment

}

#endif
