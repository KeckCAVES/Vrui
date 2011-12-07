/***********************************************************************
StandardDirectory - Pair of classes to access cluster-transparent
standard filesystem directories.
Copyright (c) 2011 Oliver Kreylos

This file is part of the Cluster Abstraction Library (Cluster).

The Cluster Abstraction Library is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Cluster Abstraction Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Cluster Abstraction Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Cluster/StandardDirectory.h>

#include <Misc/StandardMarshallers.h>
#include <Misc/GetCurrentDirectory.h>
#include <IO/StandardDirectory.h>
#include <Cluster/OpenFile.h>

namespace Cluster {

/**********************************
Methods of class StandardDirectory:
**********************************/

StandardDirectory::StandardDirectory(Multiplexer* sMultiplexer,std::string sPathName)
	:pipe(sMultiplexer),
	 entryType(Misc::PATHTYPE_DOES_NOT_EXIST)
	{
	/* Prepend the current directory path to the path name if the given path name is relative: */
	if(sPathName.empty()||sPathName[0]!='/')
		{
		pathName=Misc::getCurrentDirectory();
		pathName.push_back('/');
		pathName.append(sPathName);
		}
	else
		pathName=sPathName;
	
	/* Normalize the path name: */
	normalizePath(pathName,1);
	}

StandardDirectory::StandardDirectory(Multiplexer* sMultiplexer,std::string sPathName,int)
	:pipe(sMultiplexer),
	 pathName(sPathName),
	 entryType(Misc::PATHTYPE_DOES_NOT_EXIST)
	{
	}

std::string StandardDirectory::getName(void) const
	{
	return std::string(getLastComponent(pathName,1),pathName.end());
	}

std::string StandardDirectory::getPath(void) const
	{
	return pathName;
	}

std::string StandardDirectory::getPath(const char* relativePath) const
	{
	/* Assemble and normalize the absolute path name: */
	std::string result=pathName;
	if(result.length()>1)
		result.push_back('/');
	result.append(relativePath);
	normalizePath(result,1);
	
	return result;
	}

bool StandardDirectory::hasParent(void) const
	{
	return pathName.length()>1;
	}

IO::DirectoryPtr StandardDirectory::getParent(void) const
	{
	/* Check for the special case of the root directory: */
	if(pathName.length()==1)
		return 0;
	else
		{
		/* Find the last slash in the absolute path name: */
		std::string::const_iterator slashIt;
		for(slashIt=pathName.end()-1;*slashIt!='/';--slashIt)
			;
		
		/* Find the last component in the absolute path name: */
		std::string::const_iterator lastCompIt=getLastComponent(pathName,1);
		
		/* Strip off the last slash unless it's the prefix: */
		if(lastCompIt-pathName.begin()>1)
			--lastCompIt;
		
		/* Open and return the directory corresponding to the path name prefix before the last slash: */
		if(pipe.getMultiplexer()->isMaster())
			return new StandardDirectoryMaster(pipe.getMultiplexer(),std::string(pathName.begin(),lastCompIt),0);
		else
			return new StandardDirectorySlave(pipe.getMultiplexer(),std::string(pathName.begin(),lastCompIt),0);
		}
	}

Misc::PathType StandardDirectory::getEntryType(void) const
	{
	return entryType;
	}

IO::FilePtr StandardDirectory::openFile(const char* fileName,IO::File::AccessMode accessMode) const
	{
	/* Assemble the absolute path name of the given file: */
	std::string filePath=pathName;
	if(filePath.length()>1)
		filePath.push_back('/');
	filePath.append(fileName);
	
	/* Open and return the file: */
	return Cluster::openFile(pipe.getMultiplexer(),filePath.c_str(),accessMode);
	}

IO::DirectoryPtr StandardDirectory::openDirectory(const char* directoryName) const
	{
	/* Assemble the absolute path name of the given directory: */
	std::string directoryPath=pathName;
	if(directoryPath.length()>1)
		directoryPath.push_back('/');
	directoryPath.append(directoryName);
	
	/* Return the new directory: */
	if(pipe.getMultiplexer()->isMaster())
		return new StandardDirectoryMaster(pipe.getMultiplexer(),directoryPath);
	else
		return new StandardDirectorySlave(pipe.getMultiplexer(),directoryPath);
	}

/****************************************
Methods of class StandardDirectoryMaster:
****************************************/

StandardDirectoryMaster::StandardDirectoryMaster(Multiplexer* sMultiplexer,std::string sPathName)
	:StandardDirectory(sMultiplexer,sPathName),
	 directory(opendir(pathName.c_str())),
	 entry(0)
	{
	/* Check for failure: */
	pipe.write<char>(directory!=0?1:0);
	pipe.flush();
	if(directory==0)
		throw OpenError(pathName.c_str());
	}

StandardDirectoryMaster::StandardDirectoryMaster(Multiplexer* sMultiplexer,std::string sPathName,int)
	:StandardDirectory(sMultiplexer,sPathName,0),
	 directory(opendir(pathName.c_str())),
	 entry(0)
	{
	/* Check for failure: */
	pipe.write<char>(directory!=0?1:0);
	pipe.flush();
	if(directory==0)
		throw OpenError(pathName.c_str());
	}

StandardDirectoryMaster::~StandardDirectoryMaster(void)
	{
	/* Close the directory: */
	if(directory!=0)
		closedir(directory);
	}

void StandardDirectoryMaster::rewind(void)
	{
	rewinddir(directory);
	entry=0;
	entryType=Misc::PATHTYPE_DOES_NOT_EXIST;
	}

bool StandardDirectoryMaster::readNextEntry(void)
	{
	/* Read the next entry: */
	entry=readdir(directory);
	
	/* Send a status code to the slaves: */
	pipe.write<char>(entry!=0?1:0);
	
	if(entry!=0)
		{
		/* Send the entry name to the slaves: */
		Misc::Marshaller<std::string>::write(std::string(entry->d_name),pipe);
		
		/* Determine the entry's path type: */
		#ifdef _DIRENT_HAVE_D_TYPE
		
		/* Convert the OS-level entry type to a Misc::PathType: */
		switch(entry->d_type)
			{
			case DT_REG:
				entryType=Misc::PATHTYPE_FILE;
				break;
				
			case DT_DIR:
				entryType=Misc::PATHTYPE_DIRECTORY;
				break;
				
			case DT_CHR:
				entryType=Misc::PATHTYPE_CHARACTER_DEVICE;
				break;
				
			case DT_BLK:
				entryType=Misc::PATHTYPE_BLOCK_DEVICE;
				break;
				
			case DT_FIFO:
				entryType=Misc::PATHTYPE_NAMED_PIPE;
				break;
				
			case DT_LNK:
				entryType=Misc::PATHTYPE_SYMBOLIC_LINK;
				break;
				
			case DT_SOCK:
				entryType=Misc::PATHTYPE_SOCKET;
				break;
				
			default:
				entryType=Misc::PATHTYPE_UNKNOWN;
			}
		
		#else
		
		/* Assemble the absolute path name of the directory entry: */
		std::string entryPath=pathName;
		if(entryPath.length()>1)
			entryPath.push_back('/');
		entryPath.append(entry->d_name);
		
		/* Query the path's file type: */
		entryType=Misc::getPathType(entryPath.c_str());
		
		#endif
		
		/* Send the entry path type to the slaves: */
		pipe.write<int>(entryType);
		}
	
	pipe.flush();
	
	return entry!=0;
	}

const char* StandardDirectoryMaster::getEntryName(void) const
	{
	return entry->d_name;
	}

/***************************************
Methods of class StandardDirectorySlave:
***************************************/

StandardDirectorySlave::StandardDirectorySlave(Multiplexer* sMultiplexer,std::string sPathName)
	:StandardDirectory(sMultiplexer,sPathName)
	{
	/* Check for failure: */
	if(pipe.read<char>()==0)
		throw OpenError(pathName.c_str());
	}

StandardDirectorySlave::StandardDirectorySlave(Multiplexer* sMultiplexer,std::string sPathName,int)
	:StandardDirectory(sMultiplexer,sPathName,0)
	{
	/* Check for failure: */
	if(pipe.read<char>()==0)
		throw OpenError(pathName.c_str());
	}

StandardDirectorySlave::~StandardDirectorySlave(void)
	{
	}

void StandardDirectorySlave::rewind(void)
	{
	entryName="";
	entryType=Misc::PATHTYPE_DOES_NOT_EXIST;
	}

bool StandardDirectorySlave::readNextEntry(void)
	{
	/* Read the next entry: */
	bool haveEntry=pipe.read<char>()!=0;
	
	if(haveEntry)
		{
		/* Read the entry name: */
		entryName=Misc::Marshaller<std::string>::read(pipe);
		
		/* Read the entry type: */
		entryType=(Misc::PathType)pipe.read<int>();
		}
	
	return haveEntry;
	}

const char* StandardDirectorySlave::getEntryName(void) const
	{
	return entryName.c_str();
	}

}
