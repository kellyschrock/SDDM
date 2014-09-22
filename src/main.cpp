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

#include <QApplication>
#include "mainwindow.h"

#include "log.h"
#include "app.h"

static LogPtr logger = 0;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // set up logging
    Log _log(new ConsoleAppender());
    LogFactory::setLog(&_log);
    logger = LogFactory::getLog(__FILE__);

    App app;
    MainWindow w(&app);
    app.start(argv[0]);
    w.show();

    return a.exec();
}
