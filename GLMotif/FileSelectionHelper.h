/***********************************************************************
FileSelectionHelper - Helper class to simplify managing file selection
dialogs and their callbacks.
Copyright (c) 2013-2015 Oliver Kreylos

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

#ifndef GLMOTIF_FILESELECTIONHELPER_INCLUDED
#define GLMOTIF_FILESELECTIONHELPER_INCLUDED

#include <string>
#include <Misc/FunctionCalls.h>
#include <IO/Directory.h>
#include <GLMotif/FileSelectionDialog.h>

/* Forward declarations: */
namespace Misc {
class CallbackData;
}
namespace GLMotif {
class WidgetManager;
class Button;
}

namespace GLMotif {

class FileSelectionHelper
	{
	/* Embedded classes: */
	public:
	typedef Misc::FunctionCall<FileSelectionDialog::OKCallbackData*> FileSelectedCallback; // Type for callback functions to be called when a file was selected
	
	private:
	struct CallbackState // Structure holding additional per-callback state
		{
		/* Elements: */
		public:
		CallbackState* succ; // Pointer to create a list of callback state objects
		std::string dialogTitle; // Title for the file selection dialog
		FileSelectedCallback* callback; // Callback to call when a file was selected
		bool save; // Flag whether it is a load or a save callback
		Button* button; // Button with which this callback is associated; 0 for one-shot temporary callbacks
		FileSelectionDialog* dialog; // Pointer to the file selection dialog currently open for this callback
		};
	
	/* Elements: */
	private:
	WidgetManager* widgetManager; // Pointer to a widget manager to open new file selection dialogs
	std::string defaultFileName; // Initial file name for "save"-style dialogs
	std::string extensionFilter; // File extension filter for the file selection dialog
	IO::DirectoryPtr currentDirectory; // Initial directory for the next file selection dialog
	CallbackState* head; // Head of the callback state object list
	
	/* Private methods: */
	void closeDialog(CallbackState* cs); // Closes the file selection dialog for the given callback and cleans up after it
	void cancelCallback(FileSelectionDialog::CancelCallbackData* cbData,CallbackState* const& cs); // Callback when a file selection dialog is closed
	void saveOKCallback(FileSelectionDialog::OKCallbackData* cbData,CallbackState* const& cs); // Callback when the OK button in a "save file" dialog is pressed
	void saveCallback(Misc::CallbackData* cbData,CallbackState* const& cs); // Callback when a "save" button is pressed
	void loadOKCallback(FileSelectionDialog::OKCallbackData* cbData,CallbackState* const& cs); // Callback when the OK button in a "load file" dialog is pressed
	void loadCallback(Misc::CallbackData* cbData,CallbackState* const& cs); // Callback when a "load" button is pressed
	
	/* Constructors and destructors: */
	public:
	FileSelectionHelper(WidgetManager* sWidgetManager,const char* sDefaultFileName,const char* sExtensionFilter,IO::DirectoryPtr sCurrentDirectory);
	~FileSelectionHelper(void);
	
	/* Methods: */
	void setWidgetManager(WidgetManager* newWidgetManager); // Sets a new widget manager
	WidgetManager* getWidgetManager(void) const // Returns the widget manager used by this file selection helper
		{
		return widgetManager;
		}
	IO::DirectoryPtr getCurrentDirectory(void) const // Returns the most recently selected directory
		{
		return currentDirectory;
		}
	void setCurrentDirectory(IO::DirectoryPtr newCurrentDirectory); // Sets the initial directory for the next file selection dialog
	void addSaveCallback(Button* button,FileSelectedCallback* callback); // Adds a "save file" callback to the given button's selection callback list
	template <class CalleeParam>
	void addSaveCallback(Button* button,CalleeParam* callee,void (CalleeParam::*calleeMethod)(FileSelectionDialog::OKCallbackData*)) // Convenience method for same
		{
		addSaveCallback(button,Misc::createFunctionCall(callee,calleeMethod));
		}
	void addLoadCallback(Button* button,FileSelectedCallback* callback); // Adds a "load file" callback to the given button's selection callback list
	template <class CalleeParam>
	void addLoadCallback(Button* button,CalleeParam* callee,void (CalleeParam::*calleeMethod)(FileSelectionDialog::OKCallbackData*)) // Convenience method for same
		{
		addLoadCallback(button,Misc::createFunctionCall(callee,calleeMethod));
		}
	void removeCallback(Button* button); // Removes a callback that was associated with the given button
	void saveFile(const char* dialogTitle,FileSelectedCallback* callback); // Immediately shows a "save file" dialog
	template <class CalleeParam>
	void saveFile(const char* dialogTitle,CalleeParam* callee,void (CalleeParam::*calleeMethod)(FileSelectionDialog::OKCallbackData*)) // Convenience method for same
		{
		saveFile(dialogTitle,Misc::createFunctionCall(callee,calleeMethod));
		}
	void saveFile(const char* dialogTitle,const char* initialFileName,FileSelectedCallback* callback); // Immediately shows a "save file" dialog with the given file name
	template <class CalleeParam>
	void saveFile(const char* dialogTitle,const char* initialFileName,CalleeParam* callee,void (CalleeParam::*calleeMethod)(FileSelectionDialog::OKCallbackData*)) // Convenience method for same
		{
		saveFile(dialogTitle,initialFileName,Misc::createFunctionCall(callee,calleeMethod));
		}
	void loadFile(const char* dialogTitle,FileSelectedCallback* callback); // Immediately shows a "load file" dialog
	template <class CalleeParam>
	void loadFile(const char* dialogTitle,CalleeParam* callee,void (CalleeParam::*calleeMethod)(FileSelectionDialog::OKCallbackData*)) // Convenience method for same
		{
		loadFile(dialogTitle,Misc::createFunctionCall(callee,calleeMethod));
		}
	void closeDialogs(void); // Closes all still-open file selection dialogs
	};

}

#endif
