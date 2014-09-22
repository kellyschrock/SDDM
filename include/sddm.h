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

#ifndef _sddm_h
#define _sddm_h

#include <deque>
#include <set>

#include "audio_driver.h"
#include "midi.h"
#include "config.h"

/** A deque of notes (hence the name). */
typedef std::deque<Note*> NoteQueue;

/** A list of Submixes */
typedef std::vector<Submix*> SubmixList;

/** A dumper of MIDI events. */
struct MIDIDumper: IMIDIListener {
	void onMidiMessage(const MidiMessage& msg);
};

/** Our local exception */
struct oops: std::exception {
	const char* message;
	
	oops(const char* wtf)
	: message(wtf) {}
	
	oops(const string& msg)
	: message(msg.c_str()) {}
};

struct INoteQueueListener {
	virtual ~INoteQueueListener() {}
	
	virtual void noteQueueUpdate(const NoteQueue *) = 0;
};

typedef std::vector<INoteQueueListener *> NoteQueueListenerList;

/** Our listener. Processes MIDI and audio input from
 Alsa and Jack, and plays Notes.
 */
struct SDDM: IMIDIListener, IAudioListener {
private:
	Drumkit *kit;
	NoteQueue pendingNotes;
	NoteQueue playingNotes;
	NoteQueue availableNotes;
	std::set<Submix*> orphanSubmixes;
	std::set<Instrument*> orphanInstruments;
	unsigned sampleEndGap;
	int maxPolyphony;
	bool verbose;
	AudioDriver * audioDriver;
	MidiDriver * midiDriver;

public:
	static SDDM * instance;
	SDDM();
	virtual ~SDDM();
	
	NoteQueueListenerList noteQueueListeners;
	
	bool loadKit(
		const char *filename
	, bool ignorePorts
	, int maxSamples
	, Configuration::SubmixNameList names
	, IFileLoadProgressListener * = 0);
	
	AudioDriver * getAudioDriver() { return audioDriver; }
	void setAudioDriver(AudioDriver * driver) { audioDriver = driver; }
	
	MidiDriver * getMidiDriver() { return midiDriver; }
	void setMidiDriver(MidiDriver * driver) { midiDriver = driver; }

	Drumkit* getDrumkit() const { return kit; }
	void setDrumkit(Drumkit * dk) { kit = dk; }

	unsigned getSampleEndGap() { return sampleEndGap; }
	const SDDM& setSampleEndGap(unsigned gap) { sampleEndGap = gap; return *this; }

	int getMaxPolyphony() { return maxPolyphony; }
	const SDDM& setMaxPolyphony(int max) { maxPolyphony = max; return *this; }
	
	void onMidiMessage(const MidiMessage& msg);
	
	bool hasSubmix(SubmixList& list, Submix* mix);
	
	// Return a list of unique submixes for the specified notes.
	SubmixList getSubMixesFor(NoteQueue& notes);

	BufferRequestList getBufferRequests();
	
	// Return notes destined for the specified Submix.
	// If the specified Submix is null, return notes not destined for any submix.
	NoteQueue findPlayableNotes(NoteQueue& notes, Submix* mix);
			
	void play(BufferResponse* response);
	void pushNote(Note * note);
	Note * findLRUNote();
	
	// Remove all notes for the instruments in the specified list.
	void cancelNotesFor(InstrumentList& victims, NoteQueue& notes);
	
	static pthread_mutex_t notemutex;
	static pthread_mutex_t orphanmutex;
};

#endif //_sddm_h
