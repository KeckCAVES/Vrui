/***********************************************************************
VisletManager - Class to manage vislet classes.
Copyright (c) 2006-2007 Oliver Kreylos

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

#include <vector>
#include <Misc/ConfigurationFile.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <GLMotif/Popup.h>
#include <GLMotif/SubMenu.h>
#include <GLMotif/ToggleButton.h>
#include <Vrui/Vrui.h>

#include <Vrui/VisletManager.h>

namespace Vrui {

/******************************
Methods of class VisletManager:
******************************/

void VisletManager::visletMenuToggleButtonCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	/* Get the toggle button's index in its container: */
	GLMotif::SubMenu* visletMenu=dynamic_cast<GLMotif::SubMenu*>(cbData->toggle->getParent());
	
	if(visletMenu!=0)
		{
		int toggleIndex=visletMenu->getChildIndex(cbData->toggle);
		if(toggleIndex>=0&&toggleIndex<int(vislets.size()))
			{
			/* Activate or deactivate the vislet: */
			if(cbData->set)
				vislets[toggleIndex]->enable();
			else
				vislets[toggleIndex]->disable();
			}
		}
	}

VisletManager::VisletManager(const Misc::ConfigurationFileSection& sConfigFileSection)
	:Plugins::FactoryManager<VisletFactory>(sConfigFileSection.retrieveString("./visletDsoNameTemplate",SYSVISLETDSONAMETEMPLATE)),
	 configFileSection(sConfigFileSection)
	{
	typedef std::vector<std::string> StringList;
	
	/* Get additional search paths from configuration file section and add them to the factory manager: */
	StringList visletSearchPaths=configFileSection.retrieveValue<StringList>("./visletSearchPaths",StringList());
	for(StringList::const_iterator vspIt=visletSearchPaths.begin();vspIt!=visletSearchPaths.end();++vspIt)
		{
		/* Add the path: */
		getDsoLocator().addPath(*vspIt);
		}
	}

VisletManager::~VisletManager(void)
	{
	/* Destroy all loaded vislets: */
	for(VisletList::iterator vIt=vislets.begin();vIt!=vislets.end();++vIt)
		{
		/* Delete the vislet: */
		(*vIt)->getFactory()->destroyVislet(*vIt);
		}
	}

Misc::ConfigurationFileSection VisletManager::getVisletClassSection(const char* visletClassName) const
	{
	/* Return the section of the same name under the vislet manager's section: */
	return configFileSection.getSection(visletClassName);
	}

Vislet* VisletManager::createVislet(VisletFactory* factory,int numVisletArguments,const char* const visletArguments[])
	{
	/* Create vislet of given class: */
	Vislet* newVislet=factory->createVislet(numVisletArguments,visletArguments);
	
	/* Add the vislet to the list: */
	vislets.push_back(newVislet);
	
	return newVislet;
	}

GLMotif::Popup* VisletManager::buildVisletMenu(void)
	{
	/* Create the popup and menu: */
	GLMotif::Popup* visletMenuPopup=new GLMotif::Popup("VisletsMenuPopup",getWidgetManager());
	GLMotif::SubMenu* visletMenu=new GLMotif::SubMenu("Vislets",visletMenuPopup,false);
	
	/* Create a toggle button for each vislet: */
	for(unsigned int i=0;i<vislets.size();++i)
		{
		char toggleButtonName[40];
		snprintf(toggleButtonName,sizeof(toggleButtonName),"Vislet%u",i);
		GLMotif::ToggleButton* toggleButton=new GLMotif::ToggleButton(toggleButtonName,visletMenu,vislets[i]->getFactory()->getClassName());
		toggleButton->setToggle(vislets[i]->isActive());
		toggleButton->getValueChangedCallbacks().add(this,&VisletManager::visletMenuToggleButtonCallback);
		}
	
	/* Finalize and return the vislet menu popup: */
	visletMenu->manageChild();
	return visletMenuPopup;
	}

void VisletManager::frame(void)
	{
	/* Call all vislet's frame functions: */
	for(VisletList::iterator vIt=vislets.begin();vIt!=vislets.end();++vIt)
		if((*vIt)->isActive())
			(*vIt)->frame();
	}

void VisletManager::display(GLContextData& contextData) const
	{
	/* Call all vislet's display functions: */
	for(VisletList::const_iterator vIt=vislets.begin();vIt!=vislets.end();++vIt)
		if((*vIt)->isActive())
			(*vIt)->display(contextData);
	}

void VisletManager::sound(ALContextData& contextData) const
	{
	/* Call all vislet's sound functions: */
	for(VisletList::const_iterator vIt=vislets.begin();vIt!=vislets.end();++vIt)
		if((*vIt)->isActive())
			(*vIt)->sound(contextData);
	}

}
