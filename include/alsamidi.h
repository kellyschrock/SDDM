// alsamidi.h
// of alsa, midi, etc.
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

#ifndef alsamidi_h
#define alsamidi_h

#include <alsa/asoundlib.h>
#include "midi.h"

class AlsaMidiDriver : public MidiDriver
{
public:
	virtual ~AlsaMidiDriver();

    virtual void open(string &clientName);
	virtual void close();
	virtual PortList getOutputPortList();
	virtual void connectPort(string &port, string &target);

	void midi_action(snd_seq_t *seq_handle);
	void getPortInfo(const std::string& sPortName, int& nClient, int& nPort);

private:
	void sysexEvent(unsigned char *midiBuffer, int nBytes);
};


#endif // alsamidi_h
