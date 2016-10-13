/***********************************************************************
FrameRateViewer - Vislet class to view a live graph of Vrui frame times
for debugging and optimization purposes.
Copyright (c) 2015 Oliver Kreylos

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

#ifndef VRUI_VISLETS_FRAMERATEVIEWER_INCLUDED
#define VRUI_VISLETS_FRAMERATEVIEWER_INCLUDED

#include <GL/GLNumberRenderer.h>
#include <Vrui/Vislet.h>

namespace Vrui {

namespace Vislets {

class FrameRateViewer;

class FrameRateViewerFactory:public Vrui::VisletFactory
	{
	friend class FrameRateViewer;
	
	/* Elements: */
	size_t historySize; // Default size of history buffer
	
	/* Constructors and destructors: */
	public:
	FrameRateViewerFactory(Vrui::VisletManager& visletManager);
	virtual ~FrameRateViewerFactory(void);
	
	/* Methods from VisletFactory: */
	virtual Vislet* createVislet(int numVisletArguments,const char* const visletArguments[]) const;
	virtual void destroyVislet(Vislet* vislet) const;
	};

class FrameRateViewer:public Vrui::Vislet
	{
	friend class FrameRateViewerFactory;
	
	/* Elements: */
	private:
	static FrameRateViewerFactory* factory; // Pointer to the factory object for this class
	size_t historySize; // Number of frame rate measurements in the history buffer
	double* history; // Frame rate history buffer
	double* historyEnd; // Pointer to end of history buffer
	double* historyHead; // Pointer to oldest entry in the history buffer
	double min,max; // Minimum and maximum frame rates currently in the history buffer
	GLNumberRenderer numberRenderer; // Helper object to draw numbers
	
	/* Constructors and destructors: */
	public:
	FrameRateViewer(int numArguments,const char* const arguments[]);
	virtual ~FrameRateViewer(void);
	
	/* Methods from Vislet: */
	public:
	virtual VisletFactory* getFactory(void) const;
	virtual void disable(void);
	virtual void enable(void);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

}

}

#endif
