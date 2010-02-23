/***********************************************************************
LoadElevationGrid - Function to load an elevation grid's height values
from an external file.
Copyright (c) 2010 Oliver Kreylos

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

#include <SceneGraph/LoadElevationGrid.h>

#include <utility>
#include <string>
#include <Misc/LargeFile.h>
#include <Misc/FileCharacterSource.h>
#include <Misc/ValueSource.h>
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

}

void loadElevationGrid(ElevationGridNode& node)
	{
	/* Open the header file: */
	std::string bilFileName=node.heightUrl.getValue(0);
	Misc::FileCharacterSource headerFile(createHeaderFileName(bilFileName).c_str());
	Misc::ValueSource header(headerFile);
	header.skipWs();
	
	/* Parse the header file: */
	int size[2]={-1,-1};
	int numBits=16;
	Misc::LargeFile::Offset bandGapBytes=0;
	Misc::LargeFile::Offset bandRowBytes=0;
	Misc::LargeFile::Offset totalRowBytes=0;
	Misc::LargeFile::Endianness endianness=Misc::LargeFile::DontCare;
	Scalar cellSize[2]={Scalar(1),Scalar(1)};
	while(!header.eof())
		{
		/* Read the next token: */
		std::string token=header.readString();
		
		if(token=="LAYOUT")
			{
			std::string layout=header.readString();
			if(layout!="BIL")
				Misc::throwStdErr("loadElevationGrid: File %s does not have BIL layout",bilFileName.c_str());
			}
		else if(token=="NBANDS")
			{
			int numBands=header.readInteger();
			if(numBands!=1)
				Misc::throwStdErr("loadElevationGrid: File %s has %d bands instead of 1",bilFileName.c_str(),numBands);
			}
		else if(token=="NCOLS")
			size[0]=header.readInteger();
		else if(token=="NROWS")
			size[1]=header.readInteger();
		else if(token=="NBITS")
			{
			numBits=header.readInteger();
			if(numBits!=16&&numBits!=32)
				Misc::throwStdErr("loadElevationGrid: File %s has unsupported number of bits per sample %d",bilFileName.c_str(),numBits);
			}
		else if(token=="BANDGAPBYTES")
			bandGapBytes=Misc::LargeFile::Offset(header.readInteger());
		else if(token=="BANDROWBYTES")
			bandRowBytes=Misc::LargeFile::Offset(header.readInteger());
		else if(token=="TOTALROWBYTES")
			totalRowBytes=Misc::LargeFile::Offset(header.readInteger());
		else if(token=="BYTEORDER")
			{
			std::string byteOrder=header.readString();
			if(byteOrder=="LSBFIRST"||byteOrder=="I")
				endianness=Misc::LargeFile::LittleEndian;
			else if(byteOrder=="MSBFIRST"||byteOrder=="M")
				endianness=Misc::LargeFile::BigEndian;
			else
				Misc::throwStdErr("loadElevationGrid: File %s has unrecognized byte order %s",bilFileName.c_str(),byteOrder.c_str());
			}
		else if(token=="CELLSIZE")
			{
			Scalar cs=Scalar(header.readNumber());
			for(int i=0;i<2;++i)
				cellSize[i]=cs;
			}
		else if(token=="XDIM")
			cellSize[0]=Scalar(header.readNumber());
		else if(token=="YDIM")
			cellSize[1]=Scalar(header.readNumber());
		else if(token=="NODATA_VALUE")
			header.readNumber();
		}
	
	/* Check the image layout: */
	int numBytes=(numBits+7)/8;
	if(totalRowBytes!=bandRowBytes||bandRowBytes!=Misc::LargeFile::Offset(size[0])*Misc::LargeFile::Offset(numBytes))
		Misc::throwStdErr("loadElevationGrid: File %s has mismatching row size",bilFileName.c_str());
	if(bandGapBytes!=0)
		Misc::throwStdErr("loadElevationGrid: File %s has nonzero band gap",bilFileName.c_str());
	
	/* Read the image: */
	Misc::LargeFile image(bilFileName.c_str(),"rb",endianness);
	std::vector<Scalar> heights;
	heights.reserve(size_t(size[0])*size_t(size[1]));
	if(numBits==16)
		{
		signed short int* rowBuffer=new signed short int[size[0]];
		for(int y=size[1]-1;y>=0;--y)
			{
			/* Read the raw image row: */
			image.seekSet(totalRowBytes*Misc::LargeFile::Offset(y));
			image.read<signed short int>(rowBuffer,size[0]);
			for(int x=0;x<size[0];++x)
				heights.push_back(Scalar(rowBuffer[x]));
			}
		delete[] rowBuffer;
		}
	else if(numBits==32)
		{
		float* rowBuffer=new float[size[0]];
		for(int y=size[1]-1;y>=0;--y)
			{
			/* Read the raw image row: */
			image.seekSet(totalRowBytes*Misc::LargeFile::Offset(y));
			image.read<float>(rowBuffer,size[0]);
			for(int x=0;x<size[0];++x)
				heights.push_back(Scalar(rowBuffer[x]));
			}
		delete[] rowBuffer;
		}
	
	/* Install the height field: */
	node.xDimension.setValue(size[0]);
	node.xSpacing.setValue(cellSize[0]);
	node.zDimension.setValue(size[1]);
	node.zSpacing.setValue(cellSize[1]);
	std::swap(node.height.getValues(),heights);
	}

}
