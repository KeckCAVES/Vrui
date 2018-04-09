/***********************************************************************
ReadBILImage - Functions to read RGB images from image files in BIL
(Band Interleaved by Line), BIP (Band Interleaved by Pixel), or BSQ
(Band Sequential) formats over an IO::File abstraction.
Copyright (c) 2018 Oliver Kreylos

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

#include <Images/ReadBILImage.h>

#include <string.h>
#include <string>
#include <stdexcept>
#include <Misc/SizedTypes.h>
#include <Misc/Endianness.h>
#include <Misc/SelfDestructArray.h>
#include <Misc/FileNameExtensions.h>
#include <IO/File.h>
#include <IO/Directory.h>
#include <IO/ValueSource.h>
#include <GL/gl.h>
#include <Images/BaseImage.h>

namespace Images {

namespace {

/***********************************************
Helper classes and functions to read BIL images:
***********************************************/

struct BILLayout // Structure describing the data layout of a BIL file
	{
	/* Embedded classes: */
	public:
	enum Layout
		{
		BIP,BIL,BSQ
		};
	
	/* Elements: */
	size_t size[2]; // Image width and height
	size_t nbands; // Number of bands
	size_t nbits; // Number of bits per band per pixel
	bool pixelSigned; // Flag if pixels are signed integers
	Misc::Endianness byteOrder; // File's byte order
	Layout layout; // File's band layout
	size_t skipBytes; // Number of bytes to skip at beginning of image file
	size_t bandRowBytes; // Number of bytes per band per image row
	size_t totalRowBytes; // Number of bytes per image row
	size_t bandGapBytes; // Number of bytes between bands in a BSQ layout
	double map[2]; // Map coordinates of center of upper-left pixel
	double dim[2]; // Pixel dimension in map coordinates
	bool haveNoData; // Flag whether the image file defines an invalid pixel value
	double noData; // Pixel value indicating an invalid pixel
	};

BILLayout readHeaderFile(const IO::Directory& directory,const char* headerFileName,const char* ext,int extLen)
	{
	/* Create default BIL file layout: */
	BILLayout result;
	result.size[1]=result.size[0]=~size_t(0);
	result.nbands=1;
	result.nbits=8;
	result.pixelSigned=false;
	result.byteOrder=Misc::HostEndianness;
	
	/* Determine default file layout based on image file name extension: */
	result.layout=BILLayout::BIL;
	if(extLen==4)
		{
		if(strncasecmp(ext+1,"BIP",extLen-1)==0)
			result.layout=BILLayout::BIP;
		else if(strncasecmp(ext+1,"BSQ",extLen-1)==0)
			result.layout=BILLayout::BSQ;
		}
	
	result.skipBytes=0;
	result.bandGapBytes=0;
	result.map[0]=0.0;
	result.map[1]=0.0;
	result.dim[0]=1.0;
	result.dim[1]=1.0;
	result.haveNoData=false;
	bool haveBandRowBytes=false;
	bool haveTotalRowBytes=false;
	bool mapIsLowerLeft=true;
	
	/* Open the header file: */
	IO::ValueSource header(directory.openFile(headerFileName));
	header.setPunctuation("\n");
	
	/* Process all token=value pairs in the header file: */
	header.skipWs();
	while(!header.eof())
		{
		/* Read the next token: */
		std::string token=header.readString();
		
		if(strcasecmp(token.c_str(),"NROWS")==0||strcasecmp(token.c_str(),"ROWS")==0)
			result.size[1]=header.readUnsignedInteger();
		else if(strcasecmp(token.c_str(),"NCOLS")==0||strcasecmp(token.c_str(),"COLS")==0)
			result.size[0]=header.readUnsignedInteger();
		else if(strcasecmp(token.c_str(),"NBANDS")==0||strcasecmp(token.c_str(),"BANDS")==0)
			result.nbands=header.readUnsignedInteger();
		else if(strcasecmp(token.c_str(),"NBITS")==0)
			{
			result.nbits=header.readUnsignedInteger();
			if(result.nbits!=1&&result.nbits!=4&&result.nbits!=8&&result.nbits!=16&&result.nbits!=32)
				throw std::runtime_error("Images::readGenericBILImage: Invalid pixel size declaration in image header");
			}
		else if(strcasecmp(token.c_str(),"PIXELTYPE")==0)
			{
			/* Check if the pixel type is signed: */
			if(header.isCaseLiteral("SIGNEDINT"))
				result.pixelSigned=true;
			else
				throw std::runtime_error("Images::readGenericBILImage: Invalid pixel type declaration in image header");
			}
		else if(strcasecmp(token.c_str(),"BYTEORDER")==0||strcasecmp(token.c_str(),"BYTE_ORDER")==0)
			{
			/* Read the byte order: */
			std::string byteOrder=header.readString();
			if(strcasecmp(byteOrder.c_str(),"I")==0||strcasecmp(byteOrder.c_str(),"LSBFIRST")==0)
				result.byteOrder=Misc::LittleEndian;
			else if(strcasecmp(byteOrder.c_str(),"M")==0||strcasecmp(byteOrder.c_str(),"MSBFIRST")==0)
				result.byteOrder=Misc::BigEndian;
			else
				throw std::runtime_error("Images::readGenericBILImage: Invalid byte order declaration in image header");
			}
		else if(strcasecmp(token.c_str(),"LAYOUT")==0||strcasecmp(token.c_str(),"INTERLEAVING")==0)
			{
			/* Read the file layout: */
			std::string layout=header.readString();
			if(strcasecmp(layout.c_str(),"BIP")==0)
				result.layout=BILLayout::BIP;
			else if(strcasecmp(layout.c_str(),"BIL")==0)
				result.layout=BILLayout::BIL;
			else if(strcasecmp(layout.c_str(),"BSQ")==0)
				result.layout=BILLayout::BSQ;
			else
				throw std::runtime_error("Images::readGenericBILImage: Invalid image file layout declaration in image header");
			}
		else if(strcasecmp(token.c_str(),"SKIPBYTES")==0)
			result.skipBytes=header.readUnsignedInteger();
		else if(strcasecmp(token.c_str(),"BANDROWBYTES")==0)
			{
			result.bandRowBytes=header.readUnsignedInteger();
			haveBandRowBytes=true;
			}
		else if(strcasecmp(token.c_str(),"BANDGAPBYTES")==0)
			result.bandGapBytes=header.readUnsignedInteger();
		else if(strcasecmp(token.c_str(),"TOTALROWBYTES")==0)
			{
			result.totalRowBytes=header.readUnsignedInteger();
			haveTotalRowBytes=true;
			}
		else if(strcasecmp(token.c_str(),"ULXMAP")==0||strcasecmp(token.c_str(),"UL_X_COORDINATE")==0)
			{
			result.map[0]=header.readNumber();
			mapIsLowerLeft=false;
			}
		else if(strcasecmp(token.c_str(),"ULYMAP")==0||strcasecmp(token.c_str(),"UL_Y_COORDINATE")==0)
			{
			result.map[1]=header.readNumber();
			mapIsLowerLeft=false;
			}
		else if(strcasecmp(token.c_str(),"XLLCORNER")==0)
			{
			result.map[0]=header.readNumber();
			mapIsLowerLeft=true;
			}
		else if(strcasecmp(token.c_str(),"YLLCORNER")==0)
			{
			result.map[1]=header.readNumber();
			mapIsLowerLeft=true;
			}
		else if(strcasecmp(token.c_str(),"XDIM")==0)
			result.dim[0]=header.readNumber();
		else if(strcasecmp(token.c_str(),"YDIM")==0)
			result.dim[1]=header.readNumber();
		else if(strcasecmp(token.c_str(),"CELLSIZE")==0)
			result.dim[1]=result.dim[0]=header.readNumber();
		else if(strcasecmp(token.c_str(),"NODATA")==0||strcasecmp(token.c_str(),"NODATA_VALUE")==0)
			{
			result.haveNoData=true;
			result.noData=header.readNumber();
			}
		
		/* Skip the rest of the line: */
		header.skipLine();
		header.skipWs();
		}
	
	/* Set layout default values: */
	if(!haveBandRowBytes)
		result.bandRowBytes=(result.size[0]*result.nbits+7)/8;
	if(!haveTotalRowBytes)
		result.totalRowBytes=result.layout==BILLayout::BIL?result.nbands*result.bandRowBytes:(result.size[0]*result.nbands*result.nbits+7)/8;
	if(mapIsLowerLeft)
		result.map[1]=result.map[1]+double(result.size[1]-1)*result.dim[1];
	
	return result;
	}

template <class ComponentParam>
void readBIPImageData(IO::File& imageFile,const BILLayout& layout,ComponentParam* data)
	{
	/* Read the image file line-by-line: */
	size_t rowSize=layout.size[0]*layout.nbands;
	size_t rowSkip=layout.totalRowBytes-rowSize*sizeof(ComponentParam);
	ComponentParam* rowPtr=data+rowSize*(layout.size[1]-1);
	for(size_t y=layout.size[1];y>0;--y,rowPtr-=rowSize)
		{
		/* Read a row of image data: */
		imageFile.read<ComponentParam>(rowPtr,rowSize);
		
		/* Skip the gap between rows: */
		imageFile.skip<Misc::UInt8>(rowSkip);
		}
	}

template <class ComponentParam>
void readBILImageData(IO::File& imageFile,const BILLayout& layout,ComponentParam* data)
	{
	/* Read the image file line-by-line and band-by-band: */
	Misc::SelfDestructArray<ComponentParam> band(layout.size[0]);
	size_t rowSize=layout.size[0]*layout.nbands;
	size_t bandSkip=layout.bandRowBytes-layout.size[0]*sizeof(ComponentParam);
	size_t rowSkip=layout.totalRowBytes-layout.nbands*layout.bandRowBytes;
	ComponentParam* rowPtr=data+rowSize*(layout.size[1]-1);
	for(size_t y=layout.size[1];y>0;--y,rowPtr-=rowSize)
		{
		for(size_t i=0;i<layout.nbands;++i)
			{
			/* Read one band of data for the current line: */
			imageFile.read<ComponentParam>(band,layout.size[0]);
			
			/* Copy the band data into the row's pixels: */
			ComponentParam* bPtr=band;
			ComponentParam* pPtr=rowPtr+i;
			for(size_t x=0;x<layout.size[0];++x,++bPtr,pPtr+=layout.nbands)
				*pPtr=*bPtr;
			
			/* Skip the gap between bands: */
			imageFile.skip<Misc::UInt8>(bandSkip);
			}
		
		/* Skip the gap between rows: */
		imageFile.skip<Misc::UInt8>(rowSkip);
		}
	}

template <class ComponentParam>
void readBSQImageData(IO::File& imageFile,const BILLayout& layout,ComponentParam* data)
	{
	/* Read the image file band-by-band and line-by-line: */
	Misc::SelfDestructArray<ComponentParam> band(layout.size[0]);
	size_t rowSize=layout.size[0]*layout.nbands;
	for(size_t i=0;i<layout.nbands;++i)
		{
		ComponentParam* rowPtr=data+rowSize*(layout.size[1]-1);
		for(size_t y=layout.size[1];y>0;--y,rowPtr-=rowSize)
			{
			/* Read one band of data for the current line: */
			imageFile.read<ComponentParam>(band,layout.size[0]);
			
			/* Copy the band data into the row's pixels: */
			ComponentParam* bPtr=band;
			ComponentParam* pPtr=rowPtr+i;
			for(size_t x=0;x<layout.size[0];++x,++bPtr,pPtr+=layout.nbands)
				*pPtr=*bPtr;
			}
		
		/* Skip the band gap: */
		imageFile.skip<Misc::UInt8>(layout.bandGapBytes);
		}
	}

template <class ComponentParam>
BaseImage readImageData(IO::File& imageFile,const BILLayout& layout,GLenum scalarType)
	{
	/* Determine a compatible texture format: */
	GLenum format;
	switch(layout.nbands)
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
			throw std::runtime_error("Images::readGenericBILImage: Image has unsupported pixel format");
		}
	
	/* Create the result image: */
	BaseImage result(layout.size[0],layout.size[1],layout.nbands,(layout.nbits+7)/8,format,scalarType);
	
	/* Skip the image header: */
	imageFile.skip<Misc::UInt8>(layout.skipBytes);
	
	/* Read the image file according to its interleave format: */
	switch(layout.layout)
		{
		case BILLayout::BIP:
			readBIPImageData(imageFile,layout,static_cast<ComponentParam*>(result.modifyPixels()));
			break;
		
		case BILLayout::BIL:
			readBILImageData(imageFile,layout,static_cast<ComponentParam*>(result.modifyPixels()));
			break;
		
		case BILLayout::BSQ:
			readBSQImageData(imageFile,layout,static_cast<ComponentParam*>(result.modifyPixels()));
			break;
		}
	
	return result;
	}

}

BaseImage readGenericBILImage(const IO::Directory& directory,const char* imageFileName)
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
	
	/* Construct the header file name: */
	std::string headerFileName(imageFileName,ext);
	headerFileName.append(".hdr");
	
	/* Read the image's header file to detect its layout: */
	BILLayout layout=readHeaderFile(directory,headerFileName.c_str(),ext,extLen);
	
	/* Open the image file: */
	IO::FilePtr imageFile=directory.openFile(imageFileName);
	
	/* Read the image file according to its pixel type: */
	switch(layout.nbits)
		{
		case 8:
			if(layout.pixelSigned)
				return readImageData<Misc::SInt8>(*imageFile,layout,GL_BYTE);
			else
				return readImageData<Misc::UInt8>(*imageFile,layout,GL_UNSIGNED_BYTE);
		
		case 16:
			if(layout.pixelSigned)
				return readImageData<Misc::SInt16>(*imageFile,layout,GL_SHORT);
			else
				return readImageData<Misc::UInt16>(*imageFile,layout,GL_UNSIGNED_SHORT);
		
		case 32:
			return readImageData<Misc::Float32>(*imageFile,layout,GL_FLOAT);
		
		default:
			throw std::runtime_error("Images::readGenericBILImage: Image has unsupported pixel size");
		}
	}

}
