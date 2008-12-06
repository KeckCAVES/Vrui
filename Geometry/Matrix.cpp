/***********************************************************************
Matrix - Class for n x m matrices, used internally in the implementation
of affine and projective transformations.
Copyright (c) 2001-2005 Oliver Kreylos

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

#define GEOMETRY_MATRIX_IMPLEMENTATION

#ifndef METHODPREFIX
	#ifdef NONSTANDARD_TEMPLATES
		#define METHODPREFIX inline
	#else
		#define METHODPREFIX
	#endif
#endif

#include <Math/Math.h>

#include <Geometry/Matrix.h>

namespace Geometry {

/***************************
Get matrix helper functions:
***************************/

#include <Geometry/MatrixHelperFunctions.h>

/*********************************
Methods of class MatrixOperations:
*********************************/

template <class ScalarParam,int dimensionParam>
METHODPREFIX
ComponentArray<ScalarParam,dimensionParam>
MatrixOperations<ScalarParam,dimensionParam,dimensionParam>::divide(
	const ComponentArray<ScalarParam,dimensionParam>& ca,
	const ScalarParam m[dimensionParam][dimensionParam])
	{
	/* Create the extended matrix: */
	double temp[dimensionParam][dimensionParam+1];
	for(int i=0;i<dimensionParam;++i)
		{
		for(int j=0;j<dimensionParam;++j)
			temp[i][j]=double(m[i][j]);
		temp[i][dimensionParam]=double(ca[i]);
		}
	
	/* Perform Gaussian elimination with column pivoting on the extended matrix: */
	gaussElimination<dimensionParam,dimensionParam+1>(temp);
	
	/* Return the result vector: */
	ComponentArray<ScalarParam,dimensionParam> result;
	for(int i=dimensionParam-1;i>=0;--i)
		{
		for(int j=i+1;j<dimensionParam;++j)
			temp[i][dimensionParam]-=temp[i][j]*temp[j][dimensionParam];
		temp[i][dimensionParam]/=temp[i][i];
		result[i]=ScalarParam(temp[i][dimensionParam]);
		}
	return result;
	}

template <class ScalarParam,int dimensionParam>
METHODPREFIX
double
MatrixOperations<ScalarParam,dimensionParam,dimensionParam>::determinant(
	const ScalarParam m[dimensionParam][dimensionParam])
	{
	/* Create the temporary matrix: */
	double temp[dimensionParam][dimensionParam];
	for(int i=0;i<dimensionParam;++i)
		for(int j=0;j<dimensionParam;++j)
			temp[i][j]=double(m[i][j]);
	
	/* Perform Gaussian elimination with column pivoting on the temporary matrix: */
	double result=1.0;
	for(int step=0;step<dimensionParam-1;++step)
		{
		/* Find the column pivot: */
		double pivot=Math::abs(temp[step][step]);
		int pivotRow=step;
		for(int i=step+1;i<dimensionParam;++i)
			{
			double val=Math::abs(temp[i][step]);
			if(pivot<val)
				{
				pivot=val;
				pivotRow=i;
				}
			}
		
		/* Swap current and pivot rows if necessary: */
		if(pivotRow!=step)
			{
			/* Swap rows step and pivotRow: */
			for(int j=0;j<dimensionParam;++j)
				{
				double t=temp[step][j];
				temp[step][j]=temp[pivotRow][j];
				temp[pivotRow][j]=t;
				}
			
			/* Negate the result: */
			result=-result;
			}
		
		/* Combine all rows with the current row: */
		for(int i=step+1;i<dimensionParam;++i)
			{
			/* Combine rows i and step: */
			double factor=-temp[i][step]/temp[step][step];
			for(int j=step+1;j<dimensionParam;++j)
				temp[i][j]+=temp[step][j]*factor;
			}
		}
	
	/* Calculate the determinant: */
	for(int i=0;i<dimensionParam;++i)
		result*=temp[i][i];
	return result;
	}

template <class ScalarParam,int dimensionParam>
METHODPREFIX
void
MatrixOperations<ScalarParam,dimensionParam,dimensionParam>::invert(
	const ScalarParam m[dimensionParam][dimensionParam],
	ScalarParam result[dimensionParam][dimensionParam])
	{
	/* Create the extended matrix: */
	double temp[dimensionParam][dimensionParam*2];
	for(int i=0;i<dimensionParam;++i)
		for(int j=0;j<dimensionParam;++j)
			{
			temp[i][j]=double(m[i][j]);
			temp[i][dimensionParam+j]=i==j?1.0:0.0;
			}
	
	/* Perform Gaussian elimination with column pivoting on the extended matrix: */
	gaussElimination<dimensionParam,dimensionParam*2>(temp);
	
	/* Calculate the result matrix: */
	for(int i=dimensionParam-1;i>=0;--i)
		for(int j=0;j<dimensionParam;++j)
			{
			for(int k=i+1;k<dimensionParam;++k)
				temp[i][dimensionParam+j]-=temp[i][k]*temp[k][dimensionParam+j];
			temp[i][dimensionParam+j]/=temp[i][i];
			result[i][j]=ScalarParam(temp[i][dimensionParam+j]);
			}
	}

template <class ScalarParam>
METHODPREFIX
void
MatrixOperations<ScalarParam,3,3>::invert(
	const ScalarParam m[3][3],
	ScalarParam result[3][3])
	{
	double sub[3][3];
	sub[0][0]=double(m[1][1])*double(m[2][2])-double(m[2][1])*double(m[1][2]);
	sub[0][1]=double(m[1][2])*double(m[2][0])-double(m[2][2])*double(m[1][0]);
	sub[0][2]=double(m[1][0])*double(m[2][1])-double(m[2][0])*double(m[1][1]);
	sub[1][0]=double(m[2][1])*double(m[0][2])-double(m[0][1])*double(m[2][2]);
	sub[1][1]=double(m[2][2])*double(m[0][0])-double(m[0][2])*double(m[2][0]);
	sub[1][2]=double(m[2][0])*double(m[0][1])-double(m[0][0])*double(m[2][1]);
	sub[2][0]=double(m[0][1])*double(m[1][2])-double(m[1][1])*double(m[0][2]);
	sub[2][1]=double(m[0][2])*double(m[1][0])-double(m[1][2])*double(m[0][0]);
	sub[2][2]=double(m[0][0])*double(m[1][1])-double(m[1][0])*double(m[0][1]);
	double det=double(m[0][0])*sub[0][0]+double(m[1][0])*sub[1][0]+double(m[2][0])*sub[2][0];
	for(int i=0;i<3;++i)
		for(int j=0;j<3;++j)
			result[i][j]=ScalarParam(sub[j][i]/det);
	}

/*******************************
Static elements of class Matrix:
*******************************/

template <class ScalarParam,int numRowsParam,int numColumnsParam>
const Matrix<ScalarParam,numRowsParam,numColumnsParam> Matrix<ScalarParam,numRowsParam,numColumnsParam>::zero(ScalarParam(0));

template <class ScalarParam,int numRowsParam,int numColumnsParam>
const Matrix<ScalarParam,numRowsParam,numColumnsParam> Matrix<ScalarParam,numRowsParam,numColumnsParam>::one(ScalarParam(1));

/***********************
Methods of class Matrix:
***********************/

template <class ScalarParam,int numRowsParam,int numColumnsParam>
template <class SourceScalarParam>
METHODPREFIX Matrix<ScalarParam,numRowsParam,numColumnsParam> Matrix<ScalarParam,numRowsParam,numColumnsParam>::fromRowMajor(const SourceScalarParam* components)
	{
	/* Copy entries from the given array into the result matrix: */
	Matrix result;
	for(int i=0;i<numRows;++i)
		for(int j=0;j<numColumns;++j,++components)
			result.c[i][j]=Scalar(*components);
	return result;
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
template <class SourceScalarParam>
METHODPREFIX Matrix<ScalarParam,numRowsParam,numColumnsParam> Matrix<ScalarParam,numRowsParam,numColumnsParam>::fromColumnMajor(const SourceScalarParam* components)
	{
	/* Copy entries from the given array into the result matrix: */
	Matrix result;
	for(int j=0;j<numColumns;++j)
		for(int i=0;i<numRows;++i,++components)
			result.c[i][j]=Scalar(*components);
	return result;
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
template <class SourceScalarParam>
METHODPREFIX Matrix<ScalarParam,numRowsParam,numColumnsParam>::Matrix(const Matrix<SourceScalarParam,numRowsParam,numColumnsParam>& source)
	{
	for(int i=0;i<numRows;++i)
		for(int j=0;j<numColumns;++j)
			c[i][j]=Scalar(source(i,j));
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX Matrix<ScalarParam,numRowsParam,numColumnsParam> Matrix<ScalarParam,numRowsParam,numColumnsParam>::operator-(void) const
	{
	Matrix result;
	for(int i=0;i<numRows;++i)
		for(int j=0;j<numColumns;++j)
			result.c[i][j]=-c[i][j];
	return result;
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX Matrix<ScalarParam,numRowsParam,numColumnsParam>& Matrix<ScalarParam,numRowsParam,numColumnsParam>::operator+=(const Matrix<ScalarParam,numRowsParam,numColumnsParam>& other)
	{
	for(int i=0;i<numRows;++i)
		for(int j=0;j<numColumns;++j)
			c[i][j]+=other(i,j);
	return *this;
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX Matrix<ScalarParam,numRowsParam,numColumnsParam>& Matrix<ScalarParam,numRowsParam,numColumnsParam>::operator-=(const Matrix<ScalarParam,numRowsParam,numColumnsParam>& other)
	{
	for(int i=0;i<numRows;++i)
		for(int j=0;j<numColumns;++j)
			c[i][j]-=other(i,j);
	return *this;
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX Matrix<ScalarParam,numRowsParam,numColumnsParam>& Matrix<ScalarParam,numRowsParam,numColumnsParam>::operator*=(ScalarParam scalar)
	{
	for(int i=0;i<numRows;++i)
		for(int j=0;j<numColumns;++j)
			c[i][j]*=scalar;
	return *this;
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX Matrix<ScalarParam,numRowsParam,numColumnsParam>& Matrix<ScalarParam,numRowsParam,numColumnsParam>::operator*=(const Matrix<ScalarParam,numColumnsParam,numColumnsParam>& other)
	{
	Scalar temp[numRows][numColumns];
	for(int i=0;i<numRows;++i)
		for(int j=0;j<numColumns;++j)
			{
			temp[i][j]=Scalar(0);
			for(int k=0;k<numColumns;++k)
				temp[i][j]+=c[i][k]*other(k,j);
			}
	for(int i=0;i<numRows;++i)
		for(int j=0;j<numColumns;++j)
			c[i][j]=temp[i][j];
	return *this;
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX Matrix<ScalarParam,numRowsParam,numColumnsParam>& Matrix<ScalarParam,numRowsParam,numColumnsParam>::transposeMultiply(const Matrix<ScalarParam,numColumnsParam,numColumnsParam>& other)
	{
	Scalar temp[numRows][numColumns];
	for(int i=0;i<numRows;++i)
		for(int j=0;j<numColumns;++j)
			{
			temp[i][j]=Scalar(0);
			for(int k=0;k<numColumns;++k)
				temp[i][j]+=c[i][k]*other(j,k);
			}
	for(int i=0;i<numRows;++i)
		for(int j=0;j<numColumns;++j)
			c[i][j]=temp[i][j];
	return *this;
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX Matrix<ScalarParam,numRowsParam,numColumnsParam>& Matrix<ScalarParam,numRowsParam,numColumnsParam>::leftMultiply(const Matrix<ScalarParam,numRowsParam,numRowsParam>& other)
	{
	Scalar temp[numRows][numColumns];
	for(int i=0;i<numRows;++i)
		for(int j=0;j<numColumns;++j)
			{
			temp[i][j]=Scalar(0);
			for(int k=0;k<numRows;++k)
				temp[i][j]+=other(i,k)*c[k][j];
			}
	for(int i=0;i<numRows;++i)
		for(int j=0;j<numColumns;++j)
			c[i][j]=temp[i][j];
	return *this;
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX Matrix<ScalarParam,numRowsParam,numColumnsParam>& Matrix<ScalarParam,numRowsParam,numColumnsParam>::transposeLeftMultiply(const Matrix<ScalarParam,numRowsParam,numRowsParam>& other)
	{
	Scalar temp[numRows][numColumns];
	for(int i=0;i<numRows;++i)
		for(int j=0;j<numColumns;++j)
			{
			temp[i][j]=Scalar(0);
			for(int k=0;k<numRows;++k)
				temp[i][j]+=other(k,i)*c[k][j];
			}
	for(int i=0;i<numRows;++i)
		for(int j=0;j<numColumns;++j)
			c[i][j]=temp[i][j];
	return *this;
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX Matrix<ScalarParam,numRowsParam,numColumnsParam>& Matrix<ScalarParam,numRowsParam,numColumnsParam>::operator/=(ScalarParam scalar)
	{
	for(int i=0;i<numRows;++i)
		for(int j=0;j<numColumns;++j)
			c[i][j]/=scalar;
	return *this;
	}

/**********************************************
Functions operating on objects of class Matrix:
**********************************************/

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX bool operator==(const Matrix<ScalarParam,numRowsParam,numColumnsParam>& m1,const Matrix<ScalarParam,numRowsParam,numColumnsParam>& m2)
	{
	for(int i=0;i<numRowsParam;++i)
		for(int j=0;j<numColumnsParam;++j)
			if(m1(i,j)!=m2(i,j))
				return false;
	return true;
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX bool operator!=(const Matrix<ScalarParam,numRowsParam,numColumnsParam>& m1,const Matrix<ScalarParam,numRowsParam,numColumnsParam>& m2)
	{
	for(int i=0;i<numRowsParam;++i)
		for(int j=0;j<numColumnsParam;++j)
			if(m1(i,j)!=m2(i,j))
				return true;
	return false;
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX Matrix<ScalarParam,numRowsParam,numColumnsParam> operator+(const Matrix<ScalarParam,numRowsParam,numColumnsParam>& m1,const Matrix<ScalarParam,numRowsParam,numColumnsParam>& m2)
	{
	Matrix<ScalarParam,numRowsParam,numColumnsParam> result;
	for(int i=0;i<numRowsParam;++i)
		for(int j=0;j<numColumnsParam;++j)
			result(i,j)=m1(i,j)+m2(i,j);
	return result;
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX Matrix<ScalarParam,numRowsParam,numColumnsParam> operator-(const Matrix<ScalarParam,numRowsParam,numColumnsParam>& m1,const Matrix<ScalarParam,numRowsParam,numColumnsParam>& m2)
	{
	Matrix<ScalarParam,numRowsParam,numColumnsParam> result;
	for(int i=0;i<numRowsParam;++i)
		for(int j=0;j<numColumnsParam;++j)
			result(i,j)=m1(i,j)-m2(i,j);
	return result;
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX Matrix<ScalarParam,numRowsParam,numColumnsParam> operator*(const Matrix<ScalarParam,numRowsParam,numColumnsParam>& m,ScalarParam scalar)
	{
	Matrix<ScalarParam,numRowsParam,numColumnsParam> result;
	for(int i=0;i<numRowsParam;++i)
		for(int j=0;j<numColumnsParam;++j)
			result(i,j)=m(i,j)*scalar;
	return result;
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX Matrix<ScalarParam,numRowsParam,numColumnsParam> operator*(ScalarParam scalar,const Matrix<ScalarParam,numRowsParam,numColumnsParam>& m)
	{
	Matrix<ScalarParam,numRowsParam,numColumnsParam> result;
	for(int i=0;i<numRowsParam;++i)
		for(int j=0;j<numColumnsParam;++j)
			result(i,j)=scalar*m(i,j);
	return result;
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX Matrix<ScalarParam,numRowsParam,numColumnsParam> operator/(const Matrix<ScalarParam,numRowsParam,numColumnsParam>& m,ScalarParam scalar)
	{
	Matrix<ScalarParam,numRowsParam,numColumnsParam> result;
	for(int i=0;i<numRowsParam;++i)
		for(int j=0;j<numColumnsParam;++j)
			result(i,j)=m(i,j)/scalar;
	return result;
	}

template <class ScalarParam,int numRowsParam,int middleParam,int numColumnsParam>
METHODPREFIX Matrix<ScalarParam,numRowsParam,numColumnsParam> operator*(const Matrix<ScalarParam,numRowsParam,middleParam>& m1,const Matrix<ScalarParam,middleParam,numColumnsParam>& m2)
	{
	Matrix<ScalarParam,numRowsParam,numColumnsParam> result;
	for(int i=0;i<numRowsParam;++i)
		for(int j=0;j<numColumnsParam;++j)
			{
			ScalarParam sum(0);
			for(int k=0;k<middleParam;++k)
				sum+=m1(i,k)*m2(k,j);
			result(i,j)=sum;
			}
	return result;
	}

template <class ScalarParam,int numRowsParam,int numColumnsParam>
METHODPREFIX Matrix<ScalarParam,numColumnsParam,numRowsParam> transpose(const Matrix<ScalarParam,numRowsParam,numColumnsParam>& m)
	{
	Matrix<ScalarParam,numColumnsParam,numRowsParam> result;
	for(int i=0;i<numRowsParam;++i)
		for(int j=0;j<numColumnsParam;++j)
			result(j,i)=m(i,j);
	return result;
	}

#if !defined(NONSTANDARD_TEMPLATES)

/****************************************************************
Force instantiation of all standard Matrix classes and functions:
****************************************************************/

template class MatrixOperations<float,2,2>;
template class MatrixOperations<float,2,3>;
template class MatrixOperations<float,3,3>;
template class MatrixOperations<float,3,4>;
template class MatrixOperations<float,4,4>;
template class MatrixOperations<double,2,2>;
template class MatrixOperations<double,2,3>;
template class MatrixOperations<double,3,3>;
template class MatrixOperations<double,3,4>;
template class MatrixOperations<double,4,4>;

template class Matrix<float,2,2>;
template Matrix<float,2,2> Matrix<float,2,2>::fromRowMajor(const float*);
template Matrix<float,2,2> Matrix<float,2,2>::fromRowMajor(const double*);
template Matrix<float,2,2> Matrix<float,2,2>::fromColumnMajor(const float*);
template Matrix<float,2,2> Matrix<float,2,2>::fromColumnMajor(const double*);
template Matrix<float,2,2>::Matrix(const Matrix<double,2,2>&);
template Matrix<float,2,2> operator+(const Matrix<float,2,2>&,const Matrix<float,2,2>&);
template Matrix<float,2,2> operator-(const Matrix<float,2,2>&,const Matrix<float,2,2>&);
template Matrix<float,2,2> operator*(const Matrix<float,2,2>&,float);
template Matrix<float,2,2> operator*(float,const Matrix<float,2,2>&);
template Matrix<float,2,2> operator/(const Matrix<float,2,2>&,float);
template Matrix<float,2,2> operator*(const Matrix<float,2,2>&,const Matrix<float,2,2>&);
template Matrix<float,2,2> transpose(const Matrix<float,2,2>&);

template class Matrix<float,2,3>;
template Matrix<float,2,3> Matrix<float,2,3>::fromRowMajor(const float*);
template Matrix<float,2,3> Matrix<float,2,3>::fromRowMajor(const double*);
template Matrix<float,2,3> Matrix<float,2,3>::fromColumnMajor(const float*);
template Matrix<float,2,3> Matrix<float,2,3>::fromColumnMajor(const double*);
template Matrix<float,2,3>::Matrix(const Matrix<double,2,3>&);
template Matrix<float,2,3> operator+(const Matrix<float,2,3>&,const Matrix<float,2,3>&);
template Matrix<float,2,3> operator-(const Matrix<float,2,3>&,const Matrix<float,2,3>&);
template Matrix<float,2,3> operator*(const Matrix<float,2,3>&,float);
template Matrix<float,2,3> operator*(float,const Matrix<float,2,3>&);
template Matrix<float,2,3> operator/(const Matrix<float,2,3>&,float);
template Matrix<float,2,3> operator*(const Matrix<float,2,3>&,const Matrix<float,3,3>&);
template Matrix<float,2,3> operator*(const Matrix<float,2,2>&,const Matrix<float,2,3>&);

template class Matrix<float,3,3>;
template Matrix<float,3,3> Matrix<float,3,3>::fromRowMajor(const float*);
template Matrix<float,3,3> Matrix<float,3,3>::fromRowMajor(const double*);
template Matrix<float,3,3> Matrix<float,3,3>::fromColumnMajor(const float*);
template Matrix<float,3,3> Matrix<float,3,3>::fromColumnMajor(const double*);
template Matrix<float,3,3>::Matrix(const Matrix<double,3,3>&);
template Matrix<float,3,3> operator+(const Matrix<float,3,3>&,const Matrix<float,3,3>&);
template Matrix<float,3,3> operator-(const Matrix<float,3,3>&,const Matrix<float,3,3>&);
template Matrix<float,3,3> operator*(const Matrix<float,3,3>&,float);
template Matrix<float,3,3> operator*(float,const Matrix<float,3,3>&);
template Matrix<float,3,3> operator/(const Matrix<float,3,3>&,float);
template Matrix<float,3,3> operator*(const Matrix<float,3,3>&,const Matrix<float,3,3>&);
template Matrix<float,3,3> transpose(const Matrix<float,3,3>&);

template class Matrix<float,3,4>;
template Matrix<float,3,4> Matrix<float,3,4>::fromRowMajor(const float*);
template Matrix<float,3,4> Matrix<float,3,4>::fromRowMajor(const double*);
template Matrix<float,3,4> Matrix<float,3,4>::fromColumnMajor(const float*);
template Matrix<float,3,4> Matrix<float,3,4>::fromColumnMajor(const double*);
template Matrix<float,3,4>::Matrix(const Matrix<double,3,4>&);
template Matrix<float,3,4> operator+(const Matrix<float,3,4>&,const Matrix<float,3,4>&);
template Matrix<float,3,4> operator-(const Matrix<float,3,4>&,const Matrix<float,3,4>&);
template Matrix<float,3,4> operator*(const Matrix<float,3,4>&,float);
template Matrix<float,3,4> operator*(float,const Matrix<float,3,4>&);
template Matrix<float,3,4> operator/(const Matrix<float,3,4>&,float);
template Matrix<float,3,4> operator*(const Matrix<float,3,4>&,const Matrix<float,4,4>&);
template Matrix<float,3,4> operator*(const Matrix<float,3,3>&,const Matrix<float,3,4>&);

template class Matrix<float,4,4>;
template Matrix<float,4,4> Matrix<float,4,4>::fromRowMajor(const float*);
template Matrix<float,4,4> Matrix<float,4,4>::fromRowMajor(const double*);
template Matrix<float,4,4> Matrix<float,4,4>::fromColumnMajor(const float*);
template Matrix<float,4,4> Matrix<float,4,4>::fromColumnMajor(const double*);
template Matrix<float,4,4>::Matrix(const Matrix<double,4,4>&);
template Matrix<float,4,4> operator+(const Matrix<float,4,4>&,const Matrix<float,4,4>&);
template Matrix<float,4,4> operator-(const Matrix<float,4,4>&,const Matrix<float,4,4>&);
template Matrix<float,4,4> operator*(const Matrix<float,4,4>&,float);
template Matrix<float,4,4> operator*(float,const Matrix<float,4,4>&);
template Matrix<float,4,4> operator/(const Matrix<float,4,4>&,float);
template Matrix<float,4,4> operator*(const Matrix<float,4,4>&,const Matrix<float,4,4>&);
template Matrix<float,4,4> transpose(const Matrix<float,4,4>&);

template class Matrix<double,2,2>;
template Matrix<double,2,2> Matrix<double,2,2>::fromRowMajor(const float*);
template Matrix<double,2,2> Matrix<double,2,2>::fromRowMajor(const double*);
template Matrix<double,2,2> Matrix<double,2,2>::fromColumnMajor(const float*);
template Matrix<double,2,2> Matrix<double,2,2>::fromColumnMajor(const double*);
template Matrix<double,2,2>::Matrix(const Matrix<float,2,2>&);
template Matrix<double,2,2> operator+(const Matrix<double,2,2>&,const Matrix<double,2,2>&);
template Matrix<double,2,2> operator-(const Matrix<double,2,2>&,const Matrix<double,2,2>&);
template Matrix<double,2,2> operator*(const Matrix<double,2,2>&,double);
template Matrix<double,2,2> operator*(double,const Matrix<double,2,2>&);
template Matrix<double,2,2> operator/(const Matrix<double,2,2>&,double);
template Matrix<double,2,2> operator*(const Matrix<double,2,2>&,const Matrix<double,2,2>&);
template Matrix<double,2,2> transpose(const Matrix<double,2,2>&);

template class Matrix<double,2,3>;
template Matrix<double,2,3> Matrix<double,2,3>::fromRowMajor(const float*);
template Matrix<double,2,3> Matrix<double,2,3>::fromRowMajor(const double*);
template Matrix<double,2,3> Matrix<double,2,3>::fromColumnMajor(const float*);
template Matrix<double,2,3> Matrix<double,2,3>::fromColumnMajor(const double*);
template Matrix<double,2,3>::Matrix(const Matrix<float,2,3>&);
template Matrix<double,2,3> operator+(const Matrix<double,2,3>&,const Matrix<double,2,3>&);
template Matrix<double,2,3> operator-(const Matrix<double,2,3>&,const Matrix<double,2,3>&);
template Matrix<double,2,3> operator*(const Matrix<double,2,3>&,double);
template Matrix<double,2,3> operator*(double,const Matrix<double,2,3>&);
template Matrix<double,2,3> operator/(const Matrix<double,2,3>&,double);
template Matrix<double,2,3> operator*(const Matrix<double,2,3>&,const Matrix<double,3,3>&);
template Matrix<double,2,3> operator*(const Matrix<double,2,2>&,const Matrix<double,2,3>&);

template class Matrix<double,3,3>;
template Matrix<double,3,3> Matrix<double,3,3>::fromRowMajor(const float*);
template Matrix<double,3,3> Matrix<double,3,3>::fromRowMajor(const double*);
template Matrix<double,3,3> Matrix<double,3,3>::fromColumnMajor(const float*);
template Matrix<double,3,3> Matrix<double,3,3>::fromColumnMajor(const double*);
template Matrix<double,3,3>::Matrix(const Matrix<float,3,3>&);
template Matrix<double,3,3> operator+(const Matrix<double,3,3>&,const Matrix<double,3,3>&);
template Matrix<double,3,3> operator-(const Matrix<double,3,3>&,const Matrix<double,3,3>&);
template Matrix<double,3,3> operator*(const Matrix<double,3,3>&,double);
template Matrix<double,3,3> operator*(double,const Matrix<double,3,3>&);
template Matrix<double,3,3> operator/(const Matrix<double,3,3>&,double);
template Matrix<double,3,3> operator*(const Matrix<double,3,3>&,const Matrix<double,3,3>&);
template Matrix<double,3,3> transpose(const Matrix<double,3,3>&);

template class Matrix<double,3,4>;
template Matrix<double,3,4> Matrix<double,3,4>::fromRowMajor(const float*);
template Matrix<double,3,4> Matrix<double,3,4>::fromRowMajor(const double*);
template Matrix<double,3,4> Matrix<double,3,4>::fromColumnMajor(const float*);
template Matrix<double,3,4> Matrix<double,3,4>::fromColumnMajor(const double*);
template Matrix<double,3,4>::Matrix(const Matrix<float,3,4>&);
template Matrix<double,3,4> operator+(const Matrix<double,3,4>&,const Matrix<double,3,4>&);
template Matrix<double,3,4> operator-(const Matrix<double,3,4>&,const Matrix<double,3,4>&);
template Matrix<double,3,4> operator*(const Matrix<double,3,4>&,double);
template Matrix<double,3,4> operator*(double,const Matrix<double,3,4>&);
template Matrix<double,3,4> operator/(const Matrix<double,3,4>&,double);
template Matrix<double,3,4> operator*(const Matrix<double,3,4>&,const Matrix<double,4,4>&);
template Matrix<double,3,4> operator*(const Matrix<double,3,3>&,const Matrix<double,3,4>&);

template class Matrix<double,4,4>;
template Matrix<double,4,4> Matrix<double,4,4>::fromRowMajor(const float*);
template Matrix<double,4,4> Matrix<double,4,4>::fromRowMajor(const double*);
template Matrix<double,4,4> Matrix<double,4,4>::fromColumnMajor(const float*);
template Matrix<double,4,4> Matrix<double,4,4>::fromColumnMajor(const double*);
template Matrix<double,4,4>::Matrix(const Matrix<float,4,4>&);
template Matrix<double,4,4> operator+(const Matrix<double,4,4>&,const Matrix<double,4,4>&);
template Matrix<double,4,4> operator-(const Matrix<double,4,4>&,const Matrix<double,4,4>&);
template Matrix<double,4,4> operator*(const Matrix<double,4,4>&,double);
template Matrix<double,4,4> operator*(double,const Matrix<double,4,4>&);
template Matrix<double,4,4> operator/(const Matrix<double,4,4>&,double);
template Matrix<double,4,4> operator*(const Matrix<double,4,4>&,const Matrix<double,4,4>&);
template Matrix<double,4,4> transpose(const Matrix<double,4,4>&);

#endif

}
