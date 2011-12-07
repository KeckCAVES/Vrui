/***********************************************************************
GLLightTracker - Class to keep track of changes to OpenGL's light source
state to support just-in-time compilation of GLSL shaders depending on
the OpenGL context's current lighting state.
Copyright (c) 2011 Oliver Kreylos

This file is part of the OpenGL C++ Wrapper Library (GLWrappers).

The OpenGL C++ Wrapper Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenGL C++ Wrapper Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenGL C++ Wrapper Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <GL/GLLightTracker.h>

/***************************************
Static elements of class GLLightTracker:
***************************************/

const char* GLLightTracker::accumulateLightTemplate=
	"\
	void accumulateLight<lightIndex>(in vec4 vertexEc,in vec3 normalEc,in vec4 ambient,in vec4 diffuse,in vec4 specular,in float shininess,inout vec4 ambientDiffuseAccum,inout vec4 specularAccum)\n\
		{\n\
		/* Compute the light direction (works both for directional and point lights): */\n\
		vec3 lightDirEc=gl_LightSource[<lightIndex>].position.xyz*vertexEc.w-vertexEc.xyz*gl_LightSource[<lightIndex>].position.w;\n\
		lightDirEc=normalize(lightDirEc);\n\
		\n\
		/* Calculate per-source ambient light term: */\n\
		ambientDiffuseAccum+=gl_LightSource[<lightIndex>].ambient*ambient;\n\
		\n\
		/* Compute the diffuse lighting angle: */\n\
		float nl=dot(normalEc,lightDirEc);\n\
		if(nl>0.0)\n\
			{\n\
			/* Calculate per-source diffuse light term: */\n\
			ambientDiffuseAccum+=(gl_LightSource[<lightIndex>].diffuse*diffuse)*nl;\n\
			\n\
			/* Compute the eye direction: */\n\
			vec3 eyeDirEc=normalize(-vertexEc.xyz);\n\
			\n\
			/* Compute the specular lighting angle: */\n\
			float nhv=max(dot(normalEc,normalize(eyeDirEc+lightDirEc)),0.0);\n\
			\n\
			/* Calculate per-source specular lighting term: */\n\
			specularAccum+=(gl_LightSource[<lightIndex>].specular*specular)*pow(nhv,shininess);\n\
			}\n\
		}\n\
	\n";

const char* GLLightTracker::accumulateAttenuatedLightTemplate=
	"\
	void accumulateLight<lightIndex>(in vec4 vertexEc,in vec3 normalEc,in vec4 ambient,in vec4 diffuse,in vec4 specular,in float shininess,inout vec4 ambientDiffuseAccum,inout vec4 specularAccum)\n\
		{\n\
		/* Compute the light direction (works both for directional and point lights): */\n\
		vec3 lightDirEc=gl_LightSource[<lightIndex>].position.xyz*vertexEc.w-vertexEc.xyz*gl_LightSource[<lightIndex>].position.w;\n\
		lightDist=length(lightDirEc);\n\
		lightDirEc=normalize(lightDirEc);\n\
		\n\
		/* Calculate the source attenuation factor: */\n\
		float att=1.0/((gl_LightSource[<lightIndex>].quadraticAttenuation*lightDist+gl_LightSource[<lightIndex>].linearAttenuation)*lightDist+gl_LightSource[<lightIndex>].constantAttenuation);\n\
		\n\
		/* Calculate per-source ambient light term: */\n\
		ambientDiffuseAccum+=(gl_LightSource[<lightIndex>].ambient*ambient)*att;\n\
		\n\
		/* Compute the diffuse lighting angle: */\n\
		float nl=dot(normalEc,lightDirEc);\n\
		if(nl>0.0)\n\
			{\n\
			\n\
			/* Calculate per-source diffuse light term: */\n\
			ambientDiffuseAccum+=(gl_LightSource[<lightIndex>].diffuse*diffuse)*(nl*att);\n\
			\n\
			/* Compute the eye direction: */\n\
			vec3 eyeDirEc=normalize(-vertexEc.xyz);\n\
			\n\
			/* Compute the specular lighting angle: */\n\
			float nhv=max(dot(normalEc,normalize(eyeDirEc+lightDirEc)),0.0);\n\
			\n\
			/* Calculate per-source specular lighting term: */\n\
			specularAccum+=(gl_LightSource[<lightIndex>].specular*specular)*(pow(nhv,shininess)*att);\n\
			}\n\
		}\n\
	\n";

const char* GLLightTracker::accumulateSpotLightTemplate=
	"\
	void accumulateLight<lightIndex>(in vec4 vertexEc,in vec3 normalEc,in vec4 ambient,in vec4 diffuse,in vec4 specular,in float shininess,inout vec4 ambientDiffuseAccum,inout vec4 specularAccum)\n\
		{\n\
		/* Compute the light direction (works both for directional and point lights): */\n\
		vec3 lightDirEc=gl_LightSource[<lightIndex>].position.xyz*vertexEc.w-vertexEc.xyz*gl_LightSource[<lightIndex>].position.w;\n\
		lightDirEc=normalize(lightDirEc);\n\
		\n\
		/* Calculate the spot light angle: */\n\
		float sl=-dot(lightDirEc,normalize(gl_LightSource[<lightIndex>].spotDirection));\n\
		\n\
		/* Check if the point is inside the spot light's cone: */\n\
		if(sl>=gl_LightSource[<lightIndex>].spotCosCutoff)\n\
			{\n\
			/* Calculate the spot light attenuation factor: */\n\
			float att=pow(sl,gl_LightSource[<lightIndex>].spotExponent);\n\
			\n\
			/* Calculate per-source ambient light term: */\n\
			ambientDiffuseAccum+=(gl_LightSource[<lightIndex>].ambient*ambient)*att;\n\
			\n\
			/* Compute the diffuse lighting angle: */\n\
			float nl=dot(normalEc,lightDirEc);\n\
			if(nl>0.0)\n\
				{\n\
				/* Calculate per-source diffuse light term: */\n\
				ambientDiffuseAccum+=(gl_LightSource[<lightIndex>].diffuse*diffuse)*(nl*att);\n\
				\n\
				/* Compute the eye direction: */\n\
				vec3 eyeDirEc=normalize(-vertexEc.xyz);\n\
				\n\
				/* Compute the specular lighting angle: */\n\
				float nhv=max(dot(normalEc,normalize(eyeDirEc+lightDirEc)),0.0);\n\
				\n\
				/* Calculate per-source specular lighting term: */\n\
				specularAccum+=(gl_LightSource[<lightIndex>].specular*specular)*(pow(nhv,shininess)*att);\n\
				}\n\
			}\n\
		}\n\
	\n";

const char* GLLightTracker::accumulateAttenuatedSpotLightTemplate=
	"\
	void accumulateLight<lightIndex>(in vec4 vertexEc,in vec3 normalEc,in vec4 ambient,in vec4 diffuse,in vec4 specular,in float shininess,inout vec4 ambientDiffuseAccum,inout vec4 specularAccum)\n\
		{\n\
		/* Compute the light direction (works both for directional and point lights): */\n\
		vec3 lightDirEc=gl_LightSource[<lightIndex>].position.xyz*vertexEc.w-vertexEc.xyz*gl_LightSource[<lightIndex>].position.w;\n\
		lightDirEc=normalize(lightDirEc);\n\
		\n\
		/* Calculate the spot light angle: */\n\
		float sl=-dot(lightDirEc,normalize(gl_LightSource[<lightIndex>].spotDirection))\n\
		\n\
		/* Check if the point is inside the spot light's cone: */\n\
		if(sl>=gl_LightSource[<lightIndex>].spotCosCutoff)\n\
			{\n\
			/* Calculate the source attenuation factor: */\n\
			float att=1.0/((gl_LightSource[<lightIndex>].quadraticAttenuation*lightDist+gl_LightSource[<lightIndex>].linearAttenuation)*lightDist+gl_LightSource[<lightIndex>].constantAttenuation);\n\
			\n\
			/* Calculate the spot light attenuation factor: */\n\
			att*=pow(sl,gl_LightSource[<lightIndex>].spotExponent);\n\
			\n\
			/* Calculate per-source ambient light term: */\n\
			ambientDiffuseAccum+=((gl_LightSource[<lightIndex>].ambient*ambient)*att;\n\
			\n\
			/* Compute the diffuse lighting angle: */\n\
			float nl=dot(normalEc,lightDirEc);\n\
			if(nl>0.0)\n\
				{\n\
				/* Calculate per-source diffuse light term: */\n\
				ambientDiffuseAccum+=(gl_LightSource[<lightIndex>].diffuse*diffuse)*(nl*att);\n\
				\n\
				/* Compute the eye direction: */\n\
				vec3 eyeDirEc=normalize(-vertexEc.xyz);\n\
				\n\
				/* Compute the specular lighting angle: */\n\
				float nhv=max(dot(normalEc,normalize(eyeDirEc+lightDirEc)),0.0);\n\
				\n\
				/* Calculate per-source specular lighting term: */\n\
				specularAccum+=(gl_LightSource[<lightIndex>].specular*specular)*(pow(nhv,shininess)*att);\n\
				}\n\
			}\n\
		}\n\
	\n";

/*******************************
Methods of class GLLightTracker:
*******************************/

GLLightTracker::GLLightTracker(void)
	:version(0),
	 lightingEnabled(false),
	 maxNumLights(0),lightStates(0),
	 specularColorSeparate(false),
	 lightingTwoSided(false),
	 colorMaterials(false),
	 colorMaterialFace(GL_FRONT_AND_BACK),colorMaterialProperty(GL_AMBIENT_AND_DIFFUSE)
	{
	/* Determine the maximum number of light sources supported by the local OpenGL: */
	glGetIntegerv(GL_MAX_LIGHTS,&maxNumLights);
	
	/* Initialize the light state array: */
	lightStates=new LightState[maxNumLights];
	for(int i=0;i<maxNumLights;++i)
		{
		lightStates[i].enabled=i==0;
		lightStates[i].attenuated=false;
		lightStates[i].spotLight=false;
		}
	
	/* Query the current lighting state: */
	update();
	
	/* Reset the version number to one, even if there was no change: */
	version=1;
	}

GLLightTracker::~GLLightTracker(void)
	{
	/* Destroy the light state array: */
	delete[] lightStates;
	}

bool GLLightTracker::update(void)
	{
	bool changed=false;
	
	/* Check the lighting master switch: */
	bool newLightingEnabled=glIsEnabled(GL_LIGHTING);
	changed=changed||lightingEnabled!=newLightingEnabled;
	lightingEnabled=newLightingEnabled;
	
	if(lightingEnabled)
		{
		/* Check all light source states: */
		for(int lightIndex=0;lightIndex<maxNumLights;++lightIndex)
			{
			GLenum light=GL_LIGHT0+lightIndex;
			
			/* Get the light's enabled flag: */
			bool enabled=glIsEnabled(light);
			changed=changed||lightStates[lightIndex].enabled!=enabled;
			lightStates[lightIndex].enabled=enabled;
			
			if(enabled)
				{
				/* Determine the light's attenuation and spot light state: */
				bool attenuated=false;
				bool spotLight=false;
				
				/* Get the light's position: */
				GLfloat pos[4];
				glGetLightfv(light,GL_POSITION,pos);
				if(pos[3]!=0.0f)
					{
					/* Get the light's attenuation coefficients: */
					GLfloat att[3];
					glGetLightfv(light,GL_CONSTANT_ATTENUATION,&att[0]);
					glGetLightfv(light,GL_LINEAR_ATTENUATION,&att[1]);
					glGetLightfv(light,GL_QUADRATIC_ATTENUATION,&att[2]);
					
					/* Determine whether the light is attenuated: */
					attenuated=att[0]!=1.0f||att[1]!=0.0f||att[2]!=0.0f;
					
					/* Get the light's spot light cutoff angle: */
					GLfloat spotLightCutoff;
					glGetLightfv(light,GL_SPOT_CUTOFF,&spotLightCutoff);
					spotLight=spotLightCutoff<=90.0f;
					}
				
				changed=changed||lightStates[lightIndex].attenuated!=attenuated||lightStates[lightIndex].spotLight!=spotLight;
				lightStates[lightIndex].attenuated=attenuated;
				lightStates[lightIndex].spotLight=spotLight;
				}
			}
		
		/* Check for separate specular color: */
		GLint lightModelColorControl;
		glGetIntegerv(GL_LIGHT_MODEL_COLOR_CONTROL,&lightModelColorControl);
		bool newSpecularColorSeparate=lightModelColorControl==GL_SEPARATE_SPECULAR_COLOR;
		changed=changed||specularColorSeparate!=newSpecularColorSeparate;
		specularColorSeparate=newSpecularColorSeparate;
		
		/* Check the two-sided lighting flag: */
		GLint lightModelTwoSide;
		glGetIntegerv(GL_LIGHT_MODEL_TWO_SIDE,&lightModelTwoSide);
		bool newLightingTwoSided=lightModelTwoSide!=0;
		changed=changed||lightingTwoSided!=newLightingTwoSided;
		lightingTwoSided=newLightingTwoSided;
		
		/* Check the color material flag: */
		bool newColorMaterials=glIsEnabled(GL_COLOR_MATERIAL);
		changed=changed||colorMaterials!=newColorMaterials;
		colorMaterials=newColorMaterials;
		
		if(colorMaterials)
			{
			/* Get the color material face: */
			GLint newColorMaterialFace;
			glGetIntegerv(GL_COLOR_MATERIAL_FACE,&newColorMaterialFace);
			changed=changed||colorMaterialFace!=GLenum(newColorMaterialFace);
			colorMaterialFace=GLenum(newColorMaterialFace);
			
			/* Get the color material property: */
			GLint newColorMaterialProperty;
			glGetIntegerv(GL_COLOR_MATERIAL_PARAMETER,&newColorMaterialProperty);
			changed=changed||colorMaterialProperty!=GLenum(newColorMaterialProperty);
			colorMaterialProperty=GLenum(newColorMaterialProperty);
			}
		}
	
	/* Update the version number if there was a change: */
	if(changed)
		++version;
	
	return changed;
	}

std::string GLLightTracker::createAccumulateLightFunction(int lightIndex) const
	{
	std::string result;
	
	/* Create the light index string: */
	std::string index;
	char buffer[10];
	char* bufPtr=buffer;
	int li=lightIndex;
	do
		{
		*(bufPtr++)=char(li%10+'0');
		li/=10;
		}
	while(li!=0);
	while(bufPtr>buffer)
		index.push_back(*(--bufPtr));
	
	/* Find the appropriate function template string: */
	const char* functionTemplate;
	if(lightStates[lightIndex].attenuated&&lightStates[lightIndex].spotLight)
		functionTemplate=accumulateAttenuatedSpotLightTemplate;
	else if(lightStates[lightIndex].spotLight)
		functionTemplate=accumulateSpotLightTemplate;
	else if(lightStates[lightIndex].attenuated)
		functionTemplate=accumulateAttenuatedLightTemplate;
	else
		functionTemplate=accumulateLightTemplate;
	
	/* Replace all occurrences of <lightIndex> in the template string with the light index string: */
	const char* match="<lightIndex>";
	const char* matchStart=0;
	int matchLen=0;
	for(const char* tPtr=functionTemplate;*tPtr!='\0';++tPtr)
		{
		if(matchLen==0)
			{
			if(*tPtr=='<')
				{
				matchStart=tPtr;
				matchLen=1;
				}
			else
				result.push_back(*tPtr);
			}
		else if(matchLen<12)
			{
			if(*tPtr==match[matchLen])
				{
				++matchLen;
				if(matchLen==12)
					{
					result.append(index);
					matchLen=0;
					}
				}
			else
				{
				for(const char* cPtr=matchStart;cPtr!=tPtr;++cPtr)
					result.push_back(*cPtr);
				matchLen=0;
				--tPtr;
				}
			}
		}
	
	return result;
	}
