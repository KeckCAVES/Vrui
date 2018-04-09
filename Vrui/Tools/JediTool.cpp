/***********************************************************************
JediTool - Class for tools using light sabers to point out features in a
3D display.
Copyright (c) 2007-2018 Oliver Kreylos

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

#include <Vrui/Tools/JediTool.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLMaterialTemplates.h>
#include <GL/GLLight.h>
#include <GL/GLVertexArrayParts.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLGeometryVertex.h>
#include <Images/ReadImageFile.h>
#include <Vrui/Vrui.h>
#include <Vrui/Lightsource.h>
#include <Vrui/LightsourceManager.h>
#include <Vrui/ToolManager.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Internal/Config.h>

namespace Vrui {

/********************************
Methods of class JediToolFactory:
********************************/

JediToolFactory::JediToolFactory(ToolManager& toolManager)
	:ToolFactory("JediTool",toolManager),
	 lightsaberLength(Scalar(48)*getInchFactor()),
	 lightsaberWidth(Scalar(6)*getInchFactor()),
	 baseOffset(Scalar(3)*getInchFactor()),
	 hiltTransform(ONTransform::identity),
	 hiltLength(Scalar(8)*getInchFactor()),hiltRadius(Scalar(0.75)*getInchFactor()),
	 lightsaberImageFileName(std::string(VRUI_INTERNAL_CONFIG_SHAREDIR)+"/Textures/Lightsaber.png"),
	 numLightsources(0),lightRadius(Scalar(48)*getInchFactor())
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("PointingTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	lightsaberLength=cfs.retrieveValue<Scalar>("./lightsaberLength",lightsaberLength);
	lightsaberWidth=cfs.retrieveValue<Scalar>("./lightsaberWidth",lightsaberWidth);
	baseOffset=cfs.retrieveValue<Scalar>("./baseOffset",baseOffset);
	hiltTransform=cfs.retrieveValue<ONTransform>("./hiltTransform",hiltTransform);
	hiltLength=cfs.retrieveValue<Scalar>("./hiltLength",hiltLength);
	hiltRadius=cfs.retrieveValue<Scalar>("./hiltRadius",hiltRadius);
	lightsaberImageFileName=cfs.retrieveString("./lightsaberImageFileName",lightsaberImageFileName);
	numLightsources=cfs.retrieveValue<unsigned int>("./numLightsources",numLightsources);
	lightRadius=cfs.retrieveValue<Scalar>("./lightRadius",lightRadius);
	
	/* Set tool class' factory pointer: */
	JediTool::factory=this;
	}

JediToolFactory::~JediToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	JediTool::factory=0;
	}

const char* JediToolFactory::getName(void) const
	{
	return "Jedi Tool";
	}

const char* JediToolFactory::getButtonFunction(int) const
	{
	return "Toggle on / off";
	}

Tool* JediToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new JediTool(this,inputAssignment);
	}

void JediToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveJediToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("UtilityTool");
	}

extern "C" ToolFactory* createJediToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	JediToolFactory* jediToolFactory=new JediToolFactory(*toolManager);
	
	/* Return factory object: */
	return jediToolFactory;
	}

extern "C" void destroyJediToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/***********************************
Methods of class JediTool::DataItem:
***********************************/

JediTool::DataItem::DataItem(void)
	{
	/* Initialize required OpenGL extensions: */
	GLARBVertexBufferObject::initExtension();
	
	/* Allocate texture and buffer objects: */
	glGenTextures(1,&textureObjectId);
	glGenBuffersARB(1,&hiltVertexBufferId);
	}

JediTool::DataItem::~DataItem(void)
	{
	/* Release texture and buffer objects: */
	glDeleteTextures(1,&textureObjectId);
	glDeleteBuffersARB(1,&hiltVertexBufferId);
	}

/*********************************
Static elements of class JediTool:
*********************************/

JediToolFactory* JediTool::factory=0;

/*************************
Methods of class JediTool:
*************************/

JediTool::JediTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:PointingTool(factory,inputAssignment),
	 GLObject(false),
	 lightsaberImage(Images::readGenericImageFile(JediTool::factory->lightsaberImageFileName.c_str())),
	 lightsources(JediTool::factory->numLightsources>0?new Lightsource*[JediTool::factory->numLightsources]:0),
	 active(false)
	{
	GLObject::init();
	}

JediTool::~JediTool(void)
	{
	/* Destroy the array of light sources: */
	delete[] lightsources;
	}

void JediTool::initialize(void)
	{
	if(factory->numLightsources>0)
		{
		/* Set up common light source parameters: */
		GLLight lightSaberGlow;
		lightSaberGlow.ambient=lightSaberGlow.diffuse=lightSaberGlow.specular=GLLight::Color(0.0f,0.0f,0.0f);
		
		/* Set up the light source attenuation factors according to light radius (diminish to 1% at radius): */
		lightSaberGlow.constantAttenuation=0.5f;
		lightSaberGlow.linearAttenuation=0.0f; // 49.5f/factory->lightRadius;
		lightSaberGlow.quadraticAttenuation=99.5f/Math::sqr(factory->lightRadius);
		
		/* Create a number of light sources: */
		for(unsigned int i=0;i<factory->numLightsources;++i)
			{
			lightsources[i]=getLightsourceManager()->createLightsource(true,lightSaberGlow);
			lightsources[i]->disable();
			}
		}
	}

void JediTool::deinitialize(void)
	{
	/* Destroy all allocated light sources: */
	for(unsigned int i=0;i<factory->numLightsources;++i)
		{
		getLightsourceManager()->destroyLightsource(lightsources[i]);
		lightsources[i]=0;
		}
	}

const ToolFactory* JediTool::getFactory(void) const
	{
	return factory;
	}

void JediTool::buttonCallback(int,InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Activation button has just been pressed
		{
		if(!active)
			{
			/* Activate the light saber: */
			active=true;
			activationTime=getApplicationTime();
			
			/* Initialize the light saber billboard: */
			ONTransform lightsaberTransform=getButtonDeviceTransformation(0)*factory->hiltTransform;
			origin[1]=lightsaberTransform.getOrigin();
			axis[1]=lightsaberTransform.transform(getButtonDevice(0)->getDeviceRayDirection());
			length[1]=Scalar(0);
			
			if(factory->numLightsources>0)
				{
				/* Activate the glow light sources: */
				for(unsigned int i=0;i<factory->numLightsources;++i)
					lightsources[i]->enable();
				}
			}
		else
			{
			/* Deactivate the light saber: */
			active=false;
			
			if(factory->numLightsources>0)
				{
				/* Deactivate the glow light sources: */
				for(unsigned int i=0;i<factory->numLightsources;++i)
					lightsources[i]->disable();
				}
			}
		}
	}

void JediTool::frame(void)
	{
	/* Save last frame's state: */
	origin[0]=origin[1];
	axis[0]=axis[1];
	length[0]=length[1];
	
	/* Update the light saber hilt and billboard: */
	ONTransform lightsaberTransform=getButtonDeviceTransformation(0)*factory->hiltTransform;
	origin[1]=lightsaberTransform.getOrigin();
	axis[1]=lightsaberTransform.transform(getButtonDevice(0)->getDeviceRayDirection());
	
	if(active)
		{
		/* Scale the lightsaber during activation: */
		length[1]=factory->lightsaberLength;
		double activeTime=getApplicationTime()-activationTime;
		if(activeTime<1.5)
			{
			length[1]*=activeTime/1.5;
			
			/* Request another frame: */
			scheduleUpdate(getNextAnimationTime());
			}
		
		if(factory->numLightsources>0)
			{
			/* Turn the glow light sources on gently: */
			GLfloat intensity=activeTime<1.5?GLfloat(activeTime/1.5):1.0f;
			GLLight::Color glowColor(intensity,0.0f,0.0f,1.0f);
			
			/* Position the glow light sources evenly along the light saber blade: */
			for(unsigned int i=0;i<factory->numLightsources;++i)
				{
				Point pos=origin[1]+axis[1]*((Scalar(i)+Scalar(0.5))/Scalar(factory->numLightsources)*length[1]);
				GLLight& light=lightsources[i]->getLight();
				light.diffuse=light.specular=glowColor;
				light.position=GLLight::Position(GLfloat(pos[0]),GLfloat(pos[1]),GLfloat(pos[2]),1.0f);
				}
			}
		}
	}

void JediTool::display(GLContextData& contextData) const
	{
	/* Get the data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Set up OpenGL state: */
	glMaterialAmbientAndDiffuse(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(0.6f,0.6f,0.6f));
	glMaterialSpecular(GLMaterialEnums::FRONT,GLColor<GLfloat,4>(1.0f,1.0f,1.0f));
	glMaterialShininess(GLMaterialEnums::FRONT,32.0f);
	
	/* Transform the hilt to the light saber's position: */
	glPushMatrix();
	glTranslate(origin[1]-Point::origin);
	glRotate(Rotation::rotateFromTo(Vector(0,0,1),axis[1]));
	
	/* Bind the vertex buffer: */
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->hiltVertexBufferId);
	
	/* Draw the hilt geometry: */
	typedef GLGeometry::Vertex<void,0,void,0,GLfloat,GLfloat,3> Vertex; // Type for vertices
	const int numSegments=16;
	GLVertexArrayParts::enable(Vertex::getPartsMask());
	glVertexPointer(static_cast<const Vertex*>(0));
	glDrawArrays(GL_TRIANGLES,0,(numSegments*2+(numSegments-2)*2)*3);
	GLVertexArrayParts::disable(Vertex::getPartsMask());
	
	/* Protect the vertex buffer: */
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	
	/* Return to physical space: */
	glPopMatrix();
	}

void JediTool::initContext(GLContextData& contextData) const
	{
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Upload the light saber image as a 2D texture: */
	glBindTexture(GL_TEXTURE_2D,dataItem->textureObjectId);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	lightsaberImage.glTexImage2D(GL_TEXTURE_2D,0);
	glBindTexture(GL_TEXTURE_2D,0);
	
	typedef GLGeometry::Vertex<void,0,void,0,GLfloat,GLfloat,3> Vertex; // Type for vertices
	const int numSegments=16;
	
	/* Create a vertex array to render the light saber hilt: */
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,dataItem->hiltVertexBufferId);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,(numSegments*2+(numSegments-2)*2)*3*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
	Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	
	Scalar hr=factory->hiltRadius;
	Scalar hl=factory->hiltLength;
	
	/* Create the hilt mantle: */
	for(int i=0;i<numSegments;++i,vPtr+=6)
		{
		Scalar a0=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(i)/Scalar(numSegments);
		Scalar x0=Math::cos(a0);
		Scalar y0=Math::sin(a0);
		Scalar a1=Scalar(2)*Math::Constants<Scalar>::pi*Scalar((i+1)%numSegments)/Scalar(numSegments);
		Scalar x1=Math::cos(a1);
		Scalar y1=Math::sin(a1);
		vPtr[0].normal=Vertex::Normal(x0,y0,0);
		vPtr[0].position=Vertex::Position(x0*hr,y0*hr,-hl);
		vPtr[1].normal=Vertex::Normal(x1,y1,0);
		vPtr[1].position=Vertex::Position(x1*hr,y1*hr,-hl);
		vPtr[2].normal=Vertex::Normal(x1,y1,0);
		vPtr[2].position=Vertex::Position(x1*hr,y1*hr,0);
		vPtr[3].normal=Vertex::Normal(x1,y1,0);
		vPtr[3].position=Vertex::Position(x1*hr,y1*hr,0);
		vPtr[4].normal=Vertex::Normal(x0,y0,0);
		vPtr[4].position=Vertex::Position(x0*hr,y0*hr,0);
		vPtr[5].normal=Vertex::Normal(x0,y0,0);
		vPtr[5].position=Vertex::Position(x0*hr,y0*hr,-hl);
		}
	
	/* Create the bottom and top caps: */
	Vertex::Normal bn(0,0,-1);
	Vertex::Position bv0(hr,0,-hl);
	Vertex::Normal tn(0,0,1);
	Vertex::Position tv0(hr,0,0);
	for(int i=1;i<numSegments-1;++i,vPtr+=6)
		{
		Scalar a0=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(i)/Scalar(numSegments);
		Scalar x0=Math::cos(a0);
		Scalar y0=Math::sin(a0);
		Scalar a1=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(i+1)/Scalar(numSegments);
		Scalar x1=Math::cos(a1);
		Scalar y1=Math::sin(a1);
		vPtr[0].normal=bn;
		vPtr[0].position=bv0;
		vPtr[1].normal=bn;
		vPtr[1].position=Vertex::Position(x1*hr,y1*hr,-hl);
		vPtr[2].normal=bn;
		vPtr[2].position=Vertex::Position(x0*hr,y0*hr,-hl);
		vPtr[3].normal=tn;
		vPtr[3].position=tv0;
		vPtr[4].normal=tn;
		vPtr[4].position=Vertex::Position(x0*hr,y0*hr,0);
		vPtr[5].normal=tn;
		vPtr[5].position=Vertex::Position(x1*hr,y1*hr,0);
		}
	
	/* Unmap and protect the vertex buffer: */
	glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	}

void JediTool::glRenderActionTransparent(GLContextData& contextData) const
	{
	if(active)
		{
		/* Get the data item: */
		DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
		
		/* Get the eye position for the current rendering pass from Vrui's display state: */
		const Point& eyePosition=Vrui::getDisplayState(contextData).eyePosition;
		
		/* Calculate the midpoint plane between the blade's previous and current positions: */
		Vector midDir=axis[0]*length[0]+axis[1]*length[1];
		Point mid=Geometry::mid(origin[0],origin[1]);
		Vector midNormal=midDir^(eyePosition-mid);
		
		/* Calculate the previous and current glow billboards: */
		Point basePoint[2];
		Vector x[2],y[2];
		for(int i=0;i<2;++i)
			{
			y[i]=axis[i];
			x[i]=axis[i]^(eyePosition-origin[i]);
			x[i].normalize();
			y[i]*=length[i]*scaleFactor;
			x[i]*=Math::div2(factory->lightsaberWidth*scaleFactor);
			basePoint[i]=origin[i];
			basePoint[i]-=axis[i]*(factory->baseOffset*scaleFactor);
			}
		
		/* Draw the light saber: */
		glPushAttrib(GL_COLOR_BUFFER_BIT|GL_ENABLE_BIT|GL_POLYGON_BIT|GL_TEXTURE_BIT);
		glDisable(GL_LIGHTING);
		glBlendFunc(GL_ONE,GL_ONE);
		glDisable(GL_CULL_FACE);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,dataItem->textureObjectId);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
		glBegin(GL_QUADS);
		if((origin[1]-origin[0])*midNormal>=Scalar(0))
			{
			/* Draw the left billboard at the previous position, and the right billboard at the current position: */
			glTexCoord2f(0.0f,0.0f);
			glVertex(basePoint[0]-x[0]);
			glTexCoord2f(0.5f,0.0f);
			glVertex(basePoint[0]);
			glTexCoord2f(0.5f,1.0f);
			glVertex(basePoint[0]+y[0]);
			glTexCoord2f(0.0f,1.0f);
			glVertex(basePoint[0]-x[0]+y[0]);
			
			glTexCoord2f(0.5f,0.0f);
			glVertex(basePoint[1]);
			glTexCoord2f(1.0f,0.0f);
			glVertex(basePoint[1]+x[1]);
			glTexCoord2f(1.0f,1.0f);
			glVertex(basePoint[1]+x[1]+y[1]);
			glTexCoord2f(0.5f,1.0f);
			glVertex(basePoint[1]+y[1]);
			}
		else
			{
			/* Draw the right billboard at the previous position, and the left billboard at the current position: */
			glTexCoord2f(0.5f,0.0f);
			glVertex(basePoint[0]);
			glTexCoord2f(1.0f,0.0f);
			glVertex(basePoint[0]+x[0]);
			glTexCoord2f(1.0f,1.0f);
			glVertex(basePoint[0]+x[0]+y[0]);
			glTexCoord2f(0.5f,1.0f);
			glVertex(basePoint[0]+y[0]);
			
			glTexCoord2f(0.0f,0.0f);
			glVertex(basePoint[1]-x[1]);
			glTexCoord2f(0.5f,0.0f);
			glVertex(basePoint[1]);
			glTexCoord2f(0.5f,1.0f);
			glVertex(basePoint[1]+y[1]);
			glTexCoord2f(0.0f,1.0f);
			glVertex(basePoint[1]-x[1]+y[1]);
			}
		
		/* Draw the connecting swish panel: */
		glTexCoord2f(0.5f,0.0f);
		glVertex(basePoint[0]);
		glVertex(basePoint[1]);
		glTexCoord2f(0.5f,1.0f);
		glVertex(basePoint[1]+y[1]);
		glVertex(basePoint[0]+y[0]);
		glEnd();
		glBindTexture(GL_TEXTURE_2D,0);
		glPopAttrib();
		}
	}

}
