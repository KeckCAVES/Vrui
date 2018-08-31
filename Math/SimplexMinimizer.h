/***********************************************************************
SimplexMinimizer - Generic class to minimize a set of equations in a
least-squares sense using a simplex algorithm, templatized by a kernel
class implementing a specific optimization problem.
Copyright (c) 2014-2018 Oliver Kreylos

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

#ifndef MATH_SIMPLEXMINIMIZER_INCLUDED
#define MATH_SIMPLEXMINIMIZER_INCLUDED

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
	};

#endif

namespace Math {

template <class KernelParam>
class SimplexMinimizer:public Minimizer<KernelParam>
	{
	/* Embedded classes: */
	public:
	typedef Minimizer<KernelParam> Base; // Base class
	using typename Base::Kernel;
	using typename Base::Scalar;
	using Base::numVariables;
	static const unsigned int numVertices=numVariables+1; // Number of vertices in an optimization simplex
	using typename Base::VariableVector;
	using Base::numFunctionsInBatch;
	using typename Base::ProgressCallbackData;
	using typename Base::ProgressCallback;
	
	/* Elements: */
	
	/* Minimization parameters (public because there are no invariants): */
	public:
	Scalar expansionFactor; // Factor by which the current simplex is expanded if reflection reduces function value
	Scalar contractionFactor; // Factor by which the current simplex is contracted if reflection does not reduce function value
	Scalar initialSimplexSize[numVariables]; // Initial simplex size along each optimization space dimension
	Scalar jitter; // Amount of random noise for initial simplex as factor of initial simplex size
	Scalar minSimplexSize[numVariables]; // Minimum simplex size along each optimization space dimension
	using Base::maxNumIterations;
	size_t whackFrequency; // Frequency at which the current simplex is whacked with some scaling and random noise
	Scalar whackScale; // Scale factor by which the current simplex is whacked
	Scalar whackJitter; // Relative amount of noise added to the current simplex when whacked
	
	private:
	using Base::progressFrequency;
	using Base::progressCallback;
	
	/* Private methods: */
	static Scalar calcResidual2(Kernel& kernel,const VariableVector& v); // Calculates the total least-squares residual of the given minimization kernel w.r.t. the given variable vector
	static VariableVector calcFaceCenter(const VariableVector vertices[numVertices],unsigned int worstVertexIndex); // Returns the simplex's face center opposite of the vertex of the given index
	static VariableVector moveVertex(const VariableVector& vertex,const VariableVector& faceCenter,Scalar step); // Moves the given vertex to the given face center plus the given step size (-1 keeps vertex, 0 moves to face center)
	
	/* Constructors and destructors: */
	public:
	SimplexMinimizer(void); // Creates default simplex minimizer
	
	/* Methods: */
	Scalar minimize(Kernel& kernel); // Runs simplex minimization on the given optimization kernel; returns final least-squares residual
	};

}

#ifndef MATH_SIMPLEXMINIMIZER_IMPLEMENTATION
#include <Math/SimplexMinimizer.icpp>
#endif

#endif
