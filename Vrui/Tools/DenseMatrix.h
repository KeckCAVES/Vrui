/***********************************************************************
DenseMatrix - Helper class to solve systems of dense linear equations.
Copyright (c) 2000-2009 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef VRUI_DENSEMATRIX_INCLUDED
#define VRUI_DENSEMATRIX_INCLUDED

#include <stdexcept>

namespace Vrui {

class DenseMatrix
	{
	/* Embedded classes: */
	public:
	struct Error:public std::runtime_error // Generic exception class to signal errors when handling matrices
		{
		/* Constructors and destructors: */
		public:
		Error(const char* errorMsg)
			:std::runtime_error(errorMsg)
			{
			}
		};
	
	struct IndexError:public Error // Exception class to signal out-of-bounds indices
		{
		/* Constructors and destructors: */
		public:
		IndexError(void)
			:Error("Matrix access index out of range")
			{
			}
		};
	
	struct SizeMismatchError:public Error // Exception class to signal mismatching matrices
		{
		/* Constructors and destructors: */
		public:
		SizeMismatchError(void)
			:Error("Attempt to perform operations on matrices of mismatching sizes")
			{
			}
		};
	
	struct RankDeficientError:public Error // Exception class to signal rank-deficient matrices when solving linear systems
		{
		/* Constructors and destructors: */
		public:
		RankDeficientError(void)
			:Error("Attempt to solve linear system with rank-deficient matrix")
			{
			}
		};
	
	/* Elements: */
	private:
	int numRows,numColumns; // Size of matrix
	double** rows; // Array of pointers to rows
	double* columns; // Array of column entries
	/* Private methods: */
	void resize(int newNumRows,int newNumColumns); // Resizes a matrix, does not initialize the entries
	/* Constructors and destructors: */
	public:
	DenseMatrix(int sNumRows,int sNumColumns,const double* sEntries =0); // Create (empty) matrix
	DenseMatrix(const DenseMatrix& other); // Copy constructor
	DenseMatrix& operator=(const DenseMatrix& other); // Assignment operator
	~DenseMatrix(void);
	/* Access methods: */
	int getNumRows(void) const // Returns number of rows
		{
		return numRows;
		}
	int getNumColumns(void) const // Returns number of columns
		{
		return numColumns;
		}
	const double& operator()(int i,int j) const // Returns matrix element at row i and column j
		{
		return rows[i][j];
		}
	double& operator()(int i,int j) // Ditto
		{
		return rows[i][j];
		}
	/* Conversion methods: */
	DenseMatrix getRow(int i) const; // Returns i-th row as (1,numColumns)-matrix
	DenseMatrix getColumn(int j) const; // Returns j-th column as (numRows,1)-matrix
	DenseMatrix& setRow(int i,const DenseMatrix& source); // Copies i-th row from (1,numColumns)-matrix
	DenseMatrix& setColumn(int j,const DenseMatrix& source); // Copies j-th column from (numRows,1)-matrix
	/* Algebra methods and operators: */
	friend DenseMatrix operator-(const DenseMatrix& matrix); // Negation operator (unary minus)
	friend DenseMatrix operator+(const DenseMatrix& matrix1,const DenseMatrix& matrix2); // Addition operator
	friend DenseMatrix operator-(const DenseMatrix& matrix1,const DenseMatrix& matrix2); // Subtraction operator
	friend DenseMatrix operator*(const DenseMatrix& matrix1,const DenseMatrix& matrix2); // Multiplication operator
	friend DenseMatrix& inplaceMultiplication(DenseMatrix& result,const DenseMatrix& matrix1,const DenseMatrix& matrix2); // Multiplies two matrices into existing result matrix
	friend DenseMatrix& inplaceTransposed1Multiplication(DenseMatrix& result,const DenseMatrix& matrix1,const DenseMatrix& matrix2); // Ditto, but the first matrix is transposed
	friend DenseMatrix& inplaceTransposed2Multiplication(DenseMatrix& result,const DenseMatrix& matrix1,const DenseMatrix& matrix2); // Ditto, but the second matrix is transposed
	DenseMatrix& operator*=(const DenseMatrix& matrix2); // Self-multipliciation operator. Performed in-place if matrix2 is numColumns x numColumns
	/* Other matrix methods: */
	DenseMatrix& zero(void); // Sets all entries to zero
	DenseMatrix transpose(void) const; // Returns flipped matrix
	double findColumnPivot(int start,int& pivotI) const; // Finds element of maximal absolute value in lower part of column
	double findFullPivot(int start,int& pivotI,int& pivotJ) const; // Finds element of maximal absolute value in lower right part matrix
	DenseMatrix& swapRows(int i1,int i2); // Swaps two rows
	DenseMatrix& swapColumns(int j1,int j2); // Swaps two columns
	DenseMatrix& scaleRow(int i,double factor); // Multiplies row by given factor
	DenseMatrix& combineRows(int destI,int sourceI,double factor); // Adds multiple of row sourceI to row destI
	double determinant(void) const; // Calculates square matrix' determinant
	int rank(void) const; // Returns rank of a square matrix
	DenseMatrix solveLinearEquations(const DenseMatrix& constants) const; // Solves set of linear equation systems
	void qr(DenseMatrix& q,DenseMatrix& r) const; // Computes the QR factorization of a matrix
	};

}

#endif
