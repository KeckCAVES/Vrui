/***********************************************************************
ZipArchive - Class to represent ZIP archive files, with functionality to
traverse contained directory hierarchies and extract files using a File
interface.
Copyright (c) 2011 Oliver Kreylos

This file is part of the I/O Support Library (IO).

The I/O Support Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The I/O Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the I/O Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef IO_ZIPARCHIVE_INCLUDED
#define IO_ZIPARCHIVE_INCLUDED

#include <stdexcept>
#include <IO/SeekableFile.h>
#include <IO/OpenFile.h>

/* Forward declarations: */
namespace Misc {
class File;
}

namespace IO {

class ZipArchive
	{
	/* Embedded classes: */
	public:
	typedef IO::SeekableFile::Offset Offset; // Type for file positions
	
	class FileNotFoundError:public std::runtime_error // Class to signal errors in findFile method
		{
		/* Elements: */
		public:
		char* fileName; // Name of file that was not found
		
		/* Constructors and destructors: */
		public:
		FileNotFoundError(const char* sFileName);
		virtual ~FileNotFoundError(void) throw();
		};
	
	class FileID // Class to identify files in a ZIP archive
		{
		friend class ZipArchive;
		
		/* Elements: */
		private:
		Offset filePos; // Position of file inside archive
		size_t compressedSize; // Compressed file size
		size_t uncompressedSize; // Uncompressed file size
		
		/* Constructors and destructors: */
		public:
		FileID(void) // Creates invalid file ID
			:filePos(~Offset(0)),
			 compressedSize(0),uncompressedSize(0)
			{
			}
		
		/* Methods: */
		bool isValid(void) const // Returns true if the identifier points to an existing file
			{
			return filePos!=~Offset(0);
			}
		size_t getCompressedFileSize(void) const // Returns compressed file size
			{
			return compressedSize;
			}
		size_t getFileSize(void) const // Returns uncompressed file size
			{
			return uncompressedSize;
			}
		};
	
	class DirectoryIterator:public FileID // Class to traverse a ZIP archive's directory tree
		{
		friend class ZipArchive;
		
		/* Elements: */
		private:
		Offset nextEntryPos; // Archive position of next directory entry
		size_t fileNameBufferSize; // Allocated size for file name
		char* fileName; // File name of current directory entry
		
		/* Constructors and destructors: */
		public:
		DirectoryIterator(void) // Creates an invalid directory entry
			:nextEntryPos(0),
			 fileNameBufferSize(0),fileName(0)
			{
			};
		private:
		DirectoryIterator(Offset sNextEntryPos) // Creates a directory iterator for the given next entry position
			:nextEntryPos(sNextEntryPos),
			 fileNameBufferSize(0),fileName(0)
			{
			}
		public:
		DirectoryIterator(const DirectoryIterator& source);
		DirectoryIterator& operator=(const DirectoryIterator& source);
		~DirectoryIterator(void)
			{
			delete[] fileName;
			}
		
		/* Elements: */
		public:
		const char* getFileName(void) const // Returns file name as NUL-terminated string
			{
			return fileName;
			};
		};
	
	/* Elements: */
	private:
	SeekableFile* archive; // File object to access the ZIP archive
	Offset directoryPos; // Position of ZIP archive's root directory
	size_t directorySize; // Total size of root directory
	
	/* Private methods: */
	int initArchive(void); // Initializes the ZIP archive file structures; returns error code
	
	/* Constructors and destructors: */
	public:
	ZipArchive(const char* archiveFileName); // Opens a ZIP archive of the given file name
	ZipArchive(SeekableFile* sArchive); // Reads a ZIP archive from an already-opened file; inherits file object
	~ZipArchive(void); // Closes the ZIP archive
	
	/* Methods: */
	DirectoryIterator readDirectory(void); // Returns a new directory iterator
	DirectoryIterator& getNextEntry(DirectoryIterator& dIt); // Advances the directory iterator to the next entry
	FileID findFile(const char* fileName); // Returns a file identifier for a file of the given name; throws exception if file does not exist
	File* openFile(const FileID& fileId); // Returns a file for streaming reading
	SeekableFile* openSeekableFile(const FileID& fileId); // Returns a file for seekable reading
	};

}

#endif
