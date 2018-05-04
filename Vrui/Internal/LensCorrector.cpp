/***********************************************************************
LensCorrector - Helper class to render imagery into an off-screen buffer
and then warp the buffer to the final drawable to correct subsequent
lens distortion.
Copyright (c) 2014-2018 Oliver Kreylos

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

#include <Vrui/Internal/LensCorrector.h>

#define DEBUG_REPROJECTION 0

#if DEBUG_REPROJECTION
#include <iomanip>
#include <Geometry/OutputOperators.h>
#endif

#include <string>
#include <stdexcept>
#include <iostream>
#include <Misc/SizedTypes.h>
#include <Misc/PrintInteger.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/FixedArray.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ArrayValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/AffineTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/Extensions/GLARBMultitexture.h>
#include <GL/Extensions/GLARBTextureRectangle.h>
#include <GL/Extensions/GLARBShaderObjects.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/Extensions/GLARBVertexProgram.h>
#include <GL/Extensions/GLEXTFramebufferBlit.h>
#include <GL/Extensions/GLEXTFramebufferObject.h>
#include <GL/Extensions/GLEXTFramebufferMultisample.h>
#include <GL/Extensions/GLEXTPackedDepthStencil.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Label.h>
#include <GLMotif/TextField.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRScreen.h>
#include <Vrui/WindowProperties.h>
#include <Vrui/VRWindow.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Internal/Vrui.h>
#include <Vrui/Internal/HMDConfiguration.h>
#include <Vrui/Internal/VRDeviceClient.h>
#include <Vrui/Internal/InputDeviceAdapterDeviceDaemon.h>

namespace Vrui {

/* DEBUGging variables: */
bool lensCorrectorDisableReproject=false;

/**************************************************
Methods of class LensCorrector::DistortionEquation:
**************************************************/

LensCorrector::DistortionEquation::~DistortionEquation(void)
	{
	}

double LensCorrector::DistortionEquation::invert(double rp) const
	{
	/* Run Newton-Raphson iteration to calculate the inverse of dist(): */
	double r2=0.25;
	// double maxR2=getMaxR2(); // Currently not needed
	for(int i=0;i<20;++i)
		{
		/* Calculate distortionEquation(r) and distortionEquation'(r): */
		double dr=(*this)(r2)-rp;
		if(Math::abs(dr)<1.0e-8)
			break;
		double ddr=this->d(r2);
		
		/* Do the step: */
		r2-=dr/ddr;
		if(r2<0.0)
			r2=0.0;
		#if 0 // Must be taken out as values larger than maxR2 might crop up and have to be dealt with
		if(r2>maxR2)
			r2=maxR2;
		#endif
		}
	
	return r2;
	}

namespace {

/**************
Helper classes:
**************/

class PolynomialDistortionEquation:public LensCorrector::DistortionEquation // Class for polynomial equations
	{
	/* Elements: */
	private:
	unsigned int degree; // Polynomial degree
	double* c; // Array of polynomial coefficients
	
	/* Constructors and destructors: */
	public:
	PolynomialDistortionEquation(unsigned int sDegree,const double* sC =0)
		:degree(sDegree),
		 c(new double[degree+1])
		{
		if(sC!=0)
			{
			for(unsigned int i=0;i<=degree;++i)
				c[i]=sC[i];
			}
		else
			{
			for(unsigned int i=0;i<=degree;++i)
				c[i]=0.0;
			}
		}
	virtual ~PolynomialDistortionEquation(void)
		{
		delete[] c;
		}
	
	/* Methods from LensCorrector::DistortionEquation: */
	virtual double getMaxR2(void) const
		{
		return 1.0;
		}
	virtual double operator()(double r2) const
		{
		// result=((...(cn*r2+c(n-1))...+c2)*r2+c1)*r2+c0
		double result=c[degree];
		for(unsigned int i=degree;i>0;--i)
			result=result*r2+c[i-1];
		return result;
		}
	virtual double d(double r2) const
		{
		// result=(...(n*cn*r2+(n-1)*c(n-1))...+2*c2)*r2+c1
		double result=double(degree)*c[degree];
		for(unsigned int i=degree;i>1;--i)
			result=result*r2+double(i-1)*c[i-1];
		return result;
		}
	
	/* New methods: */
	void setC(unsigned int i,double newC)
		{
		c[i]=newC;
		}
	};

class ReciprocalPolynomialDistortionEquation:public LensCorrector::DistortionEquation // Class for reciprocal polynomial equations
	{
	/* Elements: */
	private:
	unsigned int degree; // Polynomial degree
	double* c; // Array of polynomial coefficients
	
	/* Constructors and destructors: */
	public:
	ReciprocalPolynomialDistortionEquation(unsigned int sDegree,const double* sC =0)
		:degree(sDegree),
		 c(new double[degree+1])
		{
		if(sC!=0)
			{
			for(unsigned int i=0;i<=degree;++i)
				c[i]=sC[i];
			}
		else
			{
			for(unsigned int i=0;i<=degree;++i)
				c[i]=0.0;
			}
		}
	virtual ~ReciprocalPolynomialDistortionEquation(void)
		{
		delete[] c;
		}
	
	/* Methods from LensCorrector::DistortionEquation: */
	virtual double getMaxR2(void) const
		{
		return 1.0;
		}
	virtual double operator()(double r2) const
		{
		// result=1.0/(((...(cn*r2+c(n-1))...+c2)*r2+c1)*r2+c0)
		double result=c[degree];
		for(unsigned int i=degree;i>0;--i)
			result=result*r2+c[i-1];
		return 1.0/result;
		}
	virtual double d(double r2) const
		{
		double result=c[degree];
		for(unsigned int i=degree;i>0;--i)
			result=result*r2+c[i-1];
		double dresult=double(degree)*c[degree];
		for(unsigned int i=degree;i>1;--i)
			dresult=dresult*r2+double(i-1)*c[i-1];
		return -dresult/(result*result);
		}
	
	/* New methods: */
	void setC(unsigned int i,double newC)
		{
		c[i]=newC;
		}
	};

class CatmullRomDistortionEquation:public LensCorrector::DistortionEquation // Class for special-purpose Catmull-Rom splines
	{
	/* Elements: */
	private:
	double r2Max; // Maximum squared radius for which the spline is defined; abscissa of controlPoints[numControlPoints-1]
	unsigned int numControlPoints; // Number of spline control points
	double* controlPoints; // Array of spline control point ordinates
	
	/* Constructors and destructors: */
	public:
	CatmullRomDistortionEquation(double sR2Max,unsigned int sNumControlPoints,const double* sControlPoints =0)
		:r2Max(sR2Max),
		 numControlPoints(sNumControlPoints),controlPoints(new double[numControlPoints])
		{
		if(sControlPoints!=0)
			for(unsigned int i=0;i<numControlPoints;++i)
				controlPoints[i]=sControlPoints[i];
		}
	virtual ~CatmullRomDistortionEquation(void)
		{
		delete[] controlPoints;
		}
	
	/* Methods from LensCorrector::DistortionEquation: */
	virtual double getMaxR2(void) const
		{
		return r2Max;
		}
	virtual double operator()(double r2) const
		{
		/* Find the segment index and segment location of the query value: */
		double segLoc=r2*double(numControlPoints-1)/r2Max;
		unsigned int seg=(unsigned int)segLoc;
		if(seg>numControlPoints-1)
			seg=numControlPoints-1;
		double loc=segLoc-double(seg);
		
		/* Find the left and right control point values and slopes of segment seg: */
		double r0,dr0,r1,dr1;
		if(seg==0)
			{
			r0=1.0; // Special case; first control point value is always 1
			dr0=controlPoints[1]-controlPoints[0]; // Special case; first control point only affects slope
			r1=controlPoints[1];
			dr1=0.5*(controlPoints[2]-controlPoints[0]);
			}
		else if(seg==numControlPoints-2)
			{
			r0=controlPoints[seg];
			dr0=0.5*(controlPoints[seg+1]-controlPoints[seg-1]);
			r1=controlPoints[seg+1];
			dr1=controlPoints[seg+1]-controlPoints[seg];
			}
		else if(seg==numControlPoints-1)
			{
			/* Extend the spline past the last control point as a straight line: */
			r0=controlPoints[seg];
			dr0=controlPoints[seg]-controlPoints[seg-1];
			r1=r0+dr0;
			dr1=dr0;
			}
		else
			{
			r0=controlPoints[seg];
			dr0=0.5*(controlPoints[seg+1]-controlPoints[seg-1]);
			r1=controlPoints[seg+1];
			dr1=0.5*(controlPoints[seg+2]-controlPoints[seg]);
			}
		
		/* Interpolate inside the segment: */
		return (r0*(1.0+2.0*loc)+dr0*loc)*(1.0-loc)*(1.0-loc)+(r1*(1.0+2.0*(1.0-loc))-dr1*(1.0-loc))*loc*loc;
		return (((2.0*r0+dr0-2.0*r1+dr1)*loc-3.0*r0-2.0*dr0+3.0*r1-dr1)*loc+dr0)*loc+r0;
		}
	virtual double d(double r2) const
		{
		/* Find the segment index and segment location of the query value: */
		double segLoc=r2*double(numControlPoints-1)/r2Max;
		unsigned int seg=(unsigned int)segLoc;
		if(seg>numControlPoints-1)
			seg=numControlPoints-1;
		double loc=segLoc-double(seg);
		
		/* Find the left and right control point values and slopes of segment seg: */
		double r0,dr0,r1,dr1;
		if(seg==0)
			{
			r0=1.0; // Special case; first control point value is always 1
			dr0=controlPoints[1]-controlPoints[0]; // Special case; first control point only affects slope
			r1=controlPoints[1];
			dr1=0.5*(controlPoints[2]-controlPoints[0]);
			}
		else if(seg==numControlPoints-2)
			{
			r0=controlPoints[seg];
			dr0=0.5*(controlPoints[seg+1]-controlPoints[seg-1]);
			r1=controlPoints[seg+1];
			dr1=controlPoints[seg+1]-controlPoints[seg];
			}
		else if(seg==numControlPoints-1)
			{
			/* Extend the spline past the last control point as a straight line: */
			r0=controlPoints[seg];
			dr0=controlPoints[seg]-controlPoints[seg-1];
			r1=r0+dr0;
			dr1=dr0;
			}
		else
			{
			r0=controlPoints[seg];
			dr0=0.5*(controlPoints[seg+1]-controlPoints[seg-1]);
			r1=controlPoints[seg+1];
			dr1=0.5*(controlPoints[seg+2]-controlPoints[seg]);
			}
		
		/* Interpolate the spline derivative inside the segment: */
		return (((6.0*r0+3.0*dr0-6.0*r1+3.0*dr1)*loc-6.0*r0-4.0*dr0+6.0*r1-2.0*dr1)*loc+dr0)*double(numControlPoints-1)/r2Max;
		}
	
	/* New methods: */
	void setC(unsigned int i,double newC)
		{
		controlPoints[i]=newC;
		}
	};

LensCorrector::DistortionEquation* parseDistortionEquation(const Misc::ConfigurationFileSection& configFileSection)
	{
	LensCorrector::DistortionEquation* result=0;
	
	/* Determine the distortion equation type: */
	std::string eqType=configFileSection.retrieveString("./type");
	if(strcasecmp(eqType.c_str(),"Polynomial")==0)
		{
		/* Read the list of coefficients: */
		std::vector<double> coefficients=configFileSection.retrieveValue<std::vector<double> >("./coefficients");
		
		/* Adjust the first coefficient: */
		coefficients[0]+=1.0;
		
		/* Create the result equation: */
		result=new PolynomialDistortionEquation(coefficients.size()-1,&coefficients.front());
		}
	else if(strcasecmp(eqType.c_str(),"ReciprocalPolynomial")==0)
		{
		/* Read the list of coefficients: */
		std::vector<double> coefficients=configFileSection.retrieveValue<std::vector<double> >("./coefficients");
		
		/* Adjust the first coefficient: */
		coefficients[0]+=1.0;
		
		/* Create the result equation: */
		result=new ReciprocalPolynomialDistortionEquation(coefficients.size()-1,&coefficients.front());
		}
	else if(strcasecmp(eqType.c_str(),"CatmullRomSpline")==0)
		{
		/* Read the maximum squared radius: */
		double r2Max=configFileSection.retrieveValue<double>("./r2Max");
		
		/* Read the list of coefficients: */
		std::vector<double> coefficients=configFileSection.retrieveValue<std::vector<double> >("./coefficients");
		
		/* Create the result equation: */
		result=new CatmullRomDistortionEquation(r2Max,coefficients.size(),&coefficients.front());
		}
	else
		Misc::throwStdErr("Vrui::LensCorrector: Unknown distortion function type %s",eqType.c_str());
	
	return result;
	}

}

/******************************
Methods of class LensCorrector:
******************************/

void LensCorrector::calculateWarpParameters(VRWindow& window)
	{
	/* Update lens correction coefficients based on current viewer configuration: */
	for(int eye=0;eye<2;++eye)
		{
		LensConfig& lc=lensConfigs[eye];
		VRScreen* screen=window.getVRScreen(eye);
		
		/* Place the distortion center directly underneath the lens center: */
		for(int i=0;i<2;++i)
			lc.center[i]=lc.lensCenter[i]/screen->getScreenSize()[i];
		
		/* Calculate the half-tangent physical screen FoV under assumption of collimation: */
		lc.screenFov[0]=screen->getWidth()*0.5/lc.focalLength;
		lc.screenFov[1]=screen->getHeight()*0.5/lc.focalLength;
		
		/* Transform the viewer's left/right eye position to screen space: */
		Point eyePos=window.getViewer(eye)->getDeviceEyePosition(eye==0?Viewer::LEFT:Viewer::RIGHT);
		Point screenEyePos=screen->getTransform().inverseTransform(eyePos);
		
		/* Calculate the half-tangent FoV of the final rendered pre-distortion image: */
		lc.renderedFovs[0]=-screenEyePos[0]/screenEyePos[2];
		lc.renderedFovs[1]=(screen->getWidth()-screenEyePos[0])/screenEyePos[2];
		lc.renderedFovs[2]=-screenEyePos[1]/screenEyePos[2];
		lc.renderedFovs[3]=(screen->getHeight()-screenEyePos[1])/screenEyePos[2];
		
		/* Adjust the rendered FoVs for overscan: */
		double w=lc.renderedFovs[1]-lc.renderedFovs[0];
		lc.renderedFovs[0]-=w*lc.overscan[0];
		lc.renderedFovs[1]+=w*lc.overscan[1];
		double h=lc.renderedFovs[3]-lc.renderedFovs[2];
		lc.renderedFovs[2]-=h*lc.overscan[2];
		lc.renderedFovs[3]+=h*lc.overscan[3];
		
		#if DEBUG_REPROJECTION
		std::cout<<"Eye "<<eye<<":"<<std::endl;
		std::cout<<"  Physical screen FoV: "<<lc.screenFov[0]<<", "<<lc.screenFov[1]<<std::endl;
		std::cout<<"  Eye position: "<<screenEyePos[0]<<", "<<screenEyePos[1]<<", "<<screenEyePos[2]<<std::endl;
		std::cout<<"  Half-FOVs: "<<lc.renderedFovs[0]<<", "<<lc.renderedFovs[1]<<", "<<lc.renderedFovs[2]<<", "<<lc.renderedFovs[3]<<", "<<std::endl;
		#endif
		}
	}

void LensCorrector::uploadWarpMeshes(void) const
	{
	for(int eye=0;eye<2;++eye)
		{
		const LensConfig& lc=lensConfigs[eye];
		
		/* Generate mesh vertex coordinates: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,warpMeshVertexBufferIds[eye]);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,warpMeshSize[1]*warpMeshSize[0]*sizeof(WarpMeshVertex),0,GL_STATIC_DRAW_ARB);
		WarpMeshVertex* wmvPtr=static_cast<WarpMeshVertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
		if(precomputed)
			{
			/* Adapt and upload the pre-computed warp mesh: */
			const WarpMeshVertex* wmPtr=lc.warpMesh;
			for(int y=0;y<warpMeshSize[1];++y)
				for(int x=0;x<warpMeshSize[0];++x,++wmvPtr,++wmPtr)
					{
					WarpMeshVertex wmv=*wmPtr;
					for(int i=0;i<2;++i)
						{
						wmv.redTex[i]*=GLfloat(predistortionFrameSize[i]);
						wmv.greenTex[i]*=GLfloat(predistortionFrameSize[i]);
						wmv.blueTex[i]*=GLfloat(predistortionFrameSize[i]);
						}
					*wmvPtr=wmv;
					}
			}
		else
			{
			/* Set up a transformation to convert mesh vertices from screen space to normalized device coordinates: */
			typedef Geometry::AffineTransformation<double,2> ATransform;
			ATransform meshTransform;
			ATransform::Matrix& mt=meshTransform.getMatrix();
			mt(0,0)=2.0*double(lc.finalViewport[2])/double(finalViewport[2]);
			mt(0,1)=0.0;
			mt(0,2)=2.0*double(lc.finalViewport[0])/double(finalViewport[2])-1.0;
			mt(1,0)=0.0;
			mt(1,1)=2.0*double(lc.finalViewport[3])/double(finalViewport[3]);
			mt(1,2)=2.0*double(lc.finalViewport[1])/double(finalViewport[3])-1.0;
			
			/* Rotate the mesh: */
			meshTransform*=Geometry::AffineTransformation<double,2>::rotateAround(Point2(0.5,0.5),ATransform::Rotation::rotate(Math::rad(double(displayRotation)*90.0)));
			
			/* Scale the mesh: */
			meshTransform*=ATransform::scale(ATransform::Scale(1.0/double(warpMeshSize[0]-1),1.0/double(warpMeshSize[1]-1)));
			
			/* Calculate and upload mesh vertices in sequential order: */
			for(int y=0;y<warpMeshSize[1];++y)
				{
				for(int x=0;x<warpMeshSize[0];++x,++wmvPtr)
					{
					/* The forward distortion formula: */
					
					/* Rectified final viewport position: */
					Point2 post=meshTransform.transform(Point2(x,y));
					
					/***************************************************************
					Calculate the vertex' position in the pre-distortion image
					texture by transforming from screen space to lens-centered
					tangent space, applying the lens distortion correction formula,
					and then from lens-centered tangent space to pre-distortion
					image space.
					***************************************************************/
					
					/* Point in screen space: */
					Point2 pre(double(x)/double(warpMeshSize[0]-1),double(y)/double(warpMeshSize[1]-1));
					
					/* Point in lens-centered tangent space: */
					Point2::Vector preTan=(pre-lc.center)*2.0;
					preTan[0]*=lc.screenFov[0];
					preTan[1]*=lc.screenFov[1];
					
					/* Lens distortion-corrected point in red, green, and blue: */
					double preTanR2=preTan.sqr();
					double scale=(*lc.distortionEquations[0])(preTanR2);
					Point2::Vector preTanComps[3]; // Pre-distortion sub-pixel locations in order green, red, blue
					preTanComps[0]=preTan*scale;
					for(int i=1;i<3;++i)
						preTanComps[i]=preTan*(scale*(*lc.distortionEquations[i])(preTanR2));
					
					/* Red, green, blue points in pre-distortion image texture: */
					for(int i=0;i<3;++i)
						{
						preTanComps[i][0]=(preTanComps[i][0]-lc.renderedFovs[0])*double(predistortionFrameSize[0])/(lc.renderedFovs[1]-lc.renderedFovs[0]);
						preTanComps[i][1]=(preTanComps[i][1]-lc.renderedFovs[2])*double(predistortionFrameSize[1])/(lc.renderedFovs[3]-lc.renderedFovs[2]);
						}
					
					/* Store the mesh vertex: */
					wmvPtr->redTex[0]=GLfloat(preTanComps[1][0]);
					wmvPtr->redTex[1]=GLfloat(preTanComps[1][1]);
					wmvPtr->greenTex[0]=GLfloat(preTanComps[0][0]);
					wmvPtr->greenTex[1]=GLfloat(preTanComps[0][1]);
					wmvPtr->blueTex[0]=GLfloat(preTanComps[2][0]);
					wmvPtr->blueTex[1]=GLfloat(preTanComps[2][1]);
					
					wmvPtr->pos[0]=GLfloat(post[0]);
					wmvPtr->pos[1]=GLfloat(post[1]);
					}
				}
			}
		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
		}
	
	/* Protect the mesh vertex buffers: */
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	
	/* Generate mesh vertex indices: */
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,warpMeshIndexBufferId);
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,(warpMeshSize[1]-1)*warpMeshSize[0]*2*sizeof(GLuint),0,GL_STATIC_DRAW_ARB);
	
	/* Map the mesh index buffer and enter indices in sequential order: */
	GLuint* wmiPtr=static_cast<GLuint*>(glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	for(int y=1;y<warpMeshSize[1];++y)
		{
		for(int x=0;x<warpMeshSize[0];++x,wmiPtr+=2)
			{
			wmiPtr[0]=y*warpMeshSize[0]+x;
			wmiPtr[1]=(y-1)*warpMeshSize[0]+x;
			}
		}
	
	glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
	
	/* Protect the mesh index buffers: */
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
	}

void LensCorrector::hmdConfigurationUpdatedCallback(const HMDConfiguration& hmdConfiguration)
	{
	/* Check which HMD configuration components were updated: */
	if(eyePosVersion!=hmdConfiguration.getEyePosVersion())
		{
		/* Hook a callback into Vrui's frame processing (will be ignored if the callback is already active): */
		addFrameCallback(frameCallback,this);
		}
	}

bool LensCorrector::frameCallback(void* userData)
	{
	LensCorrector* thisPtr=static_cast<LensCorrector*>(userData);
	
	/* Remember whether the HMD's screens have to be moved: */
	bool moveScreens=false;
	
	/* Check if the eye positions have changed: */
	if(thisPtr->eyePosVersion!=thisPtr->hmdConfiguration->getEyePosVersion())
		{
		/* Update the viewer's eye position: */
		Point leftEye=Point(thisPtr->hmdConfiguration->getEyePosition(0));
		Point rightEye=Point(thisPtr->hmdConfiguration->getEyePosition(1));
		thisPtr->viewer->setEyes(thisPtr->viewer->getDeviceViewDirection(),Geometry::mid(leftEye,rightEye),(rightEye-leftEye)*Scalar(0.5));
		moveScreens=true;
		
		/* Show the new IPD to the user: */
		Scalar newIpd=Geometry::dist(leftEye,rightEye)*getMeterFactor()*Scalar(1000);
		
		if(thisPtr->ipdDisplayDialog!=0)
			{
			/* Update the IPD display text field with the new value: */
			GLMotif::TextField* ipdDisplay=static_cast<GLMotif::TextField*>(static_cast<GLMotif::RowColumn*>(thisPtr->ipdDisplayDialog->getChild())->getChild(1));
			ipdDisplay->setValue(newIpd);
			
			/* Remember the displayed IPD: */
			thisPtr->lastShownIpd=newIpd;
			}
		else if(Math::abs(newIpd-thisPtr->lastShownIpd)>Scalar(0.6))
			{
			/* Create a dialog window to display the new inter-pupillary distance: */
			thisPtr->ipdDisplayDialog=new GLMotif::PopupWindow("IpdDisplayDialog",getWidgetManager(),"IPD Update");
			thisPtr->ipdDisplayDialog->setHideButton(false);
			
			GLMotif::RowColumn* ipdDisplayBox=new GLMotif::RowColumn("IpdDisplayBox",thisPtr->ipdDisplayDialog,false);
			ipdDisplayBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
			ipdDisplayBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
			ipdDisplayBox->setNumMinorWidgets(1);
			
			new GLMotif::Label("IpdDisplayLabel",ipdDisplayBox,"IPD");
			
			GLMotif::TextField* ipdDisplay=new GLMotif::TextField("IpdDisplay",ipdDisplayBox,6);
			ipdDisplay->setFieldWidth(5);
			ipdDisplay->setPrecision(1);
			ipdDisplay->setFloatFormat(GLMotif::TextField::FIXED);
			ipdDisplay->setValue(newIpd);
			
			ipdDisplayBox->manageChild();
			
			/* Remember the displayed IPD: */
			thisPtr->lastShownIpd=newIpd;
			}
		
		/* Let the dialog stay up for two seconds: */
		thisPtr->ipdDisplayDialogTakedownTime=getApplicationTime()+2.0;
		
		/* Pop up the dialog in the viewer's sight line: */
		Point hotspot=thisPtr->viewer->getHeadPosition()+thisPtr->viewer->getViewDirection()*(Scalar(24)*getInchFactor());
		popupPrimaryWidget(thisPtr->ipdDisplayDialog,hotspot,false);
		
		/* Mark the eye position as up-to-date: */
		thisPtr->eyePosVersion=thisPtr->hmdConfiguration->getEyePosVersion();
		}
	
	if(thisPtr->eyeVersion!=thisPtr->hmdConfiguration->getEyeVersion())
		{
		/* Update the per-eye tangent-space FoV boundaries: */
		for(int eye=0;eye<2;++eye)
			{
			LensConfig& lc=thisPtr->lensConfigs[eye];
			for(int i=0;i<4;++i)
				lc.renderedFovs[i]=double(thisPtr->hmdConfiguration->getFov(eye)[i]);
			}
		moveScreens=true;
		
		/* Mark the eye FoV as up-to-date: */
		thisPtr->eyeVersion=thisPtr->hmdConfiguration->getEyeVersion();
		}
	
	if(moveScreens)
		{
		/* Update the positions and sizes of the HMD's screens so that their calculated FoV matches the HMD's configured FoV: */
		for(int eye=0;eye<2;++eye)
			{
			VRScreen* screen=thisPtr->window->getVRScreen(eye);
			Point sEye=screen->getScreenTransformation().inverseTransform(thisPtr->viewer->getEyePosition(eye==0?Viewer::LEFT:Viewer::RIGHT));
			ONTransform screenT=screen->getTransform();
			LensConfig& lc=thisPtr->lensConfigs[eye];
			screenT*=ONTransform::translate(Vector(sEye[0]+lc.renderedFovs[0]*sEye[2],sEye[1]+lc.renderedFovs[2]*sEye[2],0));
			screen->setSize((lc.renderedFovs[1]-lc.renderedFovs[0])*sEye[2],(lc.renderedFovs[3]-lc.renderedFovs[2])*sEye[2]);
			screen->setTransform(screenT);
			}
		}
	
	if(thisPtr->ipdDisplayDialog!=0&&getApplicationTime()>=thisPtr->ipdDisplayDialogTakedownTime)
		{
		popdownPrimaryWidget(thisPtr->ipdDisplayDialog);
		delete thisPtr->ipdDisplayDialog;
		thisPtr->ipdDisplayDialog=0;
		
		/* Remove this callback: */
		return true;
		}
	else
		{
		/* Request another Vrui frame at the takedown time: */
		scheduleUpdate(thisPtr->ipdDisplayDialogTakedownTime);
		return false;
		}
	}

LensCorrector::LensCorrector(VRWindow& sWindow,const WindowProperties& windowProperties,int multisamplingLevel,const GLWindow::WindowPos viewportPos[2],const Misc::ConfigurationFileSection& configFileSection)
	:window(&sWindow),
	 displayRotation(0),viewer(0),hmdAdapter(0),hmdTrackerIndex(-1),hmdConfiguration(0),precomputed(false),
	 eyePosVersion(0U),eyeVersion(0U),distortionMeshVersion(0U),
	 ipdDisplayDialog(0),
	 predistortionMultisamplingLevel(multisamplingLevel),
	 predistortionStencilBufferSize(windowProperties.stencilBufferSize),
	 warpReproject(false),warpCubicLookup(false),correctOledResponse(false),fixContrast(true)
	{
	/* Initialize lens corrector state: */
	for(int eye=0;eye<2;++eye)
		{
		for(int i=0;i<3;++i)
			lensConfigs[eye].distortionEquations[i]=0;
		lensConfigs[eye].warpMesh=0;
		}
	
	/* Ensure that both viewports have the same size: */
	if(viewportPos[0].size[0]!=viewportPos[1].size[0]||viewportPos[0].size[1]!=viewportPos[1].size[1])
		{
		Misc::throwStdErr("Vrui::LensCorrector: Left and right viewports have different sizes, %dx%d vs %dx%d",
		                  viewportPos[0].size[0],viewportPos[0].size[1],
											viewportPos[1].size[0],viewportPos[1].size[1]);
		}
	
	/* Query the display's rotation: */
	double displayRotationAngle=configFileSection.retrieveValue<double>("./displayRotation",0.0);
	displayRotationAngle-=Math::floor(displayRotationAngle/360.0)*360.0;
	displayRotation=int(Math::floor(displayRotationAngle/90.0+0.5))%4;
	
	/* Calculate the rotated viewport size: */
	int viewportSize[2];
	if(displayRotation==0||displayRotation==2)
		{
		viewportSize[0]=viewportPos[0].size[0];
		viewportSize[1]=viewportPos[0].size[1];
		}
	else
		{
		viewportSize[0]=viewportPos[0].size[1];
		viewportSize[1]=viewportPos[0].size[0];
		}
	for(int i=0;i<2;++i)
		predistortionFrameSize[i]=viewportSize[i];
	
	/* Store shared and per-eye post-distortion viewport boundaries: */
	finalViewport[0]=0;
	finalViewport[1]=0;
	finalViewport[2]=sWindow.getWindowWidth();
	finalViewport[3]=sWindow.getWindowHeight();
	for(int eye=0;eye<2;++eye)
		{
		LensConfig& lc=lensConfigs[eye];
		lc.finalViewport[0]=viewportPos[eye].origin[0];
		lc.finalViewport[1]=viewportPos[eye].origin[1];
		lc.finalViewport[2]=viewportPos[eye].size[0];
		lc.finalViewport[3]=viewportPos[eye].size[1];
		}
	
	/* Get a pointer to the viewer responsible for both eyes: */
	if(sWindow.getViewer(0)==sWindow.getViewer(1))
		viewer=sWindow.getViewer(0);
	
	/* Try finding an HMD configuration for the viewer associated with this lens corrector: */
	if(viewer!=0&&viewer->getHeadDevice()!=0)
		{
		/* Find the input device adapter responsible for the viewer's head device: */
		const Vrui::InputDevice* headDevice=viewer->getHeadDevice();
		Vrui::InputDeviceAdapter* adapter=getInputDeviceManager()->findInputDeviceAdapter(headDevice);
		
		/* Check if the adapter is a VRDeviceDaemon adapter: */
		hmdAdapter=dynamic_cast<Vrui::InputDeviceAdapterDeviceDaemon*>(adapter);
		if(hmdAdapter!=0)
			{
			/* Find an HMD configuration associated with the head device: */
			hmdTrackerIndex=hmdAdapter->findInputDevice(headDevice);
			hmdConfiguration=hmdAdapter->findHmdConfiguration(headDevice);
			}
		}
	
	/* Check if this lens corrector uses pre-computed parameters: */
	precomputed=hmdConfiguration!=0;
	double overscan[2];
	if(precomputed)
		{
		/* Extract lens corrector parameters from the found HMD configuration: */
		Vrui::VRDeviceClient& dc=hmdAdapter->getDeviceClient();
		dc.lockHmdConfigurations();
		
		/* Update viewer's eye positions: */
		Vrui::Point leftEye=Vrui::Point(hmdConfiguration->getEyePosition(0));
		Vrui::Point rightEye=Vrui::Point(hmdConfiguration->getEyePosition(1));
		viewer->setEyes(viewer->getDeviceViewDirection(),Geometry::mid(leftEye,rightEye),(rightEye-leftEye)*Vrui::Scalar(0.5));
		
		/* Calculate overscan factors from recommended render target size: */
		for(int i=0;i<2;++i)
			overscan[i]=double(hmdConfiguration->getRenderTargetSize()[i])/double(viewportSize[i]);
		
		/* Copy size of lens distortion correction mesh: */
		for(int i=0;i<2;++i)
			warpMeshSize[i]=int(hmdConfiguration->getDistortionMeshSize()[i]);
		
		/* Read per-eye configuration parameters: */
		for(int eye=0;eye<2;++eye)
			{
			LensConfig& lc=lensConfigs[eye];
			
			/* Copy tangent-space FoV boundaries: */
			for(int i=0;i<4;++i)
				lc.renderedFovs[i]=double(hmdConfiguration->getFov(eye)[i]);
			
			/* Shift and scale the screen so that its calculated FoV matches the HMD's configured FoV: */
			VRScreen* screen=sWindow.getVRScreen(eye);
			Point sEye=screen->getScreenTransformation().inverseTransform(viewer->getEyePosition(eye==0?Viewer::LEFT:Viewer::RIGHT));
			ONTransform screenT=screen->getTransform();
			screenT*=ONTransform::translate(Vector(sEye[0]+lc.renderedFovs[0]*sEye[2],sEye[1]+lc.renderedFovs[2]*sEye[2],0));
			screen->setSize((lc.renderedFovs[1]-lc.renderedFovs[0])*sEye[2],(lc.renderedFovs[3]-lc.renderedFovs[2])*sEye[2]);
			screen->setTransform(screenT);
			
			/* Copy lens distortion correction mesh: */
			const HMDConfiguration::DistortionMeshVertex* dmPtr=hmdConfiguration->getDistortionMesh(eye);
			lc.warpMesh=new WarpMeshVertex[warpMeshSize[1]*warpMeshSize[0]];
			WarpMeshVertex* wmPtr=lc.warpMesh;
			for(int v=0;v<warpMeshSize[1];++v)
				{
				GLfloat vf=GLfloat(v)/GLfloat(warpMeshSize[1]-1);
				for(int u=0;u<warpMeshSize[0];++u,++dmPtr,++wmPtr)
					{
					GLfloat uf=GLfloat(u)/GLfloat(warpMeshSize[0]-1);
					
					/* Copy distortion-corrected per-color-component look-up positions: */
					for(int i=0;i<2;++i)
						wmPtr->redTex[i]=GLfloat(dmPtr->red[i]);
					for(int i=0;i<2;++i)
						wmPtr->greenTex[i]=GLfloat(dmPtr->green[i]);
					for(int i=0;i<2;++i)
						wmPtr->blueTex[i]=GLfloat(dmPtr->blue[i]);
					
					/* Calculate the mesh vertex position in whole-window normalized device coordinates: */
					wmPtr->pos[0]=2.0f*(GLfloat(lc.finalViewport[2])*uf+GLfloat(lc.finalViewport[0]))/GLfloat(finalViewport[2])-1.0f;
					wmPtr->pos[1]=2.0f*(GLfloat(lc.finalViewport[3])*vf+GLfloat(lc.finalViewport[1]))/GLfloat(finalViewport[3])-1.0f;
					}
				}
			}
		
		/* Remember HMD configuration version numbers: */
		eyePosVersion=hmdConfiguration->getEyePosVersion();
		eyeVersion=hmdConfiguration->getEyeVersion();
		distortionMeshVersion=hmdConfiguration->getDistortionMeshVersion();
		
		/* Install a callback with the device client to get notified on HMD configuration changes: */
		dc.setHmdConfigurationUpdatedCallback(hmdTrackerIndex,Misc::createFunctionCall(this,&LensCorrector::hmdConfigurationUpdatedCallback));
		
		dc.unlockHmdConfigurations();
		}
	else
		{
		/* Common lens configuration defaults: */
		double lensCenterDist=configFileSection.retrieveValue<double>("./lensCenterDist",Vrui::getInchFactor()*2.5);
		double lensFocalLength=configFileSection.retrieveValue<double>("./lensFocalLength",Vrui::getInchFactor()*2.5);
		double lensScreenDist=configFileSection.retrieveValue<double>("./lensScreenDist",Math::mid(sWindow.getVRScreen(0)->getWidth(),sWindow.getVRScreen(1)->getWidth()));
		
		double lensProjectionDists[2];
		lensProjectionDists[0]=lensProjectionDists[1]=configFileSection.retrieveValue<double>("./lensProjectionDist",0.0);
		
		/* Left eye configuration: */
		lensConfigs[0].lensCenter=Point(sWindow.getVRScreen(0)->getWidth()-lensCenterDist*0.5,sWindow.getVRScreen(0)->getHeight()*0.5,lensScreenDist);
		lensConfigs[0].lensCenter=configFileSection.retrieveValue<Point>("./leftLensCenter",lensConfigs[0].lensCenter);
		lensConfigs[0].focalLength=configFileSection.retrieveValue<double>("./leftLensFocalLength",lensFocalLength);
		
		lensProjectionDists[0]=configFileSection.retrieveValue<double>("./leftLensProjectionDist",lensProjectionDists[0]);
		
		Misc::FixedArray<std::string,3> leftFormulaNames=configFileSection.retrieveValue<Misc::FixedArray<std::string,3> >("./leftFormulaNames");
		for(int i=0;i<3;++i)
			lensConfigs[0].distortionEquations[i]=parseDistortionEquation(configFileSection.getSection(leftFormulaNames[i].c_str()));
		
		Misc::FixedArray<double,4> leftOverscan(0.0);
		leftOverscan=configFileSection.retrieveValue<Misc::FixedArray<double,4> >("./leftOverscan",leftOverscan);
		for(int i=0;i<4;++i)
			lensConfigs[0].overscan[i]=leftOverscan[i];
		
		/* Right eye configuration: */
		lensConfigs[1].lensCenter=Point(lensCenterDist*0.5,sWindow.getVRScreen(1)->getHeight()*0.5,lensScreenDist);
		lensConfigs[1].lensCenter=configFileSection.retrieveValue<Point>("./rightLensCenter",lensConfigs[1].lensCenter);
		lensConfigs[1].focalLength=configFileSection.retrieveValue<double>("./rightLensFocalLength",lensFocalLength);
		
		lensProjectionDists[1]=configFileSection.retrieveValue<double>("./rightLensProjectionDist",lensProjectionDists[1]);
		
		Misc::FixedArray<std::string,3> rightFormulaNames=configFileSection.retrieveValue<Misc::FixedArray<std::string,3> >("./rightFormulaNames");
		for(int i=0;i<3;++i)
			lensConfigs[1].distortionEquations[i]=parseDistortionEquation(configFileSection.getSection(rightFormulaNames[i].c_str()));
		
		Misc::FixedArray<double,4> rightOverscan(0.0);
		rightOverscan=configFileSection.retrieveValue<Misc::FixedArray<double,4> >("./rightOverscan",rightOverscan);
		for(int i=0;i<4;++i)
			lensConfigs[1].overscan[i]=rightOverscan[i];
		
		/* Calculate derived lens configuration: */
		bool projectScreens=configFileSection.retrieveValue<bool>("./projectScreens",false);
		for(int eye=0;eye<2;++eye)
			{
			VRScreen* screen=sWindow.getVRScreen(eye);
			LensConfig& lc=lensConfigs[eye];
			
			/* Calculate total overscan sizes: */
			for(int i=0;i<2;++i)
				lc.overscanSize[i]=1.0+lc.overscan[2*i+0]+lc.overscan[2*i+1];
			
			if(projectScreens)
				{
				/* Calculate a displacement vector for the screen's origin point: */
				double scale=lensProjectionDists[eye]/lc.lensCenter[2];
				Vector delta=(lc.lensCenter-Point::origin)*(1.0-scale);
				
				/* Calculate the new screen transformation: */
				ONTransform newTransform=screen->getTransform();
				newTransform*=ONTransform::translate(delta);
				newTransform.renormalize();
				
				/* Calculate new screen width and height: */
				Scalar newWidth=screen->getWidth()*scale;
				Scalar newHeight=screen->getHeight()*scale;
				
				/* Override the real screen with its projected virtual version: */
				screen->setSize(newWidth,newHeight);
				screen->setTransform(newTransform);
				
				/* Update the screen-coordinate lens center positions: */
				for(int i=0;i<2;++i)
					lc.lensCenter[i]*=scale;
				lc.lensCenter[2]=lensProjectionDists[eye];
				
				#if DEBUG_REPROJECTION
				std::cout<<"New screen origin: "<<screen->getTransform().getOrigin()<<std::endl;
				std::cout<<"New screen size: "<<screen->getWidth()<<" x "<<screen->getHeight()<<std::endl;
				std::cout<<"New screen axes: "<<screen->getTransform().getDirection(0)<<", "<<screen->getTransform().getDirection(1)<<std::endl;
				#endif
				}
			}
		
		/* Calculate average overscan size to construct identical frame buffers for left and right eye: */
		for(int i=0;i<2;++i)
			overscan[i]=Math::mid(lensConfigs[0].overscanSize[i],lensConfigs[1].overscanSize[i]);
		
		/* Query the size of the left and right warping meshes: */
		Misc::FixedArray<int,2> wms(64);
		wms=configFileSection.retrieveValue<Misc::FixedArray<int,2> >("./warpMeshSize",wms);
		for(int i=0;i<2;++i)
			warpMeshSize[i]=wms[i]+1;
		}
	
	/* Initialize IPD update display: */
	lastShownIpd=Geometry::dist(sWindow.getViewer(0)->getDeviceEyePosition(Viewer::LEFT),sWindow.getViewer(1)->getDeviceEyePosition(Viewer::RIGHT))*getMeterFactor()*Scalar(1000);
	
	/* Calculate final pre-distortion frame buffer size with supersampling: */
	double superSampling=configFileSection.retrieveValue<double>("./superSampling",1.0);
	for(int i=0;i<2;++i)
		predistortionFrameSize[i]=int(Math::floor(double(predistortionFrameSize[i])*superSampling*overscan[i]+0.5));
	if(vruiVerbose)
		{
		std::cout<<"\tLens correction supersampling factor: "<<superSampling<<std::endl;
		std::cout<<"\tPre-distortion frame buffer size per eye: "<<predistortionFrameSize[0]<<" x "<<predistortionFrameSize[1]<<std::endl;
		}
	
	/* Retrieve reprojection flag: */
	warpReproject=viewer!=0&&configFileSection.retrieveValue<bool>("./warpReproject",warpReproject);
	if(vruiVerbose)
		std::cout<<"\tReprojection "<<(warpReproject?"enabled":"disabled")<<std::endl;
	
	/* Retrieve cubic look-up flag: */
	warpCubicLookup=configFileSection.retrieveValue<bool>("./warpCubicLookup",warpCubicLookup);
	
	/* Initialize the required OpenGL extensions: */
	GLARBMultitexture::initExtension();
	GLEXTFramebufferObject::initExtension();
	if(predistortionStencilBufferSize>0)
		GLEXTPackedDepthStencil::initExtension();
	if(predistortionMultisamplingLevel>1)
		{
		GLEXTFramebufferBlit::initExtension();
		GLEXTFramebufferMultisample::initExtension();
		}
	GLARBVertexBufferObject::initExtension();
	GLShader::initExtensions();
	
	/* Create the pre-distortion rendering frame buffer: */
	glGenFramebuffersEXT(1,&predistortionFrameBufferId);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,predistortionFrameBufferId);
	
	/* Create the pre-distortion color image texture: */
	glGenTextures(2,predistortionColorBufferIds);
	for(int eye=0;eye<2;++eye)
		{
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,predistortionColorBufferIds[eye]);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MAX_LEVEL,0);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MIN_FILTER,warpCubicLookup?GL_NEAREST:GL_LINEAR);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MAG_FILTER,warpCubicLookup?GL_NEAREST:GL_LINEAR);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
		GLfloat texBorderColor[4]={0.0f,0.0f,0.0f,1.0f};
		glTexParameterfv(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_BORDER_COLOR,texBorderColor);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_RGB8,predistortionFrameSize[0],predistortionFrameSize[1],0,GL_RGB,GL_UNSIGNED_BYTE,0);
		}
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
	
	if(predistortionMultisamplingLevel>1)
		{
		/* Create the pre-distortion multisampling color buffer: */
		glGenRenderbuffersEXT(1,&predistortionMultisamplingColorBufferId);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,predistortionMultisamplingColorBufferId);
		glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT,predistortionMultisamplingLevel,GL_RGB8,predistortionFrameSize[0],predistortionFrameSize[1]);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
		
		/* Attach the pre-distortion multisampling color buffer to the frame buffer: */
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_RENDERBUFFER_EXT,predistortionMultisamplingColorBufferId);
		}
	else
		{
		/* Attach the pre-distortion color image textures to the frame buffer: */
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_RECTANGLE_ARB,predistortionColorBufferIds[0],0);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT1_EXT,GL_TEXTURE_RECTANGLE_ARB,predistortionColorBufferIds[1],0);
		}
	
	/* Create the pre-distortion depth buffer: */
	if(predistortionStencilBufferSize>0)
		{
		/* Create an interleaved depth+stencil render buffer: */
		if(predistortionStencilBufferSize>8)
			Misc::throwStdErr("Vrui::LensCorrector: Lens distortion correction not supported with stencil depth %d>8",int(predistortionStencilBufferSize));
		glGenRenderbuffersEXT(1,&predistortionDepthStencilBufferId);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,predistortionDepthStencilBufferId);
		if(predistortionMultisamplingLevel>1)
			glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT,predistortionMultisamplingLevel,GL_DEPTH24_STENCIL8_EXT,predistortionFrameSize[0],predistortionFrameSize[1]);
		else
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH24_STENCIL8_EXT,predistortionFrameSize[0],predistortionFrameSize[1]);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
		
		/* Attach the pre-distortion interleaved depth and stencil buffer to the frame buffer: */
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,predistortionDepthStencilBufferId);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_STENCIL_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,predistortionDepthStencilBufferId);
		}
	else
		{
		/* Create a depth-only render buffer: */
		glGenRenderbuffersEXT(1,&predistortionDepthStencilBufferId);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,predistortionDepthStencilBufferId);
		if(predistortionMultisamplingLevel>1)
			glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT,predistortionMultisamplingLevel,GL_DEPTH_COMPONENT,predistortionFrameSize[0],predistortionFrameSize[1]);
		else
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH_COMPONENT,predistortionFrameSize[0],predistortionFrameSize[1]);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
		
		/* Attach the pre-distortion depth buffer to the frame buffer: */
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,predistortionDepthStencilBufferId);
		}
	
	/* Set up pixel sources and destinations: */
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	
	/* Check the status of the lens correction frame buffer: */
	glThrowFramebufferStatusExceptionEXT("Vrui::LensCorrector: Lens correction framebuffer incomplete due to");
	
	if(predistortionMultisamplingLevel>1)
		{
		/* Create the multisample "fixing" frame buffer: */
		glGenFramebuffersEXT(1,&multisamplingFrameBufferId);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,multisamplingFrameBufferId);
		
		/* Attach the pre-distortion color image textures to the "fixing" frame buffer: */
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_RECTANGLE_ARB,predistortionColorBufferIds[0],0);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT1_EXT,GL_TEXTURE_RECTANGLE_ARB,predistortionColorBufferIds[1],0);
		
		/* Check the status of the multisample "fixing" frame buffer: */
		glThrowFramebufferStatusExceptionEXT("Vrui::LensCorrector: Multisampling framebuffer incomplete due to");
		}
	
	/* Protect the created frame buffer(s): */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	
	/* Generate the warp mesh buffers: */
	glGenBuffersARB(2,warpMeshVertexBufferIds);
	glGenBuffersARB(1,&warpMeshIndexBufferId);
	
	/* Calculate and upload the warp meshes: */
	if(!precomputed)
		calculateWarpParameters(sWindow);
	uploadWarpMeshes();
	
	/* Check for OLED response time correction factors: */
	Misc::FixedArray<double,2> ocf(0.0);
	ocf=configFileSection.retrieveValue<Misc::FixedArray<double,2> >("./oledCorrectionFactors",ocf);
	correctOledResponse=ocf[0]!=0.0||ocf[1]!=0.0;
	if(correctOledResponse)
		{
		/* Set the correction factors: */
		for(int i=0;i<2;++i)
			oledCorrectionFactors[i]=GLfloat(ocf[i]);
		
		fixContrast=configFileSection.retrieveValue<bool>("./fixContrast",fixContrast);
		if(fixContrast)
			{
			/* Calculate the OLED contrast correction coefficients: */
			oledContrast[1]=oledCorrectionFactors[0]/(1.0f+oledCorrectionFactors[0]); // Offset
			oledContrast[0]=1.0f/(1.0f+oledCorrectionFactors[1])-oledContrast[1]; // Scale
			}
		
		/* Create the texture holding the previously rendered frame: */
		glGenTextures(1,&previousFrameTextureId);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,previousFrameTextureId);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MAX_LEVEL,0);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_WRAP_S,GL_CLAMP);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,GL_TEXTURE_WRAP_T,GL_CLAMP);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_RGB8,finalViewport[2],finalViewport[3],0,GL_RGB,GL_UNSIGNED_BYTE,0);
		
		/* Protect the texture object: */
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
		}
	
	/* Construct the lens distortion correction vertex shader: */
	std::string warpingShaderVertexProgramDeclarations="\
		attribute vec2 redTexIn;\n\
		attribute vec2 greenTexIn;\n\
		attribute vec2 blueTexIn;\n\
		\n\
		varying vec2 redTex;\n\
		varying vec2 greenTex;\n\
		varying vec2 blueTex;\n\
		\n";
	
	std::string warpingShaderVertexProgramMain="\
		void main()\n\
			{\n";
	
	if(warpReproject)
		{
		warpingShaderVertexProgramDeclarations.append("\
			uniform vec2 fovScale,fovOffset,invFovScale;\n\
			uniform mat3 rotation;\n\
			\n");
		
		warpingShaderVertexProgramMain.append("\
			/* Transform the per-component corrected pixel positions from pixel space to tangent space: */\n\
			vec3 red=vec3(redTexIn*fovScale+fovOffset,-1);\n\
			vec3 green=vec3(greenTexIn*fovScale+fovOffset,-1);\n\
			vec3 blue=vec3(blueTexIn*fovScale+fovOffset,-1);\n\
			\n\
			/* Rotate the tangent-space positions: */\n\
			red=rotation*red;\n\
			green=rotation*green;\n\
			blue=rotation*blue;\n\
			\n\
			/* Project the rotated positions back into tangent space: */\n\
			red*=-1.0/red.z;\n\
			green*=-1.0/green.z;\n\
			blue*=-1.0/blue.z;\n\
			\n\
			/* Transform the projected positions back into pixel space: */\n\
			redTex=(red.xy-fovOffset)*invFovScale;\n\
			greenTex=(green.xy-fovOffset)*invFovScale;\n\
			blueTex=(blue.xy-fovOffset)*invFovScale;\n\
			\n");
		}
	else
		{
		warpingShaderVertexProgramMain.append("\
			redTex=redTexIn;\n\
			greenTex=greenTexIn;\n\
			blueTex=blueTexIn;\n\
			\n");
		}
	
	warpingShaderVertexProgramMain.append("\
		gl_Position=gl_Vertex;\n\
		}\n");
	
	
	/* Compile the warping shader's fragment program: */
	warpingShader.compileVertexShaderFromString((warpingShaderVertexProgramDeclarations+warpingShaderVertexProgramMain).c_str());
	
	/* Construct the lens distortion correction fragment shader: */
	std::string warpingShaderFragmentProgramDeclarations="\
		#extension GL_ARB_texture_rectangle : enable\n\
		\n\
		varying vec2 redTex;\n\
		varying vec2 greenTex;\n\
		varying vec2 blueTex;\n\
		\n\
		uniform sampler2DRect predistortionImageSampler;\n";
	
	std::string warpingShaderFragmentProgramBilinearMain="\
		\n\
		void main()\n\
			{\n\
			/* Get the pixel color's red, green, and blue components via their individual texture coordinates: */\n\
			float red=texture2DRect(predistortionImageSampler,redTex).r;\n\
			float green=texture2DRect(predistortionImageSampler,greenTex).g;\n\
			float blue=texture2DRect(predistortionImageSampler,blueTex).b;\n\
			vec4 newColor=vec4(red,green,blue,1.0);\n\
			\n";
	
	std::string warpingShaderFragmentProgramBicubicMain="\
		\n\
		vec4 sample(in vec2 p)\n\
			{\n\
			vec2 sp0=floor(p+0.5)-1.5;\n\
			vec2 d=sp0-p;\n\
			vec2 w[4];\n\
			w[0]=((0.5*d+2.5)*d+4.0)*d+2.0;\n\
			w[1]=((-1.5*d-7.0)*d-9.5)*d-3.0;\n\
			w[2]=((1.5*d+6.5)*d+8.0)*d+3.0;\n\
			w[3]=((-0.5*d-2.0)*d-2.5)*d-1.0;\n\
			vec4 result=vec4(0.0);\n\
			for(int y=0;y<4;++y)\n\
				{\n\
				vec4 xsum=vec4(0.0);\n\
				for(int x=0;x<4;++x)\n\
					xsum+=texture2DRect(predistortionImageSampler,sp0+vec2(x,y))*w[x].x;\n\
				result+=xsum*w[y].y;\n\
				}\n\
			return result;\n\
			}\n\
		void main()\n\
			{\n\
			/* Get the pixel color's red, green, and blue components via their individual texture coordinates: */\n\
			float red=sample(redTex).r;\n\
			float green=sample(greenTex).g;\n\
			float blue=sample(blueTex).b;\n\
			vec4 newColor=vec4(red,green,blue,1.0);\n\
			\n";
	
	std::string warpingShaderFragmentProgramMain=warpCubicLookup?warpingShaderFragmentProgramBicubicMain:warpingShaderFragmentProgramBilinearMain;
	
	if(correctOledResponse)
		{
		warpingShaderFragmentProgramDeclarations.append("\
			uniform sampler2DRect previousFrameImageSampler;\n\
			uniform float overdrive[2];\n");
		
		if(fixContrast)
			{
			warpingShaderFragmentProgramDeclarations.append("\
				uniform float contrast[2];\n");
			
			warpingShaderFragmentProgramMain.append("\
				/* Reduce contrast in the pixel's color to give room for OLED response correction: */\n\
				newColor=newColor*contrast[0]+vec4(contrast[1]);\n\
				\n");
			}
		
		warpingShaderFragmentProgramMain.append("\
			/* Get the previous frame's color for the same pixel: */\n\
			vec4 previousColor=texture2DRect(previousFrameImageSampler,gl_FragCoord.xy);\n\
			if(newColor.r>=previousColor.r)\n\
				newColor.r=newColor.r+(newColor.r-previousColor.r)*overdrive[0];\n\
			else\n\
				newColor.r=newColor.r+(newColor.r-previousColor.r)*overdrive[1];\n\
			if(newColor.g>=previousColor.g)\n\
				newColor.g=newColor.g+(newColor.g-previousColor.g)*overdrive[0];\n\
			else\n\
				newColor.g=newColor.g+(newColor.g-previousColor.g)*overdrive[1];\n\
			if(newColor.b>=previousColor.b)\n\
				newColor.b=newColor.b+(newColor.b-previousColor.b)*overdrive[0];\n\
			else\n\
				newColor.b=newColor.b+(newColor.b-previousColor.b)*overdrive[1];\n\
			\n");
		}
	
	warpingShaderFragmentProgramMain.append("\
		gl_FragColor=newColor;\n\
		}\n");
	
	/* Compile the warping shader's fragment program: */
	warpingShader.compileFragmentShaderFromString((warpingShaderFragmentProgramDeclarations+warpingShaderFragmentProgramMain).c_str());
	
	/* Link the shader and query its attribute and uniform locations: */
	warpingShader.linkShader();
	warpingShaderAttributeIndices[0]=warpingShader.getAttribLocation("redTexIn");
	warpingShaderAttributeIndices[1]=warpingShader.getAttribLocation("greenTexIn");
	warpingShaderAttributeIndices[2]=warpingShader.getAttribLocation("blueTexIn");
	warpingShaderUniformIndices[0]=warpingShader.getUniformLocation("predistortionImageSampler");
	if(correctOledResponse)
		{
		warpingShaderUniformIndices[1]=warpingShader.getUniformLocation("previousFrameImageSampler");
		warpingShaderUniformIndices[2]=warpingShader.getUniformLocation("overdrive");
		if(fixContrast)
			warpingShaderUniformIndices[3]=warpingShader.getUniformLocation("contrast");
		}
	if(warpReproject)
		{
		warpingShaderUniformIndices[4]=warpingShader.getUniformLocation("fovScale");
		warpingShaderUniformIndices[5]=warpingShader.getUniformLocation("fovOffset");
		warpingShaderUniformIndices[6]=warpingShader.getUniformLocation("invFovScale");
		warpingShaderUniformIndices[7]=warpingShader.getUniformLocation("rotation");
		}
	}

LensCorrector::~LensCorrector(void)
	{
	if(precomputed)
		{
		/* Release HMD configuration update callback: */
		hmdAdapter->getDeviceClient().setHmdConfigurationUpdatedCallback(hmdTrackerIndex,0);
		}
	
	/* Clean up lens configurations: */
	for(int eye=0;eye<2;++eye)
		{
		for(int i=0;i<3;++i)
			delete lensConfigs[eye].distortionEquations[i];
		delete[] lensConfigs[eye].warpMesh;
		}
	
	/* Release all allocated OpenGL resources: */
	glDeleteFramebuffersEXT(1,&predistortionFrameBufferId);
	glDeleteTextures(2,predistortionColorBufferIds);
	if(predistortionMultisamplingLevel>1)
		glDeleteRenderbuffersEXT(1,&predistortionMultisamplingColorBufferId);
	glDeleteRenderbuffersEXT(1,&predistortionDepthStencilBufferId);
	if(predistortionMultisamplingLevel>1)
		glDeleteFramebuffersEXT(1,&multisamplingFrameBufferId);
	glDeleteBuffersARB(2,warpMeshVertexBufferIds);
	glDeleteBuffersARB(1,&warpMeshIndexBufferId);
	if(correctOledResponse)
		glDeleteTextures(1,&previousFrameTextureId);
	}

void LensCorrector::updateViewerState(VRWindow& window,const GLWindow::WindowPos viewportPos[2])
	{
	/* Update lens correction coefficients: */
	
	/* Get the viewer used for rendering both eyes: */
	if(window.getViewer(0)==window.getViewer(1))
		viewer=window.getViewer(0);
	else
		viewer=0;
	
	/* Mark the warp meshes as outdated: */
	}

void LensCorrector::prepare(int eye,DisplayState& displayState) const
	{
	/* Bind the pre-distortion frame buffer: */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,predistortionFrameBufferId);
	if(predistortionMultisamplingLevel>1)
		{
		/* Draw into the multisampling image buffer: */
		glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
		}
	else
		{
		/* Draw directly into the left and right color image buffers: */
		glReadBuffer(GL_COLOR_ATTACHMENT0_EXT+eye);
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT+eye);
		}
	
	/* Override the viewport position and size: */
	glViewport(0,0,predistortionFrameSize[0],predistortionFrameSize[1]);
	displayState.viewport[0]=displayState.viewport[1]=0;
	displayState.viewport[2]=predistortionFrameSize[0];
	displayState.viewport[3]=predistortionFrameSize[1];
	for(int i=0;i<2;++i)
		displayState.frameSize[i]=predistortionFrameSize[i];
	}

void LensCorrector::adjustProjection(int eye,const Point& screenEyePos,double near,double& left,double& right,double& bottom,double& top) const
	{
	const LensConfig& lc=lensConfigs[eye];
	#if 0
	/* Adjust the projection matrix for overscan: */
	double w=right-left;
	left-=w*double(lc.overscan[0]);
	right+=w*double(lc.overscan[1]);
	double h=top-bottom;
	bottom-=h*double(lc.overscan[2]);
	top+=h*double(lc.overscan[3]);
	#else
	/* Replace projection matrix with lens-adjusted FoV values: */
	left=lc.renderedFovs[0]*near;
	right=lc.renderedFovs[1]*near;
	bottom=lc.renderedFovs[2]*near;
	top=lc.renderedFovs[3]*near;
	#endif
	}

void LensCorrector::finish(int eye) const
	{
	if(predistortionMultisamplingLevel>1)
		{
		/* Blit the multisampling color buffer containing the pre-distortion image into the "fixing" frame buffer: */
		glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT,multisamplingFrameBufferId);
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT+eye);
		glBlitFramebufferEXT(0,0,predistortionFrameSize[0],predistortionFrameSize[1],0,0,predistortionFrameSize[0],predistortionFrameSize[1],GL_COLOR_BUFFER_BIT,GL_NEAREST);
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT,0);
		}
	}

void LensCorrector::warp(void) const
	{
	/* Bind the final drawable's frame buffer: */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	glViewport(finalViewport[0],finalViewport[1],finalViewport[2],finalViewport[3]);
	
	/* Set up the warping mesh buffer structure: */
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,warpMeshIndexBufferId);
	for(int i=0;i<3;++i)
		glEnableVertexAttribArrayARB(warpingShaderAttributeIndices[i]);
	glEnableClientState(GL_VERTEX_ARRAY);
	
	/* Set up the warping shader: */
	warpingShader.useProgram();
	Geometry::Matrix<GLfloat,3,3> rotation;
	if(warpReproject)
		{
		/* Get the viewer's per-frame and up-to-date viewing transformations: */
		TrackerState viewerTrans0=viewer->getHeadTransformation();
		TrackerState viewerTrans1=viewer->peekHeadTransformation();
		
		/* Calculate the incremental reprojection rotation: */
		Rotation rot=Geometry::invert(viewerTrans0.getRotation())*viewerTrans1.getRotation();
		
		// DEBUGGING
		if(lensCorrectorDisableReproject)
			rot=Rotation::identity;
		
		rot.writeMatrix(rotation);
		
		#if DEBUG_REPROJECTION
		if(Math::abs(rot.getScaledAxis().sqr())>=Math::sqr(0.01))
			std::cout<<"Reprojection rotation: "<<rot<<std::endl;
		#endif
		}
	
	if(correctOledResponse)
		{
		/* Bind the previous frame's image texture: */
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,previousFrameTextureId);
		glUniformARB(warpingShaderUniformIndices[1],1);
		
		/* Set the up- and down-scaling factors: */
		glUniformARB<1>(warpingShaderUniformIndices[2],2,oledCorrectionFactors);
		
		if(fixContrast)
			{
			/* Set the contrast reduction coefficients: */
			glUniformARB<1>(warpingShaderUniformIndices[3],2,oledContrast);
			}
		}
	
	/* Render the right and left warping meshes: */
	for(int eye=1;eye>=0;--eye)
		{
		/* Bind the pre-distortion color image texture: */
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,predistortionColorBufferIds[eye]);
		glUniformARB(warpingShaderUniformIndices[0],0);
		
		/* Bind the vertex buffer: */
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,warpMeshVertexBufferIds[eye]);
		WarpMeshVertex* vertices=0;
		glVertexAttribPointerARB(warpingShaderAttributeIndices[0],2,GL_FLOAT,GL_FALSE,sizeof(WarpMeshVertex),&vertices[0].redTex);
		glVertexAttribPointerARB(warpingShaderAttributeIndices[1],2,GL_FLOAT,GL_FALSE,sizeof(WarpMeshVertex),&vertices[0].greenTex);
		glVertexAttribPointerARB(warpingShaderAttributeIndices[2],2,GL_FLOAT,GL_FALSE,sizeof(WarpMeshVertex),&vertices[0].blueTex);
		glVertexPointer(2,GL_FLOAT,sizeof(WarpMeshVertex),&vertices[0].pos);
		
		if(warpReproject)
			{
			/* Upload the transformations from and to tangent space: */
			const LensConfig& lc=lensConfigs[eye];
			
			/* Scale from pixel space to tangent space: */
			GLfloat fovScale[2];
			fovScale[0]=GLfloat((lc.renderedFovs[1]-lc.renderedFovs[0])/double(predistortionFrameSize[0]));
			fovScale[1]=GLfloat((lc.renderedFovs[3]-lc.renderedFovs[2])/double(predistortionFrameSize[1]));
			glUniformARB<2>(warpingShaderUniformIndices[4],1,fovScale);
			
			/* Offset from pixel space to tangent space: */
			GLfloat fovOffset[2];
			fovOffset[0]=GLfloat(lc.renderedFovs[0]);
			fovOffset[1]=GLfloat(lc.renderedFovs[2]);
			glUniformARB<2>(warpingShaderUniformIndices[5],1,fovOffset);
			
			/* Scale from tangent space to pixel space: */
			GLfloat invFovScale[2];
			invFovScale[0]=GLfloat(double(predistortionFrameSize[0])/(lc.renderedFovs[1]-lc.renderedFovs[0]));
			invFovScale[1]=GLfloat(double(predistortionFrameSize[1])/(lc.renderedFovs[3]-lc.renderedFovs[2]));
			glUniformARB<2>(warpingShaderUniformIndices[6],1,invFovScale);
			
			/* Upload the reprojection rotation: */
			glUniformMatrix3fvARB(warpingShaderUniformIndices[7],1,GL_TRUE,rotation.getEntries());
			}
		
		/* Render the mesh as a sequence of quad strips: */
		const GLuint* indexPtr=0;
		for(int y=1;y<warpMeshSize[1];++y,indexPtr+=warpMeshSize[0]*2)
			glDrawElements(GL_QUAD_STRIP,warpMeshSize[0]*2,GL_UNSIGNED_INT,indexPtr);
		}
	
	if(correctOledResponse)
		{
		/* Protect the previous frame's image texture: */
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
		}
	
	/* Protect the color image texture: */
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
	
	/* Disable vertex arrays: */
	glDisableClientState(GL_VERTEX_ARRAY);
	for(int i=0;i<3;++i)
		glDisableVertexAttribArrayARB(warpingShaderAttributeIndices[i]);
	
	/* Protect the mesh buffers: */
	glBindBufferARB(GL_ARRAY_BUFFER_ARB,0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,0);
	
	/* Disable the warping shader: */
	GLShader::disablePrograms();
	
	if(correctOledResponse)
		{
		/* Copy the final rendered buffer into a texture for the next frame: */
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,previousFrameTextureId);
		glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,finalViewport[0],finalViewport[1],finalViewport[2],finalViewport[3]);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
		}
	}

}
