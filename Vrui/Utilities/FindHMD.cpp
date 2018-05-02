/***********************************************************************
FindHMD - Utility to find a connected HMD based on its preferred video
mode, using the X11 Xrandr extension.
Copyright (c) 2018 Oliver Kreylos

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

#include <string.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#define VERBOSE 0

int xrandrErrorBase;
bool hadError=false;

int errorHandler(Display* display,XErrorEvent* err)
	{
	if(err->error_code==BadValue)
		std::cout<<"X error: bad value"<<std::endl;
	else
		std::cout<<"X error: unknown error"<<std::endl;
	
	hadError=true;
	return 0;
	}

XRRModeInfo* findMode(XRRScreenResources* screenResources,RRMode modeId)
	{
	/* Find the mode ID in the screen resource's modes: */
	for(int i=0;i<screenResources->nmode;++i)
		if(screenResources->modes[i].id==modeId)
			return &screenResources->modes[i];
	
	return 0;
	}

void printMode(std::ostream& os,XRRScreenResources* screenResources,RRMode modeId)
	{
	XRRModeInfo* mode=findMode(screenResources,modeId);
	if(mode!=0)
		os<<mode->width<<'x'<<mode->height<<'@'<<double(mode->dotClock)/(double(mode->vTotal)*double(mode->hTotal));
	else
		os<<"<not found>";
	}

int main(int argc,char* argv[])
	{
	/* Parse the command line: */
	const char* displayName=getenv("DISPLAY");
	unsigned int size[2]={2160,1200};
	double rate=89.5273;
	double rateFuzz=0.01;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"display")==0)
				{
				++i;
				displayName=argv[i];
				}
			else if(strcasecmp(argv[i]+1,"size")==0)
				{
				for(int j=0;j<2;++j)
					{
					++i;
					size[j]=atoi(argv[i]);
					}
				}
			else if(strcasecmp(argv[i]+1,"rate")==0)
				{
				++i;
				rate=atof(argv[i]);
				}
			else if(strcasecmp(argv[i]+1,"rateFuzz")==0)
				{
				++i;
				rateFuzz=atof(argv[i]);
				}
			}
		}
	
	if(displayName==0)
		{
		std::cerr<<"FindHMD: No display name provided"<<std::endl;
		return 1;
		}
	
	/* Open a connection to the X display: */
	Display* display=XOpenDisplay(displayName);
	if(display==0)
		{
		std::cerr<<"FindHMD: Unable to connect to display "<<displayName<<std::endl;
		return 1;
		}
	
	/* Set the error handler: */
	XSetErrorHandler(errorHandler);
	
	/* Query the Xrandr extension: */
	int xrandrEventBase;
	if(!XRRQueryExtension(display,&xrandrEventBase,&xrandrErrorBase))
		{
		std::cerr<<"FindHMD: Display "<<displayName<<" does not support RANDR extension"<<std::endl;
		XCloseDisplay(display);
		return 1;
		}
	int xrandrMajor,xrandrMinor;
	if(XRRQueryVersion(display,&xrandrMajor,&xrandrMinor)==0)
		{
		std::cerr<<"FindHMD: Display "<<displayName<<" does not support RANDR extension"<<std::endl;
		XCloseDisplay(display);
		return 1;
		}
	
	#if VERBOSE
	std::cout<<"FindHMD: Found RANDR extension version "<<xrandrMajor<<'.'<<xrandrMinor<<std::endl;
	#endif
	
	/* Iterate through all of the display's screens: */
	std::string hmdOutputName;
	bool hmdEnabled=false;
	for(int hmdScreen=0;hmdScreen<XScreenCount(display)&&hmdOutputName.empty();++hmdScreen)
		{
		/* Get the root screen's resources: */
		XRRScreenResources* screenResources=XRRGetScreenResources(display,RootWindow(display,hmdScreen));
		
		#if VERBOSE
		std::cout<<"FindHMD: Screen "<<hmdScreen<<" has "<<screenResources->noutput<<" outputs and "<<screenResources->ncrtc<<" CRTCs"<<std::endl;
		#endif
		
		/* Calculate the bounding box of all enabled outputs on this screen: */
		int hmdScreenBbox[4]={32768,32768,-32768,-32768};
		for(int i=0;i<screenResources->noutput;++i)
			{
			/* Get the output descriptor and check if there is a display connected and enabled: */
			XRROutputInfo* outputInfo=XRRGetOutputInfo(display,screenResources,screenResources->outputs[i]);
			if(outputInfo->connection==RR_Connected&&outputInfo->crtc!=None)
				{
				/* Get the CRTC state for this output: */
				XRRCrtcInfo* crtcInfo=XRRGetCrtcInfo(display,screenResources,outputInfo->crtc);
				#if VERBOSE
				std::cout<<"\tCRTC window: "<<crtcInfo->width<<'x'<<crtcInfo->height<<'+'<<crtcInfo->x<<'+'<<crtcInfo->y<<std::endl;
				#endif
				
				/* Add the CRTC's window to the screen's bounding box: */
				if(hmdScreenBbox[0]>crtcInfo->x)
					hmdScreenBbox[0]=crtcInfo->x;
				if(hmdScreenBbox[1]>crtcInfo->y)
					hmdScreenBbox[1]=crtcInfo->y;
				if(hmdScreenBbox[2]<int(crtcInfo->x+crtcInfo->width))
					hmdScreenBbox[2]=int(crtcInfo->x+crtcInfo->width);
				if(hmdScreenBbox[3]<int(crtcInfo->y+crtcInfo->height))
					hmdScreenBbox[3]=int(crtcInfo->y+crtcInfo->height);
				
				/* Clean up: */
				XRRFreeCrtcInfo(crtcInfo);
				}
			
			/* Clean up: */
			XRRFreeOutputInfo(outputInfo);
			}
			
		#if VERBOSE
		std::cout<<"FindHMD: Screen "<<hmdScreen<<" has bounding box "<<hmdScreenBbox[0]<<", "<<hmdScreenBbox[1]<<", "<<hmdScreenBbox[2]<<", "<<hmdScreenBbox[3]<<std::endl;
		#endif
		
		/* Check all outputs for a connected display with a preferred mode matching the query: */
		for(int i=0;i<screenResources->noutput&&hmdOutputName.empty();++i)
			{
			/* Get the output descriptor and check if there is a display connected: */
			XRROutputInfo* outputInfo=XRRGetOutputInfo(display,screenResources,screenResources->outputs[i]);
			if(outputInfo->connection==RR_Connected&&outputInfo->nmode>0&&outputInfo->npreferred>0&&outputInfo->npreferred<=outputInfo->nmode)
				{
				#if VERBOSE
				std::cout<<"FindHMD: Output "<<std::string(outputInfo->name,outputInfo->name+outputInfo->nameLen)<<" modes:";
				for(int j=0;j<outputInfo->nmode;++j)
					{
					std::cout<<' ';
					printMode(std::cout,screenResources,outputInfo->modes[j]);
					}
				std::cout<<std::endl;
				std::cout<<"\tpreferred mode: ";
				printMode(std::cout,screenResources,outputInfo->modes[outputInfo->npreferred-1]);
				std::cout<<std::endl;
				#endif
				
				/* Retrieve a mode descriptor for the connected display's preferred mode: */
				XRRModeInfo* preferredMode=findMode(screenResources,outputInfo->modes[outputInfo->npreferred-1]);
				if(preferredMode!=0)
					{
					#if VERBOSE
					std::cout<<"FindHMD: Output "<<std::string(outputInfo->name,outputInfo->name+outputInfo->nameLen);
					std::cout<<" preferred mode is ";
					printMode(std::cout,screenResources,outputInfo->modes[outputInfo->npreferred-1]);
					std::cout<<std::endl;
					#endif
					
					/* Check if the connected display's preferred mode matches the query: */
					if(preferredMode->width==size[0]&&preferredMode->height==size[1])
						{
						/* Calculate the mode's refresh rate and check if it matches the query: */
						double modeRate=double(preferredMode->dotClock)/(double(preferredMode->vTotal)*double(preferredMode->hTotal));
						if(modeRate>=rate/(rateFuzz+1.0)&&modeRate<=rate*(rateFuzz+1.0))
							{
							/* Remember the video output port to which the HMD is connected: */
							hmdOutputName=std::string(outputInfo->name,outputInfo->name+outputInfo->nameLen);
							
							/* Check if the HMD's display is enabled: */
							hmdEnabled=outputInfo->crtc!=None;
							}
						}
					}
				}
			
			/* Clean up: */
			XRRFreeOutputInfo(outputInfo);
			}
		
		/* Clean up: */
		XRRFreeScreenResources(screenResources);
		}
	
	if(hmdOutputName.empty())
		{
		std::cerr<<"FindHMD: No HMD matching display specifications "<<size[0]<<'x'<<size[1]<<'@'<<rate<<" found"<<std::endl;
		XCloseDisplay(display);
		return 1;
		}
	else
		{
		std::cout<<hmdOutputName<<std::endl;
		
		if(!hmdEnabled)
			{
			std::cerr<<"FindHMD: HMD found on video output port "<<hmdOutputName<<", but is not enabled"<<std::endl;
			XCloseDisplay(display);
			return 2;
			}
		}
	
	/* Clean up and return: */
	XCloseDisplay(display);
	return 0;
	}
