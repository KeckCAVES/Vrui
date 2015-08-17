/***********************************************************************
SimplexMinimizer - Generic class to perform minimization of arbitrary
target functions using a simplex algorithm.
Copyright (c) 2014 Oliver Kreylos

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

#include <utility>

/* Forward declarations: */
namespace Misc {
template <class ParameterParam>
class FunctionCall;
}

namespace Math {

template <class MinimizationFunctionParam>
class SimplexMinimizer
	{
	/* Embedded classes: */
	public:
	typedef MinimizationFunctionParam MinimizationFunction; // Type for minimization target function
	typedef typename MinimizationFunction::Scalar Scalar; // Scalar type of minimization target function's domain space
	static const unsigned int dimension=MinimizationFunction::dimension; // Dimension of the minimization target function's domain space
	typedef typename MinimizationFunction::Vertex Vertex; // Type for points in minimization target function's domain space
	typedef typename MinimizationFunction::Value Value; // Type for minimization target function's value space
	typedef std::pair<Vertex,Value> ValuedVertex; // Type for vertices with associated function values
	typedef Misc::FunctionCall<const ValuedVertex&> ProgressCallback; // Type for functions called with current minimization estimates during minimization
	
	/* Elements: */
	public:
	unsigned int maxNumSteps; // Maximum number of simplex manipulation steps in each call of minimize
	Scalar expansionFactor; // Factor by which the current simplex is expanded if reflection reduces function value
	Scalar contractionFactor; // Factor by which the current simplex is contracted if reflection does not reduce function value
	private:
	unsigned int progressFrequency; // Number of minimization steps between calls to the progress function
	ProgressCallback* progressCallback; // Function called at regular intervals during minimization
	Vertex vertices[dimension+1]; // Array of vertices defining the current simplex
	Value values[dimension+1]; // Array of minimization function values at the current simplex vertices
	
	/* Constructors and destructors: */
	public:
	SimplexMinimizer(void); // Creates default simplex minimizer
	~SimplexMinimizer(void);
	
	/* Methods: */
	void setProgressCallback(unsigned int newProgressFrequency,ProgressCallback* newProgressCallback); // Registers a progress callback with the minimizer
	ValuedVertex minimize(const MinimizationFunction& function,const Vertex& initialVertex,Scalar initialSimplexSize); // Minimizes the given target function from a simplex of the given relative size around the given initial vertex; returns best vertex and target function value at that vertex
	};

}

#ifndef MATH_SIMPLEXMINIMIZER_IMPLEMENTATION
#include <Math/SimplexMinimizer.icpp>
#endif

#endif
