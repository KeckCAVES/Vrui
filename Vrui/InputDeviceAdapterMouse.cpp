/***********************************************************************
InputDeviceAdapterMouse - Class to convert mouse and keyboard into a
Vrui input device.
Copyright (c) 2004-2006 Oliver Kreylos

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
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/VRScreen.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRWindow.h>
#include <Vrui/Vrui.h>

#include <Vrui/InputDeviceAdapterMouse.h>

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
	{"LeftShift",XK_Shift_L},{"RightShift",XK_Shift_R},{"LeftCtrl",XK_Control_L},{"RightCtrl",XK_Control_R},
	{"LeftAlt",XK_Alt_L},{"RightAlt",XK_Alt_R},{"LeftMeta",XK_Meta_L},{"RightMeta",XK_Meta_R},
	{"LeftSuper",XK_Super_L},{"RightSuper",XK_Super_R},{"LeftHyper",XK_Hyper_L},{"RightHyper",XK_Hyper_R}
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
	 buttonStates(0),
	 window(0)
	{
	typedef std::vector<std::string> StringList;
	
	/* Allocate new adapter state arrays: */
	numInputDevices=1;
	inputDevices=new InputDevice*[numInputDevices];

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
	
	/* Calculate number of buttons: */
	numButtons=configFileSection.retrieveValue<int>("./numButtons",0);
	numButtonStates=(numButtons+numButtonKeys)*(1<<numModifierKeys);
	
	/* Create new input device: */
	InputDevice* newDevice=inputDeviceManager->createInputDevice("Mouse",InputDevice::TRACK_POS|InputDevice::TRACK_DIR,numButtonStates,5,true);
	newDevice->setDeviceRayDirection(Vector(0,1,0));
	
	/* Store the input device: */
	inputDevices[0]=newDevice;
	
	/* Initialize button states: */
	modifierKeyMask=0x0;
	buttonStates=new bool[numButtonStates];
	for(int i=0;i<numButtonStates;++i)
		buttonStates[i]=false;
	numMouseWheelTicks=0;
	}

InputDeviceAdapterMouse::~InputDeviceAdapterMouse(void)
	{
	delete[] buttonKeyCodes;
	delete[] modifierKeyCodes;
	delete[] buttonStates;
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
		double mouseWheelValue=double(numMouseWheelTicks)/3.0;
		if(mouseWheelValue<-1.0)
			mouseWheelValue=-1.0;
		else if(mouseWheelValue>1.0)
			mouseWheelValue=1.0;
		inputDevices[0]->setValuator(0,mouseWheelValue);
		numMouseWheelTicks=0;
		
		inputDevices[0]->setValuator(1,Scalar(2)*mousePos[0]/window->getVRScreen()->getWidth()-Scalar(1));
		inputDevices[0]->setValuator(2,Scalar(2)*mousePos[1]/window->getVRScreen()->getHeight()-Scalar(1));
		inputDevices[0]->setValuator(3,0.0);
		inputDevices[0]->setValuator(4,0.0);
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

void InputDeviceAdapterMouse::keyPressed(int keyCode)
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
	++numMouseWheelTicks;
	
	requestUpdate();
	}

void InputDeviceAdapterMouse::decMouseWheelTicks(void)
	{
	--numMouseWheelTicks;
	
	requestUpdate();
	}

}
