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

#ifndef VIDEO_LENSDISTORTION_INCLUDED
#define VIDEO_LENSDISTORTION_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>

/* Forward declarations: */
namespace IO {
class File;
}

namespace Video {

class LensDistortion
	{
	/* Embedded classes: */
	public:
	typedef double Scalar; // Scalar type for calculations
	typedef Geometry::Point<Scalar,2> Point; // Type for points in distorted and undistorted image space
	typedef Geometry::Vector<Scalar,2> Vector; // Type for vectors in distorted and undistorted image space
	
	/* Elements: */
	private:
	Point center; // Distortion center
	Scalar kappas[3]; // Radial distortion coefficients
	Scalar rhos[2]; // Tangential distortion coefficients
	Scalar undistortMaxError; // Convergence threshold for Newton-Raphson iteration in undistortion formula
	int undistortMaxSteps; // Maximum number of Newton-Raphson steps in undistortion formula
	
	/* Constructors and destructors: */
	public:
	LensDistortion(void); // Creates an identity lens distortion correction formula
	LensDistortion(IO::File& file,bool legacyFormat =false); // Reads lens distortion correction formula from given binary file; flag selects legacy file format with radial scaling factor
	
	/* Methods: */
	const Point& getCenter(void) const
		{
		return center;
		}
	const Scalar* getKappas(void) const
		{
		return kappas;
		}
	Scalar getKappa(int index) const
		{
		return kappas[index];
		}
	const Scalar* getRhos(void) const
		{
		return rhos;
		}
	Scalar getRho(int index) const
		{
		return rhos[index];
		}
	void write(IO::File& file) const; // Writes lens distortion correction formula to given binary file
	void setCenter(const Point& newCenter);
	void setKappas(const Scalar newKappas[3]);
	void setKappa(int index,Scalar newKappa);
	void setRhos(const Scalar newRhos[2]);
	void setRho(int index,Scalar newRho);
	void read(IO::File& file,bool legacyFormat =false); // Reads lens distortion correction formula from given binary file; flag selects legacy file format with radial scaling factor
	Point distort(const Point& undistorted) const // Calculates forward lens distortion correction formula
		{
		Vector d=undistorted-center;
		Scalar r2=d.sqr();
		Scalar div=Scalar(1)+(kappas[0]+(kappas[1]+kappas[2]*r2)*r2)*r2; // Cubic radial distortion formula in r^2
		return Point(center[0]+d[0]/div+Scalar(2)*rhos[0]*d[0]*d[1]+rhos[1]*(r2+Scalar(2)*d[0]*d[0]), // Tangential distortion formula in x
		             center[1]+d[1]/div+rhos[0]*(r2+Scalar(2)*d[1]*d[1])+Scalar(2)*rhos[1]*d[0]*d[1]); // Tangential distortion formula in y
		}
	Scalar getUndistortMaxError(void) const
		{
		return undistortMaxError;
		}
	int getUndistortMaxSteps(void) const
		{
		return undistortMaxSteps;
		}
	void setUndistortMaxError(Scalar newUndistortMaxError);
	void setUndistortMaxSteps(int newUndistortMaxSteps);
	Point undistort(const Point& distorted) const; // Calculates inverse lens distortion correction formula via Newton-Raphson iteration
	};

}

#endif
