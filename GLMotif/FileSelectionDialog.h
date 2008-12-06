/***********************************************************************
FileSelectionDialog - A popup window to select a file name.
Copyright (c) 2008 Oliver Kreylos

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

#ifndef GLMOTIF_FILESELECTIONDIALOG_INCLUDED
#define GLMOTIF_FILESELECTIONDIALOG_INCLUDED

#include <string>
#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <GLMotif/Button.h>
#include <GLMotif/ListBox.h>
#include <GLMotif/DropdownBox.h>
#include <GLMotif/PopupWindow.h>

/* Forward declarations: */
namespace Comm {
class MulticastPipe;
}
namespace GLMotif {
class RowColumn;
class ScrolledListBox;
}

namespace GLMotif {

class FileSelectionDialog:public PopupWindow
	{
	/* Embedded classes: */
	public:
	class CallbackData:public Misc::CallbackData // Base class for file selection dialog callbacks
		{
		/* Elements: */
		public:
		FileSelectionDialog* fileSelectionDialog; // Pointer to the file selection dialog that caused the event
		
		/* Constructors and destructors: */
		CallbackData(FileSelectionDialog* sFileSelectionDialog)
			:fileSelectionDialog(sFileSelectionDialog)
			{
			}
		};
	
	class OKCallbackData:public CallbackData // Callback data when the OK button was clicked, or a file name was double-clicked
		{
		/* Elements: */
		public:
		std::string selectedFileName; // Fully qualified name of selected file
		
		/* Constructors and destructors: */
		OKCallbackData(FileSelectionDialog* sFileSelectionDialog,std::string sSelectedFileName)
			:CallbackData(sFileSelectionDialog),
			 selectedFileName(sSelectedFileName)
			{
			}
		};
	
	class CancelCallbackData:public CallbackData // Callback data when the cancel button was clicked
		{
		/* Constructors and destructors: */
		public:
		CancelCallbackData(FileSelectionDialog* sFileSelectionDialog)
			:CallbackData(sFileSelectionDialog)
			{
			}
		};
	
	/* Elements: */
	private:
	Comm::MulticastPipe* pipe; // A multicast pipe to synchronize instances of the file selection dialog across a cluster; file selection dialog takes over ownership from caller
	const char* fileNameFilters; // Current filter expression for file names; semicolon-separated list of allowed extensions
	RowColumn* pathButtonBox; // Box containing the path component buttons
	int selectedPathButton; // Index of the currently selected path button; determines the displayed directory
	ScrolledListBox* fileList; // Scrolled list box containing all directories and matching files in the current directory
	DropdownBox* filterList; // Drop down box containing the selectable file name filters
	Misc::CallbackList okCallbacks; // Callbacks to be called when the OK button is selected, or a file name is double-clicked
	Misc::CallbackList cancelCallbacks; // Callbacks to be called when the cancel button is selected
	
	/* Private methods: */
	std::string getCurrentPath(void) const; // Constructs the full path name of the currently displayed directory
	bool readDirectory(void); // Reads all directories and files from the selected directory into the list box
	void setSelectedPathButton(int newSelectedPathButton); // Changes the selected path button
	void pathButtonSelectedCallback(Button::SelectCallbackData* cbData); // Callback called when one of the path buttons is selected
	void listItemSelectedCallback(ListBox::ItemSelectedCallbackData* cbData); // Callback when a list item gets double-clicked
	void filterListValueChangedCallback(DropdownBox::ValueChangedCallbackData* cbData); // Callback when the selected file name filter changes
	void okButtonSelectedCallback(Misc::CallbackData* cbData); // Callback called when the OK button is pressed
	void cancelButtonSelectedCallback(Misc::CallbackData* cbData); // Callback called when the Cancel button is pressed
	
	/* Constructors and destructors: */
	public:
	FileSelectionDialog(WidgetManager* widgetManager,const char* titleString,const char* initialDirectory,const char* sFileNameFilters,Comm::MulticastPipe* sPipe =0); // Creates a file selection dialog with the given title, initial directory, and file name filter; starts from current directory if initialDirectory is 0
	virtual ~FileSelectionDialog(void);
	
	/* Methods: */
	void addFileNameFilters(const char* newFileNameFilters); // Adds another extension list to the list of selectable filters
	Misc::CallbackList& getOKCallbacks(void) // Returns the list of OK callbacks
		{
		return okCallbacks;
		}
	Misc::CallbackList& getCancelCallbacks(void) // Returns the list of cancel callbacks
		{
		return cancelCallbacks;
		}
	};

}

#endif
