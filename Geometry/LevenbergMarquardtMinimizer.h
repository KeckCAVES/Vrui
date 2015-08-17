/***********************************************************************
LevenbergMarquardtMinimizer - Class to implement n-dimensional least-
squares minimization using a version of the modified Levenberg-Marquardt
algorithm. Class is templatized by a functor class to evaluate the
target function and target function derivatives.
Copyright (c) 2007-2014 Oliver Kreylos

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

#ifndef LEVENBERGMARQUARDTMINIMIZER_INCLUDED
#define LEVENBERGMARQUARDTMINIMIZER_INCLUDED

#include <stddef.h>
#include <Math/Math.h>
#include <Geometry/ComponentArray.h>
#include <Geometry/Matrix.h>

namespace Geometry {

template <class FitterFunctor>
class LevenbergMarquardtMinimizer
	{
	/* Embedded classes: */
	public:
	typedef FitterFunctor Fitter; // Functor class for the fitting geometry
	typedef typename FitterFunctor::Scalar Scalar; // Scalar type
	static const int dimension=Fitter::dimension; // Dimension of the optimization space
	typedef typename FitterFunctor::Derivative Derivative; // Type for distance function derivatives
	typedef Matrix<Scalar,dimension,dimension> Jacobian; // Type for Jacobian matrices
	typedef ComponentArray<Scalar,dimension> Vector; // Type for vectors (in the matrix sense)
	
	/* Elements: */
	
	/* Minimization parameters (public because there are no invariants): */
	public:
	Scalar tau;
	Scalar epsilon1;
	Scalar epsilon2;
	size_t maxNumIterations;
	
	/* Constructors and destructors: */
	public:
	LevenbergMarquardtMinimizer(void) // Sets default minimization parameters
		:tau(1.0e-3),
		 epsilon1(1.0e-20),
		 epsilon2(1.0e-20),
		 maxNumIterations(1000)
		{
		}
	LevenbergMarquardtMinimizer(Scalar sTau,Scalar sEpsilon1,Scalar sEpsilon2,size_t sMaxNumIterations) // Sets given minimization parameters
		:tau(sTau),
		 epsilon1(sEpsilon1),
		 epsilon2(sEpsilon2),
		 maxNumIterations(sMaxNumIterations)
		{
		}
	
	/* Methods: */
	Scalar minimize(Fitter& fitter) const; // Minimizes the target function by manipulating the given fitter
	};

/********************************************
Methods of class LevenbergMarquardtMinimizer:
********************************************/

template <class FitterParam>
inline
typename LevenbergMarquardtMinimizer<FitterParam>::Scalar
LevenbergMarquardtMinimizer<FitterParam>::minimize(
	typename LevenbergMarquardtMinimizer<FitterParam>::Fitter& fitter) const
	{
	/* Compute the Jacobian matrix, the error vector, and the initial target function value: */
	Jacobian A;
	Vector g;
	for(int i=0;i<dimension;++i)
		{
		for(int j=0;j<dimension;++j)
			A(i,j)=Scalar(0);
		g[i]=Scalar(0);
		}
	Scalar F(0);
	for(size_t index=0;index<fitter.getNumPoints();++index)
		{
		Derivative dp=fitter.calcDistanceDerivative(index);
		Scalar d=fitter.calcDistance(index);
		for(int i=0;i<dimension;++i)
			{
			for(int j=0;j<dimension;++j)
				A(i,j)+=dp[i]*dp[j];
			g[i]+=dp[i]*d;
			}
		F+=Math::sqr(d);
		}
	F*=Scalar(0.5);
	
	/* Compute the initial damping factor: */
	Scalar maxA=A(0,0);
	for(int i=1;i<dimension;++i)
		if(maxA<A(i,i))
			maxA=A(i,i);
	Scalar mu=tau*maxA;
	Scalar nu=Scalar(2);
	
	/* Check for convergence: */
	bool found=true;
	for(int i=0;i<dimension;++i)
		if(Math::abs(g[i])>epsilon1)
			found=false;
	for(size_t iteration=0;!found&&iteration<maxNumIterations;++iteration)
		{
		/* Calculate step direction: */
		Jacobian H=A;
		for(int i=0;i<dimension;++i)
			H(i,i)+=mu;
		Vector h=g/H; // h is actually the negative of hlm in the pseudo-code
		
		/* Check for convergence: */
		if(Geometry::mag(h)<=epsilon2*(fitter.calcMag()+epsilon2))
			break;
		
		/* Try updating the current state: */
		fitter.save();
		fitter.increment(h); // Subtracts h instead of adding (h is negative, see above)
		fitter.normalize();
		
		/* Calculate the new target function value: */
		Scalar newF(0);
		for(size_t index=0;index<fitter.getNumPoints();++index)
			newF+=Math::sqr(fitter.calcDistance(index));
		newF*=Scalar(0.5);
		
		/* Calculate the gain value: */
		Scalar denom(0);
		for(int i=0;i<dimension;++i)
			denom+=h[i]*(mu*h[i]+g[i]); // Adds g instead of subtracting (h is negative, see above)
		denom*=Scalar(0.5);
		Scalar rho=(F-newF)/denom;
		
		/* Accept or deny the step: */
		if(rho>Scalar(0))
			{
			/* Compute the new Jacobian matrix and the new error vector: */
			for(int i=0;i<dimension;++i)
				{
				for(int j=0;j<dimension;++j)
					A(i,j)=Scalar(0);
				g[i]=Scalar(0);
				}
			for(size_t index=0;index<fitter.getNumPoints();++index)
				{
				Derivative dp=fitter.calcDistanceDerivative(index);
				Scalar d=fitter.calcDistance(index);
				for(int i=0;i<dimension;++i)
					{
					for(int j=0;j<dimension;++j)
						A(i,j)+=dp[i]*dp[j];
					g[i]+=dp[i]*d;
					}
				}
			
			/* Update the target function value: */
			F=newF;
			
			/* Check for convergence: */
			found=true;
			for(int i=0;i<dimension;++i)
				if(Math::abs(g[i])>epsilon1)
					found=false;
			
			/* Update the damping factor: */
			Scalar rhof=Scalar(2)*rho-Scalar(1);
			Scalar factor=Scalar(1)-rhof*rhof*rhof;
			if(factor<Scalar(1)/Scalar(3))
				factor=Scalar(1)/Scalar(3);
			mu*=factor;
			nu=Scalar(2);
			}
		else
			{
			/* Deny the step: */
			fitter.restore();
			
			/* Update the damping factor: */
			mu*=nu;
			nu*=Scalar(2);
			}
		}
	
	return F;
	}

#if 0

/************************
Minimization functor API:
************************/

class LevenbergMarquardtFunctor
	{
	/* Embedded classes: */
	public:
	typedef ... Scalar; // Scalar type
	static const int dimension=...; // Dimension of the optimization space
	typedef Geometry::ComponentArray<Scalar,dimension> Derivative; // Type for distance function derivatives
	
	/* Methods: */
	void save(void); // Saves the current estimate
	void restore(void); // Restores the last saved estimate
	size_t getNumPoints(void) const; // Returns the number of target points
	Scalar calcDistance(size_t index) const; // Calculates the distance value for the current estimate and the given target point
	Derivative calcDistanceDerivative(size_t index) const; // Calculates the derivative of the distance function for the current estimate and the given target point
	Scalar calcMag(void) const; // Returns the magnitude of the current estimate
	void increment(Derivative increment); // Increments the current estimate by *the negative of* the given difference vector
	void normalize(void); // Re-normalizes the current estimate after increment()
	};

#endif

}

#endif
