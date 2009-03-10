/***********************************************************************
SoundRecorder - Simple class to record sound from a capture device to a
sound file on the local file system. Uses ALSA under Linux, and the Core
Audio frameworks under Mac OS X.
Copyright (c) 2008-2009 Oliver Kreylos

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

#ifndef SOUND_SOUNDRECORDER_INCLUDED
#define SOUND_SOUNDRECORDER_INCLUDED

#ifdef SOUND_USE_ALSA
#include <Misc/File.h>
#include <Threads/Thread.h>
#endif
#ifdef __DARWIN__
#include <CoreAudio/CoreAudioTypes.h>
#include <AudioToolbox/AudioQueue.h>
#include <AudioToolbox/AudioConverter.h>
#include <AudioToolbox/AudioFile.h>
#endif
#include <Sound/SoundDataFormat.h>
#ifdef SOUND_USE_ALSA
#include <Sound/ALSAPCMDevice.h>
#endif

namespace Sound {

class SoundRecorder
	{
	#ifdef SOUND_USE_ALSA
	/* Embedded classes: */
	private:
	enum AudioFileFormat // Enumerated type for audio file formats
		{
		RAW,WAV
		};
	#endif
	
	/* Elements: */
	private:
	#ifdef __LINUX__
	SoundDataFormat format; // The actual format of the recorded sound data
	#ifdef SOUND_USE_ALSA
	AudioFileFormat outputFileFormat; // Format of the output file
	size_t bytesPerFrame; // Number of bytes per frame of sound data
	ALSAPCMDevice pcmDevice; // The ALSA PCM device used for recording
	Misc::File outputFile; // File to which to write the sound data
	size_t sampleBufferSize; // Size of the sample buffer in frames
	char* sampleBuffer; // A buffer to read sound data from the PCM device
	size_t numRecordedFrames; // Total number of frames written to the output file
	Threads::Thread recordingThread; // Thread ID of the background recording thread
	#endif
	#endif
	#ifdef __DARWIN__
	AudioStreamBasicDescription format; // Audio data format description
	AudioQueueRef queue; // Handle of the audio recording queue
	AudioFileID audioFile; // Handle of the audio file
	UInt32 bufferSize; // Buffer size in bytes
	AudioQueueBufferRef buffers[2]; // Array of audio buffers
	SInt64 numRecordedPackets; // Total number of packets written to the output file
	#endif
	bool active; // Flag whether the sound recorder is currently recording
	
	/* Private methods: */
	#ifdef SOUND_USE_ALSA
	void writeWAVHeader(void); // Writes a valid WAV file header to the beginning of the output file
	void* recordingThreadMethod(void); // The background recording thread's method
	#endif
	#ifdef __DARWIN__
	void setAudioFileMagicCookie(void); // Writes the audio queue's "magic cookie" into the audio file
	static void handleInputBufferWrapper(void* aqData,AudioQueueRef inAQ,AudioQueueBufferRef inBuffer,const AudioTimeStamp* inStartTime,UInt32 inNumPackets,const AudioStreamPacketDescription* inPacketDesc)
		{
		/* Get a pointer to the SoundRecorder structure: */
		SoundRecorder* thisPtr=static_cast<SoundRecorder*>(aqData);
		
		/* Call the actual handler method: */
		thisPtr->handleInputBuffer(inAQ,inBuffer,inStartTime,inNumPackets,inPacketDesc);
		}
	void handleInputBuffer(AudioQueueRef inAQ,AudioQueueBufferRef inBuffer,const AudioTimeStamp* inStartTime,UInt32 inNumPackets,const AudioStreamPacketDescription* inPacketDesc);
	#endif
	
	/* Constructors and destructors: */
	public:
	SoundRecorder(const SoundDataFormat& sFormat,const char* outputFileName); // Creates a sound recorder for the given data format and output file name
	private:
	SoundRecorder(const SoundRecorder& source); // Prohibit copy constructor
	SoundRecorder& operator=(const SoundRecorder& source); // Prohibit assignment operator
	public:
	~SoundRecorder(void); // Destroys a sound recorder
	
	/* Methods: */
	SoundDataFormat getSoundDataFormat(void) const; // Returns the actual sound format used by the sound recorder
	void start(void); // Starts recording to the output file
	void stop(void); // Stops recording to the output file
	bool isRecording(void) const // Returns true if the sound recorder is currently recording
		{
		return active;
		}
	};

}

#endif
