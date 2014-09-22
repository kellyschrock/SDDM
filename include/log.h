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

#ifndef _log_h
#define _log_h

#include <string>
#include <sstream>

using namespace std;

class Appender {
public:
	enum Mode {Trace, Debug, Info, Warn, Error, Fatal};
	
	virtual void write(Mode mode, string & str) = 0;
};

class ConsoleAppender: public Appender {
public:
	virtual void write(Mode mode, string &);
};

class Log {
	Appender * appender;
public:
	Log(Appender * appr)
	: appender(appr) {}
	
	virtual ~Log() { if(this->appender) delete appender; }

	void trace(const char *c) { string s(c); trace(s); }
	void trace(string & str) { appender->write(Appender::Trace, str); }
	
	void debug(const char * c) { string s(c); debug(s); }
	void debug(string & str) { appender->write(Appender::Debug, str); }
	
	void info(const char * c) { string s(c); info(s); }
	void info(string & str) { appender->write(Appender::Info, str); }
	
	void warn(const char * c) { string s(c); warn(s); }
	void warn(string & str) { appender->write(Appender::Warn, str); }
	
	void error(const char * c) { string s(c); error(s); }
	void error(string & str) { appender->write(Appender::Error, str); }
	
	void fatal(const char * c) { string s(c); fatal(s); }
	void fatal(string & str) { appender->write(Appender::Fatal, str); }
	
	Appender * getAppender() { return appender; }
	void setAppender(Appender *a) { appender = a; }
};

typedef Log * LogPtr;

#ifdef OSTREAM_LOGGING
	#define LOG_TRACE(log, exp) cout << exp << endl
	#define LOG_DEBUG(log, exp)	cout << exp << endl
	#define LOG_INFO(log, exp)	cout << exp << endl
	#define LOG_WARN(log, exp)	cerr << exp << endl
	#define LOG_ERROR(log, exp)	cerr << exp << endl
	#define LOG_FATAL(log, exp)	cerr << exp << endl
#else
	#define LOG_TRACE(log, exp) { std::ostringstream _s; _s << exp; std::string _str = _s.str(); log->trace(_str); }
	#define LOG_DEBUG(log, exp) { std::ostringstream _s; _s << exp; std::string _str = _s.str(); log->debug(_str); }
	#define LOG_INFO(log, exp) { std::ostringstream _s; _s << exp; std::string _str = _s.str(); log->info(_str); }
	#define LOG_WARN(log, exp) { std::ostringstream _s; _s << exp; std::string _str = _s.str(); log->warn(_str); }
	#define LOG_ERROR(log, exp) { std::ostringstream _s; _s << exp; std::string _str = _s.str(); log->error(_str); }
	#define LOG_FATAL(log, exp) { std::ostringstream _s; _s << exp; std::string _str = _s.str(); log->fatal(_str); }
#endif

class LogFactory {
private:
	
	LogPtr log;
	
	LogFactory() {}
	~LogFactory() {}
	
public:
	static LogFactory instance;
	static LogPtr getLog(const char */*name*/) { return instance.log; }
	static void setLog(LogPtr p) { instance.log = p; }
};

#endif //_log_h
