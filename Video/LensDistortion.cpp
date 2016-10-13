/***********************************************************************
LensDistortion - Class encapsulating a good lens distortion correction
formula also used by OpenCV.
Copyright (c) 2015 Oliver Kreylos

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

#include <Video/LensDistortion.h>

#include <Misc/SizedTypes.h>
#include <IO/File.h>
#include <Math/Math.h>

namespace Video {

/*******************************
Methods of class LensDistortion:
*******************************/

LensDistortion::LensDistortion(void)
	:center(Point::origin),
	 undistortMaxError(1.0e-32),undistortMaxSteps(20) // These are ridiculous values
	{
	for(int i=0;i<3;++i)
		kappas[i]=Scalar(0);
	for(int i=0;i<2;++i)
		rhos[i]=Scalar(0);
	}

LensDistortion::LensDistortion(IO::File& file,bool legacyFormat)
	:undistortMaxError(1.0e-32),undistortMaxSteps(20) // These are ridiculous values
	{
	/* Read lens distortion correction parameters from file: */
	read(file,legacyFormat);
	}

void LensDistortion::write(IO::File& file) const
	{
	/* Write center point: */
	for(int i=0;i<2;++i)
		file.write<Misc::Float64>(Misc::Float64(center[i]));
	
	/* Write radial distortion coefficients: */
	for(int i=0;i<3;++i)
		file.write<Misc::Float64>(Misc::Float64(kappas[i]));
	
	/* Write tangential distortion coefficients: */
	for(int i=0;i<2;++i)
		file.write<Misc::Float64>(Misc::Float64(rhos[i]));
	}

void LensDistortion::setCenter(const LensDistortion::Point& newCenter)
	{
	center=newCenter;
	}

void LensDistortion::setKappas(const LensDistortion::Scalar newKappas[3])
	{
	for(int i=0;i<3;++i)
		kappas[i]=newKappas[i];
	}

void LensDistortion::setKappa(int index,LensDistortion::Scalar newKappa)
	{
	kappas[index]=newKappa;
	}

void LensDistortion::setRhos(const LensDistortion::Scalar newRhos[2])
	{
	for(int i=0;i<2;++i)
		rhos[i]=newRhos[i];
	}

void LensDistortion::setRho(int index,LensDistortion::Scalar newRho)
	{
	rhos[index]=newRho;
	}

void LensDistortion::read(IO::File& file,bool legacyFormat)
	{
	/* Read center point: */
	for(int i=0;i<2;++i)
		center[i]=Scalar(file.read<Misc::Float64>());
	
	Scalar radiusScale2(1);
	if(legacyFormat)
		{
		/* Read radius scaling factor: */
		radiusScale2=Scalar(file.read<Misc::Float64>());
		}
	
	/* Read radial distortion coefficients: */
	for(int i=0;i<3;++i)
		kappas[i]=Scalar(file.read<Misc::Float64>());
	
	/* Read tangential distortion coefficients: */
	for(int i=0;i<2;++i)
		rhos[i]=Scalar(file.read<Misc::Float64>());
	
	if(legacyFormat)
		{
		/* Bake the radius scaling factor into the radial and tangential distortion coefficients: */
		kappas[0]*=radiusScale2;
		kappas[1]*=radiusScale2*radiusScale2;
		kappas[2]*=radiusScale2*radiusScale2*radiusScale2;
		Scalar radiusScale=Math::sqrt(radiusScale2);
		rhos[0]*=radiusScale;
		rhos[1]*=radiusScale;
		}
	}

void LensDistortion::setUndistortMaxError(LensDistortion::Scalar newUndistortMaxError)
	{
	undistortMaxError=newUndistortMaxError;
	}

void LensDistortion::setUndistortMaxSteps(int newUndistortMaxSteps)
	{
	undistortMaxSteps=newUndistortMaxSteps;
	}

LensDistortion::Point LensDistortion::undistort(const LensDistortion::Point& distorted) const
	{
	/*******************************************************************
	Must run two-dimensional Newton-Raphson iteration to invert the full
	distortion correction formula.
	*******************************************************************/
	
	/* Start iteration at the distorted point: */
	Point p=distorted;
	for(int iteration=0;iteration<undistortMaxSteps;++iteration)
		{
		/* Calculate the function value at p: */
		Vector d=p-center;
		Scalar r2=d.sqr();
		Scalar div=Scalar(1)+(kappas[0]+(kappas[1]+kappas[2]*r2)*r2)*r2;
		Point fp;
		fp[0]=center[0]+d[0]/div+Scalar(2)*rhos[0]*d[0]*d[1]+rhos[1]*(r2+Scalar(2)*d[0]*d[0])-distorted[0];
		fp[1]=center[1]+d[1]/div+rhos[0]*(r2+Scalar(2)*d[1]*d[1])+Scalar(2)*rhos[1]*d[0]*d[1]-distorted[1];
		
		/* Bail out if close enough: */
		if(fp.sqr()<undistortMaxError)
			break;
		
		/* Calculate the function derivative at p: */
		Scalar fpd[2][2];
		fpd[0][0]=(div-d[0]*(Scalar(2)*kappas[0]+(Scalar(4)*kappas[1]+Scalar(6)*kappas[2]*r2)*r2)*d[0])/Math::sqr(div)+Scalar(2)*rhos[0]*d[1]+Scalar(6)*rhos[1]*d[0]; // d fp[0] / d p[0]
		fpd[0][1]=-d[0]*(Scalar(2)*kappas[0]+(Scalar(4)*kappas[1]+Scalar(6)*kappas[2]*r2)*r2)*d[1]/Math::sqr(div)+Scalar(2)*rhos[0]*d[0]+Scalar(2)*rhos[1]*d[1]; // d fp[0] / d p[1]
		fpd[1][0]=-d[1]*(Scalar(2)*kappas[0]+(Scalar(4)*kappas[1]+Scalar(6)*kappas[2]*r2)*r2)*d[0]/Math::sqr(div)+Scalar(2)*rhos[0]*d[0]+Scalar(2)*rhos[1]*d[1]; // d fp[1] / d p[0]
		fpd[1][1]=(div-d[1]*(Scalar(2)*kappas[0]+(Scalar(4)*kappas[1]+Scalar(6)*kappas[2]*r2)*r2)*d[1])/Math::sqr(div)+Scalar(2)*rhos[1]*d[0]+Scalar(6)*rhos[0]*d[1]; // d fp[0] / d p[0]
		
		/* Perform the Newton-Raphson step: */
		Scalar det=fpd[0][0]*fpd[1][1]-fpd[0][1]*fpd[1][0];
		p[0]-=(fpd[1][1]*fp[0]-fpd[0][1]*fp[1])/det;
		p[1]-=(fpd[0][0]*fp[1]-fpd[1][0]*fp[0])/det;
		}
	
	return p;
	}

}
