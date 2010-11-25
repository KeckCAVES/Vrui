/***********************************************************************
ALSAAudioCaptureDevice - Wrapper class around audio capture devices as
represented by the ALSA sound library.
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

#ifndef SOUND_LINUX_ALSAAUDIOCAPTUREDEVICE_INCLUDED
#define SOUND_LINUX_ALSAAUDIOCAPTUREDEVICE_INCLUDED

#include <Sound/AudioCaptureDevice.h>
#include <alsa/asoundlib.h>

namespace Sound {

class ALSAAudioCaptureDevice:public AudioCaptureDevice
	{
	/* Embedded classes: */
	public:
	class DeviceId:public AudioCaptureDevice::DeviceId
		{
		friend class ALSAAudioCaptureDevice;
		
		/* Elements: */
		private:
		std::string pcmDeviceName; // PCM name of the device
		
		/* Constructors and destructors: */
		public:
		DeviceId(std::string sName)
			:AudioCaptureDevice::DeviceId(sName)
			{
			}
		};
	
	/* Elements: */
	private:
	snd_pcm_t* pcmDevice; // Handle to the ALSA PCM device
	snd_pcm_hw_params_t* pcmHwParams; // Hardware parameter context for the PCM device; used to accumulate settings until streaming is started
	
	/* Constructors and destructors: */
	public:
	ALSAAudioCaptureDevice(const char* pcmDeviceName); // Opens the given ALSA PCM device for capture
	virtual ~ALSAAudioCaptureDevice(void); // Closes the ALSA PCM device
	
	/* Methods from AudioCaptureDevice: */
	virtual SoundDataFormat getAudioFormat(void) const;
	virtual SoundDataFormat& setAudioFormat(SoundDataFormat& newFormat);
	virtual unsigned int allocateFrameBuffers(unsigned int requestedFrameBufferSize,unsigned int requestedNumFrameBuffers);
	virtual void startStreaming(void);
	virtual void startStreaming(StreamingCallback* newStreamingCallback);
	virtual FrameBuffer* dequeueFrame(void);
	virtual void enqueueFrame(FrameBuffer* frame);
	virtual void stopStreaming(void);
	virtual void releaseFrameBuffers(void);
	
	/* New methods: */
	static void enumerateDevices(std::vector<DeviceIdPtr>& devices); // Appends device ID objects for all available ALSA audio capture devices to the given list
	};

}

#endif
