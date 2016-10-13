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

#include <GLMotif/FileSelectionHelper.h>

#include <Misc/SelfDestructPointer.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/MessageLogger.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/Button.h>

namespace GLMotif {

/************************************
Methods of class FileSelectionHelper:
************************************/

void FileSelectionHelper::closeDialog(FileSelectionHelper::CallbackState* cs)
	{
	/* Destroy the file selection dialog: */
	cs->dialog->close();
	cs->dialog=0;
	
	/* Delete the callback structure if it was a one-time deal: */
	if(cs->button==0)
		{
		CallbackState* pred=0;
		for(CallbackState* csPtr=head;csPtr!=0;pred=csPtr,csPtr=csPtr->succ)
			if(csPtr==cs)
				{
				if(pred!=0)
					pred->succ=csPtr->succ;
				else
					head=csPtr->succ;
				delete csPtr;
				break;
				}
		}
	}

void FileSelectionHelper::cancelCallback(FileSelectionDialog::CancelCallbackData* cbData,FileSelectionHelper::CallbackState* const& cs)
	{
	/* Just close the dialog: */
	closeDialog(cs);
	}

void FileSelectionHelper::saveOKCallback(FileSelectionDialog::OKCallbackData* cbData,FileSelectionHelper::CallbackState* const& cs)
	{
	try
		{
		/* Remember the selected directory for next time: */
		currentDirectory=cbData->selectedDirectory;
		
		/* Call the callback: */
		(*cs->callback)(cbData);
		}
	catch(std::runtime_error err)
		{
		/* Show an error message: */
		Misc::formattedUserError("%s: Could not write to file %s due to exception %s",cs->dialogTitle.c_str(),cbData->getSelectedPath().c_str(),err.what());
		}
	
	/* Close the dialog: */
	closeDialog(cs);
	}

void FileSelectionHelper::saveCallback(Misc::CallbackData* cbData,FileSelectionHelper::CallbackState* const& cs)
	{
	/* Bail out if there is already an open file selection dialog for this callback: */
	if(cs->dialog!=0)
		return;
	
	try
		{
		/* Create a uniquely-named file name in the current directory: */
		std::string fileName=currentDirectory->createNumberedFileName(defaultFileName.c_str(),4);
		
		/* Create a file selection dialog to select a file name: */
		Misc::SelfDestructPointer<FileSelectionDialog> saveDialog(new FileSelectionDialog(widgetManager,cs->dialogTitle.c_str(),currentDirectory,fileName.c_str(),extensionFilter.c_str()));
		saveDialog->getOKCallbacks().add(this,&FileSelectionHelper::saveOKCallback,cs);
		saveDialog->getCancelCallbacks().add(this,&FileSelectionHelper::cancelCallback,cs);
		
		/* Show the file selection dialog: */
		widgetManager->popupPrimaryWidget(saveDialog.getTarget());
		
		/* Remember that the dialog is currently open: */
		cs->dialog=saveDialog.releaseTarget();
		}
	catch(std::runtime_error err)
		{
		/* Show an error message: */
		Misc::formattedUserError("%s: Could not select file name due to exception %s",cs->dialogTitle.c_str(),err.what());
		}
	}

void FileSelectionHelper::loadOKCallback(FileSelectionDialog::OKCallbackData* cbData,FileSelectionHelper::CallbackState* const& cs)
	{
	try
		{
		/* Remember the selected directory for next time: */
		currentDirectory=cbData->selectedDirectory;
		
		/* Call the callback: */
		(*cs->callback)(cbData);
		}
	catch(std::runtime_error err)
		{
		/* Show an error message: */
		Misc::formattedUserError("%s: Could not read from file %s due to exception %s",cs->dialogTitle.c_str(),cbData->getSelectedPath().c_str(),err.what());
		}
	
	/* Close the dialog: */
	closeDialog(cs);
	}

void FileSelectionHelper::loadCallback(Misc::CallbackData* cbData,FileSelectionHelper::CallbackState* const& cs)
	{
	/* Bail out if there is already an open file selection dialog for this callback: */
	if(cs->dialog!=0)
		return;
	
	try
		{
		/* Create a file selection dialog to select a file name: */
		Misc::SelfDestructPointer<FileSelectionDialog> loadDialog(new FileSelectionDialog(widgetManager,cs->dialogTitle.c_str(),currentDirectory,extensionFilter.c_str()));
		loadDialog->getOKCallbacks().add(this,&FileSelectionHelper::loadOKCallback,cs);
		loadDialog->getCancelCallbacks().add(this,&FileSelectionHelper::cancelCallback,cs);
		
		/* Show the file selection dialog: */
		widgetManager->popupPrimaryWidget(loadDialog.getTarget());
		
		/* Remember that the dialog is currently open: */
		cs->dialog=loadDialog.releaseTarget();
		}
	catch(std::runtime_error err)
		{
		/* Show an error message: */
		Misc::formattedUserError("%s: Could not select file name due to exception %s",cs->dialogTitle.c_str(),err.what());
		}
	}

FileSelectionHelper::FileSelectionHelper(WidgetManager* sWidgetManager,const char* sDefaultFileName,const char* sExtensionFilter,IO::DirectoryPtr sCurrentDirectory)
	:widgetManager(sWidgetManager),
	 defaultFileName(sDefaultFileName),extensionFilter(sExtensionFilter),
	 currentDirectory(sCurrentDirectory),
	 head(0)
	{
	}

FileSelectionHelper::~FileSelectionHelper(void)
	{
	/* Close all still-open file selection dialogs and delete all callback state objects: */
	while(head!=0)
		{
		CallbackState* succ=head->succ;
		if(head->dialog!=0)
			head->dialog->close();
		delete head->callback;
		delete head;
		head=succ;
		}
	}

void FileSelectionHelper::setWidgetManager(WidgetManager* newWidgetManager)
	{
	widgetManager=newWidgetManager;
	}

void FileSelectionHelper::setCurrentDirectory(IO::DirectoryPtr newCurrentDirectory)
	{
	currentDirectory=newCurrentDirectory;
	}

void FileSelectionHelper::addSaveCallback(Button* button,FileSelectionHelper::FileSelectedCallback* callback)
	{
	/* Create a new callback state object: */
	CallbackState* cs=new CallbackState;
	cs->succ=head;
	head=cs;
	cs->dialogTitle=button->getString();
	cs->callback=callback;
	cs->save=true;
	cs->button=button;
	cs->dialog=0;
	
	/* Register the callback: */
	button->getSelectCallbacks().add(this,&FileSelectionHelper::saveCallback,cs);
	}

void FileSelectionHelper::addLoadCallback(Button* button,FileSelectionHelper::FileSelectedCallback* callback)
	{
	/* Create a new callback state object: */
	CallbackState* cs=new CallbackState;
	cs->succ=head;
	head=cs;
	cs->dialogTitle=button->getString();
	cs->callback=callback;
	cs->save=false;
	cs->button=button;
	cs->dialog=0;
	
	/* Register the callback: */
	button->getSelectCallbacks().add(this,&FileSelectionHelper::loadCallback,cs);
	}

void FileSelectionHelper::removeCallback(Button* button)
	{
	/* Find the callback structure for the given button: */
	CallbackState* pred=0;
	for(CallbackState* csPtr=head;csPtr!=0;pred=csPtr,csPtr=csPtr->succ)
		if(csPtr->button==button)
			{
			/* Remove the callback function from the button: */
			if(csPtr->save)
				csPtr->button->getSelectCallbacks().remove(this,&FileSelectionHelper::saveCallback,csPtr);
			else
				csPtr->button->getSelectCallbacks().remove(this,&FileSelectionHelper::loadCallback,csPtr);
			
			/* Close the file selection dialog should it still be open: */
			if(csPtr->dialog!=0)
				csPtr->dialog->close();
			
			/* Delete the callback state: */
			if(pred!=0)
				head=csPtr->succ;
			else
				pred->succ=csPtr->succ;
			delete csPtr;
			
			break;
			}
	}

void FileSelectionHelper::saveFile(const char* dialogTitle,FileSelectionHelper::FileSelectedCallback* callback)
	{
	/* Create a new callback state object: */
	CallbackState* cs=new CallbackState;
	cs->succ=0;
	cs->dialogTitle=dialogTitle;
	cs->callback=callback;
	cs->save=true;
	cs->button=0;
	cs->dialog=0;
	
	try
		{
		/* Create a uniquely-named file name in the current directory: */
		std::string fileName=currentDirectory->createNumberedFileName(defaultFileName.c_str(),4);
		
		/* Create a file selection dialog to select a file name: */
		Misc::SelfDestructPointer<FileSelectionDialog> saveDialog(new FileSelectionDialog(widgetManager,cs->dialogTitle.c_str(),currentDirectory,fileName.c_str(),extensionFilter.c_str()));
		saveDialog->getOKCallbacks().add(this,&FileSelectionHelper::saveOKCallback,cs);
		saveDialog->getCancelCallbacks().add(this,&FileSelectionHelper::cancelCallback,cs);
		
		/* Show the file selection dialog: */
		widgetManager->popupPrimaryWidget(saveDialog.getTarget());
		
		/* Remember that the dialog is currently open: */
		cs->dialog=saveDialog.releaseTarget();
		
		/* Add the one-shot callback structure to the list: */
		cs->succ=head;
		head=cs;
		}
	catch(std::runtime_error err)
		{
		/* Show an error message: */
		Misc::formattedUserError("%s: Could not select file name due to exception %s",cs->dialogTitle.c_str(),err.what());
		}
	}

void FileSelectionHelper::saveFile(const char* dialogTitle,const char* initialFileName,FileSelectionHelper::FileSelectedCallback* callback)
	{
	/* Create a new callback state object: */
	CallbackState* cs=new CallbackState;
	cs->succ=0;
	cs->dialogTitle=dialogTitle;
	cs->callback=callback;
	cs->save=true;
	cs->button=0;
	cs->dialog=0;
	
	try
		{
		/* Create a file selection dialog to select a file name: */
		Misc::SelfDestructPointer<FileSelectionDialog> saveDialog(new FileSelectionDialog(widgetManager,cs->dialogTitle.c_str(),currentDirectory,initialFileName,extensionFilter.c_str()));
		saveDialog->getOKCallbacks().add(this,&FileSelectionHelper::saveOKCallback,cs);
		saveDialog->getCancelCallbacks().add(this,&FileSelectionHelper::cancelCallback,cs);
		
		/* Show the file selection dialog: */
		widgetManager->popupPrimaryWidget(saveDialog.getTarget());
		
		/* Remember that the dialog is currently open: */
		cs->dialog=saveDialog.releaseTarget();
		
		/* Add the one-shot callback structure to the list: */
		cs->succ=head;
		head=cs;
		}
	catch(std::runtime_error err)
		{
		/* Show an error message: */
		Misc::formattedUserError("%s: Could not select file name due to exception %s",cs->dialogTitle.c_str(),err.what());
		}
	}

void FileSelectionHelper::loadFile(const char* dialogTitle,FileSelectionHelper::FileSelectedCallback* callback)
	{
	/* Create a new callback state object: */
	CallbackState* cs=new CallbackState;
	cs->succ=0;
	cs->dialogTitle=dialogTitle;
	cs->callback=callback;
	cs->save=false;
	cs->button=0;
	cs->dialog=0;
	
	try
		{
		/* Create a file selection dialog to select a file name: */
		Misc::SelfDestructPointer<FileSelectionDialog> loadDialog(new FileSelectionDialog(widgetManager,cs->dialogTitle.c_str(),currentDirectory,extensionFilter.c_str()));
		loadDialog->getOKCallbacks().add(this,&FileSelectionHelper::loadOKCallback,cs);
		loadDialog->getCancelCallbacks().add(this,&FileSelectionHelper::cancelCallback,cs);
		
		/* Show the file selection dialog: */
		widgetManager->popupPrimaryWidget(loadDialog.getTarget());
		
		/* Remember that the dialog is currently open: */
		cs->dialog=loadDialog.releaseTarget();
		
		/* Add the one-shot callback structure to the list: */
		cs->succ=head;
		head=cs;
		}
	catch(std::runtime_error err)
		{
		/* Show an error message: */
		Misc::formattedUserError("%s: Could not select file name due to exception %s",cs->dialogTitle.c_str(),err.what());
		}
	}

void FileSelectionHelper::closeDialogs(void)
	{
	/* Close all still-open file selection dialogs and delete all one-shot callback states: */
	CallbackState* pred=0;
	CallbackState* csPtr=head;
	while(csPtr!=0)
		{
		if(csPtr->dialog!=0)
			{
			/* Close the dialog: */
			csPtr->dialog->close();
			csPtr->dialog=0;
			}
		
		if(csPtr->button==0)
			{
			/* Delete the list item: */
			CallbackState* succ=csPtr->succ;
			if(pred!=0)
				pred->succ=succ;
			else
				head=succ;
			delete csPtr;
			csPtr=succ;
			}
		else
			{
			/* Go to the next item in the list: */
			pred=csPtr;
			csPtr=csPtr->succ;
			}
		}
	}

}
