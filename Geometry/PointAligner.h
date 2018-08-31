/***********************************************************************
PointAligner - Generic base class for least-squares optimization kernels
for point set alignment compatible with the generic minimization classes
in the Math library.
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

#ifndef GEOMETRY_POINTALIGNER_INCLUDED
#define GEOMETRY_POINTALIGNER_INCLUDED

#include <utility>
#include <vector>
#include <Geometry/Point.h>

namespace Geometry {

template <class ScalarParam,int dimensionParam,template <class,int> class TransformParam>
class PointAligner
	{
	/* Embedded classes: */
	public:
	typedef ScalarParam Scalar; // Type for scalar values
	static const int dimension=dimensionParam; // Dimension of point space
	typedef Geometry::Point<ScalarParam,dimensionParam> Point; // Type for points
	typedef TransformParam<ScalarParam,dimensionParam> Transform; // Type of transformation used for point alignment
	
	struct PointPair // Structure for alignment point pairs
		{
		/* Elements: */
		public:
		Point from;
		Point to;
		
		/* Constructors and destructors: */
		PointPair(const Point& sFrom,const Point& sTo) // Creates point pair from pair of points
			:from(sFrom),to(sTo)
			{
			}
		};
	
	typedef std::vector<PointPair> PointPairList; // Type for lists of point pairs
	
	/* Optimization kernel interface: */
	typedef PointPair DataPoint; // Type of data point that can be added to a generic least-squares optimization problem
	static const unsigned int numFunctionsInBatch=dimension; // Number of functions in each batch that is calculated in one go
	
	/* Elements: */
	protected:
	PointPairList pointPairs; // List of point pairs to be aligned
	Point fromCenter,toCenter; // Centroids of from and to point sets for conditioning
	Scalar fromScale,toScale; // Sizes of from and to point sets for conditioning
	Transform current; // The current alignment transformation estimate between the conditioned point sets
	
	/* Constructors and destructors: */
	public:
	PointAligner(void)
		:fromCenter(Point::origin),toCenter(Point::origin),
		 fromScale(1),toScale(1),
		 current(Transform::identity)
		{
		}
	
	/* Methods: */
	void clearPointPairs(void) // Clears the alignment set
		{
		pointPairs.clear();
		}
	void addPointPair(const Point& newFrom,const Point& newTo) // Adds the given point pair to the alignment set
		{
		/* Append the new pair to the list: */
		pointPairs.push_back(PointPair(newFrom,newTo));
		}
	void addPointPair(const PointPair& newPair) // Ditto
		{
		/* Append the new pair to the list: */
		pointPairs.push_back(newPair);
		}
	const PointPairList& getPointPairs(void) const // Returns the list of point pairs
		{
		return pointPairs;
		}
	size_t getNumPointPairs(void) const // Returns the number of point pairs
		{
		return pointPairs.size();
		}
	const Point& getFrom(size_t index) const // Returns one of the "from" points
		{
		return pointPairs[index].from;
		}
	const Point& getTo(size_t index) const // Returns one of the "to" points
		{
		return pointPairs[index].to;
		}
	void condition(void); // Calculates conditioning transformations for the two point sets
	Scalar calcSqrDist(const PointPair& pp,const Transform& transform) const // Returns the squared distance between the given from and to points based on the given (de-normalized) transformation
		{
		return sqrDist(pp.to,transform.transform(pp.from));
		}
	std::pair<Scalar,Scalar> calcResidualToSpace(const Transform& transform) const; // Returns L2 (RMS) and LInf (maximum) residuals, in that order, of the given (de-normalized) alignment transformation estimate in "to" space
	std::pair<Scalar,Scalar> calcResidualFromSpace(const Transform& transform) const; // Returns L2 (RMS) and LInf (maximum) residuals, in that order, of the given (de-normalized) alignment transformation estimate in "from" space
	
	/* Optimization kernel methods: */
	unsigned int getNumBatches(void) const // Returns the number of function batches whose sum of squares should be minimized
		{
		return (unsigned int)pointPairs.size();
		}
	void calcValueBatch(unsigned int batchIndex,Scalar values[numFunctionsInBatch]) const; // Returns the values of a related batch of optimization target functions
	};

}

#ifndef GEOMETRY_POINTALIGNER_IMPLEMENTATION
#include <Geometry/PointAligner.icpp>
#endif

#endif
