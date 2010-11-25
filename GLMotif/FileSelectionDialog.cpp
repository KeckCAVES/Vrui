/***********************************************************************
FileSelectionDialog - A popup window to select a file name.
Copyright (c) 2008-2010 Oliver Kreylos

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

#include <iostream>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <algorithm>
#include <Misc/StringMarshaller.h>
#include <Misc/GetCurrentDirectory.h>
#include <Misc/FileTests.h>
#include <Misc/Directory.h>
#include <Comm/MulticastPipe.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/Blind.h>
#include <GLMotif/ListBox.h>
#include <GLMotif/ScrolledListBox.h>
#include <GLMotif/RowColumn.h>

#include <GLMotif/FileSelectionDialog.h>

namespace GLMotif {

namespace {

/**************
Helper classes:
**************/

class StringCompare // Class to compare strings according to case-insensitive lexicographical order
	{
	/* Methods: */
	public:
	bool operator()(const std::string& s1,const std::string& s2) const
		{
		/* Find the first different character in the common prefix of both strings: */
		int cmp=0;
		int caseCmp=0;
		std::string::const_iterator s1It,s2It;
		for(s1It=s1.begin(),s2It=s2.begin();s1It!=s1.end()&&s2It!=s2.end()&&cmp==0;++s1It,++s2It)
			{
			cmp=toupper(*s1It)-toupper(*s2It);
			if(caseCmp==0)
				caseCmp=*s1It-*s2It;
			}
		
		/* Check if the common prefix is identical: */
		if(cmp==0)
			{
			/* Check if the first string is shorter: */
			if(s2It!=s2.end())
				cmp=-1;
			else if(s1It!=s1.end())
				cmp=1;
			}
		
		/* If the strings are identical, use the case-dependent result: */
		if(cmp==0)
			cmp=caseCmp;
		
		return cmp<0;
		}
	};

}

/************************************
Methods of class FileSelectionDialog:
************************************/

std::string FileSelectionDialog::getCurrentPath(void) const
	{
	/* Assemble a fully-qualified path name by traversing the path buttons in order: */
	std::string fullPath;
	for(int i=0;i<=selectedPathButton;++i)
		{
		/* Append the button's label to the full path name: */
		if(i>1)
			fullPath.push_back('/');
		fullPath.append(static_cast<Button*>(pathButtonBox->getChild(i))->getString());
		}
	
	return fullPath;
	}

void FileSelectionDialog::updateFileNameField(void)
	{
	/* Get the current path name: */
	std::string fullName=getCurrentPath();
	
	fullName.push_back('/');
	int selectedFileName=fileList->getListBox()->getSelectedItem();
	if(selectedFileName>=0)
		{
		/* Append the current file name: */
		fullName.append(fileList->getListBox()->getItem(selectedFileName));
		}
	
	/* Set the file name field's value: */
	fileNameField->setString(fullName.c_str());
	}

bool FileSelectionDialog::readDirectory(void)
	{
	if(pipe==0||pipe->isMaster())
		{
		try
			{
			/* Open the currently selected directory: */
			Misc::Directory directory(getCurrentPath().c_str());
			
			/* Read all directory entries: */
			std::vector<std::string> directories;
			std::vector<std::string> files;
			while(!directory.eod())
				{
				/* Check for hidden entries: */
				const char* entryName=directory.getEntryName();
				if(entryName[0]!='.')
					{
					/* Determine the type of the directory entry: */
					Misc::PathType pt=directory.getEntryType();
					if(pt==Misc::PATHTYPE_DIRECTORY)
						{
						/* Store a directory name: */
						std::string dirName(entryName);
						dirName.push_back('/');
						directories.push_back(dirName);
						}
					else if(pt==Misc::PATHTYPE_FILE)
						{
						/* Check if the file name matches any of the supplied extensions: */
						bool passesFilters=true;
						if(fileNameFilters!=0)
							{
							/* Find the file name's extension: */
							const char* extPtr="";
							for(const char* enPtr=entryName;*enPtr!='\0';++enPtr)
								if(*enPtr=='.')
									extPtr=enPtr;
							
							/* Match against the list of allowed extensions: */
							passesFilters=false;
							const char* filterPtr=fileNameFilters;
							while(*filterPtr!='\0'&&!passesFilters)
								{
								/* Extract the next extension: */
								const char* extStart=filterPtr;
								for(;*filterPtr!='\0'&&*filterPtr!=';';++filterPtr)
									;
								
								/* See if it matches: */
								passesFilters=int(strlen(extPtr))==filterPtr-extStart&&memcmp(extPtr,extStart,filterPtr-extStart)==0;
								
								/* Skip the separator: */
								if(*filterPtr==';')
									++filterPtr;
								}
							}
						
						if(passesFilters)
							{
							/* Store a file name: */
							files.push_back(entryName);
							}
						}
					}
				
				/* Go to the next directory entry: */
				directory.readNextEntry();
				}
			
			/* Sort the directory and file names separately: */
			StringCompare sc;
			std::sort(directories.begin(),directories.end(),sc);
			std::sort(files.begin(),files.end(),sc);
			
			if(pipe!=0)
				{
				/* Send a success code to the slave nodes: */
				pipe->write<int>(1);
				
				/* Send the directory names to the slave nodes: */
				for(std::vector<std::string>::const_iterator dIt=directories.begin();dIt!=directories.end();++dIt)
					{
					/* Write the string: */
					Misc::writeCppString(*dIt,*pipe);
					}
				
				/* Send the file names to the slave nodes: */
				for(std::vector<std::string>::const_iterator fIt=files.begin();fIt!=files.end();++fIt)
					{
					/* Write the string: */
					Misc::writeCppString(*fIt,*pipe);
					}
				
				/* Send the list terminator message: */
				pipe->write<unsigned int>(~0x0U);
				pipe->finishMessage();
				}
			
			/* Copy all names into the list box: */
			fileList->getListBox()->clear();
			for(std::vector<std::string>::const_iterator dIt=directories.begin();dIt!=directories.end();++dIt)
				fileList->getListBox()->addItem(dIt->c_str());
			for(std::vector<std::string>::const_iterator fIt=files.begin();fIt!=files.end();++fIt)
				fileList->getListBox()->addItem(fIt->c_str());
			}
		catch(Misc::Directory::OpenError)
			{
			if(pipe!=0)
				{
				/* Send an error code to the slave nodes: */
				pipe->write<int>(0);
				pipe->finishMessage();
				}
			return false;
			}
		}
	else
		{
		/* Read the status flag from the master node: */
		if(pipe->read<int>())
			{
			/* Clear the list box: */
			fileList->getListBox()->clear();
			
			/* Read the directory and file names: */
			unsigned int length;
			unsigned int bufferSize=128;
			char* buffer=new char[bufferSize];
			while((length=pipe->read<unsigned int>())!=~0x0U)
				{
				/* Make sure the buffer is big enough: */
				if(bufferSize<length+1)
					{
					delete[] buffer;
					bufferSize=(length+1)*3/2;
					buffer=new char[bufferSize];
					}
				
				/* Read the string and add it to the list box: */
				pipe->read<char>(buffer,length);
				buffer[length]='\0';
				fileList->getListBox()->addItem(buffer);
				}
			delete[] buffer;
			}
		else
			return false;
		}
	
	return true;
	}

void FileSelectionDialog::setSelectedPathButton(int newSelectedPathButton)
	{
	/* Get the style sheet: */
	const StyleSheet& ss=*getManager()->getStyleSheet();
	
	if(selectedPathButton>=0)
		{
		/* Un-"arm" the previously selected path button: */
		Button* oldButton=static_cast<Button*>(pathButtonBox->getChild(selectedPathButton));
		oldButton->setBorderType(Widget::RAISED);
		oldButton->setBackgroundColor(ss.bgColor);
		oldButton->setArmedBackgroundColor(ss.buttonArmedBackgroundColor);
		}
	
	/* Set the index of the selected path button: */
	selectedPathButton=newSelectedPathButton;
	
	/* "Arm" the new selected path button: */
	Button* newButton=static_cast<Button*>(pathButtonBox->getChild(selectedPathButton));
	newButton->setBorderType(Widget::LOWERED);
	newButton->setBackgroundColor(ss.buttonArmedBackgroundColor);
	newButton->setArmedBackgroundColor(ss.bgColor);
	
	/* Read the directory corresponding to the path button: */
	readDirectory();
	
	/* Update the file name field: */
	updateFileNameField();
	}

void FileSelectionDialog::pathButtonSelectedCallback(Button::SelectCallbackData* cbData)
	{
	/* Change the selected path button: */
	setSelectedPathButton(pathButtonBox->getChildIndex(cbData->button));
	}

void FileSelectionDialog::fileNameFieldValueChangedCallback(TextField::ValueChangedCallbackData* cbData)
	{
	/* Find the common prefix between the new full file name and the path button box: */
	const char* fnPtr=fileNameField->getString();
	
	}

void FileSelectionDialog::listValueChangedCallback(ListBox::ValueChangedCallbackData* cbData)
	{
	/* Update the file name field: */
	updateFileNameField();
	}

void FileSelectionDialog::listItemSelectedCallback(ListBox::ItemSelectedCallbackData* cbData)
	{
	/* Get the selected list item's name: */
	std::string item=cbData->listBox->getItem(cbData->selectedItem);
	
	/* Check if it's a file or directory: */
	if(item[item.size()-1]=='/')
		{
		/* Remove all path buttons after the selected one: */
		for(GLint i=pathButtonBox->getNumColumns()-1;i>selectedPathButton;--i)
			pathButtonBox->removeWidgets(i);
		
		/* Add a new path button for the selected directory: */
		item.erase(item.end()-1);
		char pathButtonName[20];
		snprintf(pathButtonName,sizeof(pathButtonName),"PathButton%04d",selectedPathButton);
		Button* pathButton=new Button(pathButtonName,pathButtonBox,item.c_str());
		pathButton->setBorderWidth(pathButton->getBorderWidth()*0.5f);
		pathButton->getSelectCallbacks().add(this,&FileSelectionDialog::pathButtonSelectedCallback);
		
		/* Select the new path button: */
		setSelectedPathButton(selectedPathButton+1);
		}
	else
		{
		/* Assemble the fully qualified name of the selected file: */
		std::string selectedFileName=getCurrentPath();
		selectedFileName.push_back('/');
		selectedFileName+=item;
		
		/* Call the OK callbacks: */
		OKCallbackData cbData(this,selectedFileName);
		okCallbacks.call(&cbData);
		}
	}

void FileSelectionDialog::filterListValueChangedCallback(DropdownBox::ValueChangedCallbackData* cbData)
	{
	/* Set the current file name filters to the new selected item: */
	fileNameFilters=cbData->newSelectedItem>0?cbData->dropdownBox->getItem(cbData->newSelectedItem):0;
	
	/* Re-read the current directory: */
	readDirectory();
	}

void FileSelectionDialog::okButtonSelectedCallback(Misc::CallbackData* cbData)
	{
	/* Check if there is a selected directory entry: */
	int selectedItem=fileList->getListBox()->getSelectedItem();
	if(selectedItem>=0)
		{
		/* Get the selected list item's name: */
		std::string item=fileList->getListBox()->getItem(selectedItem);
		
		/* Check if it's a directory or a file: */
		if(item[item.size()-1]=='/')
			{
			/* Remove all path buttons after the selected one: */
			for(GLint i=pathButtonBox->getNumColumns()-1;i>selectedPathButton;--i)
				pathButtonBox->removeWidgets(i);
			
			/* Add a new path button for the selected directory: */
			item.erase(item.end()-1);
			char pathButtonName[20];
			snprintf(pathButtonName,sizeof(pathButtonName),"PathButton%04d",selectedPathButton);
			Button* pathButton=new Button(pathButtonName,pathButtonBox,item.c_str());
			pathButton->setBorderWidth(pathButton->getBorderWidth()*0.5f);
			pathButton->getSelectCallbacks().add(this,&FileSelectionDialog::pathButtonSelectedCallback);
			
			/* Select the new path button: */
			setSelectedPathButton(selectedPathButton+1);
			}
		else
			{
			/* Assemble the fully qualified name of the selected file: */
			std::string selectedFileName=getCurrentPath();
			selectedFileName.push_back('/');
			selectedFileName+=item;
			
			/* Call the OK callbacks: */
			OKCallbackData cbData(this,selectedFileName);
			okCallbacks.call(&cbData);
			}
		}
	}

void FileSelectionDialog::cancelButtonSelectedCallback(Misc::CallbackData* cbData)
	{
	/* Call the cancel callbacks: */
	CancelCallbackData myCbData(this);
	cancelCallbacks.call(&myCbData);
	}

FileSelectionDialog::FileSelectionDialog(WidgetManager* widgetManager,const char* titleString,const char* initialDirectory,const char* sFileNameFilters,Comm::MulticastPipe* sPipe)
	:PopupWindow("FileSelectionDialogPopup",widgetManager,titleString),
	 pipe(sPipe),
	 fileNameFilters(sFileNameFilters),
	 pathButtonBox(0),selectedPathButton(-1),
	 fileNameField(0),
	 fileList(0),filterList(0)
	{
	/* Add a close button: */
	setCloseButton(true);
	getCloseCallbacks().add(this,&FileSelectionDialog::cancelButtonSelectedCallback);
	
	/* Create the file selection dialog: */
	RowColumn* fileSelectionDialog=new RowColumn("FileSelectionDialog",this,false);
	fileSelectionDialog->setOrientation(RowColumn::VERTICAL);
	fileSelectionDialog->setPacking(RowColumn::PACK_TIGHT);
	fileSelectionDialog->setNumMinorWidgets(1);
	
	/* Create the path button box: */
	pathButtonBox=new RowColumn("PathButtonBox",fileSelectionDialog,false);
	pathButtonBox->setOrientation(RowColumn::HORIZONTAL);
	pathButtonBox->setPacking(RowColumn::PACK_TIGHT);
	pathButtonBox->setAlignment(Alignment::LEFT);
	pathButtonBox->setNumMinorWidgets(1);
	pathButtonBox->setMarginWidth(0.0f);
	pathButtonBox->setSpacing(0.0f);
	
	/* Create the path buttons for the initial directory: */
	std::string directory;
	if(pipe==0||pipe->isMaster())
		{
		if(initialDirectory!=0)
			{
			/* Copy the given starting directory: */
			directory=initialDirectory;
			}
		else
			{
			/* Start from the current directory: */
			directory=Misc::getCurrentDirectory();
			}
		
		if(pipe!=0)
			{
			/* Send the initial path to all slave nodes: */
			Misc::writeCppString(directory,*pipe);
			pipe->finishMessage();
			}
		}
	else
		{
		/* Read the initial path from the master node: */
		directory=Misc::readCppString(*pipe);
		}
	
	/* Create a button for the root directory: */
	std::string::iterator pathIt=directory.begin();
	while(pathIt!=directory.end()&&*pathIt=='/')
		++pathIt;
	Button* rootButton=new Button("RootButton",pathButtonBox,"/");
	rootButton->setBorderWidth(rootButton->getBorderWidth()*0.5f);
	rootButton->getSelectCallbacks().add(this,&FileSelectionDialog::pathButtonSelectedCallback);
	
	/* Create buttons for all other directories in the path: */
	int pathButtonIndex=0;
	while(pathIt!=directory.end())
		{
		/* Extract the name of the next directory: */
		std::string::iterator dirStart=pathIt;
		for(++pathIt;pathIt!=directory.end()&&*pathIt!='/';++pathIt)
			;
		
		/* Create a button for the next directory: */
		char pathButtonName[20];
		snprintf(pathButtonName,sizeof(pathButtonName),"PathButton%04d",pathButtonIndex);
		Button* pathButton=new Button(pathButtonName,pathButtonBox,std::string(dirStart,pathIt).c_str());
		pathButton->setBorderWidth(pathButton->getBorderWidth()*0.5f);
		pathButton->getSelectCallbacks().add(this,&FileSelectionDialog::pathButtonSelectedCallback);
		++pathButtonIndex;
		
		/* Skip slashes: */
		while(pathIt!=directory.end()&&*pathIt=='/')
			++pathIt;
		}
	
	pathButtonBox->manageChild();
	
	/* Create the file name text field: */
	fileNameField=new GLMotif::TextField("FileNameField",fileSelectionDialog,40);
	fileNameField->setHAlignment(GLFont::Left);
	fileNameField->setEditable(true);
	
	/* Create the file list box: */
	fileList=new ScrolledListBox("FileList",fileSelectionDialog,ListBox::ATMOST_ONE,50,15);
	fileList->showHorizontalScrollBar(true);
	fileList->getListBox()->getItemSelectedCallbacks().add(this,&FileSelectionDialog::listItemSelectedCallback);
	fileList->getListBox()->getValueChangedCallbacks().add(this,&FileSelectionDialog::listValueChangedCallback);
	
	/* Create the button box: */
	RowColumn* buttonBox=new RowColumn("ButtonBox",fileSelectionDialog,false);
	buttonBox->setOrientation(RowColumn::HORIZONTAL);
	buttonBox->setPacking(RowColumn::PACK_TIGHT);
	buttonBox->setNumMinorWidgets(1);
	
	{
	/* Create the filter list: */
	std::vector<std::string> filterListItems;
	filterListItems.push_back("All Files");
	if(sFileNameFilters!=0)
		{
		const char* startPtr=fileNameFilters;
		while(*startPtr!='\0')
			{
			const char* endPtr;
			for(endPtr=startPtr;*endPtr!='\0'&&*endPtr!=',';++endPtr)
				;
			filterListItems.push_back(std::string(startPtr,endPtr));
			if(*endPtr==',')
				++endPtr;
			startPtr=endPtr;
			}
		}
	filterList=new DropdownBox("FilterList",buttonBox,filterListItems);
	filterList->setSelectedItem(filterList->getNumItems()-1);
	filterList->getValueChangedCallbacks().add(this,&FileSelectionDialog::filterListValueChangedCallback);
	fileNameFilters=filterList->getItem(filterList->getNumItems()-1);
	}
	
	/* Create a separator: */
	Blind* separator=new Blind("Separator",buttonBox);
	separator->setPreferredSize(Vector(buttonBox->getSpacing(),0.0f,0.0f));
	
	/* Create the command button box: */
	RowColumn* commandButtonBox=new RowColumn("CommandButtonBox",buttonBox,false);
	commandButtonBox->setOrientation(RowColumn::HORIZONTAL);
	commandButtonBox->setPacking(RowColumn::PACK_GRID);
	commandButtonBox->setNumMinorWidgets(1);
	
	/* Create the command buttons: */
	Button* okButton=new Button("OK",commandButtonBox,"OK");
	okButton->getSelectCallbacks().add(this,&FileSelectionDialog::okButtonSelectedCallback);
	
	Button* cancelButton=new Button("Cancel",commandButtonBox,"Cancel");
	cancelButton->getSelectCallbacks().add(this,&FileSelectionDialog::cancelButtonSelectedCallback);
	
	commandButtonBox->manageChild();
	
	/* Let the separator eat any size increases: */
	buttonBox->setColumnWeight(1,1.0f);
	
	buttonBox->manageChild();
	
	fileSelectionDialog->setRowWeight(2,1.0f);
	
	/* Select the last path button (to read the initial directory): */
	setSelectedPathButton(pathButtonIndex);
	
	fileSelectionDialog->manageChild();
	}

FileSelectionDialog::~FileSelectionDialog(void)
	{
	/* Delete the pipe: */
	if(pipe!=0)
		delete pipe;
	}

void FileSelectionDialog::addFileNameFilters(const char* newFileNameFilters)
	{
	/* Add the filters to the filter list: */
	const char* startPtr=newFileNameFilters;
	while(*startPtr!='\0')
		{
		const char* endPtr;
		for(endPtr=startPtr;*endPtr!='\0'&&*endPtr!=',';++endPtr)
			;
		filterList->addItem(std::string(startPtr,endPtr).c_str());
		if(*endPtr==',')
			++endPtr;
		startPtr=endPtr;
		}
	}

void FileSelectionDialog::defaultCloseCallback(FileSelectionDialog::CallbackData* cbData)
	{
	/* Bail out if the callback is not for this dialog: */
	if(this!=cbData->fileSelectionDialog)
		return;
	
	/* Delete the file selection dialog: */
	getManager()->deleteWidget(this);
	}

}
