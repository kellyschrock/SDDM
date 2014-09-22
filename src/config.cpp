// SDDM configuration
/*
 *  Copyright (c) 2008, 2013 Kelly Schrock, John Hammen
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

#include <map>
#include <vector>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cstdlib>

#include <sndfile.h>
#include <jack/jack.h>

#define TIXML_USE_STL 1
#include "myxml.h"

#include "log.h"

using namespace std;

#include "model.h"
#include "config.h"

static string getPath(string& filename)
{
	string::size_type last = filename.rfind("/");
	return (string::npos != last)?
					filename.substr(0, last):
					filename;
}

static string getPath(const char* filename)
{
	string f(filename);
	return getPath(f);
}

static int toInt(const string & str, int /*defValue*/)
{
	int i = atoi(str.c_str());
	return i;
}

static bool toBool(const string & str, bool defValue)
{
	if(str == "false")
		return false;
	if(str == "true")
		return true;
	return defValue;
}

static string fullPath(string& path, string& filename)
{
	string full(path);

	if(*(path.end()-1) != '/')
					full += "/";

	full += filename;

	return full;
}

static bool isAbsolutePath(string& filename)
{
	return (filename[0] == '/' || filename[1] == ':'); // Whoa, cross-platform!
}

static string toString(int value)
{
	char buf[16] = "";
	sprintf(buf, "%d", value);
	return string(buf);
}

static string toString(bool value)
{
	return value? string("true"):string("false");
}

// C++ strings suck.
static string toString(unsigned value)
{
	char buf[16] = "";
	sprintf(buf, "%d", value);
	return string(buf);
}

//
//---------------InstrumentInfo
//
InstrumentInfo::~InstrumentInfo()
{
	for(LayerInfoList::iterator e = layers.begin(); e != layers.end(); ++e)
		delete *e;
}

// Convert an InstrumentInfo to an Instrument.
Instrument *InstrumentInfo::toInstrument(int maxSamples, IFileLoadProgressListener * listener)
{
	LogPtr log = LogFactory::getLog(__FILE__);

	Instrument *inst = new Instrument(name.c_str());
	unsigned instLevel = (unsigned)atoi(level.c_str());
	if(instLevel <= 0)
		instLevel = 100;
		
	short instPan = 0;
		
	if(pan.length() > 0)
		instPan = (short)atoi(pan.c_str());
		
	inst->setLevel(instLevel);
	inst->setPan(instPan);
	inst->setSubmixName(submix);
	
	int instPitch = atoi(pitch.c_str());
	inst->setPitch(instPitch);
	
	for(InstrumentInfo::LayerInfoList::iterator e = layers.begin(); e != layers.end(); ++e)
	{
		LayerInfo *layerInfo = *e;
		string wave = layerInfo->wave;
		unsigned lo = (unsigned)atoi(layerInfo->lo.c_str());
		unsigned hi = (unsigned)atoi(layerInfo->hi.c_str());
		
		if(listener)
		{
			if(!listener->progressEvent(wave))
				break;
		}
		
	  LOG_TRACE(log, "assign " << wave << " to velocity range " << lo << "-" << hi);
		Sample *sample = Sample::load(wave, maxSamples);
		if(sample)
		{
			InstrumentLayer *layer = new InstrumentLayer(sample, lo, hi);
			inst->add(layer);
			//sampleMap[sample->getFilename()] = sample;
		}
	}

	return inst;
}

InstrumentInfo* InstrumentInfo::from(Instrument* inst)
{
	InstrumentInfo* info = new InstrumentInfo();
	
	info->name = inst->getName();
	info->noteNumber = toString(inst->getNoteNumber());
	info->level = toString(inst->getLevel());
	info->pan = toString(inst->getPan());
	info->pitch = toString(inst->getPitch());
	info->submix = inst->getSubmixName();
	
	InstrumentLayerList layers = inst->getLayers();
	for(InstrumentLayerList::iterator e = layers.begin(); e != layers.end(); ++e)
	{
		InstrumentLayer *layer = *(e);
		if(layer)
			info->layers.push_back(LayerInfo::from(layer));
	}
		
	return info;
}

LayerInfo* LayerInfo::from(InstrumentLayer *layer)
{
	LayerInfo *li = new LayerInfo();
	
	li->hi = toString(layer->getVelocityHI());
	li->lo = toString(layer->getVelocityLO());
	li->wave = layer->getSample()->getFilename();
	
	return li;
}

//
//----------------Configuration
//
Configuration::Configuration(Configuration::SubmixNameList includedSubs)
: includedSubmixes(includedSubs)
{}

bool Configuration::save(const char *filename, Drumkit *dk)
{
	kit.name = dk->getName();
	
	InstrumentList instruments = dk->allInstruments();
	for(InstrumentList::iterator e = instruments.begin(); e != instruments.end(); ++e)
		kit.instruments.push_back(InstrumentInfo::from(*e));
		
	for(SceneList::iterator e = dk->getScenes().begin(); e != dk->getScenes().end(); ++e)
		kit.scenes.push_back(SceneInfo::from(*e));
		
	return save(filename);
}

bool Configuration::save(const char *filename)
{
	XMLDocument::Element *top = new XMLDocument::Element("sddm");
//	XMLDocument::Element *ports = new XMLDocument::Element("ports");
	
//    top->add(ports);

//	for(Configuration::PortList::iterator e = portNames.begin(); e != portNames.end(); ++e)
//	{
//		XMLDocument::Element *port = new XMLDocument::Element("port");
//		port->addAttribute("name", (*(e)));
//		ports->add(port);
//	}
	
	XMLDocument::Element *drumkit = new XMLDocument::Element("drumkit");
	drumkit->addAttribute("name", kit.name);
	top->add(drumkit);
	
	XMLDocument::Element *instruments = new XMLDocument::Element("instruments");
	drumkit->add(instruments);
	
	// Loop through the instruments and write them out
	for(KitInfo::InstrumentInfoList::iterator e = kit.instruments.begin(); e != kit.instruments.end(); ++e)
	{
		InstrumentInfo *info = *e;
		
		XMLDocument::Element *inst = new XMLDocument::Element("instrument");
		instruments->add(inst);

		inst->addAttribute("noteNumber", info->noteNumber);
		inst->addAttribute("name", info->name);
		inst->addAttribute("level", info->level);
		inst->addAttribute("pan", info->pan);
		inst->addAttribute("pitch", info->pitch);
		
		if(!info->submix.empty())
			inst->addAttribute("submix", info->submix);

		if(!info->victims.empty())
		{
			XMLDocument::Element* victims = new XMLDocument::Element("victims");
			inst->add(victims);
			
			for(InstrumentInfo::VictimList::iterator e = info->victims.begin(); e != info->victims.end(); ++e)
			{
				XMLDocument::Element* vic = new XMLDocument::Element("victim");
				vic->addAttribute("noteNumber", (*(e)));
				victims->add(vic);
			}
		}
		
		XMLDocument::Element* layers = new XMLDocument::Element("layers");
		inst->add(layers);
		
		for(InstrumentInfo::LayerInfoList::iterator f = info->layers.begin(); f != info->layers.end(); ++f)
		{
			LayerInfo *layer = *f;

			XMLDocument::Element *li = new XMLDocument::Element("layer");
			layers->add(li);

			li->addAttribute("velLo", layer->lo);
			li->addAttribute("velHi", layer->hi);
			li->addAttribute("wave", layer->wave);
		}
	}
	
//	XMLDocument::Element * scenes = new XMLDocument::Element("scenes");
//	top->add(scenes);
	
//	for(KitInfo::SceneInfoList::iterator e = kit.scenes.begin(); e != kit.scenes.end(); ++e)
//	{
//		SceneInfo * sceneInfo = *e;
//		XMLDocument::Element * el = new XMLDocument::Element("scene");
//		el->addAttribute("name", sceneInfo->name);
			
//		for(SceneInfo::SettingInfoList::iterator f = sceneInfo->settings.begin(); f != sceneInfo->settings.end(); ++f)
//		{
//			SceneSettingInfo * si = *f;
			
//			XMLDocument::Element * se = new XMLDocument::Element("setting");
//			se->addAttribute("level", toString(si->level));
//			se->addAttribute("pan", toString(si->pan));
//			se->addAttribute("pitch", toString(si->pitch));
//			se->addAttribute("mute", toString(si->mute));
//			se->addAttribute("solo", toString(si->solo));
			
//			el->add(se);
//		}
		
//		scenes->add(el);
//	}
	
	XMLDocument doc;
	doc.setTopElement(top);
	
	XMLStringWriter* writer = new XMLFileWriter(filename);
	bool ok = doc.write(writer);
	
	delete writer;
	
	return ok;
}

bool Configuration::load(
	const char *filename, Drumkit *drumkit
, bool ignorePorts, int maxSamples, IFileLoadProgressListener * listener)
{
	if(!load(filename, listener))
		return false;

	drumkit->setName(kit.name);
	drumkit->setLevel(kit.level);
	drumkit->setSelectedSceneName(kit.selectedScene);
	
	// Filter out those instruments not in any of the included submixes
	if(!includedSubmixes.empty())
	{
		KitInfo::InstrumentInfoList list;
		
		for(KitInfo::InstrumentInfoList::iterator e = kit.instruments.begin(); e != kit.instruments.end(); ++e)
		{
			InstrumentInfo* info = *e;
			
			string mainName = string("[main]");
			
			bool include = false;
			
			for(SubmixNameList::iterator f = includedSubmixes.begin(); f != includedSubmixes.end(); ++f)
			{
				string name = *f;
				
				// If this instrument isn't in a submix, and "[main]" is one of the 
				// specified submix names, it needs to be included.
				if(info->submix.empty())
				{
					if(name == mainName)
						include = true;
				}
				else
				{
					// Is this instrument in one of the specified submixes?
					if(info->submix == name)
						include = true;
				}
			}
			
			if(include)
				list.push_back(info);
		}
		
		kit.instruments = list;
	}

	for(KitInfo::InstrumentInfoList::iterator e = kit.instruments.begin(); e != kit.instruments.end(); ++e)
	{
		InstrumentInfo *info = *e;
		
		Instrument* inst = info->toInstrument(maxSamples);
		if(inst)
			drumkit->add((unsigned)atoi(info->noteNumber.c_str()), inst);
			
		if(listener)
		{
			string msg = inst->getName();
			if(!listener->progressEvent(msg))
				return false;
		}
			
		if(ignorePorts)
			inst->removeFromSubmix();
			
		if(inst->isInSubmix())
		{
			string name = inst->getSubmixName();
			Submix* submix = drumkit->findSubmixByName(name);
			if(!submix) {
				submix = drumkit->addSubmix(name);
			}
			inst->setSubmix(submix);
			submix->setOrphaned(false);
		}
	}
	
	// Having added all of the instruments, now set up the victims
	for(KitInfo::InstrumentInfoList::iterator e = kit.instruments.begin(); e != kit.instruments.end(); ++e)
	{
		InstrumentInfo* info = *e;
		unsigned nn = (unsigned)atoi(info->noteNumber.c_str());
		Instrument* me = drumkit->findByNoteNumber(nn);
		
		if(!me)
		{
			cerr << "Oops! Just loaded an instrument which cannot be found by its configured note number!" << endl;
			return false;
		}
		
		if(!info->victims.empty())
		{
			for(InstrumentInfo::VictimList::iterator f = info->victims.begin(); f != info->victims.end(); ++f)
			{
				unsigned noteNumber = (unsigned)atoi((*(f)).c_str());
				if(noteNumber > 0)
				{
					// Find the victim instrument
					Instrument *vic = drumkit->findByNoteNumber(noteNumber); 
					if(vic)
					{
						me->addVictim(vic);
					}
				}
			}
		}
	}
	
	InstrumentList allInst = drumkit->allInstruments();
	
	// Load scenes
	for(KitInfo::SceneInfoList::iterator e = kit.scenes.begin(); e != kit.scenes.end(); ++e)
	{
		// Walk through the SceneSettingInfos and make sure there's an instrument matched up to each one.
		for(SceneInfo::SettingInfoList::iterator f = (*e)->settings.begin(); f != (*e)->settings.end(); ++f)
		{
			string name = (*f)->instrumentName;
			Instrument *inst = drumkit->findInstrumentByName(name.c_str());
			(*f)->inst = inst;
		}
		
		// Now, make sure there's a SceneSetting for each instrument in the kit.
		for(InstrumentList::iterator f = allInst.begin(); f != allInst.end(); ++f)
		{
			Instrument * inst = *f;
			
			// Does the SceneInfo have a SceneSettingInfo for the instrument?
			// If not, make sure it does.
			SceneSettingInfo * info = (*e)->findSettingInfoFor(inst);
			if(!info)
			{
				info = SceneSettingInfo::from(inst);
				(*e)->settings.push_back(info);	
			}
		}
		
		// convert to a real Scene for the kit
		drumkit->getScenes().push_back((*e)->toScene());
	}
	
/*	for(SceneList::iterator e = drumkit->getScenes().begin(); e != drumkit->getScenes().end(); ++e)
	{
		Scene *scene = *e;
		
		for(SceneSettingList::iterator f = scene->getSettings().begin(); f != scene->getSettings().end(); ++f)
		{
			SceneSetting * setting = *f;
			
			LOG_TRACE("\t" 
				<< setting->getInstrument()->getName() 
				<< " .level=" << setting->level 
				<< " .pan=" << setting->pan 
				<< " .pitch=" << setting->pitch 
			);
		}
	}*/
	
	return true;
}

bool Configuration::load(const char *filename, IFileLoadProgressListener * listener)
{
	LogPtr log = LogFactory::getLog(__FILE__);

	if(listener)
	{
		string msg = "Load ";
		msg += filename;
		listener->progressEvent(msg);
	}
		
	string filePath = getPath(filename);
	
	IXMLReader *reader = new XMLFileReader(filename);
	XMLDocument doc(reader);
	
	if(!doc.parse()) {
		LOG_ERROR(log, "file is not valid XML: " << filePath);
		return false;
	}
	
	XMLDocument::Element *top = doc.getTopElement();
	if(!top)
		return false;
		
	XMLDocument::Element *ports = top->findElement("ports");
	XMLDocument::Element *drumkit = top->findElement("drumkit");
	
	if(ports)
	{
		for(XMLDocument::Element::List::iterator e = ports->getElements().begin(); e != ports->getElements().end(); ++e)
			portNames.push_back((*(e))->getAttributeValue("name"));
	}
	else
	{
		portNames.push_back("left");
		portNames.push_back("right");
	}
	
	if(drumkit)
	{
		kit.name = drumkit->getAttributeValue("name");
		
		unsigned lvl = (unsigned)atoi(drumkit->getAttributeValue("level").c_str());
		if(lvl <= 0)
			lvl = 100;
			
		kit.level = lvl;
		
		XMLDocument::Element *instruments = drumkit->findElement("instruments");
		
		if(!instruments)
		{
			LOG_ERROR(log, "Unable to find any instruments for drumkit '" << kit.name << "'");
			return false;
		}
		
		XMLDocument::Element::List list = instruments->getElements("instrument");
		for(XMLDocument::Element::List::iterator e = list.begin(); e != list.end(); ++e)
		{
			InstrumentInfo *info = new InstrumentInfo();
			XMLDocument::Element *elem = *e;
			
			info->noteNumber = elem->getAttributeValue("noteNumber");
			info->name = elem->getAttributeValue("name");
			info->submix = elem->getAttributeValue("submix");
			info->level = elem->getAttributeValue("level");
			info->pan = elem->getAttributeValue("pan");
			info->pitch = elem->getAttributeValue("pitch");
			
			XMLDocument::Element* layers = elem->findElement("layers");
			if(layers)
			{
				for(
					XMLDocument::Element::List::iterator f = layers->getElements().begin(); 
					f != layers->getElements().end(); ++f)
				{
					XMLDocument::Element *layer = *f;
					
					LayerInfo *linfo = new LayerInfo();
					
					linfo->lo = layer->getAttributeValue("velLo");
					linfo->hi = layer->getAttributeValue("velHi");
					linfo->wave = layer->getAttributeValue("wave");
					
					string wv = linfo->wave;
					
					if(!isAbsolutePath(wv))
						linfo->wave = fullPath(filePath, wv);
					
					// Make sure the sample file exists
					std::ifstream verify(linfo->wave.c_str(), std::ios::in | std::ios::binary);
					if(verify == NULL)
					{
						LOG_WARN(log, "Sample file not found: " << linfo->wave);
					}

					info->layers.push_back(linfo);
				}
			}
			
			XMLDocument::Element* victims = elem->findElement("victims");
			if(victims)
			{
				for(XMLDocument::Element::List::iterator f = victims->getElements().begin();
					f != victims->getElements().end(); ++f)
					info->victims.push_back((*(f))->getAttributeValue("noteNumber"));
			}
			
			kit.instruments.push_back(info);
		}
		
		XMLDocument::Element * scenes = drumkit->findElement("scenes");
		if(scenes)
		{
			kit.selectedScene = scenes->getAttributeValue("selected");
			
			XMLDocument::Element::List elems = scenes->getElements("scene");
			for(XMLDocument::Element::List::iterator e = elems.begin(); e != elems.end(); ++e)
			{
				XMLDocument::Element * elem = *e;
				
				SceneInfo * scene = new SceneInfo();
				scene->name = elem->getAttributeValue("name");
				
				XMLDocument::Element::List settings = elem->getElements("setting");
				for(XMLDocument::Element::List::iterator f = settings.begin(); f != settings.end(); ++f)
				{
					XMLDocument::Element * felem = *f;
					
					SceneSettingInfo * setting = new SceneSettingInfo();
					
					setting->instrumentName = felem->getAttributeValue("instrument");
					setting->level = toInt(felem->getAttributeValue("level"), 100);
					setting->pan = toInt(felem->getAttributeValue("pan"), 0);
					setting->pitch = toInt(felem->getAttributeValue("pitch"), 0);
					setting->mute = toBool(felem->getAttributeValue("mute"), false);
					setting->solo = toBool(felem->getAttributeValue("solo"), false);
					scene->settings.push_back(setting);
				}
				
				kit.scenes.push_back(scene);
			}
		}
	}

	return true;
}

//
// ------------------------SceneSettingInfo
//
SceneSetting * SceneSettingInfo::toSceneSetting()
{
	SceneSetting * setting = new SceneSetting();
	
	setting->level = this->level;
	setting->pitch = this->pitch;
	setting->pan = this->pan;
	setting->mute = this->mute;
	setting->solo = this->solo;
	setting->setInstrument(this->inst);
	
	return setting;
}

SceneSettingInfo * SceneSettingInfo::from(SceneSetting * setting)
{
	SceneSettingInfo * info = new SceneSettingInfo();
	
	if(setting)
	{
		info->level = setting->level;
		info->pitch = setting->pitch;
		info->pan = setting->pan;
		info->mute = setting->mute;
		info->solo = setting->solo;
		info->inst = setting->getInstrument();
		info->instrumentName = (info->inst)? info->inst->getName(): "";
	}
	
	return info;
}

SceneSettingInfo * SceneSettingInfo::from(Instrument * inst)
{
	SceneSettingInfo * info = new SceneSettingInfo();
	
	if(inst)
	{
		info->level = inst->getLevel();
		info->pan = inst->getPan();
		info->pitch = inst->getPitch();
		info->mute = inst->isMuted();
		info->solo = inst->isSoloed();
		info->inst = inst;
		info->instrumentName = inst->getName();
	}
	
	return info;
}

//
// ------------------------SceneInfo
//
Scene * SceneInfo::toScene()
{
	Scene * scene = new Scene();
	
	scene->setName(name);
	
	for(SettingInfoList::iterator e = settings.begin(); e != settings.end(); ++e)
		scene->add((*e)->toSceneSetting());
	
	return scene;
}

SceneInfo * SceneInfo::from(Scene * scene)
{
	SceneInfo * info = new SceneInfo();
	
	info->name = scene->getName();
	
	for(SceneSettingList::iterator e = scene->getSettings().begin(); e != scene->getSettings().end(); ++e)
		info->settings.push_back(SceneSettingInfo::from((*e)));
	
	return info;
}

SceneSettingInfo * SceneInfo::findSettingInfoFor(Instrument * inst)
{
	for(SceneInfo::SettingInfoList::iterator e = settings.begin(); e != settings.end(); ++e)
	{
		if((*e)->inst == inst)
			return *e;
	}
	
	return 0;
}

//
//-------------------------KitInfo
//
KitInfo::~KitInfo()
{
	for(InstrumentInfoList::iterator e = instruments.begin(); e != instruments.end(); ++e)
		delete *e;
		
	for(SceneInfoList::iterator e = scenes.begin(); e != scenes.end(); ++e)
		delete *e;
}

KitInfo& KitInfo::addSubmix(string& name)
{
	bool has = false;
	
	for(KitInfo::SubmixList::iterator e = submixes.begin(); e != submixes.end(); ++e)
	{
		if((*(e)) == name)
		{
			has = true;
			break;
		}
	}
	
	if(!has)
		submixes.push_back(name);
		
	return *this;
}

