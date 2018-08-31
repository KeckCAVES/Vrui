/***********************************************************************
GaussNewtonMinimizer - Generic class to minimize a set of equations in a
least-squares sense using the Gauss-Newton algorithm, templatized by a
kernel class implementing a specific optimization problem.
Copyright (c) 2018 Oliver Kreylos

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

#ifndef MATH_GAUSSNEWTONMINIMIZER_INCLUDED
#define MATH_GAUSSNEWTONMINIMIZER_INCLUDED

#include <Math/Minimizer.h>

/************************************
Required interface of Kernel classes:
************************************/

#if 0 // This is not actual code

class Kernel
	{
	/* Embedded classes: */
	public:
	typedef Scalar; // Scalar type for optimization space
	static const unsigned int numVariables; // Dimension of optimization space
	class VariableVector // Class representing a point in optimization problem state space
		{
		/* Constructors and destructors: */
		public:
		VariableVector(void); // Creates potentially uninitialized variable vector
		VariableVector(const VariableVector& source); // Copy constructor
		VariableVector& operator=(const VariableVector& source); // Assignment operator
		
		/* Methods: */
		Scalar& operator[](unsigned int componentIndex); // Returns the vector component of the given index (0<=index<numVariables) as a modifiable L-value
		};
	static const unsigned int numFunctionsInBatch; // Number of related optimization functions evaluated in a single call to reduce redundant operations
	
	/* Methods: */
	VariableVector getState(void) const; // Returns the current optimization system state as a variable vector
	void setState(const VariableVector& newState); // Sets the current optimization system state from the given variable vector
	unsigned int getNumBatches(void) const; // Returns the number of function batches in the optimization problem
	void calcValueBatch(unsigned int batchIndex,Scalar values[numFunctionsInBatch]); // Calculates a batch of residual values for the current optimization system state
	void calcDerivativeBatch(unsigned int batchIndex,Scalar derivatives[numFunctionsInBatch][numVariables]); // Calculates a batch of residual derivatives for the current optimization system state
	void negStep(const Scalar stepVector[numVariables]); // Changes current optimization system state by subtracting the given step vector from the current system state vector
	};

#endif

namespace Math {

template <class KernelParam>
class GaussNewtonMinimizer:public Minimizer<KernelParam>
	{
	/* Embedded classes: */
	public:
	typedef Minimizer<KernelParam> Base; // Base class
	using typename Base::Kernel;
	using typename Base::Scalar;
	using Base::numVariables;
	using typename Base::VariableVector;
	using Base::numFunctionsInBatch;
	using typename Base::ProgressCallbackData;
	using typename Base::ProgressCallback;
	
	/* Elements: */
	
	/* Minimization parameters (public because there are no invariants): */
	using Base::maxNumIterations;
	
	private:
	using Base::progressFrequency;
	using Base::progressCallback;
	
	/* Constructors and destructors: */
	public:
	GaussNewtonMinimizer(void) // Creates default Gauss-Newton minimizer
		:Base(20)
		{
		}
	GaussNewtonMinimizer(size_t sMaxNumIterations) // Creates Gauss-Newton minimizer with the given parameters
		:Base(sMaxNumIterations)
		{
		}
	
	/* Methods: */
	Scalar minimize(Kernel& kernel); // Runs Gauss-Newton minimization on the given optimization kernel; returns final least-squares residual
	};

}

#ifndef MATH_GAUSSNEWTONMINIMIZER_IMPLEMENTATION
#include <Math/GaussNewtonMinimizer.icpp>
#endif

#endif
