/***********************************************************************
HMDConfiguration - Class to represent the internal configuration of a
head-mounted display.
Copyright (c) 2016-2017 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Vrui/Internal/HMDConfiguration.h>

#include <Misc/SizedTypes.h>

namespace Vrui {

/*********************************
Methods of class HMDConfiguration:
*********************************/

HMDConfiguration::HMDConfiguration(void)
	:eyePosVersion(0U),eyeVersion(0U),distortionMeshVersion(0U)
	{
	/* Initialize distortion meshes: */
	for(int i=0;i<2;++i)
		distortionMeshSize[i]=0;
	for(int eye=0;eye<2;++eye)
		eyes[eye].distortionMesh=0;
	}

HMDConfiguration::~HMDConfiguration(void)
	{
	/* Delete distortion meshes: */
	for(int eye=0;eye<2;++eye)
		delete[] eyes[eye].distortionMesh;
	}

const HMDConfiguration::DistortionMeshVertex* HMDConfiguration::getDistortionMesh(int eye) const
	{
	/* Return the given eye's distortion mesh: */
	return eyes[eye].distortionMesh;
	}

void HMDConfiguration::setTrackerIndex(Misc::UInt16 newTrackerIndex)
	{
	trackerIndex=newTrackerIndex;
	}

void HMDConfiguration::setEyePos(const HMDConfiguration::Point& leftPos,const HMDConfiguration::Point& rightPos)
	{
	/* Update both eye positions directly: */
	eyePos[0]=leftPos;
	eyePos[1]=rightPos;
	
	/* Calculate the inter-pupillary distance: */
	ipd=Geometry::dist(eyePos[0],eyePos[1]);
	
	if(++eyePosVersion==0U)
		++eyePosVersion;
	}

void HMDConfiguration::setIpd(HMDConfiguration::Scalar newIpd)
	{
	/* Check if the new inter-pupillary distance is different from the current one: */
	if(ipd!=newIpd)
		{
		/* Update both eye positions symmetrically based on old distance vector and new distance: */
		Point monoPos=Geometry::mid(eyePos[0],eyePos[1]);
		Point::Vector dist=eyePos[1]-eyePos[0];
		dist*=Scalar(double(newIpd)*0.5/Geometry::mag(dist));
		eyePos[0]=monoPos-dist;
		eyePos[1]=monoPos+dist;
		
		ipd=newIpd;
		
		if(++eyePosVersion==0U)
			++eyePosVersion;
		}
	}

void HMDConfiguration::setRenderTargetSize(HMDConfiguration::UInt newWidth,HMDConfiguration::UInt newHeight)
	{
	/* Check if the render target size changed: */
	if(renderTargetSize[0]!=newWidth||renderTargetSize[1]!=newHeight)
		{
		/* Update the render target size: */
		renderTargetSize[0]=newWidth;
		renderTargetSize[1]=newHeight;
		
		if(++distortionMeshVersion==0U)
			++distortionMeshVersion;
		}
	}

void HMDConfiguration::setDistortionMeshSize(HMDConfiguration::UInt newWidth,HMDConfiguration::UInt newHeight)
	{
	/* Check if the mesh size changed: */
	if(distortionMeshSize[0]!=newWidth||distortionMeshSize[1]!=newHeight)
		{
		/* Re-allocate distortion meshes: */
		distortionMeshSize[0]=newWidth;
		distortionMeshSize[1]=newHeight;
		for(int eye=0;eye<2;++eye)
			{
			delete[] eyes[eye].distortionMesh;
			eyes[eye].distortionMesh=new DistortionMeshVertex[distortionMeshSize[1]*distortionMeshSize[0]];
			}
		
		if(++distortionMeshVersion==0U)
			++distortionMeshVersion;
		}
	}

void HMDConfiguration::setViewport(int eye,HMDConfiguration::UInt x,HMDConfiguration::UInt y,HMDConfiguration::UInt width,HMDConfiguration::UInt height)
	{
	/* Check if the viewport changed: */
	if(eyes[eye].viewport[0]!=x||eyes[eye].viewport[1]!=y||eyes[eye].viewport[2]!=width||eyes[eye].viewport[3]!=height)
		{
		/* Update the given eye's viewport: */
		eyes[eye].viewport[0]=x;
		eyes[eye].viewport[1]=y;
		eyes[eye].viewport[2]=width;
		eyes[eye].viewport[3]=height;
		
		if(++distortionMeshVersion==0U)
			++distortionMeshVersion;
		}
	}

void HMDConfiguration::setFov(int eye,HMDConfiguration::Scalar left,HMDConfiguration::Scalar right,HMDConfiguration::Scalar bottom,HMDConfiguration::Scalar top)
	{
	/* Check if the FoV changed: */
	if(eyes[eye].fov[0]!=left||eyes[eye].fov[1]!=right||eyes[eye].fov[2]!=bottom||eyes[eye].fov[3]!=top)
		{
		/* Update the given eye's FoV boundaries: */
		eyes[eye].fov[0]=left;
		eyes[eye].fov[1]=right;
		eyes[eye].fov[2]=bottom;
		eyes[eye].fov[3]=top;
		
		if(++eyeVersion==0U)
			++eyeVersion;
		}
	}

HMDConfiguration::DistortionMeshVertex* HMDConfiguration::getDistortionMesh(int eye)
	{
	/* Return the given eye's distortion mesh: */
	return eyes[eye].distortionMesh;
	}

void HMDConfiguration::updateDistortionMeshes(void)
	{
	if(++distortionMeshVersion==0U)
		++distortionMeshVersion;
	}

void HMDConfiguration::write(unsigned int sinkEyePosVersion,unsigned int sinkEyeVersion,unsigned int sinkDistortionMeshVersion,VRDevicePipe& sink) const
	{
	/* Write the appropriate update message ID: */
	VRDevicePipe::MessageIdType messageId=VRDevicePipe::HMDCONFIG_UPDATE;
	if(sinkEyePosVersion!=eyePosVersion)
		messageId=messageId|VRDevicePipe::MessageIdType(0x1U);
	if(sinkEyeVersion!=eyeVersion)
		messageId=messageId|VRDevicePipe::MessageIdType(0x2U);
	if(sinkDistortionMeshVersion!=distortionMeshVersion)
		messageId=messageId|VRDevicePipe::MessageIdType(0x4U);
	sink.writeMessage(messageId);
	
	/* Write the tracker index to identify this HMD: */
	sink.write(trackerIndex);
	
	/* Write out-of-date configuration components: */
	if(sinkEyePosVersion!=eyePosVersion)
		{
		/* Write both eye positions: */
		for(int eye=0;eye<2;++eye)
			sink.write(eyePos[eye].getComponents(),3);
		}
	if(sinkEyeVersion!=eyeVersion)
		{
		/* Write both eyes' FoV boundaries: */
		for(int eye=0;eye<2;++eye)
			sink.write(eyes[eye].fov,4);
		}
	if(sinkDistortionMeshVersion!=distortionMeshVersion)
		{
		/* Write recommended render target size: */
		sink.write(renderTargetSize,2);
		
		/* Write distortion mesh size: */
		sink.write(distortionMeshSize,2);
		
		/* Write per-eye state: */
		for(int eye=0;eye<2;++eye)
			{
			/* Write eye's viewport: */
			sink.write(eyes[eye].viewport,4);
			
			/* Write eye's distortion mesh: */
			const DistortionMeshVertex* dmPtr=eyes[eye].distortionMesh;
			for(unsigned int y=0;y<distortionMeshSize[1];++y)
				for(unsigned int x=0;x<distortionMeshSize[0];++x,++dmPtr)
					{
					sink.write(dmPtr->red.getComponents(),2);
					sink.write(dmPtr->green.getComponents(),2);
					sink.write(dmPtr->blue.getComponents(),2);
					}
			}
		}
	}

void HMDConfiguration::read(VRDevicePipe::MessageIdType messageId,Misc::UInt16 newTrackerIndex,VRDevicePipe& source)
	{
	/* Update the tracker index: */
	trackerIndex=newTrackerIndex;
	
	/* Check which configuration components are being sent: */
	if(messageId&VRDevicePipe::MessageIdType(0x1U))
		{
		/* Read both eye positions: */
		for(int eye=0;eye<2;++eye)
			source.read(eyePos[eye].getComponents(),3);
		
		/* Calculate the inter-pupillary distance: */
		ipd=Geometry::dist(eyePos[0],eyePos[1]);
		
		if(++eyePosVersion==0U)
			++eyePosVersion;
		}
	if(messageId&VRDevicePipe::MessageIdType(0x2U))
		{
		/* Read both eyes' FoV boundaries: */
		for(int eye=0;eye<2;++eye)
			source.read(eyes[eye].fov,4);
		
		if(++eyeVersion==0U)
			++eyeVersion;
		}
	if(messageId&VRDevicePipe::MessageIdType(0x4U))
		{
		/* Read recommended render target size: */
		source.read(renderTargetSize,2);
		
		/* Read distortion mesh size: */
		Misc::UInt32 newDistortionMeshSize[2];
		source.read(newDistortionMeshSize,2);
		if(distortionMeshSize[0]!=newDistortionMeshSize[0]||distortionMeshSize[1]!=newDistortionMeshSize[1])
			{
			/* Re-allocate both eyes' distortion mesh arrays: */
			distortionMeshSize[0]=newDistortionMeshSize[0];
			distortionMeshSize[1]=newDistortionMeshSize[1];
			for(int eye=0;eye<2;++eye)
				{
				delete[] eyes[eye].distortionMesh;
				eyes[eye].distortionMesh=new DistortionMeshVertex[distortionMeshSize[1]*distortionMeshSize[0]];
				}
			}
		
		/* Read per-eye state: */
		for(int eye=0;eye<2;++eye)
			{
			/* Read eye's viewport: */
			source.read(eyes[eye].viewport,4);
			
			/* Read eye's distortion mesh: */
			DistortionMeshVertex* dmPtr=eyes[eye].distortionMesh;
			for(unsigned int y=0;y<distortionMeshSize[1];++y)
				for(unsigned int x=0;x<distortionMeshSize[0];++x,++dmPtr)
					{
					source.read(dmPtr->red.getComponents(),2);
					source.read(dmPtr->green.getComponents(),2);
					source.read(dmPtr->blue.getComponents(),2);
					}
			}
		
		if(++distortionMeshVersion==0U)
			++distortionMeshVersion;
		}
	}

}
