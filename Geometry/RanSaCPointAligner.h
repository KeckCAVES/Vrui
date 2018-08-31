/***********************************************************************
RanSaCPointAligner - Adapter class to align two point sets using some
transformation type via the RanSaC algorithm.
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

#ifndef GEOMETRY_RANSACPOINTALIGNER_INCLUDED
#define GEOMETRY_RANSACPOINTALIGNER_INCLUDED

namespace Geometry {

template <class PointAlignerParam,template <class> class MinimizerParam>
class RanSaCPointAligner
	{
	/* Embedded classes: */
	public:
	typedef PointAlignerParam PointAligner; // Type of point aligner
	typedef typename PointAligner::Scalar Scalar; // Scalar type
	static const int dimension=PointAligner::dimension; // Dimension of point space
	typedef typename PointAligner::PointPair DataPoint; // Type for RanSaC data points
	static const unsigned int numVariables=PointAligner::numVariables; // Number of variables in the optimization problem
	typedef typename PointAligner::Transform Model; // Type for RanSaC models
	typedef MinimizerParam<PointAligner> Minimizer; // Class for least-squares optimizers
	
	/* Elements: */
	PointAligner pointAligner; // The point alignment object
	Minimizer minimizer; // The optimizer
	
	/* Methods: */
	Minimizer& getMinimizer(void) // Returns the optimizer object
		{
		return minimizer;
		}
	size_t getMinNumDataPoints(void) const // Returns the minimum number of data points required to calculate an initial model fit
		{
		/* Calculate the minimum number of required data points based on the number of model variables and dimension of the alignment space: */
		return (numVariables+(unsigned int)(dimension-1))/(unsigned int)(dimension);
		}
	void clearDataPoints(void) // Clears the model fitter's list of data points
		{
		pointAligner.clearPointPairs();
		}
	void addDataPoint(const DataPoint& newDataPoint) // Adds a data point to be fitted to a model
		{
		pointAligner.addPointPair(newDataPoint);
		}
	Model fitModel(void) // Fits a model to the current set of data points
		{
		/* Condition the point sets to increase numerical stability: */
		pointAligner.condition();
		
		/* Estimate an initial alignment transformation: */
		pointAligner.estimateTransform();
		
		/* Refine the transformation through iterative optimization: */
		minimizer.minimize(pointAligner);
		
		/* Return the final alignment transformation: */
		return pointAligner.getTransform();
		}
	Scalar calcSqrDist(const DataPoint& dataPoint,const Model& model) const // Returns the squared distance of the given data point from the given model
		{
		return pointAligner.calcSqrDist(dataPoint,model);
		}
	};

}

#endif
