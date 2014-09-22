// config.h
// configuration for SDDM
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

#ifndef config_h
#define config_h

struct IFileLoadProgressListener {
	virtual bool progressEvent(string &) { return true; }
};

// Layer configuration info
struct LayerInfo
{
	std::string lo, hi;
	std::string wave;
	
	static LayerInfo* from(InstrumentLayer *layer);
};

// Instrument configuration info
struct InstrumentInfo
{
	InstrumentInfo() {}
	~InstrumentInfo();
	
	typedef std::vector<LayerInfo*> LayerInfoList;
	typedef std::vector<string> VictimList;

	LayerInfoList layers;
	VictimList victims;

	std::string noteNumber;
	std::string name;
	std::string submix;
	std::string level;
	std::string pan;
	std::string pitch;

	Instrument* toInstrument(int maxSamples = -1, IFileLoadProgressListener * = 0);
	static InstrumentInfo* from(Instrument*);
};

// Scene info
struct SceneSettingInfo {
	SceneSettingInfo()
	: inst(0) {}
	
	string instrumentName;
	int level;
	int pan;
	int pitch;
	bool mute, solo;
	Instrument * inst;
	
	SceneSetting * toSceneSetting();
	static SceneSettingInfo * from(SceneSetting*);
	static SceneSettingInfo * from(Instrument*);
};

struct SceneInfo
{
	SceneInfo()	{}
	virtual ~SceneInfo()
	{
		for(SettingInfoList::iterator e = settings.begin(); e != settings.end(); ++e)
			delete *e;
	}
	
	typedef std::vector<SceneSettingInfo*> SettingInfoList;
	
	string name;
	SettingInfoList settings;
	
	Scene * toScene();
	static SceneInfo * from(Scene *);
	SceneSettingInfo * findSettingInfoFor(Instrument*);
};

// Kit configuration info
struct KitInfo
{
	KitInfo() {}
	~KitInfo();
	
	typedef std::vector<InstrumentInfo*> InstrumentInfoList;
	typedef std::vector<string> SubmixList;
	typedef std::vector<SceneInfo*> SceneInfoList;

	string name;
	unsigned level;
	InstrumentInfoList instruments;
	SceneInfoList scenes;
	string selectedScene;
	
	SubmixList& getSubmixes() { return submixes; }
	KitInfo& addSubmix(string& name);
	
private:
	SubmixList submixes;
};

class Configuration
{
public:
	typedef std::vector<string> PortList;
	typedef std::vector<string> SubmixNameList;

	PortList portNames;
	SubmixNameList includedSubmixes;
	KitInfo kit;

	Configuration(SubmixNameList included);
    Configuration() {}
    virtual ~Configuration() {}

	/// Load a configuration file.
	bool load(const char *, IFileLoadProgressListener * = 0);
	/// Load the configuration file into the specified drumkit.
	bool load(const char *, Drumkit *kit, bool ignorePorts = false, int maxSamples = -1, IFileLoadProgressListener * = 0);

	/// Save the configuration in a file.
	bool save(const char *);
	/// Save the specified Drumkit configuration into the specified file.
	bool save(const char*, Drumkit *kit);
};

#endif // config_h
