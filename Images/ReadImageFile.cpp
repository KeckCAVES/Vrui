/***********************************************************************
ReadImageFile - Functions to read RGB images from a variety of file
formats.
Copyright (c) 2005-2018 Oliver Kreylos

This file is part of the Image Handling Library (Images).

The Image Handling Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Image Handling Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Image Handling Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Images/ReadImageFile.h>

#include <Images/Config.h>

#include <ctype.h>
#include <string.h>
#include <Misc/Utility.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/FileNameExtensions.h>
#include <IO/SeekableFile.h>
#include <IO/OpenFile.h>
#include <IO/Directory.h>
#include <Images/ReadPNMImage.h>
#include <Images/ReadBILImage.h>
#include <Images/ReadPNGImage.h>
#include <Images/ReadJPEGImage.h>
#include <Images/ReadTIFFImage.h>

namespace Images {

namespace {

/*******************************************************************
Function to determine the format of an image file based on its name:
*******************************************************************/

enum ImageFileFormat
	{
	IFF_UNKNOWN,IFF_PNM,IFF_BIL,IFF_PNG,IFF_JPEG,IFF_TIFF
	};

int getImageFileFormat(const char* imageFileName)
	{
	/* Retrieve the file name extension: */
	const char* ext=Misc::getExtension(imageFileName);
	int extLen=strlen(ext);
	if(strcasecmp(ext,".gz")==0)
		{
		/* Strip the gzip extension and try again: */
		const char* gzExt=ext;
		ext=Misc::getExtension(imageFileName,gzExt);
		extLen=gzExt-ext;
		}
	
	/* Try to determine image file format from file name extension: */
	int iff=IFF_UNKNOWN;
	if(extLen==4
	   &&ext[0]=='.'
	   &&tolower(ext[1])=='p'
	   &&(tolower(ext[2])=='b'
	      ||tolower(ext[2])=='g'
	      ||tolower(ext[2])=='n'
	      ||tolower(ext[2])=='p')
	   &&tolower(ext[3])=='m') // It's a Portable AnyMap image
		iff=IFF_PNM;
	else if(extLen==4
	        &&ext[0]=='.'
	        &&tolower(ext[1])=='b'
	        &&((tolower(ext[2])=='i'
	            &&(tolower(ext[3])=='p'
	               ||tolower(ext[3])=='l'))
	           ||(tolower(ext[2])=='s'
	              &&tolower(ext[3])=='q'))) // It's a BIP/BIL/BSQ image
		iff=IFF_BIL;
	else if(extLen==4&&strncasecmp(ext,".png",extLen)==0) // It's a PNG image
		iff=IFF_PNG;
	else if((extLen==4&&strncasecmp(ext,".jpg",extLen)==0)
	        ||(extLen==5&&strncasecmp(ext,".jpeg",extLen)==0)) // It's a JPEG image
		iff=IFF_JPEG;
	else if((extLen==4&&strncasecmp(ext,".tif",extLen)==0)
	        ||(extLen==5&&strncasecmp(ext,".tiff",extLen)==0)) // It's a TIFF image
		iff=IFF_TIFF;
	
	return iff;
	}

}

/***************************************************************
Function to check whether the image file has a supported format:
***************************************************************/

bool canReadImageFileType(const char* imageFileName)
	{
	/* Retrieve the image file format: */
	int iff=getImageFileFormat(imageFileName);
	
	/* Check if the format is supported: */
	if(iff==IFF_PNM||iff==IFF_BIL)
		return true;
	#if IMAGES_CONFIG_HAVE_PNG
	if(iff==IFF_PNM)
		return true;
	#endif
	#if IMAGES_CONFIG_HAVE_JPEG
	if(iff==IFF_JPEG)
		return true;
	#endif
	#if IMAGES_CONFIG_HAVE_TIFF
	if(iff==IFF_TIFF)
		return true;
	#endif
	
	return false;
	}

/**********************************************************
Function to read images files in several supported formats:
**********************************************************/

RGBImage readImageFile(const char* imageFileName,IO::FilePtr file)
	{
	/* This is a legacy function; read generic image and convert it to RGB: */
	BaseImage result=readGenericImageFile(imageFileName,file);
	return Images::RGBImage(result.dropAlpha().toRgb());
	}

RGBImage readImageFile(const IO::Directory& directory,const char* imageFileName)
	{
	/* This is a legacy function; read generic image and convert it to RGB: */
	BaseImage result=readGenericImageFile(directory,imageFileName);
	return Images::RGBImage(result.dropAlpha().toRgb());
	}

RGBImage readImageFile(const char* imageFileName)
	{
	/* Read the image file through the current directory: */
	return readImageFile(*IO::Directory::getCurrent(),imageFileName);
	}

RGBAImage readTransparentImageFile(const char* imageFileName,IO::FilePtr file)
	{
	/* This is a legacy function; read generic image and convert it to RGBA: */
	BaseImage result=readGenericImageFile(imageFileName,file);
	return Images::RGBAImage(result.addAlpha(1.0).toRgb());
	}

RGBAImage readTransparentImageFile(const IO::Directory& directory,const char* imageFileName)
	{
	/* This is a legacy function; read generic image and convert it to RGBA: */
	BaseImage result=readGenericImageFile(directory,imageFileName);
	return Images::RGBAImage(result.addAlpha(1.0).toRgb());
	}

RGBAImage readTransparentImageFile(const char* imageFileName)
	{
	/* Read the image file through the current directory: */
	return readTransparentImageFile(*IO::Directory::getCurrent(),imageFileName);
	}

BaseImage readGenericImageFile(const char* imageFileName,IO::FilePtr file)
	{
	/* Retrieve the image file's format: */
	int iff=getImageFileFormat(imageFileName);
	
	BaseImage result;
	if(iff==IFF_BIL)
		{
		/* Can't read BIL files through a naked file, as we need a header file: */
		throw std::runtime_error("Images::readGenericImageFile: Cannot read BIP/BIL/BSQ image files through an already-open file");
		}
	else
		{
		/* Delegate to the appropriate image reader function: */
		switch(iff)
			{
			case IFF_PNM:
				/* Read a generic PNM image from the given file: */
				result=readGenericPNMImage(imageFileName,*file);
				break;
			
			#if IMAGES_CONFIG_HAVE_PNG
			case IFF_PNG:
				/* Read a generic PNG image from the given file: */
				result=readGenericPNGImage(imageFileName,*file);
				break;
			#endif
			
			#if IMAGES_CONFIG_HAVE_JPEG
			case IFF_JPEG:
				/* Read a generic JPEG image from the given file: */
				result=readGenericJPEGImage(imageFileName,*file);
				break;
			#endif
			
			#if IMAGES_CONFIG_HAVE_TIFF
			case IFF_TIFF:
				/* Read a generic TIFF image from the given file: */
				result=readGenericTIFFImage(imageFileName,*file);
				break;
			#endif
			
			default:
				Misc::throwStdErr("Images::readGenericImageFile: Unknown extension in image file name \"%s\"",imageFileName);
			}
		}
	
	return result;
	}

BaseImage readGenericImageFile(const IO::Directory& directory,const char* imageFileName)
	{
	/* Retrieve the image file's format: */
	int iff=getImageFileFormat(imageFileName);
	
	BaseImage result;
	if(iff==IFF_BIL)
		{
		/* Read a generic BIL image from the given directory: */
		result=readGenericBILImage(directory,imageFileName);
		}
	else
		{
		/* Open the image file: */
		IO::FilePtr imageFile=directory.openFile(imageFileName);
		
		/* Delegate to the appropriate image reader function: */
		switch(iff)
			{
			case IFF_PNM:
				/* Read a generic PNM image from the given file: */
				result=readGenericPNMImage(imageFileName,*imageFile);
				break;
			
			#if IMAGES_CONFIG_HAVE_PNG
			case IFF_PNG:
				/* Read a generic PNG image from the given file: */
				result=readGenericPNGImage(imageFileName,*imageFile);
				break;
			#endif
			
			#if IMAGES_CONFIG_HAVE_JPEG
			case IFF_JPEG:
				/* Read a generic JPEG image from the given file: */
				result=readGenericJPEGImage(imageFileName,*imageFile);
				break;
			#endif
			
			#if IMAGES_CONFIG_HAVE_TIFF
			case IFF_TIFF:
				/* Read a generic TIFF image from the given file: */
				result=readGenericTIFFImage(imageFileName,*imageFile);
				break;
			#endif
			
			default:
				Misc::throwStdErr("Images::readGenericImageFile: Unknown extension in image file name \"%s\"",imageFileName);
			}
		}
	
	return result;
	}

BaseImage readGenericImageFile(const char* imageFileName)
	{
	/* Call the general function with the current directory: */
	return readGenericImageFile(*IO::Directory::getCurrent(),imageFileName);
	}

namespace {

/********************************************
Helper structures for the cursor file reader:
********************************************/

struct CursorFileHeader
	{
	/* Elements: */
	public:
	unsigned int magic;
	unsigned int headerSize;
	unsigned int version;
	unsigned int numTOCEntries;
	};

struct CursorTOCEntry
	{
	/* Elements: */
	public:
	unsigned int chunkType;
	unsigned int chunkSubtype;
	unsigned int chunkPosition;
	};

struct CursorCommentChunkHeader
	{
	/* Elements: */
	public:
	unsigned int headerSize;
	unsigned int chunkType; // 0xfffe0001U
	unsigned int chunkSubtype;
	unsigned int version;
	unsigned int commentLength;
	/* Comment characters follow */
	};

struct CursorImageChunkHeader
	{
	/* Elements: */
	public:
	unsigned int headerSize;
	unsigned int chunkType; // 0xfffd0002U
	unsigned int chunkSubtype;
	unsigned int version;
	unsigned int size[2];
	unsigned int hotspot[2];
	unsigned int delay;
	/* ARGB pixel data follows */
	};

}

/***********************************************
Function to read cursor files in Xcursor format:
***********************************************/

RGBAImage readCursorFile(const char* cursorFileName,IO::FilePtr file,unsigned int nominalSize,unsigned int* hotspot)
	{
	/* Read the magic value to determine file endianness: */
	size_t filePos=0;
	CursorFileHeader fh;
	fh.magic=file->read<unsigned int>();
	filePos+=sizeof(unsigned int);
	if(fh.magic==0x58637572U)
		file->setSwapOnRead(true);
	else if(fh.magic!=0x72756358U)
		Misc::throwStdErr("Images::readCursorFile: Invalid cursor file header in \"%s\"",cursorFileName);
	
	/* Read the rest of the file header: */
	fh.headerSize=file->read<unsigned int>();
	filePos+=sizeof(unsigned int);
	fh.version=file->read<unsigned int>();
	filePos+=sizeof(unsigned int);
	fh.numTOCEntries=file->read<unsigned int>();
	filePos+=sizeof(unsigned int);
	
	/* Read the table of contents: */
	size_t imageChunkOffset=0;
	for(unsigned int i=0;i<fh.numTOCEntries;++i)
		{
		CursorTOCEntry te;
		te.chunkType=file->read<unsigned int>();
		filePos+=sizeof(unsigned int);
		te.chunkSubtype=file->read<unsigned int>();
		filePos+=sizeof(unsigned int);
		te.chunkPosition=file->read<unsigned int>();
		filePos+=sizeof(unsigned int);
		
		if(te.chunkType==0xfffd0002U&&te.chunkSubtype==nominalSize)
			{
			imageChunkOffset=size_t(te.chunkPosition);
			break;
			}
		}
	
	if(imageChunkOffset==0)
		Misc::throwStdErr("Images::readCursorFile: No matching image found in \"%s\"",cursorFileName);
	
	/* Skip ahead to the beginning of the image chunk: */
	file->skip<char>(imageChunkOffset-filePos);
	
	/* Read the image chunk: */
	CursorImageChunkHeader ich;
	ich.headerSize=file->read<unsigned int>();
	ich.chunkType=file->read<unsigned int>();
	ich.chunkSubtype=file->read<unsigned int>();
	ich.version=file->read<unsigned int>();
	for(int i=0;i<2;++i)
		ich.size[i]=file->read<unsigned int>();
	for(int i=0;i<2;++i)
		ich.hotspot[i]=file->read<unsigned int>();
	if(hotspot!=0)
		{
		for(int i=0;i<2;++i)
			hotspot[i]=ich.hotspot[i];
		}
	ich.delay=file->read<unsigned int>();
	if(ich.headerSize!=9*sizeof(unsigned int)||ich.chunkType!=0xfffd0002U||ich.version!=1)
		Misc::throwStdErr("Images::readCursorFile: Invalid image chunk header in \"%s\"",cursorFileName);
	
	/* Create the result image: */
	RGBAImage result(ich.size[0],ich.size[1]);
	
	/* Read the image row-by-row: */
	for(unsigned int row=result.getHeight();row>0;--row)
		{
		RGBAImage::Color* rowPtr=result.modifyPixelRow(row-1);
		file->read(rowPtr->getRgba(),result.getWidth()*4);
		
		/* Convert BGRA data into RGBA data: */
		for(unsigned int i=0;i<result.getWidth();++i)
			Misc::swap(rowPtr[i][0],rowPtr[i][2]);
		}
	
	/* Return the result image: */
	return result;
	}

RGBAImage readCursorFile(const IO::Directory& directory,const char* cursorFileName,unsigned int nominalSize,unsigned int* hotspot)
	{
	/* Open the cursor file: */
	IO::FilePtr file=directory.openFile(cursorFileName);
	
	/* Call the general method: */
	return readCursorFile(cursorFileName,file,nominalSize,hotspot);
	}

RGBAImage readCursorFile(const char* cursorFileName,unsigned int nominalSize,unsigned int* hotspot)
	{
	/* Open the cursor file: */
	IO::FilePtr file=IO::Directory::getCurrent()->openFile(cursorFileName);
	
	/* Call the general method: */
	return readCursorFile(cursorFileName,file,nominalSize,hotspot);
	}

}
