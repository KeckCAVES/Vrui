/***********************************************************************
Complex - Class for complex numbers, big surprise. Why not use the C++
standard complex  type? For fun reasons,  that's why.
Copyright (c) 2002-2016 Oliver Kreylos

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

#ifndef MATH_COMPLEX_INCLUDED
#define MATH_COMPLEX_INCLUDED

#include <Math/Math.h>
#include <Math/Constants.h>

namespace Math {

template <class ScalarParam>
class Complex // Class for complex numbers
	{
	/* Embedded classes: */
	public:
	typedef ScalarParam Scalar; // Scalar type used to represent complex numbers
	
	/* Elements: */
	private:
	Scalar real,imag; // Real and imaginary components

	/* Constructors and destructors: */
	public:
	static const Complex zero; // The complex zero element
	static const Complex one; // The complex one element
	static const Complex i; // Root of -1 (might not be the best name, but so be it)
	Complex(void) // Dummy constructor
		{
		}
	Complex(Scalar sReal,Scalar sImag =Scalar(0)) // Constructs real number by default
		:real(sReal),imag(sImag)
		{
		}
	template <class SourceScalarParam>
	Complex(const Complex<SourceScalarParam>& source) // Copy constructor with type conversion
		:real(Scalar(source.getReal())),imag(Scalar(source.getImag()))
		{
		}
	template <class SourceScalarParam>
	Complex& operator=(const Complex<SourceScalarParam>& source) // Assignment operator with type conversion
		{
		real=Scalar(source.getReal());
		imag=Scalar(source.getImag());
		return *this;
		}
	
	/* Methods: */
	Scalar getReal(void) const // Returns real component
		{
		return real;
		}
	Scalar getImag(void) const // Returns imaginary component
		{
		return imag;
		}

	/* Field operators: */
	Complex operator+(void) const // Unary plus
		{
		return *this;
		}
	Complex operator-(void) const // Unary minus, negation operator
		{
		return Complex(-real,-imag);
		}
	Complex& operator+=(const Complex& other) // Addition assignment
		{
		real+=other.real;
		imag+=other.imag;
		return *this;
		}
	Complex& operator+=(Scalar other) // Ditto, with real summand
		{
		real+=other;
		return *this;
		}
	Complex& operator-=(const Complex& other) // Subtraction assignment
		{
		real-=other.real;
		imag-=other.imag;
		return *this;
		}
	Complex& operator-=(Scalar other) // Ditto, with real whatever
		{
		real-=other;
		return *this;
		}
	Complex& operator*=(const Complex& other) // Multiplication assignment
		{
		Scalar tReal=real;
		real=tReal*other.real-imag*other.imag;
		imag=tReal*other.imag+imag*other.real;
		return *this;
		}
	Complex& operator*=(Scalar other) // Ditto, with real factor
		{
		real*=other;
		imag*=other;
		return* this;
		}
	Complex& operator/=(const Complex& other) // Division assignment
		{
		Scalar denominator=sqr(other.real)+sqr(other.imag);
		Scalar tReal=real;
		real=(tReal*other.real+imag*other.imag)/denominator;
		imag=(imag*other.real-tReal*other.imag)/denominator;
		return *this;
		}
	Complex& operator/=(Scalar other) // Ditto, with real divisor
		{
		real/=other;
		imag/=other;
		return* this;
		}
	
	/* Other methods: */
	static Scalar abs(const Complex& c) // Returns magnitude of complex number
		{
		return sqrt(sqr(c.real)+sqr(c.imag));
		}
	static Complex conjugate(const Complex& c) // Returns conjugate of complex number
		{
		return Complex(c.real,-c.imag);
		}
	static Complex exp(const Complex& exponent) // Returns natural exponent of complex number
		{
		Scalar factor=exp(exponent.real);
		return Complex(factor*cos(exponent.imag),factor*sin(exponent.imag));
		}
	static Complex rootOfUnity(int exponent,int rootLevel) // Computes a root of unity for FFT
		{
		Scalar arg=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(exponent)/Scalar(rootLevel);
		return Complex(Math::cos(arg),Math::sin(arg));
		}
	};

/**************************************
Operations on objects of class Complex:
**************************************/

template <class ScalarParam>
bool operator==(const Complex<ScalarParam>& c1,const Complex<ScalarParam>& c2)
	{
	return c1.getReal()==c2.getReal()&&c1.getImag()==c2.getImag();
	}

template <class ScalarParam>
bool operator!=(const Complex<ScalarParam>& c1,const Complex<ScalarParam>& c2)
	{
	return c1.getReal()!=c2.getReal()||c1.getImag()!=c2.getImag();
	}

template <class ScalarParam>
Complex<ScalarParam> operator+(const Complex<ScalarParam>& c1,const Complex<ScalarParam>& c2)
	{
	return Complex<ScalarParam>(c1.getReal()+c2.getReal(),c1.getImag()+c2.getImag());
	}

template <class ScalarParam>
Complex<ScalarParam> operator-(const Complex<ScalarParam>& c1,const Complex<ScalarParam>& c2)
	{
	return Complex<ScalarParam>(c1.getReal()-c2.getReal(),c1.getImag()-c2.getImag());
	}

template <class ScalarParam>
Complex<ScalarParam> operator*(const Complex<ScalarParam>& c1,const Complex<ScalarParam>& c2)
	{
	return Complex<ScalarParam>(c1.getReal()*c2.getReal()-c1.getImag()*c2.getImag(),c1.getReal()*c2.getImag()+c1.getImag()*c2.getReal());
	}

template <class ScalarParam>
Complex<ScalarParam> operator*(ScalarParam r1,const Complex<ScalarParam>& c2)
	{
	return Complex<ScalarParam>(r1*c2.getReal(),r1*c2.getImag());
	}

template <class ScalarParam>
Complex<ScalarParam> operator*(const Complex<ScalarParam>& c1,ScalarParam r2)
	{
	return Complex<ScalarParam>(c1.getReal()*r2,c1.getImag()*r2);
	}

template <class ScalarParam>
Complex<ScalarParam> operator/(const Complex<ScalarParam>& c1,const Complex<ScalarParam>& c2)
	{
	ScalarParam denominator=sqr(c2.getReal())+sqr(c2.getImag());
	return Complex<ScalarParam>((c1.getReal()*c2.getReal()+c1.getImag()*c2.getImag())/denominator,(c1.getImag()*c2.getReal()-c1.getReal()*c2.getImag())/denominator);
	}

template <class ScalarParam>
Complex<ScalarParam> operator/(const Complex<ScalarParam>& c1,ScalarParam r2)
	{
	return Complex<ScalarParam>(c1.getReal()/r2,c1.getImag()/r2);
	}

/**********************************************
Specialized Constant class for complex numbers:
**********************************************/

template <class ScalarParam>
class Constants<Complex<ScalarParam> >
	{
	/* Embedded classes: */
	public:
	typedef Complex<ScalarParam> Scalar;
	typedef Complex<typename Constants<ScalarParam>::FieldScalar> FieldScalar;
	typedef Complex<typename Constants<ScalarParam>::PrecisionScalar> PrecisionScalar;
	
	/* Elements: */
	static const bool isIntegral=false;
	static const bool isRing=true;
	static const bool isField=true;
	static const bool isReal=false;
	static const Scalar zero;
	static const Scalar one;
	};

}

#if defined(MATH_NONSTANDARD_TEMPLATES) && !defined(MATH_COMPLEX_IMPLEMENTATION)
#include <Math/Complex.icpp>
#endif

#endif
