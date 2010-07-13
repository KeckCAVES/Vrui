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

#ifndef VRUI_INTERNAL_INPUTDEVICEADAPTERMOUSE_INCLUDED
#define VRUI_INTERNAL_INPUTDEVICEADAPTERMOUSE_INCLUDED

#include <string>
#include <utility>
#include <vector>
#include <Misc/HashTable.h>
#include <GLMotif/TextEvent.h>
#include <GLMotif/TextControlEvent.h>
#include <Vrui/Geometry.h>
#include <Vrui/Internal/InputDeviceAdapter.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
namespace Vrui {
class MouseCursorFaker;
class VRWindow;
}

namespace Vrui {

class InputDeviceAdapterMouse:public InputDeviceAdapter
	{
	/* Embedded classes: */
	public:
	struct ControlKey // Structure to map key codes to text control events
		{
		/* Elements: */
		public:
		int keyCode; // Key code
		int modifierMask; // Keyboard modifier mask
		
		/* Constructors and destructors: */
		ControlKey(int sKeyCode,int sModifierMask)
			:keyCode(sKeyCode),modifierMask(sModifierMask)
			{
			}
		
		/* Methods: */
		friend bool operator!=(const ControlKey& ck1,const ControlKey& ck2)
			{
			return ck1.keyCode!=ck2.keyCode||ck1.modifierMask!=ck2.modifierMask;
			}
		static size_t hash(const ControlKey& source,size_t tableSize)
			{
			return (source.keyCode+(source.modifierMask<<24))%tableSize;
			}
		};
	
	private:
	typedef Misc::HashTable<ControlKey,GLMotif::TextControlEvent,ControlKey> ControlKeyMap; // Type for hash tables mapping control keys to GLMotif text control events
	
	/* Elements: */
	private:
	int numButtons; // Number of mapped mouse buttons
	int numButtonKeys; // Number of keys treated as mouse buttons
	int* buttonKeyCodes; // Map from key codes to button key indices
	int numModifierKeys; // Number of used modifier keys
	int* modifierKeyCodes; // Map from key codes to modifier key indices
	int numButtonStates; // Number of button states (number of buttons times number of modifier key states)
	int keyboardModeToggleKeyCode; // Key code which switches keyboard between button and key mode
	ControlKeyMap controlKeyMap; // Map from key codes and modifier states to GLMotif text control events
	int modifierKeyMask; // Current modifier key mask
	bool* buttonStates; // Array of current button states
	bool keyboardMode; // Flag whether the keyboard is in key mode
	int* numMouseWheelTicks; // Number of mouse wheel ticks for each modifier key mask accumulated during frame processing
	int nextEventOrdinal; // Ordering index for next accumulated text or text control event
	std::vector<std::pair<int,GLMotif::TextEvent> > textEvents; // List of text events accumulated during frame processing in keyboard mode
	std::vector<std::pair<int,GLMotif::TextControlEvent> > textControlEvents; // List of text control events accumulated during frame processing in keyboard mode
	VRWindow* window; // VR window containing the last reported mouse position
	Scalar mousePos[2]; // Current mouse position in screen coordinates
	MouseCursorFaker* mouseCursorFaker; // Pointer to object to render a fake mouse cursor
	
	/* Private methods: */
	static int getKeyCode(std::string keyName); // Returns the key code for a named key
	void changeModifierKeyMask(int newModifierKeyMask); // Called whenever the current modifier key mask changes
	
	/* Constructors and destructors: */
	public:
	InputDeviceAdapterMouse(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection);
	virtual ~InputDeviceAdapterMouse(void);
	
	/* Methods: */
	virtual void updateInputDevices(void);
	
	/* New methods: */
	VRWindow* getWindow(void) const // Returns the window containing the last reported mouse position
		{
		return window;
		}
	const Scalar* getMousePosition(void) const // Returns the current mouse position in normalized device coordinates
		{
		return mousePos;
		}
	void setMousePosition(VRWindow* newWindow,const Scalar newMousePos[2]); // Sets current mouse position in screen coordinates
	void keyPressed(int keyCode,int modifierMask,const char* string); // Notifies adapter that a key has been pressed
	void keyReleased(int keyCode); // Notifies adapter that a key has been released
	void setButtonState(int buttonIndex,bool newButtonState); // Sets current button state
	void incMouseWheelTicks(void); // Increases the number of mouse wheel ticks
	void decMouseWheelTicks(void); // Decreases the number of mouse wheel ticks
	};

}

#endif
