/***********************************************************************
BaseImage - Generic base class to represent images of arbitrary pixel
formats. The image coordinate system is such that pixel (0,0) is in the
lower-left corner.
Copyright (c) 2016-2018 Oliver Kreylos

This file is part of the Image Handling Library (Images).

The Image Handling Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Image Handling Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Image Handling Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Images/BaseImage.h>

#include <stddef.h>
#include <string.h>
#include <stdexcept>
#include <Math/Math.h>
#include <GL/Extensions/GLEXTFramebufferObject.h>

namespace Images {

/***********************************************
Methods of class BaseImage::ImageRepresentation:
***********************************************/

BaseImage::ImageRepresentation::ImageRepresentation(unsigned int sWidth,unsigned int sHeight,unsigned int sNumChannels,unsigned int sChannelSize,GLenum sFormat,GLenum sScalarType)
	:refCount(1U),
	 numChannels(sNumChannels),channelSize(sChannelSize),
	 image(0),
	 format(sFormat),scalarType(sScalarType)
	{
	/* Copy the image size: */
	size[0]=sWidth;
	size[1]=sHeight;
	
	/* Allocate the image array: */
	image=new char[size_t(size[1])*size_t(size[0])*size_t(numChannels)*size_t(channelSize)];
	}

BaseImage::ImageRepresentation::ImageRepresentation(const ImageRepresentation& source)
	:refCount(1U),
	 numChannels(source.numChannels),channelSize(source.channelSize),
	 image(0),
	 format(source.format),scalarType(source.scalarType)
	{
	/* Copy the image size: */
	size[0]=source.size[0];
	size[1]=source.size[1];
	
	/* Allocate the image array: */
	image=new char[size_t(size[1])*size_t(size[0])*size_t(numChannels)*size_t(channelSize)];
	
	/* Copy the source image byte-by-byte: */
	memcpy(image,source.image,size_t(size[1])*size_t(size[0])*size_t(numChannels)*size_t(channelSize));
	}

BaseImage::ImageRepresentation::~ImageRepresentation(void)
	{
	/* Destroy the allocated image array: */
	delete[] static_cast<char*>(image);
	}

namespace {

/*******************************************************
Helper classes and functions for basic image operations:
*******************************************************/

template <class ScalarParam>
inline
void
dropAlphaTyped(
	const BaseImage& source,
	BaseImage& dest)
	{
	/* Drop the alpha value of all pixels: */
	unsigned int numChannels=dest.getNumChannels();
	const ScalarParam* sPtr=static_cast<const ScalarParam*>(source.getPixels());
	ScalarParam* dPtr=static_cast<ScalarParam*>(dest.modifyPixels());
	for(size_t i=size_t(source.getSize(0))*size_t(source.getSize(1));i>0;--i)
		{
		/* Copy the non-alpha channels: */
		for(size_t j=0;j<numChannels;++j)
			*(dPtr++)=*(sPtr++);
		
		/* Skip the source's alpha channel: */
		++sPtr;
		}
	}

void dropAlphaImpl(const BaseImage& source,BaseImage& dest)
	{
	/* Delegate to a typed version of this function: */
	switch(source.getScalarType())
		{
		case GL_BYTE:
			dropAlphaTyped<signed char>(source,dest);
			break;
		
		case GL_UNSIGNED_BYTE:
			dropAlphaTyped<unsigned char>(source,dest);
			break;
		
		case GL_SHORT:
			dropAlphaTyped<signed short>(source,dest);
			break;
		
		case GL_UNSIGNED_SHORT:
			dropAlphaTyped<unsigned short>(source,dest);
			break;
		
		case GL_INT:
			dropAlphaTyped<signed int>(source,dest);
			break;
		
		case GL_UNSIGNED_INT:
			dropAlphaTyped<unsigned int>(source,dest);
			break;
		
		case GL_FLOAT:
			dropAlphaTyped<float>(source,dest);
			break;
		
		case GL_DOUBLE:
			dropAlphaTyped<double>(source,dest);
			break;
		
		default:
			throw std::runtime_error("Images::BaseImage::dropAlpha: Image has unsupported pixel format");
		}
	}

template <class ScalarParam>
inline
void
addAlphaTyped(
	const BaseImage& source,
	BaseImage& dest,
	ScalarParam alpha)
	{
	/* Add the constant alpha value to all pixels: */
	unsigned int numChannels=source.getNumChannels();
	const ScalarParam* sPtr=static_cast<const ScalarParam*>(source.getPixels());
	ScalarParam* dPtr=static_cast<ScalarParam*>(dest.modifyPixels());
	for(size_t i=size_t(source.getSize(0))*size_t(source.getSize(1));i>0;--i)
		{
		/* Copy the non-alpha channels: */
		for(size_t j=0;j<numChannels;++j)
			*(dPtr++)=*(sPtr++);
		
		/* Add an alpha value to the destination: */
		*(dPtr++)=alpha;
		}
	}

void addAlphaImpl(const BaseImage& source,BaseImage& dest,double alpha)
	{
	/* Delegate to a typed version of this function: */
	switch(source.getScalarType())
		{
		case GL_BYTE:
			addAlphaTyped<signed char>(source,dest,(signed char)(Math::clamp(Math::floor(alpha*128.0),0.0,127.0)));
			break;
		
		case GL_UNSIGNED_BYTE:
			addAlphaTyped<unsigned char>(source,dest,(unsigned char)(Math::clamp(Math::floor(alpha*256.0),0.0,255.0)));
			break;
		
		case GL_SHORT:
			addAlphaTyped<signed short>(source,dest,(signed short)(Math::clamp(Math::floor(alpha*32768.0),0.0,32767.0)));
			break;
		
		case GL_UNSIGNED_SHORT:
			addAlphaTyped<unsigned short>(source,dest,(unsigned short)(Math::clamp(Math::floor(alpha*65536.0),0.0,65535.0)));
			break;
		
		case GL_INT:
			addAlphaTyped<signed int>(source,dest,(signed int)(Math::clamp(Math::floor(alpha*2147483648.0),0.0,2147483647.0)));
			break;
		
		case GL_UNSIGNED_INT:
			addAlphaTyped<unsigned int>(source,dest,(unsigned int)(Math::clamp(Math::floor(alpha*4294967296.0),0.0,4294967295.0)));
			break;
		
		case GL_FLOAT:
			addAlphaTyped<float>(source,dest,float(alpha));
			break;
		
		case GL_DOUBLE:
			addAlphaTyped<double>(source,dest,alpha);
			break;
		
		default:
			throw std::runtime_error("Images::BaseImage::addAlpha: Image has unsupported pixel format");
		}
	}

template <class ScalarParam,class WeightParam>
inline
void
toGreyTypedInt(
	const BaseImage& source,
	BaseImage& dest)
	{
	/* Convert all pixels to luminance and retain an existing alpha channel: */
	const ScalarParam* sPtr=static_cast<const ScalarParam*>(source.getPixels());
	ScalarParam* dPtr=static_cast<ScalarParam*>(dest.modifyPixels());
	if(source.getNumChannels()==4)
		{
		/* Convert RGBA to LUMINANCE_ALPHA: */
		for(size_t i=size_t(source.getSize(0))*size_t(source.getSize(1));i>0;--i,sPtr+=4)
			{
			/* Calculate pixel luminance: */
			*(dPtr++)=ScalarParam((WeightParam(sPtr[0])*WeightParam(77)+WeightParam(sPtr[1])*WeightParam(150)+WeightParam(sPtr[2])*WeightParam(29))>>WeightParam(8));
			
			/* Copy alpha channel: */
			*(dPtr++)=sPtr[3];
			}
		}
	else
		{
		/* Convert RGB to LUMINANCE: */
		for(size_t i=size_t(source.getSize(0))*size_t(source.getSize(1));i>0;--i,sPtr+=3)
			{
			/* Calculate pixel luminance: */
			*(dPtr++)=ScalarParam((WeightParam(sPtr[0])*WeightParam(77)+WeightParam(sPtr[1])*WeightParam(150)+WeightParam(sPtr[2])*WeightParam(29))>>WeightParam(8));
			}
		}
	}

template <class ScalarParam>
inline
void
toGreyTypedFloat(
	const BaseImage& source,
	BaseImage& dest)
	{
	/* Convert all pixels to luminance and retain an existing alpha channel: */
	const ScalarParam* sPtr=static_cast<const ScalarParam*>(source.getPixels());
	ScalarParam* dPtr=static_cast<ScalarParam*>(dest.modifyPixels());
	if(source.getNumChannels()==4)
		{
		/* Convert RGBA to LUMINANCE_ALPHA: */
		for(size_t i=size_t(source.getSize(0))*size_t(source.getSize(1));i>0;--i,sPtr+=4)
			{
			/* Calculate pixel luminance: */
			*(dPtr++)=sPtr[0]*ScalarParam(0.299)+sPtr[1]*ScalarParam(0.587)+sPtr[2]*ScalarParam(0.114);
			
			/* Copy alpha channel: */
			*(dPtr++)=sPtr[3];
			}
		}
	else
		{
		/* Convert RGB to LUMINANCE: */
		for(size_t i=size_t(source.getSize(0))*size_t(source.getSize(1));i>0;--i,sPtr+=3)
			{
			/* Calculate pixel luminance: */
			*(dPtr++)=sPtr[0]*ScalarParam(0.299)+sPtr[1]*ScalarParam(0.587)+sPtr[2]*ScalarParam(0.114);
			}
		}
	}

void toGreyImpl(const BaseImage& source,BaseImage& dest)
	{
	/* Delegate to a typed version of this function: */
	switch(source.getScalarType())
		{
		case GL_BYTE:
			toGreyTypedInt<signed char,signed short>(source,dest);
			break;
		
		case GL_UNSIGNED_BYTE:
			toGreyTypedInt<unsigned char,unsigned short>(source,dest);
			break;
		
		case GL_SHORT:
			toGreyTypedInt<signed short,signed int>(source,dest);
			break;
		
		case GL_UNSIGNED_SHORT:
			toGreyTypedInt<unsigned short,unsigned int>(source,dest);
			break;
		
		case GL_INT:
			toGreyTypedInt<signed int,signed long>(source,dest);
			break;
		
		case GL_UNSIGNED_INT:
			toGreyTypedInt<unsigned int,unsigned long>(source,dest);
			break;
		
		case GL_FLOAT:
			toGreyTypedFloat<float>(source,dest);
			break;
		
		case GL_DOUBLE:
			toGreyTypedFloat<double>(source,dest);
			break;
		
		default:
			throw std::runtime_error("Images::BaseImage::toGrey: Image has unsupported pixel format");
		}
	}

template <class ScalarParam>
inline
void
toRgbTyped(
	const BaseImage& source,
	BaseImage& dest)
	{
	/* Convert all pixels to RGB and retain an existing alpha channel: */
	const ScalarParam* sPtr=static_cast<const ScalarParam*>(source.getPixels());
	ScalarParam* dPtr=static_cast<ScalarParam*>(dest.modifyPixels());
	if(source.getNumChannels()==2)
		{
		/* Convert LUMINANCE_ALPHA to RGBA: */
		for(size_t i=size_t(source.getSize(0))*size_t(source.getSize(1));i>0;--i,sPtr+=2)
			{
			/* Copy pixel luminance: */
			*(dPtr++)=sPtr[0];
			*(dPtr++)=sPtr[0];
			*(dPtr++)=sPtr[0];
			
			/* Copy alpha channel: */
			*(dPtr++)=sPtr[1];
			}
		}
	else
		{
		/* Convert LUMINANCE to RGB: */
		for(size_t i=size_t(source.getSize(0))*size_t(source.getSize(1));i>0;--i,++sPtr)
			{
			/* Copy pixel luminance: */
			*(dPtr++)=sPtr[0];
			*(dPtr++)=sPtr[0];
			*(dPtr++)=sPtr[0];
			}
		}
	}

void toRgbImpl(const BaseImage& source,BaseImage& dest)
	{
	/* Delegate to a typed version of this function: */
	switch(source.getScalarType())
		{
		case GL_BYTE:
			toRgbTyped<signed char>(source,dest);
			break;
		
		case GL_UNSIGNED_BYTE:
			toRgbTyped<unsigned char>(source,dest);
			break;
		
		case GL_SHORT:
			toRgbTyped<signed short>(source,dest);
			break;
		
		case GL_UNSIGNED_SHORT:
			toRgbTyped<unsigned short>(source,dest);
			break;
		
		case GL_INT:
			toRgbTyped<signed int>(source,dest);
			break;
		
		case GL_UNSIGNED_INT:
			toRgbTyped<unsigned int>(source,dest);
			break;
		
		case GL_FLOAT:
			toRgbTyped<float>(source,dest);
			break;
		
		case GL_DOUBLE:
			toRgbTyped<double>(source,dest);
			break;
		
		default:
			throw std::runtime_error("Images::BaseImage::toRgb: Image has unsupported pixel format");
		}
	}

template <class ScalarParam,class AccumParam>
inline
void
shrinkTypedInt(
	const BaseImage& source,
	BaseImage& dest)
	{
	/* Average all blocks of 2x2 pixels in the source image: */
	unsigned int nc=source.getNumChannels();
	const ScalarParam* sRow0Ptr=static_cast<const ScalarParam*>(source.getPixels());
	const ptrdiff_t sStride=source.getSize(0)*nc;
	const ScalarParam* sRow1Ptr=sRow0Ptr+sStride;
	ScalarParam* dPtr=static_cast<ScalarParam*>(dest.modifyPixels());
	for(unsigned int y=0;y<source.getSize(1);y+=2,sRow0Ptr+=sStride*2,sRow1Ptr+=sStride*2)
		{
		const ScalarParam* s0Ptr=sRow0Ptr;
		const ScalarParam* s1Ptr=sRow1Ptr;
		for(unsigned int x=0;x<source.getSize(0);x+=2,s0Ptr+=nc,s1Ptr+=nc)
			{
			for(unsigned int i=0;i<source.getNumChannels();++i,++s0Ptr,++s1Ptr,++dPtr)
				{
				/* Average the current 2x2 pixel block: */
				AccumParam sum0=AccumParam(s0Ptr[0])+AccumParam(s0Ptr[nc]);
				AccumParam sum1=AccumParam(s1Ptr[0])+AccumParam(s1Ptr[nc]);
				*dPtr=ScalarParam((sum0+sum1+2)>>2);
				}
			}
		}
	}

template <class ScalarParam>
inline
void
shrinkTypedFloat(
	const BaseImage& source,
	BaseImage& dest)
	{
	/* Average all blocks of 2x2 pixels in the source image: */
	unsigned int nc=source.getNumChannels();
	const ScalarParam* sRow0Ptr=static_cast<const ScalarParam*>(source.getPixels());
	const ptrdiff_t sStride=source.getSize(0)*nc;
	const ScalarParam* sRow1Ptr=sRow0Ptr+sStride;
	ScalarParam* dPtr=static_cast<ScalarParam*>(dest.modifyPixels());
	for(unsigned int y=0;y<source.getSize(1);y+=2,sRow0Ptr+=sStride*2,sRow1Ptr+=sStride*2)
		{
		const ScalarParam* s0Ptr=sRow0Ptr;
		const ScalarParam* s1Ptr=sRow1Ptr;
		for(unsigned int x=0;x<source.getSize(0);x+=2,s0Ptr+=nc,s1Ptr+=nc)
			{
			for(unsigned int i=0;i<source.getNumChannels();++i,++s0Ptr,++s1Ptr,++dPtr)
				{
				/* Average the current 2x2 pixel block: */
				*dPtr=(s0Ptr[0]+s0Ptr[nc]+s1Ptr[0]+s1Ptr[nc])*ScalarParam(0.25);
				}
			}
		}
	}

/***************************************************************************
Generic function to convert color components between supported scalar types:
***************************************************************************/

template <class SourceScalarParam,class DestScalarParam>
DestScalarParam convertColorScalar(SourceScalarParam value)
	{
	}

/***********************
Conversions from GLbyte:
***********************/

template <>
inline GLbyte convertColorScalar<GLbyte,GLbyte>(GLbyte value)
	{
	return value;
	}

template <>
inline GLubyte convertColorScalar<GLbyte,GLubyte>(GLbyte value)
	{
	if(value<GLbyte(0))
		return GLubyte(0);
	else
		{
		GLubyte v(value);
		return (v<<1)|(v>>6);
		}
	}

template <>
inline GLshort convertColorScalar<GLbyte,GLshort>(GLbyte value)
	{
	GLshort v(value^GLbyte(0x80));
	return ((v<<8)|v)^GLshort(0x8000);
	}

template <>
inline GLushort convertColorScalar<GLbyte,GLushort>(GLbyte value)
	{
	if(value<GLbyte(0))
		return GLushort(0);
	else
		{
		GLushort v(value);
		return (v<<9)|(v<<2)|(v>>5);
		}
	}

template <>
inline GLint convertColorScalar<GLbyte,GLint>(GLbyte value)
	{
	GLint v(value^GLbyte(0x80));
	return ((v<<24)|(v<<16)|(v<<8)|v)^GLint(0x80000000);
	}

template <>
inline GLuint convertColorScalar<GLbyte,GLuint>(GLbyte value)
	{
	if(value<GLbyte(0))
		return GLuint(0);
	else
		{
		GLuint v(value);
		return (v<<25)|(v<<18)|(v<<11)|(v<<4)|(v>>3);
		}
	}

template <>
inline GLfloat convertColorScalar<GLbyte,GLfloat>(GLbyte value)
	{
	return GLfloat(value^GLbyte(0x80))/127.5f-1.0f;
	}

template <>
inline GLdouble convertColorScalar<GLbyte,GLdouble>(GLbyte value)
	{
	return GLdouble(value^GLbyte(0x80))/127.5-1.0;
	}

/************************
Conversions from GLubyte:
************************/

template <>
inline GLbyte convertColorScalar<GLubyte,GLbyte>(GLubyte value)
	{
	return GLbyte(value>>1);
	}

template <>
inline GLubyte convertColorScalar<GLubyte,GLubyte>(GLubyte value)
	{
	return value;
	}

template <>
inline GLshort convertColorScalar<GLubyte,GLshort>(GLubyte value)
	{
	GLshort v(value);
	return (v<<7)|(v>>1);
	}

template <>
inline GLushort convertColorScalar<GLubyte,GLushort>(GLubyte value)
	{
	GLushort v(value);
	return (v<<8)|v;
	}

template <>
inline GLint convertColorScalar<GLubyte,GLint>(GLubyte value)
	{
	GLint v(value);
	return (v<<23)|(v<<15)|(v<<7)|(v>>1);
	}

template <>
inline GLuint convertColorScalar<GLubyte,GLuint>(GLubyte value)
	{
	GLuint v(value);
	return (v<<24)|(v<<16)|(v<<8)|v;
	}

template <>
inline GLfloat convertColorScalar<GLubyte,GLfloat>(GLubyte value)
	{
	return GLfloat(value)/255.0f;
	}

template <>
inline GLdouble convertColorScalar<GLubyte,GLdouble>(GLubyte value)
	{
	return GLdouble(value)/255.0;
	}

/************************
Conversions from GLshort:
************************/

template <>
inline GLbyte convertColorScalar<GLshort,GLbyte>(GLshort value)
	{
	return GLbyte(value>>8);
	}

template <>
inline GLubyte convertColorScalar<GLshort,GLubyte>(GLshort value)
	{
	if(value<GLshort(0))
		return GLubyte(0);
	else
		return GLubyte(value>>7);
	}

template <>
inline GLshort convertColorScalar<GLshort,GLshort>(GLshort value)
	{
	return value;
	}

template <>
inline GLushort convertColorScalar<GLshort,GLushort>(GLshort value)
	{
	if(value<GLshort(0))
		return GLushort(0);
	else
		{
		GLushort v(value);
		return (v<<1)|(v>>14);
		}
	}

template <>
inline GLint convertColorScalar<GLshort,GLint>(GLshort value)
	{
	GLint v(value^GLshort(0x8000));
	return ((v<<16)|v)^GLint(0x80000000);
	}

template <>
inline GLuint convertColorScalar<GLshort,GLuint>(GLshort value)
	{
	if(value<GLshort(0))
		return GLuint(0);
	else
		{
		GLuint v(value);
		return (v<<17)|(v<<2)|(v>>13);
		}
	}

template <>
inline GLfloat convertColorScalar<GLshort,GLfloat>(GLshort value)
	{
	return GLfloat(value^GLshort(0x8000))/32767.5f-1.0f;
	}

template <>
inline GLdouble convertColorScalar<GLshort,GLdouble>(GLshort value)
	{
	return GLdouble(value^GLshort(0x8000))/32767.5-1.0;
	}

/*************************
Conversions from GLushort:
*************************/

template <>
inline GLbyte convertColorScalar<GLushort,GLbyte>(GLushort value)
	{
	return GLbyte(value>>9);
	}

template <>
inline GLubyte convertColorScalar<GLushort,GLubyte>(GLushort value)
	{
	return GLubyte(value>>8);
	}

template <>
inline GLshort convertColorScalar<GLushort,GLshort>(GLushort value)
	{
	return GLshort(value>>1);
	}

template <>
inline GLushort convertColorScalar<GLushort,GLushort>(GLushort value)
	{
	return value;
	}

template <>
inline GLint convertColorScalar<GLushort,GLint>(GLushort value)
	{
	GLint v(value);
	return (v<<15)|(v>>1);
	}

template <>
inline GLuint convertColorScalar<GLushort,GLuint>(GLushort value)
	{
	GLuint v(value);
	return (v<<16)|v;
	}

template <>
inline GLfloat convertColorScalar<GLushort,GLfloat>(GLushort value)
	{
	return GLfloat(value)/65535.0f;
	}

template <>
inline GLdouble convertColorScalar<GLushort,GLdouble>(GLushort value)
	{
	return GLdouble(value)/65535.0;
	}

}

/**************************
Methods of class BaseImage:
**************************/

BaseImage BaseImage::dropAlpha(void) const
	{
	/* Process the image based on its format: */
	if(rep->format==GL_LUMINANCE_ALPHA)
		{
		/* Drop the alpha channel to create a luminance-only image: */
		BaseImage result(rep->size[0],rep->size[1],rep->numChannels-1,rep->channelSize,GL_LUMINANCE,rep->scalarType);
		dropAlphaImpl(*this,result);
		
		return result;
		}
	else if(rep->format==GL_RGBA)
		{
		/* Drop the alpha channel to create an RGB image: */
		BaseImage result(rep->size[0],rep->size[1],rep->numChannels-1,rep->channelSize,GL_RGB,rep->scalarType);
		dropAlphaImpl(*this,result);
		
		return result;
		}
	else if(rep->format==GL_LUMINANCE||rep->format==GL_RGB)
		{
		/* Return the image unchanged: */
		return *this;
		}
	else
		throw std::runtime_error("Images::BaseImage::dropAlpha: Image has unsupported pixel format");
	}

BaseImage BaseImage::addAlpha(double alpha) const
	{
	/* Process the image based on its format: */
	if(rep->format==GL_LUMINANCE)
		{
		/* Add an alpha channel to create a luminance-alpha image: */
		BaseImage result(rep->size[0],rep->size[1],rep->numChannels+1,rep->channelSize,GL_LUMINANCE_ALPHA,rep->scalarType);
		addAlphaImpl(*this,result,alpha);
		
		return result;
		}
	else if(rep->format==GL_RGB)
		{
		/* Add an alpha channel to create an RGBA image: */
		BaseImage result(rep->size[0],rep->size[1],rep->numChannels+1,rep->channelSize,GL_RGBA,rep->scalarType);
		addAlphaImpl(*this,result,alpha);
		
		return result;
		}
	else if(rep->format==GL_LUMINANCE_ALPHA||rep->format==GL_RGBA)
		{
		/* Return the image unchanged: */
		return *this;
		}
	else
		throw std::runtime_error("Images::BaseImage::addAlpha: Image has unsupported pixel format");
	}

BaseImage BaseImage::toGrey(void) const
	{
	/* Process the image based on its format: */
	if(rep->format==GL_RGB)
		{
		/* Convert RGB to luminance: */
		BaseImage result(rep->size[0],rep->size[1],1,rep->channelSize,GL_LUMINANCE,rep->scalarType);
		toGreyImpl(*this,result);
		
		return result;
		}
	else if(rep->format==GL_RGBA)
		{
		/* Convert RGBA to luminance-alpha: */
		BaseImage result(rep->size[0],rep->size[1],2,rep->channelSize,GL_LUMINANCE_ALPHA,rep->scalarType);
		toGreyImpl(*this,result);
		
		return result;
		}
	else if(rep->format==GL_LUMINANCE||rep->format==GL_LUMINANCE_ALPHA)
		{
		/* Return the image unchanged: */
		return *this;
		}
	else
		throw std::runtime_error("Images::BaseImage::toGrey: Image has unsupported pixel format");
	}

BaseImage BaseImage::toRgb(void) const
	{
	/* Process the image based on its format: */
	if(rep->format==GL_LUMINANCE)
		{
		/* Convert luminance to RGB: */
		BaseImage result(rep->size[0],rep->size[1],3,rep->channelSize,GL_RGB,rep->scalarType);
		toRgbImpl(*this,result);
		
		return result;
		}
	else if(rep->format==GL_LUMINANCE_ALPHA)
		{
		/* Convert luminance-alpha to RGBA: */
		BaseImage result(rep->size[0],rep->size[1],4,rep->channelSize,GL_RGBA,rep->scalarType);
		toRgbImpl(*this,result);
		
		return result;
		}
	else if(rep->format==GL_RGB||rep->format==GL_RGBA)
		{
		/* Return the image unchanged: */
		return *this;
		}
	else
		throw std::runtime_error("Images::BaseImage::toGrey: Image has unsupported pixel format");
	}

BaseImage BaseImage::shrink(void) const
	{
	/* Check that the image's size is even: */
	if(rep->size[0]%2!=0||rep->size[1]%2!=0)
		throw std::runtime_error("Images::BaseImage::shrink: Image size is not divisible by two");
	
	/* Create a reduced-sized image with the same pixel format: */
	BaseImage result(rep->size[0]/2,rep->size[1]/2,rep->numChannels,rep->channelSize,rep->format,rep->scalarType);
	
	/* Delegate to a typed version of this function: */
	switch(rep->scalarType)
		{
		case GL_BYTE:
			shrinkTypedInt<signed char,signed short>(*this,result);
			break;
		
		case GL_UNSIGNED_BYTE:
			shrinkTypedInt<unsigned char,unsigned short>(*this,result);
			break;
		
		case GL_SHORT:
			shrinkTypedInt<signed short,signed int>(*this,result);
			break;
		
		case GL_UNSIGNED_SHORT:
			shrinkTypedInt<unsigned short,unsigned int>(*this,result);
			break;
		
		case GL_INT:
			shrinkTypedInt<signed int,signed long>(*this,result);
			break;
		
		case GL_UNSIGNED_INT:
			shrinkTypedInt<unsigned int,unsigned long>(*this,result);
			break;
		
		case GL_FLOAT:
			shrinkTypedFloat<float>(*this,result);
			break;
		
		case GL_DOUBLE:
			shrinkTypedFloat<double>(*this,result);
			break;
		
		default:
			throw std::runtime_error("Images::BaseImage::shrink: Image has unsupported pixel format");
		}
	
	return result;
	}

GLenum BaseImage::getInternalFormat(void) const
	{
	/* Guess an appropriate internal image format: */
	GLint internalFormat=GL_RGBA;
	if(rep->format==GL_LUMINANCE&&rep->channelSize==8)
		internalFormat=GL_RGB8;
	else if(rep->format==GL_LUMINANCE_ALPHA&&rep->channelSize==8)
		internalFormat=GL_RGBA8;
	else if(rep->format==GL_RGB&&rep->channelSize==8)
		internalFormat=GL_RGB8;
	else if(rep->format==GL_RGBA&&rep->channelSize==8)
		internalFormat=GL_RGBA8;
	
	return internalFormat;
	}

BaseImage& BaseImage::glReadPixels(GLint x,GLint y)
	{
	/* Un-share the image representation without retaining pixels: */
	ownRepresentation(false);
	
	/* Set up pixel processing pipeline: */
	glPixelStorei(GL_PACK_ALIGNMENT,1);
	glPixelStorei(GL_PACK_SKIP_PIXELS,0);
	glPixelStorei(GL_PACK_ROW_LENGTH,0);
	glPixelStorei(GL_PACK_SKIP_ROWS,0);
	
	/* Read image: */
	::glReadPixels(x,y,rep->size[0],rep->size[1],rep->format,rep->scalarType,rep->image);
	
	return *this;
	}

void BaseImage::glDrawPixels(void) const
	{
	/* Set up pixel processing pipeline: */
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
	
	/* Write image: */
	::glDrawPixels(rep->size[0],rep->size[1],rep->format,rep->scalarType,rep->image);
	}

void BaseImage::glTexImage2D(GLenum target,GLint level,GLint internalFormat,bool padImageSize) const
	{
	/* Set up pixel processing pipeline: */
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
	
	if(padImageSize)
		{
		/* Calculate the texture width and height as the next power of two: */
		unsigned int texSize[2];
		for(int i=0;i<2;++i)
			for(texSize[i]=1;texSize[i]<rep->size[i];texSize[i]<<=1)
				;
		
		if(texSize[0]!=rep->size[0]||texSize[1]!=rep->size[1])
			{
			/* Create the padded texture image: */
			::glTexImage2D(target,level,internalFormat,texSize[0],texSize[1],0,rep->format,rep->scalarType,0);
			
			/* Upload the image: */
			::glTexSubImage2D(target,level,0,0,rep->size[0],rep->size[1],rep->format,rep->scalarType,rep->image);
			}
		else
			{
			/* Upload the image: */
			::glTexImage2D(target,level,internalFormat,rep->size[0],rep->size[1],0,rep->format,rep->scalarType,rep->image);
			}
		}
	else
		{
		/* Upload the image: */
		::glTexImage2D(target,level,internalFormat,rep->size[0],rep->size[1],0,rep->format,rep->scalarType,rep->image);
		}
	}

void BaseImage::glTexImage2DMipmap(GLenum target,GLint internalFormat,bool padImageSize) const
	{
	/* Set up pixel processing pipeline: */
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
	
	/* Set the texture's mipmap level range: */
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);
	
	/* Check if the GL_EXT_framebuffer_object OpenGL extension is supported: */
	if(GLEXTFramebufferObject::isSupported())
		{
		/* Initialize the GL_EXT_framebuffer_object extension: */
		GLEXTFramebufferObject::initExtension();
		
		/* Upload the base level texture image: */
		glTexImage2D(target,0,internalFormat,padImageSize);
		
		/* Request automatic mipmap generation: */
		glGenerateMipmapEXT(target);
		}
	else
		{
		/* Create mipmaps manually by successively downsampling this image: */
		BaseImage level=*this;
		GLint levelIndex=0;
		while(true)
			{
			/* Upload the current level texture image: */
			level.glTexImage2D(target,levelIndex,internalFormat,padImageSize);
			++levelIndex;
			
			/* Bail out if we can't shrink further: */
			if(level.getSize(0)%2!=0||level.getSize(1)%2!=0)
				break;
			
			/* Downsample the current level image: */
			level=level.shrink();
			}
		
		/* Set the texture's mipmap level range: */
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,levelIndex-1);
		}
	}

void BaseImage::glTexSubImage2D(GLenum target,GLint level,GLint xOffset,GLint yOffset) const
	{
	/* Set up pixel processing pipeline: */
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
	
	/* Upload the image: */
	::glTexSubImage2D(target,level,xOffset,yOffset,rep->size[0],rep->size[1],rep->format,rep->scalarType,rep->image);
	}

void BaseImage::glTexSubImage3D(GLenum target,GLint level,GLint xOffset,GLint yOffset,GLint zOffset) const
	{
	/* Set up pixel processing pipeline: */
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
	
	/* Upload the image: */
	::glTexSubImage3D(target,level,xOffset,yOffset,zOffset,rep->size[0],rep->size[1],1,rep->format,rep->scalarType,rep->image);
	}

}
