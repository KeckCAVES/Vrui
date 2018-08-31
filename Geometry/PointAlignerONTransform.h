/***********************************************************************
PointAlignerONTransform - Least-squares optimization kernel for point
set alignment using rigid body transformations.
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

#ifndef GEOMETRY_POINTALIGNERONTRANSFORM_INCLUDED
#define GEOMETRY_POINTALIGNERONTRANSFORM_INCLUDED

#include <Math/Math.h>
#include <Geometry/ComponentArray.h>
#include <Geometry/OrthonormalTransformation.h>

#include <Geometry/PointAligner.h>

namespace Geometry {

template <class ScalarParam,int dimensionParam>
class PointAlignerONTransform:public PointAligner<ScalarParam,dimensionParam,OrthonormalTransformation>
	{
	/* Dummy class; must make dimension-specific specializations */
	};

template <class ScalarParam>
class PointAlignerONTransform<ScalarParam,2>:public PointAligner<ScalarParam,3,OrthonormalTransformation>
	{
	/* Declarations of inherited types: */
	public:
	typedef PointAligner<ScalarParam,2,OrthonormalTransformation> Base;
	using typename Base::Scalar;
	using Base::dimension;
	using typename Base::Point;
	using typename Base::Transform;
	using typename Base::PointPair;
	using Base::numFunctionsInBatch;
	
	/* Optimization kernel interface: */
	static const unsigned int numVariables=3; // Number of variables in the optimization problem
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
		VariableVector result;
		
		/* Copy the translation vector into the variable vector: */
		for(int i=0;i<2;++i)
			result[i]=current.getTranslation()[i];
		
		/* Copy the rotation angle into the variable vector: */
		result[2]=current.getRotation().getAngle();
		
		return result;
		}
	void setState(const VariableVector& newState) // Sets the optimization kernel's current state from a variable vector
		{
		/* Get a translation vector from the variable vector: */
		typename Transform::Vector t;
		for(int i=0;i<2;++i)
			t[i]=newState[i];
		
		/* Get a rotation angle from the variable vector: */
		Scalar a=newState[2];
		
		/* Set the current transformation: */
		current=Transform(t,Transform::Rotation(a));
		}
	void calcDerivativeBatch(unsigned int batchIndex,Scalar derivs[numFunctionsInBatch][numVariables]) const; // Returns the partial derivatives of a related batch of optimization target functions
	void negStep(const Scalar step[numVariables]) // Subtracts the given variable vector from the current configuration
		{
		/* Update the translation vector: */
		typename Transform::Vector newT=current.getTranslation();
		for(int i=0;i<2;++i)
			newT[i]-=step[i];
		
		/* Update the rotation angle: */
		Scalar newA=current.getRotation().getAngle()-step[2];
		
		/* Set the current transformation: */
		current=Transform(newT,Transform::Rotation(newA));
		}
	};

template <class ScalarParam>
class PointAlignerONTransform<ScalarParam,3>:public PointAligner<ScalarParam,3,OrthonormalTransformation>
	{
	/* Declarations of inherited types: */
	public:
	typedef PointAligner<ScalarParam,3,OrthonormalTransformation> Base;
	using typename Base::Scalar;
	using Base::dimension;
	using typename Base::Point;
	using typename Base::Transform;
	using typename Base::PointPair;
	using Base::numFunctionsInBatch;
	
	/* Optimization kernel interface: */
	static const unsigned int numVariables=7; // Number of variables in the optimization problem
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
		VariableVector result;
		
		/* Copy the translation vector into the variable vector: */
		for(int i=0;i<3;++i)
			result[i]=current.getTranslation()[i];
		
		/* Copy the rotation quaternion into the variable vector: */
		for(int i=0;i<4;++i)
			result[3+i]=current.getRotation().getQuaternion()[i];
		
		return result;
		}
	void setState(const VariableVector& newState) // Sets the optimization kernel's current state from a variable vector
		{
		/* Get a translation vector from the variable vector: */
		typename Transform::Vector t;
		for(int i=0;i<3;++i)
			t[i]=newState[i];
		
		/* Get a rotation quaternion from the variable vector: */
		Scalar q[4];
		for(int i=0;i<4;++i)
			q[i]=newState[3+i];
		
		/* Set the current transformation: */
		current=Transform(t,Transform::Rotation::fromQuaternion(q));
		}
	void calcDerivativeBatch(unsigned int batchIndex,Scalar derivs[numFunctionsInBatch][numVariables]) const; // Returns the partial derivatives of a related batch of optimization target functions
	void negStep(const Scalar step[numVariables]) // Subtracts the given variable vector from the current configuration
		{
		/* Update the translation vector: */
		typename Transform::Vector newT=current.getTranslation();
		for(int i=0;i<3;++i)
			newT[i]-=step[i];
		
		/* Update the rotation quaternion: */
		Scalar newQ[4];
		for(int i=0;i<4;++i)
			newQ[i]=current.getRotation().getQuaternion()[i]-step[3+i];
		
		/* Set the current transformation: */
		current=Transform(newT,Transform::Rotation::fromQuaternion(newQ));
		}
	};

}

#ifndef GEOMETRY_POINTALIGNERONTRANSFORM_IMPLEMENTATION
#include <Geometry/PointAlignerONTransform.icpp>
#endif

#endif
