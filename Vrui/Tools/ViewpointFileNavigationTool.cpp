/***********************************************************************
ViewpointFileNavigationTool - Class for tools to play back previously
saved viewpoint data files.
Copyright (c) 2007-2009 Oliver Kreylos

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

#include <stdio.h>
#include <Misc/FileNameExtensions.h>
#include <Misc/File.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/WidgetManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Vrui.h>

#include <Vrui/Tools/ViewpointFileNavigationTool.h>

#include <Vrui/Tools/DenseMatrix.h>

namespace Vrui {

/***************************************************
Methods of class ViewpointFileNavigationToolFactory:
***************************************************/

ViewpointFileNavigationToolFactory::ViewpointFileNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("ViewpointFileNavigationTool",toolManager),
	 viewpointFileName(""),
	 showKeyframes(true),
	 pauseFileName("ViewpointFileNavigation.pauses"),
	 autostart(false)
	{
	/* Initialize tool layout: */
	layout.setNumDevices(1);
	layout.setNumButtons(0,1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("NavigationTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	viewpointFileName=cfs.retrieveString("./viewpointFileName",viewpointFileName);
	showKeyframes=cfs.retrieveValue<bool>("./showKeyframes",showKeyframes);
	pauseFileName=cfs.retrieveString("./pauseFileName",pauseFileName);
	autostart=cfs.retrieveValue<bool>("./autostart",autostart);
	
	/* Set tool class' factory pointer: */
	ViewpointFileNavigationTool::factory=this;
	}

ViewpointFileNavigationToolFactory::~ViewpointFileNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	ViewpointFileNavigationTool::factory=0;
	}

const char* ViewpointFileNavigationToolFactory::getName(void) const
	{
	return "Curve File Animation";
	}

Tool* ViewpointFileNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new ViewpointFileNavigationTool(this,inputAssignment);
	}

void ViewpointFileNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveViewpointFileNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	#if 0
	/* Load base classes: */
	manager.loadClass("NavigationTool");
	#endif
	}

extern "C" ToolFactory* createViewpointFileNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	ViewpointFileNavigationToolFactory* viewpointFileNavigationToolFactory=new ViewpointFileNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return viewpointFileNavigationToolFactory;
	}

extern "C" void destroyViewpointFileNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/****************************************************
Static elements of class ViewpointFileNavigationTool:
****************************************************/

ViewpointFileNavigationToolFactory* ViewpointFileNavigationTool::factory=0;

/********************************************
Methods of class ViewpointFileNavigationTool:
********************************************/

void ViewpointFileNavigationTool::readViewpointFile(const char* fileName)
	{
	try
		{
		/* Open the viewpoint file: */
		Misc::File viewpointFile(fileName,"rt");
		
		if(Misc::hasCaseExtension(fileName,".views"))
			{
			/* Load all viewpoint keyframes from the file: */
			Scalar time(0);
			while(true)
				{
				/* Read the next viewpoint: */
				Scalar timeInterval;
				ControlPoint v;
				if(fscanf(viewpointFile.getFilePtr(),"%lf (%lf, %lf, %lf) %lf (%lf, %lf, %lf) (%lf, %lf, %lf)\n",&timeInterval,&v.center[0],&v.center[1],&v.center[2],&v.size,&v.forward[0],&v.forward[1],&v.forward[2],&v.up[0],&v.up[1],&v.up[2])!=11)
					break;
				
				/* Store the viewpoint: */
				time+=timeInterval;
				times.push_back(time);
				v.size=Math::log(v.size); // Sizes are interpolated logarithmically
				viewpoints.push_back(v);
				}
			
			if(viewpoints.size()>1)
				{
				/* Create a big matrix to solve the C^2 spline problem: */
				int n=viewpoints.size()-1;
				DenseMatrix A(4*n,4*n);
				A.zero();
				DenseMatrix b(4*n,10);
				b.zero();
				
				A(0,0)=Scalar(1);
				writeControlPoint(viewpoints[0],b,0);
				
				#if 1
				/* Zero velocity at start: */
				A(1,0)=Scalar(-3)/(times[1]-times[0]);
				A(1,1)=Scalar(3)/(times[1]-times[0]);
				#else
				/* Zero acceleration at start: */
				A(1,0)=Scalar(6)/Math::sqr(times[1]-times[0]);
				A(1,1)=Scalar(-12)/Math::sqr(times[1]-times[0]);
				A(1,2)=Scalar(6)/Math::sqr(times[1]-times[0]);
				#endif
				
				for(int i=1;i<n;++i)
					{
					A(i*4-2,i*4-3)=Scalar(6)/Math::sqr(times[i]-times[i-1]);
					A(i*4-2,i*4-2)=Scalar(-12)/Math::sqr(times[i]-times[i-1]);
					A(i*4-2,i*4-1)=Scalar(6)/Math::sqr(times[i]-times[i-1]);
					A(i*4-2,i*4+0)=Scalar(-6)/Math::sqr(times[i+1]-times[i]);
					A(i*4-2,i*4+1)=Scalar(12)/Math::sqr(times[i+1]-times[i]);
					A(i*4-2,i*4+2)=Scalar(-6)/Math::sqr(times[i+1]-times[i]);
					
					A(i*4-1,i*4-2)=Scalar(-3)/(times[i]-times[i-1]);
					A(i*4-1,i*4-1)=Scalar(3)/(times[i]-times[i-1]);
					A(i*4-1,i*4+0)=Scalar(3)/(times[i+1]-times[i]);
					A(i*4-1,i*4+1)=Scalar(-3)/(times[i+1]-times[i]);
					
					A(i*4+0,i*4-1)=Scalar(1);
					writeControlPoint(viewpoints[i],b,i*4+0);
					
					A(i*4+1,i*4+0)=Scalar(1);
					writeControlPoint(viewpoints[i],b,i*4+1);
					}
				
				#if 1
				/* Zero velocity at end: */
				A(n*4-2,n*4-2)=Scalar(-3)/(times[n]-times[n-1]);
				A(n*4-2,n*4-1)=Scalar(3)/(times[n]-times[n-1]);
				#else
				/* Zero acceleration at end: */
				A(n*4-2,n*4-3)=Scalar(6)/Math::sqr(times[n]-times[n-1]);
				A(n*4-2,n*4-2)=Scalar(-12)/Math::sqr(times[n]-times[n-1]);
				A(n*4-2,n*4-1)=Scalar(6)/Math::sqr(times[n]-times[n-1]);
				#endif
				
				A(n*4-1,n*4-1)=Scalar(1);
				writeControlPoint(viewpoints[n],b,n*4-1);
				
				/* Solve the system of equations: */
				DenseMatrix x=A.solveLinearEquations(b);
				
				/* Create the spline segment list: */
				for(int i=0;i<n;++i)
					{
					SplineSegment s;
					for(int j=0;j<2;++j)
						s.t[j]=times[i+j];
					for(int cp=0;cp<4;++cp)
						{
						for(int j=0;j<3;++j)
							s.p[cp].center[j]=x(i*4+cp,j);
						s.p[cp].size=x(i*4+cp,3);
						for(int j=0;j<3;++j)
							s.p[cp].forward[j]=x(i*4+cp,4+j);
						for(int j=0;j<3;++j)
							s.p[cp].up[j]=x(i*4+cp,7+j);
						}
					splines.push_back(s);
					}
				}
			}
		else if(Misc::hasCaseExtension(fileName,".curve"))
			{
			/* Load all spline segments from the file: */
			while(true)
				{
				SplineSegment s;
				if(splines.empty())
					{
					/* Read the first control point: */
					ControlPoint cp;
					if(fscanf(viewpointFile.getFilePtr(),"(%lf, %lf, %lf) %lf (%lf, %lf, %lf) (%lf, %lf, %lf)\n",&cp.center[0],&cp.center[1],&cp.center[2],&cp.size,&cp.forward[0],&cp.forward[1],&cp.forward[2],&cp.up[0],&cp.up[1],&cp.up[2])!=10)
						break;
					cp.size=Math::log(cp.size); // Sizes are interpolated logarithmically
					viewpoints.push_back(cp);
					times.push_back(Scalar(0));
					s.t[0]=Scalar(0);
					s.p[0]=cp;
					}
				else
					{
					/* Copy the last control point from the previous segment: */
					s.t[0]=splines.back().t[1];
					s.p[0]=splines.back().p[3];
					}
				
				/* Read the segment's parameter interval: */
				double pi;
				if(fscanf(viewpointFile.getFilePtr(),"%lf\n",&pi)!=1)
					break;
				s.t[1]=s.t[0]+Scalar(pi);
				
				/* Read the intermediate control points: */
				ControlPoint m0;
				if(fscanf(viewpointFile.getFilePtr(),"(%lf, %lf, %lf) %lf (%lf, %lf, %lf) (%lf, %lf, %lf)\n",&m0.center[0],&m0.center[1],&m0.center[2],&m0.size,&m0.forward[0],&m0.forward[1],&m0.forward[2],&m0.up[0],&m0.up[1],&m0.up[2])!=10)
					break;
				m0.size=Math::log(m0.size); // Sizes are interpolated logarithmically
				s.p[1]=m0;
				ControlPoint m1;
				if(fscanf(viewpointFile.getFilePtr(),"(%lf, %lf, %lf) %lf (%lf, %lf, %lf) (%lf, %lf, %lf)\n",&m1.center[0],&m1.center[1],&m1.center[2],&m1.size,&m1.forward[0],&m1.forward[1],&m1.forward[2],&m1.up[0],&m1.up[1],&m1.up[2])!=10)
					break;
				m1.size=Math::log(m1.size); // Sizes are interpolated logarithmically
				s.p[2]=m1;
				
				/* Read the last control point: */
				ControlPoint cp;
				if(fscanf(viewpointFile.getFilePtr(),"(%lf, %lf, %lf) %lf (%lf, %lf, %lf) (%lf, %lf, %lf)\n",&cp.center[0],&cp.center[1],&cp.center[2],&cp.size,&cp.forward[0],&cp.forward[1],&cp.forward[2],&cp.up[0],&cp.up[1],&cp.up[2])!=10)
					break;
				cp.size=Math::log(cp.size); // Sizes are interpolated logarithmically
				viewpoints.push_back(cp);
				times.push_back(s.t[1]);
				s.p[3]=cp;
				
				/* Save the spline segment: */
				splines.push_back(s);
				}
			}
		}
	catch(...)
		{
		/* Just ignore the error */
		}
	
	if(!splines.empty()&&factory->autostart)
		{
		if(activate())
			{
			/* Save the animation's start time: */
			startTime=Scalar(getApplicationTime())+splines[0].t[0];
			paused=false;
			lastParameter=splines[0].t[0]-Scalar(1);
			}
		}
	else if(!viewpoints.empty()&&activate())
		{
		/* Go to the first viewpoint: */
		const ControlPoint& v=viewpoints[0];
		NavTransform nav=NavTransform::identity;
		nav*=NavTransform::translateFromOriginTo(getDisplayCenter());
		nav*=NavTransform::rotate(Rotation::fromBaseVectors(Geometry::cross(getForwardDirection(),getUpDirection()),getForwardDirection()));
		nav*=NavTransform::scale(getDisplaySize()/Math::exp(v.size)); // Scales are interpolated logarithmically
		nav*=NavTransform::rotate(Geometry::invert(Rotation::fromBaseVectors(Geometry::cross(v.forward,v.up),v.forward)));
		nav*=NavTransform::translateToOriginFrom(v.center);
		setNavigationTransformation(nav);
		
		deactivate();
		}
	}

void ViewpointFileNavigationTool::loadViewpointFileOKCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	/* Load the selected viewpoint file: */
	readViewpointFile(cbData->selectedFileName.c_str());
	
	/* Destroy the file selection dialog: */
	getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
	}

void ViewpointFileNavigationTool::loadViewpointFileCancelCallback(GLMotif::FileSelectionDialog::CancelCallbackData* cbData)
	{
	/* Destroy the file selection dialog: */
	getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
	}

void ViewpointFileNavigationTool::writeControlPoint(const ViewpointFileNavigationTool::ControlPoint& cp,DenseMatrix& b,int rowIndex)
	{
	for(int j=0;j<3;++j)
		b(rowIndex,j)=cp.center[j];
	b(rowIndex,3)=cp.size;
	Vector forward=Geometry::normalize(cp.forward);
	for(int j=0;j<3;++j)
		b(rowIndex,4+j)=forward[j];
	Vector up=Geometry::normalize(cp.up);
	for(int j=0;j<3;++j)
		b(rowIndex,7+j)=up[j];
	}

void ViewpointFileNavigationTool::interpolate(const ViewpointFileNavigationTool::ControlPoint& p0,const ViewpointFileNavigationTool::ControlPoint& p1,Scalar t,ViewpointFileNavigationTool::ControlPoint& result)
	{
	result.center=Geometry::affineCombination(p0.center,p1.center,t);
	result.size=p0.size*(Scalar(1)-t)+p1.size*t;
	result.forward=p0.forward*(Scalar(1)-t)+p1.forward*t;
	result.up=p0.up*(Scalar(1)-t)+p1.up*t;
	}

ViewpointFileNavigationTool::ViewpointFileNavigationTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(sFactory,inputAssignment),
	 nextViewpointIndex(0U),
	 startTime(0),
	 paused(false)
	{
	/* Load scheduled pauses if the file exists: */
	try
		{
		Misc::File pauseFile(factory->pauseFileName.c_str(),"rt");
		while(true)
			{
			double pauseTime;
			if(fscanf(pauseFile.getFilePtr(),"%lf",&pauseTime)!=1)
				break;
			pauses.push_back(Scalar(pauseTime));
			}
		}
	catch(std::runtime_error)
		{
		/* Ignore the error */
		}
	
	/* Bring up a file selection dialog if there is no pre-configured viewpoint file: */
	if(factory->viewpointFileName.empty())
		{
		GLMotif::FileSelectionDialog* loadViewpointFileDialog=new GLMotif::FileSelectionDialog(getWidgetManager(),"Load Viewpoint File",0,".views,.curve",openPipe());
		loadViewpointFileDialog->getOKCallbacks().add(this,&ViewpointFileNavigationTool::loadViewpointFileOKCallback);
		loadViewpointFileDialog->getCancelCallbacks().add(this,&ViewpointFileNavigationTool::loadViewpointFileCancelCallback);
		popupPrimaryWidget(loadViewpointFileDialog,getNavigationTransformation().transform(getDisplayCenter()));
		}
	else
		{
		/* Load the configured viewpoint file: */
		readViewpointFile(factory->viewpointFileName.c_str());
		}
	}

const ToolFactory* ViewpointFileNavigationTool::getFactory(void) const
	{
	return factory;
	}

void ViewpointFileNavigationTool::buttonCallback(int,int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Activation button has just been pressed
		{
		#if 0
		/* Set the next saved viewpoint if the tool can be activated: */
		if(!viewpoints.empty()&&activate())
			{
			/* Compute the appropriate navigation transformation from the next viewpoint: */
			const Viewpoint& v=viewpoints[nextViewpointIndex];
			NavTransform nav=NavTransform::identity;
			nav*=NavTransform::translateFromOriginTo(getDisplayCenter());
			nav*=NavTransform::rotate(Rotation::fromBaseVectors(Geometry::cross(getForwardDirection(),getUpDirection()),getForwardDirection()));
			nav*=NavTransform::scale(getDisplaySize()/Math::exp(v.size)); // Scales are interpolated logarithmically
			nav*=NavTransform::rotate(Geometry::invert(Rotation::fromBaseVectors(Geometry::cross(v.forward,v.up),v.forward)));
			nav*=NavTransform::translateToOriginFrom(v.center);
			
			/* Set the viewpoint: */
			setNavigationTransformation(nav);
			
			/* Go to the next viewpoint: */
			++nextViewpointIndex;
			if(nextViewpointIndex==viewpoints.size())
				nextViewpointIndex=0U;
			
			/* Deactivate the tool: */
			deactivate();
			}
		#else
		/* Start animating the viewpoint if there are spline segments and the tool can be activated: */
		if(!splines.empty())
			{
			if(paused&&activate())
				{
				/* Unpause the animation: */
				paused=false;
				startTime=getApplicationTime()-pauseTime;
				}
			else if(isActive())
				{
				/* Pause the animation: */
				paused=true;
				pauseTime=Scalar(getApplicationTime())-startTime;
				deactivate();
				}
			else if(activate())
				{
				/* Save the animation's start time: */
				startTime=Scalar(getApplicationTime())+splines[0].t[0];
				paused=false;
				lastParameter=splines[0].t[0]-Scalar(1);
				}
			}
		#endif
		}
	}

void ViewpointFileNavigationTool::frame(void)
	{
	/* Animate the navigation transformation if the tool is active: */
	if(!paused&&isActive())
		{
		/* Get the current animation time: */
		Scalar time=Scalar(getApplicationTime())-startTime;
		
		/* Check if a pause was scheduled between the last frame and this one: */
		bool passedPause=false;
		for(std::vector<Scalar>::const_iterator pIt=pauses.begin();pIt!=pauses.end();++pIt)
			if(lastParameter<*pIt&&*pIt<=time)
				{
				passedPause=true;
				time=*pIt;
				break;
				}
		
		/* Find the spline segment containing the animation time: */
		int l=0;
		int r=splines.size();
		while(r-l>1)
			{
			int m=(l+r)>>1;
			if(time>=splines[m].t[0])
				l=m;
			else
				r=m;
			}
		const SplineSegment& s=splines[l];
		if(time<s.t[1])
			{
			/* Evaluate the spline segment at the current time: */
			Scalar t=(time-s.t[0])/(s.t[1]-s.t[0]);
			ControlPoint cp[6];
			for(int i=0;i<3;++i)
				interpolate(s.p[i],s.p[i+1],t,cp[i]);
			for(int i=0;i<2;++i)
				interpolate(cp[i],cp[i+1],t,cp[3+i]);
			interpolate(cp[3],cp[4],t,cp[5]);
			
			/* Compute the appropriate navigation transformation from the next viewpoint: */
			NavTransform nav=NavTransform::identity;
			nav*=NavTransform::translateFromOriginTo(getDisplayCenter());
			nav*=NavTransform::rotate(Rotation::fromBaseVectors(Geometry::cross(getForwardDirection(),getUpDirection()),getForwardDirection()));
			nav*=NavTransform::scale(getDisplaySize()/Math::exp(cp[5].size)); // Scales are interpolated logarithmically
			nav*=NavTransform::rotate(Geometry::invert(Rotation::fromBaseVectors(Geometry::cross(cp[5].forward,cp[5].up),cp[5].forward)));
			nav*=NavTransform::translateToOriginFrom(cp[5].center);
			
			/* Set the viewpoint: */
			setNavigationTransformation(nav);
			
			nextViewpointIndex=l+1;
			}
		else
			{
			/* Stop animating; spline is over: */
			deactivate();
			
			nextViewpointIndex=0;
			}
		
		if(passedPause)
			{
			paused=true;
			pauseTime=time;
			deactivate();
			}
		else
			requestUpdate();
		lastParameter=time;
		}
	}

void ViewpointFileNavigationTool::display(GLContextData& contextData) const
	{
	if(!viewpoints.empty()&&factory->showKeyframes)
		{
		/* Display the next keyframe viewpoint in navigational coordinates: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(3.0f);
		glPushMatrix();
		glMultMatrix(getNavigationTransformation());
		
		glBegin(GL_LINES);
		glColor3f(1.0f,0.0f,0.0f);
		glVertex(viewpoints[nextViewpointIndex].center);
		glVertex(viewpoints[nextViewpointIndex].center+viewpoints[nextViewpointIndex].forward*Math::exp(viewpoints[nextViewpointIndex].size)*Scalar(0.25));
		glColor3f(0.0f,1.0f,0.0f);
		glVertex(viewpoints[nextViewpointIndex].center);
		glVertex(viewpoints[nextViewpointIndex].center+viewpoints[nextViewpointIndex].up*Math::exp(viewpoints[nextViewpointIndex].size)*Scalar(0.25));
		glEnd();
		
		glPopMatrix();
		glPopAttrib();
		}
	}

}
