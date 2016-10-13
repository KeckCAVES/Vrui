/***********************************************************************
Algorithms - Implementations of common numerical algorithms.
Copyright (c) 2015 Oliver Kreylos

This file is part of the Templatized Math Library (Math).

The Templatized Math Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Templatized Math Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Templatized Math Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Math/Algorithms.h>

#include <Math/Math.h>

namespace Math {

namespace {

static const double rootOffset=2.0*3.14159265358979323846/3.0; // 2/3 pi

}

unsigned int solveCubicEquation(const double coefficients[4],double solutions[3])
	{
	/* Normalize the cubic equation: */
	double nc[3];
	nc[0]=coefficients[1]/coefficients[0];
	nc[1]=coefficients[2]/coefficients[0];
	nc[2]=coefficients[3]/coefficients[0];
	
	unsigned int numRoots;
	
	double q=(Math::sqr(nc[0])-3.0*nc[1])/9.0;
	double q3=Math::sqr(q)*q;
	double r=((2.0*Math::sqr(nc[0])-9.0*nc[1])*nc[0]+27.0*nc[2])/54.0;
	if(Math::sqr(r)<q3)
		{
		/* There are three real roots: */
		double thetaThird=Math::acos(r/Math::sqrt(q3))/3.0;
		double factor=-2.0*Math::sqrt(q);
		solutions[0]=factor*Math::cos(thetaThird)-nc[0]/3.0;
		solutions[1]=factor*Math::cos(thetaThird+rootOffset)-nc[0]/3.0;
		solutions[2]=factor*Math::cos(thetaThird-rootOffset)-nc[0]/3.0;
		
		numRoots=3;
		}
	else
		{
		/* There is only one real root: */
		double a=Math::pow(Math::abs(r)+Math::sqrt(Math::sqr(r)-q3),1.0/3.0);
		if(r>0.0)
			a=-a;
		double b=a==0.0?0.0:q/a;
		solutions[0]=a+b-nc[0]/3.0;
		
		numRoots=1;
		}
	
	/* Use Newton iteration to clean up the roots: */
	for(unsigned int i=0;i<numRoots;++i)
		for(int j=0;j<2;++j)
			{
			double f=((solutions[i]+nc[0])*solutions[i]+nc[1])*solutions[i]+nc[2];
			double fp=(3.0*solutions[i]+2.0*nc[0])*solutions[i]+nc[1];
			double s=f/fp;
			solutions[i]-=s;
			}
	
	return numRoots;
	}

}
