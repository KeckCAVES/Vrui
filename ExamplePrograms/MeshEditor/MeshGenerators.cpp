/***********************************************************************
MeshGenerators - Functions to create meshes for several basic polyhedra,
and to load meshes from a variety of file formats.
Copyright (c) 2001-2006 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#define MESHGENERATORS_IMPLEMENTATION

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>
#include <vector>
#include <Misc/ThrowStdErr.h>

#include "Point.h"
#include "PlyFileStructures.h"

template <class MeshPoint>
PolygonMesh<MeshPoint>* createTetrahedron(void)
	{
	float c1=cos(30.0*M_PI/180.0);
	float s1=sin(30.0*M_PI/180.0);
	float tetAngle=acos(-1.0/3.0);
	float c2=sin(tetAngle);
	float s2=-cos(tetAngle);
	Point<float> points[]={Point<float>(-c1*c2,-s1*c2,-s2),Point<float>(c1*c2,-s1*c2,-s2),
	                       Point<float>(0.0,c2,-s2),Point<float>(0.0,0.0,1.0)};
	int vertexIndices[]={0,2,1,-1,0,1,3,-1,1,2,3,-1,2,0,3,-1,-1};
	return new PolygonMesh<MeshPoint>(4,points,vertexIndices,0,0);
	}

template <class MeshPoint>
PolygonMesh<MeshPoint>* createHexahedron(void)
	{
	float v=sqrt(3.0)/3.0;
	Point<float> points[]={Point<float>(-v,-v,-v),Point<float>(v,-v,-v),
	                       Point<float>(-v,v,-v),Point<float>(v,v,-v),
	                       Point<float>(-v,-v,v),Point<float>(v,-v,v),
	                       Point<float>(-v,v,v),Point<float>(v,v,v)};
	int vertexIndices[]={0,2,3,1,-1,4,5,7,6,-1,0,4,6,2,-1,1,3,7,5,-1,0,1,5,4,-1,2,6,7,3,-1,-1};
	return new PolygonMesh<MeshPoint>(8,points,vertexIndices,0,0);
	}

template <class MeshPoint>
PolygonMesh<MeshPoint>* createOctahedron(void)
	{
	Point<float> points[]={Point<float>(-1.0,0.0,0.0),Point<float>(1.0,0.0,0.0),
	                       Point<float>(0.0,-1.0,0.0),Point<float>(0.0,1.0,0.0),
	                       Point<float>(0.0,0.0,-1.0),Point<float>(0.0,0.0,1.0)};
	int vertexIndices[]={0,2,5,-1,0,5,3,-1,0,3,4,-1,0,4,2,-1,1,2,4,-1,1,4,3,-1,1,3,5,-1,1,5,2,-1,-1};
	return new PolygonMesh<MeshPoint>(6,points,vertexIndices,0,0);
	}

template <class MeshPoint>
PolygonMesh<MeshPoint>* createDodecahedron(void)
	{
	float a=0.35682209;
	float b=0.93417236;
	float c=sqrt(3.0)/3.0;
	Point<float> points[]={Point<float>(0.0,-a,-b),Point<float>(0.0,a,-b),
	                       Point<float>(0.0,-a,b),Point<float>(0.0,a,b),
	                       Point<float>(-b,0.0,-a),Point<float>(b,0.0,-a),
	                       Point<float>(-b,0.0,a),Point<float>(b,0.0,a),
	                       Point<float>(-a,-b,0.0),Point<float>(a,-b,0.0),
	                       Point<float>(-a,b,0.0),Point<float>(a,b,0.0),
	                       Point<float>(-c,-c,-c),Point<float>(c,-c,-c),
	                       Point<float>(-c,c,-c),Point<float>(c,c,-c),
	                       Point<float>(-c,-c,c),Point<float>(c,-c,c),
	                       Point<float>(-c,c,c),Point<float>(c,c,c)};
	int vertexIndices[]={0,13,9,8,12,-1,0,1,15,5,13,-1,0,12,4,14,1,-1,1,14,10,11,15,-1,2,16,8,9,17,-1,2,17,7,19,3,-1,
	                     2,3,18,6,16,-1,3,19,11,10,18,-1,4,12,8,16,6,-1,4,6,18,10,14,-1,5,7,17,9,13,-1,5,15,11,19,7,-1,-1};
	return new PolygonMesh<MeshPoint>(20,points,vertexIndices,0,0);
	}

template <class MeshPoint>
PolygonMesh<MeshPoint>* createIcosahedron(void)
	{
	float a=0.52573111;
	float b=0.85065081;
	Point<float> points[]={Point<float>(0.0,-a,-b),Point<float>(0.0,a,-b),
	                       Point<float>(0.0,-a,b),Point<float>(0.0,a,b),
	                       Point<float>(-b,0.0,-a),Point<float>(b,0.0,-a),
	                       Point<float>(-b,0.0,a),Point<float>(b,0.0,a),
	                       Point<float>(-a,-b,0.0),Point<float>(a,-b,0.0),
	                       Point<float>(-a,b,0.0),Point<float>(a,b,0.0)};
	int vertexIndices[]={0,9,8,-1,1,10,11,-1,2,8,9,-1,3,11,10,-1,4,1,0,-1,5,0,1,-1,6,2,3,-1,7,3,2,-1,8,6,4,-1,9,5,7,-1,
	                     10,4,6,-1,11,7,5,-1,0,5,9,-1,0,8,4,-1,1,4,10,-1,1,11,5,-1,2,6,8,-1,2,9,7,-1,3,7,11,-1,3,10,6,-1,-1};
	return new PolygonMesh<MeshPoint>(12,points,vertexIndices,0,0);
	}

template <class MeshPoint>
PolygonMesh<MeshPoint>* createTeapotahedron(void)
	{
	const int numpotpoints=306;
	Point<float> potpoints[]={Point<float>(1.4,0.0,2.4),Point<float>(1.4,-0.784,2.4),Point<float>(0.784,-1.4,2.4),
                    	      Point<float>(0.0,-1.4,2.4),Point<float>(1.3375,0.0,2.53125),Point<float>(1.3375,-0.749,2.53125),
                    	      Point<float>(0.749,-1.33750,2.53125),Point<float>(0.0,-1.3375,2.53125),Point<float>(1.4375,0.0,2.53125),
                    	      Point<float>(1.4375,-0.805,2.53125),Point<float>(0.805,-1.4375,2.53125),Point<float>(0.0,-1.4375,2.53125),
                    	      Point<float>(1.5,0.0,2.4),Point<float>(1.5,-0.84,2.4),Point<float>(0.84,-1.5,2.4),
                    	      Point<float>(0.0,-1.5,2.4),Point<float>(-0.784,-1.4,2.4),Point<float>(-1.4,-0.784,2.4),
                    	      Point<float>(-1.4,0.0,2.4),Point<float>(-0.749,-1.3375,2.53125),Point<float>(-1.3375,-0.749,2.53125),
                    	      Point<float>(-1.3375,0.0,2.53125),Point<float>(-0.805,-1.4375,2.53125),Point<float>(-1.4375,-0.805,2.53125),
                    	      Point<float>(-1.4375,0.0,2.53125),Point<float>(-0.84,-1.5,2.4),Point<float>(-1.5,-0.84,2.4),
                    	      Point<float>(-1.5,0.0,2.4),Point<float>(-1.4,0.784,2.4),Point<float>(-0.784,1.4,2.4),

                    	      Point<float>(0.0,1.4,2.4),Point<float>(-1.3375,0.749,2.53125),Point<float>(-0.749,1.3375,2.53125),
                    	      Point<float>(0.0,1.3375,2.53125),Point<float>(-1.4375,0.805,2.53125),Point<float>(-0.805,1.4375,2.53125),
                    	      Point<float>(0.0,1.4375,2.53125),Point<float>(-1.5,0.84,2.4),Point<float>(-0.84,1.5,2.4),
                    	      Point<float>(0.0,1.5,2.4),Point<float>(0.784,1.4,2.4),Point<float>(1.4,0.784,2.4),
                    	      Point<float>(0.749,1.3375,2.53125),Point<float>(1.3375,0.749,2.53125),Point<float>(0.805,1.4375,2.53125),
                    	      Point<float>(1.4375,0.805,2.53125),Point<float>(0.84,1.5,2.4),Point<float>(1.5,0.84,2.4),
                    	      Point<float>(1.75,0.0,1.875),Point<float>(1.75,-0.98,1.875),Point<float>(0.98,-1.75,1.875),
                    	      Point<float>(0.0,-1.75,1.875),Point<float>(2.0,0.0,1.35),Point<float>(2.0,-1.12,1.35),
                    	      Point<float>(1.12,-2.0,1.35),Point<float>(0.0,-2.0,1.35),Point<float>(2.0,0.0,0.9),
                    	      Point<float>(2.0,-1.12,0.9),Point<float>(1.12,-2.0,0.9),Point<float>(0.0,-2.0,0.9),

                    	      Point<float>(-0.98,-1.75,1.875),Point<float>(-1.75,-0.98,1.875),Point<float>(-1.75,0.0,1.875),
                    	      Point<float>(-1.12,-2.0,1.35),Point<float>(-2.0,-1.12,1.35),Point<float>(-2.0,0.0,1.35),
                    	      Point<float>(-1.12,-2.0,0.9),Point<float>(-2.0,-1.12,0.9),Point<float>(-2.0,0.0,0.9),
                    	      Point<float>(-1.75,0.98,1.875),Point<float>(-0.98,1.75,1.875),Point<float>(0.0,1.75,1.875),
                    	      Point<float>(-2.0,1.12,1.35),Point<float>(-1.12,2.0,1.35),Point<float>(0.0,2.0,1.35),
                    	      Point<float>(-2.0,1.12,0.9),Point<float>(-1.12,2.0,0.9),Point<float>(0.0,2.0,0.9),
                    	      Point<float>(0.98,1.75,1.875),Point<float>(1.75,0.98,1.875),Point<float>(1.12,2.0,1.35),
                    	      Point<float>(2.0,1.12,1.35),Point<float>(1.12,2.0,0.9),Point<float>(2.0,1.12,0.9),
                    	      Point<float>(2.0,0.0,0.45),Point<float>(2.0,-1.12,0.45),Point<float>(1.12,-2.0,0.45),
                    	      Point<float>(0.0,-2.0,0.45),Point<float>(1.5,0.0,0.225),Point<float>(1.5,-0.84,0.225),

                    	      Point<float>(0.84,-1.5,0.225),Point<float>(0.0,-1.5,0.225),Point<float>(1.5,0.0,0.15),
                    	      Point<float>(1.5,-0.84,0.15),Point<float>(0.84,-1.5,0.15),Point<float>(0.0,-1.5,0.15),
                    	      Point<float>(-1.12,-2.0,0.45),Point<float>(-2.0,-1.12,0.45),Point<float>(-2.0,0.0,0.45),
                    	      Point<float>(-0.84,-1.5,0.225),Point<float>(-1.5,-0.84,0.225),Point<float>(-1.5,0.0,0.225),
                    	      Point<float>(-0.84,-1.5,0.15),Point<float>(-1.5,-0.84,0.15),Point<float>(-1.5,0.0,0.15),
                    	      Point<float>(-2.0,1.12,0.45),Point<float>(-1.12,2.0,0.45),Point<float>(0.0,2.0,0.45),
                    	      Point<float>(-1.5,0.84,0.225),Point<float>(-0.84,1.5,0.225),Point<float>(0.0,1.5,0.225),
                    	      Point<float>(-1.5,0.84,0.15),Point<float>(-0.84,1.5,0.15),Point<float>(0.0,1.5,0.15),
                    	      Point<float>(1.12,2.0,0.45),Point<float>(2.0,1.12,0.45),Point<float>(0.84,1.5,0.225),
                    	      Point<float>(1.5,0.84,0.225),Point<float>(0.84,1.5,0.15),Point<float>(1.5,0.84,0.15),

                    	      Point<float>(-1.6,0.0,2.025),Point<float>(-1.6,-0.3,2.025),Point<float>(-1.5,-0.3,2.25),
                    	      Point<float>(-1.5,0.0,2.25),Point<float>(-2.3,0.0,2.025),Point<float>(-2.3,-0.3,2.025),
                    	      Point<float>(-2.5,-0.3,2.25),Point<float>(-2.5,0.0,2.25),Point<float>(-2.7,0.0,2.025),
                    	      Point<float>(-2.7,-0.3,2.025),Point<float>(-3.0,-0.3,2.25),Point<float>(-3.0,0.0,2.25),
                    	      Point<float>(-2.7,0.0,1.8),Point<float>(-2.7,-0.3,1.8),Point<float>(-3.0,-0.3,1.8),
                    	      Point<float>(-3.0,0.0,1.8),Point<float>(-1.5,0.3,2.25),Point<float>(-1.6,0.3,2.025),
                    	      Point<float>(-2.5,0.3,2.25),Point<float>(-2.3,0.3,2.025),Point<float>(-3.0,0.3,2.25),
                    	      Point<float>(-2.7,0.3,2.025),Point<float>(-3.0,0.3,1.8),Point<float>(-2.7,0.3,1.8),
                    	      Point<float>(-2.7,0.0,1.575),Point<float>(-2.7,-0.3,1.575),Point<float>(-3.0,-0.3,1.35),
                    	      Point<float>(-3.0,0.0,1.35),Point<float>(-2.5,0.0,1.125),Point<float>(-2.5,-0.3,1.125),

                    	      Point<float>(-2.65,-0.3,0.9375),Point<float>(-2.65,0.0,0.9375),Point<float>(-2.0,-0.3,0.9),
                    	      Point<float>(-1.9,-0.3,0.6),Point<float>(-1.9,0.0,0.6),Point<float>(-3.0,0.3,1.35),
                    	      Point<float>(-2.7,0.3,1.575),Point<float>(-2.65,0.3,0.9375),Point<float>(-2.5,0.3,1.125),
                    	      Point<float>(-1.9,0.3,0.6),Point<float>(-2.0,0.3,0.9),Point<float>(1.7,0.0,1.425),
                    	      Point<float>(1.7,-0.66,1.425),Point<float>(1.7,-0.66,0.6),Point<float>(1.7,0.0,0.6),
                    	      Point<float>(2.6,0.0,1.425),Point<float>(2.6,-0.66,1.425),Point<float>(3.1,-0.66,0.825),
                    	      Point<float>(3.1,0.0,0.825),Point<float>(2.3,0.0,2.1),Point<float>(2.3,-0.25,2.1),
                    	      Point<float>(2.4,-0.25,2.025),Point<float>(2.4,0.0,2.025),Point<float>(2.7,0.0,2.4),
                    	      Point<float>(2.7,-0.25,2.4),Point<float>(3.3,-0.25,2.4),Point<float>(3.3,0.0,2.4),
                    	      Point<float>(1.7,0.66,0.6),Point<float>(1.7,0.66,1.425),Point<float>(3.1,0.66,0.825),

                    	      Point<float>(2.6,0.66,1.425),Point<float>(2.4,0.25,2.025),Point<float>(2.3,0.25,2.1),
                    	      Point<float>(3.3,0.25,2.4),Point<float>(2.7,0.25,2.4),Point<float>(2.8,0.0,2.475),
                    	      Point<float>(2.8,-0.25,2.475),Point<float>(3.525,-0.25,2.49375),Point<float>(3.525,0.0,2.49375),
                    	      Point<float>(2.9,0.0,2.475),Point<float>(2.9,-0.15,2.475),Point<float>(3.45,-0.15,2.5125),
                    	      Point<float>(3.45,0.0,2.5125),Point<float>(2.8,0.0,2.4),Point<float>(2.8,-0.15,2.4),
                    	      Point<float>(3.2,-0.15,2.4),Point<float>(3.2,0.0,2.4),Point<float>(3.525,0.25,2.49375),
                    	      Point<float>(2.8,0.25,2.475),Point<float>(3.45,0.15,2.5125),Point<float>(2.9,0.15,2.475),
                    	      Point<float>(3.2,0.15,2.4),Point<float>(2.8,0.15,2.4),Point<float>(0.0,0.0,3.15),
                    	      Point<float>(0.0,-0.002,3.15),Point<float>(0.002,0.0,3.15),Point<float>(0.8,0.0,3.15),
                    	      Point<float>(0.8,-0.45,3.15),Point<float>(0.45,-0.8,3.15),Point<float>(0.0,-0.8,3.15),

                    	      Point<float>(0.0,0.0,2.85),Point<float>(0.2,0.0,2.7),Point<float>(0.2,-0.112,2.7),
                    	      Point<float>(0.112,-0.2,2.7),Point<float>(0.0,-0.2,2.7),Point<float>(-0.002,0.0,3.15),
                    	      Point<float>(-0.45,-0.8,3.15),Point<float>(-0.8,-0.45,3.15),Point<float>(-0.8,0.0,3.15),
                    	      Point<float>(-0.112,-0.2,2.7),Point<float>(-0.2,-0.112,2.7),Point<float>(-0.2,0.0,2.7),
                    	      Point<float>(0.0,0.002,3.15),Point<float>(-0.8,0.45,3.15),Point<float>(-0.45,0.8,3.15),
                    	      Point<float>(0.0,0.8,3.15),Point<float>(-0.2,0.112,2.7),Point<float>(-0.112,0.2,2.7),
                    	      Point<float>(0.0,0.2,2.7),Point<float>(0.45,0.8,3.15),Point<float>(0.8,0.45,3.15),
                    	      Point<float>(0.112,0.2,2.7),Point<float>(0.2,0.112,2.7),Point<float>(0.4,0.0,2.55),
                    	      Point<float>(0.4,-0.224,2.55),Point<float>(0.224,-0.4,2.55),Point<float>(0.0,-0.4,2.55),
                    	      Point<float>(1.3,0.0,2.55),Point<float>(1.3,-0.728,2.55),Point<float>(0.728,-1.3,2.55),

                    	      Point<float>(0.0,-1.3,2.55),Point<float>(1.3,0.0,2.4),Point<float>(1.3,-0.728,2.4),
                    	      Point<float>(0.728,-1.3,2.4),Point<float>(0.0,-1.3,2.4),Point<float>(-0.224,-0.4,2.55),
                    	      Point<float>(-0.4,-0.224,2.55),Point<float>(-0.4,0.0,2.55),Point<float>(-0.728,-1.3,2.55),
                    	      Point<float>(-1.3,-0.728,2.55),Point<float>(-1.3,0.0,2.55),Point<float>(-0.728,-1.3,2.4),
                    	      Point<float>(-1.3,-0.728,2.4),Point<float>(-1.3,0.0,2.4),Point<float>(-0.4,0.224,2.55),
                    	      Point<float>(-0.224,0.4,2.55),Point<float>(0.0,0.4,2.55),Point<float>(-1.3,0.728,2.55),
                    	      Point<float>(-0.728,1.3,2.55),Point<float>(0.0,1.3,2.55),Point<float>(-1.3,0.728,2.4),
                    	      Point<float>(-0.728,1.3,2.4),Point<float>(0.0,1.3,2.4),Point<float>(0.224,0.4,2.55),
                    	      Point<float>(0.4,0.224,2.55),Point<float>(0.728,1.3,2.55),Point<float>(1.3,0.728,2.55),
                    	      Point<float>(0.728,1.3,2.4),Point<float>(1.3,0.728,2.4),Point<float>(0.0,0.0,0.0),

                    	      Point<float>(1.5,0.0,0.15),Point<float>(1.5,0.84,0.15),Point<float>(0.84,1.5,0.15),
                    	      Point<float>(0.0,1.5,0.15),Point<float>(1.5,0.0,0.075),Point<float>(1.5,0.84,0.075),
                    	      Point<float>(0.84,1.5,0.075),Point<float>(0.0,1.5,0.075),Point<float>(1.425,0.0,0.0),
                    	      Point<float>(1.425,0.798,0.0),Point<float>(0.798,1.425,0.0),Point<float>(0.0,1.425,0.0),
                    	      Point<float>(-0.84,1.5,0.15),Point<float>(-1.5,0.84,0.15),Point<float>(-1.5,0.0,0.15),
                    	      Point<float>(-0.84,1.5,0.075),Point<float>(-1.5,0.84,0.075),Point<float>(-1.5,0.0,0.075),
                    	      Point<float>(-0.798,1.425,0.0),Point<float>(-1.425,0.798,0.0),Point<float>(-1.425,0.0,0.0),
                    	      Point<float>(-1.5,-0.84,0.15),Point<float>(-0.84,-1.5,0.15),Point<float>(0.0,-1.5,0.15),
                    	      Point<float>(-1.5,-0.84,0.075),Point<float>(-0.84,-1.5,0.075),Point<float>(0.0,-1.5,0.075),
                    	      Point<float>(-1.425,-0.798,0.0),Point<float>(-0.798,-1.425,0.0),Point<float>(0.0,-1.425,0.0),

                    	      Point<float>(0.84,-1.5,0.15),Point<float>(1.5,-0.84,0.15),Point<float>(0.84,-1.5,0.075),
                    	      Point<float>(1.5,-0.84,0.075),Point<float>(0.798,-1.425,0.0),Point<float>(1.425,-0.798,0.0)};
	
	const int numpotpatches=32;
	
	int potpatches[][16]={{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
                      	{3,16,17,18,7,19,20,21,11,22,23,24,15,25,26,27},
                      	{18,28,29,30,21,31,32,33,24,34,35,36,27,37,38,39},
                      	{30,40,41,0,33,42,43,4,36,44,45,8,39,46,47,12},
                      	{12,13,14,15,48,49,50,51,52,53,54,55,56,57,58,59},
                      	{15,25,26,27,51,60,61,62,55,63,64,65,59,66,67,68},
                      	{27,37,38,39,62,69,70,71,65,72,73,74,68,75,76,77},
                      	{39,46,47,12,71,78,79,48,74,80,81,52,77,82,83,56},
                      	{56,57,58,59,84,85,86,87,88,89,90,91,92,93,94,95},
                      	{59,66,67,68,87,96,97,98,91,99,100,101,95,102,103,104},
                      	{68,75,76,77,98,105,106,107,101,108,109,110,104,111,112,113},
                      	{77,82,83,56,107,114,115,84,110,116,117,88,113,118,119,92},
                      	{120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135},
                      	{123,136,137,120,127,138,139,124,131,140,141,128,135,142,143,132},
                      	{132,133,134,135,144,145,146,147,148,149,150,151,68,152,153,154},
                      	{135,142,143,132,147,155,156,144,151,157,158,148,154,159,160,68},
                      	{161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176},
                      	{164,177,178,161,168,179,180,165,172,181,182,169,176,183,184,173},
                      	{173,174,175,176,185,186,187,188,189,190,191,192,193,194,195,196},
                      	{176,183,184,173,188,197,198,185,192,199,200,189,196,201,202,193},
                      	{203,203,203,203,206,207,208,209,210,210,210,210,211,212,213,214},
                      	{203,203,203,203,209,216,217,218,210,210,210,210,214,219,220,221},
                      	{203,203,203,203,218,223,224,225,210,210,210,210,221,226,227,228},
                      	{203,203,203,203,225,229,230,206,210,210,210,210,228,231,232,211},
                      	{211,212,213,214,233,234,235,236,237,238,239,240,241,242,243,244},
                      	{214,219,220,221,236,245,246,247,240,248,249,250,244,251,252,253},
                      	{221,226,227,228,247,254,255,256,250,257,258,259,253,260,261,262},
                      	{228,231,232,211,256,263,264,233,259,265,266,237,262,267,268,241},
                      	{269,269,269,269,278,279,280,281,274,275,276,277,270,271,272,273},
                      	{269,269,269,269,281,288,289,290,277,285,286,287,273,282,283,284},
                      	{269,269,269,269,290,297,298,299,287,294,295,296,284,291,292,293},
                      	{269,269,269,269,299,304,305,278,296,302,303,274,293,300,301,270}};
	}

template <class MeshPoint>
PolygonMesh<MeshPoint>* loadMeshfile(const char* meshfileName)
	{
	/* Open the mesh file: */
	FILE* meshfile=fopen(meshfileName,"r");
	if(meshfile==0) // Any problems?
		return 0;
	
	/* Read all points in the mesh file: */
	std::vector<Point<float> > points;
	char bracket;
	do
		fscanf(meshfile,"%c",&bracket);
	while(bracket!='[');
	while(true)
		{
		Point<float> p;
		int numFloatsRead=fscanf(meshfile,"%f,%f,%f",&p[0],&p[1],&p[2]);
		if(numFloatsRead!=3)
			break;
		points.push_back(p);
		}
	do
		fscanf(meshfile,"%c",&bracket);
	while(bracket!=']');
	
	/* Read all indices in the mesh file: */
	std::vector<int> indices;
	do
		fscanf(meshfile,"%c",&bracket);
	while(bracket!='[');
	while(true)
		{
		int index;
		char separator;
		int numIndicesRead=fscanf(meshfile,"%d%c",&index,&separator);
		if(numIndicesRead!=2)
			break;
		indices.push_back(index);
		}
	indices.push_back(-1);
	do
		fscanf(meshfile,"%c",&bracket);
	while(bracket!=']');
	
	/* Read all sharp edge indices in the mesh file: */
	std::vector<int> sharpEdges;
	do
		fscanf(meshfile,"%c",&bracket);
	while(bracket!='[');
	while(true)
		{
		int v1,v2,sharpness;
		char separator;
		int numIndicesRead=fscanf(meshfile,"%d%c%d%c%d%c",&v1,&separator,&v2,&separator,&sharpness,&separator);
		if(numIndicesRead!=6)
			break;
		sharpEdges.push_back(v1);
		sharpEdges.push_back(v2);
		sharpEdges.push_back(sharpness);
		}
	do
		fscanf(meshfile,"%c",&bracket);
	while(bracket!=']');
	fclose(meshfile);
	
	/* Create and return the resulting mesh: */
	return new PolygonMesh<MeshPoint>(points.size(),&points.front(),&indices.front(),sharpEdges.size()/3,&sharpEdges.front());
	}

template <class MeshPoint>
PolygonMesh<MeshPoint>* loadGtsMeshfile(const char* gtsMeshfileName)
	{
	/* Open the mesh file: */
	FILE* meshfile=fopen(gtsMeshfileName,"r");
	if(meshfile==0) // Any problems?
		return 0;
	
	/* Read the number of points, edges and triangles: */
	int numPoints,numEdges,numTriangles;
	char line[80];
	fgets(line,sizeof(line),meshfile);
	sscanf(line,"%d %d %d",&numPoints,&numEdges,&numTriangles);
	
	/* Read all points in the mesh file: */
	Point<float>* points=new Point<float>[numPoints];
	for(int i=0;i<numPoints;++i)
		{
		fgets(line,sizeof(line),meshfile);
		sscanf(line,"%f %f %f",&points[i][0],&points[i][1],&points[i][2]);
		}
	
	/* Read all edges in the mesh file: */
	int* edgeIndices=new int[numEdges*2];
	for(int i=0;i<numEdges;++i)
		{
		fgets(line,sizeof(line),meshfile);
		sscanf(line,"%d %d",&edgeIndices[i*2+0],&edgeIndices[i*2+1]);
		for(int j=0;j<2;++j)
			--edgeIndices[i*2+j];
		}
	
	/* Read all triangle indices in the mesh file: */
	int* indices=new int[numTriangles*4+1];
	for(int i=0;i<numTriangles;++i)
		{
		/* Read indices of edges composing triangle from file: */
		fgets(line,sizeof(line),meshfile);
		int edges[3];
		sscanf(line,"%d %d %d",&edges[0],&edges[1],&edges[2]);
		for(int j=0;j<3;++j)
			--edges[j];
		
		/* Construct correct sequence of point indices from edge indices: */
		int pi[6];
		for(int j=0;j<3;++j)
			for(int k=0;k<2;++k)
				pi[j*2+k]=edgeIndices[edges[j]*2+k];
		
		int* iPtr=&indices[i*4];
		if(pi[0]==pi[2]||pi[0]==pi[3])
			iPtr[0]=pi[0];
		else
			iPtr[0]=pi[1];
		if(pi[2]==pi[4]||pi[2]==pi[5])
			iPtr[1]=pi[2];
		else
			iPtr[1]=pi[3];
		if(pi[4]==pi[0]||pi[4]==pi[1])
			iPtr[2]=pi[4];
		else
			iPtr[2]=pi[5];
		iPtr[3]=-1;
		}
	indices[numTriangles*4+0]=-1;
	fclose(meshfile);
	
	/* Clean up and return the resulting mesh: */
	PolygonMesh<MeshPoint>* result=new PolygonMesh<MeshPoint>(numPoints,points,indices,0,0);
	delete[] points;
	delete[] edgeIndices;
	delete[] indices;
	return result;
	}

std::pair<PlyFileMode,Misc::File::Endianness> getPlyFileMode(const char* plyMeshfileName)
	{
	/* Open the mesh file in text mode: */
	Misc::File meshfile(plyMeshfileName,"rt");
	
	/* Process the PLY file header: */
	bool isPlyFile=false;
	PlyFileMode plyFileMode=PLY_WRONGFORMAT;
	Misc::File::Endianness plyFileEndianness=Misc::File::DontCare;
	int parsedElement=0;
	Element vertex("vertex");
	int numVertices=0;
	Element face("face");
	int numFaces=0;
	char line[256];
	do
		{
		/* Read next header line: */
		meshfile.gets(line,sizeof(line));
		
		/* Parse header line: */
		if(strcmp(line,"ply\n")==0)
			isPlyFile=true;
		else if(strcmp(line,"format ascii 1.0\n")==0)
			plyFileMode=PLY_ASCII;
		else if(strcmp(line,"format binary_little_endian 1.0\n")==0)
			{
			plyFileMode=PLY_BINARY;
			plyFileEndianness=Misc::File::LittleEndian;
			}
		else if(strcmp(line,"format binary_big_endian 1.0\n")==0)
			{
			plyFileMode=PLY_BINARY;
			plyFileEndianness=Misc::File::BigEndian;
			}
		else if(strncmp(line,"element vertex ",15)==0)
			{
			numVertices=atoi(line+15);
			parsedElement=1;
			}
		else if(strncmp(line,"element face ",13)==0)
			{
			numFaces=atoi(line+13);
			parsedElement=2;
			}
		else if(strncmp(line,"property ",9)==0)
			{
			if(parsedElement==1)
				vertex.addProperty(line+9);
			else if(parsedElement==2)
				face.addProperty(line+9);
			}
		}
	while(strcmp(line,"end_header\n")!=0);
	
	/* Return the file's type: */
	if(!isPlyFile||numVertices==0||numFaces==0)
		plyFileMode=PLY_WRONGFORMAT;
	return std::pair<PlyFileMode,Misc::File::Endianness>(plyFileMode,plyFileEndianness);
	}

template <class MeshPoint>
PolygonMesh<MeshPoint>* loadPlyMeshfile(const char* plyMeshfileName)
	{
	/* Check the mesh file's type: */
	std::pair<PlyFileMode,Misc::File::Endianness>	plyFileMode=getPlyFileMode(plyMeshfileName);
	if(plyFileMode.first==PLY_WRONGFORMAT)
		Misc::throwStdErr("Input file %s is not a valid PLY file",plyMeshfileName);
	
	/* Open the mesh file: */
	Misc::File meshfile(plyMeshfileName,"rb",plyFileMode.second);
	
	/* Process the PLY file header: */
	int parsedElement=0;
	Element vertex("vertex");
	int numVertices=0;
	Element face("face");
	int numFaces=0;
	char line[160];
	do
		{
		/* Read next header line: */
		meshfile.gets(line,sizeof(line));
		
		/* Parse header line: */
		if(strncmp(line,"element vertex ",15)==0)
			{
			numVertices=atoi(line+15);
			parsedElement=1;
			}
		else if(strncmp(line,"element face ",13)==0)
			{
			numFaces=atoi(line+13);
			parsedElement=2;
			}
		else if(strncmp(line,"property ",9)==0)
			{
			if(parsedElement==1)
				vertex.addProperty(line+9);
			else if(parsedElement==2)
				face.addProperty(line+9);
			}
		}
	while(strcmp(line,"end_header\n")!=0);
	
	/* Read all vertices in the mesh file: */
	Point<float>* vertices=new Point<float>[numVertices];
	Element::Value vertexValue(&vertex);
	unsigned int xIndex=vertex.getPropertyIndex("x");
	unsigned int yIndex=vertex.getPropertyIndex("y");
	unsigned int zIndex=vertex.getPropertyIndex("z");
	for(int i=0;i<numVertices;++i)
		{
		/* Read vertex element from file: */
		vertexValue.read(meshfile,plyFileMode.first);
		
		/* Extract vertex coordinates from vertex element: */
		vertices[i][0]=float(vertexValue.getValue(xIndex).getScalar()->getDouble());
		vertices[i][1]=float(vertexValue.getValue(yIndex).getScalar()->getDouble());
		vertices[i][2]=float(vertexValue.getValue(zIndex).getScalar()->getDouble());
		}
	
	/* Read all face vertex indices in the mesh file: */
	std::vector<int> indices;
	indices.reserve(numFaces*4+1); // Assume that all faces are triangles
	Element::Value faceValue(&face);
	unsigned int vertexIndicesIndex=face.getPropertyIndex("vertex_indices");
	for(int i=0;i<numFaces;++i)
		{
		/* Read face element from file: */
		faceValue.read(meshfile,plyFileMode.first);
		
		/* Extract vertex indices from face element: */
		unsigned int numVertices=faceValue.getValue(vertexIndicesIndex).getListSize()->getUnsignedInt();
		for(unsigned int j=0;j<numVertices;++j)
			indices.push_back(faceValue.getValue(vertexIndicesIndex).getListElement(j)->getInt());
		indices.push_back(-1);
		}
	indices.push_back(-1);
	
	/* Clean up and return the resulting mesh: */
	PolygonMesh<MeshPoint>* result=new PolygonMesh<MeshPoint>(numVertices,vertices,&indices.front(),0,0);
	delete[] vertices;
	return result;
	}

template <class MeshPoint>
void saveMeshfile(const char* meshfileName,const PolygonMesh<MeshPoint>& mesh)
	{
	/* Open the mesh file: */
	FILE* meshfile=fopen(meshfileName,"w");
	if(meshfile==0) // Any problems?
		return;
	
	/* Create a hash table to associate vertices and vertex indices: */
	typedef Misc::HashTable<typename PolygonMesh<MeshPoint>::ConstVertexIterator,int,typename PolygonMesh<MeshPoint>::ConstVertexIterator> VertexIndexMap;
	VertexIndexMap vertexIndices((mesh.getNumVertices()*3)/2);
	
	/* Write and associate all vertices: */
	fprintf(meshfile,"[\n");
	int index=0;
	for(typename PolygonMesh<MeshPoint>::ConstVertexIterator vIt=mesh.beginVertices();vIt!=mesh.endVertices();++vIt,++index)
		{
		fprintf(meshfile,"%10.4lf, %10.4lf, %10.4lf\n",double((*vIt)[0]),double((*vIt)[1]),double((*vIt)[2]));
		vertexIndices.setEntry(typename VertexIndexMap::Entry(vIt,index));
		}
	fprintf(meshfile,"]\n\n");
	
	/* Write all faces: */
	fprintf(meshfile,"[\n");
	for(typename PolygonMesh<MeshPoint>::ConstFaceIterator fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		{
		/* Write all vertices of this face: */
		for(typename PolygonMesh<MeshPoint>::ConstFaceEdgeIterator feIt=fIt.beginEdges();feIt!=fIt.endEdges();++feIt)
			fprintf(meshfile,"%d, ",vertexIndices.getEntry(feIt->getStart()).getDest());
		fprintf(meshfile,"-1\n");
		}
	fprintf(meshfile,"]\n\n");
	
	/* Write all sharp edges: */
	fprintf(meshfile,"[\n");
	for(typename PolygonMesh<MeshPoint>::ConstFaceIterator fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		for(typename PolygonMesh<MeshPoint>::ConstFaceEdgeIterator feIt=fIt.beginEdges();feIt!=fIt.endEdges();++feIt)
			if(feIt->sharpness!=0&&feIt.isUpperHalf())
				fprintf(meshfile,"%d, %d, %d\n",vertexIndices.getEntry(feIt->getStart()).getDest(),vertexIndices.getEntry(feIt->getEnd()).getDest(),feIt->sharpness);
	fprintf(meshfile,"]\n");
	
	/* Close up and shut down: */
	fclose(meshfile);
	}
