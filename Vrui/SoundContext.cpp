/***********************************************************************
SoundContext - Class for OpenAL contexts that are used to map a listener
to an OpenAL sound device.
Copyright (c) 2008 Oliver Kreylos

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

#include <string>
#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#ifdef VRUI_USE_OPENAL
#ifdef __DARWIN__
#include <OpenAL/alc.h>
#include <OpenAL/al.h>
#else
#include <AL/alc.h>
#include <AL/al.h>
#endif
#endif
#include <AL/ALContextData.h>
#include <Vrui/Vrui.h>
#include <Vrui/Vrui.Internal.h>

#include <Vrui/SoundContext.h>

namespace Vrui {

/*****************************
Methods of class SoundContext:
*****************************/

SoundContext::SoundContext(const Misc::ConfigurationFileSection& configFileSection,VruiState* sVruiState)
	:vruiState(sVruiState),
	 #ifdef VRUI_USE_OPENAL
	 alDevice(0),alContext(0),
	 #endif
	 contextData(0),
	 listener(findListener(configFileSection.retrieveString("./listenerName").c_str()))
	{
	#ifdef VRUI_USE_OPENAL
	/* Open the OpenAL device: */
	std::string alDeviceName=configFileSection.retrieveValue<std::string>("./deviceName","Default");
	alDevice=alcOpenDevice(alDeviceName!="Default"?alDeviceName.c_str():0);
	if(alDevice==0)
		Misc::throwStdErr("SoundContext::SoundContext: Could not open OpenAL sound device %s",alDeviceName.c_str());
	
	/* Create an OpenAL context: */
	alContext=alcCreateContext(alDevice,0);
	if(alContext==0)
		{
		alcCloseDevice(alDevice);
		Misc::throwStdErr("SoundContext::SoundContext: Could not create OpenAL context for sound device %s",alDeviceName.c_str());
		}
	#endif
	
	/* Create an AL context data object: */
	contextData=new ALContextData(101);
	
	/* Initialize the sound context's OpenAL context: */
	makeCurrent();
	
	/* Initialize application sound state: */
	if(vruiState->perSoundInitFunction!=0)
		vruiState->perSoundInitFunction(*contextData,vruiState->perSoundInitFunctionData);
	}

SoundContext::~SoundContext(void)
	{
	ALContextData::makeCurrent(0);
	delete contextData;
	
	#ifdef VRUI_USE_OPENAL
	alcDestroyContext(alContext);
	alcCloseDevice(alDevice);
	#endif
	}

void SoundContext::makeCurrent(void)
	{
	/* Activate the sound context's OpenAL context: */
	#ifdef VRUI_USE_OPENAL
	alcMakeContextCurrent(alContext);
	#endif
	
	/* Install the sound context's AL context data manager: */
	ALContextData::makeCurrent(contextData);
	}

void SoundContext::draw(void)
	{
	#ifdef VRUI_USE_OPENAL
	makeCurrent();
	
	/* Update things in the sound context's AL context data: */
	contextData->updateThings();
	
	/* Set OpenAL parameters: */
	
	/* Set the listener: */
	
	/* Render Vrui state: */
	vruiState->sound(*contextData);
	#endif
	}

}
