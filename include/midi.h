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
// midi.h
// stuff pertaining to midi

#ifndef midi_h
#define midi_h

#include <string>
#include <vector>

using namespace std;

class MidiMessage
{
public:
	// TODO: Change to "Type"
	enum MidiMessageType {
		UNKNOWN,
		NOTE_ON,
		NOTE_OFF,
		POLYPHONIC_KEY_PRESSURE,
		CONTROL_CHANGE,
		PROGRAM_CHANGE,
		CHANNEL_PRESSURE,
		PITCH_WHEEL,
		SYSTEM_EXCLUSIVE,
		START,
		CONTINUE,
		STOP,
		SONG_POS
	};

	MidiMessageType type;
	int data1;
	int data2;
	int channel;

	MidiMessage()
		: type(UNKNOWN)
		, data1(-1)
		, data2(-1)
		, channel(-1)
	{}
};


struct MidiPortInfo
{
    string name;
	int client;
	int port;
};

// An interface for listening to midi messages.
struct IMIDIListener
{
	/// Process the incoming midi message.
	virtual void onMidiMessage(const MidiMessage& msg) = 0;

	virtual ~IMIDIListener() {}
};

// A list of port names
typedef vector<string> PortList;
// A list of IMIDIListeners
typedef vector<IMIDIListener*> MIDIListenerList;

class MidiDriver
{
public:
    MidiDriver() : active(false) {}
	virtual ~MidiDriver();

	string& getClientName() { return clientName; }

	string& getConnectPortName() { return connectPortName; }
	MidiDriver &setConnectPortName(const char *s) { connectPortName = s; return *this; }
	MidiDriver &setConnectPortName(string &s) { connectPortName = s; return *this; }

    virtual void open(string &clientName) = 0;
	virtual void close() = 0;
	virtual PortList getOutputPortList() = 0;
	virtual void connectPort(string &port, string &target) = 0;

    bool isActive() { return active; }
	void setActive(bool isActive) {	active = isActive;	}
	void handleMidiMessage(const MidiMessage& msg);

	MidiDriver& addMIDIListener(IMIDIListener* listener);
	
protected:
	string clientName;
	bool active;
	MIDIListenerList listeners;
	string connectPortName;
};

#endif // midi_h
