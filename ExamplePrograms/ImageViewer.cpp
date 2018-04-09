/***********************************************************************
Small image viewer using Vrui.
Copyright (c) 2011-2018 Oliver Kreylos

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

#include <string.h>
#include <stdexcept>
#include <Misc/MessageLogger.h>
#include <Math/Math.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLMaterial.h>
#include <GL/GLObject.h>
#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <Images/RGBImage.h>
#include <Images/ReadImageFile.h>
#include <Images/TextureSet.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>
#include <Vrui/Tool.h>
#include <Vrui/GenericToolFactory.h>
#include <Vrui/ToolManager.h>
#include <Vrui/DisplayState.h>
#include <Vrui/OpenFile.h>

class ImageViewer:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	class PipetteTool; // Forward declaration
	typedef Vrui::GenericToolFactory<PipetteTool> PipetteToolFactory; // Pipette tool class uses the generic factory class
	
	class PipetteTool:public Vrui::Tool,public Vrui::Application::Tool<ImageViewer> // A tool class to pick color values from an image, derived from application tool class
		{
		friend class Vrui::GenericToolFactory<PipetteTool>;
		
		/* Elements: */
		private:
		static PipetteToolFactory* factory; // Pointer to the factory object for this class
		bool dragging; // Flag whether there is a current dragging operation
		int x0,y0; // Initial pixel position for dragging operations
		int x,y; // Current pixel position during dragging operations
		
		/* Private methods: */
		void setPixelPos(void); // Sets the current pixel position based on current input device selection ray
		
		/* Constructors and destructors: */
		public:
		static void initClass(void); // Initializes the pipette tool factory class
		PipetteTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
		
		/* Methods: */
		virtual const Vrui::ToolFactory* getFactory(void) const;
		virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
		virtual void frame(void);
		virtual void display(GLContextData& contextData) const;
		};
	
	friend class PipetteTool;
	
	/* Elements: */
	Images::TextureSet textures; // Texture set containing the image to be displayed
	
	/* Constructors and destructors: */
	public:
	ImageViewer(int& argc,char**& argv);
	virtual ~ImageViewer(void);
	
	/* Methods from Vrui::Application: */
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	};

/*************************************************
Static elements of class ImageViewer::PipetteTool:
*************************************************/

ImageViewer::PipetteToolFactory* ImageViewer::PipetteTool::factory=0;

/*****************************************
Methods of class ImageViewer::PipetteTool:
*****************************************/

void ImageViewer::PipetteTool::setPixelPos(void)
	{
	/* Get the first button slot's device ray: */
	Vrui::Ray ray=getButtonDeviceRay(0);
	
	/* Transform the ray to navigational space: */
	ray.transform(Vrui::getInverseNavigationTransformation());
	
	/* Intersect the ray with the z=0 plane: */
	if(ray.getOrigin()[2]*ray.getDirection()[2]<Vrui::Scalar(0))
		{
		Vrui::Scalar lambda=-ray.getOrigin()[2]/ray.getDirection()[2];
		Vrui::Point intersection=ray(lambda);
		x=int(Math::floor(intersection[0]));
		y=int(Math::floor(intersection[1]));
		}
	else
		x=y=0;
	}

void ImageViewer::PipetteTool::initClass(void)
	{
	/* Create a factory object for the pipette tool class: */
	factory=new PipetteToolFactory("PipetteTool","Pick Color Value",0,*Vrui::getToolManager());
	
	/* Set the pipette tool class' input layout: */
	factory->setNumButtons(1);
	factory->setButtonFunction(0,"Pick Color");
	
	/* Register the pipette tool class with the Vrui tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

ImageViewer::PipetteTool::PipetteTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::Tool(factory,inputAssignment),
	 dragging(false)
	{
	}

const Vrui::ToolFactory* ImageViewer::PipetteTool::getFactory(void) const
	{
	return factory;
	}

namespace {

template <class ScalarParam>
class RectangleAverager
	{
	/* Methods: */
	public:
	static GLColor<GLfloat,4> averageRect(const Images::BaseImage& image,int xmin,int xmax,int ymin,int ymax)
		{
		const ScalarParam* imgPtr=static_cast<const ScalarParam*>(image.getPixels());
		ptrdiff_t stride=image.getRowStride()/sizeof(ScalarParam);
		
		/* Accumulate the given rectangle: */
		GLColor<GLfloat,4> result(0.0f,0.0f,0.0f,0.0f);
		const ScalarParam* rowPtr=imgPtr+ymin*stride;
		for(int y=ymin;y<=ymax;++y,rowPtr+=stride)
			{
			const ScalarParam* pPtr=rowPtr+xmin*image.getNumChannels();
			for(int x=xmin;x<=xmax;++x)
				for(int channel=0;channel<image.getNumChannels();++channel,++pPtr)
					result[channel]+=GLfloat(*pPtr);
			}
		
		/* Normalize the result color: */
		for(int channel=0;channel<4;++channel)
			result[channel]/=GLfloat((ymax+1-ymin)*(xmax+1-xmin));
		
		/* Swizzle the result color to better represent the image's intent: */
		switch(image.getFormat())
			{
			case GL_LUMINANCE:
				result[2]=result[1]=result[0];
				result[3]=1.0f;
				break;
			
			case GL_LUMINANCE_ALPHA:
				result[3]=result[1];
				result[2]=result[1]=result[0];
				break;
			
			case GL_RGB:
				result[3]=1.0f;
				break;
			}
		
		return result;
		}
	};

}

void ImageViewer::PipetteTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState)
		{
		/* Start a new dragging operation: */
		dragging=true;
		setPixelPos();
		x0=x;
		y0=y;
		}
	else
		{
		/* Stop dragging: */
		dragging=false;
		setPixelPos();
		
		/* Access the displayed image: */
		Images::BaseImage image(application->textures.getTexture(0U).getImage());
		
		/* Calculate the average pixel value inside the selection rectangle: */
		int xmin=Math::max(Math::min(x0,x),0);
		int xmax=Math::min(Math::max(x0,x),int(image.getSize(0))-1);
		int ymin=Math::max(Math::min(y0,y),0);
		int ymax=Math::min(Math::max(y0,y),int(image.getSize(1))-1);
		
		if(xmax>=xmin&&ymax>=ymin)
			{
			GLColor<GLfloat,4> average(0.0f,0.0f,0.0f,0.0f);
			switch(image.getScalarType())
				{
				case GL_UNSIGNED_BYTE:
					average=RectangleAverager<GLubyte>::averageRect(image,xmin,xmax,ymin,ymax);
					break;
				
				case GL_UNSIGNED_SHORT:
					average=RectangleAverager<GLushort>::averageRect(image,xmin,xmax,ymin,ymax);
					break;
				
				case GL_SHORT:
					average=RectangleAverager<GLshort>::averageRect(image,xmin,xmax,ymin,ymax);
					break;
				
				case GL_UNSIGNED_INT:
					average=RectangleAverager<GLuint>::averageRect(image,xmin,xmax,ymin,ymax);
					break;
				
				case GL_INT:
					average=RectangleAverager<GLint>::averageRect(image,xmin,xmax,ymin,ymax);
					break;
				
				case GL_FLOAT:
					average=RectangleAverager<GLfloat>::averageRect(image,xmin,xmax,ymin,ymax);
					break;
				}
			Misc::formattedUserNote("PipetteTool: Extracted RGBA color: (%f, %f, %f,%f)",average[0],average[1],average[2],average[3]);
			}
		}
	}

void ImageViewer::PipetteTool::frame(void)
	{
	if(dragging)
		{
		/* Update the current pixel position: */
		setPixelPos();
		}
	}

void ImageViewer::PipetteTool::display(GLContextData& contextData) const
	{
	if(dragging)
		{
		/* Set up OpenGL state: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glLineWidth(1.0f);
		
		/* Temporarily go to navigation coordinates: */
		glPushMatrix();
		glLoadMatrix(Vrui::getDisplayState(contextData).modelviewNavigational);
		
		/* Draw the current dragging rectangle: */
		glBegin(GL_LINE_LOOP);
		glColor(Vrui::getForegroundColor());
		glVertex3f(float(x0),float(y0),0.01f);
		glVertex3f(float(x),float(y0),0.01f);
		glVertex3f(float(x),float(y),0.01f);
		glVertex3f(float(x0),float(y),0.01f);
		glEnd();
		
		/* Go back to physical coordinates: */
		glPopMatrix();
		
		/* Restore OpenGL state: */
		glPopAttrib();
		}
	}

/****************************
Methods of class ImageViewer:
****************************/

ImageViewer::ImageViewer(int& argc,char**& argv)
	:Vrui::Application(argc,argv)
	{
	/* Parse the command line: */
	const char* imageFileName=0;
	bool printInfo=false;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"p")==0)
				printInfo=true;
			}
		else if(imageFileName==0)
			imageFileName=argv[i];
		}
	if(imageFileName==0)
		throw std::runtime_error("ImageViewer: No image file name provided");
	
	/* Load the image into the texture set: */
	Images::BaseImage image=Images::readGenericImageFile(imageFileName);
	Images::TextureSet::Texture& tex=textures.addTexture(image,GL_TEXTURE_2D,image.getInternalFormat(),0U);
	
	if(printInfo)
		{
		/* Display image size and format: */
		char messageText[2048];
		const char* componentScalarType=0;
		switch(image.getScalarType())
			{
			case GL_BYTE:
				componentScalarType="signed 8-bit integer";
				break;
			
			case GL_UNSIGNED_BYTE:
				componentScalarType="unsigned 8-bit integer";
				break;
			
			case GL_SHORT:
				componentScalarType="signed 16-bit integer";
				break;
			
			case GL_UNSIGNED_SHORT:
				componentScalarType="unsigned 16-bit integer";
				break;
			
			case GL_INT:
				componentScalarType="signed 32-bit integer";
				break;
			
			case GL_UNSIGNED_INT:
				componentScalarType="unsigned 32-bit integer";
				break;
			
			case GL_FLOAT:
				componentScalarType="32-bit floating-point number";
				break;
			
			case GL_DOUBLE:
				componentScalarType="64-bit floating-point number";
				break;
			
			default:
				componentScalarType="<unknown>";
			}
		Misc::formattedUserNote("Image: %s\nSize: %u x %u pixels\nFormat: %u %s of %u %s%s\nComponent type: %s",imageFileName,image.getSize(0),image.getSize(1),image.getNumChannels(),image.getNumChannels()!=1?"channels":"channel",image.getChannelSize(),image.getChannelSize()!=1?"bytes":"byte",image.getNumChannels()!=1?" each":"",componentScalarType);
		}
	
	/* Set clamping and filtering parameters for mip-mapped linear interpolation: */
	tex.setMipmapRange(0,1000);
	tex.setWrapModes(GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE);
	tex.setFilterModes(GL_LINEAR_MIPMAP_LINEAR,GL_LINEAR);
	
	/* Initialize the pipette tool class: */
	PipetteTool::initClass();
	}

ImageViewer::~ImageViewer(void)
	{
	}

void ImageViewer::display(GLContextData& contextData) const
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	
	/* Get the texture set's GL state: */
	Images::TextureSet::GLState* texGLState=textures.getGLState(contextData);
	
	/* Bind the texture object: */
	const Images::TextureSet::GLState::Texture& tex=texGLState->bindTexture(0U);
	const Images::BaseImage& image=tex.getImage();
	
	/* Query the range of texture coordinates: */
	const GLfloat* texMin=tex.getTexCoordMin();
	const GLfloat* texMax=tex.getTexCoordMax();
	
	/* Draw the image: */
	glBegin(GL_QUADS);
	glTexCoord2f(texMin[0],texMin[1]);
	glVertex2i(0,0);
	glTexCoord2f(texMax[0],texMin[1]);
	glVertex2i(image.getSize(0),0);
	glTexCoord2f(texMax[0],texMax[1]);
	glVertex2i(image.getSize(0),image.getSize(1));
	glTexCoord2f(texMin[0],texMax[1]);
	glVertex2i(0,image.getSize(1));
	glEnd();
	
	/* Protect the texture object: */
	glBindTexture(GL_TEXTURE_2D,0);
	
	/* Draw the image's backside: */
	glDisable(GL_TEXTURE_2D);
	glMaterial(GLMaterialEnums::FRONT,GLMaterial(GLMaterial::Color(0.7f,0.7f,0.7f)));
	
	glBegin(GL_QUADS);
	glNormal3f(0.0f,0.0f,-1.0f);
	glVertex2i(0,0);
	glVertex2i(0,image.getSize(1));
	glVertex2i(image.getSize(0),image.getSize(1));
	glVertex2i(image.getSize(0),0);
	glEnd();
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

void ImageViewer::resetNavigation(void)
	{
	/* Access the image: */
	const Images::BaseImage& image=textures.getTexture(0U).getImage();
	
	/* Reset the Vrui navigation transformation: */
	Vrui::Scalar w=Vrui::Scalar(image.getSize(0));
	Vrui::Scalar h=Vrui::Scalar(image.getSize(1));
	Vrui::Point center(Math::div2(w),Math::div2(h),Vrui::Scalar(0.01));
	Vrui::Scalar size=Math::sqrt(Math::sqr(w)+Math::sqr(h));
	Vrui::setNavigationTransformation(center,size,Vrui::Vector(0,1,0));
	}

/* Create and execute an application object: */
VRUI_APPLICATION_RUN(ImageViewer)
