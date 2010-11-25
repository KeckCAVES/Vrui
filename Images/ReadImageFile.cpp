/***********************************************************************
ReadImageFile - Functions to read RGB images from a variety of file
formats.
Copyright (c) 2005-2010 Oliver Kreylos

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
#include <stdio.h>
#include <string.h>
#if IMAGES_CONFIG_HAVE_PNG
#include <png.h>
#endif
#if IMAGES_CONFIG_HAVE_JPEG
#include <jpeglib.h>
#endif
#if IMAGES_CONFIG_HAVE_TIFF
#include <stdarg.h>
#include <stdexcept>
#include <tiffio.h>
#endif
#include <Misc/Utility.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/File.h>
#include <Math/Constants.h>

namespace Images {

namespace {

/***********************************
Helper functions for the PNM reader:
***********************************/

void readRawPbmFile(RGBImage& image,Misc::File& file)
	{
	/* Create an intermediate buffer: */
	unsigned int rowLen=(image.getWidth()+7)>>3;
	unsigned char* tempRow=new unsigned char[rowLen];
	
	/* Read each row of the image file: */
	for(unsigned int y=image.getHeight();y>0;--y)
		{
		RGBImage::Color* row=image.modifyPixelRow(y-1);
		file.read(tempRow,rowLen);
		
		/* Convert pixel values: */
		for(unsigned int x=0;x<image.getWidth();++x)
			{
			unsigned int bit=x&0x7;
			unsigned int byte=x>>3;
			RGBImage::Scalar value=(tempRow[byte]>>(7-bit))?255:0;
			row[x][0]=row[x][1]=row[x][2]=value;
			}
		}
	
	delete[] tempRow;
	}

void readRawPgmFile(RGBImage& image,Misc::File& file,unsigned int maxVal)
	{
	if(maxVal<256)
		{
		/* Create an intermediate buffer: */
		unsigned char* tempRow=new unsigned char[image.getWidth()];
		
		/* Read each row of the image file: */
		for(unsigned int y=image.getHeight();y>0;--y)
			{
			RGBImage::Color* row=image.modifyPixelRow(y-1);
			file.read(tempRow,image.getWidth());
			
			/* Convert pixel values: */
			for(unsigned int x=0;x<image.getWidth();++x)
				row[x][0]=row[x][1]=row[x][2]=RGBImage::Scalar(tempRow[x]);
			}
		
		delete[] tempRow;
		}
	else
		{
		/* Create an intermediate buffer: */
		unsigned short* tempRow=new unsigned short[image.getWidth()];
		
		/* Read each row of the image file: */
		for(unsigned int y=image.getHeight();y>0;--y)
			{
			RGBImage::Color* row=image.modifyPixelRow(y-1);
			file.read(tempRow,image.getWidth());
			
			/* Convert pixel values: */
			for(unsigned int x=0;x<image.getWidth();++x)
				row[x][0]=row[x][1]=row[x][2]=RGBImage::Scalar(tempRow[x]>>8);
			}
		
		delete[] tempRow;
		}
	}

void readRawPpmFile(RGBImage& image,Misc::File& file)
	{
	/* Read each row of the image file: */
	for(unsigned int y=image.getHeight();y>0;--y)
		{
		RGBImage::Color* row=image.modifyPixelRow(y-1);
		file.read(row,image.getWidth());
		}
	}

/*********************************************************
Function to read PNM image files in arbitrary sub-formats:
*********************************************************/

RGBImage readPnmFile(const char* imageFileName)
	{
	enum PnmFileFormat
		{
		PBM_RAW=0,PGM_RAW,PPM_RAW
		};
	
	/* Open the image file: */
	Misc::File pnmFile(imageFileName,"rb");
	char line[80];
	
	/* Check the PNM file type: */
	pnmFile.gets(line,sizeof(line));
	if(line[0]!='P'||line[1]<'4'||line[1]>'6'||line[2]!='\n')
		Misc::throwStdErr("Images::readPnmFile: illegal PNM header in image file \"%s\"",imageFileName);
	int pnmFileFormat=line[1]-'4';
	
	/* Skip any comment lines in the PNM header: */
	do
		{
		pnmFile.gets(line,sizeof(line));
		}
	while(line[0]=='#');
	
	/* Parse the image size: */
	unsigned int imageSize[2];
	sscanf(line,"%u %u",&imageSize[0],&imageSize[1]);
	
	/* Parse the max value field: */
	unsigned int maxVal=1;
	if(pnmFileFormat!=PBM_RAW)
		{
		pnmFile.gets(line,sizeof(line));
		sscanf(line,"%u",&maxVal);
		}
	
	/* Create the result image: */
	RGBImage result(imageSize[0],imageSize[1]);
	
	/* Read the image: */
	switch(pnmFileFormat)
		{
		case PBM_RAW:
			readRawPbmFile(result,pnmFile);
			break;
		
		case PGM_RAW:
			readRawPgmFile(result,pnmFile,maxVal);
			break;
		
		case PPM_RAW:
			readRawPpmFile(result,pnmFile);
			break;
		}
	
	/* Return the result image: */
	return result;
	}

#if IMAGES_CONFIG_HAVE_PNG

/********************************
Function to read PNG image files:
********************************/

RGBImage readPngFile(const char* imageFileName)
	{
	/* Open input file: */
	Misc::File pngFile(imageFileName,"rb");
	
	/* Check for PNG file signature: */
	unsigned char pngSignature[8];
	pngFile.read(pngSignature,8);
	if(!png_check_sig(pngSignature,8))
		Misc::throwStdErr("Images::readPngFile: illegal PNG header in image file \"%s\"",imageFileName);
	
	/* Allocate the PNG library data structures: */
	png_structp pngReadStruct=png_create_read_struct(PNG_LIBPNG_VER_STRING,0,0,0);
	if(pngReadStruct==0)
		Misc::throwStdErr("Images::readPngFile: Internal error in PNG library");
	png_infop pngInfoStruct=png_create_info_struct(pngReadStruct);
	if(pngInfoStruct==0)
		{
		png_destroy_read_struct(&pngReadStruct,0,0);
		Misc::throwStdErr("Images::readPngFile: Internal error in PNG library");
		}
	
	/* Set up longjump facility for PNG error handling (ouch): */
	if(setjmp(png_jmpbuf(pngReadStruct)))
		{
		png_destroy_read_struct(&pngReadStruct,&pngInfoStruct,0);
		Misc::throwStdErr("Images::readPngFile: Error while setting up PNG library error handling");
		}
	
	/* Initialize PNG I/O: */
	png_init_io(pngReadStruct,pngFile.getFilePtr());
	
	/* Read PNG image header: */
	png_set_sig_bytes(pngReadStruct,8);
	png_read_info(pngReadStruct,pngInfoStruct);
	png_uint_32 imageSize[2];
	int elementSize;
	int colorType;
	png_get_IHDR(pngReadStruct,pngInfoStruct,&imageSize[0],&imageSize[1],&elementSize,&colorType,0,0,0);
	
	/* Set up image processing: */
	if(colorType==PNG_COLOR_TYPE_PALETTE)
		png_set_expand(pngReadStruct);
	else if(colorType==PNG_COLOR_TYPE_GRAY&&elementSize<8)
		png_set_expand(pngReadStruct);
	if(elementSize==16)
		png_set_strip_16(pngReadStruct);
	if(colorType==PNG_COLOR_TYPE_GRAY||colorType==PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(pngReadStruct);
	if(colorType==PNG_COLOR_TYPE_GRAY_ALPHA||colorType==PNG_COLOR_TYPE_RGB_ALPHA)
		png_set_strip_alpha(pngReadStruct);
	double gamma;
	if(png_get_gAMA(pngReadStruct,pngInfoStruct,&gamma))
		png_set_gamma(pngReadStruct,2.2,gamma);
	png_read_update_info(pngReadStruct,pngInfoStruct);
	
	/* Create the result image: */
	RGBImage result(imageSize[0],imageSize[1]);
	
	/* Create row pointers to flip the image during reading: */
	RGBImage::Color** rowPointers=new RGBImage::Color*[result.getHeight()];
	for(unsigned int y=0;y<result.getHeight();++y)
		rowPointers[y]=result.modifyPixelRow(result.getHeight()-1-y);
	
	/* Read the PNG image: */
	png_read_image(pngReadStruct,reinterpret_cast<png_byte**>(rowPointers));
	
	/* Finish reading image: */
	png_read_end(pngReadStruct,0);
	
	/* Clean up: */
	delete[] rowPointers;
	png_destroy_read_struct(&pngReadStruct,&pngInfoStruct,0);
	
	/* Return the read image: */
	return result;
	}

#endif

#if IMAGES_CONFIG_HAVE_JPEG

/***************************************************
Helper structures and functions for the JPEG reader:
***************************************************/

struct ExceptionErrorManager:public jpeg_error_mgr
	{
	/* Constructors and destructors: */
	public:
	ExceptionErrorManager(void)
		{
		/* Set the method pointer(s) in the base class object: */
		jpeg_std_error(this);
		error_exit=exit;
		}
	
	/* Methods: */
	static void exit(j_common_ptr cinfo) // Method called when JPEG library encounters a fatal error
		{
		/* Get pointer to the error manager object: */
		// ExceptionErrorManager* myManager=static_cast<ExceptionErrorManager*>(cinfo->err);
		
		/* Throw an exception: */
		Misc::throwStdErr("Images::readJpegFile: JPEG library encountered fatal error");
		}
	};

/**************************************************************
Function to read JPEG (actually, JFIF interchange) image files:
**************************************************************/

RGBImage readJpegFile(const char* imageFileName)
	{
	/* Open input file: */
	Misc::File jpegFile(imageFileName,"rb");
	
	/* Create a JPEG error handler and a JPEG decompression object: */
	ExceptionErrorManager jpegErrorManager;
	jpeg_decompress_struct jpegDecompressStruct;
	jpegDecompressStruct.err=&jpegErrorManager;
	jpeg_create_decompress(&jpegDecompressStruct);
	RGBImage::Color** rowPointers=0;
	
	try
		{
		/* Associate the decompression object with the input file: */
		jpeg_stdio_src(&jpegDecompressStruct,jpegFile.getFilePtr());
		
		/* Read the JPEG file header: */
		jpeg_read_header(&jpegDecompressStruct,true);
		
		/* Prepare for decompression: */
		jpeg_start_decompress(&jpegDecompressStruct);
		
		/* Create the result image: */
		RGBImage result(jpegDecompressStruct.output_width,jpegDecompressStruct.output_height);
		
		/* Create row pointers to flip the image during reading: */
		rowPointers=new RGBImage::Color*[result.getHeight()];
		for(unsigned int y=0;y<result.getHeight();++y)
			rowPointers[y]=result.modifyPixelRow(result.getHeight()-1-y);
		
		/* Read the JPEG image's scan lines: */
		JDIMENSION scanline=0;
		while(scanline<jpegDecompressStruct.output_height)
			scanline+=jpeg_read_scanlines(&jpegDecompressStruct,reinterpret_cast<JSAMPLE**>(rowPointers+scanline),jpegDecompressStruct.output_height-scanline);
		
		/* Finish reading image: */
		jpeg_finish_decompress(&jpegDecompressStruct);
		
		/* Clean up: */
		delete[] rowPointers;
		jpeg_destroy_decompress(&jpegDecompressStruct);
		
		/* Return the result image: */
		return result;
		}
	catch(...)
		{
		/* Clean up: */
		delete[] rowPointers;
		jpeg_destroy_decompress(&jpegDecompressStruct);
		
		/* Re-throw the exception: */
		throw;
		}
	}

#endif

#if IMAGES_CONFIG_HAVE_TIFF

/*********************************
Function to read TIFF image files:
*********************************/

void TiffErrorHandler(const char* module,const char* fmt,va_list ap)
	{
	char msg[1024];
	vsnprintf(msg,sizeof(msg),fmt,ap);
	std::string err="Images::readTiffFile: ";
	err.append(msg);
	throw std::runtime_error(err);
	}

RGBImage readTiffFile(const char* imageFileName)
	{
	/* Open the TIFF image: */
	TIFF* image=TIFFOpen(imageFileName,"r");
	if(image==0)
		Misc::throwStdErr("Images::readTiffFile: Unable to open image file %s",imageFileName);
	
	/* Set the TIFF error handler: */
	TIFFSetErrorHandler(TiffErrorHandler);
	
	/* Get the image size: */
	uint32 width,height;
	TIFFGetField(image,TIFFTAG_IMAGEWIDTH,&width);
	TIFFGetField(image,TIFFTAG_IMAGELENGTH,&height);
	
	/* Create the result image: */
	RGBImage result(width,height);
	
	/* Allocate a temporary RGBA buffer: */
	uint32* rgbaBuffer=new uint32[height*width];
	
	try
		{
		/* Read the TIFF image into the temporary buffer: */
		if(!TIFFReadRGBAImage(image,width,height,rgbaBuffer))
			Misc::throwStdErr("Images::readTiffFile: Error while reading image file %s",imageFileName);
		
		/* Copy the RGB image data into the result image: */
		uint32* sPtr=rgbaBuffer;
		RGBImage::Color* dPtr=result.modifyPixels();
		for(uint32 y=0;y<height;++y)
			for(uint32 x=0;x<width;++x,++sPtr,++dPtr)
				{
				(*dPtr)[0]=RGBImage::Scalar(TIFFGetR(*sPtr));
				(*dPtr)[1]=RGBImage::Scalar(TIFFGetG(*sPtr));
				(*dPtr)[2]=RGBImage::Scalar(TIFFGetB(*sPtr));
				}
		}
	catch(...)
		{
		/* Clean up: */
		delete[] rgbaBuffer;
		TIFFClose(image);
		
		/* Re-throw the exception: */
		throw;
		}
	
	/* Clean up and return the result image: */
	delete[] rgbaBuffer;
	TIFFClose(image);
	return result;
	}

#endif

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

/***************************************************************
Function to check whether the image file has a supported format:
***************************************************************/

bool canReadImageFileType(const char* imageFileName)
	{
	/* Try to determine image file format from file name extension: */
	
	/* Find position of last dot in image file name: */
	const char* extStart=0;
	const char* cPtr;
	for(cPtr=imageFileName;*cPtr!='\0';++cPtr)
		if(*cPtr=='.')
			extStart=cPtr+1;
	if(extStart==0)
		return false;
	
	if(cPtr-extStart==3&&tolower(extStart[0])=='p'&&tolower(extStart[2])=='m'&&
	   (tolower(extStart[1])=='b'||tolower(extStart[1])=='g'||tolower(extStart[1])=='n'||tolower(extStart[1])=='p'))
		return true;
	#if IMAGES_CONFIG_HAVE_PNG
	else if(strcasecmp(extStart,"png")==0)
		return true;
	#endif
	#if IMAGES_CONFIG_HAVE_JPEG
	else if(strcasecmp(extStart,"jpg")==0||strcasecmp(extStart,"jpeg")==0)
		return true;
	#endif
	#if IMAGES_CONFIG_HAVE_TIFF
	else if(strcasecmp(extStart,"tif")==0||strcasecmp(extStart,"tiff")==0)
		return true;
	#endif
	else
		return false;
	}

/**********************************************************
Function to read images files in several supported formats:
**********************************************************/

RGBImage readImageFile(const char* imageFileName)
	{
	/* Try to determine image file format from file name extension: */
	
	/* Find position of last dot in image file name: */
	const char* extStart=0;
	const char* cPtr;
	for(cPtr=imageFileName;*cPtr!='\0';++cPtr)
		if(*cPtr=='.')
			extStart=cPtr+1;
	if(extStart==0)
		Misc::throwStdErr("Images::readImageFile: no extension in image file name \"%s\"",imageFileName);
	
	if(cPtr-extStart==3&&tolower(extStart[0])=='p'&&tolower(extStart[2])=='m'&&
	   (tolower(extStart[1])=='b'||tolower(extStart[1])=='g'||tolower(extStart[1])=='n'||tolower(extStart[1])=='p'))
		return readPnmFile(imageFileName);
	#if IMAGES_CONFIG_HAVE_PNG
	else if(strcasecmp(extStart,"png")==0)
		return readPngFile(imageFileName);
	#endif
	#if IMAGES_CONFIG_HAVE_JPEG
	else if(strcasecmp(extStart,"jpg")==0||strcasecmp(extStart,"jpeg")==0)
		return readJpegFile(imageFileName);
	#endif
	#if IMAGES_CONFIG_HAVE_TIFF
	else if(strcasecmp(extStart,"tif")==0||strcasecmp(extStart,"tiff")==0)
		return readTiffFile(imageFileName);
	#endif
	else
		{
		Misc::throwStdErr("Images::readImageFile: unknown extension in image file name \"%s\"",imageFileName);
		return RGBImage(); // Dummy statement just to make the compiler happy
		}
	}

/***********************************************
Function to read cursor files in Xcursor format:
***********************************************/

RGBAImage readCursorFile(const char* cursorFileName,unsigned int nominalSize)
	{
	/* Open the input file: */
	Misc::File cursorFile(cursorFileName,"rb",Misc::File::LittleEndian);
	
	/* Read the magic value to determine file endianness: */
	CursorFileHeader fh;
	fh.magic=cursorFile.read<unsigned int>();
	if(fh.magic==0x58637572U)
		cursorFile.setEndianness(Misc::File::BigEndian);
	else if(fh.magic!=0x72756358U)
		Misc::throwStdErr("Images::readCursorFile: Invalid cursor file header in %s",cursorFileName);
	
	/* Read the rest of the file header: */
	fh.headerSize=cursorFile.read<unsigned int>();
	fh.version=cursorFile.read<unsigned int>();
	fh.numTOCEntries=cursorFile.read<unsigned int>();
	
	/* Read the table of contents: */
	Misc::File::Offset imageChunkOffset=0;
	for(unsigned int i=0;i<fh.numTOCEntries;++i)
		{
		CursorTOCEntry te;
		te.chunkType=cursorFile.read<unsigned int>();
		te.chunkSubtype=cursorFile.read<unsigned int>();
		te.chunkPosition=cursorFile.read<unsigned int>();
		
		if(te.chunkType==0xfffd0002U&&te.chunkSubtype==nominalSize)
			{
			imageChunkOffset=Misc::File::Offset(te.chunkPosition);
			break;
			}
		}
	
	if(imageChunkOffset==0)
		Misc::throwStdErr("Images::readCursorFile: No matching image found in %s",cursorFileName);
	
	/* Read the image chunk: */
	cursorFile.seekSet(imageChunkOffset);
	CursorImageChunkHeader ich;
	ich.headerSize=cursorFile.read<unsigned int>();
	ich.chunkType=cursorFile.read<unsigned int>();
	ich.chunkSubtype=cursorFile.read<unsigned int>();
	ich.version=cursorFile.read<unsigned int>();
	for(int i=0;i<2;++i)
		ich.size[i]=cursorFile.read<unsigned int>();
	for(int i=0;i<2;++i)
		ich.hotspot[i]=cursorFile.read<unsigned int>();
	ich.delay=cursorFile.read<unsigned int>();
	if(ich.headerSize!=9*sizeof(unsigned int)||ich.chunkType!=0xfffd0002U||ich.version!=1)
		Misc::throwStdErr("Images::readCursorFile: Invalid image chunk header in %s",cursorFileName);
	
	/* Create the result image: */
	RGBAImage result(ich.size[0],ich.size[1]);
	
	/* Read the image row-by-row: */
	for(unsigned int row=result.getHeight();row>0;--row)
		{
		RGBAImage::Color* rowPtr=result.modifyPixelRow(row-1);
		cursorFile.read(reinterpret_cast<GLubyte*>(rowPtr),result.getWidth()*4);
		
		/* Convert BGRA data into RGBA data: */
		for(unsigned int i=0;i<result.getWidth();++i)
			Misc::swap(rowPtr[i][0],rowPtr[i][2]);
		}
	
	/* Return the result image: */
	return result;
	}

}
