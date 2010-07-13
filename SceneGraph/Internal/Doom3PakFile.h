/***********************************************************************
Doom3PakFile - Class to represent id Software pk3/pk4 game data archives
(just zip archives in disguise).
Copyright (c) 2007-2010 Oliver Kreylos

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

#ifndef SCENEGRAPH_INTERNAL_DOOM3PAKFILE_INCLUDED
#define SCENEGRAPH_INTERNAL_DOOM3PAKFILE_INCLUDED

#include <Misc/LargeFile.h>

namespace SceneGraph {

class Doom3PakFile
	{
	/* Embedded classes: */
	public:
	typedef Misc::LargeFile::Offset Offset; // Type for file offsets
	
	class DirectoryIterator;
	
	class FileID // Class to identify files in the archive
		{
		friend class Doom3PakFile;
		friend class DirectoryIterator;
		
		/* Elements: */
		private:
		Offset filePos; // Archive position of file
		size_t compressedSize; // Compressed file size
		size_t uncompressedSize; // Uncompressed file size
		
		/* Constructors and destructors: */
		public:
		FileID(void)
			:filePos(0)
			{
			};
		};
	
	class DirectoryIterator // Class to access the archive's central directory
		{
		friend class Doom3PakFile;
		
		/* Elements: */
		private:
		Offset nextEntryPos; // Archive position of next directory entry
		bool valid; // Flag if the iterator is valid
		size_t fileNameBufferSize; // Allocated size of file name buffer
		char* fileName; // Name of file
		Offset filePos; // Archive position of file
		size_t compressedSize; // Compressed file size
		size_t uncompressedSize; // Uncompressed file size
		
		/* Constructors and destructors: */
		public:
		DirectoryIterator(void)
			:valid(false),
			 fileNameBufferSize(0),fileName(0)
			{
			};
		private:
		DirectoryIterator(const DirectoryIterator& source); // Prohibit copy constructor
		DirectoryIterator& operator=(const DirectoryIterator& source); // Prohibit assignment operator
		public:
		~DirectoryIterator(void)
			{
			delete[] fileName;
			};
		
		/* Elements: */
		bool isValid(void) const // Returns true if the iterator describes a file
			{
			return valid;
			};
		const char* getFileName(void) const // Returns file name as NUL-terminated string
			{
			return fileName;
			};
		size_t getFileSize(void) const // Returns uncompressed file size
			{
			return uncompressedSize;
			};
		FileID getFileID(void) const // Returns a file ID for the current file
			{
			FileID result;
			result.filePos=filePos;
			result.compressedSize=compressedSize;
			result.uncompressedSize=uncompressedSize;
			return result;
			};
		};
	
	/* Elements: */
	private:
	Misc::LargeFile file; // Handle for the PAK file itself
	Offset directoryPos; // Position of central directory in PAK file
	size_t directorySize; // Total size of central directory
	
	/* Constructors and destructors: */
	public:
	Doom3PakFile(const char* pakFileName); // Opens a PAK file of the given name
	~Doom3PakFile(void); // Closes the PAK file
	
	/* Methods: */
	DirectoryIterator* readDirectory(void); // Returns a directory iterator; must be deleted after use
	DirectoryIterator* getNextDirectoryEntry(DirectoryIterator* dIt); // Advances the directory iterator to the next entry
	unsigned char* readFile(const FileID& fileId,size_t& fileSize); // Reads a file into a newly allocated buffer; returns file size
	};

}

#endif
