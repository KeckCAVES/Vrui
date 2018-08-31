/***********************************************************************
RanSaC - Generic class implementing a "RANdom SAmple Consensus"
algorithm for problems where a set of data points is to be fitted to a
model in a least-squares optimal sense.
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

#ifndef MATH_RANSAC_INCLUDED
#define MATH_RANSAC_INCLUDED

#include <stddef.h>
#include <vector>
#include <Math/Constants.h>

/*****************************************
Required interface of ModelFitter classes:
*****************************************/

#if 0 // This is not actual code

class ModelFitter
	{
	/* Embedded classes: */
	public:
	typedef Scalar; // Scalar type for model and data space
	
	class DataPoint // Class representing a single data point to be fitted to a model
		{
		/* Constructors and destructors: */
		public:
		DataPoint(const DataPoint& source); // Copy constructor
		};
	
	class Model // Class representing a model fit to a set of data points
		{
		/* Constructors and destructors: */
		public:
		Model(void); // Creates a model
		Model(const Model& source); // Copy constructor
		Model& operator=(const Model& source); // Assignment operator
		};
	
	/* Methods: */
	size_t getMinNumDataPoints(void) const; // Returns the minimum number of data points required to calculate an initial model fit
	void clearDataPoints(void); // Clears the model fitter's list of data points
	void addDataPoint(const DataPoint& newDataPoint); // Adds a data point to be fitted to a model
	Model fitModel(void); // Fits a model to the current set of data points
	Scalar calcSqrDist(const DataPoint& dataPoint,const Model& model) const; // Returns the squared distance of the given data point from the given model
	};

#endif

namespace Math {

template <class ModelFitterParam>
class RanSaC
	{
	/* Embedded classes: */
	public:
	typedef ModelFitterParam ModelFitter; // Type of model fitter
	typedef typename ModelFitter::Scalar Scalar; // Scalar type of model-fitting problem
	typedef typename ModelFitter::DataPoint DataPoint; // Type representing a data point to be fit to a model
	typedef typename ModelFitter::Model Model; // Type representing a data model
	typedef std::vector<DataPoint> DataPointList; // Type for lists of data points
	
	/* Elements: */
	
	/* Optimization parameters (public because there are no invariants): */
	size_t maxNumIterations; // Maximum number of RanSaC iterations
	Scalar maxInlierDist2; // Squared maximum inlier distance
	double minInlierRatio; // Minimum ratio of inliers to total points to consider a candiate model a fit
	
	private:
	DataPointList dataPoints; // The entire set of data points to which a model is to be fitted
	Model current; // The current model
	size_t currentNumInliers; // Number of inlier data points w.r.t. the current model
	std::vector<bool> currentInliers; // Array of flags whether each data point is an inlier
	Scalar currentSqrResidual; // Squared model fitting residual of current model
	
	/* Constructors and destructors: */
	public:
	RanSaC(void) // Creates an empty RanSaC fitter with default fitting parameters
		:maxNumIterations(100),
		 maxInlierDist2(1),
		 minInlierRatio(0.5),
		 currentNumInliers(0),currentSqrResidual(Constants<Scalar>::max)
		{
		}
	RanSaC(size_t sMaxNumIterations,Scalar sMaxInlierDist2,double sMinInlierRatio) // Creates an empty RanSaC fitter with the given parameters
		:maxNumIterations(sMaxNumIterations),
		 maxInlierDist2(sMaxInlierDist2),
		 minInlierRatio(sMinInlierRatio),
		 currentNumInliers(0),currentSqrResidual(Constants<Scalar>::max)
		{
		}
	
	/* Methods: */
	void addDataPoint(const DataPoint& newDataPoint) // Adds another data point to the RanSaC fitter
		{
		dataPoints.push_back(newDataPoint);
		currentInliers.push_back(false);
		}
	const DataPointList& getDataPoints(void) const // Returns the list of data points
		{
		return dataPoints;
		}
	void fitModel(ModelFitter& modelFitter); // Fits a model to the current set of data points using the given model fitter
	const Model& getModel(void) const // Returns the current model
		{
		return current;
		}
	size_t getNumInliers(void) const // Returns the number of inlier data points w.r.t. the current model
		{
		return currentNumInliers;
		}
	const std::vector<bool>& getCurrentInliers(void) const // Returns the array of inlier flags w.r.t. the current model
		{
		return currentInliers;
		}
	Scalar getSqrResidual(void) const // Returns the squared model fitting residual w.r.t. the current model
		{
		return currentSqrResidual;
		}
	};

}

#ifndef MATH_RANSAC_IMPLEMENTATION
#include <Math/RanSaC.icpp>
#endif

#endif
