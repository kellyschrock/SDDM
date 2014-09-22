/*
 *  Copyright (c) 2008 Kelly Schrock
 *
 *  This file is part of SDDM.
 *
 *  SDDM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SDDM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with SDDM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>

#include "model.h"

using namespace std;

SceneManager * SceneManager::instance = new SceneManager();
const int SceneManager::NUM_SCENES = 16;

SceneSetting::SceneSetting()
: level(0)
, pan(0)
, pitch(0)
, mute(false)
, solo(false)
, instrument(0)
{
}

SceneSetting::~SceneSetting() {}

void SceneSetting::apply()
{
	if(instrument)
	{
		Instrument * inst = instrument;
		
		inst->setLevel(this->level);
		inst->setPan(this->pan);
		inst->setPitch(this->pitch);
		inst->setMuted(this->mute);
		inst->setSoloed(this->solo);
	}
}

void SceneSetting::update()
{
	if(instrument)
	{
		Instrument * i = instrument;
		
		this->level = i->getLevel();
		this->pan = i->getPan();
		this->pitch = i->getPitch();
		this->mute = i->isMuted();
		this->solo = i->isSoloed();
	}
}

void SceneSetting::copyFrom(SceneSetting & other)
{
	level = other.level;
	pan = other.pan;
	pitch = other.pitch;
	mute = other.mute;
	solo = other.solo;
}

void SceneSetting::reset()
{
	level = pan = pitch = 0;
	mute = solo = false;
}

//
// Scene
//
Scene * Scene::fromDrumkitSettings(Drumkit * kit)
{
	if(!kit)
		return 0;
		
 	Scene * scene = new Scene();
		
	return scene->updateFrom(kit);
}

Scene::Scene()
: modified(false)
, active(false)
, name("")
{
}

Scene * Scene::updateFrom(Drumkit * kit)
{
	InstrumentList inst = kit->allInstruments();
	for(InstrumentList::iterator e = inst.begin(); e != inst.end(); ++e)
	{
		SceneSetting * setting = new SceneSetting();
		setting->setInstrument(*e);
		setting->update();
		this->add(setting);
	}
	
	return this;
}

void Scene::reset()
{
	for(SceneSettingList::iterator e = settings.begin(); e != settings.end(); ++e)
		(*e)->reset();
}

void Scene::add(SceneSetting * setting)
{
	settings.push_back(setting);
}

bool Scene::hasSettings() 
{
	return settings.size() > 0;
}

void Scene::copyFrom(Scene & other)
{
	for(unsigned i = 0; i < settings.size(); ++i)
	{
		if(i < other.settings.size())
			settings[i]->copyFrom(*(other.settings[i]));
	}
}

Scene::~Scene()
{
	for(SceneSettingList::iterator e = settings.begin(); e != settings.end(); ++e)
		delete *e;
}

//
// SceneManager
//
SceneManager::SceneManager()
{
}

SceneManager::~SceneManager()
{
	for(SceneList::iterator e = scenes.begin(); e != scenes.end(); ++e)
		delete *e;
}

void SceneManager::activate(Scene *scene)
{
// 	cout << "SceneManager::activate(): scene=" << scene << endl;
	
	this->activeScene = 0;
	
	if(scene && scene->hasSettings())
	{
		for(SceneList::iterator e = scenes.begin(); e != scenes.end(); ++e)
		{
			if(scene == *e)
			{
				this->activeScene = *e;
				break;
			}
		}
	}
	
	if(this->activeScene)
	{
		SceneSettingList list = activeScene->getSettings();
		
		for(SceneSettingList::iterator e = list.begin(); e != list.end(); ++e)
			(*e)->apply();
			
		for(SceneManagerListenerList::iterator e = listeners.begin(); e != listeners.end(); ++e)
			(*e)->sceneActivated(scene);
	}
}

void SceneManager::remove(Scene *scene)
{
	for(unsigned i = 0; i < scenes.size(); ++i)
	{
		if(scenes[i] == scene)
		{
			scenes.erase(scenes.begin() + i);
			break;
		}
	}
	
	delete scene;
}

Scene * SceneManager::addOrUpdate(Scene * scene)
{
	bool found = false;
	
	for(SceneList::iterator e = scenes.begin(); e != scenes.end(); ++e)
	{
		if(*e == scene)
		{
			found = true;
			break;
		}	
	}
	
	if(!found)
		scenes.push_back(scene);
		
	return scene;
}

void SceneManager::sceneModified()
{
// 	cout << "SceneManager::sceneModified(): activeScene=" << activeScene << endl;
	
	if(activeScene)
	{
		for(SceneManagerListenerList::iterator e = listeners.begin(); e != listeners.end(); ++e)
			(*e)->sceneModified(activeScene);
	}
}

void SceneManager::loadScenes(SceneList & toLoad)
{
	for(SceneList::iterator e = scenes.begin(); e != scenes.end(); ++e)
		delete *e;
		
	this->scenes.clear();		
		
	for(SceneList::iterator e = toLoad.begin(); e != toLoad.end(); ++e)
		this->scenes.push_back(*e);
		
	// Notify the listeners that a set of scenes has been loaded.
	for(SceneManagerListenerList::iterator e = listeners.begin(); e != listeners.end(); ++e)
		(*e)->sceneListLoaded(this->scenes);
}

Scene * SceneManager::findSceneByName(string & name)
{
	for(SceneList::iterator e = scenes.begin(); e != scenes.end(); ++e)
	{
		if((*e)->getName() == name)
			return *e;
	}
	
	return 0;
}

// eof
