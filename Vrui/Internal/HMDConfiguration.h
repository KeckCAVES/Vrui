/***********************************************************************
HMDConfiguration - Class to represent the internal configuration of a
head-mounted display.
Copyright (c) 2016 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_HMDCONFIGURATION_INCLUDED
#define VRUI_INTERNAL_HMDCONFIGURATION_INCLUDED

#include <Misc/SizedTypes.h>
#include <Geometry/Point.h>
#include <Vrui/Internal/VRDevicePipe.h>

namespace Vrui {

class HMDConfiguration
	{
	/* Embedded classes: */
	public:
	typedef Geometry::Point<Misc::Float32,3> Point;
	
	struct DistortionMeshVertex // Structure for distortion mesh vertices
		{
		/* Elements: */
		public:
		Misc::Float32 red[2]; // Distortion-corrected vertex position for red color component
		Misc::Float32 green[2]; // Distortion-corrected vertex position for green color component
		Misc::Float32 blue[2]; // Distortion-corrected vertex position for blue color component
		};
	
	struct EyeConfiguration // Structure defining the configuration of one eye
		{
		/* Elements: */
		public:
		Misc::UInt32 viewport[4]; // Eye's viewport (x, y, width, height) in final display window
		Misc::Float32 fov[4]; // Left, right, bottom, and top field-of-view boundaries in tangent space
		DistortionMeshVertex* distortionMesh; // 2D array of distortion mesh vertices
		};
	
	/* Elements: */
	private:
	Misc::UInt16 trackerIndex; // Tracker index of the HMD
	Point eyePos[2]; // Positions of left and right eyes in HMD device coordinates
	unsigned int eyePosVersion; // Version number of the eye positions
	Misc::UInt32 renderTargetSize[2]; // Recommended width and height of per-eye pre-distortion render target
	Misc::UInt32 distortionMeshSize[2]; // Number of vertices in per-eye lens distortion correction meshes
	EyeConfiguration eyes[2]; // Configurations of the left and right eyes
	unsigned int eyeVersion; // Version number of the eye configurations
	unsigned int distortionMeshVersion; // Version number of the distortion meshes
	
	/* Constructors and destructors: */
	public:
	HMDConfiguration(void); // Creates uninitialized HMD configuration structure
	~HMDConfiguration(void);
	
	/* Methods: */
	Misc::UInt16 getTrackerIndex(void) const // Returns the index of the tracker tracking this HMD
		{
		return trackerIndex;
		}
	const Point& getEyePosition(int eyeIndex) const // Returns the position of the left or right eye
		{
		return eyePos[eyeIndex];
		}
	const Misc::UInt32* getRenderTargetSize(void) const // Returns the recommended per-eye render target size
		{
		return renderTargetSize;
		}
	const Misc::UInt32* getDistortionMeshSize(void) const // Returns the per-eye distortion mesh size
		{
		return distortionMeshSize;
		}
	const Misc::UInt32* getViewport(int eyeIndex) const // Returns the final display window viewport for the given eye
		{
		return eyes[eyeIndex].viewport;
		}
	const Misc::Float32* getFov(int eyeIndex) const // Returns the tangent-space field-of-view boundaries for the given eye
		{
		return eyes[eyeIndex].fov;
		}
	const DistortionMeshVertex* getDistortionMesh(int eyeIndex) const; // Returns a pointer to the given eye's distortion mesh for reading
	void setTrackerIndex(Misc::UInt16 newTrackerIndex); // Sets the index of the tracker tracking this HMD
	void setEyePos(const Point& leftPos,const Point& rightPos); // Sets left and right eye positions directly
	void setIpd(float newIPD); // Sets left and right eye positions based on previous positions and new inter-pupillary distance
	void setRenderTargetSize(unsigned int newWidth,unsigned int newHeight); // Sets a new recommended render target size
	void setDistortionMeshSize(unsigned int newWidth,unsigned int newHeight); // Sets a new distortion mesh size; resets mesh to undefined if size changed
	void setViewport(int eye,unsigned int x,unsigned int y,unsigned int width,unsigned int height); // Sets the given eye's final display window viewport
	void setFov(int eye,float left,float right,float bottom,float top); // Sets the given eye's tangent space field-of-view boundaries
	DistortionMeshVertex* getDistortionMesh(int eye); // Returns a pointer to the given eye's distortion mesh for updates
	void updateDistortionMeshes(void); // Marks the distortion mesh as updated after access is complete
	void write(unsigned int sinkEyePosVersion,unsigned int sinkEyeVersion,unsigned int sinkDistortionMeshVersion,VRDevicePipe& sink) const; // Writes outdated components of an HMD configuration to the given sink
	void read(VRDevicePipe::MessageIdType messageId,Misc::UInt16 newTrackerIndex,VRDevicePipe& source); // Reads an HMD configuration from the given source after receiving the given update message ID
	unsigned int getEyePosVersion(void) const
		{
		return eyePosVersion;
		}
	unsigned int getEyeVersion(void) const
		{
		return eyeVersion;
		}
	unsigned int getDistortionMeshVersion(void) const
		{
		return distortionMeshVersion;
		}
	};

}

#endif
