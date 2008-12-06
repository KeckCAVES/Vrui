/***********************************************************************
FileLocator - Class to find files from an ordered list of search paths.
Copyright (c) 2007 Oliver Kreylos
Based on code written by Braden Pellett.

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

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <Misc/ThrowStdErr.h>

#include <Misc/FileLocator.h>

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

namespace {

/****************
Helper functions:
****************/

const char* findLastOf(const char* string,char ch)
	{
	const char* result=0;
	for(const char* sPtr=string;*sPtr!='\0';++sPtr)
		if(*sPtr==ch)
			result=sPtr;
	return result;
	}

std::string cleanpath(const char* path)
	{
	std::string result;
	
	/* Check for the initial slash in an absolute path: */
	const char* segBegin=path;
	if(*segBegin=='/')
		{
		result.append(1,'/');
		++segBegin;
		}
	
	/* Process the rest of the path segment by segment: */
	bool notFirstSegment=false;
	while(*segBegin!='\0')
		{
		/* Find the next path segment: */
		const char* segEnd;
		for(segEnd=segBegin;*segEnd!='\0'&&*segEnd!='/';++segEnd)
			;
		if(segEnd-segBegin==2&&segBegin[0]=='.'&&segBegin[1]=='.')
			{
			/* Remove the last segment from the result path: */
			while(result.size()>0&&result[result.size()-1]!='/')
				result.erase(result.size()-1);
			if(result.size()>1)
				result.erase(result.size()-1);
			}
		else if(segEnd!=segBegin&&(segEnd-segBegin!=1||*segBegin!='.'))
			{
			/* Append the segment to the result path: */
			if(notFirstSegment)
				result.append(1,'/');
			result.append(segBegin,segEnd);
			notFirstSegment=true;
			}
		
		/* Go to the next path segment: */
		if(*segEnd=='/')
			++segEnd;
		segBegin=segEnd;
		}
	
	return result;
	}

std::string cleanpath(const char* pathBegin,const char* pathEnd)
	{
	std::string result;
	
	/* Check for the initial slash in an absolute path: */
	const char* segBegin=pathBegin;
	if(segBegin!=pathEnd&&*segBegin=='/')
		{
		result.append(1,'/');
		++segBegin;
		}
	
	/* Process the rest of the path segment by segment: */
	bool notFirstSegment=false;
	while(segBegin!=pathEnd)
		{
		/* Find the next path segment: */
		const char* segEnd;
		for(segEnd=segBegin;segEnd!=pathEnd&&*segEnd!='/';++segEnd)
			;
		if(segEnd-segBegin==2&&segBegin[0]=='.'&&segBegin[1]=='.')
			{
			/* Remove the last segment from the result path: */
			while(result.size()>0&&result[result.size()-1]!='/')
				result.erase(result.size()-1);
			if(result.size()>1)
				result.erase(result.size()-1);
			}
		else if(segEnd!=segBegin&&(segEnd-segBegin!=1||*segBegin!='.'))
			{
			/* Append the segment to the result path: */
			if(notFirstSegment)
				result.append(1,'/');
			result.append(segBegin,segEnd);
			notFirstSegment=true;
			}
		
		/* Go to the next path segment: */
		if(segEnd!=pathEnd)
			++segEnd;
		segBegin=segEnd;
		}
	
	return result;
	}

}

namespace Misc {

/****************************
Methods of class FileLocator:
****************************/

FileLocator::FileLocator(void)
	{
	}

void FileLocator::addCurrentDirectory(void)
	{
	char cwd[PATH_MAX];
	if(getcwd(cwd,PATH_MAX)!=0)
		pathList.push_back(cwd);
	}

void FileLocator::addPath(const char* newPath)
	{
	/* Prefix relative paths with the current working directory: */
	if(newPath[0]=='/')
		pathList.push_back(cleanpath(newPath));
	else if(newPath[0]!='\0')
		{
		char cwd[PATH_MAX];
		if(getcwd(cwd,PATH_MAX)!=0)
			{
			std::string absPath=cwd;
			absPath.append(1,'/');
			absPath.append(newPath);
			pathList.push_back(cleanpath(absPath.c_str()));
			}
		else
			pathList.push_back(cleanpath(newPath));
		}
	}

void FileLocator::addPath(const char* newPathBegin,const char* newPathEnd)
	{
	/* Prefix relative paths with the current working directory: */
	if(newPathEnd-newPathBegin>1&&newPathBegin[0]=='/')
		pathList.push_back(cleanpath(newPathBegin,newPathEnd));
	else if(newPathEnd-newPathBegin>0)
		{
		char cwd[PATH_MAX];
		if(getcwd(cwd,PATH_MAX)!=0)
			{
			std::string absPath=cwd;
			absPath.append(1,'/');
			absPath.append(newPathBegin,newPathEnd);
			pathList.push_back(cleanpath(absPath.c_str()));
			}
		else
			pathList.push_back(cleanpath(newPathBegin,newPathEnd));
		}
	}

void FileLocator::addPathFromFile(const char* fileName)
	{
	/* Find the final '/': */
	const char* slashPtr=findLastOf(fileName,'/');
	if(slashPtr!=0)
		{
		/* Add the file's directory to the search path list: */
		addPath(fileName,slashPtr);
		}
	else
		{
		/* Add the current directory to the search path list: */
		addCurrentDirectory();
		}
	}

void FileLocator::addPathList(const char* newPathList)
	{
	/* Process all paths from the colon-separated list: */
	while(*newPathList!='\0')
		{
		/* Find the end of the next path: */
		const char* pathEnd;
		for(pathEnd=newPathList;*pathEnd!='\0'&&*pathEnd!=':';++pathEnd)
			;
		
		/* Add the path if it is non-empty: */
		if(pathEnd!=newPathList)
			addPath(newPathList,pathEnd);
		
		/* Go to the next path: */
		if(*pathEnd!='\0')
			++pathEnd;
		newPathList=pathEnd;
		}
	}

void FileLocator::addPathFromApplication(const char* executablePath)
	{
	/* Separate the executable name from the executable path: */
	const char* slashPtr=findLastOf(executablePath,'/');
	std::string appName=slashPtr!=0?std::string(slashPtr+1):std::string(executablePath);
	
	/* Add the standard resource search path for private installed applications: */
	if(getenv("HOME")!=0)
		{
		std::string path=getenv("HOME");
		path.append("/.");
		path.append(appName);
		addPath(path);
		}
	
	/* Add the standard resource search paths for system-wide installed applications: */
	addPath("/usr/share/"+appName);
	addPath("/usr/local/share/"+appName);
	
	/* Construct the fully-qualified executable name: */
	std::string fullExePath;
	if(executablePath[0]=='/')
		fullExePath=executablePath;
	else
		{
		/* Try to find the full path to the executable: */
		#ifdef __LINUX__
		/* Get the full executable path through the /proc interface: */
		char pse[PATH_MAX];
		int pseLength=readlink("/proc/self/exe",pse,PATH_MAX);
		if(pseLength>=0)
			fullExePath=std::string(pse,pse+pseLength);
		else
			{
		#else
		/* Search for the executable just as the shell did: */
		if(slashPtr==0&&getenv("PATH")!=0)
			{
			/* Search for the executable in the PATH list: */
			const char* pathBegin=getenv("PATH");
			while(*pathBegin!='\0')
				{
				/* Find the end of the next path: */
				const char* pathEnd;
				for(pathEnd=pathBegin;*pathEnd!='\0'&&*pathEnd!=':';++pathEnd)
					;
				
				/* Test the path if it is non-empty: */
				if(pathEnd!=pathBegin)
					{
					/* Construct the full path name: */
					std::string testName;
					if(*pathBegin!='/')
						{
						/* Start the path name with the current directory: */
						char cwd[PATH_MAX];
						if(getcwd(cwd,PATH_MAX))
							{
							testName.append(cwd);
							testName.append(1,'/');
							}
						}
					testName.append(pathBegin,pathEnd);
					testName.append(1,'/');
					testName.append(appName);
					
					/* Test if the file exists and is an executable: */
					struct stat testStat;
					if(stat(testName.c_str(),&testStat)==0)
						{
						/* Save the matching full path and stop searching: */
						fullExePath=testName;
						break;
						}
					}
				
				/* Go to the next path: */
				if(*pathEnd!='\0')
					++pathEnd;
				pathBegin=pathEnd;
				}
			}
		else
			{
			/* Use the provided path starting at the current directory: */
			char cwd[PATH_MAX];
			if(getcwd(cwd,PATH_MAX)!=0)
				{
				fullExePath=cwd;
				fullExePath.append(1,'/');
				}
			fullExePath.append(executablePath);
			}
		#endif
		#ifdef __LINUX__
			}
		#endif
		}
	
	/* Find the last slash in the cleaned fully-qualified executable path name: */
	std::string cleanFullExePath=cleanpath(fullExePath.c_str());
	executablePath=cleanFullExePath.c_str();
	slashPtr=findLastOf(executablePath,'/');
	
	#ifdef __LINUX__
	/* Check if the executable is part of a Linux application bundle: */
	if(slashPtr!=0)
		{
		if(slashPtr-executablePath>=4&&strncasecmp(slashPtr-4,"/exe",4)==0)
			{
			/* Add the bundle's base directory to the search path list: */
			addPath(executablePath,slashPtr-4);
			}
		else if(slashPtr-executablePath>=7&&strncasecmp(slashPtr-7,"/exe/64",7)==0)
			{
			/* Add the bundle's base directory to the search path list: */
			addPath(executablePath,slashPtr-7);
			}
		}
	#endif
	
	#ifdef __DARWIN__
	/* Check if the executable is part of a Mac OS X application bundle: */
	if(slashPtr!=0&&slashPtr-executablePath>=19&&strncasecmp(slashPtr-19,".app/Contents/MacOS",19)==0)
		{
		/* Add the bundle's resource directory to the search path list: */
		addPath(std::string(executablePath,slashPtr-5)+"Resources");
		}
	#endif
	}

std::string FileLocator::locateFile(const char* fileName)
	{
	/* Look for a file of the given name in all search paths in the list: */
	for(std::vector<std::string>::const_iterator plIt=pathList.begin();plIt!=pathList.end();++plIt)
		{
		/* Check if a file of the given name exists in the search path: */
		std::string pathName=*plIt;
		pathName+="/";
		pathName+=fileName;
		struct stat statBuffer;
		if(stat(pathName.c_str(),&statBuffer)==0)
			return pathName;
		}
	
	/* Instead of returning an empty string or somesuch, throw an exception: */
	Misc::throwStdErr("FileLocator::locateFile: Could not find resource %s",fileName);
	
	/* Just a dummy statement to make the compiler happy: */
	return "";
	}

}
