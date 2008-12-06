/***********************************************************************
WidgetManager - Class to manage top-level GLMotif UI components and user
events.
Copyright (c) 2001-2008 Oliver Kreylos

This file is part of the GLMotif Widget Library (GLMotif).

The GLMotif Widget Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GLMotif Widget Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the GLMotif Widget Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef GLMOTIF_WIDGETMANAGER_INCLUDED
#define GLMOTIF_WIDGETMANAGER_INCLUDED

#include <vector>
#include <Geometry/OrthogonalTransformation.h>
#include <GLMotif/Types.h>

/* Forward declarations: */
namespace Misc {
class TimerEventScheduler;
}
class GLContextData;
namespace GLMotif {
class Event;
class StyleSheet;
class Widget;
}

namespace GLMotif {

class WidgetManager
	{
	/* Embedded classes: */
	public:
	typedef Geometry::OrthogonalTransformation<Scalar,3> Transformation;
	
	private:
	struct PopupBinding // Structure to bind top level widgets
		{
		/* Elements: */
		public:
		Widget* topLevelWidget; // Pointer to top level widget
		Transformation widgetToWorld; // Transformation from widget to world coordinates or owner widget's coordinates
		bool visible; // Flag if top level widget should be drawn
		PopupBinding* parent; // Pointer to the binding this binding is secondary to
		PopupBinding* pred; // Pointer to previous binding in same hierarchy level
		PopupBinding* succ; // Pointer to previous binding in same hierarchy level
		PopupBinding* firstSecondary; // Pointer to first secondary top level window
		
		/* Constructors and destructors: */
		PopupBinding(Widget* sTopLevelWidget,const Transformation& sWidgetToWorld,PopupBinding* sParent,PopupBinding* sSucc);
		~PopupBinding(void);
		
		/* Methods: */
		const PopupBinding* getSucc(void) const // Get the successor in a DFS-traversal of bindings
			{
			if(firstSecondary!=0)
				return firstSecondary;
			else if(succ!=0)
				return succ;
			else
				{
				PopupBinding* bPtr=parent;
				while(bPtr!=0&&bPtr->succ==0)
					bPtr=bPtr->parent;
				if(bPtr!=0)
					return bPtr->succ;
				else
					return 0;
				}
			}
		PopupBinding* getSucc(void) // Ditto
			{
			if(firstSecondary!=0)
				return firstSecondary;
			else if(succ!=0)
				return succ;
			else
				{
				PopupBinding* bPtr=parent;
				while(bPtr!=0&&bPtr->succ==0)
					bPtr=bPtr->parent;
				if(bPtr!=0)
					return bPtr->succ;
				else
					return 0;
				}
			}
		PopupBinding* findTopLevelWidget(const Point& point);
		PopupBinding* findTopLevelWidget(const Ray& ray);
		void draw(bool overlayWidgets,GLContextData& contextData) const;
		};
	
	/* Elements: */
	private:
	const StyleSheet* styleSheet; // The widget manager's style sheet
	Misc::TimerEventScheduler* timerEventScheduler; // Pointer to a scheduler for timer events managed by the OS/window system binding layer
	bool drawOverlayWidgets; // Flag whether widgets are drawn in an overlay layer on top of all other 3D imagery
	PopupBinding* firstBinding; // Pointer to first bound top level widget
	double time; // The time reported to widgets
	bool hardGrab; // Flag if the current pointer grab is a hard one
	Widget* pointerGrabWidget; // Pointer to the widget grabbing the input
	Transformation pointerGrabWidgetToWorld; // Transformation from the grabbing widget's coordinates to world coordinates
	bool inEventProcessing; // Flag if the widget manager is currently processing widget events
	std::vector<Widget*> deletionList; // List of widgets to be deleted at the next opportunity
	
	/* Private methods: */
	void deleteQueuedWidgets(void); // Deletes all widgets in the deletion list
	
	/* Constructors and destructors: */
	public:
	WidgetManager(void); // Constructs an empty widget manager
	~WidgetManager(void);
	
	/* Methods: */
	void setStyleSheet(const StyleSheet* newStyleSheet); // Sets the widget manager's style sheet
	const StyleSheet* getStyleSheet(void) const // Returns the widget manager's style sheet
		{
		return styleSheet;
		}
	void setTimerEventScheduler(Misc::TimerEventScheduler* newTimerEventScheduler); // Sets the widget manager's timer event scheduler
	const Misc::TimerEventScheduler* getTimerEventScheduler(void) const // Returns a pointer to the timer event scheduler
		{
		return timerEventScheduler;
		}
	Misc::TimerEventScheduler* getTimerEventScheduler(void) // Ditto
		{
		return timerEventScheduler;
		}
	void setDrawOverlayWidgets(bool newDrawOverlayWidgets); // Sets whether widgets are drawn in an overlay layer
	bool getDrawOverlayWidgets(void) const // Returns the current setting of the overlay flag
		{
		return drawOverlayWidgets;
		}
	void popupPrimaryWidget(Widget* topLevelWidget,const Transformation& widgetToWorld); // Pops up a primary top level widget
	void popupSecondaryWidget(Widget* owner,Widget* topLevelWidget,const Vector& offset); // Pops up a secondary top level widget
	void popdownWidget(Widget* widget); // Pops down the top level widget containing the given widget
	void show(Widget* widget); // Shows the top level widget containing the given widget
	void hide(Widget* widget); // Hides the top level widget containing the given widget
	bool isManaged(const Widget* widget) const; // Returns true if the top level widget containing the given widget is popped up
	bool isVisible(const Widget* widget) const; // Returns true if the top level widget containing the given widget is popped up and visible
	Widget* findPrimaryWidget(const Point& point); // Finds the primary top level widget whose descendants contain the given point
	Widget* findPrimaryWidget(const Ray& ray); // Finds the primary top level widget whose descendants are intersected by the given ray
	Transformation calcWidgetTransformation(const Widget* widget) const; // Returns the transformation associated with a widget's root
	void setPrimaryWidgetTransformation(Widget* widget,const Transformation& newWidgetToWorld); // Sets the transformation of a primary top level widget
	void deleteWidget(Widget* widget); // Method to delete a widget that is safe to call from within a callback belonging to the widget
	void setTime(double newTime); // Sets the widget manager's time
	double getTime(void) const // Returns the current time
		{
		return time;
		}
	void draw(GLContextData& contextData) const;
	bool pointerButtonDown(Event& event); // Handles a button down event
	bool pointerButtonUp(Event& event); // Handles a button up event
	bool pointerMotion(Event& event); // Handles a pointer motion event
	void grabPointer(Widget* widget); // Allows a widget to grab all pointer events
	void releasePointer(Widget* widget); // Releases a pointer grab
	bool isPointerGrabbed(void) const // Checks for an active pointer grab (hard or soft)
		{
		return pointerGrabWidget!=0;
		}
	};

}

#endif
