/***********************************************************************
OGTransformCalculator - Helper function to calculate an orthogonal
transformation (translation + rotation + uniform scaling) that
transforms one ordered point set into another ordered point set.
Copyright (c) 2015 Oliver Kreylos

This file is part of the Vrui calibration utility package.

The Vrui calibration utility package is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Vrui calibration utility package is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui calibration utility package; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include "OGTransformCalculator.h"

#include <Math/Math.h>
#include <Math/Matrix.h>
#include <Geometry/Rotation.h>

Geometry::Rotation<double,3> calculateRotation(size_t numPoints,double ip0,const std::vector<Geometry::Point<double,3> >& cPoints0,double ip1,const std::vector<Geometry::Point<double,3> >& cPoints1)
	{
	/* Calculate the inner product between the two point sets: */
	double m[3][3];
	for(int i=0;i<3;++i)
		for(int j=0;j<3;++j)
			m[i][j]=0.0;
	for(size_t pi=0;pi<numPoints;++pi)
		for(int i=0;i<3;++i)
			for(int j=0;j<3;++j)
				m[i][j]+=cPoints0[pi][i]*cPoints1[pi][j];
	
	/* Calculate the coefficients of the quaternion-based characteristic polynomial of the quaternion key matrix: */
	double q4=1.0;
	double q3=0.0;
	double q2=0.0;
	for(int i=0;i<3;++i)
		for(int j=0;j<3;++j)
			q2-=2.0*Math::sqr(m[i][j]);
	double q1=8.0*(m[0][0]*m[1][2]*m[2][1]+m[1][1]*m[2][0]*m[0][2]+m[2][2]*m[0][1]*m[1][0])
	         -8.0*(m[0][0]*m[1][1]*m[2][2]+m[1][2]*m[2][0]*m[0][1]+m[2][1]*m[1][0]*m[0][2]);
	double qd0=Math::sqr(Math::sqr(m[0][1])+Math::sqr(m[0][2])-Math::sqr(m[1][0])-Math::sqr(m[2][0]));
	double qd1=(-Math::sqr(m[0][0])+Math::sqr(m[1][1])+Math::sqr(m[2][2])+Math::sqr(m[1][2])+Math::sqr(m[2][1])-2.0*(m[1][1]*m[2][2]-m[1][2]*m[2][1]))
	          *(-Math::sqr(m[0][0])+Math::sqr(m[1][1])+Math::sqr(m[2][2])+Math::sqr(m[1][2])+Math::sqr(m[2][1])+2.0*(m[1][1]*m[2][2]-m[1][2]*m[2][1]));
	double qd2=(-(m[0][2]+m[2][0])*(m[1][2]-m[2][1])+(m[0][1]-m[1][0])*(m[0][0]-m[1][1]-m[2][2]))
	          *(-(m[0][2]-m[2][0])*(m[1][2]+m[2][1])+(m[0][1]-m[1][0])*(m[0][0]-m[1][1]+m[2][2]));
	double qd3=(-(m[0][2]+m[2][0])*(m[1][2]+m[2][1])-(m[0][1]+m[1][0])*(m[0][0]+m[1][1]-m[2][2]))
	          *(-(m[0][2]-m[2][0])*(m[1][2]-m[2][1])-(m[0][1]+m[1][0])*(m[0][0]+m[1][1]+m[2][2]));
	double qd4=((m[0][1]+m[1][0])*(m[1][2]+m[2][1])+(m[0][2]+m[2][0])*(m[0][0]-m[1][1]+m[2][2]))
	          *(-(m[0][1]-m[1][0])*(m[1][2]-m[2][1])+(m[0][2]+m[2][0])*(m[0][0]+m[1][1]+m[2][2]));
	double qd5=((m[0][1]+m[1][0])*(m[1][2]-m[2][1])+(m[0][2]-m[2][0])*(m[0][0]-m[1][1]-m[2][2]))
	          *(-(m[0][1]-m[1][0])*(m[1][2]+m[2][1])+(m[0][2]-m[2][0])*(m[0][0]+m[1][1]-m[2][2]));
	double q0=qd0+qd1+qd2+qd3+qd4+qd5;
	
	/* Calculate the optimal rotation: */
	double lambda=Math::mid(ip0,ip1);
	double lambda0;
	do
		{
		lambda0=lambda;
		double poly=(((q4*lambda+q3)*lambda+q2)*lambda+q1)*lambda+q0;
		double dPoly=((4.0*q4*lambda+3.0*q3)*lambda+2.0*q2)*lambda+q1;
		lambda-=poly/dPoly;
		}
	while(Math::abs(lambda-lambda0)<1.0e-12);
	
	/* Find the eigenvector corresponding to the largest eigenvalue: */
	Math::Matrix k(4,4);
	k(0,0)=m[0][0]+m[1][1]+m[2][2];
	k(0,1)=m[1][2]-m[2][1];
	k(0,2)=m[2][0]-m[0][2];
	k(0,3)=m[0][1]-m[1][0];
	k(1,0)=m[1][2]-m[2][1];
	k(1,1)=m[0][0]-m[1][1]-m[2][2];
	k(1,2)=m[0][1]+m[1][0];
	k(1,3)=m[2][0]+m[0][2];
	k(2,0)=m[2][0]-m[0][2];
	k(2,1)=m[0][1]+m[1][0];
	k(2,2)=-m[0][0]+m[1][1]-m[2][2];
	k(2,3)=m[1][2]+m[2][1];
	k(3,0)=m[0][1]-m[1][0];
	k(3,1)=m[2][0]+m[0][2];
	k(3,2)=m[1][2]+m[2][1];
	k(3,3)=-m[0][0]-m[1][1]+m[2][2];
	
	std::pair<Math::Matrix,Math::Matrix> jacobi=k.jacobiIteration();
	double maxE=jacobi.second(0);
	int maxEIndex=0;
	for(int i=1;i<4;++i)
		if(maxE<jacobi.second(i))
			{
			maxE=jacobi.second(i);
			maxEIndex=i;
			}
	
	return Geometry::Rotation<double,3>::fromQuaternion(jacobi.first(1,maxEIndex),jacobi.first(2,maxEIndex),jacobi.first(3,maxEIndex),jacobi.first(0,maxEIndex));
	}
