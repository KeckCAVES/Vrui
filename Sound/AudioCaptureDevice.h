/***********************************************************************
AudioCaptureDevice - Base class for audio capture devices.
Copyright (c) 2010 Oliver Kreylos

This file is part of the Basic Sound Library (Sound).

The Basic Sound Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The Basic Sound Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Basic Sound Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SOUND_AUDIOCAPTUREDEVICE_INCLUDED
#define SOUND_AUDIOCAPTUREDEVICE_INCLUDED

#include <string>
#include <vector>
#include <Misc/RefCounted.h>
#include <Misc/Autopointer.h>
#include <Sound/SoundDataFormat.h>

/* Forward declarations: */
namespace Misc {
template <class ParameterParam>
class FunctionCall;
}
namespace Sound {
class FrameBuffer;
}

namespace Sound {

class AudioCaptureDevice
	{
	/* Embedded classes: */
	public:
	class DeviceId:public Misc::RefCounted // Class to uniquely identify audio capture devices across different device classes
		{
		/* Elements: */
		protected:
		std::string name; // Human-readable device name
		
		/* Constructors and destructors: */
		public:
		DeviceId(std::string sName)
			:name(sName)
			{
			}
		virtual ~DeviceId(void)
			{
			}
		
		/* Methods: */
		std::string getName(void) const // Returns the human-readable device name
			{
			return name;
			}
		};
	
	typedef Misc::Autopointer<DeviceId> DeviceIdPtr; // Type for smart pointers to device ID objects
	typedef Misc::FunctionCall<const FrameBuffer*> StreamingCallback; // Function call type for streaming capture callback
	
	/* Elements: */
	protected:
	StreamingCallback* streamingCallback; // Function called when a frame buffer becomes ready in streaming capture mode
	
	/* Constructors and destructors: */
	public:
	AudioCaptureDevice(void); // Creates audio capture device
	virtual ~AudioCaptureDevice(void); // Closes the audio capture device
	
	/* Device enumeration and creation methods: */
	static std::vector<DeviceIdPtr> getDevices(void); // Returns a list of device IDs for all audio capture devices currently available on the system
	static AudioCaptureDevice* createDevice(DeviceIdPtr deviceId); // Creates an audio capture device for the given device ID
	
	/* Methods: */
	virtual SoundDataFormat getAudioFormat(void) const =0; // Returns the audio device's current audio format
	virtual SoundDataFormat& setAudioFormat(SoundDataFormat& newFormat) =0; // Sets the audio device's audio format to the most closely matching supported format; changes the given format structure to the actually set format
	
	/* Streaming capture interface methods: */
	virtual unsigned int allocateFrameBuffers(unsigned int requestedFrameBufferSize,unsigned int requestedNumFrameBuffers) =0; // Allocates the given number of streaming frame buffers of the given sizes; returns actual number of buffers allocated by device
	virtual void startStreaming(void); // Starts streaming audio capture using a previously allocated set of frame buffers
	virtual void startStreaming(StreamingCallback* newStreamingCallback); // Ditto; calls callback from separate thread whenever a new frame buffer becomes ready
	virtual FrameBuffer* dequeueFrame(void) =0; // Returns the next frame buffer captured from the audio device; blocks if no frames are ready
	virtual void enqueueFrame(FrameBuffer* frame) =0; // Returns the given frame buffer to the capturing queue after the caller is done with it
	virtual void stopStreaming(void); // Stops streaming audio capture
	virtual void releaseFrameBuffers(void) =0; // Releases all previously allocated frame buffers
	};

}

#endif
