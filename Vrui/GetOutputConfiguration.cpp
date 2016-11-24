/***********************************************************************
GetOutputConfiguration - Helper function to find the physical size and
panning domain of an output connector or connected output device using
the XRANDR extension.
Copyright (c) 2014-2016 Oliver Kreylos

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

#include <Vrui/GetOutputConfiguration.h>

#include <string.h>
#include <string>
#include <iostream>
#include <X11/Xlib.h>
#include <Vrui/Internal/Vrui.h>
#include <Vrui/Internal/Config.h>
#if VRUI_INTERNAL_CONFIG_HAVE_XRANDR
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#endif

namespace Vrui {

/*********
Functions:
*********/

OutputConfiguration getOutputConfiguration(Display* display,int screen,const char* outputName)
	{
	/* Use the display's default screen if the given screen index is invalid: */
	bool matchScreen=screen>=0;
	if(screen<0)
		screen=DefaultScreen(display);
	
	/* Create a default output configuration by assuming the entire root window goes to a single output: */
	OutputConfiguration result;
	result.screen=screen;
	result.sizeMm[0]=DisplayWidthMM(display,screen);
	result.sizeMm[1]=DisplayHeightMM(display,screen);
	result.domainOrigin[0]=0;
	result.domainOrigin[1]=0;
	result.domainSize[0]=DisplayWidth(display,screen);
	result.domainSize[1]=DisplayHeight(display,screen);
	result.frameInterval=0U; // Unknown frame rate
	
	#if VRUI_INTERNAL_CONFIG_HAVE_XRANDR
	
	/* Check if the X server on the other end of the display connection understands XRANDR version >= 1.2: */
	int xrandrEventBase,xrandrErrorBase;
	if(!XRRQueryExtension(display,&xrandrEventBase,&xrandrErrorBase))
		return result;
	int xrandrMajor,xrandrMinor;
	if(!XRRQueryVersion(display,&xrandrMajor,&xrandrMinor)||xrandrMajor<1||(xrandrMajor==1&&xrandrMinor<2))
		return result;
	
	/* Query the display name if in verbose mode: */
	std::string displayName;
	if(vruiVerbose)
		{
		displayName=DisplayString(display);
		bool haveColon=false;
		std::string::iterator dotIt=displayName.end();
		for(std::string::iterator dnIt=displayName.begin();dnIt!=displayName.end();++dnIt)
			if(*dnIt==':')
				haveColon=true;
			else if(haveColon&&*dnIt=='.')
				dotIt=dnIt;
		displayName.erase(dotIt,displayName.end());
		}
	
	/* Iterate through all X screens belonging to the X display connection: */
	bool firstOutput=true;
	bool haveMatch=false;
	for(int testScreen=0;testScreen<XScreenCount(display)&&(!haveMatch||vruiVerbose);++testScreen)
		if(!matchScreen||testScreen==screen)
			{
			/* Get the screen's resources: */
			XRRScreenResources* screenResources=XRRGetScreenResources(display,RootWindow(display,testScreen));
			if(screenResources!=0)
				{
				/* Find the first CRT controller that has an output of the given name: */
				for(int crtcIndex=0;crtcIndex<screenResources->ncrtc&&(!haveMatch||vruiVerbose);++crtcIndex)
					{
					/* Get the CRT controller's information structure: */
					XRRCrtcInfo* crtcInfo=XRRGetCrtcInfo(display,screenResources,screenResources->crtcs[crtcIndex]);
					if(crtcInfo!=0)
						{
						/* Find the specification of the CRT controller's current mode: */
						unsigned int frameInterval=0U;
						for(int modeIndex=0;modeIndex<screenResources->nmode;++modeIndex)
							{
							XRRModeInfo& mode=screenResources->modes[modeIndex];
							if(mode.id==crtcInfo->mode)
								{
								#if 0
								/* Dump full modeline of detected CRT controller: */
								std::cout<<"\tModeline: "<<mode.dotClock<<' '<<mode.width<<' '<<mode.hSyncStart<<' '<<mode.hSyncEnd<<' '<<mode.hTotal<<' '<<mode.hSkew;
								std::cout<<' '<<mode.height<<' '<<mode.vSyncStart<<' '<<mode.vSyncEnd<<' '<<mode.vTotal<<std::endl;
								#endif
								
								/* Calculate CRT controller's frame interval: */
								frameInterval=(unsigned int)(((unsigned long)(mode.hTotal*mode.vTotal)*1000000000UL+mode.dotClock/2)/mode.dotClock);
								}
							}
						
						/* Try all outputs driven by the CRT controller: */
						for(int outputIndex=0;outputIndex<crtcInfo->noutput&&(!haveMatch||vruiVerbose);++outputIndex)
							{
							/* Get the output's information structure: */
							XRROutputInfo* outputInfo=XRRGetOutputInfo(display,screenResources,crtcInfo->outputs[outputIndex]);
							if(vruiVerbose&&outputName!=0&&outputName[0]!='\0')
								{
								std::cout<<"\tFound output "<<outputInfo->name<<" on display "<<displayName<<'.'<<testScreen;
								std::cout<<" at "<<crtcInfo->width<<'x'<<crtcInfo->height<<'+'<<crtcInfo->x<<'+'<<crtcInfo->y;
								std::cout<<" @ "<<1000000000.0/double(frameInterval)<<"Hz ("<<frameInterval<<")"<<std::endl;
								}
							
							/* Check if this output matches the search parameter: */
							bool matchesName=strcmp(outputInfo->name,outputName)==0;
							if(!matchesName||vruiVerbose)
								{
								/* Check if the output has an associated EDID property: */
								int numProperties;
								Atom* properties=XRRListOutputProperties(display,crtcInfo->outputs[outputIndex],&numProperties);
								for(int propertyIndex=0;propertyIndex<numProperties;++propertyIndex)
									{
									char* propertyName=XGetAtomName(display,properties[propertyIndex]);
									if(strcasecmp(propertyName,"EDID")==0)
										{
										Atom propertyType;
										int propertyFormat;
										unsigned long numItems;
										unsigned long bytes_after;
										unsigned char* propertyValue;
										XRRGetOutputProperty(display,crtcInfo->outputs[outputIndex],properties[propertyIndex],0,100,False,False,AnyPropertyType,&propertyType,&propertyFormat,&numItems,&bytes_after,&propertyValue);
										if(propertyType==XA_INTEGER&&propertyFormat==8)
											{
											/* Check the EDID's checksum and header ID: */
											unsigned char checksum=0;
											for(unsigned long i=0;i<numItems;++i)
												checksum+=propertyValue[i];
											unsigned char edidHeaderId[8]={0x00U,0xffU,0xffU,0xffU,0xffU,0xffU,0xffU,0x00U};
											bool headerOk=true;
											for(int i=0;i<8&&headerOk;++i)
												headerOk=propertyValue[i]==edidHeaderId[i];
											if(checksum==0&&headerOk)
												{
												/* Find the monitor name among the extension blocks: */
												unsigned char* blockPtr=propertyValue+0x36;
												for(int i=0;i<4&&(!haveMatch||vruiVerbose);++i,blockPtr+=18)
													if(blockPtr[0]==0x00U&&blockPtr[1]==0x00U&&blockPtr[2]==0x00U&&blockPtr[3]==0xfcU)
														{
														/* Extract the monitor name: */
														char monitorName[14];
														char* mnPtr=monitorName;
														for(unsigned char* namePtr=blockPtr+5;namePtr<blockPtr+18&&*namePtr!='\n';++namePtr,++mnPtr)
															*mnPtr=char(*namePtr);
														*mnPtr='\0';
														
														if(vruiVerbose&&outputName!=0&&outputName[0]!='\0')
															{
															std::cout<<"\tFound monitor "<<monitorName<<" on output "<<outputInfo->name<<" on display "<<displayName<<'.'<<testScreen;
															std::cout<<" at "<<crtcInfo->width<<'x'<<crtcInfo->height<<'+'<<crtcInfo->x<<'+'<<crtcInfo->y;
															std::cout<<" @ "<<1000000000.0/double(frameInterval)<<"Hz"<<std::endl;
															}
														
														matchesName=matchesName||strcmp(monitorName,outputName)==0;
														}
												}
											}
										XFree(propertyValue);
										}
									XFree(propertyName);
									}
								XFree(properties);
								}
							
							if(firstOutput||matchesName)
								{
								/* Remember the output's configuration: */
								result.screen=testScreen;
								result.sizeMm[0]=outputInfo->mm_width;
								result.sizeMm[1]=outputInfo->mm_height;
								result.domainOrigin[0]=crtcInfo->x;
								result.domainOrigin[1]=crtcInfo->y;
								result.domainSize[0]=crtcInfo->width;
								result.domainSize[1]=crtcInfo->height;
								result.frameInterval=frameInterval;
								
								firstOutput=false;
								haveMatch=matchesName||outputName==0||outputName[0]=='\0';
								}
							
							XRRFreeOutputInfo(outputInfo);
							}
						
						XRRFreeCrtcInfo(crtcInfo);
						}
					}
				
				XRRFreeScreenResources(screenResources);
				}
			}
	
	if(!haveMatch&&outputName!=0&&outputName[0]!='\0')
		std::cerr<<"\tOutput \""<<outputName<<"\" not found on display "<<DisplayString(display)<<std::endl;
	
	#endif
	
	/* Check the result configuration for sanity: */
	if(result.sizeMm[0]==0||result.sizeMm[1]==0)
		{
		/* Assign a default display size based on a fixed and uniform resolution: */
		int dpi=96; // Seems to be a reasonable number
		if(vruiVerbose)
			std::cout<<"\tSelected output advertises zero physical size; using default resolution of "<<dpi<<" dpi"<<std::endl;
		for(int i=0;i<2;++i)
			result.sizeMm[i]=(254*result.domainSize[i]+dpi*5)/(dpi*10);
		}
	
	return result;
	}

}
