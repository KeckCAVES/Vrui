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

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#ifndef _DIRENT_HAVE_D_TYPE
#include <sys/stat.h>
#endif
#include <string>
#include <vector>
#include <algorithm>
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
		fullPath.append(static_cast<Button*>(pathButtonBox->getChild(i))->getLabel());
		}
	
	return fullPath;
	}

bool FileSelectionDialog::readDirectory(void)
	{
	if(pipe==0||pipe->isMaster())
		{
		/* Open the current directory: */
		std::string fullPath=getCurrentPath();
		DIR* directory=opendir(fullPath.c_str());
		if(directory==0)
			{
			if(pipe!=0)
				{
				/* Send an error code to the slave nodes: */
				pipe->write<int>(0);
				pipe->finishMessage();
				}
			return false;
			}
		
		/* Read all directory entries: */
		std::vector<std::string> directories;
		std::vector<std::string> files;
		struct dirent* dirEntry;
		while((dirEntry=readdir(directory))!=0)
			{
			/* Ignore hidden entries: */
			if(dirEntry->d_name[0]!='.')
				{
				/* Determine the type of the directory entry: */
				int entryType=0; // unkown, directory, file
				#ifdef _DIRENT_HAVE_D_TYPE
				if(dirEntry->d_type==DT_DIR)
					entryType=1;
				else if(dirEntry->d_type==DT_REG)
					entryType=2;
				#else
				std::string fullName=fullPath;
				fullName.push_back('/');
				fullName.append(dirEntry->d_name);
				struct stat statResult;
				if(stat(fullName.c_str(),&statResult)==0)
					{
					if(S_ISDIR(statResult.st_mode))
						entryType=1;
					else if(S_ISREG(statResult.st_mode))
						entryType=2;
					}
				#endif
				
				if(entryType==1)
					{
					/* Store a directory name: */
					std::string entryName=dirEntry->d_name;
					entryName.push_back('/');
					directories.push_back(entryName);
					}
				else if(entryType==2)
					{
					/* Check if the file name matches any of the supplied extensions: */
					bool passesFilters=true;
					if(fileNameFilters!=0)
						{
						/* Find the file name's extension: */
						const char* extPtr="";
						for(const char* enPtr=dirEntry->d_name;*enPtr!='\0';++enPtr)
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
						std::string entryName=dirEntry->d_name;
						files.push_back(entryName);
						}
					}
				}
			}
		
		if(pipe!=0)
			{
			/* Send a success code to the slave nodes: */
			pipe->write<int>(1);
			}
		
		/* Sort the directory and file names separately: */
		StringCompare sc;
		std::sort(directories.begin(),directories.end(),sc);
		std::sort(files.begin(),files.end(),sc);
		if(pipe!=0)
			{
			/* Send the directory names to the slave nodes: */
			for(std::vector<std::string>::const_iterator dIt=directories.begin();dIt!=directories.end();++dIt)
				{
				/* Write the string: */
				pipe->write<int>(dIt->size());
				pipe->write<char>(dIt->c_str(),dIt->size()+1);
				}
			
			/* Send the file names to the slave nodes: */
			for(std::vector<std::string>::const_iterator fIt=files.begin();fIt!=files.end();++fIt)
				{
				/* Write the string: */
				pipe->write<int>(fIt->size());
				pipe->write<char>(fIt->c_str(),fIt->size()+1);
				}
			
			/* Send the list terminator message: */
			pipe->write<int>(-1);
			pipe->finishMessage();
			}
		
		/* Copy all names into the list box: */
		fileList->getListBox()->clear();
		for(std::vector<std::string>::const_iterator dIt=directories.begin();dIt!=directories.end();++dIt)
			fileList->getListBox()->addItem(dIt->c_str());
		for(std::vector<std::string>::const_iterator fIt=files.begin();fIt!=files.end();++fIt)
			fileList->getListBox()->addItem(fIt->c_str());
		}
	else
		{
		/* Read the status flag from the master node: */
		int success=pipe->read<int>();
		if(success)
			{
			/* Clear the list box: */
			fileList->getListBox()->clear();
			
			/* Read the directory and file names: */
			int length;
			int bufferSize=128;
			char* buffer=new char[bufferSize];
			while((length=pipe->read<int>())>=0)
				{
				/* Make sure the buffer is big enough: */
				if(bufferSize<length+1)
					{
					delete[] buffer;
					bufferSize=(length+1)*3/2;
					buffer=new char[bufferSize];
					}
				
				/* Read the string and add it to the list box: */
				pipe->read<char>(buffer,length+1);
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
	}

void FileSelectionDialog::pathButtonSelectedCallback(Button::SelectCallbackData* cbData)
	{
	/* Change the selected path button: */
	setSelectedPathButton(pathButtonBox->getChildIndex(cbData->button));
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
	 fileList(0),filterList(0)
	{
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
	char* directoryBuffer=0;
	if(pipe==0||pipe->isMaster())
		{
		if(initialDirectory!=0)
			{
			/* Copy the given starting directory: */
			directoryBuffer=new char[strlen(initialDirectory)+1];
			strcpy(directoryBuffer,initialDirectory);
			}
		else
			{
			/* Start from the current directory: */
			size_t bufferSize=512;
			while(true)
				{
				/* Try reading the current directory into the buffer: */
				directoryBuffer=new char[bufferSize];
				if(getcwd(directoryBuffer,bufferSize)!=0)
					break; // Done!
				else
					{
					/* Delete the current buffer and try again with a bigger one: */
					delete[] directoryBuffer;
					bufferSize+=bufferSize;
					}
				}
			}
		
		if(pipe!=0)
			{
			/* Send the initial path to all slave nodes: */
			int length=strlen(directoryBuffer);
			pipe->write<int>(length);
			pipe->write<char>(directoryBuffer,length+1);
			pipe->finishMessage();
			}
		}
	else
		{
		/* Read the initial path from the master node: */
		int length=pipe->read<int>();
		directoryBuffer=new char[length+1];
		pipe->read<char>(directoryBuffer,length+1);
		}
	
	/* Create a button for the root directory: */
	char* pathPtr=directoryBuffer;
	while(*pathPtr=='/')
		++pathPtr;
	Button* rootButton=new Button("RootButton",pathButtonBox,"/");
	rootButton->setBorderWidth(rootButton->getBorderWidth()*0.5f);
	rootButton->getSelectCallbacks().add(this,&FileSelectionDialog::pathButtonSelectedCallback);
	
	/* Create buttons for all other directories in the path: */
	int pathButtonIndex=0;
	while(*pathPtr!='\0')
		{
		/* Extract the name of the next directory: */
		const char* directoryName=pathPtr;
		for(++pathPtr;*pathPtr!='\0'&&*pathPtr!='/';++pathPtr)
			;
		
		/* Terminate the directory name: */
		if(*pathPtr=='/')
			{
			*pathPtr='\0';
			++pathPtr;
			}
		
		/* Create a button for the next directory: */
		char pathButtonName[20];
		snprintf(pathButtonName,sizeof(pathButtonName),"PathButton%04d",pathButtonIndex);
		Button* pathButton=new Button(pathButtonName,pathButtonBox,directoryName);
		pathButton->setBorderWidth(pathButton->getBorderWidth()*0.5f);
		pathButton->getSelectCallbacks().add(this,&FileSelectionDialog::pathButtonSelectedCallback);
		++pathButtonIndex;
		
		/* Skip slashes: */
		while(*pathPtr=='/')
			++pathPtr;
		}
	
	/* Clean up: */
	delete[] directoryBuffer;
	
	pathButtonBox->manageChild();
	
	/* Create the file list box: */
	fileList=new ScrolledListBox("FileList",fileSelectionDialog,ListBox::ALWAYS_ONE,50,15);
	fileList->showHorizontalScrollBar(true);
	fileList->getListBox()->getItemSelectedCallbacks().add(this,&FileSelectionDialog::listItemSelectedCallback);
	
	/* Select the last path button (to read the initial directory): */
	setSelectedPathButton(pathButtonIndex);
	
	/* Read the initial directory: */
	if(!readDirectory())
		{
		/* This is bad! */
		}
	
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
		const char* startPtr=sFileNameFilters;
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
	
	fileSelectionDialog->setRowWeight(1,1.0f);
	
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
