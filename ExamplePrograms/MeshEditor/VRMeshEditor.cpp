/***********************************************************************
VRMeshEditor - Vrui application to interact with self-managing triangle
meshes.
Copyright (c) 2003-2006 Oliver Kreylos

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

#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <Math/Math.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLVertex.h>
#include <GL/GLLight.h>
#include <GL/GLMaterial.h>
#include <GL/GLObject.h>
#include <GL/GLContextData.h>
#include <GL/GLModels.h>
#include <GLMotif/Popup.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/RadioBox.h>
#include <GLMotif/Menu.h>
#include <GLMotif/Button.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/CascadeButton.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/LightsourceManager.h>
#include <Vrui/Viewer.h>
#include <Vrui/Tools/DraggingTool.h>
#include <Vrui/DraggingToolAdapter.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Application.h>

#include "Point.h"
#include "PolygonMesh.h"
#include "AutoTriangleMesh.h"
#include "MeshGenerators.h"
#include "CatmullClark.h"
#include "RenderPolygonMeshGL.h"

#include "Influence.h"
#include "MorphBox.h"

struct VRMeshEditor:public Vrui::Application,public GLObject
	{
	/* Embedded classes: */
	public:
	typedef Point<float> MyPoint;
	typedef AutoTriangleMesh<MyPoint> MyMesh;
	typedef MyMesh::VertexIterator MyVIt;
	typedef GLVertex<void,0,void,0,GLfloat,GLfloat,3> MyVertex;
	typedef MorphBox<MyMesh> MyMorphBox;
	
	enum DraggerType // Enumerated type for dragger types
		{
		MESHDRAGGER,MORPHBOXDRAGGER
		};
	
	enum RenderMode // Enumerated type for rendering modes
		{
		SHADED,WIREFRAME
		};
	
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		unsigned int numVertices; // Number of vertices that can be stored in vertex array
		MyVertex* vertices; // Pointer to vertex array
		unsigned int numTriangles; // Number of triangles that can be stored in triangle array
		unsigned int* triangles; // Pointer to triangle index array
		
		/* Constructors and destructors: */
		DataItem(void)
			:numVertices(0),vertices(0),numTriangles(0),triangles(0)
			{
			};
		virtual ~DataItem(void)
			{
			delete[] vertices;
			delete[] triangles;
			};
		};
	
	class Dragger:public Vrui::DraggingToolAdapter // Base class for application draggers
		{
		/* Elements: */
		protected:
		VRMeshEditor* application; // Pointer to the application object
		
		/* Constructors and destructors: */
		Dragger(Vrui::DraggingTool* sTool,VRMeshEditor* sApplication)
			:DraggingToolAdapter(sTool),
			 application(sApplication)
			{
			};
		public:
		virtual ~Dragger(void)
			{
			};
		
		/* Methods: */
		virtual void glRenderAction(GLContextData& contextData) const
			{
			};
		};
	
	typedef std::vector<Dragger*> DraggerList;
	
	class MeshDragger:public Dragger,public Influence // Class to drag meshes with a dragging tool
		{
		/* Elements: */
		private:
		double influenceRadius; // Radius of influence sphere in physical coordinates
		bool active; // Flag whether the influence is active
		
		/* Constructors and destructors: */
		public:
		MeshDragger(Vrui::DraggingTool* sTool,VRMeshEditor* sApplication);
		
		/* Methods: */
		virtual void idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData* cbData);
		virtual void dragStartCallback(Vrui::DraggingTool::DragStartCallbackData* cbData);
		virtual void dragCallback(Vrui::DraggingTool::DragCallbackData* cbData);
		virtual void dragEndCallback(Vrui::DraggingTool::DragEndCallbackData* cbData);
		virtual void glRenderAction(GLContextData& contextData) const;
		};
	
	class MorphBoxDragger:public Dragger // Class to drag morph boxes (and morph meshes)
		{
		/* Elements: */
		private:
		bool creatingMorphBox; // Flag if the dragger is currently creating a morph box
		Vrui::Point p1,p2; // Diagonally opposite box corners during box creation
		bool draggingMorphBox; // Flag if the dragger is currently dragging a morph box
		
		/* Constructors and destructors: */
		public:
		MorphBoxDragger(Vrui::DraggingTool* sTool,VRMeshEditor* sApplication);
		
		/* Methods: */
		virtual void dragStartCallback(Vrui::DraggingTool::DragStartCallbackData* cbData);
		virtual void dragCallback(Vrui::DraggingTool::DragCallbackData* cbData);
		virtual void dragEndCallback(Vrui::DraggingTool::DragEndCallbackData* cbData);
		virtual void glRenderAction(GLContextData& contextData) const;
		};
	
	friend class MeshDragger;
	friend class MorphBoxDragger;
	
	/* Elements: */
	
	/* Mesh state: */
	MyMesh* mesh; // The mesh being edited
	MyMorphBox* morphBox; // Pointer to currently active morph box
	
	/* Interaction state: */
	DraggerType defaultDraggerType; // Type of draggers to be created
	Influence::ActionType defaultActionType; // Default influence action type for new mesh draggers
	bool overrideTools; // Flag to override action types of existing tools when default action is changed
	DraggerList draggers; // List of currently instantiated draggers
	
	/* Rendering state: */
	RenderMode renderMode; // Current rendering mode for surface
	GLMaterial meshMaterial; // Material for rendering the mesh
	
	/* Vrui state: */
	GLMotif::PopupMenu* mainMenu; // The main menu widget
	
	/* Private methods: */
	GLMotif::Popup* createDraggerTypesMenu(void); // Creates program's dragger type menu
	GLMotif::Popup* createInfluenceActionsMenu(void); // Creates program's influence actions menu
	GLMotif::Popup* createSettingsMenu(void); // Creates program's settings menu
	GLMotif::PopupMenu* createMainMenu(void); // Creates program's main menu
	void renderMesh(DataItem* dataItem) const; // Renders the mesh
	void setMeshDraggerActionType(Influence::ActionType newActionType); // Sets the editing modes of all mesh dragging tools
	
	/* Constructors and destructors: */
	VRMeshEditor(int& argc,char**& argv,char**& appDefaults);
	virtual ~VRMeshEditor(void);
	
	/* Methods: */
	virtual void toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData);
	virtual void toolDestructionCallback(Vrui::ToolManager::ToolDestructionCallbackData* cbData);
	virtual void initContext(GLContextData& contextData) const;
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	void radioBoxEntrySelectCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData);
	void overrideToolsValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void centerDisplayCallback(Misc::CallbackData* cbData);
	void createMorphBoxCallback(Misc::CallbackData* cbData);
	void deleteMorphBoxCallback(Misc::CallbackData* cbData);
	void createInputDeviceCallback(Misc::CallbackData* cbData);
	};

/******************************************
Methods of class VRMeshEditor::MeshDragger:
******************************************/

VRMeshEditor::MeshDragger::MeshDragger(Vrui::DraggingTool* sTool,VRMeshEditor* sApplication)
	:Dragger(sTool,sApplication),
	 Influence(0.0),
	 influenceRadius(Vrui::getGlyphRenderer()->getGlyphSize()*5.0),
	 active(0)
	{
	/* Set influence's action type: */
	Influence::setAction(application->defaultActionType);
	}

void VRMeshEditor::MeshDragger::idleMotionCallback(Vrui::DraggingTool::IdleMotionCallbackData* cbData)
	{
	/* Update influence object's state: */
	Influence::setPositionOrientation(Influence::ONTransform(cbData->currentTransformation.getTranslation(),cbData->currentTransformation.getRotation()));
	
	/* Set the influence object's radius: */
	Influence::setRadius(influenceRadius*cbData->currentTransformation.getScaling());
	}

void VRMeshEditor::MeshDragger::dragStartCallback(Vrui::DraggingTool::DragStartCallbackData* cbData)
	{
	/* Start performing influence's action: */
	active=true;
	}

void VRMeshEditor::MeshDragger::dragCallback(Vrui::DraggingTool::DragCallbackData* cbData)
	{
	/* Update influence object's state: */
	Influence::setPositionOrientation(Influence::ONTransform(cbData->currentTransformation.getTranslation(),cbData->currentTransformation.getRotation()));
	
	/* Set the influence object's radius: */
	Influence::setRadius(influenceRadius*cbData->currentTransformation.getScaling());
	
	/* Perform influence's action: */
	if(active)
		Influence::actOnMesh(*application->mesh);
	}

void VRMeshEditor::MeshDragger::dragEndCallback(Vrui::DraggingTool::DragEndCallbackData* cbData)
	{
	/* Stop performing influence's action: */
	active=false;
	}

void VRMeshEditor::MeshDragger::glRenderAction(GLContextData& contextData) const
	{
	Influence::glRenderAction(contextData);
	}

/**********************************************
Methods of class VRMeshEditor::MorphBoxDragger:
**********************************************/

VRMeshEditor::MorphBoxDragger::MorphBoxDragger(Vrui::DraggingTool* sTool,VRMeshEditor* sApplication)
	:Dragger(sTool,sApplication),
	 creatingMorphBox(false),draggingMorphBox(false)
	{
	}

void VRMeshEditor::MorphBoxDragger::dragStartCallback(Vrui::DraggingTool::DragStartCallbackData* cbData)
	{
	if(application->morphBox!=0)
		{
		/* Pick the morph box: */
		MyMorphBox::Scalar pd=MyMorphBox::Scalar(Vrui::getInchFactor()*cbData->startTransformation.getScaling());
		if(application->morphBox->pickBox(pd*MyMorphBox::Scalar(0.75),pd*MyMorphBox::Scalar(0.5),pd*MyMorphBox::Scalar(0.333),cbData->startTransformation.getOrigin()))
			{
			draggingMorphBox=true;
			application->morphBox->startDragBox(cbData->startTransformation);
			}
		}
	else
		{
		/* Start creating a morph box: */
		creatingMorphBox=true;
		p1=p2=cbData->startTransformation.getOrigin();
		}
	}

void VRMeshEditor::MorphBoxDragger::dragCallback(Vrui::DraggingTool::DragCallbackData* cbData)
	{
	if(draggingMorphBox)
		application->morphBox->dragBox(cbData->currentTransformation);
	else if(creatingMorphBox)
		p2=cbData->currentTransformation.getOrigin();
	}

void VRMeshEditor::MorphBoxDragger::dragEndCallback(Vrui::DraggingTool::DragEndCallbackData* cbData)
	{
	if(draggingMorphBox)
		{
		application->morphBox->stopDragBox();
		draggingMorphBox=false;
		}
	else if(creatingMorphBox)
		{
		/* Create a morph box: */
		MyMorphBox::Point origin;
		MyMorphBox::Scalar size[3];
		for(int i=0;i<3;++i)
			{
			origin[i]=MyMorphBox::Scalar(p1[i]<=p2[i]?p1[i]:p2[i]);
			size[i]=Math::abs(MyMorphBox::Scalar(p2[i]-p1[i]));
			}
		application->morphBox=new MyMorphBox(application->mesh,origin,size);
		creatingMorphBox=false;
		}
	}

void VRMeshEditor::MorphBoxDragger::glRenderAction(GLContextData& contextData) const
	{
	if(creatingMorphBox)
		{
		/* Render the current morph box: */
		GLfloat min[3],max[3];
		for(int i=0;i<3;++i)
			{
			if(p1[i]<=p2[i])
				{
				min[i]=GLfloat(p1[i]);
				max[i]=GLfloat(p2[i]);
				}
			else
				{
				min[i]=GLfloat(p2[i]);
				max[i]=GLfloat(p1[i]);
				}
			}
		glDrawBox(min,max);
		}
	}

/*****************************
Methods of class VRMeshEditor:
*****************************/

GLMotif::Popup* VRMeshEditor::createDraggerTypesMenu(void)
	{
	GLMotif::Popup* draggerTypesMenuPopup=new GLMotif::Popup("DraggerTypesMenuPopup",Vrui::getWidgetManager());
	draggerTypesMenuPopup->setBorderWidth(Vrui::getUiSize()*0.5f);
	draggerTypesMenuPopup->setBorderType(GLMotif::Widget::PLAIN);
	draggerTypesMenuPopup->setBorderColor(Vrui::getUiBgColor());
	draggerTypesMenuPopup->setBackgroundColor(Vrui::getUiBgColor());
	draggerTypesMenuPopup->setForegroundColor(Vrui::getUiFgColor());
	draggerTypesMenuPopup->setMarginWidth(0.0f);
	draggerTypesMenuPopup->setTitleSpacing(Vrui::getUiSize()*0.5f);
	draggerTypesMenuPopup->setTitle("Dragger Types",Vrui::getUiFont());
	
	GLMotif::RadioBox* draggerTypes=new GLMotif::RadioBox("Dragger Types",draggerTypesMenuPopup,false);
	draggerTypes->setBorderWidth(0.0f);
	draggerTypes->setOrientation(GLMotif::RowColumn::VERTICAL);
	draggerTypes->setNumMinorWidgets(1);
	draggerTypes->setMarginWidth(0.0f);
	draggerTypes->setSpacing(Vrui::getUiSize()*0.5f);
	draggerTypes->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
	
	GLMotif::ToggleButton* meshDraggerToggle=new GLMotif::ToggleButton("MeshDraggerToggle",draggerTypes,"Mesh Dragger",Vrui::getUiFont());
	GLMotif::ToggleButton* morphBoxDraggerToggle=new GLMotif::ToggleButton("MorphBoxDraggerToggle",draggerTypes,"Morph Box Dragger",Vrui::getUiFont());
	
	draggerTypes->manageChild();
	switch(defaultDraggerType)
		{
		case MESHDRAGGER:
			draggerTypes->setSelectedToggle(meshDraggerToggle);
			break;
		
		case MORPHBOXDRAGGER:
			draggerTypes->setSelectedToggle(morphBoxDraggerToggle);
			break;
		}
	draggerTypes->getValueChangedCallbacks().add(this,&VRMeshEditor::radioBoxEntrySelectCallback);
	
	return draggerTypesMenuPopup;
	}

GLMotif::Popup* VRMeshEditor::createInfluenceActionsMenu(void)
	{
	GLMotif::Popup* influenceActionsMenuPopup=new GLMotif::Popup("InfluenceActionsMenuPopup",Vrui::getWidgetManager());
	influenceActionsMenuPopup->setBorderWidth(Vrui::getUiSize()*0.5f);
	influenceActionsMenuPopup->setBorderType(GLMotif::Widget::PLAIN);
	influenceActionsMenuPopup->setBorderColor(Vrui::getUiBgColor());
	influenceActionsMenuPopup->setBackgroundColor(Vrui::getUiBgColor());
	influenceActionsMenuPopup->setForegroundColor(Vrui::getUiFgColor());
	influenceActionsMenuPopup->setMarginWidth(0.0f);
	influenceActionsMenuPopup->setTitleSpacing(Vrui::getUiSize()*0.5f);
	influenceActionsMenuPopup->setTitle("Influence Actions",Vrui::getUiFont());
	
	GLMotif::RowColumn* influenceActionsMenu=new GLMotif::RowColumn("InfluenceActionsMenu",influenceActionsMenuPopup,false);
	influenceActionsMenu->setBorderWidth(0.0f);
	influenceActionsMenu->setOrientation(GLMotif::RowColumn::VERTICAL);
	influenceActionsMenu->setNumMinorWidgets(1);
	influenceActionsMenu->setMarginWidth(0.0f);
	influenceActionsMenu->setSpacing(Vrui::getUiSize()*0.5f);
	
	GLMotif::RadioBox* influenceActions=new GLMotif::RadioBox("InfluenceActions",influenceActionsMenu,false);
	influenceActions->setBorderWidth(0.0f);
	influenceActions->setOrientation(GLMotif::RowColumn::VERTICAL);
	influenceActions->setNumMinorWidgets(1);
	influenceActions->setMarginWidth(0.0f);
	influenceActions->setSpacing(Vrui::getUiSize()*0.5f);
	influenceActions->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
	
	GLMotif::ToggleButton* dragToggle=new GLMotif::ToggleButton("DragToggle",influenceActions,"Drag",Vrui::getUiFont());
	GLMotif::ToggleButton* explodeToggle=new GLMotif::ToggleButton("ExplodeToggle",influenceActions,"Explode",Vrui::getUiFont());
	GLMotif::ToggleButton* whittleToggle=new GLMotif::ToggleButton("WhittleToggle",influenceActions,"Whittle",Vrui::getUiFont());
	
	influenceActions->manageChild();
	switch(defaultActionType)
		{
		case Influence::DRAG:
			influenceActions->setSelectedToggle(dragToggle);
			break;
		
		case Influence::EXPLODE:
			influenceActions->setSelectedToggle(explodeToggle);
			break;
		
		case Influence::WHITTLE:
			influenceActions->setSelectedToggle(whittleToggle);
			break;
		}
	influenceActions->getValueChangedCallbacks().add(this,&VRMeshEditor::radioBoxEntrySelectCallback);
	
	GLMotif::ToggleButton* overrideToolsToggle=new GLMotif::ToggleButton("OverrideToolsToggle",influenceActionsMenu,"Override Tools",Vrui::getUiFont());
	overrideToolsToggle->setBorderWidth(0.0f);
	overrideToolsToggle->setToggleType(GLMotif::ToggleButton::TOGGLE_BUTTON);
	overrideToolsToggle->setToggle(overrideTools);
	overrideToolsToggle->getValueChangedCallbacks().add(this,&VRMeshEditor::overrideToolsValueChangedCallback);
	
	influenceActionsMenu->manageChild();
	
	return influenceActionsMenuPopup;
	}

GLMotif::Popup* VRMeshEditor::createSettingsMenu(void)
	{
	GLMotif::Popup* settingsMenuPopup=new GLMotif::Popup("SettingsMenuPopup",Vrui::getWidgetManager());
	settingsMenuPopup->setBorderWidth(Vrui::getUiSize()*0.5f);
	settingsMenuPopup->setBorderType(GLMotif::Widget::PLAIN);
	settingsMenuPopup->setBorderColor(Vrui::getUiBgColor());
	settingsMenuPopup->setBackgroundColor(Vrui::getUiBgColor());
	settingsMenuPopup->setForegroundColor(Vrui::getUiFgColor());
	settingsMenuPopup->setMarginWidth(0.0f);
	settingsMenuPopup->setTitleSpacing(Vrui::getUiSize()*0.5f);
	settingsMenuPopup->setTitle("Settings",Vrui::getUiFont());
	
	GLMotif::RadioBox* settingsMenu=new GLMotif::RadioBox("SettingsMenu",settingsMenuPopup,false);
	settingsMenu->setBorderWidth(0.0f);
	settingsMenu->setOrientation(GLMotif::RowColumn::VERTICAL);
	settingsMenu->setNumMinorWidgets(1);
	settingsMenu->setMarginWidth(0.0f);
	settingsMenu->setSpacing(Vrui::getUiSize()*0.5f);
	settingsMenu->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
	
	GLMotif::ToggleButton* drawWireframeToggle=new GLMotif::ToggleButton("DrawWireframeToggle",settingsMenu,"Draw Wireframe",Vrui::getUiFont());
	GLMotif::ToggleButton* drawShadedToggle=new GLMotif::ToggleButton("DrawShadedToggle",settingsMenu,"Draw Shaded Surface",Vrui::getUiFont());
	
	settingsMenu->manageChild();
	settingsMenu->setSelectedToggle(drawShadedToggle);
	settingsMenu->getValueChangedCallbacks().add(this,&VRMeshEditor::radioBoxEntrySelectCallback);
	
	return settingsMenuPopup;
	}

GLMotif::PopupMenu* VRMeshEditor::createMainMenu(void)
	{
	GLMotif::PopupMenu* mainMenuPopup=new GLMotif::PopupMenu("MainMenuPopup",Vrui::getWidgetManager());
	mainMenuPopup->setBorderWidth(0.0f);
	mainMenuPopup->setBorderType(GLMotif::Widget::RAISED);
	mainMenuPopup->setBorderColor(Vrui::getUiBgColor());
	mainMenuPopup->setBackgroundColor(Vrui::getUiBgColor());
	mainMenuPopup->setForegroundColor(Vrui::getUiFgColor());
	mainMenuPopup->setMarginWidth(Vrui::getUiSize());
	mainMenuPopup->setTitleSpacing(Vrui::getUiSize());
	mainMenuPopup->setTitle("Liquid Metal Editing",Vrui::getUiFont());
	
	GLMotif::Menu* mainMenu=new GLMotif::Menu("MainMenu",mainMenuPopup,false);
	mainMenu->setBorderWidth(0.0f);
	mainMenu->setOrientation(GLMotif::RowColumn::VERTICAL);
	mainMenu->setNumMinorWidgets(1);
	mainMenu->setMarginWidth(0.0f);
	mainMenu->setSpacing(Vrui::getUiSize());
	
	GLMotif::Button* centerDisplayButton=new GLMotif::Button("CenterDisplayButton",mainMenu,"Center Display",Vrui::getUiFont());
	centerDisplayButton->getSelectCallbacks().add(this,&VRMeshEditor::centerDisplayCallback);
	
	GLMotif::CascadeButton* draggerTypesCascade=new GLMotif::CascadeButton("DraggerTypesCascade",mainMenu,"Dragger Types",Vrui::getUiFont());
	draggerTypesCascade->setPopup(createDraggerTypesMenu());
	
	GLMotif::CascadeButton* influenceActionsCascade=new GLMotif::CascadeButton("InfluenceActionsCascade",mainMenu,"Influence Actions",Vrui::getUiFont());
	influenceActionsCascade->setPopup(createInfluenceActionsMenu());
	
	GLMotif::Button* createMorphBoxButton=new GLMotif::Button("CreateMorphBoxButton",mainMenu,"Create Morph Box",Vrui::getUiFont());
	createMorphBoxButton->getSelectCallbacks().add(this,&VRMeshEditor::createMorphBoxCallback);
	
	GLMotif::Button* deleteMorphBoxButton=new GLMotif::Button("DeleteMorphBoxButton",mainMenu,"Delete Morph Box",Vrui::getUiFont());
	deleteMorphBoxButton->getSelectCallbacks().add(this,&VRMeshEditor::deleteMorphBoxCallback);
	
	GLMotif::CascadeButton* settingsCascade=new GLMotif::CascadeButton("SettingsCascade",mainMenu,"Settings",Vrui::getUiFont());
	settingsCascade->setPopup(createSettingsMenu());
	
	GLMotif::Button* createInputDeviceButton=new GLMotif::Button("CreateInputDeviceButton",mainMenu,"Create Input Device",Vrui::getUiFont());
	createInputDeviceButton->getSelectCallbacks().add(this,&VRMeshEditor::createInputDeviceCallback);
	
	mainMenu->manageChild();
	
	return mainMenuPopup;
	}

void VRMeshEditor::renderMesh(VRMeshEditor::DataItem* dataItem) const
	{
	/* Check if vertex and triangle arrays are large enough: */
	unsigned int numVertices=mesh->getNextVertexIndex();
	if(dataItem->numVertices<numVertices)
		{
		delete[] dataItem->vertices;
		dataItem->numVertices=numVertices+(numVertices/2);
		dataItem->vertices=new MyVertex[dataItem->numVertices];
		}
	unsigned int numTriangles=mesh->getNumFaces();
	if(dataItem->numTriangles<numTriangles)
		{
		delete[] dataItem->triangles;
		dataItem->numTriangles=numTriangles+(numTriangles/2);
		dataItem->triangles=new unsigned int[dataItem->numTriangles*3];
		}
	
	/* Reset vertex array: */
	for(MyMesh::ConstVertexIterator vIt=mesh->beginVertices();vIt!=mesh->endVertices();++vIt)
		{
		MyVertex* vPtr=&dataItem->vertices[vIt->index];
		for(int i=0;i<3;++i)
			{
			vPtr->normal[i]=0.0f;
			vPtr->position[i]=(*vIt)[i];
			}
		}
	
	/* Traverse triangles once to calculate normal vectors for smooth shading: */
	unsigned int* viPtr=dataItem->triangles;
	for(MyMesh::ConstFaceIterator fIt=mesh->beginFaces();fIt!=mesh->endFaces();++fIt,viPtr+=3)
		{
		/* Gather triangle's points: */
		const MyMesh::Vertex* v[3];
		const MyMesh::Edge* e=fIt->getEdge();
		for(int i=0;i<3;++i)
			{
			v[i]=e->getStart();
			viPtr[i]=v[i]->index;
			e=e->getFaceSucc();
			}
		
		/* Calculate triangle's normal vector: */
		float normal[3];
		planeNormal(*v[0],*v[1],*v[2],normal);
		
		/* Distribute normal vector to triangle's vertices: */
		for(int i=0;i<3;++i)
			{
			MyVertex* vPtr=&dataItem->vertices[viPtr[i]];
			for(int j=0;j<3;++j)
				vPtr->normal[j]+=normal[j];
			}
		}
	
	#if 0
	static bool saveVertices=true;
	if(saveVertices)
		{
		#if 0
		float min[3],max[3];
		MyMesh::ConstVertexIterator vIt=mesh->beginVertices();
		for(int i=0;i<3;++i)
			min[i]=max[i]=vIt->pos()[i];
		for(++vIt;vIt!=mesh->endVertices();++vIt)
			for(int i=0;i<3;++i)
				{
				if(min[i]>vIt->pos()[i])
					min[i]=vIt->pos()[i];
				else if(max[i]<vIt->pos()[i])
					max[i]=vIt->pos()[i];
				}
		float center[3];
		for(int i=0;i<3;++i)
			center[i]=(min[i]+max[i])*0.5f;
		float scale=60.0f/(max[0]-min[0]);
		for(int i=1;i<3;++i)
			{
			float scale2=60.0f/(max[i]-min[i]);
			if(scale>scale2)
				scale=scale2;
			}
		#endif
		
		#if 0
		FILE* sampleFile=fopen("../ScatteredData/ModelVertices.txt","wt");
		fprintf(sampleFile,"%u 3 1\n",numVertices);
		for(MyMesh::ConstVertexIterator vIt=mesh->beginVertices();vIt!=mesh->endVertices();++vIt)
			{
			MyVertex* vPtr=&dataItem->vertices[vIt->index];
			float pos[3];
			for(int i=0;i<3;++i)
				pos[i]=vPtr->position[i];
				//pos[i]=(vPtr->position[i]-center[i])*scale+64.0f;
			float normLen=sqrtf(vPtr->normal[0]*vPtr->normal[0]+vPtr->normal[1]*vPtr->normal[1]+vPtr->normal[2]*vPtr->normal[2]);
			fprintf(sampleFile,"%8.3f %8.3f %8.3f 0 0 0 %6.3f %6.3f %6.3f\n",pos[0],pos[1],pos[2],vPtr->normal[0]/normLen,vPtr->normal[1]/normLen,vPtr->normal[2]/normLen);
			}
		fclose(sampleFile);
		#endif
		
		FILE* triangleFile=fopen("../../Teaching/ECS175/Project2/SubdivisionModel.tris","wt");
		fprintf(triangleFile,"color %5.3f, %5.3f, %5.3f\n",0.5f,0.5f,0.5f);
		
		for(MyMesh::ConstFaceIterator fIt=mesh->beginFaces();fIt!=mesh->endFaces();++fIt,viPtr+=3)
			{
			fprintf(triangleFile,"beginPolygon\n");
			
			/* Gather triangle's points: */
			const MyMesh::Vertex* v[3];
			const MyVertex* vs[3];
			const MyMesh::Edge* e=fIt->getEdge();
			for(int i=0;i<3;++i)
				{
				v[i]=e->getStart();
				vs[i]=&dataItem->vertices[v[i]->index];
				e=e->getFaceSucc();
				}
			
			float normal[3];
			planeNormal(*v[0],*v[1],*v[2],normal);
			fprintf(triangleFile,"normal %6.3f, %6.3f, %6.3f\n",normal[0],normal[1],normal[2]);
			
			for(int i=0;i<3;++i)
				{
				//fprintf(triangleFile,"normal %6.3f, %6.3f, %6.3f\n",vs[i]->normal[0],vs[i]->normal[1],vs[i]->normal[2]);
				fprintf(triangleFile,"vertex %8.3f, %8.3f, %8.3f\n",vs[i]->position[0],vs[i]->position[1],vs[i]->position[2]);
				}
			
			fprintf(triangleFile,"end\n");
			}
		
		fclose(triangleFile);
		
		saveVertices=false;
		}
	#endif
	
	/* Traverse triangles again to render: */
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(dataItem->vertices);
	glDrawElements(GL_TRIANGLES,numTriangles*3,GL_UNSIGNED_INT,dataItem->triangles);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	}

void VRMeshEditor::setMeshDraggerActionType(Influence::ActionType newActionType)
	{
	/* Set the new default action type: */
	defaultActionType=newActionType;
	
	if(overrideTools)
		{
		/* Set action type of all mesh draggers to the new default: */
		for(DraggerList::iterator dIt=draggers.begin();dIt!=draggers.end();++dIt)
			if(dynamic_cast<MeshDragger*>(*dIt)!=0)
				dynamic_cast<MeshDragger*>(*dIt)->setAction(newActionType);
		}
	}

VRMeshEditor::VRMeshEditor(int& argc,char**& argv,char**& appDefaults)
	:Vrui::Application(argc,argv,appDefaults),
	 mesh(0),
	 morphBox(0),
	 defaultDraggerType(MESHDRAGGER),
	 defaultActionType(Influence::DRAG),
	 overrideTools(true),
	 renderMode(SHADED),
	 meshMaterial(GLMaterial::Color(0.7f,0.7f,0.7f),GLMaterial::Color(1.0f,1.0f,1.0f),50.0f),
	 mainMenu(0)
	{
	/* Parse the command line: */
	int meshFileType=1;
	const char* meshFileName=0;
	int subdivisionDepth=2;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"GTS")==0)
				meshFileType=2;
			else if(strcasecmp(argv[i]+1,"PLY")==0)
				meshFileType=3;
			else if(strcasecmp(argv[i]+1,"DEPTH")==0)
				{
				subdivisionDepth=atoi(argv[i+1]);
				++i;
				}
			}
		else
			meshFileName=argv[i];
		}
	if(meshFileName==0)
		{
		meshFileType=4;
		// throw std::runtime_error("VRMeshEditor: No mesh file name supplied");
		}
	
	/* Load the base mesh: */
	MyMesh::BaseMesh* baseMesh=0;
	switch(meshFileType)
		{
		case 1:
			baseMesh=loadMeshfile<MyMesh::Point>(meshFileName);
			break;
		
		case 2:
			baseMesh=loadGtsMeshfile<MyMesh::Point>(meshFileName);
			break;
		
		case 3:
			baseMesh=loadPlyMeshfile<MyMesh::Point>(meshFileName);
			break;
		
		case 4:
			baseMesh=createTetrahedron<MyMesh::Point>();
			break;
		}
	
	/* Subdivide the base mesh: */
	for(int i=0;i<subdivisionDepth;++i)
		subdivideCatmullClark(*baseMesh);
	mesh=new MyMesh(*baseMesh);
	delete baseMesh;
	
	/* Create the main menu: */
	mainMenu=createMainMenu();
	Vrui::setMainMenu(mainMenu);
	
	/* Set additional physical-space light sources: */
	Vrui::Point l1=Vrui::getDisplayCenter()+Vrui::Vector(-1.0,-0.1,0.5)*Vrui::getDisplaySize();
	Vrui::Point l2=Vrui::getDisplayCenter()+Vrui::Vector(1.0,-0.1,0.5)*Vrui::getDisplaySize();
	Vrui::getLightsourceManager()->createLightsource(true,GLLight(GLLight::Color(0.5f,0.25f,0.25f),GLLight::Position(l1[0],l1[1],l1[2],1.0f)));
	Vrui::getLightsourceManager()->createLightsource(true,GLLight(GLLight::Color(0.25f,0.25f,0.5f),GLLight::Position(l2[0],l2[1],l2[2],1.0f)));
	
	/* Initialize navigation transformation: */
	centerDisplayCallback(0);
	}

VRMeshEditor::~VRMeshEditor(void)
	{
	delete morphBox;
	delete mesh;
	delete mainMenu;
	}

void VRMeshEditor::toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData)
	{
	/* Check if the new tool is a dragging tool: */
	Vrui::DraggingTool* tool=dynamic_cast<Vrui::DraggingTool*>(cbData->tool);
	if(tool!=0)
		{
		Dragger* newDragger=0;
		switch(defaultDraggerType)
			{
			case MESHDRAGGER:
				/* Create a mesh dragger object and associate it with the new tool: */
				newDragger=new MeshDragger(tool,this);
				break;
			
			case MORPHBOXDRAGGER:
				/* Create a morph box dragger object and associate it with the new tool: */
				newDragger=new MorphBoxDragger(tool,this);
				break;
			}
		
		/* Add new dragger to list: */
		draggers.push_back(newDragger);
		}
	}

void VRMeshEditor::toolDestructionCallback(Vrui::ToolManager::ToolDestructionCallbackData* cbData)
	{
	/* Check if the to-be-destroyed tool is a dragging tool: */
	Vrui::DraggingTool* tool=dynamic_cast<Vrui::DraggingTool*>(cbData->tool);
	if(tool!=0)
		{
		/* Find the dragger associated with the tool in the list: */
		DraggerList::iterator dIt;
		for(dIt=draggers.begin();dIt!=draggers.end()&&(*dIt)->getTool()!=tool;++dIt)
			;
		if(dIt!=draggers.end())
			{
			/* Remove the dragger: */
			delete *dIt;
			draggers.erase(dIt);
			}
		}
	}

void VRMeshEditor::initContext(GLContextData& contextData) const
	{
	/* Create a new context entry: */
	contextData.addDataItem(this,new DataItem);
	
	#if 0
	/* Disable all viewers' headlights: */
	for(int i=0;i<Vrui::getNumViewers();++i)
		Vrui::getViewer(i)->setHeadlightState(false);
	#endif
	}

void VRMeshEditor::frame(void)
	{
	}

void VRMeshEditor::display(GLContextData& contextData) const
	{
	/* Retrieve context entry: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Render mesh: */
	glPushAttrib(GL_ENABLE_BIT|GL_POLYGON_BIT);
	switch(renderMode)
		{
		case SHADED:
			glEnable(GL_LIGHTING);
			glEnable(GL_NORMALIZE);
			glDisable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glFrontFace(GL_CCW);
			glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
			glMaterial(GLMaterialEnums::FRONT,meshMaterial);
			renderMesh(dataItem);
			break;
		
		case WIREFRAME:
			glDisable(GL_LIGHTING);
			glColor3f(0.0f,1.0f,0.0f);
			renderMeshWireframe(*mesh,1.0f,0.0f);
			break;
		}
	glPopAttrib();
	
	/* Render all draggers: */
	for(DraggerList::const_iterator dIt=draggers.begin();dIt!=draggers.end();++dIt)
		(*dIt)->glRenderAction(contextData);
	
	/* Render the morph box: */
	if(morphBox!=0)
		morphBox->glRenderAction(contextData);
	}

void VRMeshEditor::radioBoxEntrySelectCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData)
	{
	if(strcmp(cbData->newSelectedToggle->getName(),"MeshDraggerToggle")==0)
		defaultDraggerType=MESHDRAGGER;
	else if(strcmp(cbData->newSelectedToggle->getName(),"MorphBoxDraggerToggle")==0)
		defaultDraggerType=MORPHBOXDRAGGER;
	else if(strcmp(cbData->newSelectedToggle->getName(),"DragToggle")==0)
		setMeshDraggerActionType(Influence::DRAG);
	else if(strcmp(cbData->newSelectedToggle->getName(),"ExplodeToggle")==0)
		setMeshDraggerActionType(Influence::EXPLODE);
	else if(strcmp(cbData->newSelectedToggle->getName(),"WhittleToggle")==0)
		setMeshDraggerActionType(Influence::WHITTLE);
	else if(strcmp(cbData->newSelectedToggle->getName(),"DrawWireframeToggle")==0)
		renderMode=WIREFRAME;
	else if(strcmp(cbData->newSelectedToggle->getName(),"DrawShadedToggle")==0)
		renderMode=SHADED;
	}

void VRMeshEditor::overrideToolsValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	overrideTools=cbData->set;
	}

void VRMeshEditor::centerDisplayCallback(Misc::CallbackData* cbData)
	{
	/* Calculate bounding box of current mesh: */
	Vrui::Point bbMin,bbMax;
	MyVIt vIt=mesh->beginVertices();
	for(int i=0;i<3;++i)
		bbMin[i]=bbMax[i]=(*vIt)[i];
	for(++vIt;vIt!=mesh->endVertices();++vIt)
		{
		for(int i=0;i<3;++i)
			{
			if(bbMin[i]>(*vIt)[i])
				bbMin[i]=(*vIt)[i];
			else if(bbMax[i]<(*vIt)[i])
				bbMax[i]=(*vIt)[i];
			}
		}
	Vrui::Point modelCenter=Geometry::mid(bbMin,bbMax);
	Vrui::Scalar modelSize=Geometry::dist(modelCenter,bbMax);
	
	/* Calculate navigation transformation: */
	Vrui::NavTransform t=Vrui::NavTransform::translateFromOriginTo(Vrui::getDisplayCenter());
	t*=Vrui::NavTransform::scale(Vrui::Scalar(0.5)*Vrui::getDisplaySize()/modelSize);
	t*=Vrui::NavTransform::translateToOriginFrom(modelCenter);
	Vrui::setNavigationTransformation(t);
	}

void VRMeshEditor::createMorphBoxCallback(Misc::CallbackData* cbData)
	{
	/* Delete the old morph box: */
	if(morphBox!=0)
		{
		delete morphBox;
		morphBox=0;
		}
	
	/* Calculate bounding box of current mesh: */
	MyMorphBox::Point bbMin,bbMax;
	MyVIt vIt=mesh->beginVertices();
	for(int i=0;i<3;++i)
		bbMin[i]=bbMax[i]=(*vIt)[i];
	for(++vIt;vIt!=mesh->endVertices();++vIt)
		{
		for(int i=0;i<3;++i)
			{
			if(bbMin[i]>(*vIt)[i])
				bbMin[i]=(*vIt)[i];
			else if(bbMax[i]<(*vIt)[i])
				bbMax[i]=(*vIt)[i];
			}
		}
	
	/* Create a new morph box: */
	MyMorphBox::Scalar size[3];
	for(int i=0;i<3;++i)
		size[i]=bbMax[i]-bbMin[i];
	morphBox=new MyMorphBox(mesh,bbMin,size);
	}

void VRMeshEditor::deleteMorphBoxCallback(Misc::CallbackData* cbData)
	{
	if(morphBox!=0)
		{
		delete morphBox;
		morphBox=0;
		}
	}

void VRMeshEditor::createInputDeviceCallback(Misc::CallbackData* cbData)
	{
	/* Create a new input device: */
	Vrui::addVirtualInputDevice("Virtual",1,0);
	}

int main(int argc,char* argv[])
	{
	try
		{
		char** appDefaults=0;
		VRMeshEditor vme(argc,argv,appDefaults);
		vme.run();
		}
	catch(std::runtime_error err)
		{
		fprintf(stderr,"Caught exception %s\n",err.what());
		return 1;
		}
	
	return 0;
	}
