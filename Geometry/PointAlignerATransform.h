/***********************************************************************
PointAlignerATransform - Least-squares optimization kernel for point
set alignment using general affine transformations.
Copyright (c) 2018 Oliver Kreylos

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

#ifndef GEOMETRY_POINTALIGNERATRANSFORM_INCLUDED
#define GEOMETRY_POINTALIGNERATRANSFORM_INCLUDED

#include <Math/Math.h>
#include <Geometry/ComponentArray.h>
#include <Geometry/AffineTransformation.h>

#include <Geometry/PointAligner.h>

namespace Geometry {

template <class ScalarParam,int dimensionParam>
class PointAlignerATransform:public PointAligner<ScalarParam,dimensionParam,AffineTransformation>
	{
	/* Declarations of inherited types: */
	public:
	typedef PointAligner<ScalarParam,dimensionParam,AffineTransformation> Base;
	using typename Base::Scalar;
	using Base::dimension;
	using typename Base::Point;
	using typename Base::Transform;
	using typename Base::PointPair;
	using Base::numFunctionsInBatch;
	
	/* Optimization kernel interface: */
	static const unsigned int numVariables=dimension*(dimension+1); // Number of variables in the optimization problem
	typedef ComponentArray<Scalar,numVariables> VariableVector; // Structure representing a configuration of the optimization kernel
	
	/* Declarations of inherited elements: */
	protected:
	using Base::pointPairs;
	using Base::fromCenter;
	using Base::toCenter;
	using Base::fromScale;
	using Base::toScale;
	using Base::current;
	
	/* Methods: */
	public:
	void estimateTransform(void); // Calculates an initial estimate for the transformation to align the from and to point sets
	Transform getTransform(void) const; // Returns the (de-normalized) current transformation estimate
	
	/* Optimization kernel methods: */
	VariableVector getState(void) const // Returns the optimization kernel's current state as a variable vector
		{
		/* Copy the transformation's matrix elements into a variable vector: */
		VariableVector result;
		for(int i=0;i<dimension;++i)
			for(int j=0;j<=dimension;++j)
				result[i*(dimension+1)+j]=current.getMatrix()(i,j);
		
		return result;
		}
	void setState(const VariableVector& newState) // Sets the optimization kernel's current state from a variable vector
		{
		/* Set the transformation's matrix elements from the given variable vector: */
		for(int i=0;i<dimension;++i)
			for(int j=0;j<=dimension;++j)
				current.getMatrix()(i,j)=newState[i*(dimension+1)+j];
		}
	void calcDerivativeBatch(unsigned int batchIndex,Scalar derivs[numFunctionsInBatch][numVariables]) const; // Returns the partial derivatives of a related batch of optimization target functions
	void negStep(const Scalar step[numVariables]) // Subtracts the given variable vector from the current configuration
		{
		for(int i=0;i<dimension;++i)
			for(int j=0;j<=dimension;++j)
				current.getMatrix()(i,j)-=step[i*(dimension+1)+j];
		}
	};

}

#ifndef GEOMETRY_POINTALIGNERATRANSFORM_IMPLEMENTATION
#include <Geometry/PointAlignerATransform.icpp>
#endif

#endif
