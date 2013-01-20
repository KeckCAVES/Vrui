/***********************************************************************
ImageReader - Abstract base class to read images from files in a variety
of image file formats.
Copyright (c) 2012 Oliver Kreylos

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

#include <Images/ImageReader.h>

namespace Images {

/****************************
Methods of class ImageReader:
****************************/

ImageReader::ImageReader(IO::FilePtr sFile)
	:file(sFile),
	 numImages(0),imageSpecs(0)
	{
	canvasSize[0]=canvasSize[1]=0;
	}

ImageReader::~ImageReader(void)
	{
	delete[] imageSpecs;
	}

Image<GLubyte,1> ImageReader::readGray8(void)
	{
	/* Read the image component planes: */
	const ImageSpec& spec=imageSpecs[0];
	ImagePlane* planes=readSubImagePlanes();
	
	/* Create the result image: */
	Image<GLubyte,1> result(spec.size[0],spec.size[1]);
	Image<GLubyte,1>::Color* rPtr=result.modifyPixels();
	
	/* Copy the image planes into the result image: */
	if(spec.colorSpace==Grayscale)
		{
		/* Copy the first image component: */
		if(planes[0].pixelSize==1)
			{
			if(spec.signed)
				{
				}
			else
				{
				if(spec.bitsPerComponent==8)
					{
					/* Copy the plane 1:1: */
					const GLubyte* pRowPtr=static_cast<GLubyte*>(planes[0].basePtr);
					for(unsigned int y=0;y<spec.size[1];++y,pRowPtr+=planes[0].rowStride)
						{
						const GLubyte* pPtr=pRowPtr;
						for(unsigned int x=0;x<spec.size[0];++x,pPtr+=planes[0].pixelStride,++rPtr)
							(*rPtr)[0]=*pPtr;
						}
					}
				else
					{
					}
				}
			}
		}
	else if(spec.colorSpace==RGB)
		{
		}
	}

}
