/***********************************************************************
ViewerComponent - An application component to stream video from a camera
to an OpenGL texture for rendering, including user interfaces to select
cameras and video modes and control camera settings.
Copyright (c) 2018 Oliver Kreylos

This file is part of the Basic Video Library (Video).

The Basic Video Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The Basic Video Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Basic Video Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef VIDEO_VIEWERCOMPONENT_INCLUDED
#define VIDEO_VIEWERCOMPONENT_INCLUDED

#include <utility>
#include <vector>
#include <Threads/Spinlock.h>
#include <Threads/TripleBuffer.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLObject.h>
#include <Images/BaseImage.h>
#include <GLMotif/DropdownBox.h>
#include <Video/VideoDataFormat.h>
#include <Video/VideoDevice.h>
#include <Video/ImageExtractor.h>

/* Forward declarations: */
namespace Misc {
template <class ParameterParam>
class FunctionCall;
}
namespace GLMotif {
class Widget;
class WidgetManager;
class PopupWindow;
}

namespace Video {

class ViewerComponent:public GLObject
	{
	/* Embedded classes: */
	public:
	struct DataItem:public GLObject::DataItem // Structure to store per-context OpenGL state
		{
		friend class ViewerComponent;
		
		/* Elements: */
		private:
		const ViewerComponent* component; // Pointer to the component to which this state element belongs
		GLuint videoTextureId; // ID of image texture object
		unsigned int size[2]; // Size of the video frame in the locked video frame buffer slot
		bool haveNpotdt; // Flag whether OpenGL supports non-power-of-two dimension textures
		GLfloat texMin[2],texMax[2]; // Texture coordinate rectangle to render the image texture (to account for power-of-two only textures)
		unsigned int videoTextureVersion; // Version number of the video frame in the video texture
		
		/* Constructors and destructors: */
		DataItem(const ViewerComponent* sComponent);
		virtual ~DataItem(void);
		
		/* Methods: */
		public:
		void bindVideoTexture(void); // Binds the video frame texture object to the active texture unit
		const unsigned int* getSize(void) const // Returns the size of the video frame
			{
			return size;
			}
		const GLfloat* getTexMin(void) const // Returns the texture coordinate for the video frame's lower-left corner
			{
			return texMin;
			}
		const GLfloat* getTexMax(void) const // Returns the texture coordinate for the video frame's upper-right corner
			{
			return texMax;
			}
		};
	
	typedef Misc::FunctionCall<const Images::BaseImage&> VideoFrameCallback; // Type for callback functions called when a new video frame arrived
	typedef Misc::FunctionCall<const VideoDataFormat&> VideoFormatChangedCallback; // Type of callback function called when the video format changes
	
	/* Elements: */
	private:
	
	/* Video device streaming state: */
	std::vector<Video::VideoDevice::DeviceIdPtr> videoDeviceList; // The list of all video devices currently connected to the host
	unsigned int videoDeviceIndex; // The index of the currently used video device in the device list
	Video::VideoDevice* videoDevice; // Pointer to the video recording device
	std::vector<Video::VideoDataFormat> videoFormats; // List of the video device's advertised video formats
	Video::VideoDataFormat videoFormat; // Configured video format of the video device
	Video::ImageExtractor* videoExtractor; // Helper object to convert video frames to RGB
	bool storeVideoFrames; // Flag to store video frames into the incoming triple buffer from the frame callback; if disabled, videoFrameCallback callees are supposed to do it
	Images::BaseImage inputVideoFrame; // Image used to store incoming video frames it automatic storage is disabled
	Threads::TripleBuffer<Images::BaseImage> videoFrames; // Triple buffer to pass video frames from the video callback to the main loop
	unsigned int videoFrameVersion; // Version number of the most recent video frame in the triple buffer
	Threads::Spinlock videoFrameCallbackMutex; // Mutex serializing access to the video frame callback so that it can be changed during streaming
	VideoFrameCallback* videoFrameCallback; // Callback to call when a new video frame arrives
	VideoFormatChangedCallback* videoFormatChangedCallback; // Callback called when the streamed video format changes
	VideoFormatChangedCallback* videoFormatSizeChangedCallback; // Callback called when the streamed video format's frame size changes
	
	/* User interface state: */
	GLMotif::WidgetManager* widgetManager; // Manager for UI widgets
	GLMotif::PopupWindow* videoDevicesDialog; // The video device and video format selection dialog
	GLMotif::Widget* videoControlPanel; // The video device's control panel
	
	/* Private methods: */
	void frameCallback(const Video::FrameBuffer* frameBuffer); // Callback receiving incoming video frames
	void videoDevicesValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData);
	void frameSizesValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData);
	void frameRatesValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData);
	void pixelFormatsValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData);
	GLMotif::PopupWindow* createVideoDevicesDialog(void);
	void updateVideoDevicesDialog(void);
	void updateVideoDevicesDialog(const Video::VideoDataFormat& videoFormat);
	void openVideoDevice(unsigned int newVideoDeviceIndex,const Video::VideoDataFormat& initialFormat,int formatComponentMask); // Opens the video device of the given index from the video devices list and updates dependent application state; uses components from initial video format depending on component bit mask (0x1=frame size, 0x2=frame rate,0x4=pixel format)
	void startStreaming(void); // Starts streaming video from the open video device using its current video format
	void stopStreaming(void); // Stops streaming video from the open video device
	void changeVideoFormat(const VideoDataFormat& newVideoFormat); // Changes the streaming video format on the open video device
	void closeVideoDevice(void); // Closes the open video device
	
	/* Constructors and destructors: */
	public:
	ViewerComponent(unsigned int sVideoDeviceIndex,const Video::VideoDataFormat& initialFormat,int initialFormatComponentMask,GLMotif::WidgetManager* sWidgetManager); // Creates a video viewer component for the video device of the given index and optionally selects a video format
	ViewerComponent(const char* videoDeviceName,unsigned int videoDeviceNameIndex,const Video::VideoDataFormat& initialFormat,int initialFormatComponentMask,GLMotif::WidgetManager* sWidgetManager); // Creates a video viewer component for the video device of the given name and optionally selects a video format
	virtual ~ViewerComponent(void); // Destroys the viewer component
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New methods: */
	static std::pair<VideoDataFormat,int> parseVideoFormat(int& argc,char**& argv); // Parses a video data format and format component mask from the given command line; removes format-related command line arguments
	const VideoDataFormat& getVideoFormat(void) const // Returns the currently streaming video format
		{
		return videoFormat;
		}
	GLMotif::Widget* getVideoDevicesDialog(void); // Returns a pointer to the dialog selecting video devices and video formats
	GLMotif::Widget* getVideoControlPanel(void); // Returns a pointer to the dialog controlling the currently open video device
	void setVideoFrameCallback(VideoFrameCallback* newVideoFrameCallback,bool newStoreVideoFrames =true); // Sets the function to be called when a new video frame arrives; adopts function object; disables automatic storing of video frames in the input buffer if flag is false
	void setVideoFormatChangedCallback(VideoFormatChangedCallback* newVideoFormatChangedCallback); // Sets the function to be called when the streamed video format changes; adopts function object
	void setVideoFormatSizeChangedCallback(VideoFormatChangedCallback* newVideoFormatChangedCallback); // Sets the function to be called when the streamed video format's frame size changes; adopts function object
	bool getStoreVideoFrames(void) const // Returns true if the frame callback inserts incoming video frames into the input triple buffer
		{
		return storeVideoFrames;
		}
	void storeVideoFrame(const Images::BaseImage& frame); // Inserts a new video frame into the input triple buffer from a background thread
	void frame(void); // Synchronizes between background threads and foreground thread
	DataItem* getDataItem(GLContextData& contextData) const // Returns the viewer component's OpenGL state object for rendering operations
		{
		/* Retrieve and return the context data item: */
		return contextData.retrieveDataItem<DataItem>(this);
		}
	};

}

#endif
