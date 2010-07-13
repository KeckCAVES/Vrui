/***********************************************************************
InputDeviceAdapterMouse - Class to convert mouse and keyboard into a
Vrui input device.
Copyright (c) 2004-2010 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Vrui/Internal/InputDeviceAdapterMouse.h>

#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/Ray.h>
#include <Geometry/GeometryValueCoders.h>
#include <GLMotif/WidgetManager.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/Internal/MouseCursorFaker.h>
#include <Vrui/VRScreen.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRWindow.h>
#include <Vrui/Vrui.h>

namespace Vrui {

namespace {

struct KeyMapItem
	{
	/* Elements: */
	public:
	const char* name;
	int keysym;
	};

const KeyMapItem keyMap[]=
	{
	{"Space",XK_space},{"Tab",XK_Tab},{"Return",XK_Return},{"Backspace",XK_BackSpace},
	{"Left",XK_Left},{"Up",XK_Up},{"Right",XK_Right},{"Down",XK_Down},
	{"PageUp",XK_Page_Up},{"PageDown",XK_Page_Down},{"Home",XK_Home},{"End",XK_End},{"Insert",XK_Insert},{"Delete",XK_Delete},
	{"Num0",XK_KP_Insert},{"Num1",XK_KP_End},{"Num2",XK_KP_Down},{"Num3",XK_KP_Page_Down},{"Num4",XK_KP_Left},
	{"Num5",XK_KP_Begin},{"Num6",XK_KP_Right},{"Num7",XK_KP_Home},{"Num8",XK_KP_Up},{"Num9",XK_KP_Page_Up},
	{"Num/",XK_KP_Divide},{"Num*",XK_KP_Multiply},{"Num-",XK_KP_Subtract},{"Num+",XK_KP_Add},{"NumEnter",XK_KP_Enter},{"NumSep",XK_KP_Separator},
	{"LeftShift",XK_Shift_L},{"RightShift",XK_Shift_R},{"CapsLock",XK_Caps_Lock},{"LeftCtrl",XK_Control_L},{"RightCtrl",XK_Control_R},
	{"LeftAlt",XK_Alt_L},{"RightAlt",XK_Alt_R},{"LeftMeta",XK_Meta_L},{"RightMeta",XK_Meta_R},
	{"LeftSuper",XK_Super_L},{"RightSuper",XK_Super_R},{"LeftHyper",XK_Hyper_L},{"RightHyper",XK_Hyper_R},
	{"F1",XK_F1},{"F2",XK_F2},{"F3",XK_F3},{"F4",XK_F4},{"F5",XK_F5},{"F6",XK_F6},
	{"F7",XK_F7},{"F8",XK_F8},{"F9",XK_F9},{"F10",XK_F10},{"F11",XK_F11},{"F12",XK_F12}
	};

typedef InputDeviceAdapterMouse::ControlKey CK;
typedef GLMotif::TextControlEvent TCE;

struct ControlKeyMapItem
	{
	/* Elements: */
	public:
	CK ck;
	TCE tce;
	};

ControlKeyMapItem rawControlKeyMap[]=
	{
	{CK(XK_Left,0x0),TCE(TCE::CURSOR_LEFT,false)},
	{CK(XK_Right,0x0),TCE(TCE::CURSOR_RIGHT,false)},
	{CK(XK_Left,ControlMask),TCE(TCE::CURSOR_WORD_LEFT,false)},
	{CK(XK_Right,ControlMask),TCE(TCE::CURSOR_WORD_RIGHT,false)},
	{CK(XK_Home,0x0),TCE(TCE::CURSOR_START,false)},
	{CK(XK_End,0x0),TCE(TCE::CURSOR_END,false)},
	{CK(XK_Up,0x0),TCE(TCE::CURSOR_UP,false)},
	{CK(XK_Down,0x0),TCE(TCE::CURSOR_DOWN,false)},
	{CK(XK_Page_Up,0x0),TCE(TCE::CURSOR_PAGE_UP,false)},
	{CK(XK_Page_Down,0x0),TCE(TCE::CURSOR_PAGE_DOWN,false)},
	{CK(XK_Home,ControlMask),TCE(TCE::CURSOR_TEXT_START,false)},
	{CK(XK_End,ControlMask),TCE(TCE::CURSOR_TEXT_END,false)},
	
	{CK(XK_Left,ShiftMask),TCE(TCE::CURSOR_LEFT,true)},
	{CK(XK_Right,ShiftMask),TCE(TCE::CURSOR_RIGHT,true)},
	{CK(XK_Left,ControlMask|ShiftMask),TCE(TCE::CURSOR_WORD_LEFT,true)},
	{CK(XK_Right,ControlMask|ShiftMask),TCE(TCE::CURSOR_WORD_RIGHT,true)},
	{CK(XK_Home,ShiftMask),TCE(TCE::CURSOR_START,true)},
	{CK(XK_End,ShiftMask),TCE(TCE::CURSOR_END,true)},
	{CK(XK_Up,ShiftMask),TCE(TCE::CURSOR_UP,true)},
	{CK(XK_Down,ShiftMask),TCE(TCE::CURSOR_DOWN,true)},
	{CK(XK_Page_Up,ShiftMask),TCE(TCE::CURSOR_PAGE_UP,true)},
	{CK(XK_Page_Down,ShiftMask),TCE(TCE::CURSOR_PAGE_DOWN,true)},
	{CK(XK_Home,ControlMask|ShiftMask),TCE(TCE::CURSOR_TEXT_START,true)},
	{CK(XK_End,ControlMask|ShiftMask),TCE(TCE::CURSOR_TEXT_END,true)},
	
	{CK(XK_Delete,0x0),TCE(TCE::DELETE)},
	{CK(XK_BackSpace,0x0),TCE(TCE::BACKSPACE)},
	
	{CK(XK_Delete,ShiftMask),TCE(TCE::CUT)},
	{CK(XK_x,ControlMask),TCE(TCE::CUT)},
	{CK(XK_X,ControlMask),TCE(TCE::CUT)},
	{CK(XK_Insert,ControlMask),TCE(TCE::COPY)},
	{CK(XK_c,ControlMask),TCE(TCE::COPY)},
	{CK(XK_C,ControlMask),TCE(TCE::COPY)},
	{CK(XK_Insert,ShiftMask),TCE(TCE::PASTE)},
	{CK(XK_v,ControlMask),TCE(TCE::PASTE)},
	{CK(XK_V,ControlMask),TCE(TCE::PASTE)},
	
	{CK(XK_Return,0x0),TCE(TCE::CONFIRM)}
	};

}

/****************************************
Methods of class InputDeviceAdapterMouse:
****************************************/

int InputDeviceAdapterMouse::getKeyCode(std::string keyName)
	{
	int result=NoSymbol;
	
	/* Check for built-in lecacy key names first: */
	const int keyMapSize=sizeof(keyMap)/sizeof(KeyMapItem);
	for(int i=0;i<keyMapSize&&result==NoSymbol;++i)
		if(keyName==keyMap[i].name)
			result=keyMap[i].keysym;
		
	/* Check for X11 key names: */
	if(result==NoSymbol)
		result=XStringToKeysym(keyName.c_str());
	
	if(result==NoSymbol)
		Misc::throwStdErr("InputDeviceAdapterMouse: Unknown key name \"%s\"",keyName.c_str());
	
	return result;
	}

void InputDeviceAdapterMouse::changeModifierKeyMask(int newModifierKeyMask)
	{
	/* Copy all button states from the old layer to the new layer: */
	bool* oldLayer=buttonStates+(numButtons+numButtonKeys)*modifierKeyMask;
	bool* newLayer=buttonStates+(numButtons+numButtonKeys)*newModifierKeyMask;
	for(int i=0;i<numButtons+numButtonKeys;++i)
		newLayer[i]=oldLayer[i];
	
	/* Change the modifier key mask: */
	modifierKeyMask=newModifierKeyMask;
	}

InputDeviceAdapterMouse::InputDeviceAdapterMouse(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection)
	:InputDeviceAdapter(sInputDeviceManager),
	 numButtons(0),
	 numButtonKeys(0),buttonKeyCodes(0),
	 numModifierKeys(0),modifierKeyCodes(0),
	 numButtonStates(0),
	 keyboardModeToggleKeyCode(0),controlKeyMap(101),
	 modifierKeyMask(0x0),buttonStates(0),
	 keyboardMode(false),
	 numMouseWheelTicks(0),
	 nextEventOrdinal(0),
	 window(0),
	 mouseCursorFaker(0)
	{
	typedef std::vector<std::string> StringList;
	
	/* Allocate new adapter state arrays: */
	numInputDevices=1;
	inputDevices=new InputDevice*[numInputDevices];
	
	/* Retrieve the number of mouse buttons: */
	numButtons=configFileSection.retrieveValue<int>("./numButtons",0);
	
	/* Retrieve button key list: */
	StringList buttonKeyNames=configFileSection.retrieveValue<StringList>("./buttonKeys",StringList());
	numButtonKeys=buttonKeyNames.size();
	if(numButtonKeys>0)
		{
		/* Get key codes for all button keys: */
		buttonKeyCodes=new int[numButtonKeys];
		for(int i=0;i<numButtonKeys;++i)
			buttonKeyCodes[i]=getKeyCode(buttonKeyNames[i]);
		}
	
	/* Retrieve modifier key list: */
	StringList modifierKeyNames=configFileSection.retrieveValue<StringList>("./modifierKeys",StringList());
	numModifierKeys=modifierKeyNames.size();
	if(numModifierKeys>0)
		{
		/* Get key codes for all modifier keys: */
		modifierKeyCodes=new int[numModifierKeys];
		for(int i=0;i<numModifierKeys;++i)
			modifierKeyCodes[i]=getKeyCode(modifierKeyNames[i]);
		}
	
	/* Calculate number of buttons and valuators: */
	numButtonStates=(numButtons+numButtonKeys)*(1<<numModifierKeys);
	int numValuators=1<<numModifierKeys;
	
	/* Create new input device: */
	InputDevice* newDevice=inputDeviceManager->createInputDevice("Mouse",InputDevice::TRACK_POS|InputDevice::TRACK_DIR,numButtonStates,numValuators+4,true);
	newDevice->setDeviceRayDirection(Vector(0,1,0));
	
	/* Store the input device: */
	inputDevices[0]=newDevice;
	
	/* Retrieve the keyboard toggle key code: */
	keyboardModeToggleKeyCode=getKeyCode(configFileSection.retrieveValue<std::string>("./keyboardModeToggleKey","F1"));
	
	/* Create the control key map: */
	const int controlKeyMapSize=sizeof(rawControlKeyMap)/sizeof(ControlKeyMapItem);
	for(int i=0;i<controlKeyMapSize;++i)
		controlKeyMap.setEntry(ControlKeyMap::Entry(rawControlKeyMap[i].ck,rawControlKeyMap[i].tce));
	
	/* Initialize button and valuator states: */
	buttonStates=new bool[numButtonStates];
	for(int i=0;i<numButtonStates;++i)
		buttonStates[i]=false;
	numMouseWheelTicks=new int[numValuators];
	for(int i=0;i<numValuators;++i)
		numMouseWheelTicks[i]=0;
	
	/* Check if this adapter is supposed to draw a fake mouse cursor: */
	if(configFileSection.retrieveValue<bool>("./fakeMouseCursor",false))
		{
		/* Read the cursor file name and nominal size: */
		std::string mouseCursorImageFileName=configFileSection.retrieveString("./mouseCursorImageFileName",DEFAULTMOUSECURSORIMAGEFILENAME);
		unsigned int mouseCursorNominalSize=configFileSection.retrieveValue<unsigned int>("./mouseCursorNominalSize",24);
		
		/* Create the mouse cursor faker: */
		mouseCursorFaker=new MouseCursorFaker(newDevice,mouseCursorImageFileName.c_str(),mouseCursorNominalSize);
		mouseCursorFaker->setCursorSize(configFileSection.retrieveValue<Size>("./mouseCursorSize",mouseCursorFaker->getCursorSize()));
		mouseCursorFaker->setCursorHotspot(configFileSection.retrieveValue<Vector>("./mouseCursorHotspot",mouseCursorFaker->getCursorHotspot()));
		}
	}

InputDeviceAdapterMouse::~InputDeviceAdapterMouse(void)
	{
	delete mouseCursorFaker;
	delete[] buttonKeyCodes;
	delete[] modifierKeyCodes;
	delete[] buttonStates;
	delete[] numMouseWheelTicks;
	}

void InputDeviceAdapterMouse::updateInputDevices(void)
	{
	if(window!=0)
		{
		/* Set mouse device transformation: */
		Ray mouseRay=window->reprojectWindowPos(mousePos);
		Point mousePos=mouseRay.getOrigin();
		Vector mouseY=mouseRay.getDirection();
		Vector mouseX=Geometry::cross(mouseY,window->getVRScreen()->getScreenTransformation().getDirection(1));
		TrackerState::Rotation rot=TrackerState::Rotation::fromBaseVectors(mouseX,mouseY);
		inputDevices[0]->setTransformation(TrackerState(mousePos-Point::origin,rot));
		
		/* Set mouse device button states: */
		for(int i=0;i<numButtonStates;++i)
			inputDevices[0]->setButtonState(i,buttonStates[i]);
		
		/* Set mouse device valuator states: */
		int numValuators=1<<numModifierKeys;
		for(int i=0;i<numValuators;++i)
			{
			double mouseWheelValue=double(numMouseWheelTicks[i])/3.0;
			if(mouseWheelValue<-1.0)
				mouseWheelValue=-1.0;
			else if(mouseWheelValue>1.0)
				mouseWheelValue=1.0;
			inputDevices[0]->setValuator(i,mouseWheelValue);
			numMouseWheelTicks[i]=0;
			}
		
		inputDevices[0]->setValuator(numValuators+0,Scalar(2)*mousePos[0]/window->getVRScreen()->getWidth()-Scalar(1));
		inputDevices[0]->setValuator(numValuators+1,Scalar(2)*mousePos[1]/window->getVRScreen()->getHeight()-Scalar(1));
		inputDevices[0]->setValuator(numValuators+2,0.0);
		inputDevices[0]->setValuator(numValuators+3,0.0);
		}
	
	if(!textEvents.empty()||!textControlEvents.empty())
		{
		/* Process all accumulated text and text control events: */
		std::vector<std::pair<int,GLMotif::TextEvent> >::iterator teIt=textEvents.begin();
		int teOrd=teIt!=textEvents.end()?teIt->first:nextEventOrdinal;
		std::vector<std::pair<int,GLMotif::TextControlEvent> >::iterator tceIt=textControlEvents.begin();
		int tceOrd=tceIt!=textControlEvents.end()?tceIt->first:nextEventOrdinal;
		while(teIt!=textEvents.end()||tceIt!=textControlEvents.end())
			{
			/* Process the next event from either list: */
			if(teOrd<tceOrd)
				{
				getWidgetManager()->text(teIt->second);
				++teIt;
				teOrd=teIt!=textEvents.end()?teIt->first:nextEventOrdinal;
				}
			else
				{
				getWidgetManager()->textControl(tceIt->second);
				++tceIt;
				tceOrd=tceIt!=textControlEvents.end()?tceIt->first:nextEventOrdinal;
				}
			}
		
		/* Clear the event lists: */
		nextEventOrdinal=0;
		textEvents.clear();
		textControlEvents.clear();
		}
	}

void InputDeviceAdapterMouse::setMousePosition(VRWindow* newWindow,const Scalar newMousePos[2])
	{
	/* Set current mouse position: */
	window=newWindow;
	mousePos[0]=newMousePos[0];
	mousePos[1]=newMousePos[1];
	
	requestUpdate();
	}

void InputDeviceAdapterMouse::keyPressed(int keyCode,int modifierMask,const char* string)
	{
	if(keyCode==keyboardModeToggleKeyCode)
		keyboardMode=!keyboardMode;
	else if(keyboardMode)
		{
		/* Process the key event: */
		ControlKeyMap::Iterator ckmIt=controlKeyMap.findEntry(ControlKey(keyCode,modifierMask&(ShiftMask|ControlMask)));
		if(!ckmIt.isFinished())
			{
			/* Store a text control event: */
			textControlEvents.push_back(std::pair<int,GLMotif::TextControlEvent>(nextEventOrdinal,ckmIt->getDest()));
			++nextEventOrdinal;
			}
		else if(string!=0&&string[0]!='\0')
			{
			/* Store a text event: */
			textEvents.push_back(std::pair<int,GLMotif::TextEvent>(nextEventOrdinal,GLMotif::TextEvent(string)));
			++nextEventOrdinal;
			}
		}
	else
		{
		/* Check key code against list of button keys: */
		for(int i=0;i<numButtonKeys;++i)
			if(buttonKeyCodes[i]==keyCode)
				{
				/* Set button state: */
				int stateIndex=(numButtons+numButtonKeys)*modifierKeyMask+numButtons+i;
				buttonStates[stateIndex]=true;
				break;
				}
		
		/* Check key code against list of modifier keys: */
		for(int i=0;i<numModifierKeys;++i)
			if(modifierKeyCodes[i]==keyCode)
				{
				/* Change current modifier mask: */
				changeModifierKeyMask(modifierKeyMask|(1<<i));
				break;
				}
		}
	
	requestUpdate();
	}

void InputDeviceAdapterMouse::keyReleased(int keyCode)
	{
	/* Check key code against list of button keys: */
	for(int i=0;i<numButtonKeys;++i)
		if(buttonKeyCodes[i]==keyCode)
			{
			/* Set button state: */
			int stateIndex=(numButtons+numButtonKeys)*modifierKeyMask+numButtons+i;
			buttonStates[stateIndex]=false;
			break;
			}
	
	/* Check key code against list of modifier keys: */
	for(int i=0;i<numModifierKeys;++i)
		if(modifierKeyCodes[i]==keyCode)
			{
			/* Change current modifier mask: */
			changeModifierKeyMask(modifierKeyMask&~(1<<i));
			break;
			}
	
	requestUpdate();
	}

void InputDeviceAdapterMouse::setButtonState(int buttonIndex,bool newButtonState)
	{
	/* Check if given button is represented: */
	if(buttonIndex>=0&&buttonIndex<numButtons)
		{
		/* Set current button state: */
		int stateIndex=(numButtons+numButtonKeys)*modifierKeyMask+buttonIndex;
		buttonStates[stateIndex]=newButtonState;
		}
	
	requestUpdate();
	}

void InputDeviceAdapterMouse::incMouseWheelTicks(void)
	{
	++numMouseWheelTicks[modifierKeyMask];
	
	requestUpdate();
	}

void InputDeviceAdapterMouse::decMouseWheelTicks(void)
	{
	--numMouseWheelTicks[modifierKeyMask];
	
	requestUpdate();
	}

}
