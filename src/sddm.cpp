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

#include <vector>
#include <map>
#include <stack>
#include <deque>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <getopt.h>

#include <pthread.h>

#include <sndfile.h>
#include <jack/jack.h>
#include <values.h> // MAXFLOAT
#include <cmath>

using namespace std;

#include "model.h"
#include "midi.h"
#include "alsamidi.h"
#include "audio_driver.h"

#include "config.h"

#include "sddm.h"
#include "streamer.h"

const int MAX_POLY = 8; // TODO: Change to something higher

const float LIMIT = 1.0f;

SDDM * SDDM::instance = 0;

pthread_mutex_t SDDM::notemutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t SDDM::orphanmutex = PTHREAD_MUTEX_INITIALIZER;


SDDM::SDDM()
: kit(0)
, sampleEndGap(0)
, maxPolyphony(-1)
, verbose(false)
{
	// initialize mutex
	pthread_mutex_init(&SDDM::orphanmutex, 0);
	// Fill the "available notes" queue with Notes.
	for(int i = 0; i < MAX_POLY; ++i)
	{
		availableNotes.push_front(new Note());
	}
}

SDDM::~SDDM()
{
	cout << "SDDM dtor" << endl;

	for(NoteQueue::iterator e = availableNotes.begin(); e != availableNotes.end(); ++e)
	{
		delete (*(e));
	}

	cout << "SDDM dtor end" << endl;
}

bool SDDM::loadKit(
	const char *filename
, bool ignorePorts
, int maxSamples
, Configuration::SubmixNameList includedSubmixes
, IFileLoadProgressListener * listener)
{
	Drumkit *newKit = new Drumkit();

	string kitFile = string(filename);
	std::ifstream verify(kitFile.c_str(), std::ios::in);
	if(!verify)
	{
        //string msg = kitFile + " not found.";
        //throw oops(msg);
		return false;
	}

	Configuration conf(includedSubmixes);

	if (this->kit) {
		// reuse the existing submixes in the new kit if possible
		SubmixList existingSubmixes = this->kit->getSubmixes();
		for (unsigned i = 0; i < existingSubmixes.size(); ++i) {
			existingSubmixes[i]->setOrphaned(true);
			newKit->addSubmix(existingSubmixes[i]);
		}
		// try to reuse orphaned submixes (so they don't close ports)
		pthread_mutex_lock(&orphanmutex);
		for (std::set<Submix*>::iterator e = orphanSubmixes.begin();
				e != orphanSubmixes.end(); ++e) {
			newKit->addSubmix(*e);
		}
		orphanSubmixes.clear();
		pthread_mutex_unlock(&orphanmutex);
	}


 	if(!conf.load(filename, newKit, ignorePorts, maxSamples, listener))
 	{
 		// TODO: reorphan submixes??
 		string msg = "Unable to load ";
 		msg += filename;
 		throw oops(msg);
 		return false;
 	}

 	SubmixList submixes = newKit->getSubmixes();
 	for(unsigned i = 0; i < submixes.size(); ++i)
 	{
 		if(submixes[i]->isOrphan()) {
 			continue;
 		}

 		// Something like this:
 		string
 			portL = submixes[i]->getName() + "_L"
 		, portR = submixes[i]->getName() + "_R"
 		;
 		if(!getAudioDriver()->hasRegisteredPort(portL))
 		{
 			if(!getAudioDriver()->registerPort(portL))
 			{
 				cerr << "Unable to register left port " << portL << endl;
 			}

 			if(!getAudioDriver()->registerPort(portR))
 			{
				cerr << "Unable to register right port " << portR << endl;
			}

			if (submixes[i]->isAutoConnect()) {
				getAudioDriver()->connectMainStereoOut(portL, portR);
			}
		}
	}

 	PortNameList ports = newKit->getPortNames();
 	for(unsigned i = 0; i < ports.size(); ++i)
	{
 		cout << "kit port: " << ports[i] << endl;
	}


 	if (this->kit) {
		// block any more notes from the old kit
		pthread_mutex_lock(&notemutex);

		// orphan all old instruments and possibly submixes
		pthread_mutex_lock(&orphanmutex);
		std::vector<Instrument*> allInstruments = this->kit->allInstruments();
		for (unsigned i = 0; i < allInstruments.size(); i++) {
			orphanInstruments.insert(allInstruments[i]);
		}
		for (unsigned i = 0; i < submixes.size(); ++i) {
			if (submixes[i]->isOrphan()) {
				orphanSubmixes.insert(submixes[i]);
				newKit->removeSubmix(submixes[i]);
			}
		}
		pthread_mutex_unlock(&orphanmutex);
	}

	// switch kits
	Drumkit *oldKit = this->kit;
	this->kit = newKit;
	pthread_mutex_unlock(&notemutex);

	// delete the old kit but orphaned submixes and instruments keep playing
 	if(oldKit) {
		delete oldKit;
	}

	return true;
}

void SDDM::onMidiMessage(const MidiMessage &msg)
{
	if(!kit)
	{
		return;
	}

	switch(msg.type)
	{
		case MidiMessage::NOTE_ON:
		{
			int noteNumber = msg.data1;
			int velocity = msg.data2;

			if(velocity > 0)
			{
				Instrument *inst = kit->findByNoteNumber(static_cast<unsigned>(noteNumber));
				if(inst)
				{
					if(inst->hasVictims())
					{
						pthread_mutex_lock(&notemutex);
						cancelNotesFor(inst->getVictims(), playingNotes);
						cancelNotesFor(inst->getVictims(), pendingNotes);
						pthread_mutex_unlock(&notemutex);
					}

					InstrumentLayer* layer = inst->findLayerByVelocity(velocity);

					if(layer && layer->getSample())
					{
						Note * n = findLRUNote();
						n->set(inst, const_cast<Sample*>(layer->getSample()), velocity, noteNumber);
						pushNote(n);
					}
				}
			}

			break;
		}

		default:
			break;
	}


	// TODO: move this to its own low-priority thread
	std::set<Instrument*> playingInstruments;
	std::set<Submix*> playingSubmixes;

	// find currently playing instruments and submixes
	int rc = pthread_mutex_lock(&notemutex);
	for (NoteQueue::iterator e = playingNotes.begin(); e != playingNotes.end();
			++e) {
		Instrument* instrument = (*e)->getInstrument();
		playingInstruments.insert(instrument);
		playingSubmixes.insert(instrument->getSubmix());
	}
	rc = pthread_mutex_unlock(&notemutex);

	rc = pthread_mutex_unlock(&orphanmutex);
	// delete any instrument no longer playing
	for (std::set<Instrument*>::iterator e = orphanInstruments.begin();
			e != orphanInstruments.end(); ++e) {
		Instrument* orphanInstrument = *e;
		if (playingInstruments.find(orphanInstrument) == playingInstruments.end()) {
			orphanInstruments.erase(orphanInstrument);
			delete orphanInstrument;
			break;
		}
	}
	// delete any submix no longer attached to playing notes
	for (std::set<Submix*>::iterator e = orphanSubmixes.begin();
			e != orphanSubmixes.end(); ++e) {
		Submix* orphanSubmix = *e;
		if (playingSubmixes.find(orphanSubmix) == playingSubmixes.end()) {
			orphanSubmixes.erase(orphanSubmix);
			PortNameList ports = orphanSubmix->getPortNames();
			for (unsigned j = 0; j < ports.size(); ++j) {
				audioDriver->unregisterPort(ports[j]);
			}
			delete orphanSubmix;
			break; // once is enough per pass
		}
	}
	rc = pthread_mutex_unlock(&orphanmutex);


}

void SDDM::pushNote(Note * note)
{
	int rc = pthread_mutex_lock(&notemutex);
	pendingNotes.push_front(note);
	rc = pthread_mutex_unlock(&notemutex);
}

/** Find the least-recently-used note in the available-notes queue.
  If one is available, return it. Otherwise, push a new note onto the
	front of the queue.
	*/
Note * SDDM::findLRUNote()
{
	int rc = pthread_mutex_lock(&notemutex);
	Note * n;

	if(availableNotes.size() > 0)
	{
		n = availableNotes.back();
		// Got a note from the queue. Pop it off
		availableNotes.pop_back();
	}
	else
	{
		// Create a new Note. 
		// The new Note will be pushed onto the front of the 
		// "available" queue when it's done being used.
		n = new Note();
	}

	rc = pthread_mutex_unlock(&notemutex);
	return n;
}

bool SDDM::hasSubmix(SubmixList& list, Submix* mix)
{
	for(SubmixList::iterator e = list.begin(); e != list.end(); ++e)
	{
		if((*(e)) == mix)
		{
			return true;
		}
	}

	return false;
}

// Return a list of unique submixes for the specified notes.
SubmixList SDDM::getSubMixesFor(NoteQueue& notes)
{
	SubmixList list;

	for(NoteQueue::iterator e = notes.begin(); e != notes.end(); ++e)
	{
		Note* note = *e;

		if(!note->isFinished())
		{
			Submix* mix = note->getInstrument()->getSubmix();

			if(mix)
			{
				if(!hasSubmix(list, mix))
				{
					list.push_back(mix);
				}
			}
		}
	}

	return list;
}

BufferRequestList SDDM::getBufferRequests()
{
	BufferRequestList requests;

	// Jack is asking if we want buffers.
	if(!pendingNotes.empty())
	{
		Note* note = pendingNotes.front();
		pthread_mutex_lock(&notemutex);
		playingNotes.push_front(note);
		pendingNotes.pop_front();
		pthread_mutex_unlock(&notemutex);
	}

	// Trim off notes beyond our max polyphony setting
	// TODO: This could go away now that polyphony is fixed.
	if(maxPolyphony != -1 && (playingNotes.size() > static_cast<unsigned>(maxPolyphony)))
	{
		if(verbose)
		{
			cout << "max polyphony: " << maxPolyphony << " reached" << endl;
		}

		pthread_mutex_lock(&notemutex);

		for(NoteQueue::iterator e = playingNotes.begin() + maxPolyphony; e != playingNotes.end(); ++e)
		{
			(*(e))->finish();
		}

		pthread_mutex_unlock(&notemutex);
	}

	// If we're playing anything, we now have playing notes.
	if(!playingNotes.empty())
	{
		// Get a list of submixes for each note we're playing
		int rc = pthread_mutex_lock(&notemutex);
		SubmixList playingMixes = getSubMixesFor(playingNotes);
		rc = pthread_mutex_unlock(&notemutex);

		for(SubmixList::iterator e = playingMixes.begin(); e != playingMixes.end(); ++e)
		{
			requests.push_back(new BufferRequest((*(e)), (*(e))->getPortNames()));
		}

		// Always add requests for the main ports if data is headed there.
		PortNameList ports;
		ports.push_back(AudioDriver::LEFT_PORT_NAME);
		ports.push_back(AudioDriver::RIGHT_PORT_NAME);

		requests.push_back(new BufferRequest(0, ports));
	}

	return requests;
}

// Return notes destined for the specified Submix.
// If the specified Submix is null, return notes not destined for any submix.
// If any of the notes are associated with muted instruments, omit them anyway.
// If any of the notes are associated with soloed instruments, include ONLY them.
NoteQueue SDDM::findPlayableNotes(NoteQueue& notes, Submix* mix)
{
	NoteQueue q;

	for(NoteQueue::iterator e = notes.begin(); e != notes.end(); ++e)
	{
		Note* note = *e;

		Instrument * inst = note->getInstrument();
		if(inst->isMuted() || inst->isAutoMuted())
		{
			note->cancel();
			continue;
		}

		if(mix)
		{
			if(inst->getSubmix() == mix)
			{
				q.push_back(note);
			}
		}
		else
		{
			if(!inst->isInSubmix())
			{
				q.push_back(note);
			}
		}
	}

	return q;
}

void SDDM::play(BufferResponse* response)
{
	if(!playingNotes.empty())
	{
		unsigned frames = response->getFrames();

		// Find the submix whose ports we're supposed to populate.
		Submix* mix = (Submix*)response->getRequest()->getData();
		NoteQueue notes = findPlayableNotes(playingNotes, mix);

		float kitLevelL = (float)kit->getLevel();
		if(kitLevelL <= 0)
		{
			kitLevelL = 100;
		}

		kitLevelL /= 100;

		float kitLevelR = kitLevelL;

		// Assign pointers to the left/right buffers in the response.
		float* left = response->getLeft();
		float* right = response->getRight();

		for(unsigned i = 0; i < frames; ++i)
		{
			// Point at the current spot in the buffer.
			float *dataL = (left+i);
			float *dataR = (right+i);

			for(NoteQueue::iterator e = notes.begin(); e != notes.end(); ++e)
			{
				Note *n = (*(e));

				if(n->isCancelled())
					continue;

				float volumeL = (float)n->getInstrument()->getLevel();
				float volumeR = volumeL;

				short pan = n->getInstrument()->getPan();

				/*
				pan can have a value from -100 (full left) to 100 (full right).

				subtract (pan) from left Volume to derive its volume.
				add (pan) to right to get its volume.
				*/
				volumeL -= pan;
				volumeR += pan;

				volumeL /= 100;
				volumeR /= 100;

				volumeL *= kitLevelL;
				volumeR *= kitLevelR;

				const Sample *sample = n->getSample();

				if(sample->frames - (n->samplePosition+1) <= sampleEndGap)
				{
					n->finish();
					break;
				}

				float* sampleL = (sample->dataL + (unsigned)n->samplePosition);
				float* sampleR = (sample->dataR + (unsigned)n->samplePosition);

				/*
				"Jack" with the pitch (har har).

				If the instrument's pitch is higher than the default, step through the sample faster.
				If lower, step through slower. This is basically the same as up- or down-sampling.
				*/
				float step = 1.0f + (((float)n->getInstrument()->getPitch()) / 100);

				float instLSum = 0.0f, instRSum = 0.0f;

				// Now is the time on Sprockets when we mix!
				if(dataL && sampleL)
				{
					instLSum = (*sampleL * volumeL);
					float sum = (*dataL + (*sampleL * volumeL));
					if(fabs(sum) < LIMIT)
						*dataL = sum;
				}

				if(dataR && sampleR)
				{
					instRSum = (*sampleR * volumeR);
					float sum = (*dataR + (*sampleR * volumeR));
					if(fabs(sum) < LIMIT)
						*dataR = sum;
				}

				// This works for a "master" fader, but not for individual instruments.
// 				n->getInstrument()->setVolumes(fabs(*dataL)*100, fabs(*dataR)*100);

				// Only set the volumes on the instrument if this volume is higher than
				// the current one. 
				// (Without this, previously-played notes (which appear later in the notes queue)
				// override the higher values of the most-recently played notes, resulting in "backwards"
				// readings on VU meters, etc.
				instLSum = fabs(instLSum * 100);
				instRSum = fabs(instRSum * 100);

				if(instLSum >= n->getInstrument()->getVolumeL())
					n->getInstrument()->setVolumeL(instLSum);

				if(instRSum >= n->getInstrument()->getVolumeR())
					n->getInstrument()->setVolumeR(instRSum);

				n->samplePosition += step;
			} // for (notes...)
		} // for (frames...)

		pthread_mutex_lock(&notemutex);

		for(unsigned i = 0; i < playingNotes.size(); ++i)
		{
			Note* n = playingNotes[i];
			if(n->isFinished())
			{
				playingNotes.erase(playingNotes.begin() + i);
				availableNotes.push_front(n);
			}
		}

		for(NoteQueueListenerList::iterator e = noteQueueListeners.begin(); e != noteQueueListeners.end(); ++e)
		{
			(*(e))->noteQueueUpdate(&playingNotes);
	  }

		pthread_mutex_unlock(&notemutex);
	}
}

// Remove all notes for the instruments in the specified list.
void SDDM::cancelNotesFor(InstrumentList& victims, NoteQueue& notes)
{
	// TODO: This could be faster. See if there's a way to put victims 
	// in a map of some kind so lookups happen faster.
	for(InstrumentList::iterator e = victims.begin(); e != victims.end(); ++e)
	{
		for(NoteQueue::iterator f = notes.begin(); f != notes.end(); ++f)
		{
			Note *n = (*(f));
			if(n->getInstrument() == (*(e)))
			{
				n->cancel();
			}
		}
	}
}

// MIDIDumper implementation
void MIDIDumper::onMidiMessage(const MidiMessage& msg)
{
	switch(msg.type)
	{
		case MidiMessage::NOTE_ON:
		{
			if(msg.data2 > 0)
				cout << "Number: " << msg.data1 << " velocity: " << msg.data2 << endl;
			break;
		}

		case MidiMessage::PITCH_WHEEL:
		{
			cout << "pitch: data1=" << msg.data1 << " data2=" << msg.data2 << endl;
			break;
		}

		default:
			break;
	}
}

