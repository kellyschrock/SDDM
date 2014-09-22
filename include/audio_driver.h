// audio_driver.h
// audio driver info
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

#ifndef audio_driver_h
#define audio_driver_h

#include <vector>
#include <string>
#include <map>

#include <jack/jack.h>

using namespace std;

/** A list of buffers */
typedef vector<float*> BufferList;

/** A request for a buffer, and application-supplied information about what to do with it */
struct BufferRequest {
public:
	typedef std::vector<string> PortNameList;

	BufferRequest(void* data)
	: data(data)
	{}

	BufferRequest(void* data, PortNameList portNames)
	: data(data)
	, portNames(portNames)
	{}

	PortNameList& getPortNames() { return portNames; }

	void* getData() { return data; }

private:
	void* data;
	PortNameList portNames;
};

typedef std::vector<BufferRequest*> BufferRequestList;

/** A response to a buffer request */
struct BufferResponse {
	typedef std::map<string, void*> BufferMap;

	BufferResponse(BufferRequest* req, unsigned frames)
	: request(req)
	, frames(frames)
	{}

	virtual ~BufferResponse();

	unsigned getFrames() { return frames; }
	BufferRequest* getRequest() { return request; }
	BufferMap& getBufferMap() { return buffers; }

	float* getLeft() { return left; }
	BufferResponse& setLeft(float* buf) { left = buf; return *this; }

	float* getRight() { return right; }
	BufferResponse& setRight(float* buf) { right = buf; return *this; }

	BufferResponse& add(string& portName, void* buffer)
	{
		buffers[portName] = buffer;
		return *this;
	}

private:
	BufferRequest* request;
	unsigned frames;
	BufferMap buffers;
	float* left;
	float* right;
};


/** A listener to audio events. */
struct IAudioListener {
	virtual ~IAudioListener() {}

	virtual BufferRequestList getBufferRequests() = 0;
	virtual void play(BufferResponse* response) = 0;
};

typedef std::vector<IAudioListener*> AudioListenerList;

/** An audio driver. */
class AudioDriver
{
public:
	typedef std::vector<string> ClientNameList;

    AudioDriver() : sampleRate(44100)
	{}

	virtual ~AudioDriver() {}

    virtual bool open(string &clientName) = 0;
	virtual void close() = 0;
	virtual ClientNameList getClientNames() = 0;

	int getSampleRate() { return sampleRate; }
	void addAudioListener(IAudioListener* listener) { listeners.push_back(listener); }

	virtual bool registerPort(string &port) = 0;
	virtual bool unregisterPort(string & portName) = 0;
	virtual bool hasRegisteredPort(string &port) = 0;

	string & getClientName() { return clientName; }

	static const char* LEFT_PORT_NAME;
	static const char* RIGHT_PORT_NAME;

	virtual bool connectPort(string & portName, string & target) = 0;
	virtual bool connectMainStereoOut(string &leftPortName, string &rightPortName) = 0;

protected:
	string clientName;
	int sampleRate;
	AudioListenerList listeners;
};

/** The Jack audio driver. */
class JackAudioDriver: public AudioDriver
{
public:
	typedef std::map<string, jack_port_t*> JackPortMap;

	virtual ~JackAudioDriver();

    virtual bool open(string &clientName);
	virtual void close();
	virtual bool registerPort(string &port);
	virtual bool unregisterPort(string & portName);
	virtual bool hasRegisteredPort(string &port);
	virtual ClientNameList getClientNames();

	virtual bool connectPort(string &portName, string &target);
	virtual bool connectMainStereoOut(string &leftPortName, string &rightPortName);

	virtual void onJackShutdown();
	virtual void onJackSampleRateChange(jack_nframes_t);
	virtual void onJackBufferSize(jack_nframes_t);

	// Our jack callbacks
	static int jackProcess(jack_nframes_t, void *);
	static int jackBufferSize(jack_nframes_t, void *);
	static int jackSampleRate(jack_nframes_t, void *);
	static void jackOff(void*);

	static jack_client_t* client;

	void process(jack_nframes_t frames);

protected:
	jack_port_t *right, *left;
	JackPortMap portMap;
};

#endif // audio_driver_h
