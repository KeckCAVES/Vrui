/***********************************************************************
GLGeometryVertex - Class to encapsulate OpenGL vertex properties using
geometry data types.
Copyright (c) 2009 Oliver Kreylos

This file is part of the OpenGL C++ Wrapper Library (GLWrappers).

The OpenGL C++ Wrapper Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenGL C++ Wrapper Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenGL C++ Wrapper Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#define GLGEOMETRYVERTEX_IMPLEMENTATION

#ifndef METHODPREFIX
	#ifdef NONSTANDARD_GLVERTEX_TEMPLATES
		#define METHODPREFIX inline
	#else
		#define METHODPREFIX
	#endif
#endif

#include <GL/gl.h>
#include <GL/GLTexCoordTemplates.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLNormalTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLVertexArrayTemplates.h>

#include <GL/GLGeometryVertex.h>

namespace {

/*******************************************************************
Helper class to generate templatized glVertex/glVertexPointer calls:
*******************************************************************/

template <class TexCoordScalarParam,int numTexCoordComponentsParam,
          class ColorScalarParam,int numColorComponentsParam,
          class NormalScalarParam,
          class PositionScalarParam,int numPositionComponentsParam>
class GLVertexPointerImplementation
	{
	/* Embedded classes: */
	public:
	typedef GLGeometry::Vertex<TexCoordScalarParam,numTexCoordComponentsParam,
	                           ColorScalarParam,numColorComponentsParam,
	                           NormalScalarParam,
	                           PositionScalarParam,numPositionComponentsParam> Vertex;
	
	/* Methods: */
	static void glVertex(const Vertex& vertex)
		{
		glTexCoord<numTexCoordComponentsParam>(vertex.texCoord.getComponents());
		glColor<numColorComponentsParam>(vertex.color.getRgba());
		glNormal<3>(vertex.normal.getComponents());
		::glVertex<numPositionComponentsParam>(vertex.position.getComponents());
		}
	static void glVertexPointer(const Vertex* vertices)
		{
		glTexCoordPointer(numTexCoordComponentsParam,sizeof(Vertex),vertices[0].texCoord.getComponents());
		glColorPointer(numColorComponentsParam,sizeof(Vertex),vertices[0].color.getRgba());
		glNormalPointer(sizeof(Vertex),vertices[0].normal.getComponents());
		::glVertexPointer(numPositionComponentsParam,sizeof(Vertex),vertices[0].position.getComponents());
		}
	static void glVertexPointer(int vertexPartsMask,const Vertex* vertices)
		{
		if(vertexPartsMask&GLVertexArrayParts::TexCoord)
			glTexCoordPointer(numTexCoordComponentsParam,sizeof(Vertex),vertices[0].texCoord.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(numColorComponentsParam,sizeof(Vertex),vertices[0].color.getRgba());
		if(vertexPartsMask&GLVertexArrayParts::Normal)
			glNormalPointer(sizeof(Vertex),vertices[0].normal.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			::glVertexPointer(numPositionComponentsParam,sizeof(Vertex),vertices[0].position.getComponents());
		}
	};

template <class TexCoordScalarParam,int numTexCoordComponentsParam,
          class NormalScalarParam,
          class PositionScalarParam,int numPositionComponentsParam>
class GLVertexPointerImplementation<TexCoordScalarParam,numTexCoordComponentsParam,
                                    void,0,
                                    NormalScalarParam,
                                    PositionScalarParam,numPositionComponentsParam>
	{
	/* Embedded classes: */
	public:
	typedef GLGeometry::Vertex<TexCoordScalarParam,numTexCoordComponentsParam,
	                           void,0,
	                           NormalScalarParam,
	                           PositionScalarParam,numPositionComponentsParam> Vertex;
	
	/* Methods: */
	static void glVertex(const Vertex& vertex)
		{
		glTexCoord<numTexCoordComponentsParam>(vertex.texCoord.getComponents());
		glNormal<3>(vertex.normal.getComponents());
		::glVertex<numPositionComponentsParam>(vertex.position.getComponents());
		}
	static void glVertexPointer(const Vertex* vertices)
		{
		glTexCoordPointer(numTexCoordComponentsParam,sizeof(Vertex),vertices[0].texCoord.getComponents());
		glNormalPointer(sizeof(Vertex),vertices[0].normal.getComponents());
		::glVertexPointer(numPositionComponentsParam,sizeof(Vertex),vertices[0].position.getComponents());
		}
	static void glVertexPointer(int vertexPartsMask,const Vertex* vertices)
		{
		if(vertexPartsMask&GLVertexArrayParts::TexCoord)
			glTexCoordPointer(numTexCoordComponentsParam,sizeof(Vertex),vertices[0].texCoord.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Normal)
			glNormalPointer(sizeof(Vertex),vertices[0].normal.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			::glVertexPointer(numPositionComponentsParam,sizeof(Vertex),vertices[0].position.getComponents());
		}
	};

template <class TexCoordScalarParam,int numTexCoordComponentsParam,
          class ColorScalarParam,int numColorComponentsParam,
          class PositionScalarParam,int numPositionComponentsParam>
class GLVertexPointerImplementation<TexCoordScalarParam,numTexCoordComponentsParam,
                                    ColorScalarParam,numColorComponentsParam,
                                    void,
                                    PositionScalarParam,numPositionComponentsParam>
	{
	/* Embedded classes: */
	public:
	typedef GLGeometry::Vertex<TexCoordScalarParam,numTexCoordComponentsParam,
	                           ColorScalarParam,numColorComponentsParam,
	                           void,
	                           PositionScalarParam,numPositionComponentsParam> Vertex;
	
	/* Methods: */
	static void glVertex(const Vertex& vertex)
		{
		glTexCoord<numTexCoordComponentsParam>(vertex.texCoord.getComponents());
		glColor<numColorComponentsParam>(vertex.color.getRgba());
		::glVertex<numPositionComponentsParam>(vertex.position.getComponents());
		}
	static void glVertexPointer(const Vertex* vertices)
		{
		glTexCoordPointer(numTexCoordComponentsParam,sizeof(Vertex),vertices[0].texCoord.getComponents());
		glColorPointer(numColorComponentsParam,sizeof(Vertex),vertices[0].color.getRgba());
		::glVertexPointer(numPositionComponentsParam,sizeof(Vertex),vertices[0].position.getComponents());
		}
	static void glVertexPointer(int vertexPartsMask,const Vertex* vertices)
		{
		if(vertexPartsMask&GLVertexArrayParts::TexCoord)
			glTexCoordPointer(numTexCoordComponentsParam,sizeof(Vertex),vertices[0].texCoord.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(numColorComponentsParam,sizeof(Vertex),vertices[0].color.getRgba());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			::glVertexPointer(numPositionComponentsParam,sizeof(Vertex),vertices[0].position.getComponents());
		}
	};

template <class ColorScalarParam,int numColorComponentsParam,
          class NormalScalarParam,
          class PositionScalarParam,int numPositionComponentsParam>
class GLVertexPointerImplementation<void,0,
                                    ColorScalarParam,numColorComponentsParam,
                                    NormalScalarParam,
                                    PositionScalarParam,numPositionComponentsParam>
	{
	/* Embedded classes: */
	public:
	typedef GLGeometry::Vertex<void,0,
	                           ColorScalarParam,numColorComponentsParam,
	                           NormalScalarParam,
	                           PositionScalarParam,numPositionComponentsParam> Vertex;
	
	/* Methods: */
	static void glVertex(const Vertex& vertex)
		{
		glColor<numColorComponentsParam>(vertex.color.getRgba());
		glNormal<3>(vertex.normal.getComponents());
		::glVertex<numPositionComponentsParam>(vertex.position.getComponents());
		}
	static void glVertexPointer(const Vertex* vertices)
		{
		glColorPointer(numColorComponentsParam,sizeof(Vertex),vertices[0].color.getRgba());
		glNormalPointer(sizeof(Vertex),vertices[0].normal.getComponents());
		::glVertexPointer(numPositionComponentsParam,sizeof(Vertex),vertices[0].position.getComponents());
		}
	static void glVertexPointer(int vertexPartsMask,const Vertex* vertices)
		{
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(numColorComponentsParam,sizeof(Vertex),vertices[0].color.getRgba());
		if(vertexPartsMask&GLVertexArrayParts::Normal)
			glNormalPointer(sizeof(Vertex),vertices[0].normal.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			::glVertexPointer(numPositionComponentsParam,sizeof(Vertex),vertices[0].position.getComponents());
		}
	};

template <class TexCoordScalarParam,int numTexCoordComponentsParam,
          class PositionScalarParam,int numPositionComponentsParam>
class GLVertexPointerImplementation<TexCoordScalarParam,numTexCoordComponentsParam,
                                    void,0,
                                    void,
                                    PositionScalarParam,numPositionComponentsParam>
	{
	/* Embedded classes: */
	public:
	typedef GLGeometry::Vertex<TexCoordScalarParam,numTexCoordComponentsParam,
	                           void,0,
	                           void,
	                           PositionScalarParam,numPositionComponentsParam> Vertex;
	
	/* Methods: */
	static void glVertex(const Vertex& vertex)
		{
		glTexCoord<numTexCoordComponentsParam>(vertex.texCoord.getComponents());
		::glVertex<numPositionComponentsParam>(vertex.position.getComponents());
		}
	static void glVertexPointer(const Vertex* vertices)
		{
		glTexCoordPointer(numTexCoordComponentsParam,sizeof(Vertex),vertices[0].texCoord.getComponents());
		::glVertexPointer(numPositionComponentsParam,sizeof(Vertex),vertices[0].position.getComponents());
		}
	static void glVertexPointer(int vertexPartsMask,const Vertex* vertices)
		{
		if(vertexPartsMask&GLVertexArrayParts::TexCoord)
			glTexCoordPointer(numTexCoordComponentsParam,sizeof(Vertex),vertices[0].texCoord.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			::glVertexPointer(numPositionComponentsParam,sizeof(Vertex),vertices[0].position.getComponents());
		}
	};

template <class NormalScalarParam,
          class PositionScalarParam,int numPositionComponentsParam>
class GLVertexPointerImplementation<void,0,
                                    void,0,
                                    NormalScalarParam,
                                    PositionScalarParam,numPositionComponentsParam>
	{
	/* Embedded classes: */
	public:
	typedef GLGeometry::Vertex<void,0,
	                           void,0,
	                           NormalScalarParam,
	                           PositionScalarParam,numPositionComponentsParam> Vertex;
	
	/* Methods: */
	static void glVertex(const Vertex& vertex)
		{
		glNormal<3>(vertex.normal.getComponents());
		::glVertex<numPositionComponentsParam>(vertex.position.getComponents());
		}
	static void glVertexPointer(const Vertex* vertices)
		{
		glNormalPointer(sizeof(Vertex),vertices[0].normal.getComponents());
		::glVertexPointer(numPositionComponentsParam,sizeof(Vertex),vertices[0].position.getComponents());
		}
	static void glVertexPointer(int vertexPartsMask,const Vertex* vertices)
		{
		if(vertexPartsMask&GLVertexArrayParts::Normal)
			glNormalPointer(sizeof(Vertex),vertices[0].normal.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			::glVertexPointer(numPositionComponentsParam,sizeof(Vertex),vertices[0].position.getComponents());
		}
	};

template <class ColorScalarParam,int numColorComponentsParam,
          class PositionScalarParam,int numPositionComponentsParam>
class GLVertexPointerImplementation<void,0,
                                    ColorScalarParam,numColorComponentsParam,
                                    void,
                                    PositionScalarParam,numPositionComponentsParam>
	{
	/* Embedded classes: */
	public:
	typedef GLGeometry::Vertex<void,0,
	                           ColorScalarParam,numColorComponentsParam,
	                           void,
	                           PositionScalarParam,numPositionComponentsParam> Vertex;
	
	/* Methods: */
	static void glVertex(const Vertex& vertex)
		{
		glColor<numColorComponentsParam>(vertex.color.getRgba());
		::glVertex<numPositionComponentsParam>(vertex.position.getComponents());
		}
	static void glVertexPointer(const Vertex* vertices)
		{
		glColorPointer(numColorComponentsParam,sizeof(Vertex),vertices[0].color.getRgba());
		::glVertexPointer(numPositionComponentsParam,sizeof(Vertex),vertices[0].position.getComponents());
		}
	static void glVertexPointer(int vertexPartsMask,const Vertex* vertices)
		{
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(numColorComponentsParam,sizeof(Vertex),vertices[0].color.getRgba());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			::glVertexPointer(numPositionComponentsParam,sizeof(Vertex),vertices[0].position.getComponents());
		}
	};

template <class PositionScalarParam,int numPositionComponentsParam>
class GLVertexPointerImplementation<void,0,
                                    void,0,
                                    void,
                                    PositionScalarParam,numPositionComponentsParam>
	{
	/* Embedded classes: */
	public:
	typedef GLGeometry::Vertex<void,0,
	                           void,0,
	                           void,
	                           PositionScalarParam,numPositionComponentsParam> Vertex;
	
	/* Methods: */
	static void glVertex(const Vertex& vertex)
		{
		::glVertex<numPositionComponentsParam>(vertex.position.getComponents());
		}
	static void glVertexPointer(const Vertex* vertices)
		{
		::glVertexPointer(numPositionComponentsParam,sizeof(Vertex),vertices[0].position.getComponents());
		}
	static void glVertexPointer(int vertexPartsMask,const Vertex* vertices)
		{
		if(vertexPartsMask&GLVertexArrayParts::Position)
			::glVertexPointer(numPositionComponentsParam,sizeof(Vertex),vertices[0].position.getComponents());
		}
	};

}

/********************************
Generic version of glVertex call:
********************************/

template <class TexCoordScalarParam,int numTexCoordComponentsParam,
          class ColorScalarParam,int numColorComponentsParam,
          class NormalScalarParam,
          class PositionScalarParam,int numPositionComponentsParam>
METHODPREFIX
void glVertex(const GLGeometry::Vertex<TexCoordScalarParam,numTexCoordComponentsParam,
                                       ColorScalarParam,numColorComponentsParam,
                                       NormalScalarParam,
                                       PositionScalarParam,numPositionComponentsParam>& vertex)
	{
	GLVertexPointerImplementation<TexCoordScalarParam,numTexCoordComponentsParam,
	                              ColorScalarParam,numColorComponentsParam,
	                              NormalScalarParam,
	                              PositionScalarParam,numPositionComponentsParam>::glVertex(vertex);
	}

/***************************************
Generic version of glVertexPointer call:
***************************************/

template <class TexCoordScalarParam,int numTexCoordComponentsParam,
          class ColorScalarParam,int numColorComponentsParam,
          class NormalScalarParam,
          class PositionScalarParam,int numPositionComponentsParam>
METHODPREFIX
void glVertexPointer(const GLGeometry::Vertex<TexCoordScalarParam,numTexCoordComponentsParam,
                                              ColorScalarParam,numColorComponentsParam,
                                              NormalScalarParam,
                                              PositionScalarParam,numPositionComponentsParam>* vertexPointer)
	{
	GLVertexPointerImplementation<TexCoordScalarParam,numTexCoordComponentsParam,
	                              ColorScalarParam,numColorComponentsParam,
	                              NormalScalarParam,
	                              PositionScalarParam,numPositionComponentsParam>::glVertexPointer(vertexPointer);
	}

template <class TexCoordScalarParam,int numTexCoordComponentsParam,
          class ColorScalarParam,int numColorComponentsParam,
          class NormalScalarParam,
          class PositionScalarParam,int numPositionComponentsParam>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLGeometry::Vertex<TexCoordScalarParam,numTexCoordComponentsParam,
                                              ColorScalarParam,numColorComponentsParam,
                                              NormalScalarParam,
                                              PositionScalarParam,numPositionComponentsParam>* vertexPointer)
	{
	GLVertexPointerImplementation<TexCoordScalarParam,numTexCoordComponentsParam,
	                              ColorScalarParam,numColorComponentsParam,
	                              NormalScalarParam,
	                              PositionScalarParam,numPositionComponentsParam>::glVertexPointer(vertexPartsMask,vertexPointer);
	}

/*********************************************************************
Specialized versions of glVertexPointer call using interleaved arrays:
*********************************************************************/

template <>
METHODPREFIX
void glVertexPointer(const GLGeometry::Vertex<GLfloat,4,
                                              GLfloat,4,
                                              GLfloat,
                                              GLfloat,4>* vertexPointer)
	{
	glInterleavedArrays(GL_T4F_C4F_N3F_V4F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLGeometry::Vertex<GLfloat,2,
                                              GLfloat,4,
                                              GLfloat,
                                              GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_T2F_C4F_N3F_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLGeometry::Vertex<GLfloat,2,
                                              void,0,
                                              GLfloat,
                                              GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_T2F_N3F_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLGeometry::Vertex<GLfloat,2,
                                              GLfloat,3,
                                              void,
                                              GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_T2F_C3F_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLGeometry::Vertex<GLfloat,2,
                                              GLubyte,4,
                                              void,
                                              GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_T2F_C4UB_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLGeometry::Vertex<GLfloat,4,
                                              void,0,
                                              void,
                                              GLfloat,4>* vertexPointer)
	{
	glInterleavedArrays(GL_T4F_V4F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLGeometry::Vertex<GLfloat,2,
                                              void,0,
                                              void,
                                              GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_T2F_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLGeometry::Vertex<void,0,
                                              GLfloat,4,
                                              GLfloat,
                                              GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_C4F_N3F_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLGeometry::Vertex<void,0,
                                              void,0,
                                              GLfloat,
                                              GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_N3F_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLGeometry::Vertex<void,0,
                                              GLfloat,3,
                                              void,
                                              GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_C3F_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLGeometry::Vertex<void,0,
                                              GLubyte,4,
                                              void,
                                              GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_C4UB_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLGeometry::Vertex<void,0,
                                              GLubyte,4,
                                              void,
                                              GLfloat,2>* vertexPointer)
	{
	glInterleavedArrays(GL_C4UB_V2F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLGeometry::Vertex<void,0,
                                              void,0,
                                              void,
                                              GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLGeometry::Vertex<void,0,
                                              void,0,
                                              void,
                                              GLfloat,2>* vertexPointer)
	{
	glInterleavedArrays(GL_V2F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLGeometry::Vertex<GLfloat,4,
                                              GLfloat,4,
                                              GLfloat,
                                              GLfloat,4>* vertexPointer)
	{
	const int fullMask=GLVertexArrayParts::Position|GLVertexArrayParts::Normal|GLVertexArrayParts::Color|GLVertexArrayParts::TexCoord;
	if((vertexPartsMask&fullMask)==fullMask)
		glInterleavedArrays(GL_T4F_C4F_N3F_V4F,0,vertexPointer);
	else
		{
		if(vertexPartsMask&GLVertexArrayParts::TexCoord)
			glTexCoordPointer(4,sizeof(vertexPointer[0]),vertexPointer[0].texCoord.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(4,sizeof(vertexPointer[0]),vertexPointer[0].color.getRgba());
		if(vertexPartsMask&GLVertexArrayParts::Normal)
			glNormalPointer(sizeof(vertexPointer[0]),vertexPointer[0].normal.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(4,sizeof(vertexPointer[0]),vertexPointer[0].position.getComponents());
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLGeometry::Vertex<GLfloat,2,
                                              GLfloat,4,
                                              GLfloat,
                                              GLfloat,3>* vertexPointer)
	{
	const int fullMask=GLVertexArrayParts::Position|GLVertexArrayParts::Normal|GLVertexArrayParts::Color|GLVertexArrayParts::TexCoord;
	if((vertexPartsMask&fullMask)==fullMask)
		glInterleavedArrays(GL_T2F_C4F_N3F_V3F,0,vertexPointer);
	else
		{
		if(vertexPartsMask&GLVertexArrayParts::TexCoord)
			glTexCoordPointer(2,sizeof(vertexPointer[0]),vertexPointer[0].texCoord.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(4,sizeof(vertexPointer[0]),vertexPointer[0].color.getRgba());
		if(vertexPartsMask&GLVertexArrayParts::Normal)
			glNormalPointer(sizeof(vertexPointer[0]),vertexPointer[0].normal.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(3,sizeof(vertexPointer[0]),vertexPointer[0].position.getComponents());
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLGeometry::Vertex<GLfloat,2,
                                              void,0,
                                              GLfloat,
                                              GLfloat,3>* vertexPointer)
	{
	const int fullMask=GLVertexArrayParts::Position|GLVertexArrayParts::Normal|GLVertexArrayParts::TexCoord;
	if((vertexPartsMask&fullMask)==fullMask)
		glInterleavedArrays(GL_T2F_N3F_V3F,0,vertexPointer);
	else
		{
		if(vertexPartsMask&GLVertexArrayParts::TexCoord)
			glTexCoordPointer(2,sizeof(vertexPointer[0]),vertexPointer[0].texCoord.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Normal)
			glNormalPointer(sizeof(vertexPointer[0]),vertexPointer[0].normal.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(3,sizeof(vertexPointer[0]),vertexPointer[0].position.getComponents());
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLGeometry::Vertex<GLfloat,2,
                                              GLfloat,3,
                                              void,
                                              GLfloat,3>* vertexPointer)
	{
	const int fullMask=GLVertexArrayParts::Position|GLVertexArrayParts::Color|GLVertexArrayParts::TexCoord;
	if((vertexPartsMask&fullMask)==fullMask)
		glInterleavedArrays(GL_T2F_C3F_V3F,0,vertexPointer);
	else
		{
		if(vertexPartsMask&GLVertexArrayParts::TexCoord)
			glTexCoordPointer(2,sizeof(vertexPointer[0]),vertexPointer[0].texCoord.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(3,sizeof(vertexPointer[0]),vertexPointer[0].color.getRgba());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(3,sizeof(vertexPointer[0]),vertexPointer[0].position.getComponents());
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLGeometry::Vertex<GLfloat,2,
                                              GLubyte,4,
                                              void,
                                              GLfloat,3>* vertexPointer)
	{
	const int fullMask=GLVertexArrayParts::Position|GLVertexArrayParts::Color|GLVertexArrayParts::TexCoord;
	if((vertexPartsMask&fullMask)==fullMask)
		glInterleavedArrays(GL_T2F_C4UB_V3F,0,vertexPointer);
	else
		{
		if(vertexPartsMask&GLVertexArrayParts::TexCoord)
			glTexCoordPointer(2,sizeof(vertexPointer[0]),vertexPointer[0].texCoord.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(4,sizeof(vertexPointer[0]),vertexPointer[0].color.getRgba());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(3,sizeof(vertexPointer[0]),vertexPointer[0].position.getComponents());
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLGeometry::Vertex<GLfloat,4,
                                              void,0,
                                              void,
                                              GLfloat,4>* vertexPointer)
	{
	const int fullMask=GLVertexArrayParts::Position|GLVertexArrayParts::TexCoord;
	if((vertexPartsMask&fullMask)==fullMask)
		glInterleavedArrays(GL_T4F_V4F,0,vertexPointer);
	else
		{
		if(vertexPartsMask&GLVertexArrayParts::TexCoord)
			glTexCoordPointer(4,sizeof(vertexPointer[0]),vertexPointer[0].texCoord.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(4,sizeof(vertexPointer[0]),vertexPointer[0].position.getComponents());
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLGeometry::Vertex<GLfloat,2,
                                              void,0,
                                              void,
                                              GLfloat,3>* vertexPointer)
	{
	const int fullMask=GLVertexArrayParts::Position|GLVertexArrayParts::TexCoord;
	if((vertexPartsMask&fullMask)==fullMask)
		glInterleavedArrays(GL_T2F_V3F,0,vertexPointer);
	else
		{
		if(vertexPartsMask&GLVertexArrayParts::TexCoord)
			glTexCoordPointer(2,sizeof(vertexPointer[0]),vertexPointer[0].texCoord.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(3,sizeof(vertexPointer[0]),vertexPointer[0].position.getComponents());
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLGeometry::Vertex<void,0,
                                              GLfloat,4,
                                              GLfloat,
                                              GLfloat,3>* vertexPointer)
	{
	const int fullMask=GLVertexArrayParts::Position|GLVertexArrayParts::Normal|GLVertexArrayParts::Color;
	if((vertexPartsMask&fullMask)==fullMask)
		glInterleavedArrays(GL_C4F_N3F_V3F,0,vertexPointer);
	else
		{
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(4,sizeof(vertexPointer[0]),vertexPointer[0].color.getRgba());
		if(vertexPartsMask&GLVertexArrayParts::Normal)
			glNormalPointer(sizeof(vertexPointer[0]),vertexPointer[0].normal.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(3,sizeof(vertexPointer[0]),vertexPointer[0].position.getComponents());
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLGeometry::Vertex<void,0,
                                              void,0,
                                              GLfloat,
                                              GLfloat,3>* vertexPointer)
	{
	const int fullMask=GLVertexArrayParts::Position|GLVertexArrayParts::Normal;
	if((vertexPartsMask&fullMask)==fullMask)
		glInterleavedArrays(GL_N3F_V3F,0,vertexPointer);
	else
		{
		if(vertexPartsMask&GLVertexArrayParts::Normal)
			glNormalPointer(sizeof(vertexPointer[0]),vertexPointer[0].normal.getComponents());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(3,sizeof(vertexPointer[0]),vertexPointer[0].position.getComponents());
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLGeometry::Vertex<void,0,
                                              GLfloat,3,
                                              void,
                                              GLfloat,3>* vertexPointer)
	{
	const int fullMask=GLVertexArrayParts::Position|GLVertexArrayParts::Color;
	if((vertexPartsMask&fullMask)==fullMask)
		glInterleavedArrays(GL_C3F_V3F,0,vertexPointer);
	else
		{
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(3,sizeof(vertexPointer[0]),vertexPointer[0].color.getRgba());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(3,sizeof(vertexPointer[0]),vertexPointer[0].position.getComponents());
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLGeometry::Vertex<void,0,
                                              GLubyte,4,
                                              void,
                                              GLfloat,3>* vertexPointer)
	{
	const int fullMask=GLVertexArrayParts::Position|GLVertexArrayParts::Color;
	if((vertexPartsMask&fullMask)==fullMask)
		glInterleavedArrays(GL_C4UB_V3F,0,vertexPointer);
	else
		{
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(4,sizeof(vertexPointer[0]),vertexPointer[0].color.getRgba());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(3,sizeof(vertexPointer[0]),vertexPointer[0].position.getComponents());
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLGeometry::Vertex<void,0,
                                              GLubyte,4,
                                              void,
                                              GLfloat,2>* vertexPointer)
	{
	const int fullMask=GLVertexArrayParts::Position|GLVertexArrayParts::Color;
	if((vertexPartsMask&fullMask)==fullMask)
		glInterleavedArrays(GL_C4UB_V2F,0,vertexPointer);
	else
		{
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(4,sizeof(vertexPointer[0]),vertexPointer[0].color.getRgba());
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(2,sizeof(vertexPointer[0]),vertexPointer[0].position.getComponents());
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLGeometry::Vertex<void,0,
                                              void,0,
                                              void,
                                              GLfloat,3>* vertexPointer)
	{
	const int fullMask=GLVertexArrayParts::Position;
	if((vertexPartsMask&fullMask)==fullMask)
		glInterleavedArrays(GL_V3F,0,vertexPointer);
	else
		{
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(3,sizeof(vertexPointer[0]),vertexPointer[0].position.getComponents());
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLGeometry::Vertex<void,0,
                                              void,0,
                                              void,
                                              GLfloat,2>* vertexPointer)
	{
	const int fullMask=GLVertexArrayParts::Position;
	if((vertexPartsMask&fullMask)==fullMask)
		glInterleavedArrays(GL_V2F,0,vertexPointer);
	else
		{
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(2,sizeof(vertexPointer[0]),vertexPointer[0].position.getComponents());
		}
	}

#if !defined(NONSTANDARD_GLVERTEX_TEMPLATES)

/******************************************************************
Force instantiation of standard glVertex/glVertexPointer templates:
******************************************************************/

namespace GLGeometry {

template class Vertex<GLfloat,4,GLfloat,4,GLfloat,GLfloat,4>;
template class Vertex<GLfloat,2,GLfloat,4,GLfloat,GLfloat,3>;
template class Vertex<GLfloat,2,void,0,GLfloat,GLfloat,3>;
template class Vertex<GLfloat,2,GLfloat,3,void,GLfloat,3>;
template class Vertex<GLfloat,2,GLubyte,4,void,GLfloat,3>;
template class Vertex<GLfloat,4,void,0,void,GLfloat,4>;
template class Vertex<GLfloat,2,void,0,void,GLfloat,3>;
template class Vertex<void,0,GLfloat,4,GLfloat,GLfloat,3>;
template class Vertex<void,0,void,0,GLfloat,GLfloat,3>;
template class Vertex<void,0,GLfloat,3,void,GLfloat,3>;
template class Vertex<void,0,GLubyte,4,void,GLfloat,3>;
template class Vertex<void,0,GLubyte,4,void,GLfloat,2>;
template class Vertex<void,0,void,0,void,GLfloat,3>;
template class Vertex<void,0,void,0,void,GLfloat,2>;

}

template void glVertex(const GLGeometry::Vertex<GLfloat,4,GLfloat,4,GLfloat,GLfloat,4>&);
template void glVertex(const GLGeometry::Vertex<GLfloat,2,GLfloat,4,GLfloat,GLfloat,3>&);
template void glVertex(const GLGeometry::Vertex<GLfloat,2,void,0,GLfloat,GLfloat,3>&);
template void glVertex(const GLGeometry::Vertex<GLfloat,2,GLfloat,3,void,GLfloat,3>&);
template void glVertex(const GLGeometry::Vertex<GLfloat,2,GLubyte,4,void,GLfloat,3>&);
template void glVertex(const GLGeometry::Vertex<GLfloat,4,void,0,void,GLfloat,4>&);
template void glVertex(const GLGeometry::Vertex<GLfloat,2,void,0,void,GLfloat,3>&);
template void glVertex(const GLGeometry::Vertex<void,0,GLfloat,4,GLfloat,GLfloat,3>&);
template void glVertex(const GLGeometry::Vertex<void,0,void,0,GLfloat,GLfloat,3>&);
template void glVertex(const GLGeometry::Vertex<void,0,GLfloat,3,void,GLfloat,3>&);
template void glVertex(const GLGeometry::Vertex<void,0,GLubyte,4,void,GLfloat,3>&);
template void glVertex(const GLGeometry::Vertex<void,0,GLubyte,4,void,GLfloat,2>&);
template void glVertex(const GLGeometry::Vertex<void,0,void,0,void,GLfloat,3>&);
template void glVertex(const GLGeometry::Vertex<void,0,void,0,void,GLfloat,2>&);

template void glVertexPointer(const GLGeometry::Vertex<GLfloat,4,GLfloat,4,GLfloat,GLfloat,4>*);
template void glVertexPointer(const GLGeometry::Vertex<GLfloat,2,GLfloat,4,GLfloat,GLfloat,3>*);
template void glVertexPointer(const GLGeometry::Vertex<GLfloat,2,void,0,GLfloat,GLfloat,3>*);
template void glVertexPointer(const GLGeometry::Vertex<GLfloat,2,GLfloat,3,void,GLfloat,3>*);
template void glVertexPointer(const GLGeometry::Vertex<GLfloat,2,GLubyte,4,void,GLfloat,3>*);
template void glVertexPointer(const GLGeometry::Vertex<GLfloat,4,void,0,void,GLfloat,4>*);
template void glVertexPointer(const GLGeometry::Vertex<GLfloat,2,void,0,void,GLfloat,3>*);
template void glVertexPointer(const GLGeometry::Vertex<void,0,GLfloat,4,GLfloat,GLfloat,3>*);
template void glVertexPointer(const GLGeometry::Vertex<void,0,void,0,GLfloat,GLfloat,3>*);
template void glVertexPointer(const GLGeometry::Vertex<void,0,GLfloat,3,void,GLfloat,3>*);
template void glVertexPointer(const GLGeometry::Vertex<void,0,GLubyte,4,void,GLfloat,3>*);
template void glVertexPointer(const GLGeometry::Vertex<void,0,GLubyte,4,void,GLfloat,2>*);
template void glVertexPointer(const GLGeometry::Vertex<void,0,void,0,void,GLfloat,3>*);
template void glVertexPointer(const GLGeometry::Vertex<void,0,void,0,void,GLfloat,2>*);

template void glVertexPointer(int vertexPartsMask,const GLGeometry::Vertex<GLfloat,4,GLfloat,4,GLfloat,GLfloat,4>*);
template void glVertexPointer(int vertexPartsMask,const GLGeometry::Vertex<GLfloat,2,GLfloat,4,GLfloat,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLGeometry::Vertex<GLfloat,2,void,0,GLfloat,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLGeometry::Vertex<GLfloat,2,GLfloat,3,void,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLGeometry::Vertex<GLfloat,2,GLubyte,4,void,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLGeometry::Vertex<GLfloat,4,void,0,void,GLfloat,4>*);
template void glVertexPointer(int vertexPartsMask,const GLGeometry::Vertex<GLfloat,2,void,0,void,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLGeometry::Vertex<void,0,GLfloat,4,GLfloat,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLGeometry::Vertex<void,0,void,0,GLfloat,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLGeometry::Vertex<void,0,GLfloat,3,void,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLGeometry::Vertex<void,0,GLubyte,4,void,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLGeometry::Vertex<void,0,GLubyte,4,void,GLfloat,2>*);
template void glVertexPointer(int vertexPartsMask,const GLGeometry::Vertex<void,0,void,0,void,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLGeometry::Vertex<void,0,void,0,void,GLfloat,2>*);

#endif
