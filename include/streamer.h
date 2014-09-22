
#ifndef _streamer_h
#define _streamer_h

#include <iostream>


class insertee {
public:
	virtual void write(const char *) = 0;
	virtual void write(string &) = 0;
	virtual void write(int i) = 0;
	virtual void write(float f) = 0;
	virtual void write(ostream& ( *pf )(ostream&)) = 0;
};

class streamer: public std::ostream {
	insertee * target;
public:
	streamer(insertee * t)
	: target(t)
	{}
	
	void setInsertee(insertee* ins) {target = ins; }
	insertee * getInsertee() { return target; }
	
	streamer & operator << (const char * s)	{	return insert(s);	}
	streamer & operator << (int i) { return insert(i); }
	streamer & operator << (float f) { return insert(f); }
	streamer & operator << (ostream& ( *pf )(ostream&))	{		return insert(pf);	}
	
private:
	template <typename VT> streamer& insert(VT t)
	{
		target->write(t);
		return *this;
	}
};

extern streamer info;
extern streamer errstream;

#endif // _streamer_h
