/***********************************************************************
Minimizer - Generic base class to minimize a set of equations in a
least-squares sense, templatized by a kernel class implementing a
specific optimization problem.
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

#ifndef MATH_MINIMIZER_INCLUDED
#define MATH_MINIMIZER_INCLUDED

#include <stddef.h>

/* Forward declarations: */
namespace Misc {
template <class ParameterParam>
class FunctionCall;
}

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
	};

#endif

namespace Math {

template <class KernelParam>
class Minimizer
	{
	/* Embedded classes: */
	public:
	typedef KernelParam Kernel; // Type implementing an optimization problem
	typedef typename Kernel::Scalar Scalar; // Scalar type of optimization space
	static const unsigned int numVariables=Kernel::numVariables; // Dimension of optimization space
	typedef typename Kernel::VariableVector VariableVector; // Type to store states of the optimization problem
	static const unsigned int numFunctionsInBatch=Kernel::numFunctionsInBatch; // Number of related functions evaluated in a single call
	
	struct ProgressCallbackData // Structure passed to progress callbacks
		{
		/* Elements: */
		public:
		Kernel& kernel; // The minimization kernel with state set to the current best guess for the minimum
		Scalar residual2; // The kernel's least-squares residual at its current state
		bool final; // Flag whether this is the final minimization result
		
		/* Constructors and destructors: */
		ProgressCallbackData(Kernel& sKernel,Scalar sResidual2,bool sFinal)
			:kernel(sKernel),residual2(sResidual2),final(sFinal)
			{
			}
		};
	
	typedef Misc::FunctionCall<const ProgressCallbackData&> ProgressCallback; // Type for functions called with current minimization estimates during minimization
	
	/* Elements: */
	
	/* Minimization parameters (public because there are no invariants): */
	public:
	size_t maxNumIterations; // Maximum number of iterations
	
	protected:
	size_t progressFrequency; // Number of minimization steps between calls to the progress function
	ProgressCallback* progressCallback; // Function called at regular intervals during minimization
	
	/* Constructors and destructors: */
	public:
	Minimizer(size_t sMaxNumIterations) // Creates a minimizer for the given maximum number of iterations
		:maxNumIterations(sMaxNumIterations),
		 progressFrequency(0),progressCallback(0)
		{
		}
	~Minimizer(void); // Destroys the minimizer
	
	/* Methods: */
	void setProgressCallback(size_t newProgressFrequency,ProgressCallback* newProgressCallback); // Registers a progress callback with the minimizer
	};

}

#ifndef MATH_MINIMIZER_IMPLEMENTATION
#include <Math/Minimizer.icpp>
#endif

#endif
