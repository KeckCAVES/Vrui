/***********************************************************************
AlignPoints - Functions to align to sets of matched points using several
types of alignment transformations by minimizing RMS residual error.
These functions aim for high alignment quality and might therefore be
slow.
Copyright (c) 2009-2017 Oliver Kreylos

This file is part of the Templatized Geometry Library (TGL).

The Templatized Geometry Library is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Templatized Geometry Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Templatized Geometry Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#define GEOMETRY_NONSTANDARD_TEMPLATES
#include <Geometry/AlignPoints.icpp>

#include <Math/Math.h>
#include <Math/Constants.h>
#include <Math/Matrix.h>
#include <Geometry/LevenbergMarquardtMinimizer.h>

namespace Geometry {

namespace {

/**************
Helper classes:
**************/

class OGTransformFitter // Class to fit a uniformly scaled rigid body transformation using Levenberg-Marquardt optimization
	{
	/* Embedded classes: */
	public:
	typedef double Scalar; // Scalar type
	typedef Geometry::Point<Scalar,3> Point;
	typedef Geometry::Vector<Scalar,3> Vector;
	typedef Geometry::OrthogonalTransformation<Scalar,3> Transform;
	static const int dimension=8; // Dimension of the optimization space
	typedef Geometry::ComponentArray<Scalar,dimension> Derivative; // Type for distance function derivatives
	
	/* Elements: */
	private:
	size_t numPoints; // Number of source and target points
	const Point* sp; // Array of source points
	const Point* tp; // Array of target points
	
	/* Transient optimization state: */
	Transform transform; // The current transformation estimate
	const Vector& t; // Shortcut to transformation's translation vector
	const Scalar* q; // Shortcut to transformation's rotation quaternion
	Transform transformSave;
	
	/* Constructors and destructors: */
	public:
	OGTransformFitter(size_t sNumPoints,const Point* sSp,const Point* sTp)
		:numPoints(sNumPoints),sp(sSp),tp(sTp),
		 transform(Transform::identity),
		 t(transform.getTranslation()),
		 q(transform.getRotation().getQuaternion())
		{
		}
	
	/* Methods: */
	const Transform& getTransform(void) const // Returns the current transformation estimate
		{
		return transform;
		}
	void setTransform(const Transform& newTransform) // Sets the current transformation estimate
		{
		transform=newTransform;
		};
	
	/* Methods required by Levenberg-Marquardt optimizer: */
	void save(void) // Saves the current estimate
		{
		transformSave=transform;
		};
	void restore(void) // Restores the last saved estimate
		{
		transform=transformSave;
		};
	size_t getNumPoints(void) const // Returns the number of distance functions to minimize
		{
		return numPoints;
		}
	Scalar calcDistance(size_t index) const // Calculates the distance value for the current estimate and the given distance function index
		{
		return Geometry::dist(transform.transform(sp[index]),tp[index]);
		}
	Derivative calcDistanceDerivative(size_t index) const // Calculates the derivative of the distance value for the current estimate and the given distance function index
		{
		const Point& s=sp[index];
		
		/*******************************************************************
		Calculate the distance vector between the transformed source point
		and the target point. The transformation is spelled out in order to
		reuse the intermediate results for the derivative calculation.
		*******************************************************************/
		
		/* Calculate the first rotation part: */
		Scalar rX=q[1]*s[2]-q[2]*s[1]+q[3]*s[0];
		Scalar rY=q[2]*s[0]-q[0]*s[2]+q[3]*s[1];
		Scalar rZ=q[0]*s[1]-q[1]*s[0]+q[3]*s[2];
		Scalar rW=q[0]*s[0]+q[1]*s[1]+q[2]*s[2];
		
		/* Calculate the scaling, second rotation part, and the translation and difference: */
		Vector d;
		d[0]=(rZ*q[1]-rY*q[2]+rW*q[0]+rX*q[3])*transform.getScaling()+t[0]-tp[index][0];
		d[1]=(rX*q[2]-rZ*q[0]+rW*q[1]+rY*q[3])*transform.getScaling()+t[1]-tp[index][1];
		d[2]=(rY*q[0]-rX*q[1]+rW*q[2]+rZ*q[3])*transform.getScaling()+t[2]-tp[index][2];
		
		/* Calculate the difference magnitude: */
		Scalar dist=Geometry::mag(d);
		
		Derivative result;
		
		/* Calculate the translational partial derivatives: */
		result[0]=d[0]/dist;
		result[1]=d[1]/dist;
		result[2]=d[2]/dist;
		
		/* Calculate the rotational partial derivatives: */
		result[3]=Scalar(2)*(+d[0]*rW-d[1]*rZ+d[2]*rY)*transform.getScaling()/dist;
		result[4]=Scalar(2)*(+d[0]*rZ+d[1]*rW-d[2]*rX)*transform.getScaling()/dist;
		result[5]=Scalar(2)*(-d[0]*rY+d[1]*rX+d[2]*rW)*transform.getScaling()/dist;
		result[6]=Scalar(2)*(+d[0]*rX+d[1]*rY+d[2]*rZ)*transform.getScaling()/dist;
		
		/* Calculate the scaling partial derivatives: */
		result[7]=((rZ*q[1]-rY*q[2]+rW*q[0]+rX*q[3])*d[0]
		          +(rX*q[2]-rZ*q[0]+rW*q[1]+rY*q[3])*d[1]
		          +(rY*q[0]-rX*q[1]+rW*q[2]+rZ*q[3])*d[2])/dist;
		
		return result;
		}
	Scalar calcMag(void) const // Returns the magnitude of the current estimate
		{
		return Math::sqrt(Geometry::sqr(t)+Scalar(1)+Math::sqr(transform.getScaling()));
		}
	void increment(const Derivative& increment) // Increments the current estimate by the given difference vector
		{
		Vector newT;
		for(int i=0;i<3;++i)
			newT[i]=t[i]-increment[i];
		Scalar newQ[4];
		for(int i=0;i<4;++i)
			newQ[i]=q[i]-increment[3+i];
		Scalar newS=transform.getScaling()-increment[7];
		transform=Transform(newT,Transform::Rotation::fromQuaternion(newQ),newS);
		}
	void normalize(void) // Normalizes the current estimate
		{
		/* Not necessary; Transform constructor already normalizes the quaternion */
		}
	};

}

/****************************
Internal alignment functions:
****************************/

Geometry::OrthogonalTransformation<double,3> alignPointsOGTransformInternal(size_t numPoints,const Point<double,3>* points0,const Point<double,3>* points1,unsigned int numIterations)
	{
	typedef Geometry::OrthogonalTransformation<double,3> Transform;
	
	/* Calculate the inner product between the two point sets: */
	double m[3][3];
	for(int i=0;i<3;++i)
		for(int j=0;j<3;++j)
			m[i][j]=0.0;
	for(size_t pi=0;pi<numPoints;++pi)
		for(int i=0;i<3;++i)
			for(int j=0;j<3;++j)
				m[i][j]+=points0[pi][i]*points1[pi][j];
	
	/* Construct the inner product's key matrix: */
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
	
	/* Find the eigenvector corresponding to the largest eigenvalue of the key matrix, which is the quaternion of the optimal rotation: */
	std::pair<Math::Matrix,Math::Matrix> jacobi=k.jacobiIteration();
	double maxE=jacobi.second(0);
	int maxEIndex=0;
	for(int i=1;i<4;++i)
		if(maxE<jacobi.second(i))
			{
			maxE=jacobi.second(i);
			maxEIndex=i;
			}
	Transform::Rotation rotation=Transform::Rotation::fromQuaternion(jacobi.first(1,maxEIndex),jacobi.first(2,maxEIndex),jacobi.first(3,maxEIndex),jacobi.first(0,maxEIndex));
	
	/* Improve the result by running a few steps of Levenberg-Marquardt optimization: */
	Transform bestTransform=Transform::rotate(rotation);
	double bestDistance=Math::Constants<double>::max;
	
	for(int i=0;i<1;++i)
		{
		/* Minimize the distance: */
		Geometry::LevenbergMarquardtMinimizer<OGTransformFitter> minimizer;
		minimizer.maxNumIterations=numIterations;
		OGTransformFitter tf(numPoints,points0,points1);
		tf.setTransform(bestTransform);
		double result=minimizer.minimize(tf);
		if(bestDistance>result)
			{
			bestTransform=tf.getTransform();
			bestDistance=result;
			}
		}
	
	/* Return the best transformation: */
	return bestTransform;
	}

/*******************************************************
Force instantiation of all standard alignment functions:
*******************************************************/

template AlignResult<Geometry::OrthogonalTransformation<double,3> > alignPointsOGTransform(const std::vector<Point<float,3> >& points0,const std::vector<Point<float,3> >& points1,unsigned int numIterations);
template AlignResult<Geometry::OrthogonalTransformation<double,3> > alignPointsOGTransform(const std::vector<Point<double,3> >& points0,const std::vector<Point<double,3> >& points1,unsigned int numIterations);

}
