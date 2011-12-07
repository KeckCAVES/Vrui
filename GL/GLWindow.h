/***********************************************************************
GLWindow - Class to encapsulate details of the underlying window system
implementation from an application wishing to use OpenGL windows.
Copyright (c) 2001-2011 Oliver Kreylos

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

#ifndef GLWINDOW_INCLUDED
#define GLWINDOW_INCLUDED

#include <X11/X.h>
#include <GL/glx.h>
#include <Misc/CallbackList.h>

class GLWindow
	{
	/* Embedded classes: */
	public:
	struct WindowPos // Structure to store window positions and sizes
		{
		/* Elements: */
		public:
		int origin[2]; // (x, y) position of upper-left corner
		int size[2]; // Width and height of window
		
		/* Constructors and destructors: */
		WindowPos(void)
			{
			}
		WindowPos(int w,int h)
			{
			origin[0]=0;
			origin[1]=0;
			size[0]=w;
			size[1]=h;
			}
		WindowPos(int x,int y,int w,int h)
			{
			origin[0]=x;
			origin[1]=y;
			size[0]=w;
			size[1]=h;
			}
		WindowPos(const int sSize[2])
			{
			for(int i=0;i<2;++i)
				{
				origin[i]=0;
				size[i]=sSize[i];
				}
			}
		WindowPos(const int sOrigin[2],const int sSize[2])
			{
			for(int i=0;i<2;++i)
				{
				origin[i]=sOrigin[i];
				size[i]=sSize[i];
				}
			}
		};
	
	/* Elements: */
	private:
	bool privateConnection; // Flag if the connection to the X server is private
	Display* display; // Display this window belongs to
	int screen; // Screen this window belongs to
	Window root; // Handle of the screen's root window
	Colormap colorMap; // Colormap used in window
	Window window; // X window handle
	Atom wmProtocolsAtom,wmDeleteWindowAtom; // Atoms needed for window manager communication
	WindowPos windowPos; // Current position and size of output window
	bool fullscreen; // Flag if the window occupies the full screen (and has no decoration)
	GLXContext context; // GLX context handle
	Misc::CallbackList closeCallbacks; // List of callbacks to be called when the user attempts to close the window
	
	/* Private methods: */
	void initWindow(const char* windowName,int* visualProperties); // Common part of all constructors
	
	/* Constructors and destructors: */
	public:
	GLWindow(Display* sDisplay,int sScreen,const char* sWindowName,const WindowPos& sWindowPos,int* visualProperties =0); // Creates a window on an already open X display connection
	GLWindow(const char* sDisplayName,const char* sWindowName,const WindowPos& sWindowPos,int* visualProperties =0); // Opens a private connection to an X server and creates a window
	GLWindow(const char* sWindowName,const WindowPos& sWindowPos,int* visualProperties =0); // Same as above, but gets the display name from the environment
	~GLWindow(void); // Destroys the window and all associated resources
	
	/* Methods: */
	Display* getDisplay(void)
		{
		return display;
		}
	int getScreen(void) const
		{
		return screen;
		}
	Window getWindow(void)
		{
		return window;
		}
	const WindowPos& getWindowPos(void) const
		{
		return windowPos;
		}
	const int* getWindowOrigin(void) const
		{
		return windowPos.origin;
		}
	const int* getWindowSize(void) const
		{
		return windowPos.size;
		}
	int getWindowWidth(void) const
		{
		return windowPos.size[0];
		}
	int getWindowHeight(void) const
		{
		return windowPos.size[1];
		}
	WindowPos getRootWindowPos(void) const; // Returns the position and size of the root window containing this window
	double getScreenWidthMM(void) const; // Returns the physical width of the window's screen in mm
	double getScreenHeightMM(void) const; // Returns the physical height of the window's screen in mm
	Misc::CallbackList& getCloseCallbacks(void) // Returns the list of close callbacks
		{
		return closeCallbacks;
		}
	void makeFullscreen(void); // Turns the window into a "fake" fullscreen window by making it slightly larger than the root window
	void disableMouseEvents(void); // Tells the window to ignore mouse events (pointer motion, button click and release) from that point on
	void hideCursor(void); // Hides the cursor while inside the window
	void showCursor(void); // Resets the cursor to the one used by the parent window
	void setCursorPos(int newCursorX,int newCursorY); // Sets the cursor to the given position in window coordinates
	void redraw(void); // Signals a window that it should redraw itself (can be sent from outside window processing thread)
	void makeCurrent(void) // Sets the window's GL context as the current context
		{
		glXMakeCurrent(display,window,context);
		}
	void swapBuffers(void) // Swaps front and back buffer
		{
		glXSwapBuffers(display,window);
		}
	bool isEventForWindow(const XEvent& event) const // Returns true if the given event is intended for this window
		{
		return event.xany.window==window;
		}
	void processEvent(const XEvent& event); // Sends an X event to the window for processing
	};

#endif
