/***********************************************************************
Doom3FileManager - Class to read files from sets of pk3/pk4 files and
patch directories.
Copyright (c) 2007-2011 Oliver Kreylos

This file is part of the Simple Scene Graph Renderer (SceneGraph).

The Simple Scene Graph Renderer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Simple Scene Graph Renderer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Simple Scene Graph Renderer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <SceneGraph/Internal/Doom3FileManager.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

namespace SceneGraph {

namespace {

/****************
Helper functions:
****************/

#ifdef __APPLE__
int pakFileFilter(struct dirent* dirent)
#else
int pakFileFilter(const struct dirent* dirent)
#endif
	{
	const char* name=dirent->d_name;
	if(strncasecmp(name,"pak",3)!=0)
		return 0;
	const char* extPtr;
	for(extPtr=name+3;*extPtr!='\0'&&*extPtr!='.';++extPtr)
		if(!isdigit(*extPtr))
			return 0;
	if(strncasecmp(extPtr,".pk",3)!=0)
		return 0;
	if(!isdigit(extPtr[3]))
		return 0;
	
	return 1;
	}

}

/*********************************
Methods of class Doom3FileManager:
*********************************/

Doom3FileManager::Doom3FileManager(void)
	{
	}

Doom3FileManager::Doom3FileManager(const char* baseDirectory,const char* pakFilePrefix)
	{
	addPakFiles(baseDirectory,pakFilePrefix);
	}

Doom3FileManager::~Doom3FileManager(void)
	{
	/* Delete all pak files: */
	for(std::vector<PakFile*>::iterator pfIt=pakFiles.begin();pfIt!=pakFiles.end();++pfIt)
		delete (*pfIt);
	}

void Doom3FileManager::addPakFile(const char* pakFileName)
	{
	/* Open a new pak archive and add it to the list: */
	PakFile* pak=new PakFile(pakFileName);
	pakFiles.push_back(pak);
	
	/* Read the pak archive's directory and add all files to the pak file tree: */
	PakFile::DirectoryIterator dIt=pak->readDirectory();
	while(dIt.isValid())
		{
		/* Insert the file into the directory structure: */
		pakFileTree.insertLeaf(dIt.getFileName(),PakFileHandle(pak,dIt));
		
		/* Go to the next entry: */
		pak->getNextEntry(dIt);
		}
	}

void Doom3FileManager::addPakFiles(const char* baseDirectory,const char* pakFilePrefix)
	{
	/* Find all pak???.pk[0-9] files in the base directory: */
	struct dirent** pakFileList;
	int numPakFiles=scandir(baseDirectory,&pakFileList,pakFileFilter,alphasort);
	
	/* Add all pak files in alphabetical (numerical) order: */
	for(int i=0;i<numPakFiles;++i)
		{
		char pakFileName[1024];
		snprintf(pakFileName,sizeof(pakFileName),"%s/%s",baseDirectory,pakFileList[i]->d_name);
		addPakFile(pakFileName);
		free(pakFileList[i]);
		}
	free(pakFileList);
	}

IO::File* Doom3FileManager::getFile(const char* fileName)
	{
	/* Search the file in the pak file tree: */
	PakFileTree::LeafID leafId=pakFileTree.findLeaf(fileName);
	if(!leafId.isValid())
		throw ReadError(fileName);
	
	/* Get the file's pak file handle: */
	const PakFileHandle& pfh=pakFileTree.getLeafValue(leafId);
	
	/* Read and return the file: */
	return pfh.pakFile->openFile(pfh.fileID);
	}

IO::SeekableFile* Doom3FileManager::getSeekableFile(const char* fileName)
	{
	/* Search the file in the pak file tree: */
	PakFileTree::LeafID leafId=pakFileTree.findLeaf(fileName);
	if(!leafId.isValid())
		throw ReadError(fileName);
	
	/* Get the file's pak file handle: */
	const PakFileHandle& pfh=pakFileTree.getLeafValue(leafId);
	
	/* Read and return the file: */
	return pfh.pakFile->openSeekableFile(pfh.fileID);
	}

}
