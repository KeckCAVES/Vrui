/***********************************************************************
GLVertex - Class to encapsulate OpenGL vertex properties.
Copyright (c) 2004-2005 Oliver Kreylos

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

#define GLVERTEX_IMPLEMENTATION

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

#include <GL/GLVertex.h>

/*******************************************************************
Helper class to generate templatized glVertex/glVertexPointer calls:
*******************************************************************/

namespace {

template <class TexCoordScalarParam,GLsizei numTexCoordComponentsParam,
          class ColorScalarParam,GLsizei numColorComponentsParam,
          class NormalScalarParam,
          class PositionScalarParam,GLsizei numPositionComponentsParam>
class GLVertexPointerImplementation
	{
	/* Embedded classes: */
	public:
	typedef GLVertex<TexCoordScalarParam,numTexCoordComponentsParam,
                   ColorScalarParam,numColorComponentsParam,
                   NormalScalarParam,
                   PositionScalarParam,numPositionComponentsParam> Vertex;
	
	/* Methods: */
	static void glVertex(const Vertex& vertex)
		{
		glTexCoord(vertex.texCoord);
		glColor(vertex.color);
		glNormal(vertex.normal);
		::glVertex(vertex.position);
		}
	static void glVertexPointer(const Vertex* vertices)
		{
		glTexCoordPointer(sizeof(vertices[0]),&vertices[0].texCoord);
		glColorPointer(sizeof(vertices[0]),&vertices[0].color);
		glNormalPointer(sizeof(vertices[0]),&vertices[0].normal);
		::glVertexPointer(sizeof(vertices[0]),&vertices[0].position);
		}
	static void glVertexPointer(int vertexPartsMask,const Vertex* vertices)
		{
		if(vertexPartsMask&GLVertexArrayParts::TexCoord)
			glTexCoordPointer(sizeof(vertices[0]),&vertices[0].texCoord);
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(sizeof(vertices[0]),&vertices[0].color);
		if(vertexPartsMask&GLVertexArrayParts::Normal)
			glNormalPointer(sizeof(vertices[0]),&vertices[0].normal);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			::glVertexPointer(sizeof(vertices[0]),&vertices[0].position);
		}
	};

template <class TexCoordScalarParam,GLsizei numTexCoordComponentsParam,
          class NormalScalarParam,
          class PositionScalarParam,GLsizei numPositionComponentsParam>
class GLVertexPointerImplementation<TexCoordScalarParam,numTexCoordComponentsParam,
                                    void,0,
                                    NormalScalarParam,
                                    PositionScalarParam,numPositionComponentsParam>
	{
	/* Embedded classes: */
	public:
	typedef GLVertex<TexCoordScalarParam,numTexCoordComponentsParam,
                   void,0,
                   NormalScalarParam,
                   PositionScalarParam,numPositionComponentsParam> Vertex;
	
	/* Methods: */
	static void glVertex(const Vertex& vertex)
		{
		glTexCoord(vertex.texCoord);
		glNormal(vertex.normal);
		::glVertex(vertex.position);
		}
	static void glVertexPointer(const Vertex* vertices)
		{
		glTexCoordPointer(sizeof(vertices[0]),&vertices[0].texCoord);
		glNormalPointer(sizeof(vertices[0]),&vertices[0].normal);
		::glVertexPointer(sizeof(vertices[0]),&vertices[0].position);
		}
	static void glVertexPointer(int vertexPartsMask,const Vertex* vertices)
		{
		if(vertexPartsMask&GLVertexArrayParts::TexCoord)
			glTexCoordPointer(sizeof(vertices[0]),&vertices[0].texCoord);
		if(vertexPartsMask&GLVertexArrayParts::Normal)
			glNormalPointer(sizeof(vertices[0]),&vertices[0].normal);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			::glVertexPointer(sizeof(vertices[0]),&vertices[0].position);
		}
	};

template <class TexCoordScalarParam,GLsizei numTexCoordComponentsParam,
          class ColorScalarParam,GLsizei numColorComponentsParam,
          class PositionScalarParam,GLsizei numPositionComponentsParam>
class GLVertexPointerImplementation<TexCoordScalarParam,numTexCoordComponentsParam,
                                    ColorScalarParam,numColorComponentsParam,
                                    void,
                                    PositionScalarParam,numPositionComponentsParam>
	{
	/* Embedded classes: */
	public:
	typedef GLVertex<TexCoordScalarParam,numTexCoordComponentsParam,
                   ColorScalarParam,numColorComponentsParam,
                   void,
                   PositionScalarParam,numPositionComponentsParam> Vertex;
	
	/* Methods: */
	static void glVertex(const Vertex& vertex)
		{
		glTexCoord(vertex.texCoord);
		glColor(vertex.color);
		::glVertex(vertex.position);
		}
	static void glVertexPointer(const Vertex* vertices)
		{
		glTexCoordPointer(sizeof(vertices[0]),&vertices[0].texCoord);
		glColorPointer(sizeof(vertices[0]),&vertices[0].color);
		::glVertexPointer(sizeof(vertices[0]),&vertices[0].position);
		}
	static void glVertexPointer(int vertexPartsMask,const Vertex* vertices)
		{
		if(vertexPartsMask&GLVertexArrayParts::TexCoord)
			glTexCoordPointer(sizeof(vertices[0]),&vertices[0].texCoord);
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(sizeof(vertices[0]),&vertices[0].color);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			::glVertexPointer(sizeof(vertices[0]),&vertices[0].position);
		}
	};

template <class ColorScalarParam,GLsizei numColorComponentsParam,
          class NormalScalarParam,
          class PositionScalarParam,GLsizei numPositionComponentsParam>
class GLVertexPointerImplementation<void,0,
                                    ColorScalarParam,numColorComponentsParam,
                                    NormalScalarParam,
                                    PositionScalarParam,numPositionComponentsParam>
	{
	/* Embedded classes: */
	public:
	typedef GLVertex<void,0,
                   ColorScalarParam,numColorComponentsParam,
                   NormalScalarParam,
                   PositionScalarParam,numPositionComponentsParam> Vertex;
	
	/* Methods: */
	static void glVertex(const Vertex& vertex)
		{
		glColor(vertex.color);
		glNormal(vertex.normal);
		::glVertex(vertex.position);
		}
	static void glVertexPointer(const Vertex* vertices)
		{
		glColorPointer(sizeof(vertices[0]),&vertices[0].color);
		glNormalPointer(sizeof(vertices[0]),&vertices[0].normal);
		::glVertexPointer(sizeof(vertices[0]),&vertices[0].position);
		}
	static void glVertexPointer(int vertexPartsMask,const Vertex* vertices)
		{
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(sizeof(vertices[0]),&vertices[0].color);
		if(vertexPartsMask&GLVertexArrayParts::Normal)
			glNormalPointer(sizeof(vertices[0]),&vertices[0].normal);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			::glVertexPointer(sizeof(vertices[0]),&vertices[0].position);
		}
	};

template <class TexCoordScalarParam,GLsizei numTexCoordComponentsParam,
          class PositionScalarParam,GLsizei numPositionComponentsParam>
class GLVertexPointerImplementation<TexCoordScalarParam,numTexCoordComponentsParam,
                                    void,0,
                                    void,
                                    PositionScalarParam,numPositionComponentsParam>
	{
	/* Embedded classes: */
	public:
	typedef GLVertex<TexCoordScalarParam,numTexCoordComponentsParam,
                   void,0,
                   void,
                   PositionScalarParam,numPositionComponentsParam> Vertex;
	
	/* Methods: */
	static void glVertex(const Vertex& vertex)
		{
		glTexCoord(vertex.texCoord);
		::glVertex(vertex.position);
		}
	static void glVertexPointer(const Vertex* vertices)
		{
		glTexCoordPointer(sizeof(vertices[0]),&vertices[0].texCoord);
		::glVertexPointer(sizeof(vertices[0]),&vertices[0].position);
		}
	static void glVertexPointer(int vertexPartsMask,const Vertex* vertices)
		{
		if(vertexPartsMask&GLVertexArrayParts::TexCoord)
			glTexCoordPointer(sizeof(vertices[0]),&vertices[0].texCoord);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			::glVertexPointer(sizeof(vertices[0]),&vertices[0].position);
		}
	};

template <class NormalScalarParam,
          class PositionScalarParam,GLsizei numPositionComponentsParam>
class GLVertexPointerImplementation<void,0,
                                    void,0,
                                    NormalScalarParam,
                                    PositionScalarParam,numPositionComponentsParam>
	{
	/* Embedded classes: */
	public:
	typedef GLVertex<void,0,
                   void,0,
                   NormalScalarParam,
                   PositionScalarParam,numPositionComponentsParam> Vertex;
	
	/* Methods: */
	static void glVertex(const Vertex& vertex)
		{
		glNormal(vertex.normal);
		::glVertex(vertex.position);
		}
	static void glVertexPointer(const Vertex* vertices)
		{
		glNormalPointer(sizeof(vertices[0]),&vertices[0].normal);
		::glVertexPointer(sizeof(vertices[0]),&vertices[0].position);
		}
	static void glVertexPointer(int vertexPartsMask,const Vertex* vertices)
		{
		if(vertexPartsMask&GLVertexArrayParts::Normal)
			glNormalPointer(sizeof(vertices[0]),&vertices[0].normal);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			::glVertexPointer(sizeof(vertices[0]),&vertices[0].position);
		}
	};

template <class ColorScalarParam,GLsizei numColorComponentsParam,
          class PositionScalarParam,GLsizei numPositionComponentsParam>
class GLVertexPointerImplementation<void,0,
                                    ColorScalarParam,numColorComponentsParam,
                                    void,
                                    PositionScalarParam,numPositionComponentsParam>
	{
	/* Embedded classes: */
	public:
	typedef GLVertex<void,0,
                   ColorScalarParam,numColorComponentsParam,
                   void,
                   PositionScalarParam,numPositionComponentsParam> Vertex;
	
	/* Methods: */
	static void glVertex(const Vertex& vertex)
		{
		glColor(vertex.color);
		::glVertex(vertex.position);
		}
	static void glVertexPointer(const Vertex* vertices)
		{
		glColorPointer(sizeof(vertices[0]),&vertices[0].color);
		::glVertexPointer(sizeof(vertices[0]),&vertices[0].position);
		}
	static void glVertexPointer(int vertexPartsMask,const Vertex* vertices)
		{
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(sizeof(vertices[0]),&vertices[0].color);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			::glVertexPointer(sizeof(vertices[0]),&vertices[0].position);
		}
	};

template <class PositionScalarParam,GLsizei numPositionComponentsParam>
class GLVertexPointerImplementation<void,0,
                                    void,0,
                                    void,
                                    PositionScalarParam,numPositionComponentsParam>
	{
	/* Embedded classes: */
	public:
	typedef GLVertex<void,0,
                   void,0,
                   void,
                   PositionScalarParam,numPositionComponentsParam> Vertex;
	
	/* Methods: */
	static void glVertex(const Vertex& vertex)
		{
		::glVertex(vertex.position);
		}
	static void glVertexPointer(const Vertex* vertices)
		{
		::glVertexPointer(sizeof(vertices[0]),&vertices[0].position);
		}
	static void glVertexPointer(int vertexPartsMask,const Vertex* vertices)
		{
		if(vertexPartsMask&GLVertexArrayParts::Position)
			::glVertexPointer(sizeof(vertices[0]),&vertices[0].position);
		}
	};

}

/********************************
Generic version of glVertex call:
********************************/

template <class TexCoordScalarParam,GLsizei numTexCoordComponentsParam,
          class ColorScalarParam,GLsizei numColorComponentsParam,
          class NormalScalarParam,
          class PositionScalarParam,GLsizei numPositionComponentsParam>
METHODPREFIX
void glVertex(const GLVertex<TexCoordScalarParam,numTexCoordComponentsParam,
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

template <class TexCoordScalarParam,GLsizei numTexCoordComponentsParam,
          class ColorScalarParam,GLsizei numColorComponentsParam,
          class NormalScalarParam,
          class PositionScalarParam,GLsizei numPositionComponentsParam>
METHODPREFIX
void glVertexPointer(const GLVertex<TexCoordScalarParam,numTexCoordComponentsParam,
                                    ColorScalarParam,numColorComponentsParam,
                                    NormalScalarParam,
                                    PositionScalarParam,numPositionComponentsParam>* vertexPointer)
	{
	GLVertexPointerImplementation<TexCoordScalarParam,numTexCoordComponentsParam,
                                ColorScalarParam,numColorComponentsParam,
                                NormalScalarParam,
                                PositionScalarParam,numPositionComponentsParam>::glVertexPointer(vertexPointer);
	}

template <class TexCoordScalarParam,GLsizei numTexCoordComponentsParam,
          class ColorScalarParam,GLsizei numColorComponentsParam,
          class NormalScalarParam,
          class PositionScalarParam,GLsizei numPositionComponentsParam>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLVertex<TexCoordScalarParam,numTexCoordComponentsParam,
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
void glVertexPointer(const GLVertex<GLfloat,4,
                                    GLfloat,4,
                                    GLfloat,
                                    GLfloat,4>* vertexPointer)
	{
	glInterleavedArrays(GL_T4F_C4F_N3F_V4F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLVertex<GLfloat,2,
                                    GLfloat,4,
                                    GLfloat,
                                    GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_T2F_C4F_N3F_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLVertex<GLfloat,2,
                                    void,0,
                                    GLfloat,
                                    GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_T2F_N3F_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLVertex<GLfloat,2,
                                    GLfloat,3,
                                    void,
                                    GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_T2F_C3F_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLVertex<GLfloat,2,
                                    GLubyte,4,
                                    void,
                                    GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_T2F_C4UB_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLVertex<GLfloat,4,
                                    void,0,
                                    void,
                                    GLfloat,4>* vertexPointer)
	{
	glInterleavedArrays(GL_T4F_V4F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLVertex<GLfloat,2,
                                    void,0,
                                    void,
                                    GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_T2F_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLVertex<void,0,
                                    GLfloat,4,
                                    GLfloat,
                                    GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_C4F_N3F_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLVertex<void,0,
                                    void,0,
                                    GLfloat,
                                    GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_N3F_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLVertex<void,0,
                                    GLfloat,3,
                                    void,
                                    GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_C3F_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLVertex<void,0,
                                    GLubyte,4,
                                    void,
                                    GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_C4UB_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLVertex<void,0,
                                    GLubyte,4,
                                    void,
                                    GLfloat,2>* vertexPointer)
	{
	glInterleavedArrays(GL_C4UB_V2F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLVertex<void,0,
                                    void,0,
                                    void,
                                    GLfloat,3>* vertexPointer)
	{
	glInterleavedArrays(GL_V3F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(const GLVertex<void,0,
                                    void,0,
                                    void,
                                    GLfloat,2>* vertexPointer)
	{
	glInterleavedArrays(GL_V2F,0,vertexPointer);
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLVertex<GLfloat,4,
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
			glTexCoordPointer(sizeof(vertexPointer[0]),&vertexPointer[0].texCoord);
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(sizeof(vertexPointer[0]),&vertexPointer[0].color);
		if(vertexPartsMask&GLVertexArrayParts::Normal)
			glNormalPointer(sizeof(vertexPointer[0]),&vertexPointer[0].normal);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(sizeof(vertexPointer[0]),&vertexPointer[0].position);
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLVertex<GLfloat,2,
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
			glTexCoordPointer(sizeof(vertexPointer[0]),&vertexPointer[0].texCoord);
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(sizeof(vertexPointer[0]),&vertexPointer[0].color);
		if(vertexPartsMask&GLVertexArrayParts::Normal)
			glNormalPointer(sizeof(vertexPointer[0]),&vertexPointer[0].normal);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(sizeof(vertexPointer[0]),&vertexPointer[0].position);
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLVertex<GLfloat,2,
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
			glTexCoordPointer(sizeof(vertexPointer[0]),&vertexPointer[0].texCoord);
		if(vertexPartsMask&GLVertexArrayParts::Normal)
			glNormalPointer(sizeof(vertexPointer[0]),&vertexPointer[0].normal);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(sizeof(vertexPointer[0]),&vertexPointer[0].position);
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLVertex<GLfloat,2,
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
			glTexCoordPointer(sizeof(vertexPointer[0]),&vertexPointer[0].texCoord);
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(sizeof(vertexPointer[0]),&vertexPointer[0].color);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(sizeof(vertexPointer[0]),&vertexPointer[0].position);
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLVertex<GLfloat,2,
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
			glTexCoordPointer(sizeof(vertexPointer[0]),&vertexPointer[0].texCoord);
		if(vertexPartsMask&GLVertexArrayParts::Color)
			glColorPointer(sizeof(vertexPointer[0]),&vertexPointer[0].color);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(sizeof(vertexPointer[0]),&vertexPointer[0].position);
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLVertex<GLfloat,4,
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
			glTexCoordPointer(sizeof(vertexPointer[0]),&vertexPointer[0].texCoord);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(sizeof(vertexPointer[0]),&vertexPointer[0].position);
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLVertex<GLfloat,2,
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
			glTexCoordPointer(sizeof(vertexPointer[0]),&vertexPointer[0].texCoord);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(sizeof(vertexPointer[0]),&vertexPointer[0].position);
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLVertex<void,0,
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
			glColorPointer(sizeof(vertexPointer[0]),&vertexPointer[0].color);
		if(vertexPartsMask&GLVertexArrayParts::Normal)
			glNormalPointer(sizeof(vertexPointer[0]),&vertexPointer[0].normal);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(sizeof(vertexPointer[0]),&vertexPointer[0].position);
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLVertex<void,0,
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
			glNormalPointer(sizeof(vertexPointer[0]),&vertexPointer[0].normal);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(sizeof(vertexPointer[0]),&vertexPointer[0].position);
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLVertex<void,0,
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
			glColorPointer(sizeof(vertexPointer[0]),&vertexPointer[0].color);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(sizeof(vertexPointer[0]),&vertexPointer[0].position);
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLVertex<void,0,
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
			glColorPointer(sizeof(vertexPointer[0]),&vertexPointer[0].color);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(sizeof(vertexPointer[0]),&vertexPointer[0].position);
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLVertex<void,0,
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
			glColorPointer(sizeof(vertexPointer[0]),&vertexPointer[0].color);
		if(vertexPartsMask&GLVertexArrayParts::Position)
			glVertexPointer(sizeof(vertexPointer[0]),&vertexPointer[0].position);
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLVertex<void,0,
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
			glVertexPointer(sizeof(vertexPointer[0]),&vertexPointer[0].position);
		}
	}

template <>
METHODPREFIX
void glVertexPointer(int vertexPartsMask,
                     const GLVertex<void,0,
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
			glVertexPointer(sizeof(vertexPointer[0]),&vertexPointer[0].position);
		}
	}

#if !defined(NONSTANDARD_GLVERTEX_TEMPLATES)

/******************************************************************
Force instantiation of standard glVertex/glVertexPointer templates:
******************************************************************/

template class GLVertex<GLfloat,4,GLfloat,4,GLfloat,GLfloat,4>;
template class GLVertex<GLfloat,2,GLfloat,4,GLfloat,GLfloat,3>;
template class GLVertex<GLfloat,2,void,0,GLfloat,GLfloat,3>;
template class GLVertex<GLfloat,2,GLfloat,3,void,GLfloat,3>;
template class GLVertex<GLfloat,2,GLubyte,4,void,GLfloat,3>;
template class GLVertex<GLfloat,4,void,0,void,GLfloat,4>;
template class GLVertex<GLfloat,2,void,0,void,GLfloat,3>;
template class GLVertex<void,0,GLfloat,4,GLfloat,GLfloat,3>;
template class GLVertex<void,0,void,0,GLfloat,GLfloat,3>;
template class GLVertex<void,0,GLfloat,3,void,GLfloat,3>;
template class GLVertex<void,0,GLubyte,4,void,GLfloat,3>;
template class GLVertex<void,0,GLubyte,4,void,GLfloat,2>;
template class GLVertex<void,0,void,0,void,GLfloat,3>;
template class GLVertex<void,0,void,0,void,GLfloat,2>;

template void glVertex(const GLVertex<GLfloat,4,GLfloat,4,GLfloat,GLfloat,4>&);
template void glVertex(const GLVertex<GLfloat,2,GLfloat,4,GLfloat,GLfloat,3>&);
template void glVertex(const GLVertex<GLfloat,2,void,0,GLfloat,GLfloat,3>&);
template void glVertex(const GLVertex<GLfloat,2,GLfloat,3,void,GLfloat,3>&);
template void glVertex(const GLVertex<GLfloat,2,GLubyte,4,void,GLfloat,3>&);
template void glVertex(const GLVertex<GLfloat,4,void,0,void,GLfloat,4>&);
template void glVertex(const GLVertex<GLfloat,2,void,0,void,GLfloat,3>&);
template void glVertex(const GLVertex<void,0,GLfloat,4,GLfloat,GLfloat,3>&);
template void glVertex(const GLVertex<void,0,void,0,GLfloat,GLfloat,3>&);
template void glVertex(const GLVertex<void,0,GLfloat,3,void,GLfloat,3>&);
template void glVertex(const GLVertex<void,0,GLubyte,4,void,GLfloat,3>&);
template void glVertex(const GLVertex<void,0,GLubyte,4,void,GLfloat,2>&);
template void glVertex(const GLVertex<void,0,void,0,void,GLfloat,3>&);
template void glVertex(const GLVertex<void,0,void,0,void,GLfloat,2>&);

template void glVertexPointer(const GLVertex<GLfloat,4,GLfloat,4,GLfloat,GLfloat,4>*);
template void glVertexPointer(const GLVertex<GLfloat,2,GLfloat,4,GLfloat,GLfloat,3>*);
template void glVertexPointer(const GLVertex<GLfloat,2,void,0,GLfloat,GLfloat,3>*);
template void glVertexPointer(const GLVertex<GLfloat,2,GLfloat,3,void,GLfloat,3>*);
template void glVertexPointer(const GLVertex<GLfloat,2,GLubyte,4,void,GLfloat,3>*);
template void glVertexPointer(const GLVertex<GLfloat,4,void,0,void,GLfloat,4>*);
template void glVertexPointer(const GLVertex<GLfloat,2,void,0,void,GLfloat,3>*);
template void glVertexPointer(const GLVertex<void,0,GLfloat,4,GLfloat,GLfloat,3>*);
template void glVertexPointer(const GLVertex<void,0,void,0,GLfloat,GLfloat,3>*);
template void glVertexPointer(const GLVertex<void,0,GLfloat,3,void,GLfloat,3>*);
template void glVertexPointer(const GLVertex<void,0,GLubyte,4,void,GLfloat,3>*);
template void glVertexPointer(const GLVertex<void,0,GLubyte,4,void,GLfloat,2>*);
template void glVertexPointer(const GLVertex<void,0,void,0,void,GLfloat,3>*);
template void glVertexPointer(const GLVertex<void,0,void,0,void,GLfloat,2>*);

template void glVertexPointer(int vertexPartsMask,const GLVertex<GLfloat,4,GLfloat,4,GLfloat,GLfloat,4>*);
template void glVertexPointer(int vertexPartsMask,const GLVertex<GLfloat,2,GLfloat,4,GLfloat,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLVertex<GLfloat,2,void,0,GLfloat,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLVertex<GLfloat,2,GLfloat,3,void,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLVertex<GLfloat,2,GLubyte,4,void,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLVertex<GLfloat,4,void,0,void,GLfloat,4>*);
template void glVertexPointer(int vertexPartsMask,const GLVertex<GLfloat,2,void,0,void,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLVertex<void,0,GLfloat,4,GLfloat,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLVertex<void,0,void,0,GLfloat,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLVertex<void,0,GLfloat,3,void,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLVertex<void,0,GLubyte,4,void,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLVertex<void,0,GLubyte,4,void,GLfloat,2>*);
template void glVertexPointer(int vertexPartsMask,const GLVertex<void,0,void,0,void,GLfloat,3>*);
template void glVertexPointer(int vertexPartsMask,const GLVertex<void,0,void,0,void,GLfloat,2>*);

#endif
