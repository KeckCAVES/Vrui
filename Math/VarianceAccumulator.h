/***********************************************************************
VarianceAccumulator - Class to accumulate the mean and variance of a set
of samples. Based on the recurrence relation from D.E. Knuth, "The Art
of Computer Programming," Vol. 2.
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

#ifndef MATH_VARIANCEACCUMULATOR_INCLUDED
#define MATH_VARIANCEACCUMULATOR_INCLUDED

#include <Math/Math.h>

namespace Math {

class VarianceAccumulator
	{
	/* Elements: */
	private:
	size_t numSamples; // Number of samples already accumulated
	double accMean; // Accumulated mean
	double accVariance; // Accumulated variance
	
	/* Constructors and destructors: */
	public:
	VarianceAccumulator(void) // Creates empty accumulator
		:numSamples(0),accMean(0.0),accVariance(0.0)
		{
		}
	
	/* Methods: */
	void reset(void) // Resets the accumulator
		{
		numSamples=0;
		accMean=0.0;
		accVariance=0.0;
		}
	void addSample(double sample) // Accumulate a sample
		{
		++numSamples;
		double newAccMean=accMean+(sample-accMean)/double(numSamples);
		accVariance+=(sample-accMean)*(sample-newAccMean);
		accMean=newAccMean;
		}
	size_t getNumSamples(void) const // Returns the number of accumulated samples
		{
		return numSamples;
		}
	double calcMean(void) const // Returns the current mean value
		{
		return accMean;
		}
	double calcVariance(void) const // Returns the current variance value; only valid if getNumSamples()>=2
		{
		return accVariance/double(numSamples-1);
		}
	double calcStdDeviation(void) const // Returns the current standard deviation value; only valid if getNumSamples()>=2
		{
		return sqrt(accVariance/double(numSamples-1));
		}
	};

}

#endif
