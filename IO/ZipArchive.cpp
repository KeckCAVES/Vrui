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

#include <IO/ZipArchive.h>

#include <string.h>
#include <zlib.h>
#include <Misc/ThrowStdErr.h>
#include <IO/StandardFile.h>
#include <IO/FixedMemoryFile.h>

namespace IO {

namespace {

/**************
Helper classes:
**************/

class ZipArchiveStreamingFile:public File // Class to read ZIP archive entries in a streaming fashion
	{
	/* Embedded classes: */
	public:
	typedef SeekableFile::Offset Offset; // Type for file positions
	
	/* Elements: */
	private:
	SeekableFile& archive; // Reference to the ZIP archive containing the file
	Offset nextReadPos; // Position of next data block to read from archive
	size_t compressedSize; // Amount of data remaining to be read from archive
	size_t compressedBufferSize; // Size of allocated buffer for compressed data read from the archive
	Bytef* compressedBuffer; // Buffer for compressed data read from the archive
	z_stream* stream; // Zlib decompression object; only allocated if file is compressed
	bool eof; // Flag set to true when all uncompressed data has been read
	
	/* Protected methods from File: */
	protected:
	virtual size_t readData(Byte* buffer,size_t bufferSize);
	virtual void writeData(const Byte* buffer,size_t bufferSize);
	
	/* Constructors and destructors: */
	public:
	ZipArchiveStreamingFile(SeekableFile& sArchive,unsigned int sCompressionMethod,Offset sNextReadPos,size_t sCompressedSize);
	virtual ~ZipArchiveStreamingFile(void);
	};

/****************************************
Methods of class ZipArchiveStreamingFile:
****************************************/

size_t ZipArchiveStreamingFile::readData(File::Byte* buffer,size_t bufferSize)
	{
	if(eof)
		return 0;
	
	if(stream!=0)
		{
		/* Decompress data into the provided buffer: */
		stream->next_out=buffer;
		stream->avail_out=bufferSize;
		
		/* Repeat until some output is produced: */
		do
			{
			/* Check if the decompressor needs more input: */
			if(stream->avail_in==0)
				{
				/* Read the next chunk of compressed data from the archive: */
				size_t compressedReadSize=compressedBufferSize;
				if(compressedReadSize>compressedSize)
					compressedReadSize=compressedSize;
				archive.setReadPosAbs(nextReadPos);
				compressedReadSize=archive.readUpTo(compressedBuffer,compressedReadSize);
				nextReadPos+=compressedReadSize;
				compressedSize-=compressedReadSize;
				
				/* Pass the compressed data to the decompressor: */
				stream->next_in=compressedBuffer;
				stream->avail_in=compressedReadSize;
				}
			
			/* Decompress: */
			int result=inflate(stream,Z_NO_FLUSH);
			if(result==Z_STREAM_END)
				{
				/* Clean out the decompressor and set the eof flag: */
				if(inflateEnd(stream)!=Z_OK)
					Misc::throwStdErr("IO::ZipArchiveStreamingFile: Data corruption detected after decompressing");
				eof=true;
				break;
				}
			else if(result!=Z_OK)
				Misc::throwStdErr("IO::ZipArchiveStreamingFile: Internal zlib error while decompressing");
			}
		while(stream->avail_out==bufferSize);
		
		return bufferSize-stream->avail_out;
		}
	else
		{
		/* Read uncompressed data from the archive directly into the provided buffer: */
		size_t readSize=bufferSize;
		if(readSize>compressedSize)
			readSize=compressedSize;
		archive.setReadPosAbs(nextReadPos);
		readSize=archive.readUpTo(buffer,readSize);
		nextReadPos+=readSize;
		compressedSize-=readSize;
		eof=compressedSize==0;
		
		return readSize;
		}
	}

void ZipArchiveStreamingFile::writeData(const File::Byte* buffer,size_t bufferSize)
	{
	/* Writing is not supported; ignore silently */
	}

ZipArchiveStreamingFile::ZipArchiveStreamingFile(SeekableFile& sArchive,unsigned int sCompressionMethod,SeekableFile::Offset sNextReadPos,size_t sCompressedSize)
	:File(ReadOnly),
	 archive(sArchive),
	 nextReadPos(sNextReadPos),compressedSize(sCompressedSize),
	 compressedBufferSize(8192),compressedBuffer(sCompressionMethod!=0?new Bytef[compressedBufferSize]:0),
	 stream(0),eof(false)
	{
	if(sCompressionMethod!=0)
		{
		/* Read the first chunk of compressed data from the archive: */
		size_t compressedReadSize=compressedBufferSize;
		if(compressedReadSize>compressedSize)
			compressedReadSize=compressedSize;
		archive.setReadPosAbs(nextReadPos);
		compressedReadSize=archive.readUpTo(compressedBuffer,compressedReadSize);
		nextReadPos+=compressedReadSize;
		compressedSize-=compressedReadSize;
		
		/* Create and initialize the zlib decompression object: */
		stream=new z_stream;
		stream->next_in=compressedBuffer;
		stream->avail_in=compressedReadSize;
		stream->zalloc=0;
		stream->zfree=0;
		stream->opaque=0;
		if(inflateInit2(stream,-MAX_WBITS)!=Z_OK)
			{
			delete[] compressedBuffer;
			delete stream;
			Misc::throwStdErr("IO::ZipArchiveStreamingFile: Internal zlib error while initializing decompression");
			}
		}
	}

ZipArchiveStreamingFile::~ZipArchiveStreamingFile(void)
	{
	delete[] compressedBuffer;
	delete stream;
	}

}

/**********************************************
Methods of class ZipArchive::DirectoryIterator:
**********************************************/

ZipArchive::DirectoryIterator::DirectoryIterator(const ZipArchive::DirectoryIterator& source)
	:FileID(source),
	 nextEntryPos(source.nextEntryPos),
	 fileNameBufferSize(source.fileNameBufferSize),fileName(fileNameBufferSize!=0?new char[fileNameBufferSize]:0)
	{
	if(fileNameBufferSize!=0)
		memcpy(fileName,source.fileName,fileNameBufferSize);
	}

ZipArchive::DirectoryIterator& ZipArchive::DirectoryIterator::operator=(const ZipArchive::DirectoryIterator& source)
	{
	if(this!=&source)
		{
		FileID::operator=(source);
		nextEntryPos=source.nextEntryPos;
		if(source.fileNameBufferSize==0)
			{
			delete[] fileName;
			fileNameBufferSize=0;
			fileName=0;
			}
		else if(source.fileNameBufferSize>fileNameBufferSize)
			{
			delete[] fileName;
			fileNameBufferSize=source.fileNameBufferSize;
			fileName=new char[fileNameBufferSize];
			}
		if(fileNameBufferSize!=0)
			memcpy(fileName,source.fileName,fileNameBufferSize);
		}
	return *this;
	}

/**********************************************
Methods of class ZipArchive::FileNotFoundError:
**********************************************/

ZipArchive::FileNotFoundError::FileNotFoundError(const char* sFileName)
	:std::runtime_error(Misc::printStdErrMsg("IO::ZipArchive::findFile: File %s not found in archive",sFileName)),
	 fileName(new char[strlen(sFileName)+1])
	{
	strcpy(fileName,sFileName);
	}

ZipArchive::FileNotFoundError::~FileNotFoundError(void) throw()
	{
	delete[] fileName;
	}

/***************************
Methods of class ZipArchive:
***************************/

int ZipArchive::initArchive(void)
	{
	/* Set the archive file's endianness: */
	archive->setEndianness(File::LittleEndian);
	
	/* Check the first local file header's signature, to check if it's a zip file in the first place: */
	unsigned int signature=archive->read<unsigned int>();
	if(signature!=0x04034b50U)
		return -1;
	
	/* Read backwards from end of file until end-of-directory signature is found: */
	Offset archiveSize=archive->getSize();
	Offset readPos=archiveSize;
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
				return -2;
			readPos-=Offset(readSize);
			archive->setReadPosAbs(readPos);
			archive->read(readBuffer,readSize);
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
	archive->setReadPosAbs(endOfCentralDirPos);
	unsigned int eocdSignature=archive->read<unsigned int>();
	if(eocdSignature!=0x06054b50U)
		return -3;
	
	/* Skip irrelevant bits: */
	archive->skip<unsigned short>(4);
	
	/* Read the relevant bits: */
	unsigned int eocdCDSize=archive->read<unsigned int>();
	unsigned int eocdCDOffset=archive->read<unsigned int>();
	unsigned short eocdCommentLength=archive->read<unsigned short>();
	
	/* Remember the directory offset and size: */
	directoryPos=Offset(eocdCDOffset);
	directorySize=size_t(eocdCDSize);
	
	/* Check again if this was really the end-of-directory marker: */
	if(directoryPos+Offset(directorySize)!=endOfCentralDirPos||endOfCentralDirPos+Offset(sizeof(unsigned int)*3+sizeof(unsigned short)*5+eocdCommentLength)!=archiveSize)
		return -3;
	
	/* Signal success: */
	return 0;
	}

ZipArchive::ZipArchive(const char* archiveFileName)
	:archive(new StandardFile(archiveFileName,File::ReadOnly))
	{
	/* Initialize the archive and handle errors: */
	switch(initArchive())
		{
		case -1:
			delete archive;
			Misc::throwStdErr("IO::ZipArchive: %s is not a valid ZIP archive",archiveFileName);
			break;
		
		case -2:
			delete archive;
			Misc::throwStdErr("IO::ZipArchive: Unable to locate central directory in ZIP archive %s",archiveFileName);
			break;
		
		case -3:
			delete archive;
			Misc::throwStdErr("IO::ZipArchive: Invalid central directory in ZIP archive %s",archiveFileName);
			break;
		}
	}

ZipArchive::ZipArchive(SeekableFile* sArchive)
	:archive(sArchive)
	{
	/* Initialize the archive and handle errors: */
	switch(initArchive())
		{
		case -1:
			delete archive;
			Misc::throwStdErr("IO::ZipArchive: Source file is not a valid ZIP archive");
			break;
		
		case -2:
			delete archive;
			Misc::throwStdErr("IO::ZipArchive: Unable to locate central directory in ZIP archive");
			break;
		
		case -3:
			delete archive;
			Misc::throwStdErr("IO::ZipArchive: Invalid central directory in ZIP archive");
			break;
		}
	}

ZipArchive::~ZipArchive(void)
	{
	delete archive;
	}

ZipArchive::DirectoryIterator ZipArchive::readDirectory(void)
	{
	/* Create a new directory iterator for the root directory: */
	DirectoryIterator dIt(directoryPos);
	
	/* Read the first proper directory entry: */
	return getNextEntry(dIt);
	}

ZipArchive::DirectoryIterator& ZipArchive::getNextEntry(ZipArchive::DirectoryIterator& dIt)
	{
	/* Check if the entry is already past the last entry: */
	if(dIt.nextEntryPos==0)
		return dIt; // Do nothing
	
	/* Read the next entry header: */
	archive->setReadPosAbs(dIt.nextEntryPos);
	unsigned int entryHeader=archive->read<unsigned int>();
	if(entryHeader==0x05054b50U||entryHeader==0x06054b50U) // Digital signature entry or end-of-central-directory entry
		{
		/* Invalidate and return the iterator: */
		dIt.filePos=~Offset(0);
		dIt.nextEntryPos=0;
		delete[] dIt.fileName;
		dIt.fileNameBufferSize=0;
		dIt.fileName=0;
		return dIt;
		}
	else if(entryHeader!=0x02014b50U) // File entry
		Misc::throwStdErr("IO::ZipArchive::getNextEntry: Bad entry header in central directory");
	
	/* Read the header: */
	archive->skip<unsigned short>(6);
	archive->skip<unsigned int>(1);
	unsigned int compressedSize=archive->read<unsigned int>();
	unsigned int uncompressedSize=archive->read<unsigned int>();
	unsigned short fileNameLength=archive->read<unsigned short>();
	unsigned short extraFieldLength=archive->read<unsigned short>();
	unsigned short fileCommentLength=archive->read<unsigned short>();
	archive->skip<unsigned short>(2);
	archive->skip<unsigned int>(1);
	unsigned int localHeaderOffset=archive->read<unsigned int>();
	
	/* Read the file name: */
	if(dIt.fileNameBufferSize<size_t(fileNameLength+1))
		{
		delete[] dIt.fileName;
		dIt.fileNameBufferSize=((fileNameLength+1)*11)/10+2;
		dIt.fileName=new char[dIt.fileNameBufferSize];
		}
	archive->read(dIt.fileName,fileNameLength);
	dIt.fileName[fileNameLength]='\0';
	
	/* Store file information: */
	dIt.filePos=Offset(localHeaderOffset);
	dIt.compressedSize=size_t(compressedSize);
	dIt.uncompressedSize=size_t(uncompressedSize);
	
	/* Skip extra field and file comment: */
	archive->skip<char>(extraFieldLength);
	archive->skip<char>(fileCommentLength);
	
	/* Store the next entry's offset: */
	dIt.nextEntryPos=archive->getReadPos();
	
	return dIt;
	}

ZipArchive::FileID ZipArchive::findFile(const char* fileName)
	{
	/* Read the archive's central directory: */
	for(DirectoryIterator dIt=readDirectory();dIt.isValid();getNextEntry(dIt))
		{
		/* Check if the file was found: */
		if(strcmp(dIt.getFileName(),fileName)==0)
			return dIt;
		}
	
	/* File was not found; throw exception: */
	throw FileNotFoundError(fileName);
	}

File* ZipArchive::openFile(const ZipArchive::FileID& fileId)
	{
	/* Read the file's header: */
	archive->setReadPosAbs(fileId.filePos);
	if(archive->read<unsigned int>()!=0x04034b50U)
		Misc::throwStdErr("IO::ZipArchive::openFile: Invalid file header signature");
	
	/* Read file header information: */
	archive->skip<unsigned short>(2);
	unsigned short compressionMethod=archive->read<unsigned short>();
	archive->skip<unsigned short>(2);
	archive->skip<unsigned int>(1);
	unsigned int compressedSize=archive->read<unsigned int>();
	archive->skip<unsigned int>(1);
	unsigned short fileNameLength=archive->read<unsigned short>();
	unsigned short extraFieldLength=archive->read<unsigned short>();
	
	/* Skip file name and extra field: */
	archive->skip<char>(fileNameLength);
	archive->skip<char>(extraFieldLength);
	
	/* Create and return the result file: */
	return new ZipArchiveStreamingFile(*archive,compressionMethod,archive->getReadPos(),compressedSize);
	}

SeekableFile* ZipArchive::openSeekableFile(const ZipArchive::FileID& fileId)
	{
	/* Read the file's header: */
	archive->setReadPosAbs(fileId.filePos);
	if(archive->read<unsigned int>()!=0x04034b50U)
		Misc::throwStdErr("IO::ZipArchive::openSeekableFile: Invalid file header signature");
	
	/* Read file header information: */
	archive->skip<unsigned short>(2);
	unsigned short compressionMethod=archive->read<unsigned short>();
	archive->skip<unsigned short>(2);
	archive->skip<unsigned int>(1);
	unsigned int compressedSize=archive->read<unsigned int>();
	unsigned int uncompressedSize=archive->read<unsigned int>();
	unsigned short fileNameLength=archive->read<unsigned short>();
	unsigned short extraFieldLength=archive->read<unsigned short>();
	
	/* Skip file name and extra field: */
	archive->skip<char>(fileNameLength);
	archive->skip<char>(extraFieldLength);
	
	/* Create the result file: */
	FixedMemoryFile* result=new FixedMemoryFile(uncompressedSize);
	if(compressionMethod==0)
		{
		/* Directly read the uncompressed data: */
		archive->read<char>(static_cast<char*>(result->getMemory()),compressedSize);
		}
	else
		{
		/* Read the compressed data: */
		Bytef* compressed=new Bytef[compressedSize];
		archive->read<Bytef>(compressed,compressedSize);
		
		/* Uncompress the data: */
		z_stream stream;
		stream.zalloc=0;
		stream.zfree=0;
		stream.opaque=0;
		if(inflateInit2(&stream,-MAX_WBITS)!=Z_OK)
			{
			delete[] compressed;
			delete result;
			Misc::throwStdErr("IO::ZipArchive::openSeekableFile: Internal zlib error");
			}
		stream.next_in=compressed;
		stream.avail_in=compressedSize;
		stream.next_out=static_cast<Bytef*>(result->getMemory());
		stream.avail_out=uncompressedSize;
		if(inflate(&stream,Z_FINISH)!=Z_STREAM_END)
			{
			delete[] compressed;
			delete result;
			Misc::throwStdErr("IO::ZipArchive::openSeekableFile: Internal zlib error");
			}
		delete[] compressed;
		if(inflateEnd(&stream)!=Z_OK)
			{
			delete result;
			Misc::throwStdErr("IO::ZipArchive::openSeekableFile: Internal zlib error");
			}
		}
	
	return result;
	}

}
