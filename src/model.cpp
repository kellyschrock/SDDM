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

// model.cpp

#include <map>
#include <vector>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include <sndfile.h>
#include <jack/jack.h>

using namespace std;

#include "model.h"

//
// Drumkit
//
Drumkit& Drumkit::add(unsigned int midiNote, Instrument *inst)
{
	inst->setNoteNumber(midiNote);
	instruments[midiNote] = inst;
	return *this;
}

Instrument * Drumkit::findInstrumentByName(const char *name)
{
	string str(name);
	
	for(InstrumentMap::iterator e = instruments.begin(); e != instruments.end(); ++e)
	{
		if(e->second->getName() == str)
			return e->second;
	}
	
	return 0;
}

Instrument* Drumkit::findByNoteNumber(unsigned int noteNumber)
{
	return instruments[noteNumber];
}

InstrumentList Drumkit::allInstruments()
{
	InstrumentList list;

	for(InstrumentMap::iterator e = instruments.begin(); e != instruments.end(); ++e)
		list.push_back(e->second);

	return list;
}

Drumkit::~Drumkit()
{
	// instruments and submixes cleaned up elsewhere when free
}

PortNameList Drumkit::getPortNames()
{
	PortNameList names;
	
	SubmixList mixes = getSubmixes();

	for(SubmixList::iterator e = mixes.begin(); e < mixes.end(); ++e)
	{
		Submix* mix = *e;
		
		PortNameList ports = mix->getPortNames();
		for(PortNameList::iterator f = ports.begin(); f != ports.end(); ++f)
			names.push_back(*f);
	}

	return names;
}

Submix* Drumkit::addSubmix(string& name)
{
	string tmp = name;
	submixes[name] = new Submix(tmp);
	return submixes[name];
}

Drumkit& Drumkit::addSubmix(Submix* submix)
{
	submixes[submix->getName()] = submix;
	return *this;
}

Submix* Drumkit::findSubmixByName(string& name)
{
	return (submixes.find(name) == submixes.end())? 0: submixes[name];
}

Drumkit::SubmixList Drumkit::getSubmixes()
{
	SubmixList subs;
	
	for(SubmixMap::iterator e = submixes.begin(); e != submixes.end(); ++e)
		subs.push_back(e->second);
		
	return subs;
}

//
// InstrumentLayer
//
InstrumentLayer::~InstrumentLayer()
{
	delete sample;
}

ostream& operator << (ostream &out, InstrumentLayer &layer)
{
	out << "InstrumentLayer: velocityLo=" << layer.getVelocityLO()
			<< " velocityHi=" << layer.getVelocityHI()
			;
	return out;
}

//
// Instrument
//

// Find a layer whose range surrounds the specified velocity value
InstrumentLayer* Instrument::findLayerByVelocity(unsigned int velocity)
{
	for(InstrumentLayerList::iterator e = layers.begin(); e < layers.end(); ++e)
	{
		InstrumentLayer *layer = *e;
		if(layer->getVelocityLO() <= velocity && layer->getVelocityHI() >= velocity)
			return layer;
	}

	// Nothing found, return null
	return 0;
}

Instrument::~Instrument()
{
	for(InstrumentLayerList::iterator e = layers.begin(); e < layers.end(); ++e)
		delete *e;
}


ostream& operator << (ostream& out, Instrument& inst)
{
	out << "Instrument: " << inst.getName() << endl
			<< "\tmidi note number=" << inst.getNoteNumber() << endl
			<< "\tlevel=" << inst.getLevel() << endl
			<< "\tpan=" << inst.getPan() << endl
			<< "\tpitch=" << inst.getPitch() << endl
			;
			
	if(inst.isInSubmix())
		out << "\tsubmix=" << inst.getSubmixName() << endl;
		
	out	<< "\tLayers:" << endl;

	InstrumentLayerList layers = inst.getLayers();
	for(InstrumentLayerList::iterator e = layers.begin(); e < layers.end(); ++e)
		out << "\t" << *(*(e)) << endl;
		
	return out;
}

ostream& operator << (ostream& out, Submix& mix)
{
	out << "Submix: " << mix.getName() << endl
			<< "\tPorts:"
			;
			
	PortNameList ports = mix.getPortNames();
	for(PortNameList::iterator e = ports.begin(); e != ports.end(); ++e) {
		out << "\t" << *e;
	}
		
	return out;
}

//
// Note
//
ostream& operator << (ostream& out, Note& note)
{
	out << "Note: number=" << note.getNumber() << " velocity=" << note.getVelocity();
	return out;
}

void Note::finish()
{ 
	finished = true; 
	getInstrument()->setVolumes(0,0); 
}

void Note::cancel()
{ 
	finish(); 
	cancelled = true; 
}

void Note::set(Instrument * inst, Sample * samp, unsigned int velo, unsigned int num)
{
		instrument = inst;
		sample = samp;
		velocity = velo;
		number = num;
		finished = false;
		cancelled = false;
		samplePosition = 0.0f;
}

