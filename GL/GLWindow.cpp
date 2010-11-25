/***********************************************************************
GLWindow - Class to encapsulate details of the underlying window system
implementation from an application wishing to use OpenGL windows.
Copyright (c) 2001-2005 Oliver Kreylos

This file is part of the OpenGL/GLX Support Library (GLXSupport).

The OpenGL/GLX Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenGL/GLX Support Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenGL/GLX Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <X11/cursorfont.h>
#include <Misc/ThrowStdErr.h>

#include <GL/GLWindow.h>

/**************************
Methods of class GLWindow:
**************************/

void GLWindow::initWindow(const char* sWindowName,int* visualProperties)
	{
	/* Get a handle to the root window: */
	root=RootWindow(display,screen);
	
	/* Query for GLX extension: */
	int errorBase,eventBase;
	if(!glXQueryExtension(display,&errorBase,&eventBase))
		Misc::throwStdErr("GLWindow: GLX extension not supported");
	
	/* Use default visual properties if none were provided: */
	static int defaultVisualProperties[]={GLX_RGBA,GLX_RED_SIZE,8,GLX_GREEN_SIZE,8,GLX_BLUE_SIZE,8,GLX_DEPTH_SIZE,16,GLX_DOUBLEBUFFER,None};
	if(visualProperties==0)
		visualProperties=defaultVisualProperties;
	
	/* Look for a matching visual: */
	XVisualInfo* visInfo=glXChooseVisual(display,screen,visualProperties);
	if(visInfo==0)
		{
		/* Reduce any requested color channel sizes, and try again: */
		for(int i=0;visualProperties[i]!=None;++i)
			if(visualProperties[i]==GLX_RED_SIZE||visualProperties[i]==GLX_GREEN_SIZE||visualProperties[i]==GLX_BLUE_SIZE)
				{
				/* Ask for at least one bit per channel: */
				++i;
				visualProperties[i]=1;
				}
		
		/* Search again: */
		visInfo=glXChooseVisual(display,screen,visualProperties);
		if(visInfo==0)
			{
			/* Reduce any requested depth channel sizes, and try yet again: */
			for(int i=0;visualProperties[i]!=None;++i)
				if(visualProperties[i]==GLX_DEPTH_SIZE)
					{
					/* Ask for at least one bit of depth channel: */
					++i;
					visualProperties[i]=1;
					}
			
			/* Search one last time: */
			visInfo=glXChooseVisual(display,screen,visualProperties);
			if(visInfo==0)
				{
				/* Now fail: */
				Misc::throwStdErr("GLWindow: No suitable visual found");
				}
			}
		}
	
	/* Create an OpenGL context: */
	context=glXCreateContext(display,visInfo,0,GL_TRUE);
	if(context==0)
		Misc::throwStdErr("GLWindow: Unable to create GL context");
	
	/* Create an X colormap (visual might not be default): */
	colorMap=XCreateColormap(display,root,visInfo->visual,AllocNone);
	
	/* Create an X window with the selected visual: */
	XSetWindowAttributes swa;
	swa.colormap=colorMap;
	swa.border_pixel=0;
	if(fullscreen) // Create a fullscreen window
		{
		windowPos.origin[0]=0;
		windowPos.origin[1]=0;
		windowPos.size[0]=DisplayWidth(display,screen);
		windowPos.size[1]=DisplayHeight(display,screen);
		swa.override_redirect=True;
		}
	else
		swa.override_redirect=False;
	swa.event_mask=PointerMotionMask|EnterWindowMask|LeaveWindowMask|ButtonPressMask|ButtonReleaseMask|KeyPressMask|KeyReleaseMask|ExposureMask|StructureNotifyMask;
	unsigned long attributeMask=CWBorderPixel|CWColormap|CWOverrideRedirect|CWEventMask;
	window=XCreateWindow(display,root,
	                     windowPos.origin[0],windowPos.origin[1],windowPos.size[0],windowPos.size[1],
	                     0,visInfo->depth,InputOutput,visInfo->visual,attributeMask,&swa);
	XSetStandardProperties(display,window,sWindowName,sWindowName,None,0,0,0);
	
	/* Delete the visual information structure: */
	XFree(visInfo);
	
	/* Initiate window manager communication: */
	wmProtocolsAtom=XInternAtom(display,"WM_PROTOCOLS",False);
	wmDeleteWindowAtom=XInternAtom(display,"WM_DELETE_WINDOW",False);
	XSetWMProtocols(display,window,&wmDeleteWindowAtom,1);
	
	/* Display the window on the screen: */
	XMapWindow(display,window);
	
	if(fullscreen)
		{
		/* Grab pointer and keyboard: */
		XGrabPointer(display,window,True,0,GrabModeAsync,GrabModeAsync,None,None,CurrentTime);
		XGrabKeyboard(display,window,True,GrabModeAsync,GrabModeAsync,CurrentTime);
		}
	
	/* Gobble up the initial rush of X events regarding window creation: */
	#if 0
	XEvent event;
	while(XCheckWindowEvent(display,window,StructureNotifyMask,&event))
		{
		switch(event.type)
			{
			case ConfigureNotify:
				/* Retrieve the final window size: */
				windowWidth=event.xconfigure.width;
				windowHeight=event.xconfigure.height;
				break;
			}
		}
	#else
	while(true)
		{
		/* Look at the next event: */
		XEvent event;
		XPeekEvent(display,&event);
		if(event.type==Expose)
			break; // Leave this event for the caller to process
		
		/* Process the next event: */
		XNextEvent(display,&event);
		switch(event.type)
			{
			case ConfigureNotify:
				/* Retrieve the final window position and size: */
				windowPos.origin[0]=event.xconfigure.x;
				windowPos.origin[1]=event.xconfigure.y;
				windowPos.size[0]=event.xconfigure.width;
				windowPos.size[1]=event.xconfigure.height;
				break;
			}
		}
	#endif
	}

GLWindow::GLWindow(Display* sDisplay,int sScreen,const char* sWindowName,const GLWindow::WindowPos& sWindowPos,int* visualProperties)
	:privateConnection(false),display(sDisplay),screen(sScreen),
	 windowPos(sWindowPos),fullscreen(windowPos.size[0]==0||windowPos.size[1]==0)
	{
	/* Call common part of window initialization routine: */
	initWindow(sWindowName,visualProperties);
	}

GLWindow::GLWindow(const char* sDisplayName,const char* sWindowName,const GLWindow::WindowPos& sWindowPos,int* visualProperties)
	:privateConnection(true),
	 windowPos(sWindowPos),fullscreen(windowPos.size[0]==0||windowPos.size[1]==0)
	{
	/* Open connection to the X server: */
	display=XOpenDisplay(sDisplayName);
	if(display==0)
		Misc::throwStdErr("GLWindow: Unable to open display");
	screen=DefaultScreen(display);
	
	/* Call common part of window initialization routine: */
	initWindow(sWindowName,visualProperties);
	}

GLWindow::GLWindow(const char* sWindowName,const GLWindow::WindowPos& sWindowPos,int* visualProperties)
	:privateConnection(true),
	 windowPos(sWindowPos),fullscreen(windowPos.size[0]==0||windowPos.size[1]==0)
	{
	/* Open connection to the default X server: */
	display=XOpenDisplay(0);
	if(display==0)
		Misc::throwStdErr("GLWindow: Unable to open display");
	screen=DefaultScreen(display);
	
	/* Call common part of window initialization routine: */
	initWindow(sWindowName,visualProperties);
	}

GLWindow::~GLWindow(void)
	{
	if(fullscreen)
		{
		/* Release the pointer and keyboard grab: */
		XUngrabPointer(display,CurrentTime);
		XUngrabKeyboard(display,CurrentTime);
		}
	
	/* Close GL context and window: */
	XUnmapWindow(display,window);
	if(glXGetCurrentContext()==context)
		glXMakeCurrent(display,None,0);
	XDestroyWindow(display,window);
	XFreeColormap(display,colorMap);
	glXDestroyContext(display,context);
	
	/* Close a private display: */
	if(privateConnection)
		XCloseDisplay(display);
	}

GLWindow::WindowPos GLWindow::getRootWindowPos(void) const
	{
	return WindowPos(DisplayWidth(display,screen),DisplayHeight(display,screen));
	}

double GLWindow::getScreenWidthMM(void) const
	{
	return double(DisplayWidthMM(display,screen));
	}

double GLWindow::getScreenHeightMM(void) const
	{
	return double(DisplayHeightMM(display,screen));
	}

void GLWindow::makeFullscreen(void)
	{
	/*********************************************************************
	"Sane" version of fullscreen switch: Use the window manager protocol
	when supported; otherwise, fall back to hacky method.
	*********************************************************************/
	
	/* Get relevant window manager protocol atoms: */
	Atom netwmStateAtom=XInternAtom(display,"_NET_WM_STATE",True);
	Atom netwmStateFullscreenAtom=XInternAtom(display,"_NET_WM_STATE_FULLSCREEN",True);
	if(netwmStateAtom!=None&&netwmStateFullscreenAtom!=None)
		{
		/* Ask the window manager to make this window fullscreen: */
		XEvent fullscreenEvent;
		memset(&fullscreenEvent,0,sizeof(XEvent));
		fullscreenEvent.xclient.type=ClientMessage;
		fullscreenEvent.xclient.serial=0;
		fullscreenEvent.xclient.send_event=True;
		fullscreenEvent.xclient.display=display;
		fullscreenEvent.xclient.window=window;
		fullscreenEvent.xclient.message_type=netwmStateAtom;
		fullscreenEvent.xclient.format=32;
		fullscreenEvent.xclient.data.l[0]=1; // Should be _NET_WM_STATE_ADD, but that doesn't work for some reason
		fullscreenEvent.xclient.data.l[1]=netwmStateFullscreenAtom;
		fullscreenEvent.xclient.data.l[2]=0;
		XSendEvent(display,RootWindow(display,screen),False,SubstructureRedirectMask|SubstructureNotifyMask,&fullscreenEvent);
		XFlush(display);
		}
	else
		{
		/*******************************************************************
		Use hacky method of adjusting window size just beyond the root
		window. Only method available if there is no window manager, like on
		dedicated cluster rendering nodes.
		*******************************************************************/
		
		/* Query the window's geometry to calculate its offset inside its parent window (the window manager decoration): */
		Window win_root;
		int win_x,win_y;
		unsigned int win_width,win_height,win_borderWidth,win_depth;
		XGetGeometry(display,window,&win_root,&win_x,&win_y,&win_width,&win_height,&win_borderWidth,&win_depth);
		
		/* Set the window's position and size such that the window manager decoration falls outside the root window: */
		XMoveResizeWindow(display,window,-win_x,-win_y,DisplayWidth(display,screen),DisplayHeight(display,screen));
		}
	
	/* Raise the window to the top of the stacking hierarchy: */
	XRaiseWindow(display,window);
	}

void GLWindow::disableMouseEvents(void)
	{
	/* Get the window's current event mask: */
	XWindowAttributes wa;
	XGetWindowAttributes(display,window,&wa);
	
	/* Disable mouse-related events: */
	XSetWindowAttributes swa;
	swa.event_mask=wa.all_event_masks&~(PointerMotionMask|EnterWindowMask|LeaveWindowMask|ButtonPressMask|ButtonReleaseMask);
	XChangeWindowAttributes(display,window,CWEventMask,&swa);
	}

void GLWindow::hideCursor(void)
	{
	/* Why is it so goshdarn complicated to just hide the friggin' cursor? */
	static char emptyCursorBits[]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                               0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                               0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                               0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	Pixmap emptyCursorPixmap=XCreatePixmapFromBitmapData(display,window,emptyCursorBits,16,16,1,0,1);
	XColor black,white; // Actually, both are dummy colors
	Cursor emptyCursor=XCreatePixmapCursor(display,emptyCursorPixmap,emptyCursorPixmap,&black,&white,0,0);
	XDefineCursor(display,window,emptyCursor);
	XFreeCursor(display,emptyCursor);
	XFreePixmap(display,emptyCursorPixmap);
	}

void GLWindow::showCursor(void)
	{
	XUndefineCursor(display,window);
	}

void GLWindow::setCursorPos(int newCursorX,int newCursorY)
	{
	XWarpPointer(display,None,window,0,0,0,0,newCursorX,newCursorY);
	}

void GLWindow::redraw(void)
	{
	/* Send an expose X event for the entire area to this window: */
	XEvent event;
	memset(&event,0,sizeof(XEvent));
	event.type=Expose;
	event.xexpose.display=display;
	event.xexpose.window=window;
	event.xexpose.x=0;
	event.xexpose.y=0;
	event.xexpose.width=windowPos.size[0];
	event.xexpose.height=windowPos.size[1];
	event.xexpose.count=0;
	XSendEvent(display,window,False,0x0,&event);
	XFlush(display);
	}

bool GLWindow::processEvent(const XEvent& event)
	{
	bool closeRequested=false;
	switch(event.type)
		{
		case ConfigureNotify:
			{
			/* Retrieve the new window size: */
			windowPos.size[0]=event.xconfigure.width;
			windowPos.size[1]=event.xconfigure.height;
			
			/* Calculate the window's position on the screen: */
			Window child;
			XTranslateCoordinates(display,window,root,0,0,&windowPos.origin[0],&windowPos.origin[1],&child);
			break;
			}
		
		case ClientMessage:
			if(event.xclient.message_type==wmProtocolsAtom&&event.xclient.format==32&&(Atom)(event.xclient.data.l[0])==wmDeleteWindowAtom)
				closeRequested=true;
			break;
		}
	
	return closeRequested;
	}
