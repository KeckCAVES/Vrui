/*****************************
* MeshGenerators - Functions *
* to create meshes for       *
* several basic polyhedra    *
* (c)2001 Oliver Kreylos     *
*****************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>
#include <vector>
#include <Misc/ThrowStdErr.h>
#include <Misc/HashTable.h>
#include <Math/Math.h>
#include <Geometry/Point.h>

#include "PlyFileStructures.h"

#include "MeshGenerators.h"

PolygonMesh* loadMeshfile(const char* meshfileName)
	{
	typedef PolygonMesh::Point Point;
	typedef PolygonMesh::VertexIterator VIt;
	
	/* Open the mesh file: */
	Misc::File meshfile(meshfileName,"rt");
	
	PolygonMesh* mesh=new PolygonMesh;
	PolygonMesh::Color vertexColor(255,255,255);
	
	/* Read all points in the mesh file: */
	std::vector<VIt> vertices;
	char bracket;
	do
		fscanf(meshfile.getFilePtr(),"%c",&bracket);
	while(bracket!='[');
	while(true)
		{
		float p[3];
		int numFloatsRead=fscanf(meshfile.getFilePtr(),"%f,%f,%f",&p[0],&p[1],&p[2]);
		if(numFloatsRead!=3)
			break;
		vertices.push_back(mesh->addVertex(Point(p),vertexColor));
		}
	do
		fscanf(meshfile.getFilePtr(),"%c",&bracket);
	while(bracket!=']');
	
	PolygonMesh::EdgeHasher* edgeHasher=mesh->startAddingFaces();
	
	/* Read all faces in the mesh file: */
	do
		fscanf(meshfile.getFilePtr(),"%c",&bracket);
	while(bracket!='[');
	std::vector<VIt> faceVertices;
	while(true)
		{
		int index;
		char separator;
		int numIndicesRead=fscanf(meshfile.getFilePtr(),"%d%c",&index,&separator);
		if(numIndicesRead!=2)
			break;
		if(index>=0)
			faceVertices.push_back(vertices[index]);
		else
			{
			mesh->addFace(faceVertices,edgeHasher);
			faceVertices.clear();
			}
		}
	do
		fscanf(meshfile.getFilePtr(),"%c",&bracket);
	while(bracket!=']');
	
	/* Read all sharp edge indices in the mesh file: */
	do
		fscanf(meshfile.getFilePtr(),"%c",&bracket);
	while(bracket!='[');
	while(true)
		{
		int v1,v2,sharpness;
		char separator;
		int numIndicesRead=fscanf(meshfile.getFilePtr(),"%d%c%d%c%d%c",&v1,&separator,&v2,&separator,&sharpness,&separator);
		if(numIndicesRead!=6)
			break;
		mesh->setEdgeSharpness(vertices[v1],vertices[v2],sharpness,edgeHasher);
		}
	do
		fscanf(meshfile.getFilePtr(),"%c",&bracket);
	while(bracket!=']');
	
	/* Finalize and return the resulting mesh: */
	mesh->finishAddingFaces(edgeHasher);
	return mesh;
	}

PolygonMesh* loadObjMeshfile(const char* objMeshfileName)
	{
	/* Open the mesh file: */
	Misc::File meshfile(objMeshfileName,"rt");
	
	PolygonMesh* mesh=new PolygonMesh;
	PolygonMesh::Color vertexColor(255,255,255);
	std::vector<PolygonMesh::VertexIterator> vertices;
	PolygonMesh::EdgeHasher* edgeHasher=mesh->startAddingFaces();
	
	/* Read all lines in the mesh file: */
	char line[256];
	while(!meshfile.eof())
		{
		/* Read the next line: */
		meshfile.gets(line,sizeof(line));
		
		/* Check if it's a vertex or face line: */
		if(tolower(line[0])=='v'&&line[1]==' ')
			{
			/* Read a vertex line: */
			float p[3];
			if(sscanf(line+2,"%f %f %f",&p[0],&p[1],&p[2])==3)
				vertices.push_back(mesh->addVertex(PolygonMesh::Point(p),vertexColor));
			}
		else if(tolower(line[0])=='f'&&line[1]==' ')
			{
			/* Read a face line: */
			int vi[3];
			if(sscanf(line+2,"%d %d %d",&vi[0],&vi[1],&vi[2])==3)
				{
				PolygonMesh::VertexIterator faceVertices[3];
				for(int i=0;i<3;++i)
					faceVertices[i]=vertices[vi[i]];
				mesh->addFace(3,faceVertices,edgeHasher);
				}
			}
		}
	
	/* Finalize and return the resulting mesh: */
	mesh->finishAddingFaces(edgeHasher);
	return mesh;
	}

PolygonMesh* loadGtsMeshfile(const char* gtsMeshfileName)
	{
	/* Open the mesh file: */
	Misc::File meshfile(gtsMeshfileName,"rt");
	
	PolygonMesh* mesh=new PolygonMesh;
	PolygonMesh::Color vertexColor(255,255,255);
	
	/* Read the number of points, edges and triangles: */
	int numPoints,numEdges,numTriangles;
	char line[80];
	meshfile.gets(line,sizeof(line));
	sscanf(line,"%d %d %d",&numPoints,&numEdges,&numTriangles);
	
	/* Read all points in the mesh file: */
	PolygonMesh::VertexIterator* vertices=new PolygonMesh::VertexIterator[numPoints];
	for(int i=0;i<numPoints;++i)
		{
		meshfile.gets(line,sizeof(line));
		float p[3];
		sscanf(line,"%f %f %f",&p[0],&p[1],&p[2]);
		vertices[i]=mesh->addVertex(PolygonMesh::Point(p),vertexColor);
		}
	
	PolygonMesh::EdgeHasher* edgeHasher=mesh->startAddingFaces();
	
	/* Read all edges in the mesh file: */
	int* edgeIndices=new int[numEdges*2];
	for(int i=0;i<numEdges;++i)
		{
		meshfile.gets(line,sizeof(line));
		sscanf(line,"%d %d",&edgeIndices[i*2+0],&edgeIndices[i*2+1]);
		for(int j=0;j<2;++j)
			--edgeIndices[i*2+j];
		}
	
	/* Read all triangles in the mesh file: */
	for(int i=0;i<numTriangles;++i)
		{
		/* Read indices of edges composing triangle from file: */
		meshfile.gets(line,sizeof(line));
		int edges[3];
		sscanf(line,"%d %d %d",&edges[0],&edges[1],&edges[2]);
		for(int j=0;j<3;++j)
			--edges[j];
		
		/* Construct correct sequence of point indices from edge indices: */
		int pi[6];
		for(int j=0;j<3;++j)
			for(int k=0;k<2;++k)
				pi[j*2+k]=edgeIndices[edges[j]*2+k];
		
		PolygonMesh::VertexIterator faceVertices[3];
		if(pi[0]==pi[2]||pi[0]==pi[3])
			faceVertices[0]=vertices[pi[0]];
		else
			faceVertices[0]=vertices[pi[1]];
		if(pi[2]==pi[4]||pi[2]==pi[5])
			faceVertices[1]=vertices[pi[2]];
		else
			faceVertices[1]=vertices[pi[3]];
		if(pi[4]==pi[0]||pi[4]==pi[1])
			faceVertices[2]=vertices[pi[4]];
		else
			faceVertices[2]=vertices[pi[5]];
		mesh->addFace(3,faceVertices,edgeHasher);
		}
	
	/* Finalize and return the resulting mesh: */
	mesh->finishAddingFaces(edgeHasher);
	delete[] vertices;
	delete[] edgeIndices;
	return mesh;
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
		
		/* Strip carriage return/newline: */
		char* lPtr;
		for(lPtr=line;*lPtr!='\0'&&*lPtr!='\r'&&*lPtr!='\n';++lPtr)
			;
		*lPtr='\0';
		
		/* Parse header line: */
		if(strcmp(line,"ply")==0)
			isPlyFile=true;
		else if(strcmp(line,"format ascii 1.0")==0)
			plyFileMode=PLY_ASCII;
		else if(strcmp(line,"format binary_little_endian 1.0")==0)
			{
			plyFileMode=PLY_BINARY;
			plyFileEndianness=Misc::File::LittleEndian;
			}
		else if(strcmp(line,"format binary_big_endian 1.0")==0)
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
	while(strcmp(line,"end_header")!=0);
	
	/* Return the file's type: */
	if(!isPlyFile||numVertices==0)
		plyFileMode=PLY_WRONGFORMAT;
	return std::pair<PlyFileMode,Misc::File::Endianness>(plyFileMode,plyFileEndianness);
	}

PolygonMesh* loadPlyMeshfile(const char* plyMeshfileName)
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
		
		/* Strip carriage return/newline: */
		char* lPtr;
		for(lPtr=line;*lPtr!='\0'&&*lPtr!='\r'&&*lPtr!='\n';++lPtr)
			;
		*lPtr='\0';
		
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
	while(strcmp(line,"end_header")!=0);
	
	/* Get indices of vertex value components: */
	unsigned int xIndex=vertex.getPropertyIndex("x");
	unsigned int yIndex=vertex.getPropertyIndex("y");
	unsigned int zIndex=vertex.getPropertyIndex("z");
	unsigned int rIndex=vertex.getPropertyIndex("red");
	unsigned int gIndex=vertex.getPropertyIndex("green");
	unsigned int bIndex=vertex.getPropertyIndex("blue");
	bool hasColor=rIndex<vertex.getNumProperties()&&gIndex<vertex.getNumProperties()&&bIndex<vertex.getNumProperties();
	
	PolygonMesh* mesh=new PolygonMesh;
	PolygonMesh::Color vertexColor(255,255,255);
	
	/* Read all vertices in the mesh file: */
	PolygonMesh::VertexIterator* vertices=new PolygonMesh::VertexIterator[numVertices];
	Element::Value vertexValue(&vertex);
	for(int i=0;i<numVertices;++i)
		{
		/* Read vertex element from file: */
		vertexValue.read(meshfile,plyFileMode.first);
		
		/* Extract vertex coordinates from vertex element: */
		PolygonMesh::Point point;
		point[0]=PolygonMesh::Scalar(vertexValue.getValue(xIndex).getScalar()->getDouble());
		point[1]=PolygonMesh::Scalar(vertexValue.getValue(yIndex).getScalar()->getDouble());
		point[2]=PolygonMesh::Scalar(vertexValue.getValue(zIndex).getScalar()->getDouble());
		if(hasColor)
			{
			PolygonMesh::Color color;
			color[0]=GLubyte(vertexValue.getValue(rIndex).getScalar()->getDouble());
			color[1]=GLubyte(vertexValue.getValue(gIndex).getScalar()->getDouble());
			color[2]=GLubyte(vertexValue.getValue(bIndex).getScalar()->getDouble());
			color[3]=GLubyte(255);
			vertices[i]=mesh->addVertex(point,color);
			}
		else
			vertices[i]=mesh->addVertex(point,vertexColor);
		}
	
	PolygonMesh::EdgeHasher* edgeHasher=mesh->startAddingFaces();
	
	/* Read all face vertex indices in the mesh file: */
	Element::Value faceValue(&face);
	unsigned int vertexIndicesIndex=face.getPropertyIndex("vertex_indices");
	for(int i=0;i<numFaces;++i)
		{
		/* Read face element from file: */
		faceValue.read(meshfile,plyFileMode.first);
		
		/* Extract vertex indices from face element: */
		unsigned int numFaceVertices=faceValue.getValue(vertexIndicesIndex).getListSize()->getUnsignedInt();
		std::vector<PolygonMesh::VertexIterator> faceVertices;
		faceVertices.reserve(numFaceVertices);
		for(unsigned int j=0;j<numFaceVertices;++j)
			faceVertices.push_back(vertices[faceValue.getValue(vertexIndicesIndex).getListElement(j)->getInt()]);
		mesh->addFace(faceVertices,edgeHasher);
		}
	
	/* Finalize and return the resulting mesh: */
	mesh->finishAddingFaces(edgeHasher);
	delete[] vertices;
	return mesh;
	}

PolygonMesh* loadTsurfMeshfile(const char* tsurfMeshfileName)
	{
	/* Open the TSurf mesh file: */
	FILE* meshfile=fopen(tsurfMeshfileName,"r");
	if(meshfile==0) // Any problems?
		return 0;
	
	/* Process the TSurf header: */
	char line[256];
	fgets(line,sizeof(line),meshfile);
	if(strcmp(line,"GOCAD TSurf 0.01\n")!=0)
		{
		fclose(meshfile);
		return 0;
		}
	
	/* Create the result mesh: */
	PolygonMesh* mesh=new PolygonMesh;
	PolygonMesh::Color vertexColor(255,255,255);
	
	/* Create a hash table to associate vertices and vertex indices: */
	typedef Misc::HashTable<unsigned int,PolygonMesh::VertexIterator> VertexMap;
	VertexMap vertices(101);
	
	/* Prepare to add faces at any time: */
	PolygonMesh::EdgeHasher* eh=mesh->startAddingFaces();
	
	/* Ignore any lines that do not start with VRTX or TRGL: */
	while(!feof(meshfile))
		{
		fgets(line,sizeof(line),meshfile);
		if(isspace(line[4])&&strncasecmp(line,"VRTX",4)==0)
			{
			/* Process a vertex: */
			unsigned int index;
			double pos[3];
			sscanf(line+5,"%u %lf %lf %lf",&index,&pos[0],&pos[1],&pos[2]);
			PolygonMesh::Point point(pos[0],pos[1],pos[2]*0.01);
			PolygonMesh::VertexIterator vertex=mesh->addVertex(point,vertexColor);
			vertices.setEntry(VertexMap::Entry(index,vertex));
			}
		else if(isspace(line[4])&&strncasecmp(line,"TRGL",4)==0)
			{
			/* Process a triangle: */
			unsigned int indices[3];
			sscanf(line+5,"%u %u %u",&indices[0],&indices[1],&indices[2]);
			PolygonMesh::VertexIterator face[3];
			for(int i=0;i<3;++i)
				face[i]=vertices.getEntry(indices[i]).getDest();
			mesh->addFace(3,face,eh);
			}
		}
	fclose(meshfile);
	
	/* Clean up and return the result mesh: */
	delete eh;
	return mesh;
	}

void saveMeshfile(const char* meshfileName,const PolygonMesh& mesh)
	{
	/* Open the mesh file: */
	FILE* meshfile=fopen(meshfileName,"w");
	if(meshfile==0) // Any problems?
		return;
	
	/* Create a hash table to associate vertices and vertex indices: */
	typedef Misc::HashTable<PolygonMesh::ConstVertexIterator,int,PolygonMesh::ConstVertexIterator> VertexIndexMap;
	VertexIndexMap vertexIndices((mesh.getNumVertices()*3)/2);
	
	/* Write and associate all vertices: */
	fprintf(meshfile,"[\n");
	int index=0;
	for(PolygonMesh::ConstVertexIterator vIt=mesh.beginVertices();vIt!=mesh.endVertices();++vIt,++index)
		{
		fprintf(meshfile,"%10.4lf, %10.4lf, %10.4lf\n",double((*vIt)[0]),double((*vIt)[1]),double((*vIt)[2]));
		vertexIndices.setEntry(VertexIndexMap::Entry(vIt,index));
		}
	fprintf(meshfile,"]\n\n");
	
	/* Write all faces: */
	fprintf(meshfile,"[\n");
	for(PolygonMesh::ConstFaceIterator fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		{
		/* Write all vertices of this face: */
		for(PolygonMesh::ConstFaceEdgeIterator feIt=fIt.beginEdges();feIt!=fIt.endEdges();++feIt)
			fprintf(meshfile,"%d, ",vertexIndices.getEntry(feIt->getStart()).getDest());
		fprintf(meshfile,"-1\n");
		}
	fprintf(meshfile,"]\n\n");
	
	/* Write all sharp edges: */
	fprintf(meshfile,"[\n");
	for(PolygonMesh::ConstFaceIterator fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		for(PolygonMesh::ConstFaceEdgeIterator feIt=fIt.beginEdges();feIt!=fIt.endEdges();++feIt)
			if(feIt->sharpness!=0&&feIt.isUpperHalf())
				fprintf(meshfile,"%d, %d, %d\n",vertexIndices.getEntry(feIt->getStart()).getDest(),vertexIndices.getEntry(feIt->getEnd()).getDest(),feIt->sharpness);
	fprintf(meshfile,"]\n");
	
	/* Close up and shut down: */
	fclose(meshfile);
	}

void savePlyMeshfile(const char* meshfileName,const PolygonMesh& mesh)
	{
	typedef Misc::HashTable<const PolygonMesh::Vertex*,int> VertexHasher;
	
	/* Open the ply file: */
	Misc::File plyFile(meshfileName,"wb",Misc::File::LittleEndian);
	
	/* Write the ply file header: */
	fprintf(plyFile.getFilePtr(),"ply\n");
	fprintf(plyFile.getFilePtr(),"format binary_little_endian 1.0\n");
	fprintf(plyFile.getFilePtr(),"comment by Liquid Metal Editor\n");
	int numVertices=mesh.getNumVertices();
	fprintf(plyFile.getFilePtr(),"element vertex %d\n",numVertices);
	fprintf(plyFile.getFilePtr(),"property float x\n");
	fprintf(plyFile.getFilePtr(),"property float y\n");
	fprintf(plyFile.getFilePtr(),"property float z\n");
	fprintf(plyFile.getFilePtr(),"property uchar red\n");
	fprintf(plyFile.getFilePtr(),"property uchar green\n");
	fprintf(plyFile.getFilePtr(),"property uchar blue\n");
	int numFaces=mesh.getNumFaces();
	fprintf(plyFile.getFilePtr(),"element face %d\n",numFaces);
	fprintf(plyFile.getFilePtr(),"property list uchar int vertex_indices\n");
	fprintf(plyFile.getFilePtr(),"end_header\n");
	
	/* Write all vertices to the ply file: */
	VertexHasher vertexHasher((numVertices*3)/2);
	int i=0;
	for(PolygonMesh::ConstVertexIterator vIt=mesh.beginVertices();vIt!=mesh.endVertices();++vIt,++i)
		{
		/* Write vertex position: */
		plyFile.write<float>(vIt->getComponents(),3);
		plyFile.write<unsigned char>(vIt->color.getRgba(),3);
		
		/* Put vertex into hash table for index generation: */
		vertexHasher.setEntry(VertexHasher::Entry(&(*vIt),i));
		}
	
	/* Write all faces to the ply file: */
	for(PolygonMesh::ConstFaceIterator fIt=mesh.beginFaces();fIt!=mesh.endFaces();++fIt)
		{
		/* Collect the face's vertex indices: */
		std::vector<int> vertexIndices;
		for(PolygonMesh::ConstFaceEdgeIterator feIt=fIt.beginEdges();feIt!=fIt.endEdges();++feIt)
			vertexIndices.push_back(vertexHasher.getEntry(feIt->getStart()).getDest());
		
		/* Write the face vertex indices to the ply file: */
		plyFile.write<unsigned char>(vertexIndices.size());
		for(std::vector<int>::const_iterator viIt=vertexIndices.begin();viIt!=vertexIndices.end();++viIt)
			plyFile.write<int>(*viIt);
		}
	}
