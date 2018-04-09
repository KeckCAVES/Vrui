/***********************************************************************
ReadTIFFImage - Functions to read RGB images from image files in TIFF
formats over an IO::SeekableFile abstraction.
Copyright (c) 2011-2017 Oliver Kreylos

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

#include <Images/ReadTIFFImage.h>

#include <Images/Config.h>

#if IMAGES_CONFIG_HAVE_TIFF

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <tiffio.h>
#include <stdexcept>
#include <Misc/SizedTypes.h>
#include <Misc/SelfDestructArray.h>
#include <Misc/ThrowStdErr.h>
#include <IO/File.h>
#include <IO/SeekableFile.h>
#include <IO/SeekableFilter.h>
#include <GL/gl.h>
#include <Images/BaseImage.h>
#include <Images/RGBImage.h>
#include <Images/RGBAImage.h>

namespace Images {

namespace {

/*************************************************************
Helper functions to read TIFF images from an IO::SeekableFile:
*************************************************************/

void tiffErrorFunction(const char* module,const char* fmt,va_list ap)
	{
	/* Throw an exception with the error message: */
	char msg[1024];
	vsnprintf(msg,sizeof(msg),fmt,ap);
	throw std::runtime_error(msg);
	}

void tiffWarningFunction(const char* module,const char* fmt,va_list ap)
	{
	/* Ignore warnings */
	}

tsize_t tiffReadFunction(thandle_t handle,tdata_t buffer,tsize_t size)
	{
	IO::SeekableFile* source=static_cast<IO::SeekableFile*>(handle);
	
	/* Libtiff expects to always get the amount of data it wants: */
	source->readRaw(buffer,size);
	return size;
	}

tsize_t tiffWriteFunction(thandle_t handle,tdata_t buffer,tsize_t size)
	{
	/* Ignore silently */
	return size;
	}

toff_t tiffSeekFunction(thandle_t handle,toff_t offset,int whence)
	{
	IO::SeekableFile* source=static_cast<IO::SeekableFile*>(handle);
	
	/* Seek to the requested position: */
	switch(whence)
		{
		case SEEK_SET:
			source->setReadPosAbs(offset);
			break;

		case SEEK_CUR:
			source->setReadPosRel(offset);
			break;

		case SEEK_END:
			source->setReadPosAbs(source->getSize()-offset);
			break;
		}
	
	return source->getReadPos();
	}

int tiffCloseFunction(thandle_t handle)
	{
	/* Ignore silently */
	return 0;
	}

toff_t tiffSizeFunction(thandle_t handle)
	{
	IO::SeekableFile* source=static_cast<IO::SeekableFile*>(handle);
	
	return source->getSize();
	}

int tiffMapFileFunction(thandle_t handle,tdata_t* buffer,toff_t* size)
	{
	/* Ignore silently */
	return -1;
	}

void tiffUnmapFileFunction(thandle_t handle,tdata_t buffer,toff_t size)
	{
	/* Ignore silently */
	}

}

RGBImage readTIFFImage(const char* imageName,IO::File& source)
	{
	/* Check if the source file is seekable: */
	IO::SeekableFilePtr seekableSource(&source);
	if(seekableSource==0)
		{
		/* Create a seekable filter for the source file: */
		seekableSource=new IO::SeekableFilter(&source);
		}
	
	/* Set the TIFF error handler: */
	TIFFSetErrorHandler(tiffErrorFunction);
	TIFFSetWarningHandler(tiffWarningFunction);
	
	TIFF* tiff=0;
	RGBImage result;
	uint32* rgbaBuffer=0;
	try
		{
		/* Pretend to open the TIFF file and register the hook functions: */
		tiff=TIFFClientOpen(imageName,"rm",seekableSource.getPointer(),tiffReadFunction,tiffWriteFunction,tiffSeekFunction,tiffCloseFunction,tiffSizeFunction,tiffMapFileFunction,tiffUnmapFileFunction);
		if(tiff==0)
			throw std::runtime_error("Error while opening image");
		
		/* Get the image size: */
		uint32 width,height;
		TIFFGetField(tiff,TIFFTAG_IMAGEWIDTH,&width);
		TIFFGetField(tiff,TIFFTAG_IMAGELENGTH,&height);
		
		/* Create the result image: */
		result=RGBImage(width,height);
		
		/* Allocate a temporary RGBA buffer: */
		rgbaBuffer=new uint32[height*width];
		
		/* Read the TIFF image into the temporary buffer: */
		if(!TIFFReadRGBAImage(tiff,width,height,rgbaBuffer))
			throw std::runtime_error("Error while reading image");
		
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
	catch(std::runtime_error err)
		{
		/* Clean up: */
		delete[] rgbaBuffer;
		if(tiff!=0)
			TIFFClose(tiff);
		
		/* Wrap and re-throw the exception: */
		Misc::throwStdErr("Images::readTIFFImage: Caught exception \"%s\" while reading image \"%s\"",err.what(),imageName);
		}
	
	/* Clean up and return the result image: */
	delete[] rgbaBuffer;
	if(tiff!=0)
		TIFFClose(tiff);
	return result;
	}

RGBAImage readTransparentTIFFImage(const char* imageName,IO::File& source)
	{
	/* Check if the source file is seekable: */
	IO::SeekableFilePtr seekableSource(&source);
	if(seekableSource==0)
		{
		/* Create a seekable filter for the source file: */
		seekableSource=new IO::SeekableFilter(&source);
		}
	
	/* Set the TIFF error handler: */
	TIFFSetErrorHandler(tiffErrorFunction);
	
	TIFF* tiff=0;
	RGBAImage result;
	uint32* rgbaBuffer=0;
	try
		{
		/* Pretend to open the TIFF file and register the hook functions: */
		tiff=TIFFClientOpen(imageName,"rm",seekableSource.getPointer(),tiffReadFunction,tiffWriteFunction,tiffSeekFunction,tiffCloseFunction,tiffSizeFunction,tiffMapFileFunction,tiffUnmapFileFunction);
		if(tiff==0)
			throw std::runtime_error("Error while opening image");
		
		/* Get the image size: */
		uint32 width,height;
		TIFFGetField(tiff,TIFFTAG_IMAGEWIDTH,&width);
		TIFFGetField(tiff,TIFFTAG_IMAGELENGTH,&height);
		
		/* Create the result image: */
		result=RGBAImage(width,height);
		
		/* Allocate a temporary RGBA buffer: */
		rgbaBuffer=new uint32[height*width];
		
		/* Read the TIFF image into the temporary buffer: */
		if(!TIFFReadRGBAImage(tiff,width,height,rgbaBuffer))
			throw std::runtime_error("Error while reading image");
		
		/* Copy the RGBA image data into the result image: */
		uint32* sPtr=rgbaBuffer;
		RGBAImage::Color* dPtr=result.modifyPixels();
		for(uint32 y=0;y<height;++y)
			for(uint32 x=0;x<width;++x,++sPtr,++dPtr)
				{
				(*dPtr)[0]=RGBImage::Scalar(TIFFGetR(*sPtr));
				(*dPtr)[1]=RGBImage::Scalar(TIFFGetG(*sPtr));
				(*dPtr)[2]=RGBImage::Scalar(TIFFGetB(*sPtr));
				(*dPtr)[3]=RGBAImage::Scalar(TIFFGetA(*sPtr));
				}
		}
	catch(std::runtime_error err)
		{
		/* Clean up: */
		delete[] rgbaBuffer;
		if(tiff!=0)
			TIFFClose(tiff);
		
		/* Wrap and re-throw the exception: */
		Misc::throwStdErr("Images::readTransparentTIFFImage: Caught exception \"%s\" while reading image \"%s\"",err.what(),imageName);
		}
	
	/* Clean up and return the result image: */
	delete[] rgbaBuffer;
	if(tiff!=0)
		TIFFClose(tiff);
	return result;
	}

namespace {

/********************************************
Helper functions to read generic TIFF images:
********************************************/

template <class ScalarParam>
BaseImage
readStrippedTIFF(
	TIFF* tiff,
	uint32 width,
	uint32 height,
	unsigned int numChannels,
	unsigned int channelSize,
	GLenum format,
	GLenum channelType)
	{
	/* Create the result image: */
	BaseImage result(width,height,numChannels,channelSize,format,channelType);
	ptrdiff_t resultStride=ptrdiff_t(width)*ptrdiff_t(numChannels);
	
	/* Extract the image's strip layout: */
	uint32 rowsPerStrip;
	TIFFGetField(tiff,TIFFTAG_ROWSPERSTRIP,&rowsPerStrip);
	uint16 planarConfig;
	TIFFGetFieldDefaulted(tiff,TIFFTAG_PLANARCONFIG,&planarConfig);
	
	/* Calculate the number of strips per image or image plane: */
	uint32 stripsPerPlane=(height+rowsPerStrip-1)/rowsPerStrip;
	
	/* Create a buffer to hold a strip of image data: */
	Misc::SelfDestructArray<ScalarParam> stripBuffer((TIFFStripSize(tiff)+sizeof(ScalarParam)-1)/sizeof(ScalarParam));
	
	/* Check whether the image is planed: */
	if(planarConfig==PLANARCONFIG_SEPARATE)
		{
		/* Read image data by channels: */
		for(unsigned int channel=0;channel<numChannels;++channel)
			{
			ScalarParam* rowPtr=static_cast<ScalarParam*>(result.modifyPixels())+resultStride*ptrdiff_t(height-1);
			uint32 rowStart=0;
			for(uint32 strip=0;strip<stripsPerPlane;++strip)
				{
				/* Read the next strip of image data: */
				ScalarParam* stripPtr=stripBuffer.getArray();
				TIFFReadEncodedStrip(tiff,strip,stripPtr,tsize_t(-1));
				
				/* Copy image data from the strip buffer into the result image: */
				uint32 rowEnd=rowStart+rowsPerStrip;
				if(rowEnd>height)
					rowEnd=height;
				for(uint32 row=rowStart;row<rowEnd;++row,rowPtr-=resultStride)
					{
					ScalarParam* pPtr=rowPtr+channel;
					for(uint32 x=0;x<width;++x,++stripPtr,pPtr+=numChannels)
						*rowPtr=*stripPtr;
					}
				
				/* Prepare for the next strip: */
				rowStart=rowEnd;
				}
			}
		}
	else
		{
		/* Read image data by rows: */
		ScalarParam* rowPtr=static_cast<ScalarParam*>(result.modifyPixels())+resultStride*ptrdiff_t(height-1);
		uint32 rowStart=0;
		for(uint32 strip=0;strip<stripsPerPlane;++strip)
			{
			/* Read the next strip of image data: */
			ScalarParam* stripPtr=stripBuffer.getArray();
			TIFFReadEncodedStrip(tiff,strip,stripPtr,tsize_t(-1));
			
			/* Copy image data from the strip buffer into the result image: */
			uint32 rowEnd=rowStart+rowsPerStrip;
			if(rowEnd>height)
				rowEnd=height;
			for(uint32 row=rowStart;row<rowEnd;++row,rowPtr-=resultStride,stripPtr+=resultStride)
				{
				/* Copy all channels of all pixels in the current pixel row: */
				memcpy(rowPtr,stripPtr,resultStride*sizeof(ScalarParam));
				}
			
			/* Prepare for the next strip: */
			rowStart=rowEnd;
			}
		}
	
	return result;
	}

}

BaseImage readGenericTIFFImage(const char* imageName,IO::File& source)
	{
	/* Check if the source file is seekable: */
	IO::SeekableFilePtr seekableSource(&source);
	if(seekableSource==0)
		{
		/* Create a seekable filter for the source file: */
		seekableSource=new IO::SeekableFilter(&source);
		}
	
	/* Set the TIFF error handler: */
	TIFFSetErrorHandler(tiffErrorFunction);
	
	TIFF* tiff=0;
	BaseImage result;
	try
		{
		/* Pretend to open the TIFF file and register the hook functions: */
		tiff=TIFFClientOpen(imageName,"rm",seekableSource.getPointer(),tiffReadFunction,tiffWriteFunction,tiffSeekFunction,tiffCloseFunction,tiffSizeFunction,tiffMapFileFunction,tiffUnmapFileFunction);
		if(tiff==0)
			throw std::runtime_error("Error while opening image");
		
		/* Extract the image header: */
		uint32 width,height;
		TIFFGetField(tiff,TIFFTAG_IMAGEWIDTH,&width);
		TIFFGetField(tiff,TIFFTAG_IMAGELENGTH,&height);
		uint16 numBits,numSamples,sampleFormat;
		TIFFGetField(tiff,TIFFTAG_BITSPERSAMPLE,&numBits);
		TIFFGetField(tiff,TIFFTAG_SAMPLESPERPIXEL,&numSamples);
		TIFFGetFieldDefaulted(tiff,TIFFTAG_SAMPLEFORMAT,&sampleFormat);
		
		/* Determine the result image's pixel format: */
		unsigned int numChannels=numSamples;
		GLenum format=GL_RGB;
		switch(numChannels)
			{
			case 1:
				format=GL_LUMINANCE;
				break;
			
			case 2:
				format=GL_LUMINANCE_ALPHA;
				break;
			
			case 3:
				format=GL_RGB;
				break;
			
			case 4:
				format=GL_RGBA;
				break;
			
			default:
				Misc::throwStdErr("Unsupported number %u of channels",numChannels);
			}
		
		/* Check whether the image is tiled or stripped: */
		if(TIFFIsTiled(tiff))
			{
			/* Can't do this yet: */
			throw std::runtime_error("Tiled images not supported");
			}
		else
			{
			/* Determine the result image's pixel type and read the image data: */
			if(numBits==8)
				{
				switch(sampleFormat)
					{
					case SAMPLEFORMAT_UINT:
						result=readStrippedTIFF<Misc::UInt8>(tiff,width,height,numChannels,1,format,GL_UNSIGNED_BYTE);
						break;
					
					case SAMPLEFORMAT_INT:
						result=readStrippedTIFF<Misc::SInt8>(tiff,width,height,numChannels,1,format,GL_BYTE);
						break;
					
					default:
						Misc::throwStdErr("Unsupported 8-bit sample format %u",sampleFormat);
					}
				}
			else if(numBits==16)
				{
				switch(sampleFormat)
					{
					case SAMPLEFORMAT_UINT:
						result=readStrippedTIFF<Misc::UInt16>(tiff,width,height,numChannels,2,format,GL_UNSIGNED_SHORT);
						break;
					
					case SAMPLEFORMAT_INT:
						result=readStrippedTIFF<Misc::SInt16>(tiff,width,height,numChannels,2,format,GL_SHORT);
						break;
					
					default:
						Misc::throwStdErr("Unsupported 16-bit sample format %u",sampleFormat);
					}
				}
			else if(numBits==32)
				{
				switch(sampleFormat)
					{
					case SAMPLEFORMAT_UINT:
						result=readStrippedTIFF<Misc::UInt32>(tiff,width,height,numChannels,4,format,GL_UNSIGNED_INT);
						break;
					
					case SAMPLEFORMAT_INT:
						result=readStrippedTIFF<Misc::SInt32>(tiff,width,height,numChannels,4,format,GL_INT);
						break;
					
					case SAMPLEFORMAT_IEEEFP:
						result=readStrippedTIFF<Misc::Float32>(tiff,width,height,numChannels,4,format,GL_FLOAT);
						break;
					
					default:
						Misc::throwStdErr("Unsupported 32-bit sample format %u",sampleFormat);
					}
				}
			else
				Misc::throwStdErr("Unsupported channel bit size %u",numBits);
			}
		}
	catch(std::runtime_error err)
		{
		/* Clean up: */
		if(tiff!=0)
			TIFFClose(tiff);
		
		/* Wrap and re-throw the exception: */
		Misc::throwStdErr("Images::readGenericTIFFImage: Caught exception \"%s\" while reading image \"%s\"",err.what(),imageName);
		}
	
	/* Clean up and return the result image: */
	if(tiff!=0)
		TIFFClose(tiff);
	return result;
	}

}

#endif
