/*
 *  Copyright (c) 2013 John Hammen
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

#ifndef NSMCLIENT_H
#define NSMCLIENT_H

#include "nonlib_nsm.h"
#include <string>
#include <QObject>

class NSMClient : public QObject
{
    Q_OBJECT

public:
    NSMClient() : nsm(0) {}
    ~NSMClient() {}
    bool connectToSession();
    void announce(char *argv0);
    void open(const char *, const char *, const char*);
    void save() { emit nsmSave(); }
    bool isActive() { return nsm != 0; }
    void setDirty() { nsm_send_is_dirty(nsm); }
    void setClean() { nsm_send_is_clean(nsm); }

signals:
    void nsmOpen(QString, QString, QString);
    void nsmSave();

private:
    nsm_client_t *nsm;
    QString clientName;
};

#endif // NSMCLIENT_H
