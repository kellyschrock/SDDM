/*
 *  Copyright (c) 2008 Kelly Schrock
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

#include "log.h"

#include <iostream>

LogFactory LogFactory::instance;

void ConsoleAppender::write(Appender::Mode mode, string & str)
{
	string prefix = "";
	bool stderr = false;
	
	switch(mode)
	{
		case Appender::Trace: stderr = false; prefix = "TRACE"; break;
		case Appender::Debug: stderr = false; prefix = "DEBUG"; break;
		case Appender::Info:  stderr = false; prefix = "INFO"; break;
		case Appender::Warn:  stderr = true; prefix = "WARN"; break;
		case Appender::Error: stderr = true; prefix = "ERROR"; break;
		case Appender::Fatal: stderr = true; prefix = "FATAL"; break;
	}
	
	ostream & out = (stderr)? cout: cerr;
	out << prefix << ":\t" << str << endl;
}

void testLog()
{
	// Instantiate a logger and attach it to the "factory" (this is a cheap logging system)
	Log log(new ConsoleAppender());
	LogFactory::instance.setLog(&log);
	
	LogPtr ptr = LogFactory::getLog("foo");
	
	LOG_TRACE(ptr, "this is a test" << " and " << 4+5 << "hey");
	LOG_DEBUG(ptr, "debug statement");
	LOG_INFO(ptr, "this is an info statement");
	LOG_WARN(ptr, "this is a warning");
	LOG_ERROR(ptr, "this is an error");
	LOG_FATAL(ptr, "this is fatal");
}

