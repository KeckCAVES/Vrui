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

#include <Sound/Linux/ALSAAudioCaptureDevice.h>

#include <stdio.h>
#include <Misc/ThrowStdErr.h>

namespace Sound {

/***************************************
Methods of class ALSAAudioCaptureDevice:
***************************************/

ALSAAudioCaptureDevice::ALSAAudioCaptureDevice(const char* pcmDeviceName)
	:pcmDevice(0),pcmHwParams(0)
	{
	/* Open the PCM device: */
	int error=snd_pcm_open(&pcmDevice,pcmDeviceName,SND_PCM_STREAM_CAPTURE,0);
	if(error<0)
		{
		pcmDevice=0;
		Misc::throwStdErr("Sound::ALSAAudioCaptureDevice::ALSAAudioCaptureDevice: Error %s while opening PCM device %s",snd_strerror(error),pcmDeviceName);
		}
	
	/* Allocate and initialize a hardware parameter context: */
	if((error=snd_pcm_hw_params_malloc(&pcmHwParams))<0)
		{
		snd_pcm_close(pcmDevice);
		Misc::throwStdErr("Sound::ALSAAudioCaptureDevice::ALSAAudioCaptureDevice: Error %s while allocating hardware parameter context",snd_strerror(error));
		}
	if((error=snd_pcm_hw_params_any(pcmDevice,pcmHwParams))<0)
		{
		snd_pcm_hw_params_free(pcmHwParams);
		snd_pcm_close(pcmDevice);
		Misc::throwStdErr("Sound::ALSAAudioCaptureDevice::ALSAAudioCaptureDevice: Error %s while initializing hardware parameter context",snd_strerror(error));
		}
	
	/* Set the PCM device's access method: */
	if((error=snd_pcm_hw_params_set_access(pcmDevice,pcmHwParams,SND_PCM_ACCESS_RW_INTERLEAVED))<0)
		{
		snd_pcm_hw_params_free(pcmHwParams);
		snd_pcm_close(pcmDevice);
		Misc::throwStdErr("Sound::ALSAAudioCaptureDevice::ALSAAudioCaptureDevice: Error %s while setting PCM device's access method",snd_strerror(error));
		}
	}

ALSAAudioCaptureDevice::~ALSAAudioCaptureDevice(void)
	{
	/* Destroy the hardware parameter context: */
	if(pcmHwParams!=0)
		snd_pcm_hw_params_free(pcmHwParams);
	
	/* Close the PCM device: */
	if(pcmDevice!=0)
		snd_pcm_close(pcmDevice);
	}

void ALSAAudioCaptureDevice::enumerateDevices(std::vector<AudioCaptureDevice::DeviceIdPtr>& devices)
	{
	/* Enumerate all ALSA cards and devices: */
	int cardIndex=-1; // Start with first card
	while(true)
		{
		/* Get the index of the next card: */
		if(snd_card_next(&cardIndex)!=0||cardIndex<0)
			{
			/* There was an error, or there are no more cards: */
			break;
			}
		
		/* Open the card's control interface: */
		char cardName[20];
		snprintf(cardName,sizeof(cardName),"hw:%d",cardIndex);
		snd_ctl_t* cardHandle;
		if(snd_ctl_open(&cardHandle,cardName,0)!=0)
			break;
		
		/* Enumerate all PCM devices on this card: */
		int numCardDevices=0;
		int pcmIndex=-1;
		while(true)
			{
			/* Get the index of the next PCM device: */
			if(snd_ctl_pcm_next_device(cardHandle,&pcmIndex)!=0||pcmIndex<0)
				{
				/* There was an error, or there are no more PCM devices: */
				break;
				}
			
			/* Create an info structure for the PCM device: */
			snd_pcm_info_t* pcmInfo;
			snd_pcm_info_alloca(&pcmInfo);
			snd_pcm_info_set_device(pcmInfo,pcmIndex);
			snd_pcm_info_set_stream(pcmInfo,SND_PCM_STREAM_CAPTURE);
			
			/* Get the number of capture subdevices for the device: */
			if(snd_ctl_pcm_info(cardHandle,pcmInfo)!=0)
				break;
			int numSubDevices=snd_pcm_info_get_subdevices_count(pcmInfo);
			for(int subDeviceIndex=0;subDeviceIndex<numSubDevices;++subDeviceIndex)
				{
				/* Query information about the subdevice: */
				snd_pcm_info_set_subdevice(pcmInfo,subDeviceIndex);
				if(snd_ctl_pcm_info(cardHandle,pcmInfo)==0)
					{
					/* Query the card's name: */
					char* cardName;
					if(snd_card_get_name(cardIndex,&cardName)==0)
						{
						/* Create a device ID: */
						std::string deviceName=cardName;
						free(cardName);
						if(numCardDevices>0)
							{
							char suffix[10];
							snprintf(suffix,sizeof(suffix),":%d",numCardDevices);
							deviceName.append(suffix);
							}
						DeviceId* newDeviceId=new DeviceId(deviceName);
						
						/* Set the PCM device name: */
						char pcmDeviceName[20];
						if(numSubDevices>1)
							snprintf(pcmDeviceName,sizeof(pcmDeviceName),"plughw:%d,%d,%d",snd_pcm_info_get_card(pcmInfo),snd_pcm_info_get_device(pcmInfo),snd_pcm_info_get_subdevice(pcmInfo));
						else
							snprintf(pcmDeviceName,sizeof(pcmDeviceName),"plughw:%d,%d",snd_pcm_info_get_card(pcmInfo),snd_pcm_info_get_device(pcmInfo));
						newDeviceId->pcmDeviceName=pcmDeviceName;
						
						/* Store the device ID: */
						devices.push_back(newDeviceId);
						
						++numCardDevices;
						}
					}
				}
			}
		
		/* Close the card's control interface: */
		snd_ctl_close(cardHandle);
		}
	}

}
