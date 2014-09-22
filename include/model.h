
// model.h
// Contains all the stuff for playing drums.
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

#ifndef _model_h
#define _model_h

#include <vector>
#include <map>
#include <string>

#include "scene.h"

using namespace std;

#define MONO 1
#define STEREO 2

typedef vector<std::string> StringList;

/* A sample */
class Sample {
private:
	std::string filename;

public:
	Sample(unsigned frames, const string& filename);
	Sample(const char *filename)
	: filename(filename)
	{}

	virtual ~Sample();

	const std::string &getFilename() const { return filename; }

public:
	float *dataL;
	float *dataR;
	unsigned frames;
	unsigned sampleRate;

	/// Returns the bytes number ( 2 channels )
	unsigned getLength() const {	return frames * sizeof(float) * STEREO;	}
	unsigned getRate() const { return sampleRate; }

	/// Loads a sample from disk
	static Sample* load(const string& filename, int maxSamples = -1);

private:
	/// loads a wave file
	static Sample* loadWave(const string& filename, int maxSamples = -1);
};

ostream &operator << (ostream&, Sample&);

// A layer appearing in an instrument.
class InstrumentLayer {
private:
	Sample *sample;
	unsigned velocityLO, velocityHI;

public:
	InstrumentLayer(Sample *sample, unsigned lo, unsigned hi)
	: sample(sample)
	, velocityLO(lo)
	, velocityHI(hi)
	{}

	virtual ~InstrumentLayer();

	// Return the sample for this instrument layer.
	const Sample* getSample() const { return sample; }

	unsigned getVelocityLO() { return velocityLO; }
	InstrumentLayer& setVelocityLO(unsigned lo) { velocityLO = lo; return *this; }

	unsigned getVelocityHI() { return velocityHI; }
	InstrumentLayer& setVelocityHI(int hi) { velocityHI = hi; return *this; }
};

// A list of InstrumentLayers.
typedef std::vector<InstrumentLayer*> InstrumentLayerList;
// A list of port names.
typedef std::vector<std::string> PortNameList;

ostream& operator << (ostream&, InstrumentLayer&);

/** A submix */
class Submix {
	string name;
	bool autoConnect;
	bool orphaned;
public:
	Submix(string name)
	: name(name)
	, autoConnect(true)
	{}
	
	PortNameList getPortNames() 
	{
		PortNameList ports;
		ports.push_back(name + "_L");
		ports.push_back(name + "_R");
		return ports;
	}
	
	string& getName() { return name; }
	bool isAutoConnect() { return autoConnect; }
	bool isOrphan() { return orphaned; }
	void setOrphaned(bool used) { orphaned = used; }
};

ostream& operator << (ostream&, Submix&);

class Instrument;
/** A list of Instruments. */
typedef std::vector<Instrument*> InstrumentList;

/** An instrument. */
class Instrument {
private:
	std::string name;
	InstrumentLayerList layers;
	InstrumentList victims;
	unsigned noteNumber;
	unsigned level;
	short pan;
	string submixName;
	Submix* submix;
	int pitch;
	float volumeL, volumeR;
	bool muted, autoMuted, soloed;
public:
	Instrument(const char *name)
	: name(name)
	, level(100)
	, pan(0)
	, submixName("")
	, submix(0)
	, pitch(0)
	, volumeL(0.0f)
	, volumeR(0.0f)
	, muted(false)
	, autoMuted(false)
	, soloed(false)
	{}
	
	Instrument(const char* name, const char* submix)
	: name(name)
	, level(100)
	, pan(0)
	, submixName(submix)
	, submix(0)
	, pitch(0)
	, volumeL(0.0f)
	, volumeR(0.0f)
	, muted(false)
	, autoMuted(false)
	, soloed(false)
	{}

	virtual ~Instrument();
	
	std::string& getName() { return name; }
	Instrument& setName(std::string & n) { name = n; return *this; }
	Instrument& setName(const char * n) { name = n; return *this; }
	
	bool isMuted() { return muted; }
	void setMuted(bool m) 
	{ 
		muted = m; 
		if(muted)
			setSoloed(false);
	}
	
	bool isSoloed() { return soloed; }
	void setSoloed(bool s) 
	{ 
		soloed = s; 
		if(soloed)
			setMuted(false);
	}
	
	bool isAutoMuted() { return autoMuted; }
	void setAutoMuted(bool a) { autoMuted = a; }
	
	float getVolumeL() { return volumeL; }
	void setVolumeL(float l) { volumeL = l; }
	
	float getVolumeR() { return volumeR; }
	void setVolumeR(float r) { volumeR = r; }
	
	void setVolumes(float l, float r) { volumeL = l; volumeR = r; }
	
	unsigned getNoteNumber() { return noteNumber; }
	void setNoteNumber(unsigned nn) { noteNumber = nn; }
	
	unsigned getLevel() { return level; }
	Instrument& setLevel(unsigned l) { level = l; return *this; }
	
	int getPitch() { return pitch; }
	Instrument& setPitch(int pitch) { this->pitch = pitch; return *this; }
	
	string& getSubmixName() { return submixName; }
	Instrument& setSubmixName(string& name) { submixName = name; return *this; }
	bool isInSubmix() { return (!submixName.empty()); }
	Instrument& removeFromSubmix() { submixName = string(""); return *this; }
	
	Submix* getSubmix() { return submix; }
	Instrument& setSubmix(Submix* mix) { submix = mix; return *this; }
	
	short getPan() { return pan; }
	Instrument& setPan(short p) { pan = p; return *this; }
	
	InstrumentLayerList& getLayers() { return layers; }
	
	Instrument& addVictim(Instrument *vic) { victims.push_back(vic); return *this; }
	InstrumentList& getVictims() { return victims; }
	bool hasVictims() { return !victims.empty(); }

	Instrument &add(InstrumentLayer* layer)
	{
		layers.push_back(layer);
		return *this;
	}

	InstrumentLayer* findLayerByVelocity(unsigned int velocity);

	bool operator < (Instrument *other) const
	{
		return noteNumber < other->getNoteNumber();
	}
};

ostream& operator << (ostream&, Instrument&);

/** A note being played. This is a transient type that persists from the time a note is "played"
	until it reaches the end of the sample it's supposed to play. Then it goes away. */
class Note {
private:
	Instrument *instrument;
	Sample *sample;
	unsigned int velocity;
	unsigned int number;
	bool finished;
	bool cancelled;

public:
	float samplePosition;
	
	Note()
	: instrument(0)
	, sample(0)
	, velocity(0)
	, number(0)
	, finished(false)
	, cancelled(false)
	, samplePosition(0.0f)
	{}

	Instrument *getInstrument() { return instrument; }
	Sample *getSample() { return sample; }
	unsigned int getVelocity() { return velocity; }
	unsigned int getNumber() { return number; }
	
	bool isFinished() { return finished; }
	void finish(); 
	
	bool isCancelled() { return cancelled; }
	void cancel();
	/** Set this note up for use with the specified Instrument, sample, etc. */ 
	void set(Instrument * inst, Sample * sample, unsigned int velocity, unsigned int number);
};

ostream &operator << (ostream&, Note&);

// A map of MIDI note numbers to Instruments.
typedef std::map<unsigned int, Instrument *> InstrumentMap;

/** Settings for an instrument in a Scene. */
struct SceneSetting {
	SceneSetting();
	~SceneSetting();
	
	void apply();
	void update();
	void reset();
	void copyFrom(SceneSetting &);
	
	void setInstrument(Instrument * inst) { instrument = inst; }
	Instrument * getInstrument() { return instrument; }
	
	unsigned int level;
	int pan;
	int pitch;
	bool mute;
	bool solo;	
private:	
	Instrument * instrument;
};

/** A list of SceneSettings. */
typedef std::vector<SceneSetting*> SceneSettingList;

class Drumkit;

/** A group of SceneSettings */
class Scene {
public:
	Scene();
	~Scene();
		
	bool hasSettings();
	void markModified(bool m = true) { modified = m; }
	void activate(bool a = true) { active = a; }
	void deactivate();
	void reset();
	void copyFrom(Scene &);
	void add(SceneSetting *setting);
	
	string & getName() { return name; }
	void setName(string & n) { name = n; }
	
	SceneSettingList & getSettings() { return settings; }
	Scene * updateFrom(Drumkit *kit);
	
	static Scene * fromDrumkitSettings(Drumkit *kit);
	
private:	
	bool modified;
	bool active;
	string name;
	SceneSettingList settings;
};

/** A list of scenes. */
typedef std::vector<Scene*> SceneList;

/** A listener for things pertaining to a SceneManager */
struct ISceneManagerListener {
	virtual void sceneModified(Scene *) = 0;
	virtual void sceneActivated(Scene *) = 0;
	virtual void sceneListLoaded(SceneList &) = 0;
};

typedef std::vector<ISceneManagerListener*> SceneManagerListenerList;

/** A manager of scenes. Comments like this are pointless. But I'm the boss, and I require them. */
class SceneManager {
private:
	SceneManager();
public:
	~SceneManager();
	
	static SceneManager * instance;
	static const int NUM_SCENES;
	
	SceneManagerListenerList listeners;
	SceneList scenes;
	Scene * activeScene;
	
	Scene * addOrUpdate(Scene *);
	void remove(Scene *);
	
	void activate(Scene *);
	void loadScenes(SceneList &);
	void sceneModified();
	
	Scene * findSceneByName(string & name);
};

/** A collection of instruments that are hit with sticks. */
class Drumkit {
public:	
	typedef std::vector<Submix*> SubmixList;
	typedef std::map<string, Submix*> SubmixMap;
	
	Drumkit()
	: level(100)
	{}

	virtual ~Drumkit();
	
	string& getName() { return name; }
	Drumkit& setName(string s) { name = s; return *this; }
	
	unsigned getLevel() { return level; }
	Drumkit& setLevel(unsigned l) { level = l; return *this; }

	Drumkit& clear() { instruments.clear(); return *this; }
	Drumkit& add(unsigned int, Instrument *);
	Instrument * findInstrumentByName(const char *);
	Instrument * findByNoteNumber(unsigned int);
	InstrumentList allInstruments();
	
	Submix* addSubmix(string&);
	Drumkit& addSubmix(Submix *);
	Submix* findSubmixByName(string& name);
	void removeSubmix(Submix * submix) { submixes.erase(submix->getName());}
	SubmixList getSubmixes();
	Drumkit & clearSubmixes() { submixes.clear(); return *this; }
	
	SceneList & getScenes() { return this->scenes; }
	void setScenes(SceneList & scns) { this->scenes = scns; }
	
	string & getSelectedSceneName() { return this->selectedSceneName; }
	void setSelectedSceneName(string & name) { this->selectedSceneName = name; }

	PortNameList getPortNames();

private:
	string name;
	unsigned level;
	
	SubmixMap submixes;
	InstrumentMap instruments;
	SceneList scenes;
	string selectedSceneName;
};

/** Something that provides a UI for a Drumkit */
class DrumkitUI {
public:
	virtual ~DrumkitUI() {}

	virtual void initUI(Drumkit *) = 0;
};

/** Something that owns/references a drumkit */
class DrumkitOwner {
public:
	DrumkitOwner(): kit(0) {}
	DrumkitOwner(Drumkit * k): kit(k) {}
	
	virtual ~DrumkitOwner() {}

private:
	Drumkit * kit;

public:
	Drumkit * getDrumkit() { return kit; }
	void setDrumkit(Drumkit * k) { kit = k; }
};

/** Something that provides a UI for an Instrument */
class InstrumentUI {
public:
	virtual ~InstrumentUI() {}

	virtual void initUI(Instrument *) = 0;
};

/** An owner of an Instrument */
class InstrumentOwner {
private:
	Instrument * instrument;

public:
	InstrumentOwner(): instrument(0) {}
	InstrumentOwner(Instrument * inst): instrument(inst) {}
	
	virtual ~InstrumentOwner() {}
	
	Instrument * getInstrument() const { return instrument; }
	void setInstrument(Instrument * inst) { instrument = inst; }
};

#endif // _model_h
