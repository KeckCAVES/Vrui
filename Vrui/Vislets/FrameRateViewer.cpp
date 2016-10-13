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

#include <Vrui/Vislets/FrameRateViewer.h>

// DEBUGGING
#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/MessageLogger.h>
#include <Math/Math.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLColorTemplates.h>
#include <Vrui/Vrui.h>
#include <Vrui/VisletManager.h>
#include <Vrui/DisplayState.h>

namespace Vrui {

namespace Vislets {

/***************************************
Methods of class FrameRateViewerFactory:
***************************************/

FrameRateViewerFactory::FrameRateViewerFactory(VisletManager& visletManager)
	:VisletFactory("FrameRateViewer",visletManager),
	 historySize(1024)
	{
	#if 0
	/* Insert class into class hierarchy: */
	VisletFactory* visletFactory=visletManager.loadClass("Vislet");
	visletFactory->addChildClass(this);
	addParentClass(visletFactory);
	#endif
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=visletManager.getVisletClassSection(getClassName());
	
	historySize=cfs.retrieveValue<unsigned int>("./historySize",(unsigned int)(historySize));
	
	/* Set vislet class' factory pointer: */
	FrameRateViewer::factory=this;
	}

FrameRateViewerFactory::~FrameRateViewerFactory(void)
	{
	/* Reset vislet class' factory pointer: */
	FrameRateViewer::factory=0;
	}

Vislet* FrameRateViewerFactory::createVislet(int numArguments,const char* const arguments[]) const
	{
	return new FrameRateViewer(numArguments,arguments);
	}

void FrameRateViewerFactory::destroyVislet(Vislet* vislet) const
	{
	delete vislet;
	}

extern "C" void resolveFrameRateViewerDependencies(Plugins::FactoryManager<VisletFactory>& manager)
	{
	#if 0
	/* Load base classes: */
	manager.loadClass("Vislet");
	#endif
	}

extern "C" VisletFactory* createFrameRateViewerFactory(Plugins::FactoryManager<VisletFactory>& manager)
	{
	/* Get pointer to vislet manager: */
	VisletManager* visletManager=static_cast<VisletManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	FrameRateViewerFactory* factory=new FrameRateViewerFactory(*visletManager);
	
	/* Return factory object: */
	return factory;
	}

extern "C" void destroyFrameRateViewerFactory(VisletFactory* factory)
	{
	delete factory;
	}

/****************************************
Static elements of class FrameRateViewer:
****************************************/

FrameRateViewerFactory* FrameRateViewer::factory=0;

/********************************
Methods of class FrameRateViewer:
********************************/

FrameRateViewer::FrameRateViewer(int numArguments,const char* const arguments[])
	:historySize(factory->historySize),
	 history(0),historyEnd(0),historyHead(0),
	 min(0.0),max(0.0),
	 numberRenderer(12.0f,false)
	{
	/* Parse the command line: */
	for(int i=0;i<numArguments;++i)
		{
		if(arguments[i][0]=='-')
			{
			if(strcasecmp(arguments[i]+1,"hs")==0||strcasecmp(arguments[i]+1,"historySize")==0)
				{
				++i;
				if(i<numArguments)
					historySize=atoi(arguments[i]);
				else
					Misc::formattedConsoleError("FrameRateViewer: Ignoring dangling %s option",arguments[i-1]);
				}
			else
				Misc::formattedConsoleError("FrameRateViewer: Ignoring unknown %s option",arguments[i]);
			}
		else
			Misc::formattedConsoleError("FrameRateViewer: Ignoring unknown %s parameter",arguments[i]);
		}
	
	/* Allocate and initialize the history buffer: */
	history=new double[historySize];
	historyEnd=history+historySize;
	historyHead=history;
	for(size_t i=0;i<historySize;++i)
		history[i]=0.0;
	}

FrameRateViewer::~FrameRateViewer(void)
	{
	delete[] history;
	}

VisletFactory* FrameRateViewer::getFactory(void) const
	{
	return factory;
	}

void FrameRateViewer::disable(void)
	{
	Vislet::disable();
	}

void FrameRateViewer::enable(void)
	{
	Vislet::enable();
	}

void FrameRateViewer::frame(void)
	{
	/* Query the duration of the last Vrui frame: */
	double frameTime=getFrameTime();
	
	/* Remember the to-be-removed history entry: */
	double oldTime=*historyHead;
	
	/* Update the frame rate history: */
	*historyHead=frameTime;
	++historyHead;
	if(historyHead==historyEnd)
		historyHead=history;
	
	/* Update the history's frame time ranges: */
	if(min>frameTime)
		min=frameTime;
	if(max<frameTime)
		max=frameTime;
	
	/* Check if the just removed history entry was the min or max: */
	if(oldTime==min||oldTime==max)
		{
		/* Recalculate minimum and maximum: */
		min=max=history[0];
		for(size_t i=1;i<historySize;++i)
			{
			if(min>history[i])
				min=history[i];
			else if(max<history[i])
				max=history[i];
			}
		}
	}

void FrameRateViewer::display(GLContextData& contextData) const
	{
	/* Get the viewport size of the current window: */
	const DisplayState& ds=getDisplayState(contextData);
	
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(1.0f);
	
	/* Go to pixel coordinates: */
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0,ds.viewport[2],0.0,ds.viewport[3],0.0,1.0);
	
	/* Get graph colors: */
	Color bg=getBackgroundColor();
	Color fg=getForegroundColor();
	
	/* Find the closest powers of ten bracketing the frame rate range: */
	double bottom=0.0;
	if(min>0.0)
		bottom=Math::pow(10.0,Math::floor(Math::log10(min)));
	double top=0.0;
	if(max>0.0)
		top=Math::pow(10.0,Math::ceil(Math::log10(max)));
	
	/* Ensure top and bottom are at least one power of ten apart: */
	if(bottom>top/10.0)
		bottom=top/10.0;
	
	/* Calculate graph offsets and scaling factors: */
	double xs=double(ds.viewport[2])*0.8/double(historySize);
	double x0=double(ds.viewport[2])*0.15;
	double ys=double(ds.viewport[3])*0.2/(top-bottom);
	double y0=double(ds.viewport[3])*0.05;
	
	/* Draw a grid: */
	glBegin(GL_LINES);
	glColor3f(Math::mid(bg[0],fg[0]),Math::mid(bg[1],fg[1]),Math::mid(bg[2],fg[2]));
	
	/* Draw the bottom line: */
	glVertex2d(x0-5.0,y0);
	glVertex2d(x0+double(historySize)*xs+5.0,y0);
	
	/* Draw intermediate and top lines: */
	for(int i=1;i<=10;++i)
		{
		double l=top*double(i)/10.0;
		glVertex2d(x0-5.0,y0+(l-bottom)*ys);
		glVertex2d(x0+double(historySize)*xs+5.0,y0+(l-bottom)*ys);
		}
	glEnd();
	
	/* Draw bottom and top labels: */
	numberRenderer.drawNumber(GLNumberRenderer::Vector(x0-10.0,y0,0.0),bottom*1000.0,2,contextData,1,0);
	numberRenderer.drawNumber(GLNumberRenderer::Vector(x0-10.0,y0+(top*0.5-bottom)*ys,0.0),top*500.0,2,contextData,1,0);
	numberRenderer.drawNumber(GLNumberRenderer::Vector(x0-10.0,y0+(top-bottom)*ys,0.0),top*1000.0,2,contextData,1,0);
	
	/* Draw the frame rate graph: */
	glBegin(GL_LINE_STRIP);
	glColor(fg);
	int x=0;
	for(const double* hPtr=historyHead;hPtr!=historyEnd;++hPtr,++x)
		glVertex2d(x0+double(x)*xs,y0+(*hPtr-bottom)*ys);
	for(const double* hPtr=history;hPtr!=historyHead;++hPtr,++x)
		glVertex2d(x0+double(x)*xs,y0+(*hPtr-bottom)*ys);
	glEnd();
	
	/* Restore OpenGL state: */
	glPopAttrib();
	
	/* Return to physical coordinates: */
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	}

}

}
