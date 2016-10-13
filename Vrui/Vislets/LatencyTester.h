/***********************************************************************
LatencyTester - Vislet class to measure the frame-to-display latency of
arbitrary Vrui applications using an Oculus latency tester.
Copyright (c) 2016 Oliver Kreylos

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

#ifndef VRUI_VISLETS_LATENCYTESTER_INCLUDED
#define VRUI_VISLETS_LATENCYTESTER_INCLUDED

#include <Threads/Thread.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLObject.h>
#include <GL/GLShader.h>
#include <Vrui/Vislet.h>

/* Forward declarations: */
namespace Vrui {
namespace Vislets {
class LatencyTesterDevice;
}
}

namespace Vrui {

namespace Vislets {

class LatencyTester;

class LatencyTesterFactory:public Vrui::VisletFactory
	{
	friend class LatencyTester;
	
	/* Elements: */
	private:
	
	/* Constructors and destructors: */
	public:
	LatencyTesterFactory(Vrui::VisletManager& visletManager);
	virtual ~LatencyTesterFactory(void);
	
	/* Methods from VisletFactory: */
	virtual Vislet* createVislet(int numVisletArguments,const char* const visletArguments[]) const;
	virtual void destroyVislet(Vislet* vislet) const;
	};

class LatencyTester:public Vrui::Vislet,public GLObject
	{
	friend class LatencyTesterFactory;
	
	/* Embedded classes: */
	private:
	enum TestState
		{
		IDLE,
		SAMPLING_BLACK_PREP1,
		SAMPLING_BLACK_PREP2,
		SAMPLING_BLACK,
		SAMPLING_WHITE_PREP,
		SAMPLING_WHITE,
		WAITING_BLACK,
		PREPARE_WHITE1,
		PREPARE_WHITE2,
		WAITING_WHITE,
		PREPARE_BLACK1,
		PREPARE_BLACK2,
		FINISH
		};
	
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLShader displayOverrideShader; // A shader to paint the entire display to a solid color
		int displayOverrideShaderUniforms[1]; // Handles to display override shader's uniform variables
		};
	
	/* Elements: */
	private:
	static LatencyTesterFactory* factory; // Pointer to the factory object for this class
	LatencyTesterDevice* device; // Pointer to the latency tester device
	TestState testState; // Current state of the latency testing automaton
	double stateAdvanceTime; // Application time at which the current state is advanced to the next
	unsigned int accumR,accumG,accumB,accumSum; // Color accumulator while sampling black or white levels
	Threads::Thread communicationThread; // Thread receiving raw HID reports from the latency tester
	bool override; // Flag whether to override the display
	GLColor<GLfloat,4> color; // Current color with which to paint all displays
	
	/* Private methods: */
	void* communicationThreadMethod(void); // Method receiving raw HID reports from the latency tester
	
	/* Constructors and destructors: */
	public:
	LatencyTester(int numArguments,const char* const arguments[]);
	virtual ~LatencyTester(void);
	
	/* Methods from Vislet: */
	public:
	virtual VisletFactory* getFactory(void) const;
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

}

}

#endif
