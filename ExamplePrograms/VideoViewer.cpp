/***********************************************************************
VideoViewer - A simple viewer for live video from a video source
connected to the local computer.
Copyright (c) 2013-2016 Oliver Kreylos

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
#include <Misc/ThrowStdErr.h>
#include <Misc/MessageLogger.h>
#include <Misc/FunctionCalls.h>
#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>
#include <Misc/Timer.h>
#include <Threads/TripleBuffer.h>
#include <Math/Math.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <GL/gl.h>
#include <GL/GLMaterial.h>
#include <GL/GLObject.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBTextureNonPowerOfTwo.h>
#include <Images/RGBImage.h>
#include <Images/WriteImageFile.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/DropdownBox.h>
#include <Video/VideoDataFormat.h>
#include <Video/VideoDevice.h>
#include <Video/ImageExtractor.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>

class VideoViewer:public Vrui::Application,public GLObject
	{
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint videoTextureId; // ID of image texture object
		bool haveNpotdt; // Flag whether OpenGL supports non-power-of-two dimension textures
		GLfloat texMin[2],texMax[2]; // Texture coordinate rectangle to render the image texture (to account for power-of-two only textures)
		unsigned int videoTextureVersion; // Version number of the video frame in the video texture
		unsigned int videoFormatVersion; // Version number of the format of the video texture
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	std::vector<Video::VideoDevice::DeviceIdPtr> videoDeviceList; // The list of all video devices currently connected to the host
	unsigned int videoDeviceIndex; // The index of the currently used video device in the device list
	Video::VideoDevice* videoDevice; // Pointer to the video recording device
	std::vector<Video::VideoDataFormat> videoFormats; // List of the video device's advertised video formats
	Video::VideoDataFormat videoFormat; // Configured video format of the video device
	Video::ImageExtractor* videoExtractor; // Helper object to convert video frames to RGB
	volatile bool saveVideoFrames; // Flag to save video frames to disk as they arrive
	Misc::Timer saveVideoTimer; // A free-running timer to time-stamp saved video frames
	std::string saveVideoFrameNameTemplate; // Printf-style template to save video frames
	unsigned int saveVideoNextFrameIndex; // Index for the next video frame to be saved
	Threads::TripleBuffer<Images::RGBImage> videoFrames; // Triple buffer to pass video frames from the video callback to the main loop
	unsigned int videoFrameVersion; // Version number of the most recent video frame in the triple buffer
	unsigned int videoFormatVersion; // Version number of the video format of video frames in the triple buffer
	GLMotif::PopupWindow* videoDevicesDialog; // The video device and video format selection dialog
	GLMotif::Widget* videoControlPanel; // The video device's control panel
	GLMotif::PopupMenu* mainMenu; // The program's main menu
	
	/* Private methods: */
	void videoFrameCallback(const Video::FrameBuffer* frameBuffer); // Callback receiving incoming video frames
	void openVideoDevice(unsigned int newVideoDeviceIndex); // Opens another video device
	void showVideoDevicesDialogCallback(Misc::CallbackData* cbData); // Method to pop up the video device selection dialog
	void showControlPanelCallback(Misc::CallbackData* cbData); // Method to pop up the video device's control panel
	GLMotif::PopupMenu* createMainMenu(void); // Creates the program's main menu
	void videoDevicesValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData);
	void changeVideoFormat(void);
	void frameSizesValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData);
	void frameRatesValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData);
	void pixelFormatsValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData);
	void updateVideoDevicesDialog(void);
	void updateVideoDevicesDialog(const Video::VideoDataFormat& videoFormat);
	GLMotif::PopupWindow* createVideoDevicesDialog(void);
	
	/* Constructors and destructors: */
	public:
	VideoViewer(int& argc,char**& argv);
	virtual ~VideoViewer(void);
	
	/* Methods from Vrui::Application: */
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	virtual void eventCallback(EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData);
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

/**************************************
Methods of class VideoViewer::DataItem:
**************************************/

VideoViewer::DataItem::DataItem(void)
	:videoTextureId(0),
	 videoTextureVersion(0),videoFormatVersion(0)
	{
	/* Check whether non-power-of-two-dimension textures are supported: */
	haveNpotdt=GLARBTextureNonPowerOfTwo::isSupported();
	if(haveNpotdt)
		GLARBTextureNonPowerOfTwo::initExtension();
	
	glGenTextures(1,&videoTextureId);
	}

VideoViewer::DataItem::~DataItem(void)
	{
	glDeleteTextures(1,&videoTextureId);
	}

/****************************
Methods of class VideoViewer:
****************************/

void VideoViewer::videoFrameCallback(const Video::FrameBuffer* frameBuffer)
	{
	double timeStamp=saveVideoTimer.peekTime();
	
	/* Start a new value in the input triple buffer: */
	Images::RGBImage& image=videoFrames.startNewValue();
	
	/* Extract an RGB image from the provided frame buffer into the new value: */
	videoExtractor->extractRGB(frameBuffer,image.replacePixels());
	
	/* Finish the new value in the triple buffer and wake up the main loop: */
	videoFrames.postNewValue();
	Vrui::requestUpdate();
	
	if(saveVideoFrames)
		{
		/* Create a filename for the new video frame: */
		char videoFrameFileName[1024];
		snprintf(videoFrameFileName,sizeof(videoFrameFileName),saveVideoFrameNameTemplate.c_str(),saveVideoNextFrameIndex);
		
		/* Save the new video frame: */
		Images::writeImageFile(image,videoFrameFileName);
		
		std::cout<<"Saving frame "<<videoFrameFileName<<" at "<<timeStamp*1000.0<<" ms"<<std::endl;
		
		/* Increment the frame counter: */
		++saveVideoNextFrameIndex;
		}
	}

void VideoViewer::showVideoDevicesDialogCallback(Misc::CallbackData* cbData)
	{
	/* Pop up the video devices dialog: */
	Vrui::popupPrimaryWidget(videoDevicesDialog);
	}

void VideoViewer::showControlPanelCallback(Misc::CallbackData* cbData)
	{
	/* Pop up the video control panel: */
	Vrui::popupPrimaryWidget(videoControlPanel);
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
	if(videoControlPanel==0)
		showControlPanelButton->setEnabled(false);
	
	/* Finish building the main menu: */
	mainMenu->manageMenu();
	return mainMenu;
	}

void VideoViewer::videoDevicesValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData)
	{
	if(videoDevice!=0)
		{
		/* Stop streaming: */
		videoDevice->stopStreaming();
		videoDevice->releaseFrameBuffers();
		delete videoExtractor;
		videoExtractor=0;
		
		/* Close the video device: */
		delete videoDevice;
		videoDevice=0;
		
		/* Delete the video device's control panel: */
		delete videoControlPanel;
		videoControlPanel=0;
		}
	
	try
		{
		/* Open the new video device: */
		videoDeviceIndex=cbData->newSelectedItem;
		videoDevice=videoDeviceList[videoDeviceIndex]->createDevice();
		
		/* Query the new video device's supported video formats: */
		videoFormats=videoDevice->getVideoFormatList();
		
		/* Update the video devices dialog with the new device's video formats: */
		updateVideoDevicesDialog();
		
		/* Get the new video device's current video format: */
		videoFormat=videoDevice->getVideoFormat();
		
		/* Update the video devices dialog with the new device's selected video format: */
		updateVideoDevicesDialog(videoFormat);
		
		/* Create an image extractor to convert from the video device's raw image format to RGB: */
		videoExtractor=videoDevice->createImageExtractor();
		
		/* Initialize the incoming video frame triple buffer: */
		for(int i=0;i<3;++i)
			{
			Images::RGBImage img(videoFormat.size[0],videoFormat.size[1]);
			img.clear(Images::RGBImage::Color(128,128,128));
			videoFrames.getBuffer(i)=img;
			}
		++videoFrameVersion;
		++videoFormatVersion;
		
		/* Create the video device's control panel: */
		videoControlPanel=videoDevice->createControlPanel(Vrui::getWidgetManager());
		
		/* Check if the control panel is a pop-up window; if so, add a close button: */
		GLMotif::PopupWindow* vcp=dynamic_cast<GLMotif::PopupWindow*>(videoControlPanel);
		if(vcp!=0)
			{
			/* Add a close button: */
			vcp->setCloseButton(true);
			
			/* Set it so that the popup window will pop itself down, but not destroy itself, when closed: */
			vcp->popDownOnClose();
			}
		
		/* Enable or disable the "Show Video Control Panel" button: */
		mainMenu->findDescendant("_Menu/ShowControlPanelButton")->setEnabled(videoControlPanel!=0);
		
		/* Start capturing video in the new format from the video device: */
		videoDevice->allocateFrameBuffers(5);
		videoDevice->startStreaming(Misc::createFunctionCall(this,&VideoViewer::videoFrameCallback));
		}
	catch(std::runtime_error err)
		{
		/* Something went horribly awry; clean up as much as possible: */
		
		/* Destroy the video device's control panel if it was created: */
		if(videoControlPanel!=0)
			{
			delete videoControlPanel;
			videoControlPanel=0;
			
			/* Disable the "Show Video Control Panel" button: */
			mainMenu->findDescendant("_Menu/ShowControlPanelButton")->setEnabled(false);
			}
		
		/* Close the video device: */
		delete videoExtractor;
		videoExtractor=0;
		delete videoDevice;
		videoDevice=0;
		
		/* Show an error message: */
		Misc::formattedUserError("VideoViewer: Could not open video device %s due to exception %s",videoDeviceList[videoDeviceIndex]->getName().c_str(),err.what());
		}
	}

void VideoViewer::changeVideoFormat(void)
	{
	if(videoDevice==0)
		return;
	
	/* Stop streaming: */
	videoDevice->stopStreaming();
	videoDevice->releaseFrameBuffers();
	delete videoExtractor;
	videoExtractor=0;
	
	/* Set the changed video format: */
	videoDevice->setVideoFormat(videoFormat);
	
	/* Update the video devices dialog with the new video format: */
	updateVideoDevicesDialog(videoFormat);
	
	/* Create an image extractor to convert from the video device's raw image format to RGB: */
	videoExtractor=videoDevice->createImageExtractor();
	
	/* Initialize the incoming video frame triple buffer: */
	for(int i=0;i<3;++i)
		{
		Images::RGBImage img(videoFormat.size[0],videoFormat.size[1]);
		img.clear(Images::RGBImage::Color(128,128,128));
		videoFrames.getBuffer(i)=img;
		}
	++videoFrameVersion;
	++videoFormatVersion;
	
	/* Start capturing video in the new format from the video device: */
	videoDevice->allocateFrameBuffers(5);
	videoDevice->startStreaming(Misc::createFunctionCall(this,&VideoViewer::videoFrameCallback));
	}

namespace {

/**************
Helper classes:
**************/

struct VideoFrameSize // Structure for video frame sizes, duh, to enable hashing and widget association
	{
	/* Elements: */
	public:
	unsigned int size[2]; // Video frame width and height
	
	/* Constructors and destructors: */
	VideoFrameSize(unsigned int sWidth,unsigned int sHeight)
		{
		size[0]=sWidth;
		size[1]=sHeight;
		}
	
	/* Methods: */
	bool operator!=(const VideoFrameSize& other) const
		{
		return size[0]!=other.size[0]||size[1]!=other.size[1];
		}
	static size_t hash(const VideoFrameSize& source,size_t tableSize)
		{
		return (size_t(source.size[0])*31U+size_t(source.size[1])*17U)%tableSize;
		}
	};

struct VideoFrameInterval // Structure for video frame intervals, duh, to enable hashing and widget association
	{
	/* Elements: */
	public:
	unsigned int counter,denominator; // Frame *interval* counter and denominator
	
	/* Constructors and destructors: */
	VideoFrameInterval(unsigned int sCounter,unsigned int sDenominator)
		:counter(sCounter),denominator(sDenominator)
		{
		}
	
	/* Methods: */
	bool operator!=(const VideoFrameInterval& other) const
		{
		return counter!=other.counter||denominator!=other.denominator;
		}
	static size_t hash(const VideoFrameInterval& source,size_t tableSize)
		{
		return (size_t(source.counter)*31U+size_t(source.denominator)*17U)%tableSize;
		}
	};

}

void VideoViewer::frameSizesValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData)
	{
	/* Find the closest video format with the requested frame size: */
	const VideoFrameSize& vfs=cbData->dropdownBox->getManager()->getWidgetAttribute<VideoFrameSize>(cbData->getItemWidget());
	unsigned int bestFormat=~0x0U;
	unsigned int bestDist=~0x0U;
	for(unsigned int i=0;i<videoFormats.size();++i)
		if(videoFormats[i].size[0]==vfs.size[0]&&videoFormats[i].size[1]==vfs.size[1])
			{
			/* Calculate the mismatch in frame rate and pixel format: */
			unsigned int d1=videoFormats[i].frameIntervalCounter*videoFormat.frameIntervalDenominator;
			unsigned int d2=videoFormats[i].frameIntervalDenominator*videoFormat.frameIntervalCounter;
			unsigned int dist=d1>=d2?d1-d2:d2-d1;
			
			if(videoFormats[i].pixelFormat!=videoFormat.pixelFormat)
				++dist;
			
			if(bestDist>dist)
				{
				bestFormat=i;
				bestDist=dist;
				}
			}
	
	/* Update the video format: */
	videoFormat=videoFormats[bestFormat];
	changeVideoFormat();
	}

void VideoViewer::frameRatesValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData)
	{
	/* Find the closest video format with the requested frame interval: */
	const VideoFrameInterval& vfi=cbData->dropdownBox->getManager()->getWidgetAttribute<VideoFrameInterval>(cbData->getItemWidget());
	unsigned int bestFormat=~0x0U;
	unsigned int bestDist=~0x0U;
	for(unsigned int i=0;i<videoFormats.size();++i)
		if(videoFormats[i].frameIntervalCounter*vfi.denominator==videoFormats[i].frameIntervalDenominator*vfi.counter)
			{
			/* Calculate the mismatch in frame size and pixel format: */
			unsigned int d1=videoFormats[i].size[0]>=videoFormat.size[0]?videoFormats[i].size[0]-videoFormat.size[0]:videoFormat.size[0]-videoFormats[i].size[0];
			unsigned int d2=videoFormats[i].size[1]>=videoFormat.size[1]?videoFormats[i].size[1]-videoFormat.size[1]:videoFormat.size[1]-videoFormats[i].size[1];
			unsigned int dist=d1+d2;
			
			if(videoFormats[i].pixelFormat!=videoFormat.pixelFormat)
				++dist;
			
			if(bestDist>dist)
				{
				bestFormat=i;
				bestDist=dist;
				}
			}
	
	/* Update the video format: */
	videoFormat=videoFormats[bestFormat];
	changeVideoFormat();
	}

void VideoViewer::pixelFormatsValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData)
	{
	/* Find the closest video format with the requested pixel format: */
	unsigned int pixelFormat=cbData->dropdownBox->getManager()->getWidgetAttribute<unsigned int>(cbData->getItemWidget());
	unsigned int bestFormat=~0x0U;
	unsigned int bestDist=~0x0U;
	for(unsigned int i=0;i<videoFormats.size();++i)
		if(videoFormats[i].pixelFormat==pixelFormat)
			{
			/* Calculate the mismatch in frame size and frame interval: */
			unsigned int d1=videoFormats[i].size[0]>=videoFormat.size[0]?videoFormats[i].size[0]-videoFormat.size[0]:videoFormat.size[0]-videoFormats[i].size[0];
			unsigned int d2=videoFormats[i].size[1]>=videoFormat.size[1]?videoFormats[i].size[1]-videoFormat.size[1]:videoFormat.size[1]-videoFormats[i].size[1];
			unsigned int dist=d1+d2;
			
			d1=videoFormats[i].frameIntervalCounter*videoFormat.frameIntervalDenominator;
			d2=videoFormats[i].frameIntervalDenominator*videoFormat.frameIntervalCounter;
			dist+=d1>=d2?d1-d2:d2-d1;
			
			if(bestDist>dist)
				{
				bestFormat=i;
				bestDist=dist;
				}
			}
	
	/* Update the video format: */
	videoFormat=videoFormats[bestFormat];
	changeVideoFormat();
	}

void VideoViewer::updateVideoDevicesDialog(void)
	{
	GLMotif::WidgetManager* widgetManager=videoDevicesDialog->getManager();
	
	/* Create a drop-down menu of video frame sizes: */
	GLMotif::DropdownBox* frameSizes=dynamic_cast<GLMotif::DropdownBox*>(videoDevicesDialog->findDescendant("VideoDeviceDialog/FrameSizes"));
	frameSizes->clearItems();
	Misc::HashTable<VideoFrameSize,void,VideoFrameSize> addedFrameSizes(17);
	for(unsigned int i=0;i<videoFormats.size();++i)
		{
		VideoFrameSize vfs(videoFormats[i].size[0],videoFormats[i].size[1]);
		if(!addedFrameSizes.setEntry(vfs))
			{
			/* Add the frame size to the drop-down menu: */
			char frameSizeBuffer[64];
			snprintf(frameSizeBuffer,sizeof(frameSizeBuffer),"%u x %u",vfs.size[0],vfs.size[1]);
			GLMotif::Widget* newItem=frameSizes->addItem(frameSizeBuffer);
			
			/* Associate the video frame size with the new menu entry: */
			widgetManager->setWidgetAttribute(newItem,vfs);
			}
		}
	
	/* Create a drop-down menu of video frame rates: */
	GLMotif::DropdownBox* frameRates=dynamic_cast<GLMotif::DropdownBox*>(videoDevicesDialog->findDescendant("VideoDeviceDialog/FrameRates"));
	frameRates->clearItems();
	Misc::HashTable<VideoFrameInterval,void,VideoFrameInterval> addedFrameIntervals(17);
	for(unsigned int i=0;i<videoFormats.size();++i)
		{
		VideoFrameInterval vfi(videoFormats[i].frameIntervalCounter,videoFormats[i].frameIntervalDenominator);
		if(!addedFrameIntervals.setEntry(vfi))
			{
			/* Add the frame rate to the drop-down menu: */
			char frameRateBuffer[64];
			snprintf(frameRateBuffer,sizeof(frameRateBuffer),"%g",double(videoFormats[i].frameIntervalDenominator)/double(videoFormats[i].frameIntervalCounter));
			GLMotif::Widget* newItem=frameRates->addItem(frameRateBuffer);
			
			/* Associate the video frame rate with the new menu entry: */
			widgetManager->setWidgetAttribute(newItem,vfi);
			}
		}
	
	/* Create a drop-down menu of video pixel videoFormats. */
	GLMotif::DropdownBox* pixelFormats=dynamic_cast<GLMotif::DropdownBox*>(videoDevicesDialog->findDescendant("VideoDeviceDialog/PixelFormats"));
	pixelFormats->clearItems();
	Misc::HashTable<unsigned int,void> addedPixelFormats(17);
	for(unsigned int i=0;i<videoFormats.size();++i)
		{
		if(!addedPixelFormats.setEntry(videoFormats[i].pixelFormat))
			{
			/* Add the frame rate to the drop-down menu: */
			char fourCCBuffer[5];
			GLMotif::Widget* newItem=pixelFormats->addItem(videoFormats[i].getFourCC(fourCCBuffer));
			
			/* Associate the video frame rate with the new menu entry: */
			widgetManager->setWidgetAttribute(newItem,videoFormats[i].pixelFormat);
			}
		}
	}

void VideoViewer::updateVideoDevicesDialog(const Video::VideoDataFormat& videoFormat)
	{
	GLMotif::WidgetManager* widgetManager=videoDevicesDialog->getManager();
	
	/* Select the video format's frame size: */
	GLMotif::DropdownBox* frameSizes=dynamic_cast<GLMotif::DropdownBox*>(videoDevicesDialog->findDescendant("VideoDeviceDialog/FrameSizes"));
	VideoFrameSize currentVfs(videoFormat.size[0],videoFormat.size[1]);
	for(int i=0;i<frameSizes->getNumItems();++i)
		if(!(widgetManager->getWidgetAttribute<VideoFrameSize>(frameSizes->getItemWidget(i))!=currentVfs)) // Already implemented operator!=, so...
			{
			frameSizes->setSelectedItem(i);
			break;
			}
	
	/* Select the video format's frame rate: */
	GLMotif::DropdownBox* frameRates=dynamic_cast<GLMotif::DropdownBox*>(videoDevicesDialog->findDescendant("VideoDeviceDialog/FrameRates"));
	VideoFrameInterval currentVfi(videoFormat.frameIntervalCounter,videoFormat.frameIntervalDenominator);
	for(int i=0;i<frameRates->getNumItems();++i)
		if(!(widgetManager->getWidgetAttribute<VideoFrameInterval>(frameRates->getItemWidget(i))!=currentVfi)) // Already implemented operator!=, so...
			{
			frameRates->setSelectedItem(i);
			break;
			}
	
	/* Select the video format's pixel format: */
	GLMotif::DropdownBox* pixelFormats=dynamic_cast<GLMotif::DropdownBox*>(videoDevicesDialog->findDescendant("VideoDeviceDialog/PixelFormats"));
	for(int i=0;i<pixelFormats->getNumItems();++i)
		if(widgetManager->getWidgetAttribute<unsigned int>(pixelFormats->getItemWidget(i))==videoFormat.pixelFormat)
			{
			pixelFormats->setSelectedItem(i);
			break;
			}
	}

GLMotif::PopupWindow* VideoViewer::createVideoDevicesDialog(void)
	{
	/* Create a popup shell to hold the video device control dialog: */
	GLMotif::PopupWindow* videoDeviceDialogPopup=new GLMotif::PopupWindow("VideoDeviceDialogPopup",Vrui::getWidgetManager(),"Video Device Selection");
	// videoDeviceDialogPopup->setResizableFlags(true,false);
	videoDeviceDialogPopup->setCloseButton(true);
	videoDeviceDialogPopup->popDownOnClose();
	
	GLMotif::RowColumn* videoDeviceDialog=new GLMotif::RowColumn("VideoDeviceDialog",videoDeviceDialogPopup,false);
	videoDeviceDialog->setOrientation(GLMotif::RowColumn::VERTICAL);
	videoDeviceDialog->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	videoDeviceDialog->setNumMinorWidgets(2);
	
	new GLMotif::Label("VideoDeviceLabel",videoDeviceDialog,"Video Device");
	
	/* Create a drop-down menu containing all connected video devices: */
	GLMotif::DropdownBox* videoDevices=new GLMotif::DropdownBox("VideoDevices",videoDeviceDialog,false);
	
	for(unsigned int i=0;i<videoDeviceList.size();++i)
		{
		/* Add the video device's name to the drop-down menu: */
		videoDevices->addItem(videoDeviceList[i]->getName().c_str());
		
		/* Associate the video device's device ID with the just-added drop-down menu entry: */
		Vrui::getWidgetManager()->setWidgetAttribute(videoDevices->getItemWidget(i),videoDeviceList[i]);
		}
	
	videoDevices->setSelectedItem(videoDeviceIndex);
	videoDevices->getValueChangedCallbacks().add(this,&VideoViewer::videoDevicesValueChangedCallback);
	videoDevices->manageChild();
	
	new GLMotif::Label("FrameSizeLabel",videoDeviceDialog,"Frame Size");
	
	/* Create a drop-down menu containing all supported frame sizes on the currently-selected video device, which will be populated later: */
	GLMotif::DropdownBox* frameSizes=new GLMotif::DropdownBox("FrameSizes",videoDeviceDialog,true);
	frameSizes->getValueChangedCallbacks().add(this,&VideoViewer::frameSizesValueChangedCallback);
	
	new GLMotif::Label("FrameRateLabel",videoDeviceDialog,"Frame Rate");
	
	/* Create a drop-down menu containing all supported frame rates on the currently-selected video device, which will be populated later: */
	GLMotif::DropdownBox* frameRates=new GLMotif::DropdownBox("FrameRates",videoDeviceDialog,true);
	frameRates->getValueChangedCallbacks().add(this,&VideoViewer::frameRatesValueChangedCallback);
	
	new GLMotif::Label("PixelFormatLabel",videoDeviceDialog,"Pixel Format");
	
	/* Create a drop-down menu containing all supported pixel formats on the currently-selected video device, which will be populated later: */
	GLMotif::DropdownBox* pixelFormats=new GLMotif::DropdownBox("PixelFormats",videoDeviceDialog,true);
	pixelFormats->getValueChangedCallbacks().add(this,&VideoViewer::pixelFormatsValueChangedCallback);
	
	videoDeviceDialog->manageChild();
	
	return videoDeviceDialogPopup;
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
	 videoDevice(0),videoExtractor(0),
	 saveVideoFrames(false),saveVideoFrameNameTemplate("Frame%06u.ppm"),saveVideoNextFrameIndex(0),
	 videoFrameVersion(1),videoFormatVersion(1),
	 videoDevicesDialog(0),videoControlPanel(0),mainMenu(0)
	{
	/* Query the list of all connected video devices: */
	videoDeviceList=Video::VideoDevice::getVideoDevices();
	if(videoDeviceList.empty())
		throw std::runtime_error("VideoViewer: No video devices connected to host");
	
	/* Parse the command line: */
	const char* videoDeviceName=0;
	unsigned int videoDeviceNameIndex=0;
	bool listFormats=false;
	bool requestSize=false;
	int videoSize[2];
	bool requestRate=false;
	int videoRate;
	const char* pixelFormat=0;
	bool requestHexFormat=false;
	unsigned int hexFormat=0x0;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			/* Parse a command line option: */
			if(strcasecmp(argv[i]+1,"listFormats")==0||strcasecmp(argv[i]+1,"LF")==0)
				listFormats=true;
			else if(strcasecmp(argv[i]+1,"size")==0||strcasecmp(argv[i]+1,"S")==0)
				{
				/* Parse the desired video frame size: */
				i+=2;
				if(i<argc)
					{
					for(int j=0;j<2;++j)
						videoSize[j]=atoi(argv[i-1+j]);
					requestSize=true;
					}
				else
					std::cerr<<"Ignoring dangling -size option"<<std::endl;
				}
			else if(strcasecmp(argv[i]+1,"rate")==0||strcasecmp(argv[i]+1,"R")==0)
				{
				/* Parse the desired video frame rate: */
				++i;
				if(i<argc)
					{
					videoRate=atoi(argv[i]);
					requestRate=true;
					}
				else
					std::cerr<<"Ignoring dangling -rate option"<<std::endl;
				}
			else if(strcasecmp(argv[i]+1,"format")==0||strcasecmp(argv[i]+1,"F")==0)
				{
				/* Parse the desired pixel format: */
				++i;
				if(i<argc)
					pixelFormat=argv[i];
				else
					std::cerr<<"Ignoring dangling -format option"<<std::endl;
				}
			else if(strcasecmp(argv[i]+1,"hexFormat")==0||strcasecmp(argv[i]+1,"HF")==0)
				{
				/* Read a binary pixel format in hexadecimal: */
				++i;
				if(i<argc)
					{
					long hf=strtol(argv[i],0,16);
					if(hf>=0&&hf<1L<<32)
						{
						hexFormat=(unsigned int)hf;
						requestHexFormat=true;
						}
					else
						std::cerr<<"Ignoring invalid hexadecimal pixel format "<<argv[i]<<std::endl;
					}
				else
					std::cerr<<"Ignoring dangling -hexFormat option"<<std::endl;
				}
			else if(strcasecmp(argv[i]+1,"saveName")==0||strcasecmp(argv[i]+1,"SN")==0)
				{
				++i;
				if(i<argc)
					saveVideoFrameNameTemplate=argv[i];
				else
					std::cerr<<"Ignoring dangling -saveName option"<<std::endl;
				}
			else
				std::cerr<<"Ignoring unknown command line option "<<argv[i]<<std::endl;
			}
		else if(strcasecmp(argv[i],"list")==0)
			{
			/* Print a list of all connected video devices: */
			std::cout<<"Connected video devices:"<<std::endl;
			for(std::vector<Video::VideoDevice::DeviceIdPtr>::iterator vdlIt=videoDeviceList.begin();vdlIt!=videoDeviceList.end();++vdlIt)
				std::cout<<(*vdlIt)->getName()<<std::endl;
			
			/* Exit from the program: */
			Vrui::shutdown();
			return;
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
			std::cerr<<"Ignoring extra device name argument "<<argv[i]<<std::endl;
		}
	
	/* Select a video device to open: */
	if(videoDeviceName==0)
		{
		/* Select the first video device: */
		videoDeviceIndex=0;
		}
	else
		{
		/* Find a video device whose name matches the given name and index: */
		videoDeviceIndex=videoDeviceList.size();
		for(unsigned int i=0;i<videoDeviceIndex;++i)
			if(strcasecmp(videoDeviceList[i]->getName().c_str(),videoDeviceName)==0)
				{
				if(videoDeviceNameIndex==0U)
					{
					/* Select the matching video device and bail out: */
					videoDeviceIndex=i;
					break;
					}
				
				/* Try the next video device of the same name: */
				--videoDeviceNameIndex;
				}
		}
	if(videoDeviceIndex>=videoDeviceList.size())
		throw std::runtime_error("VideoViewer: Could not find requested video device");
	
	/* Create the video devices dialog: */
	videoDevicesDialog=createVideoDevicesDialog();
	
	/* Open the video device: */
	videoDevice=videoDeviceList[videoDeviceIndex]->createDevice();
	
	/* Query the opened video device's supported video formats: */
	videoFormats=videoDevice->getVideoFormatList();
	
	/* Update the video devices dialog with the opened device's video formats: */
	updateVideoDevicesDialog();
	
	if(listFormats)
		{
		/* Retrieve the video device's list of video formats: */
		for(std::vector<Video::VideoDataFormat>::iterator vfIt=videoFormats.begin();vfIt!=videoFormats.end();++vfIt)
			{
			char fourCCBuffer[5];
			std::cout<<"Pixel format: \""<<vfIt->getFourCC(fourCCBuffer)<<"\" (0x"<<std::hex<<std::setfill('0')<<std::setw(8)<<vfIt->pixelFormat;
			std::cout<<std::dec<<std::setfill(' ')<<"), size "<<vfIt->size[0]<<"x"<<vfIt->size[1]<<", ";
			std::cout<<double(vfIt->frameIntervalDenominator)/double(vfIt->frameIntervalCounter)<<" Hz"<<std::endl;
			}
		}
	
	/* Get and modify the video device's current video format: */
	videoFormat=videoDevice->getVideoFormat();
	if(requestSize)
		for(int i=0;i<2;++i)
			videoFormat.size[i]=(unsigned int)videoSize[i];
	if(requestRate)
		{
		/* Convert from frame rate in Hz to frame interval as a rational number: */
		videoFormat.frameIntervalCounter=1;
		videoFormat.frameIntervalDenominator=videoRate;
		}
	if(pixelFormat!=0)
		videoFormat.setPixelFormat(pixelFormat);
	if(requestHexFormat)
		videoFormat.pixelFormat=hexFormat;
	videoDevice->setVideoFormat(videoFormat);
	
	/* Update the video devices dialog with the opened device's selected video format: */
	updateVideoDevicesDialog(videoFormat);
	
	/* Print the actual video format after adaptation: */
	std::cout<<"Selected video format on video device "<<(videoDeviceName!=0?videoDeviceName:"Default")<<":"<<std::endl;
	std::cout<<"Frame size "<<videoFormat.size[0]<<"x"<<videoFormat.size[1]<<" at "<<double(videoFormat.frameIntervalDenominator)/double(videoFormat.frameIntervalCounter)<<" Hz"<<std::endl;
	char videoPixelFormatBuffer[5];
	std::cout<<"Pixel format "<<videoFormat.getFourCC(videoPixelFormatBuffer)<<", (0x"<<std::hex<<std::setfill('0')<<std::setw(8)<<videoFormat.pixelFormat<<std::dec<<std::setfill(' ')<<")"<<std::endl;
	std::cout<<"Frame line size "<<videoFormat.lineSize<<", total size "<<videoFormat.frameSize<<" bytes"<<std::endl;
	
	/* Create an image extractor to convert from the video device's raw image format to RGB: */
	videoExtractor=videoDevice->createImageExtractor();
	
	/* Initialize the incoming video frame triple buffer: */
	for(int i=0;i<3;++i)
		{
		Images::RGBImage img(videoFormat.size[0],videoFormat.size[1]);
		img.clear(Images::RGBImage::Color(128,128,128));
		videoFrames.getBuffer(i)=img;
		}
	
	/* Create the video device's control panel: */
	videoControlPanel=videoDevice->createControlPanel(Vrui::getWidgetManager());
	
	/* Check if the control panel is a pop-up window; if so, add a close button: */
	GLMotif::PopupWindow* vcp=dynamic_cast<GLMotif::PopupWindow*>(videoControlPanel);
	if(vcp!=0)
		{
		/* Add a close button: */
		vcp->setCloseButton(true);
		
		/* Set it so that the popup window will pop itself down, but not destroy itself, when closed: */
		vcp->popDownOnClose();
		}
	
	/* Create and install the main menu: */
	mainMenu=createMainMenu();
	Vrui::setMainMenu(mainMenu);
	
	/* Create an event tool to start/stop saving video frames: */
	addEventTool("Save Video Frames",0,0);
	
	/* Start capturing video from the video device: */
	videoDevice->allocateFrameBuffers(5);
	videoDevice->startStreaming(Misc::createFunctionCall(this,&VideoViewer::videoFrameCallback));
	}

VideoViewer::~VideoViewer(void)
	{
	if(videoDevice!=0)
		{
		/* Stop streaming: */
		videoDevice->stopStreaming();
		videoDevice->releaseFrameBuffers();
		
		/* Close the video device: */
		delete videoExtractor;
		delete videoDevice;
		
		delete videoControlPanel;
		}
	
	delete videoDevicesDialog;
	delete mainMenu;
	}

void VideoViewer::frame(void)
	{
	/* Lock the most recent video frame in the input triple buffer: */
	if(videoFrames.lockNewValue())
		{
		/* Bump up the video frame's version number to invalidate the cached texture: */
		++videoFrameVersion;
		}
	}

void VideoViewer::display(GLContextData& contextData) const
	{
	/* Get the context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_TEXTURE_BIT);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	
	/* Bind the texture object: */
	glBindTexture(GL_TEXTURE_2D,dataItem->videoTextureId);
	
	/* Check if the cached texture is outdated: */
	if(dataItem->videoTextureVersion!=videoFrameVersion)
		{
		/* Check if the video format changed as well: */
		if(dataItem->videoFormatVersion!=videoFormatVersion)
			{
			/* Calculate the texture coordinate rectangle: */
			unsigned int texSize[2];
			if(dataItem->haveNpotdt)
				{
				for(int i=0;i<2;++i)
					texSize[i]=videoFormat.size[i];
				}
			else
				{
				/* Find the next larger power-of-two texture size: */
				for(int i=0;i<2;++i)
					for(texSize[i]=1U;texSize[i]<videoFormat.size[i];texSize[i]<<=1)
						;
				}
			
			/* Calculate texture coordinates to map the (padded) texture onto the geometry: */
			for(int i=0;i<2;++i)
				{
				dataItem->texMin[i]=0.0f;
				dataItem->texMax[i]=GLfloat(videoFormat.size[i])/GLfloat(texSize[i]);
				}
			
			dataItem->videoFormatVersion!=videoFormatVersion;
			}
		
		/* Upload the most recent texture image: */
		videoFrames.getLockedValue().glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,!dataItem->haveNpotdt);
		dataItem->videoTextureVersion=videoFrameVersion;
		}
	
	glBegin(GL_QUADS);
	glTexCoord2f(dataItem->texMin[0],dataItem->texMin[1]);
	glVertex2i(0,0);
	glTexCoord2f(dataItem->texMax[0],dataItem->texMin[1]);
	glVertex2i(videoFormat.size[0],0);
	glTexCoord2f(dataItem->texMax[0],dataItem->texMax[1]);
	glVertex2i(videoFormat.size[0],videoFormat.size[1]);
	glTexCoord2f(dataItem->texMin[0],dataItem->texMax[1]);
	glVertex2i(0,videoFormat.size[1]);
	glEnd();
	
	/* Protect the texture object: */
	glBindTexture(GL_TEXTURE_2D,0);
	
	/* Draw the video's backside: */
	glDisable(GL_TEXTURE_2D);
	glMaterial(GLMaterialEnums::FRONT,GLMaterial(GLMaterial::Color(0.7f,0.7f,0.7f)));
	
	glBegin(GL_QUADS);
	glNormal3f(0.0f,0.0f,-1.0f);
	glVertex2i(0,0);
	glVertex2i(0,videoFormat.size[1]);
	glVertex2i(videoFormat.size[0],videoFormat.size[1]);
	glVertex2i(videoFormat.size[0],0);
	glEnd();
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

void VideoViewer::resetNavigation(void)
	{
	/* Calculate the center point and diagonal size of the video frame: */
	Vrui::Point center=Vrui::Point::origin;
	Vrui::Scalar size=Vrui::Scalar(0);
	for(int i=0;i<2;++i)
		{
		/* Calculate the center point and width/height of the video frame: */
		Vrui::Scalar s=Math::div2(Vrui::Scalar(videoFormat.size[i]));
		center[i]=s;
		size+=Math::sqr(s);
		}
	center[2]=Vrui::Scalar(0.01);
	
	/* Center and size the video frame, and rotate it so that Y points up: */
	Vrui::setNavigationTransformation(center,Math::sqrt(size),Vrui::Vector(0,1,0));
	}

void VideoViewer::eventCallback(Vrui::Application::EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	if(eventId==0)
		{
		/* Save video frames while the tool button is pressed: */
		saveVideoFrames=cbData->newButtonState;
		}
	}

void VideoViewer::initContext(GLContextData& contextData) const
	{
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Bind the texture object: */
	glBindTexture(GL_TEXTURE_2D,dataItem->videoTextureId);
	
	/* Initialize basic texture settings: */
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	
	/* Protect the texture object: */
	glBindTexture(GL_TEXTURE_2D,0);
	}

/* Create and execute an application object: */
VRUI_APPLICATION_RUN(VideoViewer)
