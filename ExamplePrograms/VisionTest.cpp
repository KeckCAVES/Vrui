/***********************************************************************
VisionTest - Utility to draw a vision test chart to test the visual
acuity provided by a VR display.
Copyright (c) 2015-2017 Oliver Kreylos

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

#include <iostream>
#include <Misc/MessageLogger.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Math/Random.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/LinearUnit.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLMaterialTemplates.h>
#include <GL/GLObject.h>
#include <GL/GLContextData.h>
#include <GL/GLFrameBuffer.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/NewButton.h>
#include <GLMotif/Texture.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>
#include <Vrui/Viewer.h>

/****************
Helper functions:
****************/

void renderLandoltC(double centerX,double centerY,double diameter,double angle)
	{
	double r=diameter*0.5;
	double w=diameter*0.2;
	double innerAlpha=Math::asin(0.5*w/(r-w));
	double outerAlpha=Math::asin(0.5*w/r);
	
	glBegin(GL_QUAD_STRIP);
	glNormal3d(0.0,0.0,1.0);
	int numSegments=128;
	for(int i=0;i<=numSegments;++i)
		{
		double ia=(innerAlpha*double(numSegments-i)+(2.0*Math::Constants<double>::pi-innerAlpha)*double(i))/double(numSegments);
		double oa=(outerAlpha*double(numSegments-i)+(2.0*Math::Constants<double>::pi-outerAlpha)*double(i))/double(numSegments);
		glVertex2d(centerX+Math::cos(ia+angle)*(r-w),centerY+Math::sin(ia+angle)*(r-w));
		glVertex2d(centerX+Math::cos(oa+angle)*r,centerY+Math::sin(oa+angle)*r);
		}
	glEnd();
	}

/**************
Helper classes:
**************/

class OptotypeTexture:public GLMotif::Texture
	{
	/* Elements: */
	private:
	double angle; // Angle of the optotype drawn in this texture
	
	/* Protected methods from GLMotif::Texture: */
	protected:
	virtual void uploadTexture(GLuint textureObjectId,bool npotdtSupported,const unsigned int textureSize[2],GLContextData& contextData) const;
	
	/* Constructors and destructors: */
	public:
	OptotypeTexture(const char* sName,GLMotif::Container* sParent,Vrui::Scalar sAngle,bool sManageChild =true);
	
	/* New methods: */
	double getAngle(void) const // Returns the optotype's angle
		{
		return angle;
		}
	};

/********************************
Methods of class OptotypeTexture:
********************************/

void OptotypeTexture::uploadTexture(GLuint textureObjectId,bool npotdtSupported,const unsigned int textureSize[2],GLContextData& contextData) const
	{
	/* Create a frame buffer to render an optotype into the widget's texture: */
	GLFrameBuffer fb(textureSize[0],textureSize[1],!npotdtSupported);
	
	#if 0
	/* Create the texture image: */
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,textureSize[0],textureSize[1],0,GL_RGB,GL_UNSIGNED_BYTE,0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	#endif
	
	/* Unbind the texture and attach it to the frame buffer: */
	// glBindTexture(GL_TEXTURE_2D,0);
	fb.attachDepthBuffer();
	fb.attachColorTexture(0,GL_RGB8,GL_LINEAR);
	
	/* Bind the frame buffer: */
	{
	GLFrameBuffer::Binder fbBinder(fb);
	
	/* Select the target buffers: */
	fb.selectBuffers(GL_COLOR_ATTACHMENT0_EXT,GL_COLOR_ATTACHMENT0_EXT);
	
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	
	/* Set up viewport and matrices: */
	GLint previousViewport[4];
	glGetIntegerv(GL_VIEWPORT,previousViewport);
	glViewport(0,0,getSize(0),getSize(1));
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-1.0,1.0,-1.0,1.0,-1.0,1.0);
	
	/* Draw the optotype: */
	glColor3f(1.0f,1.0f,1.0f);
	glBegin(GL_QUADS);
	glVertex2d(-1.0,-1.0);
	glVertex2d(1.0,-1.0);
	glVertex2d(1.0,1.0);
	glVertex2d(-1.0,1.0);
	glEnd();
	glColor(getForegroundColor());
	if(angle>=0.0)
		renderLandoltC(0.0,0.0,1.6,angle);
	else
		{
		/* Draw a fat "X": */
		glBegin(GL_TRIANGLE_FAN);
		glVertex2d(0.0,0.0);
		glVertex2d(0.5,-0.7);
		glVertex2d(0.7,-0.5);
		glVertex2d(0.2,0.0);
		glVertex2d(0.7,0.5);
		glVertex2d(0.5,0.7);
		glVertex2d(0.0,0.2);
		glVertex2d(-0.5,0.7);
		glVertex2d(-0.7,0.5);
		glVertex2d(-0.2,0.0);
		glVertex2d(-0.7,-0.5);
		glVertex2d(-0.5,-0.7);
		glVertex2d(0.0,-0.2);
		glVertex2d(0.5,-0.7);
		glEnd();
		}
	
	/* Restore the viewport and matrices: */
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glViewport(previousViewport[0],previousViewport[1],previousViewport[2],previousViewport[3]);
	
	/* Restore OpenGL state: */
	glPopAttrib();
	
	/* Copy texture image from the frame buffer into the bound texture: */
	glCopyTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,0,0,textureSize[0],textureSize[1],0);
	}
	
	/* Detach the texture image from the frame buffer and bind it: */
	// glBindTexture(GL_TEXTURE_2D,fb.detachColorTexture(0));
	}

OptotypeTexture::OptotypeTexture(const char* sName,GLMotif::Container* sParent,Vrui::Scalar sAngle,bool sManageChild)
	:GLMotif::Texture(sName,sParent),
	 angle(sAngle)
	{
	/* Set the texture size and resolution: */
	static const unsigned int size[2]={128,128};
	setSize(size);
	GLfloat resolution[2];
	for(int i=0;i<2;++i)
		resolution[i]=GLfloat(size[i])/(GLfloat(Vrui::getUiSize())*4.0f);
	setResolution(resolution);
	
	/* Set render settings: */
	setInterpolationMode(GL_LINEAR);
	setIlluminated(true);
	
	if(sManageChild)
		manageChild();
	}

class VisionTest:public Vrui::Application,public GLObject
	{
	friend class OptotypeTexture;
	
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint displayListId; // ID of display list to render the vision chart
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	private:
	Vrui::Scalar distance; // Distance at which to render the vision chart in Vrui physical units
	Vrui::Scalar spacing; // Spacing between optotypes as a multiple of diameter
	Vrui::Scalar initialMax,initialMin; // Max and min optotype sizes at beginning of test
	Vrui::OGTransform chartTransform; // Transformation from normalized chart space (20/20 row) to Vrui physical space
	Vrui::Scalar chartSize; // Size of the chart's background rectangle
	GLMotif::PopupWindow* confirmDialog; // The optotype confirmation dialog
	Vrui::Scalar rowSizes[3]; // Optotype sizes in the top, middle, and bottom row currently displayed
	Vrui::Scalar rowPos[3]; // Vertical positions of the three optotype rows
	double angles[3*5]; // Random angles at which to draw the Langolt "C" optotypes, in 45 degree steps, in the top, middle, and bottom row
	unsigned int numGuesses; // Number of confirmed optotypes of the current size
	bool guessedRights[5]; // Array of flags indicating correct confirmations of the most recent five optotypes
	double unblankTime; // Timer to blank the display while changing between optotype sizes
	
	/* Private methods: */
	void setupTest(Vrui::Scalar bigSize,Vrui::Scalar smallSize); // Sets up test environment for the given bracket optotype sizes
	void advanceTest(bool guessedRight); // Advances the vision test with a correct or wrong optotype identification
	void confirmButtonCallback(GLMotif::NewButton::SelectCallbackData* cbData);
	void dunnoButtonCallback(GLMotif::NewButton::SelectCallbackData* cbData);
	GLMotif::PopupWindow* createConfirmDialog(void);
	
	/* Constructors and destructors: */
	public:
	VisionTest(int& argc,char**& argv);
	virtual ~VisionTest(void);
	
	/* Methods from Vrui::Application: */
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

/*************************************
Methods of class VisionTest::DataItem:
*************************************/

VisionTest::DataItem::DataItem(void)
	:displayListId(glGenLists(1))
	{
	}

VisionTest::DataItem::~DataItem(void)
	{
	glDeleteLists(displayListId,1);
	}

/***************************
Methods of class VisionTest:
***************************/

void VisionTest::setupTest(Vrui::Scalar bigSize,Vrui::Scalar smallSize)
	{
	/* Initialize the row sizes based on binary search between the big and small sizes: */
	rowSizes[0]=bigSize;
	rowSizes[2]=smallSize;
	rowSizes[1]=Math::sqrt(rowSizes[0]*rowSizes[2]); // Geometric mean
	
	int visionRatings[3];
	for(int i=0;i<3;++i)
		{
		/* Calculate angle subtended by optotype in radians: */
		Vrui::Scalar angle=Math::mul2(Math::tan(Math::div2(rowSizes[i])/distance));
		
		/* Convert subtended angle to minutes of arc: */
		angle/=Math::rad(Vrui::Scalar(1.0/60.0));
		
		/* Convert angle to vision rating: */
		visionRatings[i]=int(Math::floor(angle*Vrui::Scalar(4)+Vrui::Scalar(0.5))); // 5 minutes of arc correspond to 20/20 vision
		}
	
	if(visionRatings[0]==visionRatings[2])
		{
		/* Show a success message: */
		Misc::formattedUserNote("Vision Test Completed: Congratulations, you completed your vision test! Your visual acuity is 20/%d",visionRatings[1]);
		
		/* Start the vision test over: */
		rowSizes[0]=initialMax;
		rowSizes[2]=initialMin;
		rowSizes[1]=Math::sqrt(rowSizes[0]*rowSizes[2]); // Geometric mean
		}
	
	/* Initialize the vertical row positions: */
	rowPos[0]=Vrui::Scalar(0);
	for(int row=1;row<3;++row)
		rowPos[row]=rowPos[row-1]-rowSizes[row-1]*spacing;
	
	/* Put the middle row in the center: */
	Vrui::Scalar offset=rowPos[1];
	for(int row=0;row<3;++row)
		rowPos[row]-=offset;
	
	/* Initialize the optotype rendering angles: */
	for(int row=0;row<3;++row)
		for(int column=0;column<5;++column)
			angles[row*5+column]=double(Math::randUniformCO(0,8))*2.0*Math::Constants<double>::pi/8.0;
	
	/* Reset test criterion: */
	numGuesses=0;
	}

void VisionTest::advanceTest(bool guessedRight)
	{
	++numGuesses;
	for(int i=1;i<5;++i)
		guessedRights[i-1]=guessedRights[i];
	guessedRights[4]=guessedRight;
	
	/* Check if we're done with the current optotype size: */
	unsigned int numCorrects=0;
	for(int i=0;i<5;++i)
		if(guessedRights[i])
			++numCorrects;
	if(numGuesses>=5&&numCorrects>=4)
		{
		/* User could identify the current optotype size; move to smaller size: */
		setupTest(rowSizes[1],rowSizes[2]);
		
		/* Blank the display for two seconds: */
		unblankTime=Vrui::getApplicationTime()+2.0;
		}
	else if(numGuesses>=5&&numCorrects<=1)
		{
		/* User could not identify the current optotype size; move to larger size: */
		setupTest(rowSizes[0],rowSizes[1]);
		
		/* Blank the display for two seconds: */
		unblankTime=Vrui::getApplicationTime()+2.0;
		}
	else
		{
		/* Move to the next optotype: */
		for(int column=1;column<5;++column)
			angles[1*5+(column-1)]=angles[1*5+column];
		angles[1*5+4]=double(Math::randUniformCO(0,8))*2.0*Math::Constants<double>::pi/8.0;
		}
	}

void VisionTest::confirmButtonCallback(GLMotif::NewButton::SelectCallbackData* cbData)
	{
	/* Ignore button presses while the display is blanked: */
	if(Vrui::getApplicationTime()>=unblankTime)
		{
		/* Check if the identification was correct and advance the test: */
		advanceTest(static_cast<OptotypeTexture*>(cbData->button->getFirstChild())->getAngle()==angles[1*5+2]);
		}
	}

void VisionTest::dunnoButtonCallback(GLMotif::NewButton::SelectCallbackData* cbData)
	{
	/* Ignore button presses while the display is blanked: */
	if(Vrui::getApplicationTime()>=unblankTime)
		{
		/* Advance the test: */
		advanceTest(false);
		}
	}

GLMotif::PopupWindow* VisionTest::createConfirmDialog(void)
	{
	/* Create the dialog shell: */
	GLMotif::PopupWindow* confirmDialogPopup=new GLMotif::PopupWindow("ConfirmDialogPopup",Vrui::getWidgetManager(),"Confirmation");
	
	/* Create a rowcolumn to hold the confirmation buttons: */
	GLMotif::RowColumn* confirmDialog=new GLMotif::RowColumn("ConfirmDialog",confirmDialogPopup,false);
	confirmDialog->setOrientation(GLMotif::RowColumn::VERTICAL);
	confirmDialog->setPacking(GLMotif::RowColumn::PACK_GRID);
	confirmDialog->setNumMinorWidgets(3);
	
	/* Create confirmation buttons: */
	GLMotif::NewButton* topLeftButton=new GLMotif::NewButton("TopLeftButton",confirmDialog,false);
	OptotypeTexture* topLeft=new OptotypeTexture("TopLeft",topLeftButton,3.0*2.0*Math::Constants<double>::pi/8.0);
	topLeftButton->getSelectCallbacks().add(this,&VisionTest::confirmButtonCallback);
	topLeftButton->manageChild();
	
	GLMotif::NewButton* topButton=new GLMotif::NewButton("TopButton",confirmDialog,false);
	OptotypeTexture* top=new OptotypeTexture("Top",topButton,2.0*2.0*Math::Constants<double>::pi/8.0);
	topButton->getSelectCallbacks().add(this,&VisionTest::confirmButtonCallback);
	topButton->manageChild();
	
	GLMotif::NewButton* topRightButton=new GLMotif::NewButton("TopRightButton",confirmDialog,false);
	OptotypeTexture* topRight=new OptotypeTexture("TopRight",topRightButton,1.0*2.0*Math::Constants<double>::pi/8.0);
	topRightButton->getSelectCallbacks().add(this,&VisionTest::confirmButtonCallback);
	topRightButton->manageChild();
	
	GLMotif::NewButton* leftButton=new GLMotif::NewButton("LeftButton",confirmDialog,false);
	OptotypeTexture* left=new OptotypeTexture("Left",leftButton,4.0*2.0*Math::Constants<double>::pi/8.0);
	leftButton->getSelectCallbacks().add(this,&VisionTest::confirmButtonCallback);
	leftButton->manageChild();
	
	GLMotif::NewButton* dunnoButton=new GLMotif::NewButton("DunnoButton",confirmDialog,false);
	OptotypeTexture* dunno=new OptotypeTexture("Dunno",dunnoButton,-1.0);
	dunnoButton->getSelectCallbacks().add(this,&VisionTest::dunnoButtonCallback);
	dunnoButton->manageChild();
	
	GLMotif::NewButton* rightButton=new GLMotif::NewButton("RightButton",confirmDialog,false);
	OptotypeTexture* right=new OptotypeTexture("Right",rightButton,0.0*2.0*Math::Constants<double>::pi/8.0);
	rightButton->getSelectCallbacks().add(this,&VisionTest::confirmButtonCallback);
	rightButton->manageChild();
	
	GLMotif::NewButton* bottomLeftButton=new GLMotif::NewButton("BottomLeftButton",confirmDialog,false);
	OptotypeTexture* bottomLeft=new OptotypeTexture("BottomLeft",bottomLeftButton,5.0*2.0*Math::Constants<double>::pi/8.0);
	bottomLeftButton->getSelectCallbacks().add(this,&VisionTest::confirmButtonCallback);
	bottomLeftButton->manageChild();
	
	GLMotif::NewButton* bottomButton=new GLMotif::NewButton("BottomButton",confirmDialog,false);
	OptotypeTexture* bottom=new OptotypeTexture("Bottom",bottomButton,6.0*2.0*Math::Constants<double>::pi/8.0);
	bottomButton->getSelectCallbacks().add(this,&VisionTest::confirmButtonCallback);
	bottomButton->manageChild();
	
	GLMotif::NewButton* bottomRightButton=new GLMotif::NewButton("BottomRightButton",confirmDialog,false);
	OptotypeTexture* bottomRight=new OptotypeTexture("BottomRight",bottomRightButton,7.0*2.0*Math::Constants<double>::pi/8.0);
	bottomRightButton->getSelectCallbacks().add(this,&VisionTest::confirmButtonCallback);
	bottomRightButton->manageChild();
	
	confirmDialog->manageChild();
	
	return confirmDialogPopup;
	}

VisionTest::VisionTest(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 spacing(1.5),
	 confirmDialog(0),
	 unblankTime(0.0)
	{
	/* Parse the command line: */
	Geometry::LinearUnit distanceUnit(Geometry::LinearUnit::FOOT,Geometry::LinearUnit::Scalar(1)); // Default unit is foot
	distance=Vrui::Scalar(20); // Default distance is 20'
	if(argc>1)
		distance=Vrui::Scalar(atof(argv[1]));
	if(argc>2)
		distanceUnit=Geometry::LinearUnit(argv[2],Geometry::LinearUnit::Scalar(1));
	
	/* Convert the distance to Vrui physical units through meters: */
	distance*=Vrui::getMeterFactor()/distanceUnit.getMeterFactor();
	
	/* Initialize the row sizes based on binary search between 20/100 and 20/20: */
	initialMax=Math::mul2(Math::tan(Math::div2(Math::rad(Vrui::Scalar(25.0/60.0)))))*distance; // 20/100 row
	initialMin=Math::mul2(Math::tan(Math::div2(Math::rad(Vrui::Scalar(2.5/60.0)))))*distance; // 20/10 row
	setupTest(initialMax,initialMin);
	
	/* Calculate the size of the background chart: */
	chartSize=rowSizes[0]*Vrui::Scalar(7);
	
	/* Create and show the confirmation dialog: */
	confirmDialog=createConfirmDialog();
	Vrui::popupPrimaryWidget(confirmDialog);
	}

VisionTest::~VisionTest(void)
	{
	delete confirmDialog;
	}

void VisionTest::frame(void)
	{
	/* Schedule another frame to unblank the display: */
	if(Vrui::getApplicationTime()<unblankTime)
		Vrui::scheduleUpdate(unblankTime);
	}

void VisionTest::display(GLContextData& contextData) const
	{
	/* Retrieve the context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(1.0f);
	
	/* Render vision chart selected distance in front of display center: */
	glPushMatrix();
	glMultMatrix(chartTransform);
	
	/* Draw a white background: */
	glColor3f(1.0f,1.0f,1.0f);
	Vrui::Scalar border=Vrui::getInchFactor()*Vrui::Scalar(4);
	glBegin(GL_QUADS);
	glVertex3d(-chartSize,-chartSize*Vrui::Scalar(1.2),-0.1);
	glVertex3d(chartSize,-chartSize*Vrui::Scalar(1.2),-0.1);
	glVertex3d(chartSize,chartSize*Vrui::Scalar(1.2),-0.1);
	glVertex3d(-chartSize,chartSize*Vrui::Scalar(1.2),-0.1);
	glEnd();
	
	if(Vrui::getApplicationTime()>=unblankTime)
		{
		/* Draw the three rows of optotypes in black: */
		glColor3f(0.0f,0.0f,0.0f);
		for(int row=0;row<3;++row)
			{
			/* Calculate this row's optotype spacing: */
			Vrui::Scalar colSpacing=rowSizes[row]*spacing;
			
			/* Draw the row's optotypes: */
			for(int column=0;column<5;++column)
				{
				if(row==1&&column==2)
					glColor3f(0.0f,0.0f,0.0f);
				else
					glColor3f(0.5f,0.5f,0.5f);
				Vrui::Scalar x=Vrui::Scalar(column-2)*colSpacing;
				renderLandoltC(x,rowPos[row],rowSizes[row],angles[row*5+column]);
				}
			}
		
		/* Draw the active optotype indicators: */
		glBegin(GL_LINES);
		glVertex2d(Vrui::Scalar(-6)*rowSizes[1]*spacing,0.0);
		glVertex2d(Vrui::Scalar(-3)*rowSizes[1]*spacing,0.0);
		glVertex2d(Vrui::Scalar(3)*rowSizes[1]*spacing,0.0);
		glVertex2d(Vrui::Scalar(6)*rowSizes[1]*spacing,0.0);
		
		glVertex2d(0.0,rowPos[0]+Vrui::Scalar(4)*rowSizes[0]*spacing);
		glVertex2d(0.0,rowPos[0]+rowSizes[0]*spacing);
		glVertex2d(0.0,rowPos[2]-rowSizes[2]*spacing);
		glVertex2d(0.0,rowPos[2]-Vrui::Scalar(4)*rowSizes[2]*spacing);
		glEnd();
		}
	
	/* Go back to physical coordinates: */
	glPopMatrix();
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

void VisionTest::resetNavigation(void)
	{
	/* Calculate the chart transformation by placing the test chart distance ahead of the current viewer position: */
	Vrui::Point center=Vrui::getMainViewer()->getHeadPosition();
	center+=Vrui::getForwardDirection()*distance;
	chartTransform=Vrui::OGTransform::translateFromOriginTo(center);
	
	/* Place the chart vertically: */
	Vrui::Vector y=Vrui::getUpDirection();
	Vrui::Vector x=Vrui::getForwardDirection()^y;
	chartTransform*=Vrui::OGTransform::rotate(Vrui::Rotation::fromBaseVectors(x,y));
	
	chartTransform.renormalize();
	
	/* Render everything in physical coordinates: */
	Vrui::setNavigationTransformation(Vrui::NavTransform::identity);
	}

void VisionTest::initContext(GLContextData& contextData) const
	{
	/* Create a context data item and associate it with the OpenGL context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Initialize the chart display list: */
	glNewList(dataItem->displayListId,GL_COMPILE);
	
	glColor(Vrui::getForegroundColor());
	
	/* Draw the rows upwards from the central 20/20 row: */
	double spacing=1.5;
	
	double diameter=1.0;
	double y=0.0;
	for(int row=0;row<10;++row)
		{
		for(int i=0;i<5;++i)
			renderLandoltC(0.0-diameter*spacing*double(i-2),y,diameter,angles[(row+4)*5+i]);
		
		y+=diameter*spacing;
		diameter*=1.25;
		}
	
	/* Draw the rows downwards from the central 20/20 row: */
	diameter=1.0;
	y=0.0;
	
	/* Draw the central row indicators: */
	glBegin(GL_LINES);
	glVertex2d(-diameter*spacing*-10.0,0.0);
	glVertex2d(-diameter*spacing*-3.0,0.0);
	
	glVertex2d(-diameter*spacing*3.0,0.0);
	glVertex2d(-diameter*spacing*10.0,0.0);
	glEnd();
	
	for(int row=1;row<5;++row)
		{
		diameter/=1.25;
		y-=diameter*spacing;
		
		for(int i=0;i<5;++i)
			renderLandoltC(0.0-diameter*spacing*double(i-2),y,diameter,angles[(4-row)*5+i]);
		}
	
	glEndList();
	}

/* Create and execute an application object: */
VRUI_APPLICATION_RUN(VisionTest)
