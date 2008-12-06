/***********************************************************************
SoundDataFormat - System-independent data structure to describe the
format of sound data.
Copyright (c) 2008 Oliver Kreylos

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

#include <Sound/SoundDataFormat.h>

namespace Sound {

/********************************
Methods of class SoundDataFormat:
********************************/

#ifdef SOUND_USE_ALSA

snd_pcm_format_t SoundDataFormat::getPCMFormat(void) const
	{
	snd_pcm_format_t pcmFormat;
	if(bitsPerSample==8)
		pcmFormat=signedSamples?SND_PCM_FORMAT_S8:SND_PCM_FORMAT_U8;
	else if(bitsPerSample==16)
		{
		if(sampleEndianness==SoundDataFormat::LittleEndian)
			pcmFormat=signedSamples?SND_PCM_FORMAT_S16_LE:SND_PCM_FORMAT_U16_LE;
		else if(sampleEndianness==SoundDataFormat::BigEndian)
			pcmFormat=signedSamples?SND_PCM_FORMAT_S16_BE:SND_PCM_FORMAT_U16_BE;
		else
			pcmFormat=signedSamples?SND_PCM_FORMAT_S16:SND_PCM_FORMAT_U16;
		}
	else if(bitsPerSample==24)
		{
		if(sampleEndianness==SoundDataFormat::LittleEndian)
			pcmFormat=signedSamples?SND_PCM_FORMAT_S24_LE:SND_PCM_FORMAT_U24_LE;
		else if(sampleEndianness==SoundDataFormat::BigEndian)
			pcmFormat=signedSamples?SND_PCM_FORMAT_S24_BE:SND_PCM_FORMAT_U24_BE;
		else
			pcmFormat=signedSamples?SND_PCM_FORMAT_S24:SND_PCM_FORMAT_U24;
		}
	else if(bitsPerSample==32)
		{
		if(sampleEndianness==SoundDataFormat::LittleEndian)
			pcmFormat=signedSamples?SND_PCM_FORMAT_S32_LE:SND_PCM_FORMAT_U32_LE;
		else if(sampleEndianness==SoundDataFormat::BigEndian)
			pcmFormat=signedSamples?SND_PCM_FORMAT_S32_BE:SND_PCM_FORMAT_U32_BE;
		else
			pcmFormat=signedSamples?SND_PCM_FORMAT_S32:SND_PCM_FORMAT_U32;
		}
	else
		pcmFormat=SND_PCM_FORMAT_UNKNOWN;
	
	return pcmFormat;
	}

int SoundDataFormat::setPCMDeviceParameters(snd_pcm_t* pcmDevice) const
	{
	int error;
	
	/* Allocate a hardware parameter context: */
	snd_pcm_hw_params_t* pcmHwParams;
	if((error=snd_pcm_hw_params_malloc(&pcmHwParams))<0)
		return error;
	
	/* Get the current parameters from the PCM device: */
	if((error=snd_pcm_hw_params_any(pcmDevice,pcmHwParams))<0)
		{
		snd_pcm_hw_params_free(pcmHwParams);
		return error;
		}
	
	/* Set the PCM device's access method: */
	if((error=snd_pcm_hw_params_set_access(pcmDevice,pcmHwParams,SND_PCM_ACCESS_RW_INTERLEAVED))<0)
		{
		snd_pcm_hw_params_free(pcmHwParams);
		return error;
		}
	
	/* Set the PCM device's sample format: */
	snd_pcm_format_t pcmFormat=getPCMFormat();
	if((error=snd_pcm_hw_params_set_format(pcmDevice,pcmHwParams,pcmFormat))<0)
		{
		snd_pcm_hw_params_free(pcmHwParams);
		return error;
		}
	
	/* Set the PCM device's number of channels: */
	if((error=snd_pcm_hw_params_set_channels(pcmDevice,pcmHwParams,samplesPerFrame))<0)
		{
		snd_pcm_hw_params_free(pcmHwParams);
		return error;
		}
	
	#if 0
	/* Enable PCM device's hardware resampling for non-native sample rates: */
	if((error=snd_pcm_hw_params_set_rate_resample(pcmDevice,pcmHwParams,1))<0)
		return error;
	#endif
	
	/* Set the PCM device's sample rate: */
	unsigned int sampleRate=framesPerSecond;
	if((error=snd_pcm_hw_params_set_rate_near(pcmDevice,pcmHwParams,&sampleRate,0))<0)
		{
		snd_pcm_hw_params_free(pcmHwParams);
		return error;
		}
	
	/* Check if the requested sample rate was correctly set: */
	if(sampleRate!=(unsigned int)(framesPerSecond))
		{
		snd_pcm_hw_params_free(pcmHwParams);
		return error;
		}
	
	/* Write the changed hardware parameter set to the PCM device: */
	if((error=snd_pcm_hw_params(pcmDevice,pcmHwParams))<0)
		{
		snd_pcm_hw_params_free(pcmHwParams);
		return error;
		}
	
	/* Clean up and signal success: */
	snd_pcm_hw_params_free(pcmHwParams);
	return 0;
	}

#endif

}
