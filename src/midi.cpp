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
#include <string>
#include <iostream>

using namespace std;

#include "midi.h"

MidiDriver::~MidiDriver()
{
}

MidiDriver& MidiDriver::addMIDIListener(IMIDIListener* listener)
{
	listeners.push_back(listener);
	return *this;
}

void MidiDriver::handleMidiMessage(const MidiMessage& msg)
{
	// Tell the midi listeners about the message.
	for(MIDIListenerList::iterator e = listeners.begin(); e != listeners.end(); ++e)
		(*(e))->onMidiMessage(msg);
}


