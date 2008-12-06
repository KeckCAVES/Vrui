/***********************************************************************
ToolInputLayout - Class to represent the input requirements of tools.
Copyright (c) 2004-2005 Oliver Kreylos

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

#include <Vrui/ToolInputLayout.h>

namespace Vrui {

/********************************
Methods of class ToolInputLayout:
********************************/

ToolInputLayout::ToolInputLayout(void)
	:numDevices(0),
	 numButtons(0),buttonCascadable(0),
	 numValuators(0),valuatorCascadable(0)
	{
	}

ToolInputLayout::ToolInputLayout(int sNumDevices,const int sNumButtons[],const int sNumValuators[])
	:numDevices(sNumDevices),
	 numButtons(new int[numDevices]),buttonCascadable(new bool*[numDevices]),
	 numValuators(new int[numDevices]),valuatorCascadable(new bool*[numDevices])
	{
	/* Reset cascadable arrays: */
	for(int i=0;i<numDevices;++i)
		{
		buttonCascadable[i]=0;
		valuatorCascadable[i]=0;
		}
	
	/* Copy numbers of buttons and valuators and initialize cascadable flags: */
	for(int i=0;i<numDevices;++i)
		{
		numButtons[i]=sNumButtons[i];
		buttonCascadable[i]=new bool[numButtons[i]];
		for(int j=0;j<numButtons[i];++j)
			buttonCascadable[i][j]=false;
		numValuators[i]=sNumValuators[i];
		valuatorCascadable[i]=new bool[numValuators[i]];
		for(int j=0;j<numValuators[i];++j)
			valuatorCascadable[i][j]=false;
		}
	}

ToolInputLayout::~ToolInputLayout(void)
	{
	delete[] numButtons;
	if(buttonCascadable!=0)
		{
		for(int i=0;i<numDevices;++i)
			delete[] buttonCascadable[i];
		delete[] buttonCascadable;
		}
	delete[] numValuators;
	if(valuatorCascadable!=0)
		{
		for(int i=0;i<numDevices;++i)
			delete[] valuatorCascadable[i];
		delete[] valuatorCascadable;
		}
	}

void ToolInputLayout::setNumDevices(int newNumDevices)
	{
	/* Check if number of devices has changed: */
	if(newNumDevices!=numDevices)
		{
		/* Delete old arrays: */
		delete[] numButtons;
		if(buttonCascadable!=0)
			{
			for(int i=0;i<numDevices;++i)
				delete[] buttonCascadable[i];
			delete[] buttonCascadable;
			}
		delete[] numValuators;
		if(valuatorCascadable!=0)
			{
			for(int i=0;i<numDevices;++i)
				delete[] valuatorCascadable[i];
			delete[] valuatorCascadable;
			}
		
		/* Change number of devices: */
		numDevices=newNumDevices;
		
		/* Allocate and initialize new arrays: */
		numButtons=new int[numDevices];
		buttonCascadable=new bool*[numDevices];
		numValuators=new int[numDevices];
		valuatorCascadable=new bool*[numDevices];
		for(int i=0;i<numDevices;++i)
			{
			numButtons[i]=0;
			buttonCascadable[i]=0;
			numValuators[i]=0;
			valuatorCascadable[i]=0;
			}
		}
	}

void ToolInputLayout::setNumButtons(int deviceIndex,int newNumButtons)
	{
	if(numButtons[deviceIndex]!=newNumButtons)
		{
		/* Delete old cascadable array: */
		delete[] buttonCascadable[deviceIndex];
		
		/* Set new number of buttons: */
		numButtons[deviceIndex]=newNumButtons;
		
		/* Allocate new cascadable array: */
		buttonCascadable[deviceIndex]=new bool[numButtons[deviceIndex]];
		}
	
	/* Reset all cascadable flags: */
	for(int j=0;j<numButtons[deviceIndex];++j)
		buttonCascadable[deviceIndex][j]=false;
	}

void ToolInputLayout::setButtonCascadable(int deviceIndex,int buttonIndex,bool newCascadable)
	{
	buttonCascadable[deviceIndex][buttonIndex]=newCascadable;
	}

void ToolInputLayout::setNumValuators(int deviceIndex,int newNumValuators)
	{
	if(numValuators[deviceIndex]!=newNumValuators)
		{
		/* Delete old cascadable array: */
		delete[] valuatorCascadable[deviceIndex];
		
		/* Set new number of valuators: */
		numValuators[deviceIndex]=newNumValuators;
		
		/* Allocate new cascadable array: */
		valuatorCascadable[deviceIndex]=new bool[numValuators[deviceIndex]];
		}
	
	/* Reset all cascadable flags: */
	for(int j=0;j<numValuators[deviceIndex];++j)
		valuatorCascadable[deviceIndex][j]=false;
	}

void ToolInputLayout::setValuatorCascadable(int deviceIndex,int valuatorIndex,bool newCascadable)
	{
	valuatorCascadable[deviceIndex][valuatorIndex]=newCascadable;
	}

}
