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

#include "app.h"

using namespace std;

#include "midi.h"
#include "model.h"
#include "sddm.h"
#include "nsmclient.h"


#include <iostream>
using namespace std;

void App::start(char *argv0)
{
    SDDM::instance = new SDDM();

    SDDM::instance->setMidiDriver(&midiDriver);
    midiDriver.addMIDIListener(SDDM::instance);
    midiDriver.addMIDIListener(this);

    SDDM::instance->setAudioDriver(&jackDriver);
    jackDriver.addAudioListener(SDDM::instance);

    // NSM - are we part of a session?
    bool nsmConnected = nsmClient.connectToSession();

    if(nsmConnected) {
        QObject::connect(&nsmClient, SIGNAL(nsmOpen(QString,QString,QString)),
                         this, SLOT(openSession(QString,QString,QString)));
        QObject::connect(&nsmClient, SIGNAL(nsmSave()), this, SLOT(saveSession()));
        nsmClient.announce(argv0);
    }
    else {
        openDrivers("SDDM");
    }
}

void App::openDrivers(QString clientId) {
    string clientName = clientId.toStdString();
    if(midiDriver.isActive()) {
        midiDriver.close();
        jackDriver.close();
    }
    // MIDI driver
    midiDriver.open(clientName);
    midiDriver.setActive(true);
    // Audio driver
    jackDriver.open(clientName);
}

bool App::openFile(QString fileName) {
    return SDDM::instance->loadKit(fileName.toLatin1(), false, -1, Configuration::SubmixNameList(), NULL);
}

void App::onMidiMessage(const MidiMessage &msg) {
    switch(msg.type)
    {
    case MidiMessage::NOTE_ON:
    {
        int velocity = msg.data2;
        if(velocity > 0) {
            emit midiNoteOn((velocity * 100) / 127);
        }
        break;
    }
    default:
        break;
    }
}

// called from GUI
bool App::loadFile(QString path)
{
    if(!openFile(path)) {
        return false;
    }
    if(this->isInSession()) {
        saveFile(); // save to current session-set location
    } else {
        this->fileLocation = path;
    }
    return true;
}

// called from NSMClient
void App::openSession(QString name, QString displayName, QString clientId) {
    fileLocation = name + ".xml";
    openDrivers(clientId);
    openFile(fileLocation);
    emit openNsm(fileLocation, displayName, clientId);
}


// called from NSMClient -or- GUI
void App::saveFile() {
    Configuration conf;
    conf.save(fileLocation.toLatin1(), SDDM::instance->getDrumkit());
}
