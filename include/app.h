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

#ifndef APP_H
#define APP_H

#include "alsamidi.h"
#include "audio_driver.h"
#include "nsmclient.h"
#include <QObject>

class App : public QObject, public IMIDIListener
{
    Q_OBJECT

public:
    void start(char *argv0);
    bool isInSession() { return nsmClient.isActive(); }
    void onMidiMessage(const MidiMessage &msg);
    bool loadFile(QString path);

public slots:
    void openSession(QString, QString, QString);
    void saveFile();

signals:
    void openNsm(QString, QString, QString);
    void midiNoteOn(int);

private:
    NSMClient nsmClient;
    AlsaMidiDriver midiDriver;
    JackAudioDriver jackDriver;
    QString fileLocation;

protected:
    bool openFile(QString fileName);
    void openDrivers(QString);
};

#endif // APP_H
