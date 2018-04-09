/***********************************************************************
LoadElevationGrid - Function to load an elevation grid's height values
from an external file.
Copyright (c) 2010-2018 Oliver Kreylos

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

#include <SceneGraph/Internal/LoadElevationGrid.h>

#include <utility>
#include <string>
#include <Misc/SizedTypes.h>
#include <Misc/ThrowStdErr.h>
#include <IO/File.h>
#include <IO/ValueSource.h>
#include <Cluster/OpenFile.h>
#include <Images/BaseImage.h>
#include <Images/ReadImageFile.h>
#include <SceneGraph/ElevationGridNode.h>

namespace SceneGraph {

namespace {

/****************
Helper functions:
****************/

std::string createHeaderFileName(const std::string& bilFileName)
	{
	/* Find the BIL file name's extension: */
	std::string::const_iterator extPtr=bilFileName.end();
	for(std::string::const_iterator sPtr=bilFileName.begin();sPtr!=bilFileName.end();++sPtr)
		if(*sPtr=='.')
			extPtr=sPtr;
	
	/* Create the header file name: */
	std::string result=std::string(bilFileName.begin(),extPtr);
	result.append(".hdr");
	
	return result;
	}

void loadBILGrid(ElevationGridNode& node,Cluster::Multiplexer* multiplexer)
	{
	/* Open the header file: */
	std::string bilFileName=node.heightUrl.getValue(0);
	IO::ValueSource header(Cluster::openFile(multiplexer,createHeaderFileName(bilFileName).c_str()));
	header.skipWs();
	
	/* Parse the header file: */
	unsigned int size[2]={0,0};
	unsigned int numBits=16;
	unsigned int skipBytes=0;
	unsigned int bandGapBytes=0;
	unsigned int bandRowBytes=0;
	unsigned int totalRowBytes=0;
	Misc::Endianness endianness=Misc::HostEndianness;
	Scalar map[2]={Scalar(0),Scalar(0)};
	bool mapIsUpperLeft=false;
	Scalar cellSize[2]={Scalar(1),Scalar(1)};
	while(!header.eof())
		{
		/* Read the next token: */
		std::string token=header.readString();
		
		if(token=="LAYOUT"||token=="INTERLEAVING")
			{
			std::string layout=header.readString();
			if(layout!="BIL"&&layout!="BIP"&&layout!="BSQ")
				Misc::throwStdErr("SceneGraph::loadElevationGrid: File %s has unsupported layout %s",bilFileName.c_str(),layout.c_str());
			}
		else if(token=="NBANDS"||token=="BANDS")
			{
			unsigned int numBands=header.readUnsignedInteger();
			if(numBands!=1)
				Misc::throwStdErr("SceneGraph::loadElevationGrid: File %s has %d bands instead of 1",bilFileName.c_str(),numBands);
			}
		else if(token=="NCOLS"||token=="COLS")
			size[0]=header.readUnsignedInteger();
		else if(token=="NROWS"||token=="ROWS")
			size[1]=header.readUnsignedInteger();
		else if(token=="NBITS")
			{
			numBits=header.readUnsignedInteger();
			if(numBits!=16&&numBits!=32)
				Misc::throwStdErr("SceneGraph::loadElevationGrid: File %s has unsupported number of bits per sample %d",bilFileName.c_str(),numBits);
			}
		else if(token=="SKIPBYTES")
			skipBytes=header.readUnsignedInteger();
		else if(token=="BANDGAPBYTES")
			bandGapBytes=header.readUnsignedInteger();
		else if(token=="BANDROWBYTES")
			bandRowBytes=header.readUnsignedInteger();
		else if(token=="TOTALROWBYTES")
			totalRowBytes=header.readUnsignedInteger();
		else if(token=="BYTE_ORDER"||token=="BYTEORDER")
			{
			std::string byteOrder=header.readString();
			if(byteOrder=="LSBFIRST"||byteOrder=="I")
				endianness=Misc::LittleEndian;
			else if(byteOrder=="MSBFIRST"||byteOrder=="M")
				endianness=Misc::BigEndian;
			else
				Misc::throwStdErr("SceneGraph::loadElevationGrid: File %s has unrecognized byte order %s",bilFileName.c_str(),byteOrder.c_str());
			}
		else if(token=="ULXMAP"||token=="UL_X_COORDINATE")
			{
			map[0]=Scalar(header.readNumber());
			mapIsUpperLeft=true;
			}
		else if(token=="ULYMAP"||token=="UL_Y_COORDINATE")
			{
			map[1]=Scalar(header.readNumber());
			mapIsUpperLeft=true;
			}
		else if(token=="XLLCORNER")
			{
			map[0]=Scalar(header.readNumber());
			mapIsUpperLeft=false;
			}
		else if(token=="YLLCORNER")
			{
			map[1]=Scalar(header.readNumber());
			mapIsUpperLeft=false;
			}
		else if(token=="XDIM")
			cellSize[0]=Scalar(header.readNumber());
		else if(token=="YDIM")
			cellSize[1]=Scalar(header.readNumber());
		else if(token=="CELLSIZE")
			{
			Scalar cs=Scalar(header.readNumber());
			for(int i=0;i<2;++i)
				cellSize[i]=cs;
			}
		else if(token=="NODATA_VALUE"||token=="NODATA")
			{
			/* Set the node's invalid removal flag and invalid height value: */
			node.removeInvalids.setValue(true);
			node.invalidHeight.setValue(header.readNumber());
			}
		}
	
	/* Check the image layout: */
	if(size[0]==0||size[1]==0)
		Misc::throwStdErr("SceneGraph::loadElevationGrid: File %s has undefined image size",bilFileName.c_str());
	int numBytes=(numBits+7)/8;
	if(totalRowBytes!=bandRowBytes||bandRowBytes!=size[0]*numBytes)
		Misc::throwStdErr("SceneGraph::loadElevationGrid: File %s has mismatching row size",bilFileName.c_str());
	if(bandGapBytes!=0)
		Misc::throwStdErr("SceneGraph::loadElevationGrid: File %s has nonzero band gap",bilFileName.c_str());
	
	/* Initialize the height array: */
	std::vector<Scalar> heights;
	size_t numPixels=size_t(size[0])*size_t(size[1]);
	heights.reserve(numPixels);
	for(size_t i=numPixels;i>0;--i)
		heights.push_back(Scalar(0));
	
	/* Read the image: */
	IO::FilePtr imageFile(Cluster::openFile(multiplexer,bilFileName.c_str()));
	imageFile->setEndianness(endianness);
	imageFile->skip<Misc::UInt8>(skipBytes);
	if(numBits==16)
		{
		/* Read the grid as 16-bit signed integers: */
		Misc::SInt16* rowBuffer=new Misc::SInt16[size[0]];
		for(unsigned int y=size[1];y>0;--y)
			{
			/* Read the raw image row: */
			imageFile->read<Misc::SInt16>(rowBuffer,size[0]);
			
			/* Copy the image row into the height array: */
			size_t rowBase=size_t(y-1)*size_t(size[0]);
			for(unsigned int x=0;x<size[0];++x)
				heights[rowBase+x]=Scalar(rowBuffer[x]);
			}
		delete[] rowBuffer;
		}
	else if(numBits==32)
		{
		/* Read the grid as 32-bit floating-point numbers: */
		Misc::Float32* rowBuffer=new Misc::Float32[size[0]];
		for(unsigned int y=size[1];y>0;--y)
			{
			/* Read the raw image row: */
			imageFile->read<Misc::Float32>(rowBuffer,size[0]);
			
			/* Copy the image row into the height array: */
			size_t rowBase=size_t(y-1)*size_t(size[0]);
			for(unsigned int x=0;x<size[0];++x)
				heights[rowBase+x]=Scalar(rowBuffer[x]);
			}
		delete[] rowBuffer;
		}
	
	/* Install the height field: */
	node.xDimension.setValue(size[0]);
	node.xSpacing.setValue(cellSize[0]);
	node.zDimension.setValue(size[1]);
	node.zSpacing.setValue(cellSize[1]);
	Point origin=Point::origin;
	for(int i=0;i<2;++i)
	 origin[i]=map[i]+Math::div2(cellSize[i]);
	if(mapIsUpperLeft)
		origin[1]-=Scalar(size[1])*cellSize[1];
	if(node.heightIsY.getValue())
		std::swap(origin[1],origin[2]);
	std::swap(node.height.getValues(),heights);
	}

void loadAGRGrid(ElevationGridNode& node,Cluster::Multiplexer* multiplexer)
	{
	/* Open the grid file: */
	std::string gridFileName=node.heightUrl.getValue(0);
	IO::ValueSource grid(Cluster::openFile(multiplexer,gridFileName.c_str()));
	grid.skipWs();
	
	/* Read the grid header: */
	unsigned int gridSize[2]={0,0};
	double gridOrigin[2]={0.0,0.0};
	double cellSize=1.0;
	double nodata=9999.0;
	bool ok=grid.readString()=="ncols";
	if(ok)
		gridSize[0]=grid.readUnsignedInteger();
	ok=ok&&grid.readString()=="nrows";
	if(ok)
		gridSize[1]=grid.readUnsignedInteger();
	ok=ok&&grid.readString()=="xllcorner";
	if(ok)
		gridOrigin[0]=grid.readNumber();
	ok=ok&&grid.readString()=="yllcorner";
	if(ok)
		gridOrigin[1]=grid.readNumber();
	ok=ok&&grid.readString()=="cellsize";
	if(ok)
		cellSize=grid.readNumber();
	ok=ok&&grid.readString()=="NODATA_value";
	if(ok)
		nodata=grid.readNumber();
	if(!ok)
		Misc::throwStdErr("SceneGraph::loadElevationGrid: File %s is not an ARC/INFO ASCII grid",gridFileName.c_str());
	
	/* Read the grid: */
	std::vector<Scalar> heights;
	heights.reserve(size_t(gridSize[0])*size_t(gridSize[1]));
	for(size_t i=size_t(gridSize[0])*size_t(gridSize[1]);i>0;--i)
		heights.push_back(Scalar(0));
	for(unsigned int y=gridSize[1];y>0;--y)
		for(unsigned int x=0;x<gridSize[0];++x)
			heights[(y-1)*gridSize[0]+x]=grid.readNumber();
	
	/* Install the height field: */
	Point origin=node.origin.getValue();
	origin[0]=Scalar(gridOrigin[0]+cellSize*Scalar(0.5));
	if(node.heightIsY.getValue())
		origin[2]=Scalar(gridOrigin[1]+cellSize*Scalar(0.5));
	else
		origin[1]=Scalar(gridOrigin[1]+cellSize*Scalar(0.5));
	node.origin.setValue(origin);
	node.xDimension.setValue(gridSize[0]);
	node.xSpacing.setValue(cellSize);
	node.zDimension.setValue(gridSize[1]);
	node.zSpacing.setValue(cellSize);
	std::swap(node.height.getValues(),heights);
	
	/* Set the node's invalid removal flag and invalid height value: */
	node.removeInvalids.setValue(true);
	node.invalidHeight.setValue(nodata);
	}

/*************************************************************
Helper function to extract elevation data from generic images:
*************************************************************/

template <class ImageScalarParam>
inline
void
readImageGrid(
	const Images::BaseImage& image,
	std::vector<Scalar>& heights)
	{
	/* Stuff all image pixels into the given vector: */
	size_t numPixels=size_t(image.getSize(1))*size_t(image.getSize(0));
	heights.reserve(numPixels);
	const ImageScalarParam* iPtr=static_cast<const ImageScalarParam*>(image.getPixels());
	for(size_t i=numPixels;i>0;--i,++iPtr)
		heights.push_back(Scalar(*iPtr));
	}

void loadImageGrid(ElevationGridNode& node,Cluster::Multiplexer* multiplexer)
	{
	/* Open and read the image file: */
	IO::FilePtr imageFile(Cluster::openFile(multiplexer,node.heightUrl.getValue(0).c_str()));
	Images::BaseImage image=Images::readGenericImageFile(node.heightUrl.getValue(0).c_str(),imageFile);
	
	/* Convert the image to greyscale and strip the alpha channel should it exist: */
	image=image.toGrey().dropAlpha();
	
	/* Read the grid: */
	std::vector<Scalar> heights;
	switch(image.getScalarType())
		{
		case GL_BYTE:
			readImageGrid<GLbyte>(image,heights);
			break;
		
		case GL_UNSIGNED_BYTE:
			readImageGrid<GLubyte>(image,heights);
			break;
		
		case GL_SHORT:
			readImageGrid<GLshort>(image,heights);
			break;
		
		case GL_UNSIGNED_SHORT:
			readImageGrid<GLushort>(image,heights);
			break;
		
		case GL_INT:
			readImageGrid<GLint>(image,heights);
			break;
		
		case GL_UNSIGNED_INT:
			readImageGrid<GLuint>(image,heights);
			break;
		
		case GL_FLOAT:
			readImageGrid<GLfloat>(image,heights);
			break;
		
		case GL_DOUBLE:
			readImageGrid<GLdouble>(image,heights);
			break;
		
		default:
			throw std::runtime_error("SceneGraph::loadElevationGrid: Source image has unsupported pixel type");
		}
	
	/* Install the height field: */
	node.xDimension.setValue(image.getWidth());
	node.zDimension.setValue(image.getHeight());
	std::swap(node.height.getValues(),heights);
	}

template <class ValueParam>
inline
void
loadRawGrid(
	ElevationGridNode& node,
	Misc::Endianness endianness,
	Cluster::Multiplexer* multiplexer)
	{
	/* Open the grid file: */
	IO::FilePtr gridFile=Cluster::openFile(multiplexer,node.heightUrl.getValue(0).c_str());
	gridFile->setEndianness(endianness);
	
	/* Read the grid: */
	int width=node.xDimension.getValue();
	int height=node.zDimension.getValue();
	std::vector<Scalar> heights;
	heights.reserve(size_t(height)*size_t(width));
	ValueParam* row=new ValueParam[width];
	for(int y=0;y<height;++y)
		{
		/* Read a row of values in the file format: */
		gridFile->read(row,width);
		
		/* Convert the row values to elevation grid format: */
		for(int x=0;x<width;++x)
			heights.push_back(Scalar(row[x]));
		}
	delete[] row;
	
	/* Install the height field: */
	std::swap(node.height.getValues(),heights);
	}

inline bool startsWith(std::string::const_iterator string,std::string::const_iterator stringEnd,const char* prefix)
	{
	while(string!=stringEnd&&*prefix!='\0')
		{
		if(*string!=*prefix)
			return false;
		++string;
		++prefix;
		}
	return *prefix=='\0';
	}

}

void loadElevationGrid(ElevationGridNode& node,Cluster::Multiplexer* multiplexer)
	{
	/* Determine the format of the height file: */
	if(node.heightUrlFormat.getNumValues()>=1&&node.heightUrlFormat.getValue(0)=="BIL")
		{
		/* Load an elevation grid in BIL format: */
		loadBILGrid(node,multiplexer);
		}
	else if(node.heightUrlFormat.getNumValues()>=1&&node.heightUrlFormat.getValue(0)=="ARC/INFO ASCII GRID")
		{
		/* Load an elevation grid in ARC/INFO ASCII GRID format: */
		loadAGRGrid(node,multiplexer);
		}
	else if(node.heightUrlFormat.getNumValues()>=1&&startsWith(node.heightUrlFormat.getValue(0).begin(),node.heightUrlFormat.getValue(0).end(),"RAW "))
		{
		std::string::const_iterator sIt=node.heightUrlFormat.getValue(0).begin()+4;
		std::string::const_iterator sEnd=node.heightUrlFormat.getValue(0).end();
		std::string format;
		while(sIt!=sEnd&&!isspace(*sIt))
			{
			format.push_back(*sIt);
			++sIt;
			}
		while(sIt!=sEnd&&isspace(*sIt))
			++sIt;
		std::string endiannessCode;
		while(sIt!=sEnd&&!isspace(*sIt))
			{
			endiannessCode.push_back(*sIt);
			++sIt;
			}
		Misc::Endianness endianness=Misc::HostEndianness;
		if(endiannessCode=="LE")
			endianness=Misc::LittleEndian;
		else if(endiannessCode=="BE")
			endianness=Misc::BigEndian;
		else if(!endiannessCode.empty())
			Misc::throwStdErr("SceneGraph::loadElevationGrid: Unknown endianness %s",endiannessCode.c_str());
		
		/* Load an elevation grid in raw format, containing values of the requested type: */
		if(format=="UINT8")
			loadRawGrid<Misc::UInt8>(node,endianness,multiplexer);
		else if(format=="SINT8")
			loadRawGrid<Misc::SInt8>(node,endianness,multiplexer);
		else if(format=="UINT16")
			loadRawGrid<Misc::UInt16>(node,endianness,multiplexer);
		else if(format=="SINT16")
			loadRawGrid<Misc::SInt16>(node,endianness,multiplexer);
		else if(format=="UINT32")
			loadRawGrid<Misc::UInt32>(node,endianness,multiplexer);
		else if(format=="SINT32")
			loadRawGrid<Misc::SInt32>(node,endianness,multiplexer);
		else if(format=="FLOAT32")
			loadRawGrid<Misc::Float32>(node,endianness,multiplexer);
		else if(format=="FLOAT64")
			loadRawGrid<Misc::Float64>(node,endianness,multiplexer);
		else
			Misc::throwStdErr("SceneGraph::loadElevationGrid: Unknown raw data type %s",format.c_str());
		}
	else
		{
		/* Find the height file name's extension: */
		std::string::const_iterator extPtr=node.heightUrl.getValue(0).end();
		for(std::string::const_iterator sPtr=node.heightUrl.getValue(0).begin();sPtr!=node.heightUrl.getValue(0).end();++sPtr)
			if(*sPtr=='.')
				extPtr=sPtr;
		std::string extension(extPtr,node.heightUrl.getValue(0).end());
		
		if(extension==".bil")
			{
			/* Load an elevation grid in BIL format: */
			loadBILGrid(node,multiplexer);
			}
		else if(Images::canReadImageFileType(node.heightUrl.getValue(0).c_str()))
			{
			/* Load the elevation grid as an image file with height defined by luminance: */
			loadImageGrid(node,multiplexer);
			}
		else
			Misc::throwStdErr("SceneGraph::loadElevationGrid: File %s has unknown format",node.heightUrl.getValue(0).c_str());
		}
	}

}
