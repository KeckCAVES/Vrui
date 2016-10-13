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

#include <Math/Complex.icpp>

namespace Math {

/*****************************************************************
Force instantiation of all standard Complex classes and functions:
*****************************************************************/

template const Complex<float> Complex<float>::zero;
template const Complex<float> Complex<float>::one;
template const Complex<float> Complex<float>::i;

template const Complex<double> Complex<double>::zero;
template const Complex<double> Complex<double>::one;
template const Complex<double> Complex<double>::i;

/****************************************************************************
Force instantiation of all standard Constants<Complex> classes and functions:
****************************************************************************/

template const bool Constants<Complex<float> >::isIntegral;
template const bool Constants<Complex<float> >::isRing;
template const bool Constants<Complex<float> >::isField;
template const bool Constants<Complex<float> >::isReal;
template const Constants<Complex<float> >::Scalar Constants<Complex<float> >::zero;
template const Constants<Complex<float> >::Scalar Constants<Complex<float> >::one;

template const bool Constants<Complex<double> >::isIntegral;
template const bool Constants<Complex<double> >::isRing;
template const bool Constants<Complex<double> >::isField;
template const bool Constants<Complex<double> >::isReal;
template const Constants<Complex<double> >::Scalar Constants<Complex<double> >::zero;
template const Constants<Complex<double> >::Scalar Constants<Complex<double> >::one;

}
