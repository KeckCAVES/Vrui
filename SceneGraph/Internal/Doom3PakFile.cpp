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

#include <SceneGraph/Internal/Doom3PakFile.h>

#include <zlib.h>
#include <Misc/ThrowStdErr.h>

namespace SceneGraph {

/*****************************
Methods of class Doom3PakFile:
*****************************/

Doom3PakFile::Doom3PakFile(const char* pakFileName)
	:file(pakFileName,"rb",Misc::LargeFile::LittleEndian)
	{
	/* Check the first local file header's signature, to check if it's a zip file in the first place: */
	unsigned int signature=file.read<unsigned int>();
	if(signature!=0x04034b50U)
		Misc::throwStdErr("Doom3PakFile::Doom3PakFile: %s is not a valid pk3/pk4 file",pakFileName);
	
	/* Locate the central directory (it's at the end of the file): */
	file.seekEnd(0);
	Offset fileSize=file.tell();
	
	/* Read backwards from end of file until end-of-directory signature is found: */
	Offset readPos=fileSize;
	Offset firstReadPos=readPos>Offset(70000)?readPos-Offset(70000):Offset(0); // If no signature is found after this pos, there is none
	unsigned char readBuffer[256];
	unsigned char* rbPtr=readBuffer;
	int state=0;
	while(state!=4)
		{
		/* Get the previous byte from the buffer: */
		if(rbPtr==readBuffer)
			{
			/* Read another chunk of data from the file: */
			size_t readSize=sizeof(readBuffer);
			if(size_t(readPos-firstReadPos)<readSize)
				readSize=size_t(readPos-firstReadPos);
			if(readSize==0) // Haven't found the signature, and there's nothing more to read
				Misc::throwStdErr("Doom3PakFile::Doom3PakFile: Unable to locate central directory in file %s",pakFileName);
			readPos-=Offset(readSize);
			file.seekSet(readPos);
			file.read(readBuffer,readSize);
			rbPtr=readBuffer+readSize;
			}
		--rbPtr;
		
		/* Process the byte through the state machine: */
		switch(state)
			{
			case 0: // Nothing matching
				if(*rbPtr==0x06U)
					state=1;
				break;
			
			case 1: // Read 0x06
				if(*rbPtr==0x05U)
					state=2;
				else if(*rbPtr!=0x06U)
					state=0;
				break;
			
			case 2: // Read 0x0605
				if(*rbPtr==0x4bU)
					state=3;
				else if(*rbPtr==0x06U)
					state=1;
				else
					state=0;
				break;
			
			case 3: // Read 0x06054b
				if(*rbPtr==0x50U)
					state=4;
				else if(*rbPtr==0x06U)
					state=1;
				else
					state=0;
			}
		}
	Offset endOfCentralDirPos=readPos+Offset(rbPtr-readBuffer);
	
	/* Read the end-of-central-directory entry: */
	file.seekSet(endOfCentralDirPos);
	unsigned int eocdSignature=file.read<unsigned int>();
	if(eocdSignature!=0x06054b50U)
		Misc::throwStdErr("Doom3PakFile::Doom3PakFile: Invalid central directory in file %s",pakFileName);
	
	/* Skip irrelevant bits: */
	file.seekCurrent(sizeof(unsigned short)*4);
	
	/* Read the relevant bits: */
	unsigned int eocdCDSize=file.read<unsigned int>();
	unsigned int eocdCDOffset=file.read<unsigned int>();
	unsigned short eocdCommentLength=file.read<unsigned short>();
	
	/* Remember the directory offset and size: */
	directoryPos=Offset(eocdCDOffset);
	directorySize=size_t(eocdCDSize);
	
	/* Check again if this was really the end-of-directory marker: */
	if(directoryPos+Offset(directorySize)!=endOfCentralDirPos||endOfCentralDirPos+Offset(sizeof(unsigned int)*3+sizeof(unsigned short)*5+eocdCommentLength)!=fileSize)
		Misc::throwStdErr("Doom3PakFile::Doom3PakFile: Invalid central directory in file %s",pakFileName);
	}

Doom3PakFile::~Doom3PakFile(void)
	{
	}

Doom3PakFile::DirectoryIterator* Doom3PakFile::readDirectory(void)
	{
	/* Create a new directory entry: */
	DirectoryIterator* result=new DirectoryIterator;
	result->nextEntryPos=directoryPos;
	result->valid=true;
	
	/* Read the first directory entry: */
	return getNextDirectoryEntry(result);
	}

Doom3PakFile::DirectoryIterator* Doom3PakFile::getNextDirectoryEntry(Doom3PakFile::DirectoryIterator* dIt)
	{
	/* Check if the entry is still valid: */
	if(!dIt->valid)
		return dIt; // Do nothing
	
	/* Read the next entry: */
	file.seekSet(dIt->nextEntryPos);
	
	/* Read the next entry header: */
	unsigned int entryHeader=file.read<unsigned int>();
	if(entryHeader==0x05054b50U||entryHeader==0x06054b50U) // Digital signature entry or end-of-central-directory entry
		{
		/* Invalidate and return the iterator: */
		dIt->valid=false;
		dIt->fileNameBufferSize=0;
		delete[] dIt->fileName;
		dIt->fileName=0;
		return dIt;
		}
	else if(entryHeader!=0x02014b50U) // File entry
		Misc::throwStdErr("Doom3PakFile::getNextDirectoryEntry: Bad entry header in central directory");
	
	/* Skip irrelevant bits: */
	file.seekCurrent(sizeof(unsigned short)*6+sizeof(unsigned int));
	
	/* Read relevant bits: */
	unsigned int compressedSize=file.read<unsigned int>();
	unsigned int uncompressedSize=file.read<unsigned int>();
	unsigned short fileNameLength=file.read<unsigned short>();
	unsigned short extraFieldLength=file.read<unsigned short>();
	unsigned short fileCommentLength=file.read<unsigned short>();
	
	/* Skip irrelevant bits: */
	file.seekCurrent(sizeof(unsigned short)*2+sizeof(unsigned int));
	
	/* Read relevant bits: */
	unsigned int localHeaderOffset=file.read<unsigned int>();
	
	/* Read the file name: */
	if(size_t(fileNameLength+1)>dIt->fileNameBufferSize)
		{
		dIt->fileNameBufferSize=((fileNameLength+1)*11)/10+2;
		delete[] dIt->fileName;
		dIt->fileName=new char[dIt->fileNameBufferSize];
		}
	file.read(dIt->fileName,fileNameLength);
	dIt->fileName[fileNameLength]='\0';
	
	/* Store file information: */
	dIt->filePos=Offset(localHeaderOffset);
	dIt->compressedSize=size_t(compressedSize);
	dIt->uncompressedSize=size_t(uncompressedSize);
	
	/* Skip extra field and file comment: */
	file.seekCurrent(Offset(extraFieldLength+fileCommentLength));
	
	/* Store the next entry's offset: */
	dIt->nextEntryPos=file.tell();
	
	return dIt;
	}

unsigned char* Doom3PakFile::readFile(const Doom3PakFile::FileID& fileId,size_t& fileSize)
	{
	/* Read the file's header: */
	file.seekSet(fileId.filePos);
	if(file.read<unsigned int>()!=0x04034b50U)
		Misc::throwStdErr("Doom3PakFile::readFile: Invalid file header signature");
	
	/* Skip irrelevant bits: */
	file.seekCurrent(sizeof(unsigned short)*2);
	
	/* Read relevant bits: */
	unsigned short compressionMethod=file.read<unsigned short>();
	
	/* Skip irrelevant bits: */
	file.seekCurrent(sizeof(unsigned short)*2+sizeof(unsigned int));
	
	/* Read relevant bits: */
	unsigned int compressedSize=file.read<unsigned int>();
	unsigned int uncompressedSize=file.read<unsigned int>();
	unsigned short fileNameLength=file.read<unsigned short>();
	unsigned short extraFieldLength=file.read<unsigned short>();
	
	/* Skip file name and extra field: */
	file.seekCurrent(Offset(fileNameLength+extraFieldLength));
	
	/* Read the compressed data: */
	Bytef* compressed=new Bytef[compressedSize];
	file.read(compressed,compressedSize);
	
	Bytef* uncompressed;
	if(compressionMethod==0)
		{
		/* Just return the compressed data: */
		uncompressed=compressed;
		uncompressedSize=compressedSize;
		}
	else
		{
		/* Uncompress the data: */
		uncompressed=new Bytef[uncompressedSize];
		z_stream stream;
		stream.zalloc=0;
		stream.zfree=0;
		stream.opaque=0;
		if(inflateInit2(&stream,-MAX_WBITS)!=Z_OK)
			{
			delete[] compressed;
			delete[] uncompressed;
			Misc::throwStdErr("Doom3PakFile::readFile: Internal zlib error");
			}
		stream.next_in=compressed;
		stream.avail_in=compressedSize;
		stream.next_out=uncompressed;
		stream.avail_out=uncompressedSize;
		if(inflate(&stream,Z_FINISH)!=Z_STREAM_END)
			{
			delete[] compressed;
			delete[] uncompressed;
			Misc::throwStdErr("Doom3PakFile::readFile: Internal zlib error");
			}
		delete[] compressed;
		if(inflateEnd(&stream)!=Z_OK)
			{
			delete[] uncompressed;
			Misc::throwStdErr("Doom3PakFile::readFile: Internal zlib error");
			}
		}
	
	/* Return the data: */
	fileSize=size_t(uncompressedSize);
	return reinterpret_cast<unsigned char*>(uncompressed);
	}

}
