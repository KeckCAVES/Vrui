/***********************************************************************
JediTool - Class for tools using light sabers to point out features in a
3D display.
Copyright (c) 2007-2017 Oliver Kreylos

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
#include <GL/GLVertexArrayParts.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLGeometryVertex.h>
#include <Images/ReadImageFile.h>
#include <Vrui/Vrui.h>
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
	 lightsaberImageFileName(std::string(VRUI_INTERNAL_CONFIG_SHAREDIR)+"/Textures/Lightsaber.png")
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
	 lightsaberImage(Images::readImageFile(JediTool::factory->lightsaberImageFileName.c_str())),
	 active(false)
	{
	GLObject::init();
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
			}
		else
			{
			/* Deactivate the light saber: */
			active=false;
			}
		}
	}

void JediTool::frame(void)
	{
	/* Update the light saber hilt and billboard: */
	ONTransform lightsaberTransform=getButtonDeviceTransformation(0)*factory->hiltTransform;
	origin=lightsaberTransform.getOrigin();
	axis=lightsaberTransform.transform(getButtonDevice(0)->getDeviceRayDirection());
	
	if(active)
		{
		/* Scale the lightsaber during activation: */
		length=factory->lightsaberLength;
		double activeTime=getApplicationTime()-activationTime;
		if(activeTime<1.5)
			{
			length*=activeTime/1.5;
			
			/* Request another frame: */
			scheduleUpdate(getNextAnimationTime());
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
	glTranslate(origin-Point::origin);
	glRotate(Rotation::rotateFromTo(Vector(0,0,1),axis));
	
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
	lightsaberImage.glTexImage2D(GL_TEXTURE_2D,0,GL_RGB);
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
		
		/* Calculate the billboard size and orientation: */
		Vector y=axis;
		Vector x=axis^(eyePosition-origin);
		x.normalize();
		y*=length*scaleFactor;
		x*=Math::div2(factory->lightsaberWidth*scaleFactor);
		Point basePoint=origin;
		basePoint-=axis*(factory->baseOffset*scaleFactor);
		
		/* Draw the light saber: */
		glPushAttrib(GL_COLOR_BUFFER_BIT|GL_ENABLE_BIT|GL_POLYGON_BIT|GL_TEXTURE_BIT);
		glDisable(GL_LIGHTING);
		glBlendFunc(GL_ONE,GL_ONE);
		glDisable(GL_CULL_FACE);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,dataItem->textureObjectId);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f,0.0f);
		glVertex(basePoint-x);
		glTexCoord2f(1.0f,0.0f);
		glVertex(basePoint+x);
		glTexCoord2f(1.0f,1.0f);
		glVertex(basePoint+x+y);
		glTexCoord2f(0.0f,1.0f);
		glVertex(basePoint-x+y);
		glEnd();
		glBindTexture(GL_TEXTURE_2D,0);
		glPopAttrib();
		}
	}

}
