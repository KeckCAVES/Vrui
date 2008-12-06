/***********************************************************************
CallbackList - Class for lists of callback functions associated with
certain events. Uses new-style templatized callback mechanism and offers
backwards compatibility for traditional C-style callbacks.
Copyright (c) 2000-2005 Oliver Kreylos

This file is part of the Miscellaneous Support Library (Misc).

The Miscellaneous Support Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Miscellaneous Support Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Miscellaneous Support Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef MISC_CALLBACKLIST_INCLUDED
#define MISC_CALLBACKLIST_INCLUDED

#include <typeinfo>
#include <Misc/CallbackData.h>

namespace Misc {

class CallbackList // Class for lists of callbacks
	{
	/* Embedded classes: */
	private:
	
	/* Abstract base class for callback list items: */
	class CallbackListItem
		{
		friend class CallbackList;
		
		/* Elements: */
		private:
		CallbackListItem* succ; // Pointer to next item in callback list
		
		/* Constructors and destructors: */
		public:
		CallbackListItem(void); // Creates an empty singleton callback list item
		virtual ~CallbackListItem(void); // Virtual destructor
		
		/* Methods: */
		virtual bool operator==(const CallbackListItem& other) const =0; // Virtual method to compare callbacks
		virtual void call(CallbackData* callbackData) const =0; // Virtual method to invoke callback
		};
	
	/* Class to call C functions with an additional void* parameter (traditional C-style callback): */
	class FunctionCallback:public CallbackListItem
		{
		/* Embedded classes: */
		public:
		typedef void (*CallbackFunction)(CallbackData*,void*); // Type of called callback function
		
		/* Elements: */
		private:
		CallbackFunction callbackFunction; // Pointer to callback function
		void* userData; // Additional callback function parameter
		
		/* Constructors and destructors: */
		public:
		FunctionCallback(CallbackFunction sCallbackFunction,void* sUserData); // Creates callback for given function with given additional parameter
		
		/* Methods: */
		virtual bool operator==(const CallbackListItem& other) const;
		virtual void call(CallbackData* cbData) const;
		};
	
	/* Class to call arbitrary methods on objects of arbitrary type: */
	template <class CallbackClassParam>
	class MethodCallback:public CallbackListItem
		{
		/* Embedded classes: */
		public:
		typedef CallbackClassParam CallbackClass; // Class of called objects
		typedef void (CallbackClass::*CallbackMethod)(CallbackData*); // Type of called callback method
		
		/* Elements: */
		private:
		CallbackClass* callbackObject; // Pointer to callback object
		CallbackMethod callbackMethod; // Pointer to callback method
		
		/* Constructors and destructors: */
		public:
		MethodCallback(CallbackClass* sCallbackObject,CallbackMethod sCallbackMethod) // Creates callback for given method on given object
			:callbackObject(sCallbackObject),callbackMethod(sCallbackMethod)
			{
			}
		
		/* Methods: */
		virtual bool operator==(const CallbackListItem& other) const
			{
			if(typeid(other)!=typeid(MethodCallback))
				return false;
			const MethodCallback* other2=static_cast<const MethodCallback*>(&other);
			return callbackObject==other2->callbackObject&&callbackMethod==other2->callbackMethod;
			}
		virtual void call(CallbackData* callbackData) const
			{
			/* Call the callback method on the callback object: */
			(callbackObject->*callbackMethod)(callbackData);
			}
		};
	
	/* Class to call arbitrary methods taking a parameter derived from CallbackData on objects of arbitrary type: */
	template <class CallbackClassParam,class DerivedCallbackDataParam>
	class MethodCastCallback:public CallbackListItem
		{
		/* Embedded classes: */
		public:
		typedef CallbackClassParam CallbackClass; // Class of called objects
		typedef DerivedCallbackDataParam DerivedCallbackData; // Class of callback data (must be derived from CallbackData)
		typedef void (CallbackClass::*CallbackMethod)(DerivedCallbackData*); // Type of called callback method
		
		/* Elements: */
		private:
		CallbackClass* callbackObject; // Pointer to callback object
		CallbackMethod callbackMethod; // Pointer to callback method
		
		/* Constructors and destructors: */
		public:
		MethodCastCallback(CallbackClass* sCallbackObject,CallbackMethod sCallbackMethod) // Creates callback for given method on given object
			:callbackObject(sCallbackObject),callbackMethod(sCallbackMethod)
			{
			}
		
		/* Methods: */
		virtual bool operator==(const CallbackListItem& other) const
			{
			if(typeid(other)!=typeid(MethodCastCallback))
				return false;
			const MethodCastCallback* other2=static_cast<const MethodCastCallback*>(&other);
			return callbackObject==other2->callbackObject&&callbackMethod==other2->callbackMethod;
			}
		virtual void call(CallbackData* callbackData) const
			{
			/* Call the callback method on the callback object with downcasted callback data: */
			(callbackObject->*callbackMethod)(static_cast<DerivedCallbackData*>(callbackData));
			}
		};
	
	/* Elements: */
	private:
	CallbackListItem* head; // Pointer to first callback in list
	CallbackListItem* tail; // Pointer to last callback in list
	mutable bool interruptRequested; // Flag that the current call() operation is to be aborted after the current callback
	
	/* Private methods: */
	void addCli(CallbackListItem* newCli); // Adds a new callback list item to the back of the list
	void addCliToFront(CallbackListItem* newCli); // Adds a new callback list item to the front of the list
	void removeCli(const CallbackListItem& removeCli); // Removes the first callback list item equal to the given one from the list
	
	/* Constructors and destructors: */
	public:
	CallbackList(void); // Creates an empty callback list
	private:
	CallbackList(const CallbackList& source); // Prohibit copy constructor
	CallbackList& operator=(const CallbackList& source); // Prohibit assignment operator
	public:
	~CallbackList(void); // Destroys the callback list and all its callbacks
	
	/* Callback list creation/manipulation methods: */
	
	/* Interface for traditional C-style callbacks: */
	void add(CallbackType newCallbackFunction,void* newUserData) // Adds a callback to the end of the list
		{
		addCli(new FunctionCallback(newCallbackFunction,newUserData));
		}
	void addToFront(CallbackType newCallbackFunction,void* newUserData) // Adds a callback to the front of the list
		{
		addCliToFront(new FunctionCallback(newCallbackFunction,newUserData));
		}
	void remove(CallbackType removeCallbackFunction,void* removeUserData) // Removes the first matching callback from the list
		{
		removeCli(FunctionCallback(removeCallbackFunction,removeUserData));
		}
	
	/* Interface for method callbacks: */
	template <class CallbackClassParam>
	void add(CallbackClassParam* newCallbackObject,void (CallbackClassParam::*newCallbackMethod)(CallbackData*)) // Adds a callback to the end of the list
		{
		addCli(new MethodCallback<CallbackClassParam>(newCallbackObject,newCallbackMethod));
		}
	template <class CallbackClassParam>
	void addToFront(CallbackClassParam* newCallbackObject,void (CallbackClassParam::*newCallbackMethod)(CallbackData*)) // Adds a callback to the front of the list
		{
		addCliToFront(new MethodCallback<CallbackClassParam>(newCallbackObject,newCallbackMethod));
		}
	template <class CallbackClassParam>
	void remove(CallbackClassParam* removeCallbackObject,void (CallbackClassParam::*removeCallbackMethod)(CallbackData*)) // Removes the first matching callback from the list
		{
		removeCli(MethodCallback<CallbackClassParam>(removeCallbackObject,removeCallbackMethod));
		}
	
	/* Interface for method callbacks with automatic callback data downcast: */
	template <class CallbackClassParam,class DerivedCallbackDataParam>
	void add(CallbackClassParam* newCallbackObject,void (CallbackClassParam::*newCallbackMethod)(DerivedCallbackDataParam*)) // Adds a callback to the end of the list
		{
		addCli(new MethodCastCallback<CallbackClassParam,DerivedCallbackDataParam>(newCallbackObject,newCallbackMethod));
		}
	template <class CallbackClassParam,class DerivedCallbackDataParam>
	void addToFront(CallbackClassParam* newCallbackObject,void (CallbackClassParam::*newCallbackMethod)(DerivedCallbackDataParam*)) // Adds a callback to the front of the list
		{
		addCliToFront(new MethodCastCallback<CallbackClassParam,DerivedCallbackDataParam>(newCallbackObject,newCallbackMethod));
		}
	template <class CallbackClassParam,class DerivedCallbackDataParam>
	void remove(CallbackClassParam* removeCallbackObject,void (CallbackClassParam::*removeCallbackMethod)(DerivedCallbackDataParam*)) // Removes the first matching callback from the list
		{
		removeCli(MethodCastCallback<CallbackClassParam,DerivedCallbackDataParam>(removeCallbackObject,removeCallbackMethod));
		}
	
	/* Callback list calling interface: */
	void call(CallbackData* callbackData) const; // Calls all callbacks in the list
	void requestInterrupt(void) const; // Allows a callback to request interrupting callback processing
	};

}

#endif
