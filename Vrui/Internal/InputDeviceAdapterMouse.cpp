/***********************************************************************
InputDeviceAdapterMouse - Class to convert mouse and keyboard into a
Vrui input device.
Copyright (c) 2004-2011 Oliver Kreylos

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

#include <stdlib.h>
#include <stdio.h>
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
#include <Vrui/Vrui.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceFeature.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/VRScreen.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRWindow.h>

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

std::string InputDeviceAdapterMouse::getKeyName(int keyCode)
	{
	/* Check for built-in legacy key names first: */
	const int keyMapSize=sizeof(keyMap)/sizeof(KeyMapItem);
	for(int i=0;i<keyMapSize;++i)
		if(keyCode==keyMap[i].keysym)
			return std::string(keyMap[i].name);
	
	/* Check for X11 key names: */
	char* name=XKeysymToString(keyCode);
	if(name!=0)
		return std::string(name);
	
	Misc::throwStdErr("InputDeviceAdapterMouse: Unknown key code %d",keyCode);
	
	/* Never reached; just to make compiler happy: */
	return std::string();
	}

int InputDeviceAdapterMouse::getButtonIndex(int keyCode) const
	{
	for(int i=0;i<numButtonKeys;++i)
		if(buttonKeyCodes[i]==keyCode)
			return i;
	
	return -1;
	}

int InputDeviceAdapterMouse::getModifierIndex(int keyCode) const
	{
	for(int i=0;i<numModifierKeys;++i)
		if(modifierKeyCodes[i]==keyCode)
			return i;
	
	return -1;
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
	 fakeMouseCursor(false)
	{
	typedef std::vector<std::string> StringList;
	
	/* Allocate new adapter state arrays: */
	numInputDevices=1;
	inputDevices=new InputDevice*[numInputDevices];
	inputDevices[0]=0;
	
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
	InputDevice* newDevice=inputDeviceManager->createInputDevice("Mouse",InputDevice::TRACK_POS|InputDevice::TRACK_DIR,numButtonStates,numValuators,true);
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
	fakeMouseCursor=configFileSection.retrieveValue<bool>("./fakeMouseCursor",fakeMouseCursor);
	if(fakeMouseCursor)
		{
		/* Enable the device's glyph as a cursor: */
		Glyph& deviceGlyph=inputDeviceManager->getInputGraphManager()->getInputDeviceGlyph(newDevice);
		deviceGlyph.enable();
		deviceGlyph.setGlyphType(Glyph::CURSOR);
		}
	}

InputDeviceAdapterMouse::~InputDeviceAdapterMouse(void)
	{
	delete[] buttonKeyCodes;
	delete[] modifierKeyCodes;
	delete[] buttonStates;
	delete[] numMouseWheelTicks;
	}

std::string InputDeviceAdapterMouse::getFeatureName(const InputDeviceFeature& feature) const
	{
	std::string result;
	
	/* Calculate the feature's modifier mask: */
	int featureModifierMask=0x0;
	if(feature.isButton())
		featureModifierMask=feature.getIndex()/(numButtons+numButtonKeys);
	if(feature.isValuator())
		featureModifierMask=feature.getIndex();
	
	/* Create the feature's modifier prefix: */
	for(int i=0;i<numModifierKeys;++i)
		if(featureModifierMask&(0x1<<i))
			{
			/* Append the modifier key's name to the prefix: */
			result.append(getKeyName(modifierKeyCodes[i]));
			result.push_back('+');
			}
	
	/* Append the feature's name: */
	if(feature.isButton())
		{
		int buttonIndex=feature.getIndex()%(numButtons+numButtonKeys);
		
		/* Check if the button is a mouse button or a button key: */
		if(buttonIndex<numButtons)
			{
			/* Append a mouse button name: */
			char buttonName[40];
			snprintf(buttonName,sizeof(buttonName),"Mouse%d",buttonIndex+1);
			result.append(buttonName);
			}
		else
			{
			/* Append a button key name: */
			result.append(getKeyName(buttonKeyCodes[buttonIndex-numButtons]));
			}
		}
	if(feature.isValuator())
		result.append("MouseWheel");
	
	return result;
	}

int InputDeviceAdapterMouse::getFeatureIndex(InputDevice* device,const char* featureName) const
	{
	int result=-1;
	
	/* Extract a modifier key mask from the feature name: */
	int featureModifierKeyMask=0x0;
	const char* fPtr=featureName;
	bool matchedPrefix;
	do
		{
		/* Match the current prefix against a modifier key name: */
		matchedPrefix=false;
		for(int i=0;!matchedPrefix&&i<numModifierKeys;++i)
			{
			/* Get the modifier key name: */
			std::string modifierKeyName=getKeyName(modifierKeyCodes[i]);
			
			/* Match against the prefix: */
			const char* mknPtr=modifierKeyName.c_str();
			const char* prefixPtr;
			for(prefixPtr=fPtr;*mknPtr!='\0'&&*prefixPtr==*mknPtr;++prefixPtr,++mknPtr)
				;
			
			/* Check for successful match: */
			if(*mknPtr=='\0'&&*prefixPtr=='+')
				{
				/* Update the modifier key mask: */
				featureModifierKeyMask|=0x1<<i;
				
				/* Skip the prefix and start over: */
				fPtr=prefixPtr+1;
				matchedPrefix=true;
				}
			}
		}
	while(matchedPrefix);
	
	/* Check if the feature suffix matches a mouse feature or a button key: */
	if(strncmp(fPtr,"Mouse",5)==0)
		{
		fPtr+=5;
		
		/* Check if the feature is the mouse wheel or a mouse button: */
		if(strcmp(fPtr,"Wheel")==0)
			{
			/* Return the mouse wheel feature: */
			result=device->getValuatorFeatureIndex(featureModifierKeyMask);
			}
		else
			{
			/* Return a mouse button feature: */
			int buttonIndex=atoi(fPtr)-1;
			result=device->getButtonFeatureIndex((numButtons+numButtonKeys)*featureModifierKeyMask+buttonIndex);
			}
		}
	else
		{
		/* Match the feature suffix against a button key name: */
		for(int i=0;i<numButtonKeys;++i)
			if(getKeyName(buttonKeyCodes[i])==fPtr)
				{
				result=device->getButtonFeatureIndex((numButtons+numButtonKeys)*featureModifierKeyMask+numButtons+i);
				break;
				}
		}
	
	return result;
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
			/* Convert the mouse wheel tick count into a valuator value (ugh): */
			double mouseWheelValue=double(numMouseWheelTicks[i])/3.0;
			if(mouseWheelValue<-1.0)
				mouseWheelValue=-1.0;
			else if(mouseWheelValue>1.0)
				mouseWheelValue=1.0;
			inputDevices[0]->setValuator(i,mouseWheelValue);
			
			/* If there were mouse ticks, request another Vrui frame in a short while because there will be no "no mouse ticks" message: */
			if(numMouseWheelTicks[i]!=0)
				scheduleUpdate(getApplicationTime()+0.1);
			numMouseWheelTicks[i]=0;
			}
		
		#if 0
		inputDevices[0]->setValuator(numValuators+0,Scalar(2)*mousePos[0]/window->getVRScreen()->getWidth()-Scalar(1));
		inputDevices[0]->setValuator(numValuators+1,Scalar(2)*mousePos[1]/window->getVRScreen()->getHeight()-Scalar(1));
		inputDevices[0]->setValuator(numValuators+2,0.0);
		inputDevices[0]->setValuator(numValuators+3,0.0);
		#endif
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
	
	// requestUpdate();
	}

bool InputDeviceAdapterMouse::keyPressed(int keyCode,int modifierMask,const char* string)
	{
	bool stateChanged=false;
	
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
		
		stateChanged=true;
		}
	else
		{
		/* Check if the key is a button key: */
		int buttonIndex=getButtonIndex(keyCode);
		if(buttonIndex>=0)
			{
			/* Set button state: */
			int stateIndex=(numButtons+numButtonKeys)*modifierKeyMask+numButtons+buttonIndex;
			stateChanged=!buttonStates[stateIndex];
			buttonStates[stateIndex]=true;
			}
		
		/* Check if the key is a modifier key: */
		int modifierIndex=getModifierIndex(keyCode);
		if(modifierIndex>=0)
			{
			/* Change current modifier mask: */
			changeModifierKeyMask(modifierKeyMask|(0x1<<modifierIndex));
			stateChanged=true;
			}
		}
	
	// requestUpdate();
	
	return stateChanged;
	}

bool InputDeviceAdapterMouse::keyReleased(int keyCode)
	{
	bool stateChanged=false;
	
	if(!keyboardMode)
		{
		/* Check if the key is a button key: */
		int buttonIndex=getButtonIndex(keyCode);
		if(buttonIndex>=0)
			{
			/* Set button state: */
			int stateIndex=(numButtons+numButtonKeys)*modifierKeyMask+numButtons+buttonIndex;
			stateChanged=buttonStates[stateIndex];
			buttonStates[stateIndex]=false;
			}
		
		/* Check if the key is a modifier key: */
		int modifierIndex=getModifierIndex(keyCode);
		if(modifierIndex>=0)
			{
			/* Change current modifier mask: */
			changeModifierKeyMask(modifierKeyMask&~(0x1<<modifierIndex));
			stateChanged=true;
			}
		
		// requestUpdate();
		}
	
	return stateChanged;
	}

void InputDeviceAdapterMouse::resetKeys(const XKeymapEvent& event)
	{
	/* Calculate the new modifier key mask: */
	int newModifierKeyMask=0x0;
	for(int i=0;i<256;++i)
		if(event.key_vector[i>>3]&(0x1<<(i&0x7)))
			{
			/* Convert the keycode to a keysym: */
			XKeyEvent keyEvent;
			keyEvent.type=KeyPress;
			keyEvent.serial=event.serial;
			keyEvent.send_event=event.send_event;
			keyEvent.display=event.display;
			keyEvent.window=event.window;
			keyEvent.state=0x0;
			keyEvent.keycode=i;
			KeySym keyCode=XLookupKeysym(&keyEvent,0);
			
			int modifierIndex=getModifierIndex(keyCode);
			if(modifierIndex>=0)
				newModifierKeyMask|=0x1<<modifierIndex;
			}
	
	/* Set the new modifier key mask: */
	changeModifierKeyMask(newModifierKeyMask);
	
	/* Set the states of all button keys: */
	for(int i=0;i<numButtonKeys;++i)
		{
		int stateIndex=(numButtons+numButtonKeys)*modifierKeyMask+numButtons+i;
		buttonStates[stateIndex]=false;
		}
	for(int i=0;i<256;++i)
		if(event.key_vector[i>>3]&(0x1<<(i&0x7)))
			{
			/* Convert the keycode to a keysym: */
			XKeyEvent keyEvent;
			keyEvent.type=KeyPress;
			keyEvent.serial=event.serial;
			keyEvent.send_event=event.send_event;
			keyEvent.display=event.display;
			keyEvent.window=event.window;
			keyEvent.state=0x0;
			keyEvent.keycode=i;
			KeySym keyCode=XLookupKeysym(&keyEvent,0);
			
			int buttonIndex=getButtonIndex(keyCode);
			if(buttonIndex>=0)
				{
				int stateIndex=(numButtons+numButtonKeys)*modifierKeyMask+numButtons+buttonIndex;
				buttonStates[stateIndex]=true;
				}
			}
	
	// requestUpdate();
	}

bool InputDeviceAdapterMouse::setButtonState(int buttonIndex,bool newButtonState)
	{
	bool stateChanged=false;
	
	/* Check if given button is represented: */
	if(buttonIndex>=0&&buttonIndex<numButtons)
		{
		/* Set current button state: */
		int stateIndex=(numButtons+numButtonKeys)*modifierKeyMask+buttonIndex;
		stateChanged=buttonStates[stateIndex]!=newButtonState;
		buttonStates[stateIndex]=newButtonState;
		
		// requestUpdate();
		}
	
	return stateChanged;
	}

void InputDeviceAdapterMouse::incMouseWheelTicks(void)
	{
	++numMouseWheelTicks[modifierKeyMask];
	
	// requestUpdate();
	}

void InputDeviceAdapterMouse::decMouseWheelTicks(void)
	{
	--numMouseWheelTicks[modifierKeyMask];
	
	// requestUpdate();
	}

ONTransform getMouseScreenTransform(InputDeviceAdapterMouse* mouseAdapter,Scalar viewport[4])
	{
	/* Check if the mouse adapter is valid: */
	VRScreen* screen=0;
	if(mouseAdapter!=0&&mouseAdapter->getWindow()!=0)
		{
		/* Use the window associated with the mouse adapter: */
		VRWindow* window=mouseAdapter->getWindow();
		screen=window->getVRScreen();
		window->getScreenViewport(viewport);
		}
	else
		{
		/* Use the main screen: */
		screen=getMainScreen();
		screen->getViewport(viewport);
		}
	
	/* Return the screen's transformation: */
	return screen->getScreenTransformation();
	}

}
