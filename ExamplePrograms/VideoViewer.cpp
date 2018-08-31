/***********************************************************************
VideoViewer - A simple viewer for live video from a video source
connected to the local computer.
Copyright (c) 2013-2018 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <Misc/FunctionCalls.h>
#include <Misc/Timer.h>
#include <Misc/MessageLogger.h>
#include <Math/Math.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <GL/gl.h>
#include <GL/GLMaterial.h>
#include <Images/RGBImage.h>
#include <Images/WriteImageFile.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/Button.h>
#include <Video/VideoDataFormat.h>
#include <Video/ViewerComponent.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>

class VideoViewer:public Vrui::Application
	{
	/* Elements: */
	private:
	Video::ViewerComponent* viewer; // Pointer to a video viewer application component
	volatile bool saveVideoFrames; // Flag to save video frames to disk as they arrive
	Misc::Timer saveVideoTimer; // A free-running timer to time-stamp saved video frames
	std::string saveVideoFrameNameTemplate; // Printf-style template to save video frames
	unsigned int saveVideoNextFrameIndex; // Index for the next video frame to be saved
	bool paused; // Flag to disable updates to the viewer's current image
	GLMotif::PopupMenu* mainMenu; // The program's main menu
	
	/* Private methods: */
	void videoFrameCallback(const Images::BaseImage& image); // Callback receiving incoming video frames
	void videoFormatChangedCallback(const Video::VideoDataFormat& format); // Callback called when the streamed video format changes
	void videoFormatSizeChangedCallback(const Video::VideoDataFormat& format); // Callback called when the streamed video format's frame size changes
	void showVideoDevicesDialogCallback(Misc::CallbackData* cbData); // Method to pop up the video device selection dialog
	void showControlPanelCallback(Misc::CallbackData* cbData); // Method to pop up the video device's control panel
	GLMotif::PopupMenu* createMainMenu(void); // Creates the program's main menu
	
	/* Constructors and destructors: */
	public:
	VideoViewer(int& argc,char**& argv);
	virtual ~VideoViewer(void);
	
	/* Methods from Vrui::Application: */
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	virtual void eventCallback(EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData);
	};

/****************************
Methods of class VideoViewer:
****************************/

void VideoViewer::videoFrameCallback(const Images::BaseImage& image)
	{
	double timeStamp=saveVideoTimer.peekTime();
	
	/* Wake up the main loop: */
	Vrui::requestUpdate();
	
	if(saveVideoFrames)
		{
		/* Create a filename for the new video frame: */
		char videoFrameFileName[1024];
		snprintf(videoFrameFileName,sizeof(videoFrameFileName),saveVideoFrameNameTemplate.c_str(),saveVideoNextFrameIndex);
		
		try
			{
			/* Save the new video frame: */
			Images::RGBImage saveImage(image);
			std::cout<<"Saving frame "<<videoFrameFileName<<" at "<<timeStamp*1000.0<<" ms..."<<std::flush;
			Images::writeImageFile(saveImage,videoFrameFileName);
			std::cout<<" done"<<std::endl;
			
			/* Increment the frame counter: */
			++saveVideoNextFrameIndex;
			}
		catch(std::runtime_error err)
			{
			/* Show an error message and carry on: */
			Misc::formattedUserError("VideoViewer: Unable to save frame to file %s due to exception %s",videoFrameFileName,err.what());
			}
		}
	}

void VideoViewer::videoFormatChangedCallback(const Video::VideoDataFormat& format)
	{
	/* Enable or disable the "Show Video Control Panel" button depending on whether there is a control panel: */
	mainMenu->findDescendant("_Menu/ShowControlPanelButton")->setEnabled(viewer->getVideoControlPanel()!=0);
	}

void VideoViewer::videoFormatSizeChangedCallback(const Video::VideoDataFormat& format)
	{
	/* Recenter the view on the new video stream: */
	resetNavigation();
	}

void VideoViewer::showVideoDevicesDialogCallback(Misc::CallbackData* cbData)
	{
	/* Pop up the video devices dialog: */
	Vrui::popupPrimaryWidget(viewer->getVideoDevicesDialog());
	}

void VideoViewer::showControlPanelCallback(Misc::CallbackData* cbData)
	{
	/* Pop up the video control panel: */
	Vrui::popupPrimaryWidget(viewer->getVideoControlPanel());
	}

GLMotif::PopupMenu* VideoViewer::createMainMenu(void)
	{
	/* Create the main menu shell: */
	GLMotif::PopupMenu* mainMenu=new GLMotif::PopupMenu("MainMenu",Vrui::getWidgetManager());
	mainMenu->setTitle("Video Viewer");
	
	/* Create a button to pop up the video devices dialog: */
	GLMotif::Button* showVideoDevicesDialogButton=new GLMotif::Button("ShowVideoDevicesDialogButton",mainMenu,"Show Video Device List");
	showVideoDevicesDialogButton->getSelectCallbacks().add(this,&VideoViewer::showVideoDevicesDialogCallback);
	
	/* Create a button to pop up the video control panel: */
	GLMotif::Button* showControlPanelButton=new GLMotif::Button("ShowControlPanelButton",mainMenu,"Show Video Device Controls");
	showControlPanelButton->getSelectCallbacks().add(this,&VideoViewer::showControlPanelCallback);
	if(viewer->getVideoControlPanel()==0)
		showControlPanelButton->setEnabled(false);
	
	/* Finish building the main menu: */
	mainMenu->manageMenu();
	return mainMenu;
	}

namespace {

/****************
Helper functions:
****************/

bool isIndex(const char* str)
	{
	while(*str!='\0')
		{
		if(*str<'0'||*str>'9')
			return false;
		++str;
		}
	return true;
	}

}

VideoViewer::VideoViewer(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 viewer(0),
	 saveVideoFrames(false),saveVideoFrameNameTemplate("Frame%06u.ppm"),saveVideoNextFrameIndex(0),
	 paused(false),
	 mainMenu(0)
	{
	/* Parse an initial video format request from the command line: */
	std::pair<Video::VideoDataFormat,int> formatRequest=Video::ViewerComponent::parseVideoFormat(argc,argv);
	
	/* Parse the remaining command line: */
	const char* videoDeviceName=0;
	unsigned int videoDeviceNameIndex=0;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			/* Parse a command line option: */
			if(strcasecmp(argv[i]+1,"saveName")==0||strcasecmp(argv[i]+1,"SN")==0)
				{
				++i;
				if(i<argc)
					saveVideoFrameNameTemplate=argv[i];
				else
					std::cerr<<"VideoViewer: Ignoring dangling -saveName option"<<std::endl;
				}
			else
				std::cerr<<"VideoViewer: Ignoring unknown command line option "<<argv[i]<<std::endl;
			}
		else if(videoDeviceName==0)
			{
			/* Treat the argument as the name of a video device: */
			videoDeviceName=argv[i];
			}
		else if(isIndex(argv[i]))
			{
			/* Treat the argument as the index of a video device among devices with the same name: */
			videoDeviceNameIndex=(unsigned int)(atoi(argv[i]));
			}
		else
			std::cerr<<"VideoViewer: Ignoring extra device name argument "<<argv[i]<<std::endl;
		}
	
	/* Create a viewer component for the selected video device: */
	if(videoDeviceName!=0)
		viewer=new Video::ViewerComponent(videoDeviceName,videoDeviceNameIndex,formatRequest.first,formatRequest.second,Vrui::getWidgetManager());
	else
		viewer=new Video::ViewerComponent(0,formatRequest.first,formatRequest.second,Vrui::getWidgetManager());
	
	/* Install callbacks: */
	viewer->setVideoFrameCallback(Misc::createFunctionCall(this,&VideoViewer::videoFrameCallback));
	viewer->setVideoFormatChangedCallback(Misc::createFunctionCall(this,&VideoViewer::videoFormatChangedCallback));
	viewer->setVideoFormatSizeChangedCallback(Misc::createFunctionCall(this,&VideoViewer::videoFormatSizeChangedCallback));
	
	/* Create and install the main menu: */
	mainMenu=createMainMenu();
	Vrui::setMainMenu(mainMenu);
	
	/* Create an event tool to start/stop saving video frames: */
	addEventTool("Pause Video",0,0);
	addEventTool("Save Video Frames",0,1);
	}

VideoViewer::~VideoViewer(void)
	{
	/* Delete the viewer component: */
	delete viewer;
	
	/* Delete UI components: */
	delete mainMenu;
	}

void VideoViewer::frame(void)
	{
	if(!paused)
		{
		/* Call the viewer's frame method: */
		viewer->frame();
		}
	}

void VideoViewer::display(GLContextData& contextData) const
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_TEXTURE_BIT);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	
	/* Bind the viewer component's video texture: */
	Video::ViewerComponent::DataItem* dataItem=viewer->getDataItem(contextData);
	dataItem->bindVideoTexture();
	
	/* Draw the video display rectangle: */
	const unsigned int* frameSize=dataItem->getSize();
	const GLfloat* texMin=dataItem->getTexMin();
	const GLfloat* texMax=dataItem->getTexMax();
	
	glBegin(GL_QUADS);
	glTexCoord2f(texMin[0],texMin[1]);
	glVertex2i(0,0);
	glTexCoord2f(texMax[0],texMin[1]);
	glVertex2i(frameSize[0],0);
	glTexCoord2f(texMax[0],texMax[1]);
	glVertex2i(frameSize[0],frameSize[1]);
	glTexCoord2f(texMin[0],texMax[1]);
	glVertex2i(0,frameSize[1]);
	glEnd();
	
	/* Protect the texture object: */
	glBindTexture(GL_TEXTURE_2D,0);
	
	/* Draw the video's backside: */
	glDisable(GL_TEXTURE_2D);
	glMaterial(GLMaterialEnums::FRONT,GLMaterial(GLMaterial::Color(0.7f,0.7f,0.7f)));
	
	glBegin(GL_QUADS);
	glNormal3f(0.0f,0.0f,-1.0f);
	glVertex2i(0,0);
	glVertex2i(0,frameSize[1]);
	glVertex2i(frameSize[0],frameSize[1]);
	glVertex2i(frameSize[0],0);
	glEnd();
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

void VideoViewer::resetNavigation(void)
	{
	/* Bail out if the video format is invalid: */
	const unsigned int* frameSize=viewer->getVideoFormat().size;
	if(frameSize[0]==0||frameSize[1]==0)
		return;
	
	/* Calculate the center point and diagonal size of the video frame: */
	Vrui::Point center=Vrui::Point::origin;
	Vrui::Scalar size=Vrui::Scalar(0);
	for(int i=0;i<2;++i)
		{
		/* Calculate the center point and width/height of the video frame: */
		Vrui::Scalar s=Math::div2(Vrui::Scalar(frameSize[i]));
		center[i]=s;
		size+=Math::sqr(s);
		}
	center[2]=Vrui::Scalar(0.01);
	
	/* Center and size the video frame, and rotate it so that Y points up: */
	Vrui::setNavigationTransformation(center,Math::sqrt(size),Vrui::Vector(0,1,0));
	}

void VideoViewer::eventCallback(Vrui::Application::EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	switch(eventId)
		{
		case 0:
			/* Toggle pause flag if button was pressed: */
			if(cbData->newButtonState)
				paused=!paused;
			break;
		
		case 1:
			/* Save video frames while the tool button is pressed: */
			saveVideoFrames=cbData->newButtonState;
			break;
		}
	}

/* Create and execute an application object: */
VRUI_APPLICATION_RUN(VideoViewer)
