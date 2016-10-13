/***********************************************************************
CrosstalkCorrector - Helper class to render sterep imagery into an off-
screen  buffer and then apply a crosstalk correction filter between the
left and right images to improve stereo quality on shared-screen stereo
displays.
Copyright (c) 2015 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_CROSSTALKCORRECTOR_INCLUDED
#define VRUI_INTERNAL_CROSSTALKCORRECTOR_INCLUDED

#include <GL/gl.h>
#include <GL/GLShader.h>

class CrosstalkCorrector
	{
	/* Elements: */
	private:
	int precorrectionFrameSize[2]; // Width and height of pre-correction frame buffer
	int precorrectionMultisamplingLevel; // Multisampling level in the pre-correction frame buffer
	int predistortionStencilBufferSize; // Bit depth of the optional pre-correction stencil buffer
	GLuint precorrectionFrameBufferId; // ID of the pre-correction frame buffer
	GLuint precorrectionColorBufferIds[2]; // IDs of the left and right pre-correction color image textures
	GLuint precorrectionMultisamplingColorBufferId; // ID of the shared pre-correction multisampling color buffer
	GLuint precorrectionDepthStencilBufferId; // ID of the pre-correction depth buffer, potentially interleaved with a stencil buffer
	GLuint multisamplingFrameBufferId; // ID of a frame buffer to "fix" a multisampled image texture into a regular image texture
	
	GLShader correctionShader; // GLSL shader to correct the pre-correction color image buffers into the final drawable
	int correctionShaderAttributeIndices[3]; // Attribute indices of the correction shader's attribute variables
	int correctionShaderUniformIndices[4]; // Locations of the correction shader's uniform variables
	
	
	};

#endif
